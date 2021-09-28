
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "i2c.h"
#include "rtc.h"
#include "tim.h"
#include "gpio.h"
#include "ssd1306.h"
#include "mpu6050.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "timers.h"
#include "task.h"
#include "event_groups.h"


/*Definitions -----------------------------------------------------------*/


#define DISPLAY_OFF				 0
#define Get_Date				 	(1UL << 0UL)
#define Get_Menu					(1UL << 1UL)
#define GO_SLEEP					(1UL << 2UL)
#define SET_ALARM					(1UL << 3UL)
#define SET_DATE					(1UL << 4UL)
#define SET_TIME					(1UL << 5UL)



/*Variable initialization-----------------------------------------------*/


uint8_t Hours=0x0;
uint8_t Mins=0x0;
uint8_t Secs=0x0;

uint8_t Day=0;
uint8_t Month=0x0;
uint8_t Year=0x0;

uint8_t num_presses=0x0; 			//Number of presses When setting up either Date or Time
												
uint8_t Set_Time=0;						/* Three flags that will be used to determine where in the workflow  the system is currently is.*/
uint8_t Set_Alarm=0;																			
uint8_t Set_Date=0;						/*Will allow to select the right behaviour accordingly*/
uint8_t In_Menu=0;


uint8_t Menu_choice=0; 				//Determine the Menu choice selected by the User

int idle=0;

uint32_t press_duration=0; 		//Determine whether the button press was short of long



		




TimerHandle_t Button1,SLEEP,Button2,Date_ ;
EventBits_t Event_G;
EventGroupHandle_t Event;


/* Prototypes ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
void Date(void);
void Menu(void);
void setalarm(int,int,int);
void Show_Date(int,int,int);
void Show_Time(int,int,int);

/*Functions Implementations ------------------------------------------*/

/**********************************************************************
This Task will be used to Show the Menu on Screen.
**********************************************************************/

