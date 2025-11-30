#ifndef Serial_picture_h
#define Serial_picture_h

#include "stm32f1xx_hal.h"

//==============================
// 對外函式宣告
//==============================

// 初始化 UART（你 faker_image.c 裡的函式）
void serial_init(void);

// 生成一張 128×96 的灰階假影像
void Image_GenerateTestPattern(void);

// 傳送一整幀影像（分片處理 + UART 傳送）
HAL_StatusTypeDef Image_SendFrame_UART(void);


//==============================
// 對外可用的健康指標（如果 main.c 想印出）
//==============================

extern volatile uint32_t g_frame_count;
extern volatile uint32_t g_last_frame_time_ms;
extern volatile uint32_t g_max_frame_time_ms;
extern volatile uint32_t g_total_bytes_sent;
extern volatile uint32_t g_uart_error_count;

#endif // Serial_picture_h
