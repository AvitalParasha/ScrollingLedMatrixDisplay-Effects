#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <MD_UISwitch.h>
#include "Parola_Fonts_data.h"
#include <TimeLib.h>
#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC 
#include <Wire.h>         //http://arduino.cc/en/Reference/Wire (included with Arduino IDE)


//Serial1 stuff

static int8_t Send_buf[8] = {0} ;

#define CMD_PLAY_W_INDEX 0X03
#define CMD_SET_VOLUME 0X06
#define CMD_SEL_DEV 0X09
#define DEV_TF 0X02
#define CMD_PLAY 0X0D
#define CMD_PAUSE 0X0E
#define CMD_SINGLE_CYCLE 0X19
#define SINGLE_CYCLE_ON 0X00
#define SINGLE_CYCLE_OFF 0X01
#define CMD_PLAY_W_VOL 0X22



#define PRINT(s, x) { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x) Serial.print(F(x))
#define PRINTX(x) Serial.println(x, HEX)


#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
//#define CLK_PIN   10
//#define DATA_PIN  8
#define CS_PIN  10
#define TIME_PIN 9

const int s_clk = A2;
const int s_dat = A1;
const int s_lat = A0;


byte arr[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7c, 0x07, 0x7f, 0x67};

void display7Seg(byte decision)
{

  shiftOut(s_dat, s_clk, MSBFIRST, arr[decision]);
  digitalWrite(s_lat, HIGH);
  digitalWrite(s_lat, LOW);
}


// HARDWARE SPI
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
const uint8_t SPEED_IN = A3;//set the pin no. of the speeed control(analog)
const uint8_t DIRECTION_SET = 5;  // set the pin no. of the direction control
const uint8_t INVERT_SET = 6;     // set the pin no. of the invert control

const uint8_t SPEED_DEADBAND = 5;


uint8_t scrollSpeed = 25;    // default frame delay value
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 2000; // in milliseconds

// Global message buffers shared by Serial and Scrolling functions
#define  SIZE  75
char curMessage[SIZE] = { "" };
char newMessage[SIZE] = { "" };
bool newMessageAvailable = true;


MD_UISwitch_Digital uiDirection(DIRECTION_SET);//create a switch initialized with "DIRECTION_SET"
MD_UISwitch_Digital uiInvert(INVERT_SET);//create a switch initialized with "INVERT_SET"
MD_UISwitch_Digital displayTest(TIME_PIN);
void doUI(void)
{
  // set the speed if it has changed at least DEADBAND
  {
    int16_t speed = map(analogRead(SPEED_IN), 0, 1023, 10, 150);

    if ((speed >= ((int16_t)P.getSpeed() + SPEED_DEADBAND)) ||
        (speed <= ((int16_t)P.getSpeed() - SPEED_DEADBAND)))
    {
      P.setSpeed(speed);
      scrollSpeed = speed;
      PRINT("\nChanged speed to ", P.getSpeed());
    }
  }

  if (uiDirection.read() == MD_UISwitch::KEY_PRESS) // SCROLL DIRECTION
  {
    PRINTS("\nChanging scroll direction");
    scrollEffect = (scrollEffect == PA_SCROLL_LEFT ? PA_SCROLL_RIGHT : PA_SCROLL_LEFT);
    P.setTextEffect(scrollEffect, scrollEffect);
    P.displayClear();
    P.displayReset();
  }

  if (uiInvert.read() == MD_UISwitch::KEY_PRESS)  // INVERT MODE
  {
    PRINTS("\nChanging invert mode");
    P.setInvert(!P.getInvert());
  }
}//end of using the control of: speed, inverting, direction
char buf[75] = {"test"};

void reverse()
{
  String temp ;
  int sizee;//count the size of the hebrew string
  String strNew = "";
  char tempTav;
  String strLanguage=String(newMessage);//temp for checking the language;
  int index1=0,index2=0,index=0,pos;//index 1 is the pos of the first hebrew char and 2 is of the last one and index is helpOne
  while(index<strLanguage.length())//while didnt get to the end of cthe string
  {

    if(byte(strLanguage[index])>128)
    {
      Serial.print("hi");
      strNew="";
     sizee=1;
     index1=index;
     index++;//save the pos of the first hebrewChar
     while(index<strLanguage.length()&&(byte(strLanguage[index])>128||strLanguage[index]==' '&&byte(strLanguage[index+1])>128))
     {
      
      index++;
      sizee++;//go on till the last hebrew char and cound its length
     }
     index2=index-1; //keep the last hebrew char pos
     if(sizee%2==0)
      sizee-=2;
     for (int i = index1; i <= sizee/2+index1; i++) 
     {
      tempTav=newMessage[i];
      newMessage[i]=newMessage[index2+index1-i];
      newMessage[index2+index1-i]=tempTav;
     }
   
     }
    else
      index++;
  }
  newMessage[index] = '\0';
  Serial.println(newMessage);
      Serial.println(strNew);
  
}
void readSerial(void)//read text from the monitor with serial communication
{
  static char *cp = newMessage;

  while (Serial.available())
  {
    *cp = (char)Serial.read();
    if ((*cp == 0x0D || *cp == '\n') || (cp - newMessage >= SIZE - 2)) // end of message character or full buffer
    {
      *cp = '\0'; // end the string
      // restart the index for next filling spree and flag we have a message waiting
      cp = newMessage;
     reverse();
     //Serial.print(newMessage);
      newMessageAvailable = true;
    }
    else  // move char pointer to next position
      cp++;
  }

}
void setRtc()//function initalize the RTC settings
{
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if (timeStatus() != timeSet) //check if the rtc is connected
    Serial.println("Unable to sync with the RTC");
  else
    Serial.println("RTC has set the system time");
}
char timeBuf[75];
void printTime()
{
  char hours[3], minutes[3], seconds[3];
  int h = hour();
  int m = minute();
  int s = second();
  if (h < 10) {
    hours[0] = '0';
    hours[1] = h + '0';
    hours[2] = '\0';
  }
  else {
    String(h).toCharArray(hours, 3);
  }
  if (m < 10) {
    minutes[0] = '0';
    minutes[1] = m + '0';
    minutes[2] = '\0';
  }
  else
    String(m).toCharArray(minutes, 3);
  if (s < 10) {
    seconds[0] = '0';
    seconds[1] ='0'+ s ;
    seconds[2] = '\0';
  }
  else {
    String(s).toCharArray(seconds, 3);
  }

  sprintf(timeBuf, "%s:%s:%s", hours, minutes, seconds);
  newMessageAvailable = true;
  strcpy(curMessage, timeBuf);
}

