#include "Keypad.h"
#include "sha1.h"
#include "TOTP.h"
#include <DS3231.h>
#include <Wire.h>  
#include <Eeprom24C32_64.h> 
#include <Eeprom24C04_16.h> 


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

#define EEPROM_ADDRESS 0x57
#define EEPROM_CODE 32
const word EEPROM_MEMORY_SIZE = EEPROM_CODE /8 * 1024;
const byte EEPROM_PAGE_COUNTS = EEPROM_CODE;
const word EEPROM_BATCHES = EEPROM_MEMORY_SIZE / EEPROM_PAGE_COUNTS;


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

struct State {
    Eeprom24C32_64 eeprom = Eeprom24C32_64(EEPROM_ADDRESS);
    RTClib RTC;
    Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
    void (*delay)(unsigned long) = delay;
    void (*digitalWrite)(uint8_t, uint8_t ) = digitalWrite;
    void (*tone)(uint8_t, unsigned int, long unsigned int) = tone;
    int (*digitalRead)(uint8_t) = digitalRead;
    void (*delayMicroseconds)(unsigned int) = delayMicroseconds;
    String userInput = "";
    String userInputPrev = "";
} global_state;

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



void setup(){
  State* state = &global_state;
  Serial.begin(9600);
  Wire.begin();
  state->eeprom.initialize();
  pinMode(BUTTON_OPEN_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LOCK_PIN, OUTPUT);
  int size = sizeof(melodyMain) / sizeof(int);
  playMaintenanceMelody(state, melodyMain, size);
}


void echo_morse_reversed_int(State* state, unsigned long value) { 
  // Serial.println(value);
  while (value > 0) {
    int digit = value % 10;
    value = value / 10;
    // Serial.println(digit);
    state->delay(MORSE_SOUND_TIME * MORSE_PAUSE); 
    for(int i =0; i < 5; i++ ) { 
      if (morseKeys[digit][i]) {
          state->tone(BUZZER_PIN, MORSE_FREQ, MORSE_SOUND_TIME);
          state->delay(MORSE_SOUND_TIME + MORSE_SOUND_TIME); 
      } else {
          state->tone(BUZZER_PIN, MORSE_FREQ, MORSE_SOUND_TIME * 3);
          state->delay(MORSE_SOUND_TIME * 3 + MORSE_SOUND_TIME); 
      }; 
    } 
  } 
}


void accessDenied(State* state) {
  state->tone(BUZZER_PIN, FREQ_ERROR, SOUND_TIME_ERROR);
  state->delay(SOUND_TIME_ERROR);
}