void vShowMenu(void *pvParameters){
	
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
by putting vShowTime Task in Running Mode every ~1000 ms
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


void vShowTime(void *pvParameters){
	
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
In the callback we will reset almost all of the GroupBits to reiniliaze the parameters on WakeUp
Then the system will go into STOP_Mode.
STM32 STOP Mode can be woken from by both an EXTI through a button press for example or an RTC Alarm interrupt,
which makes it ideal for a watch.
************************************************************************/
void SLEEP_Callback(void *pvParameters){
	
	BaseType_t xHigherPriorityTaskWoken =pdFALSE;
	xEventGroupSetBitsFromISR(Event,GO_SLEEP,&xHigherPriorityTaskWoken);

}

void vSLEEP(void *pvParameters){
for(;;){	
	BaseType_t xHigherPriorityTaskWoken =pdFALSE; 
	xEventGroupWaitBits(Event,GO_SLEEP,pdTRUE,pdTRUE,portMAX_DELAY);
	xTimerStopFromISR(Date_,&xHigherPriorityTaskWoken);
	ssd1306_SetDisplayOn(DISPLAY_OFF);
	xEventGroupClearBitsFromISR(Event,(Get_Menu|SET_ALARM|SET_DATE));
	In_Menu=0;
	Menu_choice=0;
	xTimerStopFromISR(SLEEP,&xHigherPriorityTaskWoken);
	HAL_SuspendTick();
	

	PWR->CR |= PWR_CR_CWUF; // clear the WUF flag after 2 clock cycles
  PWR->CR &= ~( PWR_CR_PDDS ); // Enter stop mode when the CPU enters deepsleep
  RCC->CFGR |= RCC_CFGR_SWS_HSI ;// HSI16 oscillator is wake-up from stop clock
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; // low-power mode = stop mode
  __WFI();
	}

}

/**********************************************************************************
The TimerCallback is used to debounce the "Main" button presses as well as determine what kind of press it is:
Short Press:To show Date /navigate through menu.
Long Press :To get access to the Menu.
It is a 50ms Timer that is started (Through EXTI Callback) whenever there is Button Press Detected
*The "press_duration" variable is incremented whenever  every 50ms the button is still being pressed and lets determine
whether it is a long press or a short one.
*Flage variable is set to 1 if the program is currently present in the menu
The Callback Treats 4 Different Scenarios:
1- A short press + Not in menu (In_Menu=0) : This will get us the Date on the screen
2- A Long  press + Not in menu (In_Menu=0)	: This will allow us to proceed to the menu
3- A Long press  + In Menu (In_Menu=1)			:	This will allow us to choose one of the 2 menu options (Change Date 
																																																	set Alarm)
4- A Button press whenever we are in the SETDATE/SETALARM Menu to validate the input on the screen
***********************************************************************************/

void Button1_Callback(void *pvParameters){
	
	BaseType_t xHigherPriorityTaskWoken =pdFALSE; 
	Event_G=xEventGroupGetBitsFromISR(Event);
	press_duration++;
	
	if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==0 && press_duration < 15 && In_Menu==0 && ( Set_Date 	^  Set_Time )==0){
		press_duration=0;
		xEventGroupSetBitsFromISR(Event,Get_Date,&xHigherPriorityTaskWoken);
		xTimerStartFromISR(Date_,&xHigherPriorityTaskWoken);
		xTimerStop(Button1,portMAX_DELAY);	
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	
	else if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==0 && press_duration > 15 && In_Menu==0)
	{
		press_duration=0;	
		if(idle>0){
		xTimerStopFromISR(Date_,&xHigherPriorityTaskWoken);
		idle=0;
		}
		xEventGroupClearBitsFromISR(Event,Get_Date);
		xEventGroupSetBitsFromISR(Event,Get_Menu,&xHigherPriorityTaskWoken);
		In_Menu=1;
		xTimerStop(Button1,portMAX_DELAY);	
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	
	else if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==0  &&   In_Menu==1){
		press_duration=0;
		switch (Menu_choice){
			case 0 :
					xEventGroupSetBitsFromISR(Event,SET_ALARM,&xHigherPriorityTaskWoken);
					break;
			case 1 :
					xEventGroupSetBitsFromISR(Event,SET_DATE,&xHigherPriorityTaskWoken);
					break;

			}
				xTimerStop(Button1,portMAX_DELAY);
				portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	else if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12)==0   && ( Set_Date 	|  Set_Time  | Set_Alarm ) != 0 ){
			RTC_DateTypeDef datebuff;
			RTC_TimeTypeDef timebuff;
			xTimerStop(Button1,portMAX_DELAY);
			press_duration=0;
			num_presses++;
			if (num_presses==3){
				if(Set_Date==1 && Set_Alarm==0) { 		//Setting Date
					datebuff.Date=Day;
					datebuff.Month=Month;
					datebuff.Year=Year;
					Set_Date=0;
					num_presses=0;
					HAL_RTC_SetDate(&hrtc,&datebuff,RTC_FORMAT_BIN);
					HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2);
					xEventGroupSetBitsFromISR(Event,SET_TIME,&xHigherPriorityTaskWoken);
			}
				if(Set_Time==1	&&	Set_Alarm==0){ //Setting Time
					timebuff.Seconds=Secs;
					timebuff.Minutes=Mins;
					timebuff.Hours=Hours;
					Set_Time=0;
					num_presses=0;
					HAL_RTC_SetTime(&hrtc,&timebuff,RTC_FORMAT_BIN);
					HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, 0x32F2);

			}
				if(Set_Alarm==1 && Set_Date==1){		//Setting the date associated to the Alarm
					num_presses=0;
					Set_Date=0;
					xEventGroupSetBitsFromISR(Event,SET_TIME,&xHigherPriorityTaskWoken);
				}
				if(Set_Alarm==1 && Set_Time ==1 ){ //Setting the time associated to the Alarm
					num_presses=0;
					Set_Time=0;
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
2-Changing the values in Real-Time on the screen when setting up the Time or the Alarm
*********************************************************************/
void Button2_Callback(void *pvParameters){
	
	BaseType_t xHigherPriorityTaskWoken =pdFALSE; 
	if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_13)==0 && In_Menu==1){
		if(Menu_choice==1){
			Menu_choice=0;
		}
		else {Menu_choice=1;}
		
		xEventGroupSetBitsFromISR(Event,Get_Menu,&xHigherPriorityTaskWoken);
		xTimerStop(Button2,portMAX_DELAY);	
	
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
 }
	if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_13)==0  && Set_Date==1){
		switch (num_presses){
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
			xTimerStop(Button2,portMAX_DELAY);
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	if (HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_13)==0  && Set_Time==1){
		switch (num_presses){
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
			xTimerStop(Button2,portMAX_DELAY);
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}

void Menu(){
		
	ssd1306_Init();
	In_Menu=1;
	ssd1306_Fill(Black);
	if (Menu_choice==0){
		ssd1306_SetCursor(2, 0);
		ssd1306_WriteString("> SET ALARM",Font_7x10,White);
		ssd1306_SetCursor(2, 18);
		ssd1306_WriteString(" CHANGE TIME",Font_7x10,White);
		ssd1306_UpdateScreen();
	
	}
		else if (Menu_choice == 1){
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
void SetTime(void *pVParameters){
		char buff[10] ;
		RTC_TimeTypeDef timebuff;
		HAL_RTC_GetTime(&hrtc,&timebuff,RTC_FORMAT_BIN);
		Hours=timebuff.Hours;
		Mins=timebuff.Minutes;
		Secs=timebuff.Seconds;
		
		for(;;){
			
			xEventGroupWaitBits(Event,SET_TIME,pdTRUE,pdTRUE,portMAX_DELAY);
			Set_Time=1;
			Show_Time(Hours,Mins,Secs);
			In_Menu=0;
			
			
		}
			
}
/************************************************************************
Task for setting up the Date
************************************************************************/


void SetDate(void *pVParameters){
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
			Set_Date=1;
			Show_Date(Year,Month,Day);
			In_Menu=0;
			
	}
}
/************************************************************************
Task for Setting up the Alarm
*************************************************************************/

void SetAlarm(void *pVParameters){
	BaseType_t xHigherPriorityTaskWoken =pdFALSE; 
	for(;;)
	{
			xEventGroupWaitBits(Event,SET_ALARM,pdTRUE,pdTRUE,portMAX_DELAY);
			Set_Alarm=1;
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
		xTimerStartFromISR(Button1,&xHigherPriorityTaskWoken);
		}
		if (GPIO_Pin==GPIO_PIN_13){
		xTimerStartFromISR(Button2,&xHigherPriorityTaskWoken);	
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
	static int i=0;
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




/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);


/* Main Function -------------------------------------------------------------*/

int main(void)
{
  /* USER CODE BEGIN 1 */
	
	Button1=xTimerCreate( "Timer",
										pdMS_TO_TICKS(50),
										pdTRUE,
										NULL,
										Button1_Callback );
	SLEEP=xTimerCreate( "Sleep_Timer",
										pdMS_TO_TICKS(5000),
										pdTRUE,
										NULL,
										SLEEP_Callback );
	Button2=xTimerCreate( "Menu_Choice_Timer",
										pdMS_TO_TICKS(50),
										pdTRUE,
										NULL,
										Button2_Callback );
	Date_=xTimerCreate( "Timer",
										pdMS_TO_TICKS(900),
										pdTRUE,
										NULL,
										Date_Callback );
	xTaskCreate(SetTime,"SetTime",configMINIMAL_STACK_SIZE,NULL,1,NULL);
	xTaskCreate(SetAlarm,"SetAlarm",configMINIMAL_STACK_SIZE,NULL,1,NULL);
	xTaskCreate(SetDate,"SetDate",configMINIMAL_STACK_SIZE,NULL,1,NULL);
	xTaskCreate(vSLEEP,"GO_SLEEP",configMINIMAL_STACK_SIZE,NULL,1,NULL);
	xTaskCreate(vShowTime,"ScreenRefresh",configMINIMAL_STACK_SIZE,NULL,1,NULL);
  xTaskCreate(vShowMenu,"Menu",configMINIMAL_STACK_SIZE,NULL,1,NULL);
	
	Event=xEventGroupCreate();



 
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_RTC_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();


  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();
  /* Start scheduler */
  osKernelStart();

  while (1)
  {

		
		
		
		

  }

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

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }


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



void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

  if (htim->Instance == TIM4) {
    HAL_IncTick();
  }

}


void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }

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
#endif 

