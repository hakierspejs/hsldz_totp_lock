decrypt:
	openssl aes-256-cbc -d -a -in shared.secrets.enc -out shared.secrets
	openssl aes-256-cbc -d -a -in Secrets.h.enc -out Secrets.h
	openssl aes-256-cbc -d -a -in qrcodes.html.enc -out qrcodes.html

encrypt:
	openssl aes-256-cbc -a -salt -in shared.secrets -out shared.secrets.enc
	openssl aes-256-cbc -a -salt -in Secrets.h -out Secrets.h.enc
	openssl aes-256-cbc -a -salt -in qrcodes.html -out qrcodes.html.enc

genkeys:
	bash ./gen_secrets.sh

build:
	docker build -t hs-ldz-totp-lock .

genqr:
	docker run --mount src="$(shell pwd)",target=/usr/src/app,type=bind -it --rm hs-ldz-totp-lock python ./gen_qr_codes.py

showcode:
	docker run --mount src="$(shell pwd)",target=/usr/src/app,type=bind -it --rm hs-ldz-totp-lock python ./gen_qr_codes.py 1

sim:
	g++ -DMOCK test.cpp -o test && ./test

uno:
	arduino-cli compile --fqbn arduino:avr:uno --libraries vendor/librares/ hsldz_totp_lock.ino
