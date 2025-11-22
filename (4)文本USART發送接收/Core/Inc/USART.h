#ifndef __SERIAL_H
#define __SERIAL_H


#include <string.h>

extern uint8_t Serial_TxPacket[];
extern char Serial_RxPacket[];

void USART_init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
void Serial_SendPacket(void);
void Serial_Printf(char *format, ...);
void Serial_GetRxFlag(void);
uint8_t Serial_GetRxData(void);
uint8_t Serial_GetpacketFlag(void);
#endif
