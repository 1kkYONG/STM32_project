/*************************************************************
 * 轉換說明：
 * 本檔將原 Keil5（StdPeriph）版本改為 STM32CubeIDE（HAL）版。
 * 腳位不變：PB8 -> SCL、PB9 -> SDA（開漏輸出）。
 * 仍採用「軟體 I2C」位元敲擊（bit-bang），不用 HAL_I2C。
 *
 * 需搭配你的字型檔：OLED_Font.h（含 OLED_F8x16 陣列）。
 *************************************************************/

#include "stm32f1xx_hal.h"
#include "OLED_Font.h"

/*========================= 腳位/巨集 =========================
  原版：
    #define OLED_W_SCL(x) GPIO_WriteBit(GPIOB, GPIO_Pin_8, (BitAction)(x))
    #define OLED_W_SDA(x) GPIO_WriteBit(GPIOB, GPIO_Pin_9, (BitAction)(x))
  HAL 版改為：
    使用 HAL_GPIO_WritePin()；仍保留同名巨集，呼叫點不用改。
================================================================*/
#define OLED_W_SCL(x)  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define OLED_W_SDA(x)  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)

/*（可選）軟性延遲：軟體 I2C 有時需要些微延遲以確保時序穩定。
   若你不需要延遲，可將迴圈次數改為 0。*/
static void OLED_SoftDelay(void)
{
    /* 簡單 NOP 迴圈，不使用 HAL_Delay（HAL_Delay 是毫秒等級） */
    for (volatile int d = 0; d < 20; d++)
    {
        __NOP();
    }
}

/*========================= 引腳初始化 =========================
  * @brief  I2C 引腳初始化（PB8=SCL、PB9=SDA，開漏輸出）
  * @param  無
  * @retval 無
================================================================*/
void OLED_I2C_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStructure = {0};

    /* PB8：SCL = 開漏輸出，50MHz 等效速度 */
    GPIO_InitStructure.Pin   = GPIO_PIN_8;
    GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_OD;   // 開漏輸出（對應原 GPIO_Mode_Out_OD）
    GPIO_InitStructure.Pull  = GPIO_NOPULL;           // 外接上拉（硬體上通常有 4.7k~10k）
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;  // 50MHz 等級
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* PB9：SDA = 開漏輸出 */
    GPIO_InitStructure.Pin = GPIO_PIN_9;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 線路預設為空閒（高電位） */
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

/*=========================== I2C 開始 ==========================
  * @brief  I2C 開始條件
  * @param  無
  * @retval 無
================================================================*/
void OLED_I2C_Start(void)
{
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_SoftDelay();

    OLED_W_SDA(0);
    OLED_SoftDelay();

    OLED_W_SCL(0);
    OLED_SoftDelay();
}

/*=========================== I2C 停止 ==========================
  * @brief  I2C 停止條件
  * @param  無
  * @retval 無
================================================================*/
void OLED_I2C_Stop(void)
{
    OLED_W_SDA(0);
    OLED_SoftDelay();

    OLED_W_SCL(1);
    OLED_SoftDelay();

    OLED_W_SDA(1);
    OLED_SoftDelay();
}

/*======================== I2C 發送一個字節 ======================
  * @brief  I2C 發送一個字節（不處理 ACK）
  * @param  Byte 要發送的 1 個字節
  * @retval 無
================================================================*/
void OLED_I2C_SendByte(uint8_t Byte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        OLED_W_SDA( !!(Byte & (0x80 >> i)) );
        OLED_SoftDelay();

        OLED_W_SCL(1);
        OLED_SoftDelay();

        OLED_W_SCL(0);
        OLED_SoftDelay();
    }

    /* 額外打一個時鐘，不處理應答訊號（維持與原程式相同行為） */
    OLED_W_SCL(1);
    OLED_SoftDelay();
    OLED_W_SCL(0);
    OLED_SoftDelay();
}

/*=========================== OLED 寫命令 ========================
  * @brief  OLED 寫入命令
  * @param  Command 要寫入的命令
  * @retval 無
================================================================*/
void OLED_WriteCommand(uint8_t Command)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);   // 從機地址（SSD1306 常見為 0x78/0x7A，依面板而定）
    OLED_I2C_SendByte(0x00);   // 控制位元：寫命令
    OLED_I2C_SendByte(Command);
    OLED_I2C_Stop();
}

/*=========================== OLED 寫資料 ========================
  * @brief  OLED 寫入資料
  * @param  Data 要寫入的資料
  * @retval 無
================================================================*/
void OLED_WriteData(uint8_t Data)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);   // 從機地址
    OLED_I2C_SendByte(0x40);   // 控制位元：寫資料
    OLED_I2C_SendByte(Data);
    OLED_I2C_Stop();
}

/*========================= 設定游標位置 ========================
  * @brief  OLED 設定游標位置
  * @param  Y 以左上角為原點，向下方向的座標，範圍：0~7
  * @param  X 以左上角為原點，向右方向的座標，範圍：0~127
  * @retval 無
================================================================*/
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    OLED_WriteCommand(0xB0 | Y);                   // 設定 Y 位置（頁）
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));   // 設定 X 高 4 位
    OLED_WriteCommand(0x00 | (X & 0x0F));          // 設定 X 低 4 位
}

