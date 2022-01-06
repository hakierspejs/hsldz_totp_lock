import serial
import sys
import time
from datetime import datetime


def read_time(serial_device):
    with serial.Serial(serial_device, 115200, timeout=10) as ser:
        time.sleep(3)
        # print(ser.readline())
        # print(b'read \r\n')
        # write_serial(ser, 'read \r\n')
        ser.write(b'cread\r\n')
        ser.flush()
        curr_timestamp = time.time()
        line = ""
        value = ""
        for i in range(100000):
            line = ser.readline().strip()
            if line == b"DONE":
                break
            else:
                value = line
    ds3231_unix_timestamp = float(value.decode('utf-8'))
    deviation = abs(ds3231_unix_timestamp-curr_timestamp)
    print("DS3231:", ds3231_unix_timestamp, "UnixTimestamp:", curr_timestamp, "Deviation:",  deviation, "Is correct:",  deviation < 1.5)
    return ds3231_unix_timestamp


def set_time(serial_device):
    with serial.Serial(serial_device, 115200, timeout=10) as ser:
        time.sleep(3)
        ser.write(b'cset ')
        ser.flush()
        line = ""
        for i in range(100000):
            line = ser.readline().strip()
            # print(line)
            if line == b"START WRITING":
                break

        value = datetime.utcnow().strftime("%y%m%d%w%H%M%Sx").encode('utf-8')
        ser.write(value)
        ser.flush()
        time.sleep(0.1)
        # print(ser.readline().strip())

        line = ""
        for i in range(100000):
            line = ser.readline().strip()
            if line == b"DONE":
                break
            # print(line)


def main():
    serial_device = sys.argv[1]
    read_time(serial_device)
    set_time(serial_device)
    read_time(serial_device)
    print("Done!")


if __name__ == '__main__':
    main()

