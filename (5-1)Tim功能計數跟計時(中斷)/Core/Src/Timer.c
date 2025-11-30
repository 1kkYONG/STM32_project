#include "main.h"

TIM_HandleTypeDef htim2;   // TIM2 的控制結構

/**
  * 函式名稱：Timer_Init
  * 功能說明：設定 TIM2 做基本計時，並啟用更新中斷
  * 參數說明：無
  * 回傳值  ：無
  */

uint16_t Num=0;
void Timer_Init(void)
{
    /* 1. 開啟 TIM2 時鐘 */
    __HAL_RCC_TIM2_CLK_ENABLE();   // 啟用 APB1 上的 TIM2 時鐘

    /* 2. 設定時基單元（對應以前的 TimeBaseInit） */
    htim2.Instance = TIM2;                         // 選擇 TIM2
    htim2.Init.Prescaler         = 1 - 1;       // 預分頻器 PSC
                                                   // 72MHz / 7200 = 10kHz
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;         // 向上計數
    htim2.Init.Period            = 10 - 1;      // 自動重裝值 ARR
                                                   // 10kHz / 10000 = 1Hz (1 秒一次)
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;     // 時鐘不再額外分頻
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; // 不使用預裝載

    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        // 這邊可以放錯誤處理，例如點亮錯誤 LED
    }

    /* GPIO PA0：上拉輸入（給 TIM2 ETR） */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 外部時鐘模式 2：用 ETR 當 TIM2 時鐘 */
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    sClockSourceConfig.ClockSource    = TIM_CLOCKSOURCE_ETRMODE2;
    sClockSourceConfig.ClockPolarity  = TIM_CLOCKPOLARITY_NONINVERTED;
    sClockSourceConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
    sClockSourceConfig.ClockFilter    = 0x0F;
    HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig);

    /* 4. 先清除更新旗標，避免一啟用中斷就馬上進一次
    	a.初始化 TIM 的時候，HAL 會打一次更新事件，讓 PSC/ARR 的設定真正載入，所以 UIF 可能先被設成 1。

    	b.我如果直接 HAL_TIM_Base_Start_IT() 開中斷，因為 UIF 已經是 1，所以一開始就會先進一次中斷。

    	c.如果我想「第一個中斷 = 第一秒」，就要在 Start_IT 之前先用__HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE); 把舊的 UIF 清掉。*/

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);


    /* 5. NVIC 中斷分組設定
     *    整個專案只需要設定一次，如果你已經在別的地方呼叫過
     *    HAL_NVIC_SetPriorityGrouping()，這行就可以省略。
     */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_2); // 搶占優先權 0~3、次優先權 0~3

    /* 6. 設定 NVIC 針對 TIM2 的中斷 */
    HAL_NVIC_SetPriority(TIM2_IRQn, 2, 1);  // 搶占優先權 2，次優先權 1
    HAL_NVIC_EnableIRQ(TIM2_IRQn);          // 開啟 TIM2 中斷線

    /* 7. 啟動 TIM2 並開啟更新中斷功能 */
    HAL_TIM_Base_Start_IT(&htim2);          // 啟動計時器 + 允許 Update 中斷
}



uint16_t Timer_GetCounter(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2);
}
/* ===================== 中斷服務函式 & 回呼 ===================== */


/**
  * 計時器更新事件回呼函式
  * 每次 TIM2 溢位（更新事件）時，HAL_TIM_IRQHandler 會呼叫到這裡
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)   // 確認是 TIM2 觸發的中斷
    {
        Num++;
    }
}
