/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "i2c.h"
#include "rtc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "mpu6050.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "timers.h"
#include "task.h"
#include "event_groups.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define DISPLAY_OFF				 0
#define Get_Date				 	(1UL << 0UL)
#define Get_Menu					(1UL << 1UL)
#define GO_SLEEP					(1UL << 2UL)
#define SET_ALARM					(1UL << 3UL)
#define SET_DATE					(1UL << 4UL)
#define SET_TIME					(1UL << 5UL)



/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */


uint8_t Hours=0x0;
uint8_t Mins=0x0;
uint8_t Secs=0x0;

uint8_t Day=0;
uint8_t Month=0x0;
uint8_t Year=0x0;

uint8_t s_Date=0x0;

uint8_t Alarm_Day=0;
uint8_t time=0;
int idle=0;
uint32_t state=0;
int i=0;
int j=0;
int test=0;
int choix=0; //Variable qui permet de detrminer la selection de l'utiliateur dans le menu
int flag=0;	//Permet de Determiner qu'il y a bien eu un appui prolongé

uint8_t confirmation=0 ;
uint8_t Alarm=0;


TimerHandle_t Timer,SLEEP,Bouton,Date_ ;
EventBits_t Event_G;
EventGroupHandle_t Event;

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
void Date(void);
void Menu(void);
void setalarm(int,int,int);
void Show_Date(int,int,int);
void Show_Time(int,int,int);
/* When in Idle Mode, The system goes into SleepMode and will wake up
	 when an EXTI interrupt is triggered (Button Push) 									*/


void vApplicationIdleHook( void ){
	
}
/**********************************************************************
This Task will be used to Show the Menu on Screen.
**********************************************************************/

void vOLED(void *pvParameters){
	
  for( ;; )
    {
			Event_G=xEventGroupWaitBits(Event,Get_Menu,pdTRUE,pdTRUE,portMAX_DELAY);
			if( (Event_G & (Get_Menu)) != 0){	
				Menu();
			}
	}
}
/*********************************************************************
Timer:900 ms
This Timer will allow to update the time on Screen every Second
by putting vLED Task in Running Mode every ~1000 ms
**********************************************************************/
void Date_Callback(void *pvParameters){

BaseType_t xHigherPriorityTaskWoken =pdFALSE;

xEventGroupSetBitsFromISR(Event,Get_Date,&xHigherPriorityTaskWoken);
idle++;


}


/************************************************************
Task Used to show the date whenever the user presses on the main Button
Waits for the Get_Date to be set by the EXTI Callback to put the date on the screen
The Get_Date bit is only set to 0 in 2 cases :
1-NO User interaction for 5 seconds which triggers the SLEEP_TIMER that puts the system in
STOP Mode (LOW POWER)
2-There is a long Press Detected which means the user want to access the Menu
This allows the the Task to be called continuosly as long as none of the cases above happened
and updates the time shown in real time.
*************************************************************/


void vLED(void *pvParameters){
	
   for( ;; )
    {
      
			Event_G=xEventGroupWaitBits(Event,Get_Date,pdTRUE,pdTRUE,portMAX_DELAY);
			if( (Event_G & Get_Date) !=0 ){
				Date();
				
			}
		}
}

void Date(){
	RTC_DateTypeDef datebuff;
	RTC_TimeTypeDef timebuff;
	char buffer[10];
	ssd1306_Init();
	ssd1306_Fill(Black);
	HAL_RTC_GetDate(&hrtc,&datebuff,RTC_FORMAT_BIN);
	sprintf(buffer,"%d/%d/%d",datebuff.Date,datebuff.Month,datebuff.Year);
	ssd1306_SetCursor(2, 0);
	ssd1306_WriteString(buffer,Font_7x10,White);
	ssd1306_SetCursor(2, 18);
	HAL_RTC_GetTime(&hrtc,&timebuff,RTC_FORMAT_BIN);
	sprintf(buffer,"%dH:%dmn:%ds",timebuff.Hours,timebuff.Minutes,timebuff.Seconds);
	ssd1306_WriteString(buffer,Font_7x10,White);
	ssd1306_UpdateScreen();
}

