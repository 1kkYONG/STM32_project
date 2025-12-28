#ifndef __MPU6050_H
#define __MPU6050_H

void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data);
uint8_t MPU6050_ReadReg(uint8_t RegAddress);

void MPU6050_Init(void);
uint8_t MPU6050_GetID(void);

typedef struct{
	  int16_t AX, AY, AZ, GX, GY, GZ;			//定义用于存放各个数据的变量
}MPU;

void MPU6050_GetData(MPU *MPU_data);

#endif
