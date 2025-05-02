import serial
import time
import os
from datetime import datetime
import csv
import signal
import sys
import requests

SERIAL_PORT = "COM4"
BAUD_RATE = 115200
READ_FILE = "response_file.txt"
WRITE_FILE = "command_file.txt"
TEMP_CSV = "SampleTemps.csv"
DIRECTORY_PATH = "./"

def sigint_handler(signum, frame):
    if ser and ser.is_open:
        ser.close()
        print("Serial connection closed.")
    sys.exit(0)

signal.signal(signal.SIGINT, sigint_handler)

def setup():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)
        print(f"Connected to {SERIAL_PORT}")
        return ser
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        return None

def queryForWaterLevel():
    url = "https://api.tidesandcurrents.noaa.gov/api/prod/datagetter?date=latest&station=9412110&product=water_level&datum=MLLW&time_zone=lst&units=metric&format=json"
    try:
        response = requests.get(url)
        if response.status_code == 200:
            data = response.json()
            return data['data'][0]['v']
        else:
            return -1000
    except requests.exceptions.RequestException:
        return -1000

def readCommand(ser):
    if ser.in_waiting:
        response = ser.readline().decode().strip()
        return response
    return None

def communicate(ser, sample_time_sec):
    try:
        with open(READ_FILE, "r") as input, open(WRITE_FILE, "w") as output:
            now = datetime.now()
            curr_time = now.strftime("%y%m%d_%H%M%S")
            directory = DIRECTORY_PATH + curr_time
            os.makedirs(directory, exist_ok=True)

            while True:
                output.seek(0)
                save_dir = "SaveToDirectory(" + directory + ")"
                output.write(save_dir)
                output.flush()

                while True:
                    input.seek(0)
                    recv = input.read()
                    if len(recv) >= 16:
                        break

                if recv[0] == "0" and recv[1:16].lower() == "savetodirectory":
                    break
                os.makedirs(directory, exist_ok=True)

            while True:
                output.close()
                output = open(WRITE_FILE, "w")
                output.write("StartPump()")
                output.flush()

                while True:
                    input.seek(0)
                    recv = input.read()
                    if len(recv) >= 10:
                        break

                if recv[0] == "0" and recv[1:10].lower() == "startpump":
                    break

            print(f"Starting {sample_time_sec // 60} minute {sample_time_sec % 60} second timer")

            frame_number = "1"
            while True:
                output.close()
                output = open(WRITE_FILE, "w")
                output.write("StartSampleCollection(" + frame_number + ")")
                output.flush()

                while True:
                    input.seek(0)
                    recv = input.read()
                    if len(recv) >= 22:
                        break

                if recv[0] == "0" and recv[1:22].lower() == "startsamplecollection":
                    break

            print(f"Collecting temperatures for {sample_time_sec} seconds")
            csv_file = open(TEMP_CSV, "a", newline="\n")
            writer = csv.writer(csv_file)
            if os.stat(TEMP_CSV).st_size == 0:
                writer.writerow(["Timestamp", "Min Temp(C)", "Max Temp(C)", "Avg Temp(C)"])

            temperatures = []
            num_samples = sample_time_sec // 15

            for i in range(num_samples):
                time.sleep(15)
                ser.write("T\n".encode())
                now = datetime.now()
                curr_time = now.strftime("%y-%m-%d_%H:%M:%S")

                while True:
                    if ser.in_waiting:
                        temp_data = ser.readline().decode().strip()
                        if temp_data.isdigit():
                            print(f"{curr_time} - Temperature: {temp_data}°C")
                            temperatures.append(int(temp_data))
                            break
                    time.sleep(0.5)

            min_temp = min(temperatures)
            max_temp = max(temperatures)
            average_temp = sum(temperatures) / len(temperatures)
            print(f"Min: {min_temp}, Max: {max_temp}, Avg: {average_temp:.2f}")
            writer.writerow([curr_time, min_temp, max_temp, average_temp])
            csv_file.close()

            while True:
                output.close()
                output = open(WRITE_FILE, "w")
                output.write("StopSampleCollection()")
                output.flush()

                while True:
                    input.seek(0)
                    recv = input.read()
                    if len(recv) >= 21:
                        break

                if recv[0] == "0" and recv[1:21].lower() == "stopsamplecollection":
                    break

        ser.write("D\n".encode())
        input.close()
        output.close()

    except Exception as e:
        print(f"Error during communication: {e}")
        if ser and ser.is_open:
            ser.close()

def stopPump(ser):
    try:
        with open(READ_FILE, "r") as input, open(WRITE_FILE, "w") as output:
            while True:
                output.close()
                output = open(WRITE_FILE, "w")
                output.write("StopPump()")
                output.flush()

                while True:
                    input.seek(0)
                    recv = input.read()
                    if len(recv) >= 9:
                        break

                if recv[0] == "0" and recv[1:9].lower() == "stoppump":
                    break

        ser.write("D\n".encode())
        input.close()
        output.close()

    except Exception as e:
        print(f"Error stopping pump: {e}")
        if ser and ser.is_open:
            ser.close()

def startPump(ser):
    try:
        with open(READ_FILE, "r") as input, open(WRITE_FILE, "w") as output:
            while True:
                output.close()
                output = open(WRITE_FILE, "w")
                output.write("StartPump()")
                output.flush()

                while True:
                    input.seek(0)
                    recv = input.read()
                    if len(recv) >= 10:
                        break

                if recv[0] == "0" and recv[1:10].lower() == "startpump":
                    break

        ser.write("D\n".encode())
        input.close()
        output.close()

    except Exception as e:
        print(f"Error starting pump: {e}")
        if ser and ser.is_open:
            ser.close()

def sendEpochTime(ser):
    try:
        epoch_time = int(time.time())
        print(f"Sending epoch time: {epoch_time}")
        ser.write((str(epoch_time) + "\n").encode())
        return epoch_time
    except Exception as e:
        print(f"Error sending epoch time: {e}")
        return None

if __name__ == "__main__":
    ser = setup()
    while ser is None:
        print("Unable to set up serial connection. Retrying...")
        ser = setup()

    while True:
        if ser is None or not ser.is_open:
            print("Serial disconnected. Reconnecting...")
            ser = setup()
            continue

        write_to = readCommand(ser)
        if write_to is None:
            continue

        if write_to == "T":
            tide_level = queryForWaterLevel()
            print(f"Tide level is {tide_level}")
            ser.write((str(tide_level) + "\n").encode())
        elif write_to == "S":
            # Read next line: sample time
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
        elif write_to == "F":
            stopPump(ser)
        elif write_to == "P":
            startPump(ser)
        elif write_to == "C":
            sendEpochTime(ser)
