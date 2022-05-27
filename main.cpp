#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <Arduino.h>


/*
Project: automatic irrigation system for arduino with website
Author: kl0ibi
date: 2022-05-27
MCU: Arduino Mega 2560
external MCU: esp32
*/

//constants
//Valve
#define VALVE_LEFT_TOP 0
#define VALVE_LEFT_BOTTOM 1
#define VALVE_RIGHT_TOP 2
#define VALVE_RIGHT_BOTTOM 3
//LDR
#define LDR 4
//humidity sensor outdoor
#define HUMIDITY_SENSOR_OUTDOOR 5
//temperature sensor
#define TEMPERATURE_SENSOR 16
//humidity sensor earth
#define HUM_LEFT_TOP 6
#define HUM_LEFT_BOTTOM 7
#define HUM_RIGHT_TOP 8
#define HUM_RIGHT_BOTTOM 9
//rain sensor
#define RAIN_SENSOR 10
//voltage sensor
#define VOLTAGE_SENSOR 11
//hall effect sensor
#define HALL_LEFT_TOP 12
#define HALL_LEFT_BOTTOM 13
#define HALL_RIGHT_TOP 14
#define HALL_RIGHT_BOTTOM 15

#define HUMIDITY_THRESHOLD_LOW 40
#define HUMIDITY_THRESHOLD_HIGH 60
#define TIME_AFTER_WATERING 300
#define WATERING_TIME 30

volatile uint32_t millis_counter = 0;
volatile uint32_t seconds_counter = 0;
volatile double usedWater = 0;

float humidity=80;
float temperature=23.5;
float voltage=12.4;
int hum_right_top=343;
int hum_right_bottom=342;
int hum_left_top=34;
int hum_left_bottom=34; 
int rain=0;
int light=0;


#define WINDOW_SIZE		5

int value_hum_left_top[WINDOW_SIZE] = {};
int value_hum_left_bottom[WINDOW_SIZE] = {};
int value_hum_right_top[WINDOW_SIZE] = {};
int value_hum_right_bottom[WINDOW_SIZE] = {};

int filtered_hum_left_top[WINDOW_SIZE] = {};
int filtered_hum_left_bottom[WINDOW_SIZE] = {};
int filtered_hum_right_top[WINDOW_SIZE] = {};
int filtered_hum_right_bottom[WINDOW_SIZE] = {};

bool water_left_top = false;
bool water_left_bottom = false;
bool water_right_top = false;
bool water_right_bottom = false;

bool valve_left_top_locked = false;
bool valve_left_bottom_locked = false;
bool valve_right_top_locked = false;
bool valve_right_bottom_locked = false;

bool timer_isRunning = false;


char newDay_keyword[] = "newDay";
char recievedChar;
char * strtokIndx;
char buf[20];

enum States{
ST_BOOTUP,
ST_SENSOR,
ST_WATERING,
ST_RAIN,
ST_CLOSED,
ST_ERROR
};

enum Events{
EV_HUM,
EV_HALL1,
EV_HALL2,
EV_HALL3,
EV_HALL4,
EV_RAIN,
EV_TIMEOUT,
EV_TIMEOUT0,
EV_TIMEOUT3,
EV_TIMEOUT4,
EV_TIMEOUT5,
EV_NONE
};

volatile enum States curState = ST_SENSOR;
volatile enum Events curEvent = EV_NONE;



int readline(int readch, char *buffer, int len) {
  static int pos = 0;
  int rpos;
  if (readch > 0) {
    switch (readch) {
      default:
        if (pos < len - 1) {
          buffer[pos++] = readch;
          buffer[pos] = 0;
        }
      case '\r': // Ignore CR
        break;
      case '\n': // Return on new-line
        rpos = pos;
        pos = 0;  // Reset position index ready for next time
        return rpos;

    }
  }
  return 0;
}


void checkSerialInput() {
  strcpy(buf, "");
  while (Serial.available())  {
    readline(Serial.read(), buf, 80);
  }
  if ((buf[0] != NULL)) {

    strtokIndx  = strtok(buf, ",");
    
    if (strcmp(strtokIndx, newDay_keyword) == 0) {
      usedWater= 0;
    }
    else {
      Serial.println("Unknown Command: " + String(strtokIndx));
    }

  }
}