/***********************************************************************
The Sleep Timer is set whenever there is a bouton press (which showcases a user interaction)
It is set to 5sec which means that if there are no button pressed in 5sec the system will go to Sleep
In the callback we will clear almost all of the GroupBits to reiniliaze the parameters on WakeUp
Then the system will go into STOP_Mode.
STM32 STOP Mode can be woken from by both an EXTI or an RTC Alarm interrupt,
which makes it ideal for a watch.
************************************************************************/
void vSLEEP(void *pvParameters){
for(;;){	
	BaseType_t xHigherPriorityTaskWoken =pdFALSE; 
	xEventGroupWaitBits(Event,GO_SLEEP,pdTRUE,pdTRUE,portMAX_DELAY);
	xTimerStopFromISR(Date_,&xHigherPriorityTaskWoken);
	ssd1306_SetDisplayOn(DISPLAY_OFF);
	xEventGroupClearBitsFromISR(Event,(Get_Menu|SET_ALARM|SET_DATE));
	flag=0;
	choix=0;
	xTimerStopFromISR(SLEEP,&xHigherPriorityTaskWoken);
	HAL_SuspendTick();
	

	PWR->CR |= PWR_CR_CWUF; // clear the WUF flag after 2 clock cycles
  PWR->CR &= ~( PWR_CR_PDDS ); // Enter stop mode when the CPU enters deepsleep
  RCC->CFGR |= RCC_CFGR_SWS_HSI ;// HSI16 oscillator is wake-up from stop clock
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; // low-power mode = stop mode
  __WFI();
	}

}
void SLEEP_Callback(void *pvParameters){
	BaseType_t xHigherPriorityTaskWoken =pdFALSE;

	xEventGroupSetBitsFromISR(Event,GO_SLEEP,&xHigherPriorityTaskWoken);

}
/**********************************************************************************
The TimerCallback is used to debounce the "Main" button presses as well as determine what kind of press it is:
Short Press:To show Date /navigate through menu.
Long Press :To get access to the Menu.
It is a 50ms Timer that is started (Through EXTI Callback) whenever there is Button Press Detected
*The "state" variable is incremented whenever  every 50ms the button is still being pressed and lets determine
whether it is a long press or a short one.
*Flage variable is set to 1 if the program is currently present in the menu
The Callback Treats 4 Different Scenarios:
1- A short press + Not in menu (flag=0) : This will get us the Date on the screen
2- A Long  press + Not in menu (flag=0)	: This will allow us to proceed to the menu
3- A Long press  + In Menu (flag=1)			:	This will allow us to choose one of the 2 menu options (Change Date 
																																																	set Alarm)
4- A Button press whenever we are in the SETDATE/SETALARM Menu.
***********************************************************************************/