/*============================= 清屏 ============================
  * @brief  OLED 清屏
  * @param  無
  * @retval 無
================================================================*/
void OLED_Clear(void)
{
    for (uint8_t j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        for (uint8_t i = 0; i < 128; i++)
        {
            OLED_WriteData(0x00);
        }
    }
}

/*========================== 顯示單一字元 ========================
  * @brief  OLED 顯示一個字元（8x16）
  * @param  Line   行位置，範圍：1~4
  * @param  Column 列位置，範圍：1~16
  * @param  Char   要顯示的字元（ASCII 可見字元）
  * @retval 無
================================================================*/
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i;

    // 上半部
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[(uint8_t)(Char - ' ')][i]);
    }

    // 下半部
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[(uint8_t)(Char - ' ')][i + 8]);
    }
}

/*========================== 顯示字串 ===========================
  * @brief  OLED 顯示字串
  * @param  Line   起始行位置，範圍：1~4
  * @param  Column 起始列位置，範圍：1~16
  * @param  String 要顯示的字串（ASCII 可見字元）
  * @retval 無
================================================================*/
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    for (uint8_t i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

/*=========================== 乘冪函式 ==========================
  * @brief  OLED 次方函式
  * @retval 回傳值等於 X 的 Y 次方
================================================================*/
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

/*======================= 顯示數字（無號） ======================
  * @brief  OLED 顯示十進位正數
  * @param  Line   起始行位置：1~4
  * @param  Column 起始列位置：1~16
  * @param  Number 數值範圍：0~4294967295
  * @param  Length 顯示長度：1~10
  * @retval 無
================================================================*/
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    for (uint8_t i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i,
                      (char)(Number / OLED_Pow(10, Length - i - 1) % 10 + '0'));
    }
}

/*======================= 顯示數字（有號） ======================
  * @brief  OLED 顯示十進位帶符號數
  * @param  Line   起始行位置：1~4
  * @param  Column 起始列位置：1~16
  * @param  Number 數值範圍：-2147483648 ~ 2147483647
  * @param  Length 顯示長度：1~10（不含正負號）
  * @retval 無
================================================================*/
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint32_t Number1;
    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = (uint32_t)Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = (uint32_t)(-Number);
    }

    for (uint8_t i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i + 1,
                      (char)(Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0'));
    }
}

/*======================= 顯示十六進位數 =======================
  * @brief  OLED 顯示十六進位正數
  * @param  Line   起始行位置：1~4
  * @param  Column 起始列位置：1~16
  * @param  Number 數值範圍：0~0xFFFFFFFF
  * @param  Length 顯示長度：1~8
  * @retval 無
================================================================*/
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t SingleNumber;
    for (uint8_t i = 0; i < Length; i++)
    {
        SingleNumber = (uint8_t)(Number / OLED_Pow(16, Length - i - 1) % 16);
        if (SingleNumber < 10)
        {
            OLED_ShowChar(Line, Column + i, (char)(SingleNumber + '0'));
        }
        else
        {
            OLED_ShowChar(Line, Column + i, (char)(SingleNumber - 10 + 'A'));
        }
    }
}

/*======================== 顯示二進位數 =========================
  * @brief  OLED 顯示二進位正數
  * @param  Line   起始行位置：1~4
  * @param  Column 起始列位置：1~16
  * @param  Number 數值範圍：0~1111 1111 1111 1111
  * @param  Length 顯示長度：1~16
  * @retval 無
================================================================*/
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    for (uint8_t i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i,
                      (char)(Number / OLED_Pow(2, Length - i - 1) % 2 + '0'));
    }
}

/*=========================== OLED 初始化 =======================
  * @brief  OLED 初始化流程（SSD1306 常見設定）
  * @param  無
  * @retval 無
================================================================*/
void OLED_Init(void)
{
    /* 上電延遲（原程式為雙重 for 迴圈），此處改為 100ms */
    HAL_Delay(100);

    OLED_I2C_Init();           // 端口初始化

    OLED_WriteCommand(0xAE);   // 關閉顯示

    OLED_WriteCommand(0xD5);   // 設定顯示時鐘分頻比/振盪器頻率
    OLED_WriteCommand(0x80);

    OLED_WriteCommand(0xA8);   // 設定多路複用率
    OLED_WriteCommand(0x3F);

    OLED_WriteCommand(0xD3);   // 設定顯示偏移
    OLED_WriteCommand(0x00);

    OLED_WriteCommand(0x40);   // 設定顯示起始行

    OLED_WriteCommand(0xA1);   // 左右方向：0xA1 正常、0xA0 左右反置

    OLED_WriteCommand(0xC8);   // 上下方向：0xC8 正常、0xC0 上下反置

    OLED_WriteCommand(0xDA);   // 設定 COM 腳硬體配置
    OLED_WriteCommand(0x12);

    OLED_WriteCommand(0x81);   // 設定對比度
    OLED_WriteCommand(0xCF);

    OLED_WriteCommand(0xD9);   // 預充電週期
    OLED_WriteCommand(0xF1);

    OLED_WriteCommand(0xDB);   // VCOMH 取消選擇等級
    OLED_WriteCommand(0x30);

    OLED_WriteCommand(0xA4);   // 整個顯示開/關（保留 RAM 內容）

    OLED_WriteCommand(0xA6);   // 正常/反白顯示

    OLED_WriteCommand(0x8D);   // 充電幫浦
    OLED_WriteCommand(0x14);

    OLED_WriteCommand(0xAF);   // 開啟顯示

    OLED_Clear();              // 清屏
}