void sendDatatoEsp32()
{
  static float data[8] = {0, 0, 0, 0 ,0 ,0 ,0, 0};
  //static int temp_data[2]= {0,0};
      data[0] = humidity;
      data[1] = temperature;
      data[2] = voltage;
      data[3] = hum_right_top;
      data[4] = hum_right_bottom;
      data[5] = hum_left_top;
      data[6] = hum_left_bottom;
      data[7] = usedWater;

      Serial.println(String(data[0]) + "," + String(data[1]) + "," + String(data[2]) + "," + String(data[3]) + "," + String(data[4]) + "," + String(data[5]) + "," + String(data[6]) + "," + String(data[7]));
}




void init_pins()
{
  /*
  * 4 Valve Pins
  * atleast 4 humidity sensors for watering
  * 1 Pin for rainsensor
  * 1 Pin for light sensor
  * 1 Pin for voltage sensor
  * 4 Pins for hall effect sensors
  */

  pinMode(VALVE_LEFT_BOTTOM, OUTPUT);
  pinMode(VALVE_LEFT_TOP, OUTPUT);
  pinMode(VALVE_RIGHT_BOTTOM, OUTPUT);
  pinMode(VALVE_RIGHT_TOP, OUTPUT);
  pinMode(LDR, INPUT);
  pinMode(HUMIDITY_SENSOR_OUTDOOR, INPUT);
  pinMode(HUM_LEFT_TOP, INPUT);
  pinMode(HALL_RIGHT_TOP, INPUT);
  pinMode(HALL_RIGHT_BOTTOM, INPUT);
}


void init_Timer1()
{
  //just for debugging not calculated time
  //1ms Timer
  TCCR1A=0x00;
  TCCR1B|=(1<<WGM12);
  TIMSK1|=(1<<OCIE1A);
  OCR1A=200;
}

void Timer3_init()
{
  //just for debugging not calculated time
  //1ms Timer
  TCCR3A=0x00;
  TCCR3B|=(1<<WGM32);
  TIMSK3|=(1<<OCIE3A);
  OCR3A=200;
}

void Timer4_init()
{
  //just for debugging not calculated time
  //1ms Timer
  TCCR4A=0x00;
  TCCR4B|=(1<<WGM42);
  TIMSK4|=(1<<OCIE4A);
  OCR4A=200;
}

void Timer5_init()
{
  //just for debugging not calculated time
  //1ms Timer
  TCCR5A=0x00;
  TCCR5B|=(1<<WGM52);
  TIMSK5|=(1<<OCIE5A);
  OCR5A=200;
}

void Timer0_init()
{
  //just for debugging not calculated time
  //1ms Timer
  //TCCR0A=0x00;
  //TCCR0B|=(1<<WGM02);
 // TIMSK0|=(1<<OCIE0A);
  //OCR0A=200;
}



void timer_start()
{
  //start timer
  TCCR1B|=(1<<CS11)|(1<<CS10);
}

void timer_stop()
{
  //stop timer
  TCCR1B&=~((1<<CS11)|(1<<CS10));
}

void timer0_start()
{
  //start timer
  TCCR0B|=(1<<CS02)|(1<<CS00);
}

void timer0_stop()
{
  //stop timer
  TCCR0B&=~((1<<CS02)|(1<<CS00));
}

void timer3_start()
{
  //start timer
  TCCR3B|=(1<<CS32)|(1<<CS30);
}

void timer3_stop()
{
  //stop timer
  TCCR3B&=~((1<<CS32)|(1<<CS30));
}

void timer4_start()
{
  //start timer
  TCCR4B|=(1<<CS42)|(1<<CS40);
}

void timer4_stop()
{
  //stop timer
  TCCR4B&=~((1<<CS42)|(1<<CS40));
}

void timer5_start()
{
  //start timer
  TCCR5B|=(1<<CS52)|(1<<CS50);
}

