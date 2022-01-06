#ifndef MOCK
#include "Secrets.h"
#include "Keypad.h"
#include "sha1.h"
#include "TOTP.h"
#include <Wire.h>  
#include <DS3231.h>
#include <Eeprom24C32_64.h> 
#include <Eeprom24C04_16.h> 
#endif

#define EEPROM_ADDRESS 0x57
#define EEPROM_CODE 32

#ifndef MOCK
static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);
#else
#define word int
#endif

#define BUZZER_PIN 10
#define LOCK_PIN 12
#define BUTTON_OPEN_PIN 13

#define SOUND_TIME_BUTTON_PRESS 50
#define SOUND_TIME_OPEN 3000
#define SOUND_TIME_ERROR 750

#define FREQ_BUTTON_PRESS 5000
#define FREQ_OPEN_BUTTON_PRESS 3000
#define FREQ_OPEN 2500
#define FREQ_ERROR 800

#define MORSE_SOUND_TIME  100  
#define MORSE_PAUSE 15
#define MORSE_FREQ 500

const word EEPROM_MEMORY_SIZE = EEPROM_CODE /8 * 1024;
const byte EEPROM_PAGE_COUNTS = EEPROM_CODE;
const word EEPROM_BATCHES = EEPROM_MEMORY_SIZE / EEPROM_PAGE_COUNTS;

void playMaintenanceMelody(int melody[], int size);

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



const bool morseKeys[10][5] = {
  {false, false, false, false, false}, // 0
  { true, false, false, false, false}, // 1
  { true,  true, false, false, false}, // 2
  { true,  true,  true, false, false}, // 3
  { true,  true,  true,  true, false}, // 4
  { true,  true,  true,  true,  true}, // 5
  {false,  true,  true,  true,  true}, // 6
  {false, false,  true,  true,  true}, // 7 
  {false, false, false,  true,  true}, // 8 
  {false, false, false, false,  true}, // 9 
};


#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_C4  262
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_C5  523
#define NOTE_C7  2093
#define NOTE_E7  2637
#define NOTE_G7  3136

int melodyMain[] = {
  NOTE_E7, NOTE_E7, 0, NOTE_E7,
  0, NOTE_C7, NOTE_E7, 0,
  NOTE_G7, 0, 0,  0,
};

int melodyUnderworld[] = {
  NOTE_C4, NOTE_C5, NOTE_A3, NOTE_A4,
  NOTE_AS3, NOTE_AS4,
};


String userInput = "";
String userInputPrev = "";


void setup(){
  Serial.begin(9600);
  Wire.begin();
  eeprom.initialize();
  pinMode(BUTTON_OPEN_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LOCK_PIN, OUTPUT);
  int size = sizeof(melodyMain) / sizeof(int);
  //playMaintenanceMelody(melodyMain, size);
}


