
#include "main.h"
#include "stm32f1xx_hal.h"




UART_HandleTypeDef huart1;   // 宣告 UART1 控制結構
uint8_t Serial_TxPacket[4];		//定义串口輸出的数据
char Serial_RxPacket[100];		//定义串口接收的数据
uint8_t Serial_RxFlag;		//定义串口接收的标志位变量
uint8_t Serial_packetFlag;		//定义串口接收的标志位变量
uint8_t Serial_RxData;
void USART_init(void){
	__HAL_RCC_USART1_CLK_ENABLE();   // 開啟 USART1 時鐘
	__HAL_RCC_GPIOA_CLK_ENABLE();    // 開啟 GPIOA 時鐘

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitTypeDef GPIO_RX = {0};
	GPIO_RX.Pin = GPIO_PIN_10;
	GPIO_RX.Mode = GPIO_MODE_INPUT;
	GPIO_RX.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &GPIO_RX);

	/* 初始化結構體 */
	huart1.Instance = USART1;                        // 指定外設
	huart1.Init.BaudRate = 9600;                     // 波特率
	huart1.Init.WordLength = UART_WORDLENGTH_8B;     // 字長 8 bits
	huart1.Init.StopBits = UART_STOPBITS_1;          // 停止位 1 bit
	huart1.Init.Parity = UART_PARITY_NONE;           // 無奇偶校驗
	huart1.Init.Mode = UART_MODE_TX_RX;                 // 啟用發送與接收
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;     // 無硬體流控
	huart1.Init.OverSampling = UART_OVERSAMPLING_16; // 16倍過採樣（常用）

	if (HAL_UART_Init(&huart1) != HAL_OK)
	    {
	        Error_Handler();
	    }


	HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(USART1_IRQn);  // ✔ 開啟 NVIC 的 USART1 IRQ
	HAL_UART_Receive_IT(&huart1, &Serial_RxData, 1);//當RDR有資料申請中斷，資料數據在中間的變數指標中
	//HAL_UART_Receive_IT = 開啟 RXNE 中斷 + 指定資料存放位置 + HAL 自動讀資料

}

void Serial_SendByte(uint8_t byte)
{
    HAL_UART_Transmit(&huart1, &byte, 1, HAL_MAX_DELAY);
}

/* 串口發送陣列 */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i++)
    {
        Serial_SendByte(Array[i]);
    }
}

/* 串口發送字串（以 '\0' 結尾） */
void Serial_SendString(char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        Serial_SendByte(String[i]);
    }
}

/* 次方函式，內部用 */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

/* 串口發送數字（補零到指定長度） */
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');
    }
}


void Serial_SendPacket(void){
	Serial_SendByte(0XFF);
	Serial_SendArray(Serial_TxPacket,4);
	Serial_SendByte(0XFE);
}



/**
  * 函    数：获取串口接收标志位
  * 参    数：无
  * 返 回 值：串口接收标志位，范围：0~1，接收到数据后，标志位置1，读取后标志位自动清零
  */
uint8_t Serial_GetRxFlag(void)
{
	if (Serial_RxFlag == 1)			//如果标志位为1
	{
		Serial_RxFlag = 0;
		return 1;					//则返回1，并自动清零标志位
	}
	return 0;						//如果标志位为0，则返回0
}


uint8_t Serial_GetpacketFlag(void)
{
	if (Serial_packetFlag== 1)			//如果标志位为1
	{
		Serial_packetFlag = 0;
		return 1;					//则返回1，并自动清零标志位
	}
	return 0;						//如果标志位为0，则返回0
}
/**
  * 函    数：获取串口接收的数据
  * 参    数：无
  * 返 回 值：接收的数据，范围：0~255
  */
 void Serial_GetRxData(void) //因為一次只有一個bit，所以跳出去沒差，下一次進來就是走1了
{

	if(Serial_GetRxFlag() == 1){
		static uint8_t state,i = 0;

		switch (state){
			case 0 :
				if (Serial_RxData == '@')state=1;
				break;
			case 1:
				if (Serial_RxData == '\r'){
					state=2;
				}else{
					Serial_RxPacket[i]=Serial_RxData;
					i++;
				}
				break;
			case 2:
				if (Serial_RxData=='\n'){
					state=0;
					Serial_RxPacket[i] = '\0';
					i=0;
					Serial_packetFlag=1;
				}
				break;

		}
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)   // 確認是 USART1
    {
        Serial_RxFlag = 1;
        Serial_GetRxData();


        // 很重要：再啟動下一次接收，才能一直收
        HAL_UART_Receive_IT(&huart1, &Serial_RxData, 1);//開頭一次跟callback裡也要這樣才能依值都有

    }
}
