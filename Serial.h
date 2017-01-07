#ifndef SERIALSOFTWARE_H
#define SERIALSOFTWARE_H

#pragma anon_unions

#include "stm32f7xx_hal.h"
#include "ANSI_VT100.h"

#define SIZE_BUFFER_ATENCION	100
#define SIZE_BUFFER_RESPUESTA 100
#define ESPACIO								0x08
#define SIN_CARACTERES				-1

uint8_t	 SERIAL_HEADER[] = 
	{ 																				
		" --------------------------------------------\r\n Nombre: Comunicación Serial printf (Viewer) \r\n Versión: v0.01 \r\n Autor: Aurelio Siordia González \r\n Última modificación: 06/Enero/2017 \r\n--------------------------------------------\r\n\n"
	};

	uint8_t BufferAtencion[SIZE_BUFFER_ATENCION];
uint8_t	BufferRespuesta[SIZE_BUFFER_RESPUESTA];

extern UART_HandleTypeDef huart7;

typedef union
{
	uint8_t all;
	struct
	{
		uint8_t bit7   								 :1;
		uint8_t bit6	 								 :1;
		uint8_t bit5	 								 :1;
		uint8_t bit4	 								 :1;
		uint8_t bit3	 								 :1;
		uint8_t bit2	 								 :1;
		uint8_t bit1	 								 :1;
		uint8_t SerialMensajeRecibido	 :1;
	};

}uFlags;

enum
{
false,
true

}eSerialStatus;

typedef struct
{
	uFlags Flags;
}eSerial;

eSerial gsSerial;

//---------------------------------------------------
//
//
//
//
//---------------------------------------------------
void Serial_Iniciar(void)
{
	gsSerial.Flags.all = 0x00;
	printf("%s", SERIAL_HEADER);
}

//---------------------------------------------------
//
//
//
//
//---------------------------------------------------
void Serial_Atencion(void)
{
	static uint8_t SerialAtencionIndice = 0;
	int32_t StatusReceiveChar;
	
	if(SerialAtencionIndice>SIZE_BUFFER_ATENCION)
	{
		SerialAtencionIndice = 0;
		printf("\nADVERTENCIA: TAMAÑO DE BUFFER EXCEDIDO\n");
		return;
	}
	StatusReceiveChar = ITM_ReceiveChar();
	
	if(StatusReceiveChar == SIN_CARACTERES) return; 
	
	if(StatusReceiveChar == ESC)return;

	if(StatusReceiveChar == ESPACIO)
	{	
		if(SerialAtencionIndice==0) return;
		SerialAtencionIndice--;
		BufferAtencion[SerialAtencionIndice] = 0x00;
		printf("%s %s", CURSOR_IZQUIERDA,CURSOR_IZQUIERDA);
		return;
	}
	
	if(StatusReceiveChar == 0x0d)
	{
		BufferAtencion[SerialAtencionIndice]   = 0x0d;
		BufferAtencion[SerialAtencionIndice+1] = 0x0a;
		BufferAtencion[SerialAtencionIndice+2] = 0x00;
		printf("\r\n");
		gsSerial.Flags.SerialMensajeRecibido = true;
		SerialAtencionIndice = 0;
		return;
	}
	
	BufferAtencion[SerialAtencionIndice] = StatusReceiveChar;
/*
	if(BufferAtencion[0] == 'c' &&
		 BufferAtencion[1] == 'l' &&
		 BufferAtencion[2] == 'c' &&
		 BufferAtencion[3] == '\r'&&
		 BufferAtencion[4] == '\n'&&
		 SerialAtencionIndice > 4
		)
		{
			printf("%s",LIMPIAR_PANTALLA);
			SerialAtencionIndice = 0;
			return;
		}
*/
	printf("%c",  BufferAtencion[SerialAtencionIndice]);
	SerialAtencionIndice++;
}

//---------------------------------------------------
//
//
//
//
//---------------------------------------------------
void Serial_getString(uint8_t * String)
{
	uint8_t SerialgetDatoIndice = 0;
	if(gsSerial.Flags.SerialMensajeRecibido == false) return;
	gsSerial.Flags.SerialMensajeRecibido = false;
	while(BufferAtencion[SerialgetDatoIndice] != 0x00)
	{
		*String = BufferAtencion[SerialgetDatoIndice];
		String++;
		SerialgetDatoIndice++;
	}
}

//---------------------------------------------------
//
//
//
//
//---------------------------------------------------
void Serial_ImprimirString(uint8_t * String)
{
	printf("\n");
	while(*String != '\n')
	{
		printf("%c", *String);
		String++;
	}
	printf("\n");
}

//---------------------------------------------------
//
//
//
//
//---------------------------------------------------
void Serial_AboutIt(void)
{
	printf("%s", SERIAL_HEADER);
}

#endif
