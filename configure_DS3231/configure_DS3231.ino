#include <Wire.h> 
#include <Eeprom24C32_64.h>
#include <Eeprom24C04_16.h>
#include <DS3231.h>

DS3231 Clock;
RTClib RTC;

#define EEPROM_ADDRESS 0x57
#define EEPROM_CODE 32

static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);
const word EEPROM_MEMORY_SIZE = EEPROM_CODE /8 * 1024;
const byte EEPROM_PAGE_COUNTS = EEPROM_CODE;
const word EEPROM_BATCHES = EEPROM_MEMORY_SIZE / EEPROM_PAGE_COUNTS;
String current_command = "";


byte Year;
byte Month;
byte Date;
byte DoW;
byte Hour;
byte Minute;
byte Second;

bool Century=false;
bool h12;
bool PM;

const int ClockPowerPin = 6; // Activates voltage regulator to power the RTC (otherwise is on backup power from VCC or batt)
const int LEDpin = 9; // LED to tell user if logger is working properly
const int SDpowerPin = 8;
const int wakePin = 2; // Keep pin high

void GetDateStuff(byte& Year, byte& Month, byte& Day, byte& DoW, 
    byte& Hour, byte& Minute, byte& Second) {
  // Call this if you notice something coming in on 
  // the serial port. The stuff coming in should be in 
  // the order YYMMDDwHHMMSS, with an 'x' at the end.
  boolean GotString = false;
  char InChar;
  byte Temp1, Temp2;
  char InString[20];

  byte j=0;
  while (!GotString) {
    if (Serial.available()) {
      InChar = Serial.read();
      InString[j] = InChar;
      j += 1;
      if (InChar == 'x') {
        GotString = true;
      }
    }
  }
  Serial.println(InString);
  // Read Year first
  Temp1 = (byte)InString[0] -48;
  Temp2 = (byte)InString[1] -48;
  Year = Temp1*10 + Temp2;
  // now month
  Temp1 = (byte)InString[2] -48;
  Temp2 = (byte)InString[3] -48;
  Month = Temp1*10 + Temp2;
  // now date
  Temp1 = (byte)InString[4] -48;
  Temp2 = (byte)InString[5] -48;
  Day = Temp1*10 + Temp2;
  // now Day of Week
  DoW = (byte)InString[6] - 48;   
  // now Hour
  Temp1 = (byte)InString[7] -48;
  Temp2 = (byte)InString[8] -48;
  Hour = Temp1*10 + Temp2;
  // now Minute
  Temp1 = (byte)InString[9] -48;
  Temp2 = (byte)InString[10] -48;
  Minute = Temp1*10 + Temp2;
  // now Second
  Temp1 = (byte)InString[11] -48;
  Temp2 = (byte)InString[12] -48;
  Second = Temp1*10 + Temp2;
}
 

void fillArrayWithValues(byte inputBytes[], byte count) {
    byte checksum = 0x1A;
    int value = 0;
    for (byte i = 0; i < count - 1; i++)
    {    
        value = random(64) + 33; 
        inputBytes[i] = byte(value); 
        checksum ^= inputBytes[i];  
    }
    inputBytes[count-1] = checksum; 
}

byte getChecksum(byte arrayBytes[], byte count) {  
    byte checksum = 0x1A; 
    for (byte i = 0; i < count; i++) { 
        checksum ^= arrayBytes[i];   
        // Serial.print(i); Serial.print("=");  Serial.print(checksum, HEX);Serial.print(" "); 
    }; 
    return checksum ;
}

void showArray(byte arrayBytes[], byte count) { 
    byte checksum = getChecksum(arrayBytes, count);
    // Serial.println();
    for (byte i = 0; i < count; i++)
    {   
        Serial.print(arrayBytes[i], HEX);
        Serial.print(" "); 
    }
    Serial.print(checksum, HEX);
    Serial.print(" ");
} 


