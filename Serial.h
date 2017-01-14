#ifndef SERIALSOFTWARE_H
#define SERIALSOFTWARE_H

#pragma anon_unions

#include "stm32f7xx_hal.h"
#include "ANSI_VT100.h"

#define SIZE_BUFFER_ATENCION	100
#define SIZE_BUFFER_RESPUESTA 100
#define RETROCESO							0x08
#define SIN_CARACTERES				-1
#define SIN_CADENA						-1
#define SERIAL_OK							0
#define BUZZERTIME						50

uint8_t	 SERIAL_HEADER[] = 
	{ 																				
		" --------------------------------------------\r\n Nombre: Comunicación Serial printf (Viewer) \r\n Versión: v0.01 \r\n Autor: Aurelio Siordia González \r\n Última modificación: 12/Enero/2017 \r\n--------------------------------------------\r\n\n"
	};


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
true,
SerialBusy,
SerialIdle
}eSerialStatus;

typedef struct
{
	GPIO_TypeDef * SerialPuertoBuzzer;
	uFlags Flags;
	uint8_t BufferAtencion[SIZE_BUFFER_ATENCION];
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
	HAL_NVIC_SetPriority(TIM7_IRQn,2,0);
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
  Serial_BuzzerInitStruct.Pull = GPIO_NOPULL;
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
		HAL_TIM_Base_Stop_IT(&htim7);
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
	gsSerial.Flags.all = 0x00;
	printf("%s", SERIAL_HEADER);
	fflush(stdin);
}
//-------------------------------------------------------------------------------------------------------------------------
//Name: Serial_Atencion
//Autor: Aurelio Siordia González
//
//Parameters: void
//Return: void
//
//Description:
//Permite escribir en la terminal Viwer (printf) del IDE Keil, así como editar el texto 
//de la manera más parecida a una terminal de Windows.
//
//-------------------------------------------------------------------------------------------------------------------------
void Serial_Atencion(void)
{
	static uint8_t SerialAtencionIndice = 0;
	int32_t StatusReceiveChar;
	
	Serial_AtencionBuzzer();

	//Si se excede el tamaño del buffer asignado
	if(SerialAtencionIndice>SIZE_BUFFER_ATENCION)
	{
		SerialAtencionIndice = 0;
		printf("\nADVERTENCIA: TAMAÑO DE BUFFER EXCEDIDO\n");
		fflush(stdin);
		return;
	}
	StatusReceiveChar = ITM_ReceiveChar();
	
	//Si no hay nada qué recibir de la consola
	if(StatusReceiveChar == SIN_CARACTERES) return; 
	
	//Si se recibe algún comando ANSI/VT100
	if(StatusReceiveChar == ESC)
		{			
			//Se pregunta si se inicializó el buzzer
			if(gsSerial.Flags.SerialBuzzerIniciado == true) gsSerial.Flags.SerialEnciendeBuzzer = true;
			return;
		}
	
	//Si se recibió un caracter de retroceso
	if(StatusReceiveChar == RETROCESO)
	{	
		//Si el puntero de la consola está en la posición inicial
		if(SerialAtencionIndice==0) 
		{
			//Activa el zumbido del buzzer
			if(gsSerial.Flags.SerialBuzzerIniciado == true) gsSerial.Flags.SerialEnciendeBuzzer = true;
			HAL_TIM_Base_Start_IT(&htim7);
			return;
		}
		
		//Si está en otra posición, limpiar el espacio anterior, tanto en la consola como en el buffer
		SerialAtencionIndice--;
		gsSerial.BufferAtencion[SerialAtencionIndice] = 0x00;
		printf("%s %s", CURSOR_IZQUIERDA,CURSOR_IZQUIERDA);
		fflush(stdin);
		return;
	}
	
	//Si se recibe un ENTER
	if(StatusReceiveChar == '\r')
	{
		gsSerial.BufferAtencion[SerialAtencionIndice]   = '\r';
		gsSerial.BufferAtencion[SerialAtencionIndice+1] = '\n';		
		gsSerial.BufferAtencion[SerialAtencionIndice+2] = 0x00;
		printf("\r\n");
		fflush( stdin );
		gsSerial.Flags.SerialMensajeRecibido = true;
		SerialAtencionIndice = 0;
		return;
	}
	
	//Si no se dio ninguno de los casos anteriores, almacenar en el buffer e imprimirlo en la consola
	gsSerial.BufferAtencion[SerialAtencionIndice] = StatusReceiveChar;
	printf("%c",  gsSerial.BufferAtencion[SerialAtencionIndice]);
	fflush(stdin);
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
	static uint8_t SerialgetDatoIndice = 0;
	uint8_t SerialgetSize;
	//Si no hay String para transmitir, no se ejecuta lo siguiente
	if(gsSerial.Flags.SerialMensajeRecibido == false) return(SIN_CADENA);
	
	//Se tranfiere valor de una cadena a otra
	if(gsSerial.BufferAtencion[SerialgetDatoIndice] != 0x00)
	{
		String[SerialgetDatoIndice] = gsSerial.BufferAtencion[SerialgetDatoIndice];
		SerialgetDatoIndice++;
		return(SerialBusy);
	}
	SerialgetSize = SerialgetDatoIndice;
	SerialgetDatoIndice = 0;
	gsSerial.Flags.SerialMensajeRecibido = false;
	return(SerialgetSize);
}

//---------------------------------------------------
//
//
//
//
//---------------------------------------------------
uint8_t Serial_ImprimirString(uint8_t * String)
{
	static uint8_t ImprimirTasks = 0;
	static uint8_t * ImprimirAuxiliar;
	uint8_t StatusImprimir;
	
	switch(ImprimirTasks)
	{
		case 0:
			ImprimirAuxiliar = String;
			printf("\n");
			fflush(stdin);
			ImprimirTasks++;
			StatusImprimir = SerialBusy;
			break;

		case 1:
			printf("%c", *ImprimirAuxiliar);
			fflush(stdin);
			ImprimirAuxiliar++;
			if(*ImprimirAuxiliar != '\n') return(SerialBusy);
			printf("\n");
			ImprimirTasks = 0;
			StatusImprimir = SerialIdle;
			break;
	}
	return (StatusImprimir);
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
	fflush(stdin);
}

void HardFault_Handler(void)
{
printf("HardFault_Handler");
while(1);
}


#endif
