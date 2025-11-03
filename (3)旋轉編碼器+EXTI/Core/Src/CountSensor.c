#include "stm32f1xx_hal.h"
#include <stdio.h>

int16_t en_count=0;

void CountSensor_Init(void){
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();

	GPIO_InitTypeDef encode = {0};
	encode.Pin = GPIO_PIN_0 | GPIO_PIN_1  ;
	encode.Mode = GPIO_MODE_IT_FALLING; ;
	encode.Pull = GPIO_PULLUP;
	encode.Speed = GPIO_SPEED_FREQ_LOW ;

	HAL_GPIO_Init(GPIOB, &encode);

	HAL_NVIC_SetPriority(EXTI0_IRQn,1,1);
	HAL_NVIC_EnableIRQ(EXTI0_IRQn);

	HAL_NVIC_SetPriority(EXTI1_IRQn,1,2);
	HAL_NVIC_EnableIRQ(EXTI1_IRQn);

}

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch(GPIO_Pin){
		case GPIO_PIN_1:
			if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0) == GPIO_PIN_SET){

			    		en_count ++;
			    	}
			break;
		case GPIO_PIN_0:
			if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1) == GPIO_PIN_SET){

			    		en_count --;
			    	}
			break;

	}
}


int16_t get_encount(void){
	int16_t get = 0;
	get=en_count;
	en_count = 0;
	return get;
}

