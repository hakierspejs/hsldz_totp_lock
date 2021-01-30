# The Hackerspace TOTP door lock

### Description
The TOTP-lock is based on Arduino Pro Mini Atmega328 - 5V 16MHz. The parts connected to the microcontroller are: 
 - [the relay](https://allegro.pl/oferta/modul-1-kanalowy-przekaznik-5v-10a-7953863748), which opens the electric strike ([PIN 12](https://github.com/hakierspejs/hsldz_totp_lock/blob/aedc9e4bd50019f3f2bc459a3ce09191bc713dde/hsldz_totp_lock.ino#L10)) 
 - [the pin pad 4x4](https://allegro.pl/oferta/klawiatura-membranowa-numeryczna-4x4-arduino-9611679849), which allows to input TOTP-codes ([PINs 2-8](https://github.com/hakierspejs/hsldz_totp_lock/blob/aedc9e4bd50019f3f2bc459a3ce09191bc713dde/hsldz_totp_lock.ino#L32))
 - the buzzer, to signal about wrong/correct input ([PIN 10](https://github.com/hakierspejs/hsldz_totp_lock/blob/aedc9e4bd50019f3f2bc459a3ce09191bc713dde/hsldz_totp_lock.ino#L9))
 - the button to open the door from the inside  ([PIN 11](https://github.com/hakierspejs/hsldz_totp_lock/blob/aedc9e4bd50019f3f2bc459a3ce09191bc713dde/hsldz_totp_lock.ino#L11))
 
The open-button is connected via a [software debouncer](https://github.com/hakierspejs/hsldz_totp_lock/blob/aedc9e4bd50019f3f2bc459a3ce09191bc713dde/hsldz_totp_lock.ino#L113) and hardware low-pass filter with a pull-up resistor:
![image alt](https://i.imgur.com/PGhv3sQ.png)
It's necessary to avoid spontaneous door opening because of an electromagnetic noise (for instance, when fluorescent lamps turn on)


There is also a [DC-DC converter 12V->5V](https://www.aphelektra.com/regulatory-napiecia/6033-modul-przetwornicy-step-down-mp2307-1v-17v-18a.html). 

### Known issues:
 - too high memory consumption because of in-memory secrets placement, it's necessary to place secrets on the additional i2c memory chip (24c64 e. g.)
 - spontaneous door opening  because of an electromagnetic noise  (solved with software debouncer and LP-filter)
 - spontaneous PinPad presses because of an electromagnetic noise (may be solved by cable with a concentric conducting shield)
 - RTC desynchronization (solved with DS3231)
 
### Debugging 
* disconnect 12v power cable from the UPS 
* disconnect 12v cable from the TOTP-lock 
* disconnect the pin pad Rj-45 
* disconnect the electric strike cable 
 
**Now you can remove (unhook) the TOTP-lock case from the door**
* disconnect the voltage converter from the Arduino board  
* disconnect the relay from the Arduino board if you are not going to debug it
* you may want to disconnect the buzzer because it's too loud
* connect to the microcontroller via Serial USB-TTL adapter from your laptop

Now you able to upload the sketch via Arduino IDE and see the output in the serial monitor.
By default, every time the lock opens microcontroller [prints out the Unix timestamp from the RTC to the serial interface](https://github.com/hakierspejs/hsldz_totp_lock/blob/aedc9e4bd50019f3f2bc459a3ce09191bc713dde/hsldz_totp_lock.ino#L9).
  

### QR-codes distribution
There is an ability to generate QR-code of the necessary secret. 
``` 
gen_single_qr_html.sh 12
``` 
You may share the QR-code-HTML in any eligible way.

There is also an ability to print out all QR-codes - you have to decrypt secrets.
```
make decrypt 
```

### Other

To generate new qrcode file:
```
make build
make genqr
```

To view one qrcode:
```
make showcode
```
-----------------------------
Feel free to contribute