void read_eeprom()
{  
    const byte buffer_size = 32;
    const word batches = EEPROM_MEMORY_SIZE / buffer_size;
    byte outputBytes[buffer_size + 1] = { 0 };   
    // Serial.println("Read bytes from EEPROM memory...");  
    
    word address = 0;  
    for (word i = 0; i < batches; i++)
    {  
      address = word(i * int(buffer_size));  
      eeprom.readBytes(address, buffer_size+1, outputBytes);  
      showArray(outputBytes, buffer_size); 
      Serial.print(" - "); 
      Serial.println(address);   
    }
    Serial.println("DONE");  
}  


void write_eeprom()
{  
    Serial.println("START WRITING");
    const byte buffer_size = 32;
    const word batches = EEPROM_MEMORY_SIZE / buffer_size;
    byte outputBytes[buffer_size + 1] = { 0 };    
    
    word address = 0;       

    // Declare byte arrays.
    byte inputBytes[buffer_size] = { 0 }; 
    
    // Write input array to EEPROM memory.
    int i = 0; 
    int j = 0; 
    for (i = 0; i < batches; i++)
    {  
      address = word(i * int(buffer_size));    
      int result = Serial.readBytes(inputBytes, buffer_size);  
      showArray(inputBytes, buffer_size);  
      Serial.print(" - "); 
      Serial.print(address);   
      Serial.print(" - "); 
      Serial.print(result); 
      Serial.println();
      eeprom.writeBytes(address, buffer_size, inputBytes);    
    }
     Serial.println("DONE");
} 

void clock_set()
{  
    Serial.println("START WRITING");  
    
    GetDateStuff(Year, Month, Date, DoW, Hour, Minute, Second);
    
    Clock.setClockMode(false);  // set to 24h 
    
    Clock.setSecond(Second);
    Clock.setMinute(Minute);
    Clock.setHour(Hour);
    Clock.setDate(Date);
    Clock.setMonth(Month);
    Clock.setYear(Year);
    Clock.setDoW(DoW); 
    Serial.println("accepted");
    
    Serial.print(Clock.getYear(), DEC);
    Serial.print("-");
    Serial.print(Clock.getMonth(Century), DEC);
    Serial.print("-");
    Serial.print(Clock.getDate(), DEC);
    Serial.print(" ");
    Serial.print(Clock.getHour(h12, PM), DEC); //24-hr
    Serial.print(":");
    Serial.print(Clock.getMinute(), DEC);
    Serial.print(":");
    Serial.println(Clock.getSecond(), DEC); 
    Serial.println("DONE");
} 

void clock_read()
{   
    Serial.println("START READING"); 
//    Serial.print(Clock.getYear(), DEC);
//    Serial.print("-");
//    Serial.print(Clock.getMonth(Century), DEC);
//    Serial.print("-");
//    Serial.print(Clock.getDate(), DEC);
//    Serial.print(" ");
//    Serial.print(Clock.getHour(h12, PM), DEC); //24-hr
//    Serial.print(":");
//    Serial.print(Clock.getMinute(), DEC);
//    Serial.print(":");
//    Serial.println(Clock.getSecond(), DEC); 
    DateTime now = RTC.now();
    unsigned long currentUnixTimestamp = now.unixtime(); 
    Serial.println(currentUnixTimestamp);
    Serial.println("DONE");
} 


void setup()
{ 
    Serial.begin(115200);    
    eeprom.initialize();
    current_command = ""; 
    
} 

void loop()
{   
   if (current_command == "") {
     if (Serial.available()) {
        String raw_string = "";  
        while (Serial.available())  
        {  
          delay(10);
          char c = Serial.read();
          raw_string += c;
        }
        current_command = raw_string.substring(0, 5); 
        // Serial.print("raw_string: "); Serial.println(raw_string);
      
    };
  };  
  if (current_command == "read ") {
    read_eeprom();
    current_command = ""; 
  };
  if (current_command == "write") { 
    // Serial.print("Current command: "); Serial.println(current_command);
    write_eeprom();
    current_command = ""; 
  };
  if (current_command == "cset ") { 
    Serial.print("Current command: "); Serial.println(current_command);
    clock_set();
    current_command = ""; 
  };
  if (current_command == "cread") { 
    Serial.print("Current command: "); Serial.println(current_command);
    clock_read();
    current_command = ""; 
  };
  delay(1000);  
}
