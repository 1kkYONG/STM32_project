#include "stm32f1xx_hal.h"
#include <stdio.h>
uint16_t count=0;

void CountSensor_Init(void){
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();

	GPIO_InitTypeDef laser = {0};
	laser.Pin = GPIO_PIN_14  ;
	laser.Mode = GPIO_MODE_IT_FALLING; ;
	laser.Pull = GPIO_PULLUP ;
	laser.Speed = GPIO_SPEED_FREQ_LOW ;

	HAL_GPIO_Init(GPIOB, &laser);


	HAL_NVIC_SetPriority(EXTI15_10_IRQn,1,1);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);



	__HAL_RCC_GPIOA_CLK_ENABLE();   // 開啟 GPIOC 時脈

	GPIO_InitTypeDef led = {0};
	led.Pin = GPIO_PIN_0;
	led.Mode = GPIO_MODE_OUTPUT_PP;   // 推挽輸出
	led.Pull = GPIO_NOPULL;
	led.Speed = GPIO_SPEED_FREQ_LOW;

	HAL_GPIO_Init(GPIOA, &led);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);

}

uint16_t get_count(void){
	return count;
}



void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_14)
    {
    	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);  // LED 狀態反轉
        count ++;
    }
}
