import serial
import serial.tools.list_ports
import glob
import time
import os
from datetime import datetime
import csv
import signal
import sys
import requests
import platform
import pzyt


BAUD_RATE = 115200
READ_FILE = "response_file.txt"
WRITE_FILE = "command_file.txt"
TEMP_CSV = "SampleTemps.csv"
DIRECTORY_PATH = "D:/Data/Raw"
NOAA_TIDE_LEVEL_QUERY_URL = "https://api.tidesandcurrents.noaa.gov/api/prod/datagetter?date=latest&station=9412110&product=water_level&datum=MLLW&time_zone=lst&units=metric&format=json"

TIDE_LEVEL_QUERY_TYPE   = "T"
SAMPLE_MESSAGE_TYPE     = "S"
STOP_PUMP_MESSAGE_TYPE  = "F"
START_PUMP_MESSAGE_TYPE = "P"
EPOCH_TIME_QUERY_TYPE   = "C"

CLI_DEBUG_MODE = False  # Set to False for real serial communication, True if you want to simulate usage on the command line

class DebugSerial:
    """Simulates a serial.Serial-like interface for debugging."""
    def __init__(self):
        self.buffer = []
        self.is_open = True

    def write(self, data):
        print(f"[DEBUG SENT]: {data.decode().strip()}")

    def readline(self):
        user_input = input("[DEBUG RECEIVE] > ")
        return user_input.encode()

    def close(self):
        print("[DEBUG] Serial connection closed.")
        self.is_open = False

    @property
    def in_waiting(self):
        return True  # Always ready to simulate input


def detect_serial_port():
    """Detects the appropriate serial port based on OS."""
    if CLI_DEBUG_MODE:
        return None  # Not used in debug mode

    system = platform.system()
    if system == "Windows":
        return "COM3"
    elif system == "Linux":
        for pattern in ["/dev/tty0", "/dev/ttyACM*"]:
            ports = glob.glob(pattern)
            if ports:
                return ports[0]
    elif system == "Darwin":
        for pattern in ["/dev/tty.usb*", "/dev/cu.usb*"]:
            ports = glob.glob(pattern)
            if ports:
                return ports[0]
    raise RuntimeError("No valid serial port found for your OS.")



def sigint_handler(signum, frame):
    if ser and ser.is_open:
        ser.close()
        print("Serial connection closed.")
    sys.exit(0)

signal.signal(signal.SIGINT, sigint_handler)

def setup():
    if CLI_DEBUG_MODE:
        print("[DEBUG] Using mock serial interface.")
        return DebugSerial()

    import serial
    port = detect_serial_port()
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)
        print(f"Connected to {port}")
        return ser
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        return None

def queryForWaterLevel():
    try:
        response = requests.get(NOAA_TIDE_LEVEL_QUERY_URL)
        if response.status_code == 200:
            data = response.json()
            return data['data'][0]['v']
        else:
            return -1000
    except requests.exceptions.RequestException:
        return -1000

def wait_for_file_response(expected_prefix, expected_content, min_length):
    with open(READ_FILE, "r") as input:
        while True:
            input.seek(0)
            recv = input.read()
            if len(recv) >= min_length:
                if recv[0] == expected_prefix and recv[1:1+len(expected_content)].lower() == expected_content:
                    return True
            time.sleep(0.1)

def write_command(command):
    with open(WRITE_FILE, "w") as output:
        output.write(command)
        output.flush()

