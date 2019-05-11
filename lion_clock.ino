#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"


// LCD 0x27
// CLOCK 0x68

DateTime now;
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
volatile int alarmHour = 18;
volatile int alarmMinute = 54;
volatile int alarmProgress = 0;
volatile int buttonHourState = LOW;
volatile int buttonMinuteState = LOW;
volatile bool screenOn = true;
volatile long screenOnSince;
const int alarmLength = 300; // in seconds
const int redPin = 9;
const int greenPin = 10;
const int bluePin = 11;

void setup() {
  Serial.begin(9600);
  pinMode(redPin, OUTPUT); //redPin
  pinMode(greenPin, OUTPUT); //greenPin
  pinMode(bluePin, OUTPUT); //bluePin
  if (! rtc.begin())
  {
    Serial.println("Couldn't find RTC Module");
    while (1);
  }
  lcd.begin(16,2);
  lcd.clear();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  now = rtc.now();
  screenOnSince = now.unixtime();
}

void showTime() {
  char timeBuffer[16];
  sprintf(timeBuffer,"%2u:%02u:%02u",now.hour(),now.minute(),now.second());
  lcd.setCursor(0,0);
  Serial.print("lcd showing: "); Serial.println(timeBuffer);
  lcd.print(timeBuffer);
}

void showAlarmTime() {
  char timeBuffer[16];
  sprintf(timeBuffer,"%13d:%02d",alarmHour,alarmMinute);
  lcd.setCursor(0,1);
  lcd.print(timeBuffer);
}

void updateAlarmProgress() {
  long currentSeconds = (now.hour() * 3600) + (now.minute() * 60) + now.second();
  long alarmSeconds = alarmHour * 3600 + alarmMinute * 60;
  Serial.print("    alarmHour: ");
  Serial.print(alarmHour);
  Serial.print("    alarmMinute: ");
  Serial.print(alarmMinute);
  int hourGap = now.hour() - alarmHour;
  Serial.print("    hourGap: ");
  Serial.print(hourGap);
  if((hourGap * hourGap) > 2) {
    return;
  }
  int timeGap = (hourGap * 3600) + ((now.minute() * 60) + now.second() - alarmMinute*60);
  Serial.print("    timeGap: ");
  Serial.print(timeGap);
  Serial.print("    alarmLength: ");
  Serial.print(alarmLength);
  if(timeGap > 0 && timeGap < alarmLength){
    alarmProgress = (timeGap * 100 / alarmLength); // percentage
  } else {
    alarmProgress = 0;
  }
  Serial.print("    AlarmProgress: ");
  Serial.println(alarmProgress);
}

int maybeTurnScreenOn() {
  if(!screenOn) {
    screenOnSince = now.unixtime();
    lcd.setBacklight(BACKLIGHT_ON);
    screenOn = true;
    Serial.println("Turning screen on");
    return true;
  } else { return false; }
}

// Any button should first turn the screen on if it's on standby. A second push should do the button's normal function.
void respondToButtons() {
  if(buttonHourState == LOW && digitalRead(4) == HIGH) {
    Serial.println("Hour button pushed!");
    buttonHourState = HIGH;
    if(! maybeTurnScreenOn()) {
      alarmHour += 1;
      // limit between 2am and 9am to stop endless clicking
      if(alarmHour > 9){
        alarmHour = 2;
      }
    }
  } else {
    buttonHourState = digitalRead(4);
  }

  if(buttonMinuteState == LOW && digitalRead(5) == HIGH) {
    Serial.println("Minute button pushed!");
    buttonMinuteState = HIGH;
    if(! maybeTurnScreenOn()) {
     alarmMinute += 5;
     if(alarmMinute > 60){
       alarmMinute = 0;
     }
    }
  } else {
    buttonMinuteState = digitalRead(5);
  }
}

void loop() {
  Serial.println(".");
  // turn screen off if it has been on for longer than 5 seconds
  if(screenOn && ((now.unixtime() - screenOnSince) > 5)) {
    lcd.setBacklight(BACKLIGHT_OFF);
    screenOn = false;
    Serial.println("Turning screen off");
  }
  respondToButtons();
  now = rtc.now();
  updateAlarmProgress();
//  analogWrite(redPin,51);
//  analogWrite(greenPin,255);
//  analogWrite(bluePin,51);

  // set lights according to progress
  if(alarmProgress > 0) {
    if(alarmProgress == 99) {
      analogWrite(redPin,0);
      analogWrite(greenPin,0);
      analogWrite(bluePin,0);
    } else {
      int square = 20;
      float brightnessRoot = (square * alarmProgress / 100.0);
      int currentBrightness = min(255, brightnessRoot * brightnessRoot);

      Serial.print("brightness: ");
      Serial.println(currentBrightness);
      analogWrite(redPin,currentBrightness/5);
      analogWrite(greenPin,currentBrightness);
      analogWrite(bluePin,currentBrightness/8);
    }
  }

  showTime();
  showAlarmTime();

  delay(300);
}
