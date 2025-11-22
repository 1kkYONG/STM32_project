#include "stm32f1xx_hal.h"

/**
  * 函    數：LED 初始化（HAL 版本）
  * 參    數：無
  * 返 回 值：無
  */
void LED_Init(void)
{
    /* 開啟 GPIOA 時鐘 */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* GPIO 初始化 */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_1 | GPIO_PIN_2;   // PA1、PA2
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;       // 推挽輸出
    GPIO_InitStruct.Pull  = GPIO_NOPULL;               // 不上拉、不下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;      // 高速
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 預設輸出高電位（LED 熄燈的那種接法）*/
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_SET);
}

/**
  * 函    數：LED1 開啟
  */
void LED1_ON(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);   // PA1 = 0
}

/**
  * 函    數：LED1 關閉
  */
void LED1_OFF(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     // PA1 = 1
}

/**
  * 函    數：LED1 狀態翻轉
  */
void LED1_Turn(void)
{
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);                  // 直接用 HAL Toggle
}

/**
  * 函    數：LED2 開啟
  */
void LED2_ON(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);   // PA2 = 0
}

/**
  * 函    數：LED2 關閉
  */
void LED2_OFF(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);     // PA2 = 1
}

/**
  * 函    數：LED2 狀態翻轉
  */
void LED2_Turn(void)
{
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_2);                  // 直接用 HAL Toggle
}