void timer5_stop()
{
  //stop timer
  TCCR5B&=~((1<<CS52)|(1<<CS50));
}

double calculatedWater()
{
  //0.6309 l/min

   return (((double)millis_counter/1000)*0.6309);
}

void readSensor()
{
  //read sensors
  humidity = analogRead(HUMIDITY_SENSOR_OUTDOOR);
  temperature = analogRead(TEMPERATURE_SENSOR);
  voltage = analogRead(VOLTAGE_SENSOR);
  hum_right_top = analogRead(HUM_RIGHT_TOP);
  hum_right_bottom = analogRead(HUM_RIGHT_BOTTOM);
  
  hum_left_bottom = analogRead(HUM_LEFT_BOTTOM);
  //read rain sensor
  rain = digitalRead(RAIN_SENSOR);
  //read light sensor
  light = analogRead(LDR);
}



int compare_int(const void *a, const void *b)
{
	int *x = (int *)a;
	int *y = (int *)b;
	return *x - *y;
}

int median_filter_2(int feld[WINDOW_SIZE])
{
	int temp[WINDOW_SIZE];
	int median;

	memcpy(temp, feld, WINDOW_SIZE*sizeof(int));

	//search median (sort temp-array and take the middle)
	qsort(temp, WINDOW_SIZE, sizeof(int), compare_int);
	median = temp[WINDOW_SIZE / 2];

	return median;
}

int average_filter(int feld[], int anz)
{
	int average;
	int sum = 0;

	for (int i = 0; i < anz; i++)
		sum += feld[i];

	average = sum / anz;
	return average;
}



