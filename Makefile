DEVICE_SERIAL ?= /dev/ttyACM0
KEY_NUM ?= 99
DOCKER_ARDUINO_CLI = docker run --device=$(DEVICE_SERIAL) --mount src="$(shell pwd)",target=/usr/src/app,type=bind -it --rm arduino-cli
DOCKER_PYTHON_CLI = docker run --device=$(DEVICE_SERIAL) --mount src="$(shell pwd)",target=/usr/src/app,type=bind -it --rm hs-ldz-totp-lock

decrypt:
	openssl aes-256-cbc -d -a -md sha512 -pbkdf2 -iter 100000 -salt -pass env:HSLOCK_SECRET -in backup/rawdata.out.enc -out backup/rawdata.out

encrypt:
	openssl aes-256-cbc -a -md sha512 -pbkdf2 -iter 100000 -salt -pass env:HSLOCK_SECRET -in backup/rawdata.out -out backup/rawdata.out.enc

ac-build:
	docker build -t arduino-cli -f Dockerfile.arduino .

build:
	docker build -t hs-ldz-totp-lock .

genqr: build
	$(DOCKER_PYTHON_CLI) python ./utils/gen_qr_codes.py

gen-qr-code: build
	$(DOCKER_PYTHON_CLI) python ./utils/gen_qr_codes.py -f ./backup/rawdata.out -n $(KEY_NUM) -u qr-html-gen

show-qr-code: build
	$(DOCKER_PYTHON_CLI) python ./utils/gen_qr_codes.py -f ./backup/rawdata.out -n $(KEY_NUM) -u qr-console-gen

show-totp-code: build
	$(DOCKER_PYTHON_CLI) python ./utils/gen_qr_codes.py -f ./backup/rawdata.out -n $(KEY_NUM) -u totp-token-gen

generate-example-secrets: build
	$(DOCKER_PYTHON_CLI) python ./utils/gen_qr_codes.py -f ./backup/rawdata.out -u example-secrets-gen
	echo './backup/rawdata.out'
	echo 'xxd -c 22 -s 44 -l 22 ./backup/rawdata.out'
	xxd -c 22 -s 0 -l 88 ./backup/rawdata.out

uno-configure-ds3231: ac-build
	$(DOCKER_ARDUINO_CLI) arduino-cli board list
	$(DOCKER_ARDUINO_CLI) bash -c "cd /usr/src/app/configure_DS3231 && arduino-cli compile --fqbn arduino:avr:uno --libraries=../vendor/librares/ && arduino-cli upload -p $(DEVICE_SERIAL) --fqbn arduino:avr:uno"
	$(DOCKER_PYTHON_CLI) python ./configure_DS3231/configure_time.py $(DEVICE_SERIAL)
	$(DOCKER_PYTHON_CLI) python ./configure_DS3231/dump_eeprom.py $(DEVICE_SERIAL)
	$(DOCKER_PYTHON_CLI) python ./configure_DS3231/load_eeprom.py $(DEVICE_SERIAL) backup/rawdata.out

uno-upload-lock: ac-build
	$(DOCKER_ARDUINO_CLI) arduino-cli board list
	$(DOCKER_ARDUINO_CLI) bash -c "cd /usr/src/app/hsldz_totp_lock && arduino-cli compile --fqbn arduino:avr:uno --libraries=../vendor/librares/ && arduino-cli upload -p $(DEVICE_SERIAL) --fqbn arduino:avr:uno"

serial-read: build
	python3 ./configure_DS3231/read_serial.py $(DEVICE_SERIAL) 9600

uno-upload-isp: ac-build
	$(DOCKER_ARDUINO_CLI) bash -c "cd /usr/src/app/ArduinoISP && arduino-cli compile --fqbn arduino:avr:uno --libraries=../vendor/librares/ && arduino-cli upload --verbose --verify -p $(DEVICE_SERIAL) --fqbn arduino:avr:uno"

ac-compile: ac-build
	$(DOCKER_ARDUINO_CLI) bash -c "cd /usr/src/app/ArduinoISP && arduino-cli compile --fqbn arduino:avr:uno --libraries=../vendor/librares/"

sim:
	g++ -DMOCK test.cpp -o test && ./test

uno:
	arduino-cli compile --fqbn arduino:avr:uno --libraries vendor/librares/ hsldz_totp_lock.ino

pro-mini-upload-lock: ac-build
	$(DOCKER_ARDUINO_CLI) bash -c "cd /usr/src/app/hsldz_totp_lock && arduino-cli compile --fqbn arduino:avr:uno --libraries=../vendor/librares/ && arduino-cli upload --verbose --verify -p $(DEVICE_SERIAL) --fqbn arduino:avr:mini --programmer arduinoasisp"
