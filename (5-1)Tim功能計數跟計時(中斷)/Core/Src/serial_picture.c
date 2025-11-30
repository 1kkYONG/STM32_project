
#include "main.h"
#include "stm32f1xx_hal.h"
#include <string.h>




UART_HandleTypeDef huart2;   // 宣告 UART1 控制結構

//==============================
// 影像參數
//==============================

#define FRAME_WIDTH         128
#define FRAME_HEIGHT        96
#define FRAME_SIZE          (FRAME_WIDTH * FRAME_HEIGHT)   // 12288 bytes

// 每片 payload 最大長度（不含 header/start/end）
#define FRAG_PAYLOAD_MAX    256

// 分片封包長度（最大值）：Start(1) + seq(2) + total(2) + len(2) + payload + End(1)
#define PACKET_OVERHEAD     1 + 2 + 2 + 2 + 1
#define PACKET_MAX_SIZE     (PACKET_OVERHEAD + FRAG_PAYLOAD_MAX)

//==============================
// 影像緩衝區 & TX Buffer
//==============================

static uint8_t frame_buffer[FRAME_SIZE];  //為什麼不是給PACKET_MAX_SIZE?
static uint8_t tx_packet[PACKET_MAX_SIZE];

//==============================
// 健檢指標
//==============================

volatile uint32_t g_frame_count         = 0;    // 已送出的幀數
volatile uint32_t g_last_frame_time_ms  = 0;    // 最近一幀耗時
volatile uint32_t g_max_frame_time_ms   = 0;    // 目前最大耗時
volatile uint32_t g_total_bytes_sent    = 0;    // 累積傳送 bytes
volatile uint32_t g_uart_error_count    = 0;    // UART錯誤次數（可另外在錯誤回呼增加）

//==============================
// 小工具：16-bit 高低位
//==============================

static inline uint8_t hi8(uint16_t v) { return (uint8_t)((v >> 8) & 0xFF); }
static inline uint8_t lo8(uint16_t v) { return (uint8_t)(v & 0xFF); }

void serial_init(void){
	__HAL_RCC_USART1_CLK_ENABLE();   // 開啟 USART1 時鐘
	__HAL_RCC_GPIOA_CLK_ENABLE();    // 開啟 GPIOA 時鐘

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* 初始化結構體 */
	huart2.Instance = USART1;                        // 指定外設
	huart2.Init.BaudRate = 115200;                     // 波特率
	huart2.Init.WordLength = UART_WORDLENGTH_8B;     // 字長 8 bits
	huart2.Init.StopBits = UART_STOPBITS_1;          // 停止位 1 bit
	huart2.Init.Parity = UART_PARITY_NONE;           // 無奇偶校驗
	huart2.Init.Mode = UART_MODE_TX_RX;                 // 啟用發送與接收
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;     // 無硬體流控
	huart2.Init.OverSampling = UART_OVERSAMPLING_16; // 16倍過採樣（常用）

	if (HAL_UART_Init(&huart2) != HAL_OK)
	    {
	        Error_Handler();
	    }

}

//==============================
// 生成假影像（128x96 GRAY8）
// 這裡做一個「橫向漸層 + 每 8 行反轉」方便肉眼確認
//==============================

void Image_GenerateTestPattern(void)
{
    for (uint16_t y = 0; y < FRAME_HEIGHT; y++)
    {
        for (uint16_t x = 0; x < FRAME_WIDTH; x++)
        {
            uint8_t base = (uint8_t)((x * 255) / (FRAME_WIDTH - 1)); // 左黑右白漸層

            // 每 8 行做反轉：明暗交錯條紋
            if ((y / 8) % 2)  //「用每 8 行」分成一組,在每一組改變
            {
                base = 255 - base;
            }

            frame_buffer[y * FRAME_WIDTH + x] = base;
        }
    }
}

//==============================
// 傳送一整幀 (frame_buffer) 的分片函式
//==============================

HAL_StatusTypeDef Image_SendFrame_UART(void)
{
    uint32_t start_tick = HAL_GetTick(); //從 MCU 啟動到現在所經過的「毫秒數」

    uint32_t remaining = FRAME_SIZE;
    uint32_t offset    = 0;

    // 計算總片數
    uint16_t total_fragments = (FRAME_SIZE + FRAG_PAYLOAD_MAX - 1) / FRAG_PAYLOAD_MAX; //有小數點就進位,因為C語言中有小數點的會直接被捨棄

    for (uint16_t seq = 0; seq < total_fragments; seq++)
    {
        uint16_t frag_len = (remaining > FRAG_PAYLOAD_MAX) ? FRAG_PAYLOAD_MAX : (uint16_t)remaining;

        uint16_t packet_len = 0;

        // Start byte
        tx_packet[packet_len++] = 0xFF;

        // 序號（seq）: 0 ~ total_fragments-1
        tx_packet[packet_len++] = hi8(seq);
        tx_packet[packet_len++] = lo8(seq);

        // 總片數（total_fragments）
        tx_packet[packet_len++] = hi8(total_fragments);
        tx_packet[packet_len++] = lo8(total_fragments);

        // 本片 payload 長度
        tx_packet[packet_len++] = hi8(frag_len);
        tx_packet[packet_len++] = lo8(frag_len);

        // payload
        memcpy(&tx_packet[packet_len], &frame_buffer[offset], frag_len);
        packet_len += frag_len;

        // End byte
        tx_packet[packet_len++] = 0xFE;

        // 送出
        HAL_StatusTypeDef status = HAL_UART_Transmit(&huart2, tx_packet, packet_len, HAL_MAX_DELAY);
        if (status != HAL_OK)
        {
            g_uart_error_count++;
            return status;
        }

        offset    += frag_len;
        remaining -= frag_len;
        g_total_bytes_sent += packet_len;
    }

    uint32_t end_tick = HAL_GetTick();
    uint32_t diff = end_tick - start_tick;

    g_last_frame_time_ms = diff;
    if (diff > g_max_frame_time_ms)
    {
        g_max_frame_time_ms = diff;
    }

    g_frame_count++;

    return HAL_OK;
}