def communicate(ser, sample_time_sec):
    try:
        now = datetime.now()
        curr_time = now.strftime("%y%m%d_%H%M%S")
        directory = os.path.join(DIRECTORY_PATH, curr_time)
        os.makedirs(directory, exist_ok=True)

        write_command(f"SaveToDirectory({directory})")
        wait_for_file_response("0", "savetodirectory", 16)

        write_command("StartPump()")
        wait_for_file_response("0", "startpump", 10)

        print(f"Starting {sample_time_sec // 60} minute {sample_time_sec % 60} second timer")

        write_command("StartSampleCollection(1)")
        wait_for_file_response("0", "startsamplecollection", 22)

        print(f"Collecting temperatures for {sample_time_sec} seconds")
        with open(TEMP_CSV, "a", newline="\n") as csv_file:
            writer = csv.writer(csv_file)
            if os.stat(TEMP_CSV).st_size == 0:
                writer.writerow(["Timestamp", "Min Temp(C)", "Max Temp(C)", "Avg Temp(C)"])

            temperatures = []
            num_samples = (sample_time_sec // 15) - 1
            print(num_samples)
            for _ in range(num_samples):
                time.sleep(15)
                ser.write("T\n".encode())
                print("sending temp request!")
                now = datetime.now()
                curr_time = now.strftime("%y-%m-%d_%H:%M:%S")
                tmp = 0
                while tmp < 10:
                    print("in true loop!")
                    if ser.in_waiting:
                        print("got temp data!")
                        temp_data = ser.readline().decode().strip()
                        if temp_data.isdigit():
                            print(f"{curr_time} - Temperature: {temp_data}°C")
                            temperatures.append(int(temp_data))
                            break
                    tmp += 1
                    time.sleep(0.5)

            min_temp = min(temperatures)
            max_temp = max(temperatures)
            average_temp = sum(temperatures) / len(temperatures)
            print(f"Min: {min_temp}, Max: {max_temp}, Avg: {average_temp:.2f}")
            writer.writerow([curr_time, min_temp, max_temp, average_temp])

        write_command("StopSampleCollection()")
        wait_for_file_response("0", "stopsamplecollection", 21)

        ser.write("D\n".encode())

    except Exception as e:
        print(f"Error during communication: {e}")
        if ser and ser.is_open:
            ser.close()

def controlPump(ser, command_name, expected_ack):
    try:
        write_command(command_name)
        wait_for_file_response("0", expected_ack, len(expected_ack))
        ser.write("D\n".encode())
    except Exception as e:
        print(f"Error with pump command '{command_name}': {e}")
        if ser and ser.is_open:
            ser.close()

def stopPump(ser):
    controlPump(ser, "StopPump()", "stoppump")

def startPump(ser):
    controlPump(ser, "StartPump()", "startpump")

def sendEpochTime(ser):
    try:
        pacific_tz = pytz.timezone('US/Pacific')


        naive_now = datetime.now()  # naive local time (no tz info)
        pacific_now = pacific_tz.localize(naive_now, is_dst=None)  # let pytz determine DST
        epoch_time = int(pacific_now.timestamp())

        #epoch_time = int(pacific_now.timestamp())
        print(f"Sending epoch time: {epoch_time}")
        ser.write((str(epoch_time) + "\n").encode())
        return epoch_time
    except Exception as e:
        print(f"Error sending epoch time: {e}")
        return None

if __name__ == "__main__":
    ser = setup()
    while ser is None:
        time.sleep(0.5)
        print("Unable to set up serial connection. Retrying...")
        ser = setup()

    while True:
        if ser is None or not ser.is_open:
            print("Serial disconnected. Reconnecting...")
            ser = setup()
            continue

        write_to = ser.readline().decode().strip() if ser.in_waiting else None
        if not write_to:
            continue

        if write_to == TIDE_LEVEL_QUERY_TYPE:
            tide_level = queryForWaterLevel()
            print(f"Tide level is {tide_level}")
            ser.write((str(tide_level) + "\n").encode())

        elif write_to == SAMPLE_MESSAGE_TYPE:
            time_line = None
            start = time.time()
            while time.time() - start < 2:
                if ser.in_waiting:
                    time_line = ser.readline().decode().strip()
                    break
                time.sleep(0.1)

            if time_line and time_line.isdigit():
                sample_time_sec = int(time_line)
                print(f"Received sample time: {sample_time_sec} seconds")
                communicate(ser, sample_time_sec)
            else:
                print("Warning: Expected sample time value after 'S' but none received.")

        elif write_to == STOP_PUMP_MESSAGE_TYPE:
            stopPump(ser)

        elif write_to == START_PUMP_MESSAGE_TYPE:
            startPump(ser)

        elif write_to == EPOCH_TIME_QUERY_TYPE:
            sendEpochTime(ser)
        else:
           print(f"ERR: RECEIVED UNKNOWN COMMAND {write_to}")
