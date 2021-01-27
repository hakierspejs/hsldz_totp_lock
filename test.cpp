#ifdef MOCK
#include <string>
#define makeKeymap(x) ((char*)x)
typedef std::string String;
typedef char byte;
typedef bool boolean;

const int LOW=1;
const int HIGH=1;
const int INPUT=1;
const int OUTPUT=1;

struct Keypad{
    Keypad(char*, const byte [4], const byte [3], const byte&, const byte&){}
    char getKey() {}
};

struct DateTime {
    unsigned long unixtime(){}
};

struct RTClib{
    DateTime now() {}
};
struct {

    template<class T> void println(T x){}
    template<class T> void print(T x){}
    void begin(int x){}

} Serial;
struct {
    void begin() {}
} Wire;

struct TOTP{

    TOTP(const uint8_t [20], const int&){}
    char* getCode(int x) {}
};

void pinMode(int BUTTON_OPEN_PIN, int INPUT){}
void tone(int BUZZER_PIN, int FREQ_BUTTON_PRESS, int SOUND_TIME_BUTTON_PRESS){}
void digitalWrite(int LOCK_PIN, int HIGH){}
void delay(int a){}
int digitalRead(int pin) { return 1; }

#include "hsldz_totp_lock.ino"

int main()
{
}
#endif
