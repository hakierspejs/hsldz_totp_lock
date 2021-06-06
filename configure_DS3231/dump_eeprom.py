from binascii import unhexlify
import serial
import sys
import os
import time
from datetime import datetime

def getval(raw_value):
    return list(map(unhexlify, map(lambda x: x.zfill(2), raw_value.split())))

def get_checksum(value):
    checksum = 0x1A
    for i in value:
        checksum ^= ord(i)
    return checksum


def write_serial(ser, cmd):
    if ser.in_waiting == 0:
        for i in cmd:
            ser.write(i.encode())
            time.sleep(0.1)
    else:
        print("arrr")

def get_data(line):
    data, pos = line.split(b" - ")
    return getval(data.strip()), pos


def is_data_valid(data):
    return ord(data[-1]) == get_checksum(data[:-1])

def main():
    serial_device = sys.argv[1]
    out_filename = os.path.join(
        "backup",
        "eeprom_dump.{mark}.out".format(mark=datetime.now().strftime("%Y-%m-%d--h%Hm%Ms%S"))
    )
    dump_data = []
    with serial.Serial(serial_device, 115200, timeout=10) as ser:
        time.sleep(3)
        # print(ser.readline())
        # print(b'read \r\n')
        # write_serial(ser, 'read \r\n')
        ser.write(b'read \r\n')
        ser.flush()
        line = ""
        for i in range(100000):
            line = ser.readline().strip()
            if line == b"DONE":
                break
            data, pos = get_data(line)
            assert int(pos) == i * 32
            print(line)
            if is_data_valid(data):
                dump_data += data[:-1]
            else:
                print(hex(ord(data[-1])), hex(get_checksum(data[:-1])))
                raise
        # print(dump_data)
    with open(out_filename, 'wb') as out_file:
        out_file.write(b"".join(dump_data))
    print(out_filename)
    print("Done!")

if __name__ == '__main__':
    main()

