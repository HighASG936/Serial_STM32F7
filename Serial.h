#ifndef SERIALSOFTWARE_H
#define SERIALSOFTWARE_H
/*
=============================================================================================
Name of File: Serial.h

Author:	Aurelio Siordia González
Date:	20/01/2017

Description:
Esta librería tiene la función de administrar las funciones relacionadas con la terminal 
incorporada en el Keilv5, Debug(printf)Viewer y la comunicación serial UART incorporada en el 
microcontrolador STM32F7. Aunque cabe señalar que puede ser compatible con cualquier tarjeta
de desarrollo Discovery.

How to use:

+Inicializar la libreria: 
Invocar <Serial_Iniciar> para inicializar.

+Habilitar la función de buzzer:
Utilice la función <Serial_InitBuzzer> indicando el pin y puerto. Para observar el estado del
buzzer y cuando se debe activar, se consulta <Serial_AtencionBuzzer>.

+Consultar estado de la terminal:
Cada vez que desee obsevar el estado de envío o recepción entre la terminal y la comunicación 
serial UART, invoque <Serial_Atencion>.

=============================================================================================
*/

//pragmas------------------------------------------------------------------------------------/
#pragma anon_unions

//includes----------------------------------------------------------------------------------/
#include "stm32f7xx_hal.h"
#include "ANSI_VT100.h"

//defines-----------------------------------------------------------------------------------/
#define SIZE_BUFFER_ATENCION	100
#define SIZE_BUFFER_RESPUESTA 100
#define RETROCESO							0x08
#define SIN_CARACTERES				-1
#define SIN_CADENA						-1
#define SERIAL_OK							0
#define BUZZERTIME						50

//Prototipos------------------------------------------------------------------------------------/

void Serial_Iniciar(void);
void Serial_InitBuzzer(GPIO_TypeDef * GPIOPortBuzzer, uint16_t BUZZER_Pin);
void Serial_AtencionBuzzer(void);
void Serial_Atencion(void);
int8_t Serial_getString(uint8_t * String);
uint8_t Serial_ImprimirString(uint8_t * String);
void Serial_AboutIt(void);

//Variables externas----------------------------------------------------------------------------/
extern UART_HandleTypeDef huart7;

//Estructuras y uniones-------------------------------------------------------------------------/
typedef union
{
	uint8_t all;
	struct
	{
		uint8_t bit7   								 :1;
		uint8_t bit6	 								 :1;
		uint8_t bit5	 								 :1;
		uint8_t bit4	 								 :1;
		uint8_t SerialVTComandRecibido :1;
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

	void		(*InitBuzzer)(GPIO_TypeDef * GPIOPortBuzzer, uint16_t BUZZER_Pin);
	void		(*AtencionBuzzer)(void);
	void 		(*Atencion)(void);
	int8_t	(*getString)(uint8_t * String);
	uint8_t (*ImprimirString)(uint8_t * String);
	void 		(*AboutIt)(void);
	
}eSerial;

//Variables privadas----------------------------------------------------------------------------/
GPIO_InitTypeDef Serial_BuzzerInitStruct;		//Estructura del GPIO para el buzzer
uint16_t LoudTime;													//Contador para medir el tiempo de encendido del buzer
eSerial gsSerial;														//Estructura General de la librería Serial.h
TIM_HandleTypeDef Timer;										//Estructura del temporizador (ms) del buzzer y otros servicios
uint8_t	 SERIAL_HEADER[] = {" --------------------------------------------\r\n Nombre: Comunicación Serial printf (Viewer) \r\n Versión: v1.0.2 \r\n Autor: Aurelio Siordia González \r\n Última modificación: 20/Enero/2017 \r\n--------------------------------------------\r\n\n"};

/* Inicialización del temporizador */
void Timer_Inicializar(void)
{
  TIM_MasterConfigTypeDef sMasterConfig;

  Timer.Instance = TIM7;
  Timer.Init.Prescaler = 108;
  Timer.Init.CounterMode = TIM_COUNTERMODE_DOWN;
  Timer.Init.Period = 999;
  HAL_TIM_Base_Init(&Timer);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(&Timer, &sMasterConfig);
	HAL_NVIC_SetPriority(TIM7_IRQn,2,0);
	HAL_NVIC_EnableIRQ(TIM7_IRQn);
}

//-------------------------------------------------------------------------------------------------------------------------
//Name: Serial_AtencionBuzzer
//
//Description:
//Observa si es requerido que suene el buzzer
//
//Parameters: void
//
//Return: void
//
//Author: Aurelio Siordia González
//Fecha: 20/01/2017
//-------------------------------------------------------------------------------------------------------------------------
void Serial_AtencionBuzzer(void)
{
	if(gsSerial.Flags.SerialEnciendeBuzzer == true && LoudTime != 0x00)
	{
	HAL_GPIO_WritePin(gsSerial.SerialPuertoBuzzer, Serial_BuzzerInitStruct.Pin, GPIO_PIN_SET);
	LoudTime--;
	return;
	}
	HAL_TIM_Base_Stop_IT(&Timer);
	HAL_GPIO_WritePin(gsSerial.SerialPuertoBuzzer, Serial_BuzzerInitStruct.Pin, GPIO_PIN_RESET);
	gsSerial.Flags.SerialEnciendeBuzzer = false;
	LoudTime = BUZZERTIME;
}

/* Handler de interrupción temporizador */
void TIM7_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&Timer);
	Serial_AtencionBuzzer();
}

