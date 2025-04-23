import serial
import time
import os
from datetime import datetime
import csv
import signal
import sys
import requests

# SERIAL_PORT = "COM4"  # For use with Windows
SERIAL_PORT = "/dev/tty0"    # For Linux
BAUD_RATE = 115200
READ_FILE = "response_file.txt"
WRITE_FILE = "command_file.txt"
TEMP_CSV = "SampleTemps.csv"
DIRECTORY_PATH = "./"

# Timeout constants (in seconds)
SERIAL_TIMEOUT = 5
FILE_READ_TIMEOUT = 10
API_TIMEOUT = 30
ACK_TIMEOUT = 20

def sigint_handler(signum, frame):
    global ser
    if 'ser' in globals() and ser and ser.is_open:
        ser.close()
        print("Serial connection closed.")
    sys.exit(0)

signal.signal(signal.SIGINT, sigint_handler)

def setup():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=SERIAL_TIMEOUT)
        time.sleep(2)  # Give Arduino time to reset
        print(f"Connected to {SERIAL_PORT}")
        return ser
    
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        return None

def queryForWaterLevel():
    url = "https://api.tidesandcurrents.noaa.gov/api/prod/datagetter?date=latest&station=9412110&product=water_level&datum=MLLW&time_zone=lst&units=metric&format=json"
    # Queries for the current water level from Port San Luis pier, updates every 6 minutes.

    # Tries to get NOAA data from url with timeout
    try:
        response = requests.get(url, timeout=API_TIMEOUT)
        if response.status_code == 200:
            data = response.json()
            water_level = data['data'][0]['v']
            return water_level
        else:
            print(f"API request failed with status code: {response.status_code}")
            return -1000
    except requests.exceptions.RequestException as e:
        print(f"API request error: {e}")
        return -1000

def readCommand(ser):
    # Read arduino response for gather Tide data or writing to Aqusens
    if ser.in_waiting:
        response = ser.readline().decode().strip()
        return response
    # If nothing in serial return None
    return None

def wait_for_file_content(file, min_length, timeout=FILE_READ_TIMEOUT):
    """Wait for file to contain content of minimum length with timeout."""
    start_time = time.time()
    while time.time() - start_time < timeout:
        file.seek(0)
        content = file.read()
        if len(content) >= min_length:
            return content
        time.sleep(0.5)
    return None  # Timeout occurred

def communicate(ser):
    try:
        with open(READ_FILE, "r") as input, open(WRITE_FILE, "w") as output:
            # Create Sample Folder
            now = datetime.now()
            curr_time = now.strftime("%y%m%d_%H%M%S")

            directory = DIRECTORY_PATH + curr_time
            print(f"Creating directory: {directory}")
            os.makedirs(directory, exist_ok=True)

            # Sampling Process ------------------------------------------
            # Set directory for data
            save_dir = f"SaveToDirectory({directory})"
            
            for attempt in range(3):  # Retry up to 3 times
                output.seek(0)
                output.truncate(0)  # Clear the file
                output.write(save_dir)
                output.flush()
                
                # Wait for ACK with timeout
                recv = wait_for_file_content(input, 16, ACK_TIMEOUT)
                if recv is None:
                    print(f"Timeout waiting for SaveToDirectory ACK, attempt {attempt+1}")
                    continue
                
                # Process ACK
                if recv[0] == "0" and recv[1:16].lower() == "savetodirectory":
                    print("Received SaveToDirectory ACK")
                    break  # Got a successful ACK
                else:
                    print(f"Invalid ACK received: {recv}")
            else:
                print("Failed to get SaveToDirectory ACK after retries")
                return

            # Starting Pump ------------------------------------------------
            for attempt in range(3):  # Retry up to 3 times
                output.close()
                output = open(WRITE_FILE, "w")
                
                output.write("StartPump()")
                output.flush()
                
                # Wait for ACK with timeout
                recv = wait_for_file_content(input, 10, ACK_TIMEOUT)
                if recv is None:
                    print(f"Timeout waiting for StartPump ACK, attempt {attempt+1}")
                    continue
                
                # Process ACK
                if recv[0] == "0" and recv[1:10].lower() == "startpump":
                    print("Received StartPump ACK")
                    break  # Got a successful ACK
                else:
                    print(f"Invalid ACK received: {recv}")
            else:
                print("Failed to get StartPump ACK after retries")
                return

            # Start 2min Timer for sample to Aqusens ------------------------
            print("Starting 2 minute timer")
            # time.sleep(60 * 2)  # Commented out in original code

            # Start Sample Collection ---------------------------------------
            frame_number = "1"
            for attempt in range(3):  # Retry up to 3 times
                output.close()
                output = open(WRITE_FILE, "w")
                
                start_sample = f"StartSampleCollection({frame_number})"
                output.write(start_sample)
                output.flush()
                
                # Wait for ACK with timeout
                recv = wait_for_file_content(input, 22, ACK_TIMEOUT)
                if recv is None:
                    print(f"Timeout waiting for StartSampleCollection ACK, attempt {attempt+1}")
                    continue
                
                # Process ACK
                if recv[0] == "0" and recv[1:22].lower() == "startsamplecollection":
                    print("Received StartSampleCollection ACK")
                    break  # Got a successful ACK
                else:
                    print(f"Invalid ACK received: {recv}")
            else:
                print("Failed to get StartSampleCollection ACK after retries")
                return
            
            # Start collecting temperature data -------------------------
            print("Starting temperature collection (5 minute process)")

            # create CSV file
            csv_file = open(TEMP_CSV, "a", newline="\n")
            writer = csv.writer(csv_file)
            # Create CSV header if empty
            if os.stat(TEMP_CSV).st_size == 0:
                writer.writerow(["Timestamp", "Min Temp(C)", "Max Temp(C)", "Avg Temp(C)"])

            temperatures = []

            # Each 15 seconds request temperature -- 20 total requests
            for i in range(20):
                time.sleep(15)
                ser.write("T\n".encode())
                # Get Current date and time
                now = datetime.now()
                curr_time = now.strftime("%y-%m-%d_%H:%M:%S")

                # Wait for temperature with timeout
                start_time = time.time()
                temp_received = False
                
                while time.time() - start_time < SERIAL_TIMEOUT:
                    if ser.in_waiting:
                        temp_data = ser.readline().decode().strip()
                        if temp_data.isdigit():
                            print(f"{curr_time} - Temperature: {temp_data}°C")
                            temperatures.append(int(temp_data))
                            temp_received = True
                            break
                    time.sleep(0.5)
                
                if not temp_received:
                    print(f"Timeout waiting for temperature reading {i+1}")
                    # Use previous temperature if available, or a default value
                    if temperatures:
                        temperatures.append(temperatures[-1])
                    else:
                        temperatures.append(0)  # Default value if no readings yet

            if temperatures:
                min_temp = min(temperatures)
                max_temp = max(temperatures)
                average_temp = sum(temperatures) / len(temperatures)
                print(f"Min temp: {min_temp}, Max temp: {max_temp}, Average temp: {average_temp}")
                writer.writerow([curr_time, min_temp, max_temp, average_temp])
            else:
                print("No temperature readings collected")
            
            csv_file.close()

            # Stop Sample Collection -----------------------------------------
            for attempt in range(3):  # Retry up to 3 times
                output.close()
                output = open(WRITE_FILE, "w")
                
                output.write("StopSampleCollection()")
                output.flush()
                
                # Wait for ACK with timeout
                recv = wait_for_file_content(input, 21, ACK_TIMEOUT)
                if recv is None:
                    print(f"Timeout waiting for StopSampleCollection ACK, attempt {attempt+1}")
                    continue
                
                # Process ACK
                if recv[0] == "0" and recv[1:21].lower() == "stopsamplecollection":
                    print("Received StopSampleCollection ACK")
                    break  # Got a successful ACK
                else:
                    print(f"Invalid ACK received: {recv}")
            else:
                print("Failed to get StopSampleCollection ACK after retries")
        
        # send done signal to Arduino
        ser.write("D\n".encode())
        print("Sample collection process completed")

    except serial.SerialException as e:
        print(f"Serial error: {e}")
        if ser and ser.is_open:
            ser.close()
    except FileNotFoundError as e:
        print(f"File error: {e}")
        if ser and ser.is_open:
            ser.close()
    except Exception as e:
        print(f"Unexpected error: {e}")
        if ser and ser.is_open:
            ser.close()

