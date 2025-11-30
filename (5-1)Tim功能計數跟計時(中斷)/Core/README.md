TIM2 筆記：內部時鐘與外部時鐘重點整理1. 內部時鐘模式（基本定時器用法）使用內部時鐘時，Timer 的更新頻率公式如下：$$\text{Update Frequency} = \frac{\text{TimerClock}}{(\text{PSC} + 1) \times (\text{ARR} + 1)}$$實作範例硬體環境： STM32F103，TIM2 掛載於 APB1（TimerClock $\approx$ 72 MHz）。目標設定： 1 秒中斷一次（1 Hz）。Chtim2.Instance = TIM2;
htim2.Init.Prescaler         = 7200 - 1;       // PSC = 7199 (除以 7200)
htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
htim2.Init.Period            = 10000 - 1;      // ARR = 9999 (數 10000 下)
htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
HAL_TIM_Base_Init(&htim2);
計算過程分頻後頻率：$72\text{ MHz} \div 7200 = 10\text{ kHz}$溢位時間：$10\text{ kHz} \div 10000 = 1\text{ Hz}$ (每秒溢位一次)為什麼設定值都要 -1？硬體暫存器的運作邏輯如下：PSC (Prescaler)：實際除數為 PSC + 1。想除以 7200，PSC 需設 7200 - 1。ARR (Auto-Reload Register)：CNT 從 0 數到 ARR，共經歷 ARR + 1 個數。想數 10000 下，ARR 需設 10000 - 1。2. 更新事件與中斷機制 (UIF)運作流程在向上計數模式 (Up-counting) 下：CNT 變化：$0 \rightarrow 1 \rightarrow \dots \rightarrow \text{ARR}$溢位瞬間：下一個 Clock 回到 0，產生 更新事件 (Update Event)。旗標設定：SR 暫存器中的 UIF (Update Interrupt Flag) 變為 1。中斷觸發條件若同時滿足以下兩點，將進入 TIM2_IRQHandler：開啟更新中斷 (DIER.UIE = 1)NVIC 啟用 (EnableIRQ(TIM2_IRQn))HAL 庫回調函式：Cvoid HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        Num++;   // 每次溢位 +1
    }
}
⚠️ 重要：啟動前先清除 UIF在初始化 Timer (HAL_TIM_Base_Init) 時，HAL 庫通常會觸發一次「軟體更新事件」以載入 PSC/ARR 設定，這會導致 UIF = 1。若直接啟動中斷：現象：一啟動就會立刻進一次中斷（即使還沒真正數完第一輪）。解法：啟動前手動清除 Flag。C__HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE); // 先清旗標
HAL_TIM_Base_Start_IT(&htim2);                 // 再啟動
3. TIM2 計數來源比較3-1. 內部時鐘 (Internal Clock)用途：一般定時任務。來源：APB Bus TimerClock。CTIM_ClockConfigTypeDef sClockSourceConfig = {0};
sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL; // 預設值，可省略
HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig);
3-2. 外部時鐘 (External Clock - ETR Mode 2)用途：硬體計數器（紅外線計數、脈波輸入）。腳位：TIM2 的 ETR 預設在 PA0。設定步驟：GPIO 初始化 (PA0)：C__HAL_RCC_GPIOA_CLK_ENABLE();
GPIO_InitTypeDef GPIO_InitStruct = {0};
GPIO_InitStruct.Pin   = GPIO_PIN_0;
GPIO_InitStruct.Mode  = GPIO_MODE_INPUT; // 輸入模式
GPIO_InitStruct.Pull  = GPIO_PULLUP;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
Timer 設定 (ETR Mode)：C// 基本設定：每 10 個脈波觸發一次中斷
htim2.Instance = TIM2;
htim2.Init.Prescaler = 0;       // 不分頻
htim2.Init.Period    = 10 - 1;  // 每 10 下溢位
HAL_TIM_Base_Init(&htim2);

// 設定外部時鐘源
TIM_ClockConfigTypeDef sClockSourceConfig = {0};
sClockSourceConfig.ClockSource    = TIM_CLOCKSOURCE_ETRMODE2;      // ETR 模式
sClockSourceConfig.ClockPolarity  = TIM_CLOCKPOLARITY_NONINVERTED; // 不反相
sClockSourceConfig.ClockFilter    = 0x0F;                          // 開啟最大濾波
HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig);
結果：每收到 10 個外部脈波 $\rightarrow$ 觸發更新事件 $\rightarrow$ Num++。4. 關鍵參數詳解4-1. ClockDivisionChtim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
功能：影響輸入濾波器採樣頻率的分頻。設定建議：純定時任務：DIV1 (無影響)。外部輸入/編碼器模式且需要濾波時：才需考慮調整。4-2. AutoReloadPreload (ARR 預裝載)決定修改 ARR 值時的生效時機：設定值機制適用場景DISABLE不使用影子暫存器寫入 ARR 立即生效。適合初始化一次設定好就不動。ENABLE使用影子暫存器寫入 ARR 先存入緩衝，等下一次更新事件才生效。適合動態調整 PWM 週期或頻率，避免波形錯亂。5. 外部時鐘濾波設定 (ClockFilter)僅在外部 Clock (ETR) 或輸入捕捉時有效。CsClockSourceConfig.ClockPolarity  = TIM_CLOCKPOLARITY_NONINVERTED;
sClockSourceConfig.ClockFilter    = 0x0F;
ClockPolarity：NONINVERTED：正邏輯（High 為 High）。INVERTED：反相（High 視為 Low）。ClockFilter (0x00 ~ 0x0F)：0x00：不濾波，任何邊緣都計數。0x0F：最大濾波，訊號需穩定一段時間才視為有效。作用：硬體抗雜訊、防抖動（Debounce）。6. 應用場景總結：GPIO vs Timer在「讀取外部訊號」時，該選哪種方式？6-1. 一般按鈕 (低頻)特性：人手按壓速度慢，不需要極高精確度。推薦方案：GPIO + EXTI + 軟體防抖。優點：不占用寶貴的 Timer 資源。程式邏輯簡單直觀。軟體防抖範例：Cif (now - last_key_tick > 20) { /* 視為有效 */ }
6-2. 高速/雜訊訊號 (高頻)特性：編碼器、流量計、霍爾感測器、紅外線計數。推薦方案：Timer 外部時鐘 (ETR) + Hardware Filter。優點：硬體濾波 (ClockFilter)：自動濾除毛刺與 EMI 雜訊。不佔用 CPU：計數由硬體完成，溢位才通知 CPU。精確：不會因為中斷頻繁而遺漏計數。
