import pyotp
import sys
import os
from io import BytesIO
import pyqrcode
import typing
from base64 import b32decode, b32encode

SECRETS_FILE = 'shared.secrets'
OUT_FILE = 'qrcodes.html'

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
    '<div style="page-break-after: always"><br/><br/>'
    '<h1 style="text-align: center">The door key: {key:02d}</h1>{svg}</div>'
)


def splitline(line: str, size: int) -> typing.List[str]:
    return [line[i:i+size] for i in range(0, len(line), size)]


def print_secret(secret: str):
    data = splitline(secret, 2)
    value = ['0x' + i for i in data]
    key_repr = "{" + ", ".join(value) + '}, '
    print(key_repr)


def generate_qr_code(num: int, secret: str, console_print=False) -> str:
    uri = pyotp.totp.TOTP(b32encode(bytes.fromhex(secret))).provisioning_uri(
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
        print(' '.join(splitline(b32encode(bytes.fromhex(secret)).decode('utf-8'), 4)))
    svg_image = svg_io.read()
    result = svg_image.decode('utf-8').strip().split('\n')[-1]
    return result


def read_secrets() -> typing.Tuple[bytes, ...]:
    with open(SECRETS_FILE, 'r') as secrets_file:
        result = tuple(i.strip() for i in secrets_file.readlines() if i.strip())
    return result


def generate_html_qr_code(num: int, secret: str) -> str:
    qrcode = generate_qr_code(num, secret)
    codes = CODE_HTML_BLOCK.format(key=num, svg=qrcode)
    filename = "0x{num:02d}x{code}.html".format(num=num, code=b32encode(os.urandom(128)).decode('utf-8').replace('=', ''))
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


def main() -> None:
    secrets = read_secrets()
    print(sys.argv)
    if len(sys.argv) > 1:
        if sys.argv[1] == 'html':
            num = int(sys.argv[2])
            generate_html_qr_code(num, secrets[num])
        else:
            num = int(sys.argv[1])
            generate_qr_code(num, secrets[num], console_print=True)
    else:
        for secret in secrets:
            print_secret(secret)
        codes = ''
        for num, secret in enumerate(secrets):
            qrcode = generate_qr_code(num, secret)
            codes += CODE_HTML_BLOCK.format(key=num, svg=qrcode)
        with open(OUT_FILE, 'w') as outfile:
            outfile.write(CODES_HTML.format(code=codes))
        print(OUT_FILE)
    return


if __name__ == "__main__":
    main()
