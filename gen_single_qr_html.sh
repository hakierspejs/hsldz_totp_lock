docker run --mount src="$(pwd)",target=/usr/src/app,type=bind -it --rm hs-ldz-totp-lock python ./gen_qr_codes.py html $1
