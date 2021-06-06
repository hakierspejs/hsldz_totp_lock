from binascii import unhexlify
import serial
import sys
import time
from datetime import datetime


def get_checksum(value): 
    checksum = 0x1A 
    for i in value: 
        checksum ^= ord(i) 
    return checksum 


def main():
    serial_device = sys.argv[1]
    with serial.Serial(serial_device, 115200, timeout=10) as ser: 
        time.sleep(3)
        # print(ser.readline()) 
        # print(b'read \r\n')
        # write_serial(ser, 'read \r\n')
        ser.write(b'cset ')
        ser.flush()
        line = ""
        for i in range(100000): 
            line = ser.readline().strip()
            print(line)
            if line == b"START WRITING":
                break

        value = datetime.utcnow().strftime("%y%m%d%w%H%M%Sx").encode('utf-8')
        ser.write(value)
        ser.flush()
        time.sleep(0.1)
        print(ser.readline().strip())

        line = ""
        for i in range(100000): 
            line = ser.readline().strip()
            if line == b"DONE":
                break
            print(line)
    print("Done!")

if __name__ == '__main__':
    main()

