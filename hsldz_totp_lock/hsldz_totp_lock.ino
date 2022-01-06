#ifndef MOCK
#include "Keypad.h"
#include "sha1.h"
#include "TOTP.h"
#include <DS3231.h>
#include <Wire.h> 
#include "Arduino.h"
#endif

#include "Secrets.h"

#define BUZZER_PIN 10
#define LOCK_PIN 12
#define BUTTON_OPEN_PIN 11

#define SOUND_TIME_BUTTON_PRESS 50
#define SOUND_TIME_OPEN 3000
#define SOUND_TIME_ERROR 750

#define FREQ_BUTTON_PRESS 5000
#define FREQ_OPEN_BUTTON_PRESS 3000
#define FREQ_OPEN 2500
#define FREQ_ERROR 800


// Keypad
const byte ROWS = 4;
const byte COLS = 3;
const char hexaKeys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
const byte rowPins[ROWS] = {2, 3, 4, 5};
const byte colPins[COLS] = {6, 7, 8};
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// RTC 
RTClib RTC;

String userInput = "";


void setup(){
  Serial.begin(9600);
  Wire.begin();
  pinMode(BUTTON_OPEN_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LOCK_PIN, OUTPUT);
 
}


void accessDenied() {
  tone(BUZZER_PIN, FREQ_ERROR, SOUND_TIME_ERROR);
  delay(SOUND_TIME_ERROR);
}


void unlockTheDoor() {
  digitalWrite(LOCK_PIN, HIGH);
  tone(BUZZER_PIN, FREQ_OPEN, SOUND_TIME_OPEN);
  delay(SOUND_TIME_OPEN);
  digitalWrite(LOCK_PIN, LOW);
}


boolean arrayIncludeElement(const int data[], int arraySize,  int element) {
  for (int i = 0; i < arraySize; i++) {
    if (data[i] == element) {
      return true;
    }
  }
  return false;
}
// ostatni zegar 1608246920 sync
// 2020-12-26 / 9 days - 23 sec 

String getUserCode(String userInput) {
#ifndef MOCK
    return userInput.substring(2, 8);
#else
    return ""; //TODO
#endif
}

int getKeyNum(String userInput) {
#ifndef MOCK
    return userInput.substring(0, 2).toInt();
#else
    return 0; //TODO
#endif
}

bool isTOTPCodeValid(String userInput) {
  Serial.print("userInput: "); Serial.println(userInput);
  int keyNum = getKeyNum(userInput);
  Serial.print("keyNum: "); Serial.println(keyNum);
  bool keyIsActive = arrayIncludeElement(secrets::keysActive, secrets::hmacKeysCount, keyNum);
  Serial.print("keyIsActive: "); Serial.println(keyIsActive);
  if (!keyIsActive) {
    accessDenied();
    return false;
  }; 
  DateTime now = RTC.now(); 
  unsigned long currentUnixTimestamp = now.unixtime(); 
  int timeDeviations[5] = {-60, -30, 0, 30, 60};
  String userCode = getUserCode(userInput);
  Serial.print("Time: "); Serial.println(currentUnixTimestamp);
  TOTP totp = TOTP(secrets::hmacKeys[keyNum], secrets::hmacKeySize);
  for (int i = 0; i < 5; i++) {
    int delta = timeDeviations[i];
    char* correctCode = totp.getCode(currentUnixTimestamp + delta);
    // Serial.println(userCode);
    // Serial.println(correctCode);
    if (userCode == correctCode) {
      Serial.println("+++++++++++++++++++++++");
      return userCode == correctCode;
    }
  }
  Serial.println("==========================");
  accessDenied();
  return false;
}


void loop(){
  digitalWrite(LOCK_PIN, LOW); 
  
  boolean openButtonIsDown = digitalRead(BUTTON_OPEN_PIN);
  if (openButtonIsDown) {
     // Serial.println(__TIMESTAMP__);   
     int timeout = 30; // ms
     int iterations = 15; 
     int limit = 7;
     int press_counter = 0;
     for (int i = 0; i < iterations; i++) { 
       tone(BUZZER_PIN, FREQ_OPEN_BUTTON_PRESS, timeout - 10);
       delay(timeout); 
       if (digitalRead(BUTTON_OPEN_PIN) == HIGH) {
           press_counter += 1;
       }; 
     };
     if (press_counter > limit) {
        unlockTheDoor(); 
        isTOTPCodeValid(userInput);
     } else {
        press_counter = 0;
     }; 
  };

  char customKey = customKeypad.getKey();
  if (customKey){
    tone(BUZZER_PIN, FREQ_BUTTON_PRESS, SOUND_TIME_BUTTON_PRESS);
    if (customKey == '*') {
      userInput = "";
    }
    else if (customKey == '#') {
      if (isTOTPCodeValid(userInput)) {
        unlockTheDoor();
      };
      userInput = "";
    } else {
      if (userInput.length() == 8) {
        userInput = "";
      };
      userInput += customKey;
      delay(100);
    };
  }
}
