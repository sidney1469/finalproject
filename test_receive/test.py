import serial
import time

SERIAL_PORT = "/dev/ttyACM0"      # Change to your port (e.g. COM5 on Windows, /dev/ttyACM0 on Linux)
BAUD_RATE = 115200
TEST_LINE = "hello from python serial\n"

def main():
    with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
        time.sleep(2)  # give device time after opening port
        while (1):
            ser.write(TEST_LINE.encode("utf-8"))
            ser.flush()
            print(f"Sent: {TEST_LINE.strip()}")
            time.sleep(1)

if __name__ == "__main__":
    main()