void printDate()
{
  // digital clock display of the date
   char day1[3], month1[3];
  int d1 = day();
  int m1 = month();
  if (d1 < 10) {
    day1[0] = '0';
    day1[1] =  '0'+d1;
    day1[2] = '\0';}
    else {
    String(d1).toCharArray(day1, 3);}
   if (m1 < 10) {
    month1[0] = '0';
    month1[1] = '0'+m1  ;
    month1[2] = '\0'; }
    else {
    String(m1).toCharArray(month1, 3);}
  sprintf(timeBuf, "%s/%s/%d", day1, month1, year());
  newMessageAvailable = true;
  strcpy(curMessage, timeBuf);
}

int getTemp(int tempPin) {
  int val = analogRead(tempPin);
  int mv = ( val / 1024.0) * 5000;
  return mv / 10;
}

const int TempPIN = A2;

void printTemp()
{
char tmp[10];
String(RTC.temperature() / 4.).toCharArray(tmp, 10);
  sprintf(timeBuf, "%sC",tmp );

  newMessageAvailable = true;
  strcpy(curMessage, timeBuf);
  Serial.println(RTC.temperature() / 4.);
}


void sendCommand(int8_t command, int16_t dat)
{
  delay(20);
  Send_buf[0] = 0x7e; //starting byte
  Send_buf[1] = 0xff; //version
  Send_buf[2] = 0x06; //the number of bytes of the command without starting byte and ending byte
  Send_buf[3] = command; //
  Send_buf[4] = 0x00;//0x00 = no feedback, 0x01 = feedback
  Send_buf[5] = (int8_t)(dat >> 8);//data high
  Send_buf[6] = (int8_t)(dat); //data low
  Send_buf[7] = 0xef; //ending byte
  for (uint8_t i = 0; i < 8; i++) //
  {
    Serial1.write(Send_buf[i]) ;
  }
}

void play_mode(byte state)
{
  int nowPlay = 3840 + state; //the decimal value of 0x0f00+ state
  sendCommand(CMD_PLAY_W_VOL, nowPlay);//play the first song with volume 15 class
}

int lastButtonState;
void setup()
{
  pinMode(s_clk, OUTPUT);
  pinMode(s_dat, OUTPUT);
  pinMode(s_lat, OUTPUT);
  digitalWrite(s_lat, LOW);
  digitalWrite(s_clk, LOW);
  Serial.begin(57600);
  uiDirection.begin();
  uiInvert.begin();
  displayTest.begin();
  pinMode(SPEED_IN, INPUT);
  // pinMode(TIME_PIN, INPUT_PULLUP);
  //
  doUI();
  P.begin();
  P.setIntensity (0);
  P.setFont(fontHebrew);
  P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);

  setRtc();
  Serial1.begin(9600);
  delay(500);//Wait chip initialization is complete
  sendCommand(CMD_SEL_DEV, DEV_TF);//select the TF card
  delay(200);//wait for 200ms
  sendCommand(CMD_SET_VOLUME, 30);
}
int state = 1;

int lastState = 0;

void loop()
{
  doUI();
  //P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
  if (P.displayAnimate())
  {
    if (newMessageAvailable)
    {
      strcpy(curMessage, newMessage);
      newMessageAvailable = false;
    }
    P.displayReset();
  }
  if (displayTest.read() == MD_UISwitch::KEY_PRESS)//chaeck i the buttom pressed
    state++;//if did count it
  if (state > 4)//if was last one
    state = 1;//go to first one

  switch (state)//in each state do its functions
  {
    case 1: {//date case

        printDate();// display the date on the max matrices
        display7Seg(1);//send 1 to the 7 segment
        if (lastState != state) {//for stability
          play_mode(1);//turn on the mp3 the date 
          P.setTextEffect(PA_SCROLL_RIGHT, PA_SCROLL_RIGHT);//set the effect to scroll
        }
        break;
      }
    case 2: {//time state
        printTime();
        display7Seg(2);//send 2 to the 7 segment
        if (lastState != state) {//for stabiity
          play_mode(2);//turn on the 2nd file in the mp3
          P.setTextEffect(PA_PRINT, PA_NO_EFFECT);//show the time with no effect
        }
        break;
      }
    case 3:{ //free text mode
        readSerial();
        display7Seg(3);//send 3 to the 7 egment
        if (lastState != state) {
          play_mode(3);
          P.setTextEffect(PA_SCROLL_RIGHT, PA_SCROLL_RIGHT);
        }
        break;
      }
    case 4:{//temprature state 
        printTemp();
        display7Seg(4);
        if (lastState != state) {
          play_mode(4);
          P.setTextEffect(PA_PRINT, PA_NO_EFFECT);
        }
        break;
      }
    default:
      break;
  }
  lastState = state;//for stability
}
