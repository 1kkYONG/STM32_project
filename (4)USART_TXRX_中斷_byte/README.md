要使用Serial步驟
1. 開啟時鐘	
  __HAL_RCC_USART1_CLK_ENABLE();   // 開啟 USART1 時鐘
	__HAL_RCC_GPIOA_CLK_ENABLE();    // 開啟 GPIOA 時鐘
2. 要使用有USART功能的GPIO並且推薦TX設定成 GPIO_MODE_AF_PP  RX設定成 GPIO_MODE_INPUT
3. 初始化USART裡面的資訊EX:波特律,字長,停止位,奇偶校驗,硬體流控,採樣
4. 初始化HAL_UART_Init(&huart1)
5. 設定 NVIC 優先權
6. 開啟 NVIC 的 USART1 IRQ
7. 開啟 UART 的 RXNE 中斷（RDR 有資料時產生中斷）  
8. 當中斷發生時，CPU會跳到對應的 IRQ Handler（由 NVIC 查表決定）
9. 注意這裡開啟 UART 的 RXNE 中斷這裡還要放不然不會繼續接收

主要判斷的邏輯是靠 狀態旗標：

TXE（Transmit Data Register Empty）：表示 TDR 已空，可寫入新資料。

RXNE（Receive Data Register Not Empty）：表示 RDR 有新資料可讀。
