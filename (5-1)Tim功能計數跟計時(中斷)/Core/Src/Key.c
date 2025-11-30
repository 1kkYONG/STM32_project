#include "stm32f1xx_hal.h"

/**
  * 函    數：按鍵初始化（HAL 版本）
  */
void Key_Init(void)
{
    /* 開啟 GPIOB 時鐘 */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 設定 PB1、PB11 為 上拉輸入 */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = GPIO_PIN_1 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;     // 輸入模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;         // 上拉
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
  * 函    數：讀取按鍵鍵碼（阻塞式 + HAL_Delay 消抖）
  * 返回值：0~2，0 表示沒有按鍵按下
  */
uint8_t Key_GetNum(void)
{
    uint8_t KeyNum = 0;

    /* PB1 == 0 → 按鍵 1 按下 */
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET)
    {
        HAL_Delay(20);   // 消抖
        while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET);
        HAL_Delay(20);   // 消抖
        KeyNum = 1;
    }

    /* PB11 == 0 → 按鍵 2 按下 */
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_RESET)
    {
        HAL_Delay(20);   // 消抖
        while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_RESET);
        HAL_Delay(20);   // 消抖
        KeyNum = 2;
    }

    return KeyNum;
}