//-------------------------------------------------------------------------------------------------------------------------
//Name: Serial_InitBuzzer
//
//Description:
//Inicializar el GPIO y la bandera de Buzzer iniciado
//
//Parameters:	<GPIOPortBuzzer>	Puerto del GPIO del Buzzer
//						<BUZZER_Pin>			Pin del GPIO del Buzzer
//
//Return: void
//
//Author: Aurelio Siordia González
//Fecha: 20/01/2017
//-------------------------------------------------------------------------------------------------------------------------
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

//-------------------------------------------------------------------------------------------------------------------------
//Name: Serial_Iniciar
//
//Description:
//Inicializa temporizador, resetea todas las banderas, imprime el header a imprimir y limpiar buffer de printf
//
//Parameters: void
//
//Return: void
//
//Author: Aurelio Siordia González
//Fecha: 20/01/2017
//-------------------------------------------------------------------------------------------------------------------------
void Serial_Iniciar(void)
{
	//Asociar punteros a función con las funciones declaradas
	gsSerial.InitBuzzer 		= Serial_InitBuzzer;
	gsSerial.AtencionBuzzer	=	Serial_AtencionBuzzer;
	gsSerial.Atencion				= Serial_Atencion;
	gsSerial.getString			= Serial_getString;
	gsSerial.ImprimirString	= Serial_ImprimirString;
	gsSerial.AboutIt 				= Serial_AboutIt;	
	
	Timer_Inicializar();
	gsSerial.Flags.all = 0x00;
	printf("%s", SERIAL_HEADER);
	fflush(stdin);
}

//-------------------------------------------------------------------------------------------------------------------------
//Name: Serial_Atencion
//
//Description:
//Permite escribir en la terminal Viwer (printf) del IDE Keil, así como editar el texto 
//de la manera más parecida a una terminal de Windows.
//
//Parameters: void
//
//Return: void
//
//Author: Aurelio Siordia González
//Fecha: 20/01/2017
//-------------------------------------------------------------------------------------------------------------------------
void Serial_Atencion(void)
{
	static uint8_t SerialAtencionIndice = 0;
	int32_t StatusReceiveChar;

	//Si se excede el tamaño del buffer asignado
	if(SerialAtencionIndice>SIZE_BUFFER_ATENCION)
	{
		SerialAtencionIndice = 0;
		printf("\nADVERTENCIA: TAMAÑO DE BUFFER EXCEDIDO\n");
		fflush(stdin);
		return;
	}
	
	//Observar lo que se ha ingresado en la terminal
	StatusReceiveChar = ITM_ReceiveChar();
	
	//Si se ha recibido un archivo un comando VT100
	if(gsSerial.Flags.SerialVTComandRecibido == true)
	{

		if(StatusReceiveChar == '[' ) return;
		if(StatusReceiveChar == '1') printf("%s", CURSOR_IZQUIERDA);
		if(StatusReceiveChar == 'C')printf("%s", CURSOR_DERECHA);

		gsSerial.Flags.SerialVTComandRecibido = false;
	}
	
	//Si no hay nada qué recibir de la consola
	if(StatusReceiveChar == SIN_CARACTERES) return; 
	
	//Si se recibe algún comando ANSI/VT100
	if(StatusReceiveChar == ESC)
		{
				gsSerial.Flags.SerialVTComandRecibido = true;
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
			if(gsSerial.Flags.SerialBuzzerIniciado == true)
			{ 
				gsSerial.Flags.SerialEnciendeBuzzer = true;
				HAL_TIM_Base_Start_IT(&Timer);
			}
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

//-------------------------------------------------------------------------------------------------------------------------
//Name: Serial_getString
//
//Description:
//Obtiene la cadena de caracteres que se recibieron atraves de la consola
//
//Parameters: <String> 			Cadena de caracteres en donde se almacenará la cadena recibida 
//
//Return: 		<SIN_CADENA>		No hay cadena 
//						<SerialBusy>  	Procesando cadena
//						<SerialgetSize>	Tamaño de la cadena obtenida
//
//Author: Aurelio Siordia González
//Fecha: 20/01/2017
//-------------------------------------------------------------------------------------------------------------------------
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

//-------------------------------------------------------------------------------------------------------------------------
//Name: Serial_ImprimirString
//
//Description:
//Imprime cadena en consola, obtenida por comunicación UART
//
//Parameters: <String> 			Cadena de caracteres a imprimir
//
//Return: 		<SerialBusy>		Ejecutando impresion en consola 
//						<SerialIdle>  	Función ociosa
//
//Author: Aurelio Siordia González
//Fecha: 20/01/2017
//-------------------------------------------------------------------------------------------------------------------------
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

//-------------------------------------------------------------------------------------------------------------------------
//Name: Serial_AboutIt
//
//Description:
//Imprime el header con la información de la librería
//
//Parameters: void
//
//Return: 		void
//
//Author: Aurelio Siordia González
//Fecha: 20/01/2017
//-------------------------------------------------------------------------------------------------------------------------
void Serial_AboutIt(void)
{
	printf("%s", SERIAL_HEADER);
	fflush(stdin);
}

//-------------------------------------------------------------------------------------------------------------------------
//Name: HardFault_Handler
//
//Description:
//En caso de haber ocurrido un error grave en MCU, imprime el mensaje del tipo de error ocurrido
//
//Parameters: void
//
//Return: 		void
//
//Author: Aurelio Siordia González
//Fecha: 20/01/2017
//-------------------------------------------------------------------------------------------------------------------------
void HardFault_Handler(void)
{
printf("HardFault_Handler");
while(1);
}


#endif
