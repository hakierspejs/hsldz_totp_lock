decrypt:
	openssl aes-256-cbc -d -a -in shared.secrets.enc -out shared.secrets
	openssl aes-256-cbc -d -a -in Secrets.h.enc -out Secrets.h

encrypt:
	openssl aes-256-cbc -a -salt -in shared.secrets -out shared.secrets.enc
	openssl aes-256-cbc -a -salt -in Secrets.h -out Secrets.h.enc

genkeys:
	bash ./gen_secrets.sh

build:
	docker build -t hs-ldz-totp-lock .

genqr:
	docker run --mount src="$(shell pwd)",target=/usr/src/app,type=bind -it --rm hs-ldz-totp-lock python ./gen_qr_codes.py

showcode:
	docker run --mount src="$(shell pwd)",target=/usr/src/app,type=bind -it --rm hs-ldz-totp-lock python ./gen_qr_codes.py 1