int main()
{
  Serial.begin(115200);
  init_pins();
  init_Timer1();
  sei();

  while(1)
  {
    switch(curState)
      {
        case ST_BOOTUP:
        //wait for serial connection with esp32
        if(Serial.available()>0)
        {
          Serial.println("Serial connection established");
          curState=ST_SENSOR;
         
        }
        break;
        case ST_SENSOR:
        //read sensors
        for (int j=0; j< WINDOW_SIZE; j++)
        {
        for(int i=0; i < WINDOW_SIZE; i++)
         {
         
            value_hum_left_bottom[i] = analogRead(HUM_LEFT_BOTTOM);
          
            value_hum_left_top[i] = analogRead(HUM_LEFT_TOP);
          
            value_hum_right_bottom[i] = analogRead(HUM_RIGHT_BOTTOM);
          
            value_hum_right_top[i] = analogRead(HUM_RIGHT_TOP);
         }
          filtered_hum_left_bottom[j] = median_filter_2(value_hum_left_bottom);
          filtered_hum_left_top[j] = median_filter_2(value_hum_left_top);
          filtered_hum_right_bottom[j] = median_filter_2(value_hum_right_bottom);
          filtered_hum_right_top[j] = median_filter_2(value_hum_right_top);
        }
        //calculate average of filtered values
        hum_left_bottom = average_filter(filtered_hum_left_bottom, WINDOW_SIZE);
        hum_left_top = average_filter(filtered_hum_left_top, WINDOW_SIZE);
        hum_right_bottom = average_filter(filtered_hum_right_bottom, WINDOW_SIZE);
        hum_right_top = average_filter(filtered_hum_right_top, WINDOW_SIZE);

        if(hum_left_bottom > HUMIDITY_THRESHOLD_LOW && hum_left_bottom < HUMIDITY_THRESHOLD_HIGH && !valve_left_bottom_locked)
        {
          water_left_bottom=true;
          curEvent=EV_HUM;
        }
        
        if(hum_left_top > HUMIDITY_THRESHOLD_LOW && hum_left_top < HUMIDITY_THRESHOLD_HIGH && !valve_left_top_locked)
        {
          water_left_top=true;
          curEvent=EV_HUM;
        }

        if(hum_right_bottom > HUMIDITY_THRESHOLD_LOW && hum_right_bottom < HUMIDITY_THRESHOLD_HIGH && !valve_right_bottom_locked)
        {
          water_right_bottom=true;
          curEvent=EV_HUM;
        }

        if(hum_right_top > HUMIDITY_THRESHOLD_LOW && hum_right_top < HUMIDITY_THRESHOLD_HIGH && !valve_right_top_locked)
        {
          water_right_top=true;
          curEvent=EV_HUM;
        }

        if(curEvent==EV_HUM)
        {
          curState=ST_WATERING;
          curEvent=EV_NONE;
        }
        break;
        case ST_WATERING:
        //valve open
        if(water_left_bottom)
        {
          valve_left_bottom_locked=true;
          water_left_bottom=false;
          digitalWrite(VALVE_LEFT_BOTTOM, HIGH);
        }
        if(water_left_top)
        {
          valve_left_top_locked=true;
          water_left_top=false;
          digitalWrite(VALVE_LEFT_TOP, HIGH);
        }
        if(water_right_bottom)
        {
          valve_right_bottom_locked=true;
          water_right_bottom=false;
          digitalWrite(VALVE_RIGHT_BOTTOM, HIGH);
        }
        if(water_right_top)
        {
          valve_right_top_locked=true;
          water_right_top=false;
          digitalWrite(VALVE_RIGHT_TOP, HIGH);
        }
        //wait for watering
        if(timer_isRunning==false)
        {
          timer_isRunning=true;
          timer_start();
        }
        if(curEvent==EV_TIMEOUT)
        {
          timer_stop();
          //valve close
          digitalWrite(VALVE_LEFT_BOTTOM, LOW);
          digitalWrite(VALVE_LEFT_TOP, LOW);
          digitalWrite(VALVE_RIGHT_BOTTOM, LOW);
          digitalWrite(VALVE_RIGHT_TOP, LOW);
          curState=ST_SENSOR;
          curEvent=EV_NONE;

          if(valve_left_bottom_locked)
          {
            timer0_start();
          }

          if(valve_left_top_locked)
          {
            timer5_start();
          }

          if(valve_right_bottom_locked)
          {
            timer3_start();
          }

          if(valve_right_top_locked)
          {
            timer4_start();
          }
        }

        

        if(curEvent==EV_TIMEOUT0)
        {
          timer0_stop();
          valve_left_bottom_locked=false;
        }

        if(curEvent==EV_TIMEOUT3)
        {
          timer3_stop();
          valve_right_bottom_locked=false;
        }

        if(curEvent==EV_TIMEOUT4)
        {
          timer4_stop();
          valve_right_top_locked=false;
        }

        if(curEvent==EV_TIMEOUT5)
        {
          timer5_stop();
          valve_left_top_locked=false;
        }
        
        break;
        case ST_RAIN:
        Serial.println("Rain detected");
        break;
        case ST_CLOSED:
        Serial.println("Lid closed");
        break;
        case ST_ERROR:
        Serial.println("Error");
        break;
      }

      sendDatatoEsp32();

  }
}

ISR(TIMER1_COMPA_vect)
{
  millis_counter++;
  if(millis_counter>=WATERING_TIME)
  {
    millis_counter=0;
    seconds_counter++;
  }
  usedWater=calculatedWater();
}

ISR(TIMER1_OVF_vect)
{
  millis_counter++;
  if(millis_counter>=TIME_AFTER_WATERING)
  {
    millis_counter=0;
    seconds_counter++;
    curEvent=EV_TIMEOUT3;
  }
}

ISR(TIMER3_COMPA_vect)
{
  millis_counter++;
  if(millis_counter>=TIME_AFTER_WATERING)
  {
    millis_counter=0;
    seconds_counter++;
    curEvent=EV_TIMEOUT3;
  }
}

ISR(TIMER4_COMPA_vect)
{
  millis_counter++;
  if(millis_counter>=TIME_AFTER_WATERING)
  {
    millis_counter=0;
    seconds_counter++;
    curEvent=EV_TIMEOUT4;
  }
}

ISR(TIMER5_COMPA_vect)
{
  millis_counter++;
  if(millis_counter>=TIME_AFTER_WATERING)
  {
    millis_counter=0;
    seconds_counter++;
    curEvent=EV_TIMEOUT5;
  }
}



