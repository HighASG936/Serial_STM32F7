#ifndef SERIALSOFTWARE_H
#define SERIALSOFTWARE_H

#pragma anon_unions

#include "stm32f7xx_hal.h"
#include "ANSI_VT100.h"

#define SIZE_BUFFER_ATENCION	100
#define SIZE_BUFFER_RESPUESTA 100
#define ESPACIO								0x08
#define SIN_CARACTERES				-1
#define SIN_CADENA						-1
#define SERIAL_OK							0
#define BUZZERTIME						50

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
		uint8_t SerialEnciendeBuzzer	 :1;
		uint8_t SerialBuzzerIniciado	 :1;
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
	GPIO_TypeDef * SerialPuertoBuzzer;
	uFlags Flags;
}eSerial;

eSerial gsSerial;

GPIO_InitTypeDef Serial_BuzzerInitStruct;
uint16_t LoudTime;

TIM_HandleTypeDef htim7;

/* TIM7 init function */
void MX_TIM7_Init(void)
{

  TIM_MasterConfigTypeDef sMasterConfig;

  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 108;
  htim7.Init.CounterMode = TIM_COUNTERMODE_DOWN;
  htim7.Init.Period = 999;
  HAL_TIM_Base_Init(&htim7);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig);
	HAL_NVIC_SetPriority(TIM7_IRQn,1,0);
	HAL_NVIC_EnableIRQ(TIM7_IRQn);
}

void TIM7_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&htim7);
	LoudTime--;
}
//---------------------------------------------------
//
//
//
//
//---------------------------------------------------
void Serial_InitBuzzer(GPIO_TypeDef * GPIOPortBuzzer, uint16_t BUZZER_Pin)
{
	
	gsSerial.SerialPuertoBuzzer = GPIOPortBuzzer;
	
  Serial_BuzzerInitStruct.Pin = BUZZER_Pin;
  Serial_BuzzerInitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  Serial_BuzzerInitStruct.Pull = GPIO_PULLUP;
  Serial_BuzzerInitStruct.Speed = GPIO_SPEED_LOW;
	
  HAL_GPIO_Init(GPIOPortBuzzer, &Serial_BuzzerInitStruct);
	HAL_GPIO_WritePin(GPIOPortBuzzer, BUZZER_Pin, GPIO_PIN_RESET);
	gsSerial.Flags.SerialBuzzerIniciado = true;
}


//---------------------------------------------------
//
//
//
//
//---------------------------------------------------
void Serial_AtencionBuzzer(void)
{
	if(gsSerial.Flags.SerialEnciendeBuzzer == true	&&	LoudTime != 0)
	{
		HAL_GPIO_WritePin(gsSerial.SerialPuertoBuzzer, Serial_BuzzerInitStruct.Pin, GPIO_PIN_SET);
		return;
	}
	HAL_GPIO_WritePin(gsSerial.SerialPuertoBuzzer, Serial_BuzzerInitStruct.Pin, GPIO_PIN_RESET);
	LoudTime = BUZZERTIME;
	gsSerial.Flags.SerialEnciendeBuzzer = false;
}

//---------------------------------------------------
//
//
//
//
//---------------------------------------------------
void Serial_Iniciar(void)
{
	MX_TIM7_Init();
	HAL_TIM_Base_Start_IT(&htim7);
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
	
	Serial_AtencionBuzzer();
	
	if(SerialAtencionIndice>SIZE_BUFFER_ATENCION)
	{
		SerialAtencionIndice = 0;
		printf("\nADVERTENCIA: TAMAÑO DE BUFFER EXCEDIDO\n");
		return;
	}
	StatusReceiveChar = ITM_ReceiveChar();
	
	if(StatusReceiveChar == SIN_CARACTERES) return; 
	
	if(StatusReceiveChar == ESC)
		{
			if(gsSerial.Flags.SerialBuzzerIniciado == true) gsSerial.Flags.SerialEnciendeBuzzer = true;
			return;
		}
	if(StatusReceiveChar == ESPACIO)
	{	
		if(SerialAtencionIndice==0) 
		{
			if(gsSerial.Flags.SerialBuzzerIniciado == true) gsSerial.Flags.SerialEnciendeBuzzer = true;
			return;
		}
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
	printf("%c",  BufferAtencion[SerialAtencionIndice]);
	SerialAtencionIndice++;
}

//---------------------------------------------------
//
//
//
//
//---------------------------------------------------
int8_t Serial_getString(uint8_t * String)
{
	uint8_t SerialgetDatoIndice = 0;
	if(gsSerial.Flags.SerialMensajeRecibido == false) return(SIN_CADENA);
	gsSerial.Flags.SerialMensajeRecibido = false;
	while(BufferAtencion[SerialgetDatoIndice] != 0x00)
	{
		*String = BufferAtencion[SerialgetDatoIndice];
		String++;
		SerialgetDatoIndice++;
	}
	return(SerialgetDatoIndice);
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
