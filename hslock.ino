#include "Keypad.h"
#include "sha1.h"
#include "TOTP.h"
#include <Wire.h>
#include <TroykaRTC.h>
#include "Secrets.h"
 

#define BUZZER_PIN 9
#define LOCK_PIN 12
#define BUTTON_OPEN_PIN 10
 
#define SOUND_TIME_BUTTON_PRESS 50
#define SOUND_TIME_OPEN 3000
#define SOUND_TIME_ERROR 750

#define FREQ_BUTTON_PRESS 5000
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
RTC clock;

String userInput = "";  


void setup(){
  Serial.begin(9600);
  pinMode(BUTTON_OPEN_PIN, INPUT); 
  pinMode(BUZZER_PIN, OUTPUT); 
  pinMode(LOCK_PIN, OUTPUT);
  
  clock.begin(); 
  clock.set(__TIMESTAMP__); 
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


boolean arrayIncludeElement(int data[], int arraySize,  int element) {  
  for (int i = 0; i < arraySize; i++) {  
    if (data[i] == element) {
      return true;
    }
  } 
  return false;
}
  

bool isTOTPCodeValid(String userInput) { 
  Serial.print("userInput: "); Serial.println(userInput);
  int keyNum = userInput.substring(0, 2).toInt();  
  Serial.print("keyNum: "); Serial.println(keyNum); 
  bool keyIsActive = arrayIncludeElement(secrets::keysActive, secrets::hmacKeysCount, keyNum);
  Serial.print("keyIsActive: "); Serial.println(keyIsActive); 
  if (!keyIsActive) {
    accessDenied();
    return false;
  };  
  clock.read();   
  delay(200); 
  clock.read(); 
  unsigned long currentUnixTimestamp = clock.getUnixTime() + 86400 - 3600; 
  int timeDeviations[3] = {-30, 0, 30};  
  String userCode = userInput.substring(2, 8);
  Serial.print("Time: ");
  Serial.println(currentUnixTimestamp);  
  TOTP totp = TOTP(secrets::hmacKeys[keyNum], secrets::hmacKeySize);
  for (int i = 0; i < 3; i++) {
    int delta = timeDeviations[i]; 
    char* correctCode = totp.getCode(currentUnixTimestamp + delta); 
    Serial.println(userCode);
    Serial.println(correctCode); 
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
    unlockTheDoor();
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
      delay(50);
    }; 
  }
}