void Timer_Callback(void *pvParameters){
	
	BaseType_t xHigherPriorityTaskWoken =pdFALSE; 
	Event_G=xEventGroupGetBitsFromISR(Event);
	state++;
	
	if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==0 && state < 15 && flag==0 && ( j 	^  time )==0){
		state=0;
		xEventGroupSetBitsFromISR(Event,Get_Date,&xHigherPriorityTaskWoken);
		xTimerStartFromISR(Date_,&xHigherPriorityTaskWoken);
		xTimerStop(Timer,portMAX_DELAY);	
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	
	else if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==0 && state > 15 && flag==0)
	{
		state=0;	
		if(idle>0){
		xTimerStopFromISR(Date_,&xHigherPriorityTaskWoken);
		idle=0;
		}
		xEventGroupClearBitsFromISR(Event,Get_Date);
		xEventGroupSetBitsFromISR(Event,Get_Menu,&xHigherPriorityTaskWoken);
		flag=1;
		xTimerStop(Timer,portMAX_DELAY);	
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	
	else if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==0  &&   flag==1){
		state=0;
		switch (choix){
			case 0 :
					xEventGroupSetBitsFromISR(Event,SET_ALARM,&xHigherPriorityTaskWoken);
					break;
			case 1 :
					xEventGroupSetBitsFromISR(Event,SET_DATE,&xHigherPriorityTaskWoken);
					break;

			}
				xTimerStop(Timer,portMAX_DELAY);
				portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	else if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==0   && ( j 	|  time  | Alarm ) != 0 ){
			RTC_DateTypeDef datebuff;
			RTC_TimeTypeDef timebuff;
			xTimerStop(Timer,portMAX_DELAY);
			state=0;
			s_Date++;
			if (s_Date==3){
				if(j==1 && Alarm==0) { 		
					datebuff.Date=Day;
					datebuff.Month=Month;
					datebuff.Year=Year;
					j=0;
					s_Date=0;
					HAL_RTC_SetDate(&hrtc,&datebuff,RTC_FORMAT_BIN);
					HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);
					xEventGroupSetBitsFromISR(Event,SET_TIME,&xHigherPriorityTaskWoken);
			}
				if(time==1	&&	Alarm==0){
					timebuff.Seconds=Secs;
					timebuff.Minutes=Mins;
					timebuff.Hours=Hours;
					time=0;
					s_Date=0;
					HAL_RTC_SetTime(&hrtc,&timebuff,RTC_FORMAT_BIN);
					HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, 0x32F2);

			}
				if(Alarm==1 && j==1){
					s_Date=0;
					j=0;
					Alarm_Day=Day;
					xEventGroupSetBitsFromISR(Event,SET_TIME,&xHigherPriorityTaskWoken);
				}
				if(Alarm==1 && time ==1 ){
					s_Date=0;
					time=0;
					setalarm(Hours,Mins,Secs);
					xEventGroupSetBitsFromISR(Event,GO_SLEEP,&xHigherPriorityTaskWoken);
				}
		}
			
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}	
/**********************************************************************
Secondary_Button_Callback will serve as debounce for the "secondary button"
This button will serve as a way as to navigate through the different menus and change the values.
Will use this button for 3 Cases :
1-When we are in the main menu to choose between the differents Options (Set_Alarm or Set_Date)
2-Changing the values in Real-Time when setting up the Time or the Alarm
*********************************************************************/
void Menu_Callback(void *pvParameters){
	
	BaseType_t xHigherPriorityTaskWoken =pdFALSE; 
	if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_13)==0 && flag==1){
		if(choix==1){
			choix=0;
		}
		else {choix=1;}
		
		xEventGroupSetBitsFromISR(Event,Get_Menu,&xHigherPriorityTaskWoken);
		xTimerStop(Bouton,portMAX_DELAY);	
	
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
 }
	if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_13)==0  && j==1){
		switch (s_Date){
			case 0:
				(Day==31) ? Day=0x1 : Day++ ;
				xEventGroupSetBitsFromISR(Event,SET_DATE,&xHigherPriorityTaskWoken);
				break;
			case 1:
				(Month==12) ? Month=0x1 : Month++;
				xEventGroupSetBitsFromISR(Event,SET_DATE,&xHigherPriorityTaskWoken);
				break;
			case 2:
				(Year==99) ?	Year=0x63	:	Year++ ;
				xEventGroupSetBitsFromISR(Event,SET_DATE,&xHigherPriorityTaskWoken);
				break;
		
		}
			xTimerStop(Bouton,portMAX_DELAY);
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_13)==0  && time==1){
		switch (s_Date){
			case 0:
				(Hours==0x17) ?	Hours=0x0	:	Hours++ ;
				xEventGroupSetBitsFromISR(Event,SET_TIME,&xHigherPriorityTaskWoken);
				break;
			case 1:
				(Mins==0x3B) ? Mins=0x0 : Mins++;
				xEventGroupSetBitsFromISR(Event,SET_TIME,&xHigherPriorityTaskWoken);
				break;
			case 2:
				(Secs==0x3B) ? Secs=0x0 : Secs++ ;
				xEventGroupSetBitsFromISR(Event,SET_TIME,&xHigherPriorityTaskWoken);
				break;
		
		}
			xTimerStop(Bouton,portMAX_DELAY);
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}

