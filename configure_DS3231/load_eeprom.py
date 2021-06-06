from binascii import unhexlify
import serial
import sys
import time


def get_checksum(value): 
    checksum = 0x1A 
    for i in value: 
        checksum ^= ord(i) 
    return checksum 


def main():
    print('xxd -c 32 dumpfile.out')
    serial_device = sys.argv[1]
    filename = sys.argv[2]
    dump_data = b""
    with serial.Serial(serial_device, 115200, timeout=10) as ser: 
        time.sleep(3)
        # print(ser.readline()) 
        # print(b'read \r\n')
        # write_serial(ser, 'read \r\n')
        ser.write(b'write')
        ser.flush()
        line = ""
        for i in range(100000): 
            line = ser.readline().strip()
            print(line)
            if line == b"START WRITING":
                break

        with open(filename, 'rb') as input_file:
            raw_data = input_file.read()

        n = 32
        data = [raw_data[i:i+n] for i in range(0, len(raw_data), n)]
        for batch in data:
            print([hex(i) for i in batch])
            ser.write(batch)
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