void unlockTheDoor(State* state) {
  state->digitalWrite(LOCK_PIN, HIGH);
  state->tone(BUZZER_PIN, FREQ_OPEN, SOUND_TIME_OPEN);
  state->delay(SOUND_TIME_OPEN);
  state->digitalWrite(LOCK_PIN, LOW);
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


bool read_key_eeprom(State* state, int key_num, byte keyBytes[], byte key_len)
{  
    bool result = false;
    const byte buffer_size = key_len + 2; 
    byte outputBytes[buffer_size + 1] = { 0 };    
    word address = word(key_num * int(buffer_size));  
    state->eeprom.readBytes(address, buffer_size+1, outputBytes);  
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
        playMaintenanceMelody(state, melodyUnderworld, melodyUnderworldSize);
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


bool isTOTPCodeValid(State* state, String userInput) {
  // Serial.print("userInput: "); Serial.println(userInput);
  int keyNum = userInput.substring(0, 2).toInt();
  Serial.print("keyNum: "); Serial.println(keyNum); 
  const int hmacKeySize = 20;
  byte keyBytes[hmacKeySize] = { 0 };
  bool is_key_valid = read_key_eeprom(state, keyNum, keyBytes, hmacKeySize); 
  Serial.print("keyIsActive: "); Serial.println(is_key_valid);
  if (!is_key_valid) {
    return false;
  }; 
  DateTime now = state->RTC.now(); 
  unsigned long currentUnixTimestamp = now.unixtime(); 
  int timeDeviations[5] = {-60, -30, 0, 30, 60};
  String userCode = userInput.substring(2, 8);
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
  int keyNum = userInput.substring(0, 2).toInt();
  // Serial.print("keyNum: "); Serial.println(keyNum); 
  return (( keyNum == 0 ) && ( userInputPrev != ""));
}


void buzz(State* state, int targetPin, long frequency, long length) {
  long delayValue = 1000000 / frequency / 2; // calculate the delay value between transitions
  //// 1 second's worth of microseconds, divided by the frequency, then split in half since
  //// there are two phases to each cycle
  long numCycles = frequency * length / 1000; // calculate the number of cycles for proper timing
  //// multiply frequency, which is really cycles per second, by the number of seconds to
  //// get the total number of cycles to produce
  for (long i = 0; i < numCycles; i++) { // for the calculated length of time...
    state->digitalWrite(targetPin, HIGH); // write the buzzer pin high to push out the diaphram
    delayMicroseconds(delayValue); // wait for the calculated delay value
    state->digitalWrite(targetPin, LOW); // write the buzzer pin low to pull back the diaphram
    delayMicroseconds(delayValue); // wait again or the calculated delay value
  }
}

void playMaintenanceMelody(State* state, int melody[], int size) {
    int melodyPin = BUZZER_PIN;
    for (int thisNote = 0; thisNote < size; thisNote++) {
      // to calculate the note duration, take one second
      // divided by the note type.
      //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
      int noteDuration = 1000 / 12;
      buzz(state, melodyPin, melody[thisNote], noteDuration);
      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      int pauseBetweenNotes = noteDuration * 1.30;
      state->delay(pauseBetweenNotes);
      // stop the tone playing:
      buzz(state, melodyPin, 0, noteDuration);
    }
}

bool write_key_eeprom(State* state, int key_num, byte keyBytes[], byte key_len, bool is_active)  {
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
    state->eeprom.writeBytes(address, buffer_size, inputBytes);
}


bool disableKey(State* state, int keyNum) {
  const int hmacKeySize = 20;
  byte keyBytes[hmacKeySize] = { 0 };
  bool is_key_valid = read_key_eeprom(state, keyNum, keyBytes, hmacKeySize); 
  if (is_key_valid) {
      bool is_active = false;
      write_key_eeprom(state, keyNum, keyBytes, hmacKeySize, is_active); 
      is_key_valid = read_key_eeprom(state, keyNum, keyBytes, hmacKeySize); 
  };
  return is_key_valid;
}


void makeMaintenance(State* state, String userInputPrev) {
  Serial.println(userInputPrev);
  int melodyUnderworldSize = sizeof(melodyUnderworld) / sizeof(int);
  int melodyMainSize = sizeof(melodyMain) / sizeof(int);

  playMaintenanceMelody(state, melodyUnderworld, melodyUnderworldSize);
  int command = userInputPrev.substring(2, 4).toInt();
  Serial.print("raw command "); Serial.println(userInputPrev.substring(2, 4));
  Serial.print("command "); Serial.println(command);

  if (command == 0) {
    state->delay(1000);
    DateTime now = state->RTC.now(); 
    unsigned long currentUnixTimestamp = now.unixtime();
    echo_morse_reversed_int(state, currentUnixTimestamp);
    state->delay(1000);
    playMaintenanceMelody(state, melodyMain, melodyMainSize);
  }
  if (command == 1) {
    int keyNum = userInputPrev.substring(4, 6).toInt();
    int keyNumRepeat = userInputPrev.substring(6, 8).toInt();
    if (keyNum == keyNumRepeat) {
        bool result = disableKey(state, keyNum);
        if (result) {
            accessDenied(state);
            state->delay(500);
            playMaintenanceMelody(state, melodyUnderworld, melodyUnderworldSize);
        } else {
            playMaintenanceMelody(state, melodyMain, melodyMainSize);
        };
    } else {
      accessDenied(state);
      state->delay(500);
      playMaintenanceMelody(state, melodyUnderworld, melodyUnderworldSize);
    };
  };
}


void loop(){

  State* state = &global_state;
  state->digitalWrite(LOCK_PIN, LOW);  
  boolean openButtonIsDown = state->digitalRead(BUTTON_OPEN_PIN);
  if (openButtonIsDown) {
     // Serial.println(__TIMESTAMP__);   
     int timeout = 30; // ms
     int iterations = 15; 
     int limit = 7;
     int press_counter = 0;
     for (int i = 0; i < iterations; i++) { 
       state->tone(BUZZER_PIN, FREQ_OPEN_BUTTON_PRESS, timeout - 10);
       state->delay(timeout); 
       if (state->digitalRead(BUTTON_OPEN_PIN) == HIGH) {
           press_counter += 1;
       }; 
     };
     if (press_counter > limit) {
        unlockTheDoor(state); 
     } else {
        press_counter = 0;
     }; 
  };

  char customKey = state->customKeypad.getKey();
  if (customKey){
    state->tone(BUZZER_PIN, FREQ_BUTTON_PRESS, SOUND_TIME_BUTTON_PRESS);
    if (customKey == '*') {
      state->userInput = "";
      state->userInputPrev = "";
    }
    else if (customKey == '#') {
      if (isTOTPCodeValid(state, state->userInput)) {
          if (isMaintenance(state->userInput, state->userInputPrev)) {
            makeMaintenance(state, state->userInputPrev);
          } else {
            unlockTheDoor(state);
          };
      } else {
         accessDenied(state);
      };
      state->userInput = "";
      state->userInputPrev = "";
    } else {
      if (state->userInput.length() == 8) {
        state->userInputPrev = state->userInput;
        state->userInput = "";
      };
      state->userInput += customKey;
      state->delay(100);
    };
  }
}