def flush(ser):
    try:
        with open(READ_FILE, "r") as input, open(WRITE_FILE, "w") as output:
            # Stop Pump -----------------------------------------
            for attempt in range(3):  # Retry up to 3 times
                output.seek(0)
                output.truncate(0)  # Clear the file
                output.write("StopPump()")
                output.flush()
                
                # Wait for ACK with timeout
                recv = wait_for_file_content(input, 9, ACK_TIMEOUT)
                if recv is None:
                    print(f"Timeout waiting for StopPump ACK, attempt {attempt+1}")
                    continue
                
                # Process ACK
                if recv[0] == "0" and recv[1:9].lower() == "stoppump":
                    print("Received StopPump ACK")
                    break  # Got a successful ACK
                else:
                    print(f"Invalid ACK received: {recv}")
            else:
                print("Failed to get StopPump ACK after retries")
        
        # send done signal to Arduino
        ser.write("D\n".encode())
        print("Flush process completed")
        
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        if ser and ser.is_open:
            ser.close()
    except FileNotFoundError as e:
        print(f"File error: {e}")
        if ser and ser.is_open:
            ser.close()
    except Exception as e:
        print(f"Unexpected error: {e}")
        if ser and ser.is_open:
            ser.close()

def handle_command(command, ser):
    if command == "T":
        tide_level = queryForWaterLevel()
        print(f"Tide level is {tide_level}")
        ser.write((str(tide_level) + "\n").encode())
    elif command == "S":
        communicate(ser)
    elif command == "F":
        flush(ser)
    else:
        print(f"Unknown command received: {command}")

if __name__ == "__main__":
    ser = None
    retry_count = 0
    max_retries = 5
    
    while ser is None and retry_count < max_retries:
        print(f"Attempt {retry_count + 1}/{max_retries} to set up serial connection...")
        ser = setup()
        if ser is None:
            retry_count += 1
            time.sleep(2)  # Wait before retry
    
    if ser is None:
        print("Failed to establish serial connection after maximum retries. Exiting.")
        sys.exit(1)

    try:
        while True:
            try:
                command = ser.readline().decode().strip()
                if command:
                    print(f"Received command: {command}")
                    handle_command(command, ser)
            except serial.SerialTimeoutException:
                pass  # Timeout is expected, continue
            except serial.SerialException as e:
                print(f"Serial error: {e}")
                # Try to reconnect
                ser.close()
                ser = setup()
                if ser is None:
                    print("Failed to reconnect. Exiting.")
                    sys.exit(1)
    except KeyboardInterrupt:
        sigint_handler(None, None)