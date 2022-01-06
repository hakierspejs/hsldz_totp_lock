#ifdef MOCK
#include <string>
#include <iostream>
#define makeKeymap(x) ((char*)x)
typedef std::string String;
typedef char byte;
typedef bool boolean;

const int LOW=1;
const int HIGH=1;
const int INPUT=1;
const int OUTPUT=1;

struct {
    void initialize() {};
    void writeBytes(int&, int, byte []) {};
    void readBytes(int&, int, byte []) {};
} eeprom;

struct Keypad{
    Keypad(char*, const byte [4], const byte [3], const byte&, const byte&){}
    char getKey() { return ' '; }
};

struct DateTime {
    unsigned long unixtime(){ return 1; }
};

struct RTClib{
    DateTime now() {return DateTime(); }
};
struct {

    template<class T> void println(T x){
        std::cout << x << std::endl;
    }
    template<class T> void print(T x){}
    void begin(int x){}

} Serial;
struct {
    void begin() {}
} Wire;

struct TOTP{

    char* buffer = (char*) "000004";

    TOTP(const char [20], const int&){}
    char* getCode(int x) { return buffer; }
};

void pinMode(int BUTTON_OPEN_PIN, int INPUT){}
void tone(int BUZZER_PIN, int FREQ_BUTTON_PRESS, int SOUND_TIME_BUTTON_PRESS){}
void digitalWrite(int LOCK_PIN, int HIGH){}
void delay(int a){}
void delayMicroseconds(int a){}
int digitalRead(int pin) { return 1; }

#include "hsldz_totp_lock/hsldz_totp_lock.ino"

#include <sstream>
#include <iomanip>
#include <iostream>

int main()
{
    for(int i=0; i<1000000; i++) {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(6) << i;
        std::string s = ss.str();
        if(isTOTPCodeValid("01" + s))
        {
            std::cout << s << std::endl;
            break;
        }
    }
    setup();
//    for(;;) loop();
}
#endif
