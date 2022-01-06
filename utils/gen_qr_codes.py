import pyotp
import sys
from enum import Enum, auto
import os
from io import BytesIO
import argparse
import pyqrcode
import typing
from pathlib import Path
from base64 import b32decode, b32encode



def _splitline(line: str, size: int) -> typing.List[str]:
    return [line[i:i+size] for i in range(0, len(line), size)]


def _print_secret(secret: str):
    data = _splitline(secret, 2)
    value = ['0x' + i for i in data]
    key_repr = "{" + ", ".join(value) + '}, '
    print(key_repr)


def get_secret_checksum(value):
    checksum = 0x1A
    for i in value:
        checksum ^= i
    return checksum


def get_key_secret(num: int, secrets: bytes):
    secret_len = 22
    offset = secret_len * num
    # print(offset, secret_len)
    secret = secrets[offset:offset + secret_len]
    checksum = get_secret_checksum(secret[:-1])
    assert secret[-2] == 0xFF
    assert secret[-1] == checksum
    return secret[:20]


def generate_qr_code(num: int, secrets: bytes, console_print=False) -> str:
    secret = get_key_secret(num, secrets)
    uri = pyotp.totp.TOTP(b32encode(secret)).provisioning_uri(
        name='key {num:02d}'.format(num=num),
        issuer_name='Hackerspace Łódź'
    )
    qr_code = pyqrcode.create(uri, error='L')
    svg_io = BytesIO()
    qr_code.svg(svg_io, scale=10)
    svg_io.seek(0)
    if console_print:
        print(uri)
        print(qr_code.terminal('red', 'white'))
        print()
        print(' '.join(_splitline(b32encode(secret).decode('utf-8'), 4)))
    svg_image = svg_io.read()
    result = svg_image.decode('utf-8').strip().split('\n')[-1]
    return result


CODES_HTML = '''
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
</head>
<body>
{code}
</body>
</html>
'''

CODE_HTML_BLOCK = (
    '<div style="page-break-after: always; display: block; text-align: center;"><br/><br/>'
    '<h1 style="text-align: center">The door key: {key:02d}</h1>{svg}'
    '<h4 style="text-align: center">{key_repr}</h4>'
    '</div>'
)

def generate_html_qr_code(num: int, secrets: bytes) -> None:
    qrcode = generate_qr_code(num, secrets)
    secret = get_key_secret(num, secrets)
    key_repr = ' '.join(_splitline(b32encode(secret).decode('utf-8'), 4))
    codes = CODE_HTML_BLOCK.format(key=num, svg=qrcode, key_repr=key_repr)
    filename = "0x{num:02d}x{code}.html".format(num=num, code=b32encode(os.urandom(128)).decode('utf-8').replace('=', ''))
    if os.path.exists(filename):
        os.remove(filename)
    Path(filename).touch(mode=0o600)
    with open(filename, 'w') as outfile:
        outfile.write(CODES_HTML.format(code=codes))
    print(filename)
    print("="*100)
    print("scp ./{filename} hs-ldz.pl:~/jedenGET/data/ && rm ./0x*.html".format(filename=filename))
    print("="*100)
    print(" "*100)
    print("Username: hsldz")
    print("Password: zielona")
    print("https://jget.hs-ldz.pl/files/{filename}".format(filename=filename))
    print("="*100)
    print(" "*100)
    return


def generate_totp_token(num: int, secrets: bytes) -> None:
    secret = get_key_secret(num, secrets)
    totp_instance = pyotp.totp.TOTP(b32encode(secret))
    print("*{num:02}{token}#".format(num=num, token=totp_instance.now()))
    return


def generate_example_secrets(filename: str, size=4096) -> None:
    assert not os.path.exists(filename)
    Path(filename).touch(mode=0o600)
    result = []
    for i in range(1024):
        raw_key = [i for i in bytearray(os.urandom(20))]
        raw_key += [0xFF]  # key is active
        checksum = get_secret_checksum(raw_key)
        key = raw_key + [checksum]
        result += key
    with open(filename, 'wb') as datafile:
        datafile.write(bytes(result[:size]))
    return


def read_secrets(filepath) -> bytes:
    with open(filepath, 'rb') as secrets_file:
        result = secrets_file.read()
    return result


class UseCases(Enum):
    QR_HTML_GEN = 'qr-html-gen'
    QR_CONSOLE_GEN = 'qr-console-gen'
    TOTP_TOKEN_GEN = 'totp-token-gen'
    EXAMPLE_SECRETS_GEN = 'example-secrets-gen'

    def __str__(self):
        return self.value

    def __repr__(self):
        return str(self.value)

    @staticmethod
    def argparse(value):
        try:
            return UseCases(value)
        except KeyError:
            return value


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--filepath', help='eeprom dump path')
    parser.add_argument('-n', '--num', type=int, help='TOTP key number')
    parser.add_argument('-u', '--use-case', type=UseCases.argparse, choices=list(UseCases), help='use case to execute')
    args = parser.parse_args()
    # print(args)
    match args.use_case:
        case UseCases.EXAMPLE_SECRETS_GEN:
            generate_example_secrets(args.filepath)
        case UseCases.QR_CONSOLE_GEN:
            secrets = read_secrets(args.filepath)
            generate_qr_code(args.num, secrets, console_print=True)
        case UseCases.QR_HTML_GEN:
            secrets = read_secrets(args.filepath)
            generate_html_qr_code(args.num, secrets)
        case UseCases.TOTP_TOKEN_GEN:
            secrets = read_secrets(args.filepath)
            generate_totp_token(args.num, secrets)
    return


if __name__ == "__main__":
    main()