void echo_morse_reversed_int(unsigned long value) { 
  // Serial.println(value);
  while (value > 0) {
    int digit = value % 10;
    value = value / 10;
    // Serial.println(digit);
    delay(MORSE_SOUND_TIME * MORSE_PAUSE); 
    for(int i =0; i < 5; i++ ) { 
      if (morseKeys[digit][i]) {
          tone(BUZZER_PIN, MORSE_FREQ, MORSE_SOUND_TIME);
          delay(MORSE_SOUND_TIME + MORSE_SOUND_TIME); 
      } else {
          tone(BUZZER_PIN, MORSE_FREQ, MORSE_SOUND_TIME * 3);
          delay(MORSE_SOUND_TIME * 3 + MORSE_SOUND_TIME); 
      }; 
    } 
  } 
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


byte getChecksum(byte arrayBytes[], byte count) {  
    byte checksum = 0x1A; 
    for (byte i = 0; i < count; i++) { 
        checksum ^= arrayBytes[i];   
        // Serial.print(i); Serial.print("=");  Serial.print(checksum, HEX);Serial.print(" "); 
    }; 
    return checksum ;
} 

//
//void showArray(byte arrayBytes[], byte count) { 
//    byte checksum = getChecksum(arrayBytes, count);
//    // Serial.println();
//    for (byte i = 0; i < count; i++)
//    {   
//        Serial.print(arrayBytes[i], HEX);
//        Serial.print(" "); 
//    }
//    Serial.print(checksum, HEX);
//    Serial.print(" ");
//} 


#ifndef MOCK
bool read_key_eeprom(int key_num, byte keyBytes[], byte key_len)
{  
    bool result = false;
    const byte buffer_size = key_len + 2; 
    byte outputBytes[buffer_size + 1] = { 0 };    
    word address = word(key_num * int(buffer_size));  
    eeprom.readBytes(address, buffer_size+1, outputBytes);  
    // showArray(outputBytes, buffer_size); 
    for (byte i = 0; i < key_len; i++)
    {   
        keyBytes[i] = outputBytes[i];
    }
    byte checksum = getChecksum(outputBytes, buffer_size-1);
    if (checksum == outputBytes[buffer_size-1]) {
      if (outputBytes[buffer_size - 2] == 0xFF) {
        result = true ;
      };
    } else {
        int melodyUnderworldSize = sizeof(melodyUnderworld) / sizeof(int);
        playMaintenanceMelody(melodyUnderworld, melodyUnderworldSize);
    };
    
//    Serial.print(" result- "); 
//    Serial.print(result, HEX);    
//    Serial.print(" checksum- "); 
//    Serial.print(checksum, HEX); 
//    Serial.print(" outputBytes[buffer_size - 2]- "); 
//    Serial.print(outputBytes[buffer_size - 2], HEX); 
//    Serial.print(" address- "); 
//    Serial.println(address);  
//    Serial.println("DONE");  
    return result;
}  
#else
bool read_key_eeprom(int key_num, byte keyBytes[], byte key_len) {
    return true;
}
#endif MOCK

String substr(String s, int from, int to) {
    #ifdef MOCK
        return s.substr(from, to);
    #else
        return s.substring(from, to);
    #endif
}

int toInt(String s) {
    #ifdef MOCK
        return stoi(s);
    #else
        return s.toInt();
    #endif
}

bool isTOTPCodeValid(String userInput) {
  // Serial.print("userInput: "); Serial.println(userInput);
  int keyNum = toInt(substr(userInput, 0, 2));
  Serial.print("keyNum: "); Serial.println(keyNum); 
  const int hmacKeySize = 20;
  byte keyBytes[hmacKeySize] = { 0 };
  bool is_key_valid = read_key_eeprom(keyNum, keyBytes, hmacKeySize); 
  Serial.print("keyIsActive: "); Serial.println(is_key_valid);
  if (!is_key_valid) {
    return false;
  }; 
  DateTime now = RTC.now(); 
  unsigned long currentUnixTimestamp = now.unixtime(); 
  int timeDeviations[5] = {-60, -30, 0, 30, 60};
  String userCode = substr(userInput, 2, 8);
  Serial.print("Time: "); Serial.println(currentUnixTimestamp);
  TOTP totp = TOTP(keyBytes, hmacKeySize);
  for (int i = 0; i < 5; i++) {
    int delta = timeDeviations[i];
    char* correctCode = totp.getCode(currentUnixTimestamp + delta);
    // Serial.println(userCode);
    // Serial.println(correctCode);
    if (userCode == correctCode) {
      // Serial.println("+++++++++++++++++++++++");
      return userCode == correctCode;
    }
  }
  // Serial.println("==========================");
  return false;
}


bool isMaintenance(String userInput, String userInputPrev) {
  // Serial.print("userInput: "); Serial.println(userInput);
  int keyNum = toInt(substr(userInput, 0, 2));
  // Serial.print("keyNum: "); Serial.println(keyNum); 
  return (( keyNum == 0 ) && ( userInputPrev != ""));
}


void buzz(int targetPin, long frequency, long length) {
  long delayValue = 1000000 / frequency / 2; // calculate the delay value between transitions
  //// 1 second's worth of microseconds, divided by the frequency, then split in half since
  //// there are two phases to each cycle
  long numCycles = frequency * length / 1000; // calculate the number of cycles for proper timing
  //// multiply frequency, which is really cycles per second, by the number of seconds to
  //// get the total number of cycles to produce
  for (long i = 0; i < numCycles; i++) { // for the calculated length of time...
    digitalWrite(targetPin, HIGH); // write the buzzer pin high to push out the diaphram
    delayMicroseconds(delayValue); // wait for the calculated delay value
    digitalWrite(targetPin, LOW); // write the buzzer pin low to pull back the diaphram
    delayMicroseconds(delayValue); // wait again or the calculated delay value
  }
}

void playMaintenanceMelody(int melody[], int size) {
    int melodyPin = BUZZER_PIN;
    for (int thisNote = 0; thisNote < size; thisNote++) {
      // to calculate the note duration, take one second
      // divided by the note type.
      //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
      int noteDuration = 1000 / 12;
      //buzz(melodyPin, melody[thisNote], noteDuration);
      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      int pauseBetweenNotes = noteDuration * 1.30;
      delay(pauseBetweenNotes);
      // stop the tone playing:
      //buzz(melodyPin, 0, noteDuration);
    }
}

void write_key_eeprom(int key_num, byte keyBytes[], byte key_len, bool is_active)  {
    bool result = false;
    const byte buffer_size = key_len + 2; 
    byte inputBytes[buffer_size + 1] = { 0 };    
    word address = word(key_num * int(buffer_size));  
    for (byte i = 0; i < key_len; i++) {   
        inputBytes[i] = keyBytes[i];
    };
    if (is_active) {
        inputBytes[buffer_size - 2] = 0xFF;
    } else {
        inputBytes[buffer_size - 2] = 0x00;
    }
    byte checksum = getChecksum(inputBytes, buffer_size-1);
    inputBytes[buffer_size-1] = checksum; 
    eeprom.writeBytes(address, buffer_size, inputBytes);
}


bool disableKey(int keyNum) {
  const int hmacKeySize = 20;
  byte keyBytes[hmacKeySize] = { 0 };
  bool is_key_valid = read_key_eeprom(keyNum, keyBytes, hmacKeySize); 
  if (is_key_valid) {
      bool is_active = false;
      write_key_eeprom(keyNum, keyBytes, hmacKeySize, is_active); 
      is_key_valid = read_key_eeprom(keyNum, keyBytes, hmacKeySize); 
  };
  return is_key_valid;
}


void makeMaintenance(String userInputPrev) {
  Serial.println(userInputPrev);
  int melodyUnderworldSize = sizeof(melodyUnderworld) / sizeof(int);
  int melodyMainSize = sizeof(melodyMain) / sizeof(int);

  playMaintenanceMelody(melodyUnderworld, melodyUnderworldSize);
  int command = toInt(substr(userInputPrev, 2, 4));
  Serial.print("raw command "); Serial.println(substr(userInputPrev, 2, 4));
  Serial.print("command "); Serial.println(command);

  if (command == 0) {
    delay(1000);
    DateTime now = RTC.now(); 
    unsigned long currentUnixTimestamp = now.unixtime();
    echo_morse_reversed_int(currentUnixTimestamp);
    delay(1000);
    playMaintenanceMelody(melodyMain, melodyMainSize);
  }
  if (command == 1) {
    int keyNum = toInt(substr(userInputPrev, 4, 6));
    int keyNumRepeat = toInt(substr(userInputPrev, 6, 8));
    if (keyNum == keyNumRepeat) {
        bool result = disableKey(keyNum);
        if (result) {
            accessDenied();
            delay(500);
            playMaintenanceMelody(melodyUnderworld, melodyUnderworldSize);
        } else {
            playMaintenanceMelody(melodyMain, melodyMainSize);
        };
    } else {
      accessDenied();
      delay(500);
      playMaintenanceMelody(melodyUnderworld, melodyUnderworldSize);
    };
  };
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
     } else {
        press_counter = 0;
     }; 
  };

  char customKey = customKeypad.getKey();
  if (customKey){
    tone(BUZZER_PIN, FREQ_BUTTON_PRESS, SOUND_TIME_BUTTON_PRESS);
    if (customKey == '*') {
      userInput = "";
      userInputPrev = "";
    }
    else if (customKey == '#') {
      if (isTOTPCodeValid(userInput)) {
          if (isMaintenance(userInput, userInputPrev)) {
            makeMaintenance(userInputPrev);
          } else {
            unlockTheDoor();
          };
      } else {
         accessDenied();
      };
      userInput = "";
      userInputPrev = "";
    } else {
      if (userInput.length() == 8) {
        userInputPrev = userInput;
        userInput = "";
      };
      userInput += customKey;
      delay(100);
    };
  }
}
