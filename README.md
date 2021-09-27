

# STM32 based DIGITAL-Clock 

## Table of contents

<!-- TABLE OF CONTENTS -->
<details open="open">
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#about-the-project">About The Project</a></li>  
    <li><a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#hardware-requirements">Hardware Requirements</a></li>
        <li><a href="#software-requirements">Software Requirements</a></li>
        <li><a href="#backend-setup">Backend Setup</a></li>
      </ul>
    </li>
    <li><a href="#understanding-the-hardware">Understanding the hardware</a>
      <ul>
        <li><a href="#the-dht11-humidity-and-temperature-sensor">the DHT11 humidity and temperature sensor</a></li>
        <li><a href="#the-rgb-leds">the RGB LEDs</a></li>
      </ul>
 </li> 
    <li><a href="#software-implementation">Software implementation</a></li>
    <li><a href="#executing-the-program">Executing the program</a></li>
    <li><a href="#making-an-upgrade">Making an upgrade</a></li>
    <li><a href="#3d-conception">3D Conception</a></li>
    <li><a href="#conclusion-and-perspective">Conclusion and perspective</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgements">Acknowledgements</a></li>
    
    
  </ol>
</details>

   
## About the project

![image](https://user-images.githubusercontent.com/86969450/128428449-4470b309-326e-4470-a66a-8fdc706914c3.png)



This projects aims on implementing a Digital clock with a simple GUI to allow the user to do basic operations like change up the time/date or set up an alarm
While FreeRTOS isn't particularly needed,it greatly simplifies the implementation with the different mechanism it offers to ensure sequential and synchronized behaviour 

<!-- GETTING STARTED -->
## Getting Started

### Hardware Requirements:
* An STM32 microcontroller (for this project I used an STM32F103 bluepill).
* An ST-Link for flashing/Debugging (Other development boards like Nucleo offer on-board Flasher/Debugger)
* An SSD1306 OLED 
* x2 Buttons that will be used to interface with the GUI
* x2 10KOhm resistors to be wired with the pushbuttons
* A Buzzer ( While optional ,an amplifier is recommended as most Low Cost buzzers barely make a hearable sound)
* Wires

### Software Requirements:
* STMCubeMX will be used to set all peripherals and clocks
* SSD1306 OLED library made by Afiskon (https://github.com/afiskon/stm32-ssd1306/tree/master/ssd1306)
* GNU Make or an IDE like Keil/IAR depending on your CubeMX project configuration 



### Setting up the Peripherals using STMCubeMX:

In the project we will need these peripherals:
* I2C1 (PB7 SDA and PB6 SCL) will be used in order to communicate with the Display
* an RTC will be utilized to increment the time
* Timer1 to Generate a PWM signal to make the Buzzer beep.
* Timer2 will be used to stop the PWM ganerated by Timer1
 
Lets look in detail at the configuartion

Timer1 will utilize the internal clock of the MCU and will generate a  PWM on channel 4 (could use any of the channels)  
Will activate interrupts associated to it which will allow us to stop the PWM the moment the counter overflows.  
Setting up the adequate Prescaler and Counter Period to obtain a 1s period using the formula in the datasheet  
![image](https://user-images.githubusercontent.com/33790012/134830553-c2c42971-5329-420c-be14-24cead341909.png)
Considering we will use the MCU at 8MHz =>TIM_CLK=8MHZ by setting the prescalor to 7999 and Autoreload register to 999 we get an update event of 1000=1s which is what we want  
![image](https://user-images.githubusercontent.com/33790012/134830754-8a1facf7-cdb5-4b2c-adba-faf07ff86c40.png)

The RTC will be used to increment the time but also to generate interrupts when the Alarm value entered by the User is reached.    
In CubeMX you can just put placeholder values everywhere in its configuration,we will modify them in the Code.  
Should set the format to BCD and also activate both RTC Golabal interrupt and the RTC alarm interrupt through the NVIC.  
![image](https://user-images.githubusercontent.com/33790012/134831014-5cd7843c-5760-461d-8528-b500ccbe1066.png)

Enabling FreeRTOS CMSIS_V2 under the Middleware tab.  
![image](https://user-images.githubusercontent.com/33790012/134892873-0381c9e5-31a9-4a10-a59f-24b5f08af792.png)


### CubeMX Project Configuration
In the project Tab , choose the adequate Toolchain to use , be it a Makefile or an IDE
![image](https://user-images.githubusercontent.com/33790012/134831179-60f7bfda-a1f6-4e66-91cc-853eb9afd387.png)
In the Code Generator Tab :  
1.Check copy only the necessary library files  
2.Generate peripheral initialization as a pair of .c/.h files to an unbloated  main.c file (optional)
![image](https://user-images.githubusercontent.com/33790012/134831327-1831aca3-a477-4ad2-bd14-8ae88c60d324.png)
  


## Software  Breakdown:
The  application will rely on FreeRTOS to ensure synchronization between the differents tasks and also to improve  the readabilty of the code.  
Each unique element of the system will have its own task and Different.  
We have all in all 6 different tasks that implement the following behaviour of the system :  
* Set_Time : For setting up the time of the Clock
* Set_Alarm : For setting up the Alarm timing
* Set_Date : Allows the user to change the Date
* Sleep : Will put the system in a low power mode (STM32 STOP MODE) if there is no user interaction
* Show_Time :Will display on the screen the current time
* Show_Menu :Will display on the screen the menu 

Additinally we will use 4 Software Timers from FreeRTOS API :

                                          Date Timer  :
                                        
This Timer will allow to update the time on Screen every Second by putting vShowTime Task in Running Mode every ~1000 ms.  

                                          Sleep Timer :  
                                        
The Sleep Timer is set whenever there is a bouton press (which showcases a user interaction).  
It is set to 5sec which means that if there are no button pressed in 5sec the system will go to Sleep.  
In the callback we will reset almost all of the GroupBits to reiniliaze the parameters on WakeUp.  
Then the system will go into STOP_Mode.  
STM32 STOP Mode can be woken from by both an EXTI through a button press for example or an RTC Alarm interrupt,which makes it ideal for a watch. 

                                          Button1_Timer :  
                                         
The TimerCallback is used to debounce the "Main" button presses as well as determine what kind of press it is  
It is a 50ms Timer that is started (Through EXTI Callback) whenever there is Button Press Detected  
Is Capable of differentitationg between short and long presses   
The Callback Treats 4 Different Scenarios:  
1. A short press + Not in menu                          : This will get us the Date on the screen  
2. A Long  press + Not in menu 	                        : This will allow us to proceed to the menu  
3. A Long press  + In Menu 			                        :	This will allow us to choose one of the 2 menu options (Change Date or set Alarm)  
4. A short press + In (Set_Time) or (set_Date) Menus    : Will allow to validate the input displayed on the screen  

                                          Button2_Timer :  
                                        
Button2_Callback will serve as way to debounce the "secondary button".  
This button will serve as a way as to navigate through the different menus and change the values.  
Will use this button for 2 Cases :  
1-When we are in the main menu to choose between the differents Options (Set_Alarm or Set_Date)  
2-Changing the values in Real-Time on the screen when setting up the Time ,Date or the Alarm.

To synchronize between all these tasks we will use Group_Events and we will assign to each task a different event_bit.  
Event bits are very convenient because they can be set and cleared from ISRs . 
![image](https://user-images.githubusercontent.com/33790012/134966984-8231665d-9c06-431c-8624-889a97747f11.png)

#### General workflow  
Since the entirety of interaction between the User and Clock is done through Button presses,  
1. The program entry point will always be the EXTI handler (when a button is pressed) which will then start the corresponding debounce routine.  
2. The timer callback will notify  one of the Program's  4 Tasks by setting their corresponding GroupBits according to the value of different flags.  
3. The task will do what it has to do and then go idle.  
4. The Clock will go to sleep if there is no button press for 5 seconds ortherwise will go back to point 1.  

#### Debouncing routine  
Since mostly use buttons to interact with the watch,we needed to include a debounce routine to filter all the noise and get an accurate press count.  
For this we use a timer with a 50ms period that will get started when there is a button press (through EXTI Callback).  
During the Timer Callback ,the system will assess the state of the Button and if he finds it 0 that means there was a one button press.
Generally a 50ms period is  enough to drive away most of the noise of mechanical pushup Buttons.
![image](https://user-images.githubusercontent.com/33790012/134999045-49e0320e-30e8-4229-92d2-08cf144deed2.png)

### Gifs showcasing the different elements implemented in the Digital Clock  

#### Screen Update


#### Setting up the Date and the time  



#### Setting up an Alarm


#### Alarm getting triggered

#### System going to Sleep and then waking up when a Button is pressed




### Conclusion and perspective:

To sum up, in this tutorial we learned how to interface sensors and actuators with our ESP32 DEV kit V-1 , we learned how to avoid incorrect readings from the DHT11 sensor,how to properly control RGB LEDs colors and how to upload sensor values and update them continuously on our Firebase RealTime Database (RTBD) using anonymous sign up at first and then using a more advanced authentification method with user mail and password.  
**However**, improvements can be made. In fact, as we can a see there is a bit of lag or delay between the data dispalyed our arduino IDE serial monitor and the data displayed on our firebase realtime database.  
Luckily for us there is a fairly easy solution for that using **multitasking** since the esp32 board allows **multithreading** thanks to its **dual Core architecture**.
(Please refer to the [multithreading readme](https://github.com/mohamedamine99/esp32-iot-temperature-and-humidity-monitoring-with-firebase-real-time-data-base/blob/main/src/Multi-threading%20Read%20me.md) , [multithreading code](https://github.com/mohamedamine99/esp32-iot-temperature-and-humidity-monitoring-with-firebase-real-time-data-base/blob/main/src/dht11_fireBase_multi_thread_Email_Password_auth_git.ino) in the [src](https://github.com/mohamedamine99/esp32-iot-temperature-and-humidity-monitoring-with-firebase-real-time-data-base/tree/main/src) file for more informations)

### Contact:
* Mail : mohamedamine.benabdeljelil@insat.u-carthage.tn -- mohamedaminebenjalil@yahoo.fr
* Linked-in profile: https://www.linkedin.com/in/mohamed-amine-ben-abdeljelil-86a41a1a9/

### Acknowledgements:
* [mobizt](https://github.com/mobizt) for making the [ESP32 Firebase library](https://github.com/mobizt/Firebase-ESP-Client)
* [Rob Tillaart](https://github.com/RobTillaart) for making the [DHT11-22 library](https://github.com/RobTillaart/DHTNEW)
* [othneildrew](https://github.com/othneildrew) for making the [readme templates](https://github.com/othneildrew/Best-README-Template)





