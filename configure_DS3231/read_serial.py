import serial
import sys
import time

def main():
    serial_device = sys.argv[1]
    speed = 115200
    try:
        speed = int(sys.argv[2])
    except Exception as e:
        pass
    with serial.Serial(serial_device, speed, timeout=3) as ser:
        time.sleep(3)
        for i in range(100000):
            line = ser.readline().strip()
            if line:
                print(line.decode('utf-8'))
    print("Done!")

if __name__ == '__main__':
    main()