void Menu(){
		
	ssd1306_Init();
	flag=1;
	ssd1306_Fill(Black);
	if (choix==0){
		ssd1306_SetCursor(2, 0);
		ssd1306_WriteString("> SET ALARM",Font_7x10,White);
		ssd1306_SetCursor(2, 18);
		ssd1306_WriteString(" CHANGE TIME",Font_7x10,White);
		ssd1306_UpdateScreen();
	
	}
		else if (choix == 1){
		ssd1306_SetCursor(2, 0);
		ssd1306_WriteString("SET ALARM",Font_7x10,White);
		ssd1306_SetCursor(2, 18);
		ssd1306_WriteString("> CHANGE TIME",Font_7x10,White);
		ssd1306_UpdateScreen();
	}
}
/*************************************************************
Task to Set up the Time 
**************************************************************/
void Set_Time(void *pVParameters){
		char buff[10] ;
		RTC_TimeTypeDef timebuff;
		HAL_RTC_GetTime(&hrtc,&timebuff,RTC_FORMAT_BIN);
		Hours=timebuff.Hours;
		Mins=timebuff.Minutes;
		Secs=timebuff.Seconds;
		
		for(;;){
			
			xEventGroupWaitBits(Event,SET_TIME,pdTRUE,pdTRUE,portMAX_DELAY);
			time=1;
			Show_Time(Hours,Mins,Secs);
			flag=0;
			
			
		}
			
}
/************************************************************************
Task for setting up the Date
************************************************************************/


void Set_Date(void *pVParameters){
		char buff[10] ;
		RTC_DateTypeDef datebuff;
		RTC_TimeTypeDef timebuff;
		HAL_RTC_GetDate(&hrtc,&datebuff,RTC_FORMAT_BIN);
		Day=datebuff.Date;
		Month=datebuff.Month;
		Year=datebuff.Year;
		
	
		
		for(;;)
	{
			
			xEventGroupWaitBits(Event,SET_DATE,pdTRUE,pdTRUE,portMAX_DELAY);
			j=1;
			Show_Date(Year,Month,Day);
			flag=0;
			
	}
}
/************************************************************************
Task for Setting up the Alarm
*************************************************************************/

void Set_Alarm(void *pVParameters){
	BaseType_t xHigherPriorityTaskWoken =pdFALSE; 
	for(;;)
	{
			xEventGroupWaitBits(Event,SET_ALARM,pdTRUE,pdTRUE,portMAX_DELAY);
			Alarm=1;
			//xEventGroupClearBits(Event,SET_ALARM);
			xEventGroupSetBitsFromISR(Event,SET_DATE,&xHigherPriorityTaskWoken); 
	}
}
/*************************************************************************
EXTI Callback to detect the button presses.
Depending on the which button was pressed ( "main" or "secondary"),the adequate
Debouncing Timer will be called.
Also Starts a 5sec "Sleep" Timer that will put the system to sleep if there is no
button press in 5 seconds. 
*************************************************************************/




void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
		HAL_ResumeTick();		
		BaseType_t xHigherPriorityTaskWoken =pdFALSE; 
		if (GPIO_Pin==GPIO_PIN_12){
		xTimerStartFromISR(Timer,&xHigherPriorityTaskWoken);
		}
		if (GPIO_Pin==GPIO_PIN_13){
		xTimerStartFromISR(Bouton,&xHigherPriorityTaskWoken);	
		}
		xTimerStartFromISR(SLEEP,&xHigherPriorityTaskWoken);
}





/***********************************************************************
Functions to show the date and Time on screen when setting them up or
when setting the alarm
***********************************************************************/


