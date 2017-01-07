#ifndef SERIALSOFTWARE_H
#define SERIALSOFTWARE_H

#include "stm32f7xx_hal.h"
#include "ANSI_VT100.h"

#define SIZE_BUFFER_ATENCION	100
#define SIZE_BUFFER_RESPUESTA 100

#pragma anon_unions

uint8_t BufferAtencion[SIZE_BUFFER_ATENCION];
uint8_t	BufferRespuesta[SIZE_BUFFER_RESPUESTA];

extern UART_HandleTypeDef huart7;

typedef union
{
	uint8_t all;
	struct
	{
		uint8_t bit7   :1;
		uint8_t bit6	 :1;
		uint8_t bit5	 :1;
		uint8_t bit4	 :1;
		uint8_t bit3	 :1;
		uint8_t bit2	 :1;
		uint8_t bit1	 :1;
		uint8_t bit0	 :1;
	};

}uFlags;


typedef struct
{
	uFlags Flags;
}eSerial;

eSerial gsSerial;

void Serial_Iniciar(void)
{
	printf("%s", HEADER);

}


void Serial_EnviarComando(void)
{
	uint8_t j = 0;
	uint8_t u8SizeMensaje = 0;
	while( (j < SIZE_BUFFER_ATENCION)  &&  (BufferAtencion[j] != 0x00) )
	{
		HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
		u8SizeMensaje++;
		j++;
	}
	HAL_UART_Transmit(&huart7, BufferAtencion,u8SizeMensaje,100);
	HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,GPIO_PIN_RESET);
	__asm("NOP");
}


void Serial_Atencion(void)
{
	static uint8_t SerialAtencionIndice = 0;
	
	if(SerialAtencionIndice>SIZE_BUFFER_ATENCION) return;
	int32_t StatusReceiveChar = ITM_ReceiveChar();
	if(StatusReceiveChar == -1) return; 
	if(StatusReceiveChar == ESC)return;
	if(StatusReceiveChar == 0x08)
	{	
		if(SerialAtencionIndice==0) return;
		SerialAtencionIndice--;
		BufferAtencion[SerialAtencionIndice] = 0x00;
		printf("%s %s", CURSOR_IZQUIERDA,CURSOR_IZQUIERDA);
		return;
	}
	BufferAtencion[SerialAtencionIndice] = StatusReceiveChar;
	if(StatusReceiveChar == 0x0d)
		{
			BufferAtencion[SerialAtencionIndice+1] = 0x0a;
			BufferAtencion[SerialAtencionIndice+2] = 0x00;
			printf("\r\n");
			SerialAtencionIndice = 0;
			Serial_EnviarComando();
			return;
		}
	printf("%c",  BufferAtencion[SerialAtencionIndice]);
	SerialAtencionIndice++;
}

#endif
