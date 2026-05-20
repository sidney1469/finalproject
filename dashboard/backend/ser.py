from serial import Serial, SerialException, SerialTimeoutException
import time
import random
import string

PORT = "/dev/ttyACM0"
BAUD = 115200
NUM_PLANES = 30


def random_flight_name():
    letters = ''.join(random.choices(string.ascii_uppercase, k=3))
    numbers = ''.join(random.choices(string.digits, k=3))
    return f"{letters}{numbers}"


def random_coord():
    # Arbitrary test range — not geographically meaningful
    return round(random.uniform(80.0, 100.0), 4)


def read_available(port, settle=0.3):
    """Wait for `settle` seconds then drain everything in the RX buffer."""
    time.sleep(settle)
    while port.in_waiting:
        line = port.readline()
        if line:
            print("RX:", line.decode(errors="replace").strip())


def read_for(port, duration=2.0):
    """Read and print all lines received within `duration` seconds."""
    end = time.time() + duration
    while time.time() < end:
        print("New Line")
        line = port.readline()
        if line:
            print(line.decode(errors="replace").strip())


try:
    with Serial(
        PORT,
        BAUD,
        timeout=0.2,
        write_timeout=1,
        rtscts=False,
        dsrdtr=False,
        xonxoff=False,
    ) as port:
        time.sleep(2)  # wait for board to boot and shell to initialise

        port.reset_input_buffer()
        port.reset_output_buffer()

        for i in range(NUM_PLANES):
            flight_name = random_flight_name()
            longitude = random_coord()
            latitude  = random_coord()
            loclong   = random_coord()
            loclat    = random_coord()

            command = (
                f"plane add {flight_name} "
                f"{longitude} {latitude} "
                f"{loclong} {loclat}\r\n"
            )

            try:
                port.write(command.encode())
            except SerialTimeoutException:
                print("Serial write timed out — board may not be reading fast enough.")
                break

            # Give Zephyr shell time to process, then drain echo/response
            read_available(port, settle=0.01)

        print("\nQuerying heap...\n")
        port.write(b"plane print\r\n")
        read_for(port, duration=2.0)

except SerialException as e:
    print(f"Could not open {PORT}: {e}")
    print("Check the board is connected and the port is correct.")