void Show_Date (int Year_,int Month_,int Day_){
			
			char buff[10];
			sprintf(buff,"%d/%d/%d",Day_,Month_,Year_);
			ssd1306_Fill(Black);
			ssd1306_SetCursor(2,0);
			ssd1306_WriteString("Set Date:",Font_7x10,White);
			ssd1306_SetCursor(15,35);
			ssd1306_WriteString(buff,Font_11x18,White);
			ssd1306_UpdateScreen();
		
}
void Show_Time (int	Hours_,int Mins_,int Secs_){
			
			char buff[10];
			sprintf(buff,"%dh:%dm:%ds",Hours_,Mins_,Secs_);
			ssd1306_Fill(Black);
			ssd1306_SetCursor(2,0);
			ssd1306_WriteString("Set Time:",Font_7x10,White);
			ssd1306_SetCursor(5,35);
			ssd1306_WriteString(buff,Font_11x18,White);
			ssd1306_UpdateScreen();
		
}
/*************************************************************
*************************************************************/

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim){
	BaseType_t xHigherPriorityTaskWoken =pdFALSE;	
	i++;

	if(i==5){
	HAL_TIM_PWM_Stop(&htim2,(TIM_CHANNEL_1));
	HAL_TIM_PWM_Stop(&htim1,(TIM_CHANNEL_4));
	i=0;
	xEventGroupSetBitsFromISR(Event,GO_SLEEP,&xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	
}
/**********************************************************************
	When the Alarm is 
**********************************************************************/


void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{ 

	BaseType_t xHigherPriorityTaskWoken =pdFALSE; 	
	HAL_ResumeTick();
	ssd1306_Init();
	ssd1306_Fill(Black);
	ssd1306_SetCursor(2, 0);
	ssd1306_WriteString("SET ALARM",Font_7x10,White);
	ssd1306_UpdateScreen();
	xTimerStopFromISR(Date_,&xHigherPriorityTaskWoken);
	xTimerResetFromISR(SLEEP,&xHigherPriorityTaskWoken);
	htim2.Instance->CCR1|=230;
	HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start_IT(&htim1,TIM_CHANNEL_4);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	
	

}
/*********************************************************
Function to Set up the Alarm
**********************************************************/
void setalarm(int hours,int minutes,int seconds)
{
  RTC_AlarmTypeDef sAlarm ;
  RTC_TimeTypeDef stime;
    //Fill in the alarm structure variable
	HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);   //Get the time when the interrupt is set
	sAlarm.Alarm=RTC_ALARM_A;
	sAlarm.AlarmTime.Hours=hours;
	sAlarm.AlarmTime.Minutes=minutes;
	sAlarm.AlarmTime.Seconds=seconds;

	HAL_RTC_SetAlarm_IT(&hrtc,&sAlarm, RTC_FORMAT_BIN);
}

/*void MPU_Val(MPU6050_t MPU){
	
	char buffer[10]; 
		MPU6050_Init(&hi2c1);
		MPU6050_Read_Accel(&hi2c1,&MPU);
		
		ssd1306_Init();
		ssd1306_Fill(Black);
		ssd1306_SetCursor(2, 0);
		sprintf(buffer,"%.5f",MPU.Ax);
		ssd1306_WriteString("AX=",Font_7x10,White);
		ssd1306_WriteString(buffer,Font_7x10,White);
	  ssd1306_SetCursor(2, 18);
		ssd1306_WriteString("AY=",Font_7x10,White);
		sprintf(buffer,"%.5f",MPU.Ay);
		ssd1306_WriteString(buffer,Font_7x10,White);
		ssd1306_SetCursor(2, 36);
		ssd1306_WriteString("AZ=",Font_7x10,White);
		sprintf(buffer,"%.5f",MPU.Az);
		ssd1306_WriteString(buffer,Font_7x10,White);
		ssd1306_UpdateScreen();
	
}*/
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	
	Timer=xTimerCreate( "Timer",
										pdMS_TO_TICKS(50),
										pdTRUE,
										NULL,
										Timer_Callback );
	SLEEP=xTimerCreate( "Sleep_Timer",
										pdMS_TO_TICKS(5000),
										pdTRUE,
										NULL,
										SLEEP_Callback );
	Bouton=xTimerCreate( "Menu_Choice_Timer",
										pdMS_TO_TICKS(50),
										pdTRUE,
										NULL,
										Menu_Callback );
	Date_=xTimerCreate( "Timer",
										pdMS_TO_TICKS(900),
										pdTRUE,
										NULL,
										Date_Callback );
	xTaskCreate(Set_Time,"SetTime",configMINIMAL_STACK_SIZE,NULL,1,NULL);
	xTaskCreate(Set_Alarm,"SetAlarm",configMINIMAL_STACK_SIZE,NULL,1,NULL);
	xTaskCreate(Set_Date,"SetDate",configMINIMAL_STACK_SIZE,NULL,1,NULL);
	xTaskCreate(vSLEEP,"GO_SLEEP",configMINIMAL_STACK_SIZE,NULL,1,NULL);
	xTaskCreate(vLED,"LED",configMINIMAL_STACK_SIZE,NULL,1,NULL);
  xTaskCreate(vOLED,"OLED",configMINIMAL_STACK_SIZE,NULL,1,NULL);
	Event=xEventGroupCreate();

  //xEventGroupSetBits(Event,NOT_IN_Menu);
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_RTC_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
	//setalarm(0,0,5);
  //char buffer[10]; 
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();
  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

		//MPU_Val(MPU6050);
		//Date();
		
		
		
		
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/