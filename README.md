

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
  


## Understanding the hardware:
Before building our circuit we need to understand the hardware we're using :

### the DHT11 humidity and temperature sensor:
![image](https://user-images.githubusercontent.com/86969450/128260693-fb78d2f9-0f95-48f1-9bd8-9bf41570ac61.png)

The DHT sensors are made of two parts, a capacitive humidity sensor and a thermistor. There is also a very basic chip inside that does some analog to digital conversion and spits out a digital signal with the temperature and humidity. The digital signal is fairly easy to read using any microcontroller.

#### DHT11 Pinout Identification and Configuration:
 
![image](https://user-images.githubusercontent.com/86969450/128260826-39c05af8-e2ee-4c76-b41a-012059d6e8c5.png)

**NB : In our case we'll be using the DHT11 sensor module**

#### Technical details:

* Operating Voltage: 3.5V to 5.5V
* Operating current: 0.3mA (measuring) 60uA (standby)
* Output: Serial data
* Temperature Range: 0°C to 50°C
* Humidity Range: 20% to 90%
* Resolution: Temperature and Humidity both are 16-bit
* Accuracy: ±1°C and ±1%
  
### the RGB LEDs:

 ![image](https://user-images.githubusercontent.com/86969450/128378423-cd65d819-6719-4e48-806f-38448030afcf.png)  
An RGB LED is basically an LED package that can produce almost any color. It can be used in different applications such as outdoor decoration lighting, stage lighting designs, home decoration lighting, LED matrix display, and more.

RGB LEDs are made up of three internal LEDs (Red, Green, and Blue) that may be combined to produce nearly any color. To produce different colors, we must adjust the brightness of each internal LED and combine the three color outputs.
We'll use PWM to adjust the intensity of the red, green, and blue LEDs individually, and the key is that because the LEDs are all so close together inside, our eyes will see the mixture of colors rather than the individual ones.

**NB: the LEDs used in our project are common Cathode**


## Software implementation:
**NB:   
1- you can test your sensor individually via the sensor library link provided [here](https://github.com/RobTillaart/DHTNEW) you can find an implementation example [here](https://github.com/RobTillaart/DHTNew/blob/master/examples/dhtnew_suppressError/dhtnew_suppressError.ino)**\
**2-you can test your Firebase individually via the library link provided [here](https://github.com/mobizt/Firebase-ESP32) you can find an implementation example of anonymous authentification [here](https://github.com/mobizt/Firebase-ESP32/blob/master/examples/Authentications/SignInAsGuest/AnonymousSignin/AnonymousSignin.ino)**  


* Now let's begin with an anonymous authentification code where we upload the data read from the sensor to our database  :
note that this example will create a new anonymous user with  different UID (user ID) every time you run this example.

 First things first , we should begin by including all the libraries we're intending to use :
 
   ```cpp
#include <dhtnew.h> // humidity and temperature sensor library

/* the below if defined and endif bloc will detect the current esp board (32 or 8266) and include the adequate libraries 
if it detects esp32 the it will includeesp32 librairies else it will include esp8266 libraries*/

#if defined(ESP32)
#include <WiFi.h> //wifi library 
#include <FirebaseESP32.h> //firebase for ESP32 library
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#endif

//Provide the token generation process info this will mostly help us debug the progress and the state of token generation 
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions. 
#include "addons/RTDBHelper.h"

  ```
 Now we need to capture our WiFi credentials. Replace `WIFI_ID` with your WiFi Identifier, and `WIFI_PASSWORD` with your WiFi Password.
    
**Note: It’s never a good idea to hardcode password information in your embedded application, for production cases you need to apply a device provision strategy that includes a secure device registration process.**

 ```cpp
 /* 1. Define the WiFi credentials */
#define WIFI_SSID "WIFI_ID"
#define WIFI_PASSWORD "WIFI_PASSWORD"
 ```
Next, we'll create a constant to store our API key; as previously said, you can retrieve your Firebase project API key from the projects settings page.     
`API_KEY` should be replaced with your API key. Replace `URL` with your own Firebase Realtime Database URL.

 ```cpp
#define API_KEY "API_KEY"

/* 3. If work with RTDB, define the RTDB URL */

#define DATABASE_URL "URL" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app
 ```
 We need to define a unique device ID which can used to differentiate data coming from multiple sensors.
 the same should be applied for the Locations
  ```cpp
#define DEVICE_ID "dev-1"

#define LOCATION "Living Room"
 ```
Now we define our 2 rgb LEDs pins
    
 ```cpp
 // Led 1 Pins
#define Led_1_Red 25
#define Led_1_Green 26
#define Led_1_Blue 27

// Led 2 Pins
#define Led_2_Red 2
#define Led_2_Green 4
#define Led_2_Blue 5
 ```
Each RGB LED has 4 pins : one for gnd and 3 pins as input to determine the output color


   
 Next we initialise 3 objects courtesy of the `FirebaseESP32` library which will be critical to linking our application to Firebase.

 ```cpp
 /* 4. Define the Firebase Data object */
FirebaseData fbdo;

/* 5. Define the FirebaseAuth data for authentication data */
FirebaseAuth auth;

/* 6. Define the FirebaseConfig data for config data */
FirebaseConfig config;
 ``` 
Following that, we define a few global variables that will be useful.

 ``` cpp
// Sensor data
float humidity=0.0;
float temperature=0.0;

unsigned long dataMillis = 0;

bool signupOK = false;

// thresholds for temperature & humidity 
//these values represent indoor human confort range
float hum_min_threshhold=30.0;
float hum_max_threshhold=60.0;

float temp_min_threshhold=20.0;
float temp_max_threshhold=30.0;

//notifications concerning sensor values
String SensorState = "OK";
String Warning = "NONE";
int Sensor_Error_Code=0;
 ```
 We also need to create two Firebase Json objects that will hold our sensor data .
 ```cpp
// Json objects containing data
FirebaseJson Tempreature_json;

FirebaseJson Humidity_json;
 ```
 We also need to declare string variables indicating the paths to our json objects containg the sensor data.
 ```cpp
 // declaring string variables containing the data paths
String path="";//base path 
String temp_path="";
String hum_path="";
```
  Finally let's not forget to create a DHT11 sensor object and attach it to an analog pin (15 in our case).
 ```cpp
//creating sensor object attached to pin 15
DHTNEW Sensor(15);
```
Now let's take a look at our setup function :
```cpp
void setup()
{ 
  //Starting serial communication with our ESP32 board
    Serial.begin(115200);
    
    //initializations 
    Led_Init();
    Sensor_init();
    WiFi_init();
    FireBase_init();
    Json_init();
    
    Serial.println(path);
    Serial.println(temp_path);
    Serial.println(hum_path);
    
    Serial.println(String(millis()));
}
```
As you can see First we need to initailize and establish serial communication with our ESP32 board using `Serial.begin(115200)`.  
note that we have used multiple functions In order to initialize our hardware.    
the first function is `Led_init()` which initializes our RGB LEDs as outputs and attaches them to their respective pins.
```cpp
void Led_Init()
{
  
pinMode(Led_1_Red,OUTPUT);
pinMode(Led_1_Green,OUTPUT);
pinMode(Led_1_Blue,OUTPUT);

pinMode(Led_2_Red,OUTPUT);
pinMode(Led_2_Green,OUTPUT);
pinMode(Led_2_Blue,OUTPUT);
    
}
```
The next step is to setup a connection between our board and the wifi network.In order to do that we daclare a function named `WiFi_init()`.  
We make use of the built in WiFi API provided by the Arduino framework.  
We use the `WiFi.begin(WIFI_SSID, WIFI_PASSWORD)`  function to initialise a WiFi connection using our credentials `WIFI_SSID` and `WIFI_PASSWORD`.  
After that check every 300 ms to see if the connection has been successfully established using the `WiFi.status()` function in this case we pirnt the local IP adress using
`WiFi.localIP()`.  
For more detailed informations about WiFi API you might want to check out the [Arduino WiFi API documentation](https://www.arduino.cc/en/Reference/WiFi).

```cpp
void WiFi_init()
{

  //attempting to connect with the wifi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
    Serial.print("Connecting to Wi-Fi");

    //while not connected 
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    
    //Print the local IP adress if connection is successfully established
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
   
}
```
Next, we'll create a Firebase initialization function that connects to the Firebase backend, authenticates our device, and sets up our Firebase library.  
We start by passing our API key and database URL to the configuration object, along with enabling WiFi reconnection on upon setup.  
We then attempt to sign up anonymously as a new user using the function `Firebase.signUp(&config, &auth, "", "")`.  
In order to accomplish anonymous authentification we pass to the function empty mail and password.
After signing up sucessfully we initialze our Firebase library using `Firebase.begin(&config, &auth)`.  
Now we need to determine the **paths** to where to upload our data :
* the base path `path` includes the user ID generated after a successfull sign up ,we use this instruction ` auth.token.uid.c_str()` in order to get the UID.
* temperature path which contains the json objects that holds sensor tempertaure data `temp_path` .
* humidity path which contains the json objects that holds sensor humidity data `hum_path` .  

**For more details you can check the Library link and documentation [here](https://github.com/mobizt/Firebase-ESP32) you can find an implementation example of anonymous authentification [here](https://github.com/mobizt/Firebase-ESP32/blob/master/examples/Authentications/SignInAsGuest/AnonymousSignin/AnonymousSignin.ino)**  

```cpp
void FireBase_init(){
Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    /* Assign the API key (required) */
    config.api_key = API_KEY;

    /* Assign the RTDB URL */
    config.database_url = DATABASE_URL;

    Firebase.reconnectWiFi(true);
    Serial.print("Sign up new user... ");

    /* Sign up */
    if (Firebase.signUp(&config, &auth, "", ""))
    {
        Serial.println("ok");
        signupOK = true;

    }
    else
        Serial.printf("%s\n", config.signer.signupError.message.c_str());

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
    Firebase.begin(&config, &auth);
    path = auth.token.uid.c_str(); //<- user uid
    path+="/";

    // determining the paths
    
    path+=LOCATION;
    temp_path=path+"/temperature";
    hum_path=path+"/humidity";
}
```
The next function is `Json_init()` which initializes our json objects keys and values and assigns them to thier respectives paths
```cpp
void Json_init()
{
Tempreature_json.add("Device ID",DEVICE_ID);
Tempreature_json.add("Value",temperature);
Tempreature_json.add("Warning","NONE");
Tempreature_json.add("Sensor State","OK");

Humidity_json.add("Device ID",DEVICE_ID);
Humidity_json.add("Value",humidity);
Humidity_json.add("Warning","NONE");
Humidity_json.add("Sensor State","OK");
}

```
and finally we initialize our sensor. Sometimes the DHT11 sensors fails to read values correctly and as a result it returns -999.0 so in order to avoid such inconveniences the functions we tend to use the `setSuppressError(true)` function: its main role is to replace the -999.0 error value by the latest value read by the sensor.
  
**For more detailed informations you can check the sensor library link provided [here](https://github.com/RobTillaart/DHTNEW) you can find an implementation example [here](https://github.com/RobTillaart/DHTNew/blob/master/examples/dhtnew_suppressError/dhtnew_suppressError.ino)**

```cpp
void Sensor_init()
{
    Sensor.setSuppressError(true);// avoiding spikes and error values for more detailed info check this  

}

```

Now that we have completed the setup functions, lets get to the loop function.  
As we can see we we use the `if` statement to check whether we are well connected and signed up and to keep our functions running at a constant rate, in our case very 2 seconds.  
We used `Update_Sensor_readings()` function in order to read sensor values and  `Update_data()` in order to upload them to our Firebase realtime database.

```cpp
void loop()
{
    if (millis() - dataMillis > 2000 && signupOK && Firebase.ready())
    {
        dataMillis = millis();
        Update_Sensor_readings();
        Update_data(); 
    }

}
```
Now let's dive into our `Update_Sensor_readings()` function.  
this functions updates error_code and sensor values that will later be uploaded to our database.

```cpp
void Update_Sensor_readings()
{
 // if (millis() - Sensor.lastRead() > 2000){
Sensor_Error_Code = Sensor.read();
humidity=Sensor.getHumidity();
temperature= Sensor.getTemperature();
Serial.println("/*****/");
Serial.println(Sensor_Error_Code);
Serial.println("/*****/");
Serial.println("temperature : ");
Serial.print(temperature);
Serial.println("/*****/");
Serial.println("humidity : ");
Serial.print(humidity);
Serial.println("/*****/");
  }  
}
```
Now the `Led_Signal()` function:
```cpp
void Led_Signal(String Led,byte r,byte g,byte b)
{
  if (Led=="Led 1")
  {
    digitalWrite(Led_1_Red ,r);
    digitalWrite(Led_1_Green ,g);
    digitalWrite(Led_1_Blue ,b);
  
  }
  else if (Led=="Led 2")
  {
    digitalWrite(Led_2_Red ,r);
    digitalWrite(Led_2_Green ,g);
    digitalWrite(Led_2_Blue ,b);
    
  }
}
  
```
Now let's explore the `Update_data()` function that will update our data on the Firebase realtime database.
we begin by setting the json values of temperature and humidity acquired by our sensor, then we compare them with the threshhold , set warning strings and signal diodes accordingly using the `if` statements.  

the system has 2 rgb leds : one for temperature and one for humidity
 *  determining Led signaling outputs dependq on values read by the sensor and the thresholds
 *  Blue led 1 indicates high levels of humidity 
 *  Red led 1 indicates low levels of humidity 
 *  green led 1 indicates that the level of humidity is within human indoor confort range
 *  Blue led 2 indicates low levels of temperature 
 *  Red led 2 indicates high levels of temperature 
 *  green led 2 indicates that the level of temperature is within human indoor confort range

We then set the sensor state value using the `switch`, `case` statement.
Finally we upload our data to the database using `Firebase.updateNode()` .
```cpp
void Update_data()
{

  // setting the values of temperature and humidity
Tempreature_json.set("Value",temperature);
Humidity_json.set("Value",humidity);


/* the system has 2 rgb leds : one for temperature and one for humidity
 *  determining Led signaling outputs dependq on values read by the sensor and the thresholds
 *  Blue led 1 indicates high levels of humidity 
 *  Red led 1 indicates low levels of humidity 
 *  green led 1 indicates that the level of humidity is within human indoor confort range
 */

if (humidity>hum_max_threshhold)
{
  Humidity_json.set("Warning","HUMIDITY ABOVE 60 !!" );
  Led_Signal("Led 1",0,0,255);  
  }
else if (humidity<hum_min_threshhold){
  Humidity_json.set("Warning","HUMIDITY BELOW 30 !!"  );
  Led_Signal("Led 1",255,0,0);
  }
else {
  Humidity_json.set("Warning","NONE");
  Led_Signal("Led 1",0,255,0);
  }


/*
 *  Blue led 2 indicates low levels of temperature 
 *  Red led 2 indicates high levels of temperature 
 *  green led 1 indicates that the level of temperature is within human indoor confort range
 */
        
if (temperature>temp_max_threshhold)
{
  Tempreature_json.set("Warning","TEMPERATURE ABOVE 30 !!");
  Led_Signal("Led 2",255,0,0);
  }
else if (temperature<temp_min_threshhold)
{
  Tempreature_json.set("Warning","TEMPERATURE BELOW 20 !!");
  Led_Signal("Led 2",0,0,255);
  }
else {
  Tempreature_json.set("Warning","NONE");
  Led_Signal("Led 2",0,255,0);
  }

 switch (Sensor_Error_Code)
  {
    case DHTLIB_OK:
      SensorState="OK";
      break;
    case DHTLIB_ERROR_CHECKSUM:
      SensorState="Checksum error";
      break;
    case DHTLIB_ERROR_TIMEOUT_A:
      SensorState="Time out A error";
      break;
    case DHTLIB_ERROR_TIMEOUT_B:
      SensorState="Time out B error";
      break;
    case DHTLIB_ERROR_TIMEOUT_C:
      SensorState="Time out C error";
      break;
    case DHTLIB_ERROR_TIMEOUT_D:
      SensorState="Time out D error";
      break;
    case DHTLIB_ERROR_SENSOR_NOT_READY:
      SensorState="Sensor not Ready : check sensor connections ";
      break;
    case DHTLIB_ERROR_BIT_SHIFT:
      SensorState="Bit shift error";
      break;
    case DHTLIB_WAITING_FOR_READ:
      SensorState="Waiting for read";
      break;
    default:
      SensorState="Unknown ";
      break;
  }


  Tempreature_json.set("Sensor State",SensorState);
  Humidity_json.set("Sensor State",SensorState);


Serial.println("updating");

Firebase.updateNode(fbdo, temp_path , Tempreature_json);
Firebase.updateNode(fbdo, hum_path , Humidity_json);
}
```
you can find the full code for **anonymous authentification [here](https://github.com/mohamedamine99/esp32-iot-temperature-and-humidity-monitoring-with-firebase-real-time-data-base/blob/main/src/dht11__fireBase_Anonymous_auth_.ino)**


---------------------------------------------------------------------------------------------------------------------------------------------------
### Executing the program:
Now is the moment to execute the program and observe the results:
* So as you can see we have sign up anonymous users each having his own token.  
* We can also see that the data are successfully uploaded on our RTDB and are continuously updating in real-time indicating that our values are well monitored.  


![image](https://user-images.githubusercontent.com/86969450/128270751-faebf612-2e97-496e-97e4-26935ed65734.png)
  
![image](https://user-images.githubusercontent.com/86969450/128270765-7b770f5d-9586-4d57-836d-7cde2104b34d.png)
  
![image](https://user-images.githubusercontent.com/86969450/128270796-33b1d534-90a0-4a03-b039-fd7f2855bb86.png)  

Now let's check our board and our LEDs:    
* The LEDs are working and appropriately reflecting the temperature and humidity levels.  
 
![image](https://user-images.githubusercontent.com/86969450/128270911-228a45fa-8b23-4afe-9a9e-a36505edee9d.png)  

### Making an upgrade:

to round up what we have already achieved, we have successfully established a connection between our RTBD and our device but we have only allowed for anonymous sign ups.
In fact the problem with anonymous sign up is that each time we rerun the program a new anonymous user is created even if it's the same actual user who's uploading the data.
this should pose a problem if we ought to keep track of what's going on inside our database, hence, we need for a more advanced method of authentification to overcome this issue.
the simplest solution is to use authentification method with user mail and password , which is not very different than its anonymous counterpart in its code stucture or complexity.  
**you can check the user mail and password authentification code [here](https://github.com/mohamedamine99/esp32-iot-temperature-and-humidity-monitoring-with-firebase-real-time-data-base/blob/main/src/dht11__fireBase_Email_Password_authentification.ino)**

Now we should see the difference : 
* no matter haw many times we run our program the data is always uploaded to the unique user path for each user.

![image](https://user-images.githubusercontent.com/86969450/128424276-27505bc2-8fe9-4d0a-81b1-b42b974128e5.png)
  
![image](https://user-images.githubusercontent.com/86969450/128424299-cbd6ce50-1b9a-441c-a0ce-0071405dfc97.png)

### 3D Conception:
Now that our project works fine we can conceive an enclosure for our device .
In this regard we have used solidworks 2018 to create a 3d conception for this enclosure.  
**You can find the design for our temperature & humidity sensor and other devices [here](https://github.com/mohamedamine99/IOT-devices-enclosures-3d-conception-with-solidworks).**

![image](https://user-images.githubusercontent.com/86969450/128525113-78a6386a-c3d4-4d29-b1a7-9e042ac1ab91.png)




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





