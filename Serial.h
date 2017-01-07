#ifndef SERIALSOFTWARE_H
#define SERIALSOFTWARE_H

#include "stm32f7xx_hal.h"

#define SIZE_BUFFER_ATENCION	100
#define SIZE_BUFFER_RESPUESTA 100

uint8_t BufferAtencion[SIZE_BUFFER_ATENCION];
uint8_t	BufferRespuesta[SIZE_BUFFER_RESPUESTA];

extern UART_HandleTypeDef huart7;

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
	static uint8_t i = 0;
	if(i>SIZE_BUFFER_ATENCION) return;
	int32_t StatusReceiveChar = ITM_ReceiveChar();
	if(StatusReceiveChar == 0x08)
	{
		BufferAtencion[i] = 0x00;
		i--;
		printf("%c%c%c%c",0x1B,'[','D',0x08);
	}
	if(StatusReceiveChar == -1) return; 
	BufferAtencion[i] = StatusReceiveChar;
	if(StatusReceiveChar == 0x0d)
		{
			BufferAtencion[i+1] = 0x0a;
			BufferAtencion[i+2] = 0x00;
			printf("\r\n");
			i = 0;
			Serial_EnviarComando();
			return;
		}
	printf("%c",  BufferAtencion[i]);
	i++;
}

#endif
