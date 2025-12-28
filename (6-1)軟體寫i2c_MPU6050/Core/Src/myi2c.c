#include "stm32f1xx_hal.h"

// 初始化 DWT 計數器
static void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // 開啟 Trace 輔助功能
    DWT->CYCCNT = 0;                                // 計數器歸零
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;           // 開啟計數器
}


// 精準微秒延時
static void Delay_us(uint32_t us) {
    uint32_t startTick = DWT->CYCCNT;
    // F103 的 SystemCoreClock 通常是 72000000 (72MHz)
    // 每 1us 的 Ticks 數 = SystemCoreClock / 1000000
    uint32_t delayTicks = us * (SystemCoreClock / 1000000);

    while ((DWT->CYCCNT - startTick) < delayTicks);
}

void MyI2C_W_SCL(uint8_t BitValue)
{
    // 使用三元運算子，確保非 0 即 1
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, BitValue ? GPIO_PIN_SET : GPIO_PIN_RESET);
    Delay_us(10);
}

void MyI2C_W_SDA(uint8_t BitValue)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, BitValue ? GPIO_PIN_SET : GPIO_PIN_RESET);
    Delay_us(10);
}

uint8_t MyI2C_R_SDA(void)
{
    GPIO_PinState bit_status;

    // HAL 庫的讀取函式
    // 注意：GPIO_Pin_11 改為 GPIO_PIN_11 (HAL庫是大寫 PIN)
    bit_status = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11);

    Delay_us(10); // 這裡假設你已經有寫好微秒延遲函式

    return (uint8_t)bit_status; // 將枚舉類型轉回 uint8_t (0 或 1)
}


void MyI2C_Init(void){

	__HAL_RCC_GPIOB_CLK_ENABLE();    // 開啟 GPIOB 時鐘

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; // 開漏模式
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10 | GPIO_PIN_11, GPIO_PIN_SET); //一開始都先放手(HIGH)
	DWT_Init();
}

//基礎6大組件
void MyI2C_Start(void)
{
	MyI2C_W_SDA(1);							//释放SDA，确保SDA为高电平
	MyI2C_W_SCL(1);							//释放SCL，确保SCL为高电平
	MyI2C_W_SDA(0);							//在SCL高电平期间，拉低SDA，产生起始信号
	MyI2C_W_SCL(0);							//起始后把SCL也拉低，即为了占用总线，也为了方便总线时序的拼接
}


void MyI2C_Stop(void)
{
	MyI2C_W_SDA(0);							//拉低SDA，确保SDA为低电平
	MyI2C_W_SCL(1);							//释放SCL，使SCL呈现高电平
	MyI2C_W_SDA(1);							//在SCL高电平期间，释放SDA，产生终止信号
}

void MyI2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i ++)				//循环8次，主机依次发送数据的每一位
	{
		/*两个!可以对数据进行两次逻辑取反，作用是把非0值统一转换为1，即：!!(0) = 0，!!(非0) = 1*/
		MyI2C_W_SDA(!!(Byte & (0x80 >> i)));//使用掩码的方式取出Byte的指定一位数据并写入到SDA线
		MyI2C_W_SCL(1);						//释放SCL，从机在SCL高电平期间读取SDA
		MyI2C_W_SCL(0);						//拉低SCL，主机开始发送下一位数据
	}
}

uint8_t MyI2C_ReceiveByte(void)
{
	uint8_t i ,Byte = 0x00;
	MyI2C_W_SDA(1);
	for (i=0 ; i<8 ; i++){

		MyI2C_W_SCL(1);
		if (MyI2C_R_SDA()) Byte |= (0x80 >> i);
		MyI2C_W_SCL(0);
	}
	return Byte;
}

void MyI2C_SendAck(uint8_t AckBit)
{
	MyI2C_W_SDA(AckBit);					//主机把应答位数据放到SDA线
	MyI2C_W_SCL(1);							//释放SCL，从机在SCL高电平期间，读取应答位
	MyI2C_W_SCL(0);							//拉低SCL，开始下一个时序模块
}

uint8_t MyI2C_ReceiveAck(void)
{
	uint8_t AckBit;
	MyI2C_W_SDA(1);
	MyI2C_W_SCL(1);
	AckBit = MyI2C_R_SDA();
	MyI2C_W_SCL(0);							//拉低SCL，开始下一个时序模块
	return AckBit;
}

