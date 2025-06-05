# import serial
# import serial.tools.list_ports
# import glob
# import time
# import os
# from datetime import datetime
# import csv
# import signal
# import sys
# import requests
# import platform
# import calendar
# import pytz
# from terminalThread import *
# import email_errs



# BAUD_RATE = 115200
# READ_FILE = "response_file.txt"
# WRITE_FILE = "command_file.txt"
# TEMP_CSV = "SampleTemps.csv"
# DIRECTORY_PATH = "D:/Data/Raw"
# NOAA_TIDE_LEVEL_QUERY_URL = "https://api.tidesandcurrents.noaa.gov/api/prod/datagetter?date=latest&station=9412110&product=water_level&datum=MLLW&time_zone=lst&units=metric&format=json"

# TIDE_LEVEL_QUERY_TYPE   = "T"
# SAMPLE_MESSAGE_TYPE     = "S"
# STOP_PUMP_MESSAGE_TYPE  = "F"
# START_PUMP_MESSAGE_TYPE = "P"
# EPOCH_TIME_QUERY_TYPE   = "C"

# COMMS_REPORT_MOTOR_ERR                     = "EM" 
# COMMS_REPORT_TUBE_ERR                      = "ET"  
# COMMS_REPORT_SAMPLE_WATER_NOT_DETECTED_ERR = "EW"  
# COMMS_REPORT_ESTOP_PRESSED                 = "EE"



# CLI_DEBUG_MODE = False  # Set to False for real serial communication, True if you want to simulate usage on the command line

# class DebugSerial:
#     """Simulates a serial.Serial-like interface for debugging."""
#     def __init__(self):
#         self.buffer = []
#         self.is_open = True

#     def write(self, data):
#         print(f"[DEBUG SENT]: {data.decode().strip()}")

#     def readline(self):
#         user_input = input("[DEBUG RECEIVE] > ")
#         return user_input.encode()

#     def close(self):
#         print("[DEBUG] Serial connection closed.")
#         self.is_open = False

#     @property
#     def in_waiting(self):
#         return True  # Always ready to simulate input


# def detect_serial_port():
#     """Detects the appropriate serial port based on OS."""
#     if CLI_DEBUG_MODE:
#         return None  # Not used in debug mode

#     system = platform.system()
#     if system == "Windows":
#         return "COM3"
#     elif system == "Linux":
#         for pattern in ["/dev/tty0", "/dev/ttyACM*"]:
#             ports = glob.glob(pattern)
#             if ports:
#                 return ports[0]
#     elif system == "Darwin":
#         for pattern in ["/dev/tty.usb*", "/dev/cu.usb*"]:
#             ports = glob.glob(pattern)
#             if ports:
#                 return ports[0]
#     raise RuntimeError("No valid serial port found for your OS.")


# def reportErr(err_type):
#     email_subject = ""
#     email_body    = ""
#     if (err_type == COMMS_REPORT_ESTOP_PRESSED):
#         email_body = "ESTOP ERR"
#         email_subject = "ESTOP_ERR"
#     elif (err_type == COMMS_REPORT_MOTOR_ERR):
#         email_body = "MOTOR ERR"
#         email_subject = "MOTOR_ERR"
#     elif (err_type == COMMS_REPORT_SAMPLE_WATER_NOT_DETECTED_ERR):
#         email_body = "SAMPLE WATER NOT DETECTED ERR"
#         email_subject = "ESTOP_ERR"
#     elif (err_type == COMMS_REPORT_TUBE_ERR):
#         email_body = "TUBE TIMEOUT ERR"
#         email_subject = "TUBE TIMEOUT ERR"
#     else:
#         email_body = "UNKNOWN ERR"
#         email_subject = "UNKNOWN ERR"
    
#     email_errs.send_email(email_subject, email_body)

# def get_pacific_unix_epoch():
#     """
#     Get a Unix timestamp that, when interpreted as UTC by a device with no timezone awareness,
#     will display the equivalent Pacific Time.
    
#     For a device that interprets the timestamp as UTC but displays it as local time,
#     we need to ADD the UTC-to-Pacific offset to the current UTC timestamp.
    
#     Returns:
#         int: Unix timestamp adjusted to show Pacific Time when interpreted as UTC
#     """
#     # Get current UTC time
#     utc_now = datetime.now(pytz.UTC)
    
#     # Get current Pacific time
#     pacific_tz = pytz.timezone('America/Los_Angeles')
#     pacific_now = utc_now.astimezone(pacific_tz)
    
#     # Calculate the offset in seconds
#     offset_seconds = pacific_now.utcoffset().total_seconds()
    
#     # Get standard UTC timestamp
#     utc_timestamp = int(time.time())
    
#     # ADD the offset to the UTC timestamp
#     # Since Pacific is behind UTC (negative offset), we're subtracting the absolute value
#     adjusted_timestamp = utc_timestamp + int(offset_seconds)
    
#     return adjusted_timestamp


# def sigint_handler(signum, frame):
#     if ser and ser.is_open:
#         ser.close()
#         print("Serial connection closed.")
#     sys.exit(0)

# signal.signal(signal.SIGINT, sigint_handler)

# def setup():
#     if CLI_DEBUG_MODE:
#         print("[DEBUG] Using mock serial interface.")
#         return DebugSerial()

#     import serial
#     port = detect_serial_port()
#     try:
#         ser = serial.Serial(port, 115200, timeout=1)
#         time.sleep(2)
#         print(f"Connected to {port}")
#         return ser
#     except serial.SerialException as e:
#         print(f"Serial error: {e}")
#         return None

# def queryForWaterLevel():
#     try:
#         response = requests.get(NOAA_TIDE_LEVEL_QUERY_URL)
#         if response.status_code == 200:
#             data = response.json()
#             return data['data'][0]['v']
#         else:
#             return -1000
#     except requests.exceptions.RequestException:
#         return -1000

# def wait_for_file_response(expected_prefix, expected_content, min_length):
#     with open(READ_FILE, "r") as input:
#         while True:
#             input.seek(0)
#             recv = input.read()
#             if len(recv) >= min_length:
#                 if recv[0] == expected_prefix and recv[1:1+len(expected_content)].lower() == expected_content:
#                     return True
#             time.sleep(0.1)

# def write_command(command):
#     with open(WRITE_FILE, "w") as output:
#         output.write(command)
#         output.flush()

# def communicate(ser, sample_time_sec):
#     try:
#         now = datetime.now()
#         curr_time = now.strftime("%y%m%d_%H%M%S")
#         directory = os.path.join(DIRECTORY_PATH, curr_time)
#         os.makedirs(directory, exist_ok=True)

#         write_command(f"SaveToDirectory({directory})")
#         wait_for_file_response("0", "savetodirectory", 16)

#         write_command("StartPump()")
#         wait_for_file_response("0", "startpump", 10)

#         print(f"Starting {sample_time_sec // 60} minute {sample_time_sec % 60} second timer")

#         write_command("StartSampleCollection(1)")
#         wait_for_file_response("0", "startsamplecollection", 22)

#         print(f"Collecting temperatures for {sample_time_sec} seconds")
#         with open(TEMP_CSV, "a", newline="\n") as csv_file:
#             writer = csv.writer(csv_file)
#             if os.stat(TEMP_CSV).st_size == 0:
#                 writer.writerow(["Timestamp", "Min Temp(C)", "Max Temp(C)", "Avg Temp(C)"])

#             temperatures = []
#             num_samples = (sample_time_sec // 15) - 1
#             print(num_samples)
#             for _ in range(num_samples):
#                 time.sleep(15)
#                 ser.write("T\n".encode())
#                 print("sending temp request!")
#                 now = datetime.now()
#                 curr_time = now.strftime("%y-%m-%d_%H:%M:%S")
#                 tmp = 0
#                 while tmp < 10:
#                     print("in true loop!")
#                     if ser.in_waiting:
#                         print("got temp data!")
#                         temp_data = ser.readline().decode().strip()
#                         if temp_data.isdigit():
#                             print(f"{curr_time} - Temperature: {temp_data}°C")
#                             temperatures.append(int(temp_data))
#                             break
#                     tmp += 1
#                     time.sleep(0.5)

#             min_temp = min(temperatures)
#             max_temp = max(temperatures)
#             average_temp = sum(temperatures) / len(temperatures)
#             print(f"Min: {min_temp}, Max: {max_temp}, Avg: {average_temp:.2f}")
#             writer.writerow([curr_time, min_temp, max_temp, average_temp])

#         write_command("StopSampleCollection()")
#         wait_for_file_response("0", "stopsamplecollection", 21)

#         ser.write("D\n".encode())

#     except Exception as e:
#         print(f"Error during communication: {e}")
#         if ser and ser.is_open:
#             ser.close()

# def controlPump(ser, command_name, expected_ack):
#     try:
#         write_command(command_name)
#         wait_for_file_response("0", expected_ack, len(expected_ack))
#         ser.write("D\n".encode())
#     except Exception as e:
#         print(f"Error with pump command '{command_name}': {e}")
#         if ser and ser.is_open:
#             ser.close()

# def stopPump(ser):
#     controlPump(ser, "StopPump()", "stoppump")

# def startPump(ser):
#     controlPump(ser, "StartPump()", "startpump")

# def sendEpochTime(ser):
#     try:
#         epoch_time = get_pacific_unix_epoch()
#         print(f"Sending epoch time: {epoch_time}")
#         ser.write((str(epoch_time) + "\n").encode())
#         return epoch_time
#     except Exception as e:
#         print(f"Error sending epoch time: {e}")
#         return None

# if __name__ == "__main__":
#     ser = setup()

#     terminal = TerminalInterface()
#     terminal.start()
    

#     while ser is None:
#         time.sleep(1)
#         print("Unable to set up serial connection. Retrying...")
#         ser = setup()
    
#     print("[ASRS TERMINAL] > ", end="", flush=True)
    
#     while True:
#         if ser is None or not ser.is_open:
#             print("Serial disconnected. Reconnecting...")
#             ser = setup()
#             continue
        
#         cmd = terminal.get_command()
#         if cmd:
#             handleTerminalInput(cmd)
#             print("[ASRS TERMINAL] > ", end="", flush=True)

#         write_to = ser.readline().decode().strip() if ser.in_waiting else None
        
#         if not write_to:
#             continue
        
#         #print("GOT: " + write_to)

#         if write_to == TIDE_LEVEL_QUERY_TYPE:
#             tide_level = queryForWaterLevel()
#             print(f"Tide level is {tide_level}")
#             ser.write((str(tide_level) + "\n").encode())

#         elif write_to == SAMPLE_MESSAGE_TYPE:
#             time_line = None
#             start = time.time()
#             while time.time() - start < 2:
#                 if ser.in_waiting:
#                     time_line = ser.readline().decode().strip()
#                     break
#                 time.sleep(0.1)

#             if time_line and time_line.isdigit():
#                 sample_time_sec = int(time_line)
#                 print(f"Received sample time: {sample_time_sec} seconds")
#                 communicate(ser, sample_time_sec)
#             else:
#                 print("Warning: Expected sample time value after 'S' but none received.")

#         elif write_to == STOP_PUMP_MESSAGE_TYPE:
#             stopPump(ser)

#         elif write_to == START_PUMP_MESSAGE_TYPE:
#             startPump(ser)

#         elif write_to == EPOCH_TIME_QUERY_TYPE:
#             sendEpochTime(ser)
#         elif (len(write_to) == 2 and write_to[0] == 'E'):
#             reportErr(write_to)
#         else:
#            print(f"ERR: RECEIVED UNKNOWN COMMAND {write_to}")

import threading
import queue
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
import calendar
import pytz
import re
from terminalThread import *
import email_errs

# isSampling = True
# hours = 8
# minutes = 0

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

COMMS_REPORT_MOTOR_ERR                     = "EM" 
COMMS_REPORT_TUBE_ERR                      = "ET"  
COMMS_REPORT_SAMPLE_WATER_NOT_DETECTED_ERR = "EW"  
COMMS_REPORT_ESTOP_PRESSED                 = "EE"
AQUSENS_NACK_RECEIVED                      = "EA"
AQUSENS_ACK_TIMEOUT                        = "EF"

AQUSENS_ACK_TIMEOUT_SEC                    = 10

CLI_DEBUG_MODE = False

class DebugSerial:
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
        return True

class TerminalInterface:
    def __init__(self):
        self.input_queue = queue.Queue()
        self.output_queue = queue.Queue()
        self.thread = threading.Thread(target=self._terminal_input_loop, daemon=True)
        self.running = False

    def start(self):
        """Starts the terminal input thread."""
        self.running = True
        self.thread.start()

    def _terminal_input_loop(self):
        """Reads user input and puts the full command as a list of strings into the input queue."""
        while self.running:
            try:
                line = input("").strip()
                if not line:
                    continue
                tokens = line.split()
                # Instead of splitting into (command, args), put the entire tokens list
                self.input_queue.put(tokens)
            except EOFError:
                break
            except Exception as e:
                self.output_queue.put(f"[ERROR] Terminal input thread encountered: {e}")


    def get_command(self):
        """Returns the next terminal command as a list of strings, or None if queue empty."""
        try:
            return self.input_queue.get_nowait()
        except queue.Empty:
            return None


    def send_output(self, message):
        """Puts a message in the output queue to be printed by the main thread."""
        self.output_queue.put(message)

def handleTerminalInput(ser, terminalCommand):
    global isSampling, hours, minutes

    match terminalCommand[0]:
        case "status":
            safe_serial_write(ser, "Q1\n")
            time.sleep(0.05)
            replyString = safe_serial_readline(ser)
            match = re.match(r"([01])H(\d+)M(\d+)", replyString)
            if match:
                isSampling = match.group(1) == '1'
                hours = int(match.group(2))
                mins = int(match.group(3))

                status_string = "enabled" if isSampling else "disabled"
                hour_str = " hour" if hours == 1 else " hours"
                minute_str = " minute" if mins == 1 else " minutes"
                print(f"NORA has interval sampling {status_string}, sampling every {hours}{hour_str} and {minutes}{minute_str}.\n")

        case "set-interval":
            if len(terminalCommand) != 3:
                print("ERR: Invalid set-interval usage!\n"
                      "  Usage: set-interval <hours> <minutes>\n")
            else:
                hours = (terminalCommand[1])
                minutes = (terminalCommand[2])
                sendString = "Q1H" + hours + "M" + minutes + "\n"
                safe_serial_write(ser, sendString)
                time.sleep(0.05)
                reply = safe_serial_readline(ser)
                if (reply[0] == 'S' and reply[1] == '0'):
                    print("WARNING: NORA is not currently interval sampling!\n")
                elif (reply[0] == '1'):
                    print(f"ERR: Recv err ack -> {reply}, retry command")
                # hour_str = "hour" if hours == 1 else "hours"
                # minute_str = "minute" if minutes == 1 else "minutes"
                
                # print(f"Setting sampling interval to {hours} {hour_str} and {minutes} {minute_str}...")
                # print("Success")
                # if not isSampling:
                #     print("WARNING: ASRS is not currently interval sampling!\n")
                # else:
                #     print("")
        case "start-sampling":
            
            print("Starting interval sampling...")
            safe_serial_write(ser, "Q2\n")
            time.sleep(0.05)
            reply = safe_serial_readline(ser)
            ack = (reply == "0")

            if ack:
                print("Success\n")
            
            else:
                print(f"ERR: Recv err ack -> {ack}")


        case "stop-sampling":
            print("Stopping interval sampling...")
            safe_serial_write(ser, "Q3\n")
            time.sleep(0.05)
            reply = safe_serial_readline
            ack = (reply == "0")

            if ack:
                print("Success\n")
            else:
                print(f"ERR: Recv unknown ack -> {ack}")
        case "run-sample":
            safe_serial_write(ser, "Q4\n")
            time.sleep(0.05)
            reply = safe_serial_readline(ser)
            if (reply == "0"):
                print("Sample event started successfully!")
            elif (reply == "1"):
                print("ERR: NORA is not currently in standby mode, cannot start sample!")

        case "read-temps":
            safe_serial_write(ser, "Q5\n")
            time.sleep(0.1)
            reply = safe_serial_readline(ser)
            ack = (len(reply) > 0 and reply == '0')

            if ack:
                match = re.match(r"0R1([0-9.]+)R2([0-9.]+)R3([0-9.]+)", reply)
                if match:
                    rtd1_sample = float(match.group(1))
                    rtd2_flushwater = float(match.group(2)) #beep boop
                    rtd3_airtemp = float(match.group(3))

                    print("RTD 1 (Sampler Tube):            ", rtd1_sample, "C")
                    print("RTD 2 (Flushwater):              ", rtd2_flushwater, "C")
                    print("RTD 3 (NORA Internal Air Temp):  ", rtd3_airtemp, "C")
            else:
                print(f"ERR: Recv unknown reply -> {reply}")
            
        case "help":
            print("Known commands:\n"
                "  status                              — View the current status of NORA\n" #Q0, recv 1HxxMxx for sampling, or 0HxxMxx for not sampling
                "  set-interval <hours> <minutes>      — Set the sampling interval (hours and minutes)\n" #Q1HxxMxx
                "  start-sampling                      — Enable interval sampling\n" #Q2
                "  stop-sampling                       — Disable interval sampling\n" #Q3
                "  run-sample                          — Start a sample manually\n" #Q4
                "  read-temps                          — Returns the temperatures of all system RTDs\n" #Q5, recv 0R1XX.XXR2xx.xxR3xx.xx
                "  help                                — See this lovely help message again")
        case _:
            print(
                f"ERR: UNKNOWN COMMAND {terminalCommand[0]}\n"
                  "Type \"help\" to view all commands\n"
            )

def detect_serial_port():
    if CLI_DEBUG_MODE:
        return None

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

def reportErr(err_type, last_aqusens_command=None):
    email_subject = ""
    email_body    = ""
    if (err_type == COMMS_REPORT_ESTOP_PRESSED):
        email_body = "ESTOP ERR"
        email_subject = "ESTOP_ERR"
    elif (err_type == COMMS_REPORT_MOTOR_ERR):
        email_body = "MOTOR ERR"
        email_subject = "MOTOR_ERR"
    elif (err_type == COMMS_REPORT_SAMPLE_WATER_NOT_DETECTED_ERR):
        email_body = "SAMPLE WATER NOT DETECTED ERR"
        email_subject = "ESTOP_ERR"
    elif (err_type == COMMS_REPORT_TUBE_ERR):
        email_body = "TUBE TIMEOUT ERR"
        email_subject = "TUBE TIMEOUT ERR"
    elif (err_type == AQUSENS_NACK_RECEIVED):
        email_body = "AQUSENS NACK RECEIVED"
        if email_subject is not None:
            email_subject = f"AQUSENS NACK RECEIVED: {last_aqusens_command}"
        else:
            email_subject = "AQUSENS NACK RECEIVED FOR UNKNOWN COMMAND" #Should never trigger but for safety
    elif (err_type == AQUSENS_ACK_TIMEOUT):
        email_body = "AQUSENS ACK TIMEOUT"
        email_subject = "AQUSENS ACK TIMEOUT"
    else:
        email_body = "UNKNOWN ERR"
        email_subject = "UNKNOWN ERR"

    email_errs.send_email(email_subject, email_body)

def get_pacific_unix_epoch():
    utc_now = datetime.now(pytz.UTC)
    pacific_tz = pytz.timezone('America/Los_Angeles')
    pacific_now = utc_now.astimezone(pacific_tz)
    offset_seconds = pacific_now.utcoffset().total_seconds()
    utc_timestamp = int(time.time())
    adjusted_timestamp = utc_timestamp + int(offset_seconds)
    return adjusted_timestamp

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

    port = detect_serial_port()
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)
        print(f"Connected to {port}")
        return ser
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        return None

def safe_serial_write(ser, message):
    try:
        ser.write(message.encode())
    except serial.SerialException as e:
        print(f"[ERROR] Failed to write to serial: {e}")
        if ser and ser.is_open:
            ser.close()
        raise

def safe_serial_readline(ser):
    try:
        return ser.readline().decode().strip()
    except serial.SerialException as e:
        print(f"[ERROR] Failed to read from serial: {e}")
        if ser and ser.is_open:
            ser.close()
        raise

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
    start_time = time.time()  # Record the start time
    
    with open(READ_FILE, "r") as input:
        while True:
            input.seek(0)
            recv = input.read()
            if len(recv) >= min_length:
                if recv[0] == expected_prefix and recv[1:1+len(expected_content)].lower() == expected_content:
                    return True
                else:
                    reportErr(AQUSENS_NACK_RECEIVED, recv)
                    return False

            # Check if timeout has been exceeded
            if time.time() - start_time > AQUSENS_ACK_TIMEOUT_SEC:
                reportErr(AQUSENS_ACK_TIMEOUT)
                return False  # Timeout has occurred

            time.sleep(0.1)

def write_command(command):
    with open(WRITE_FILE, "w") as output:
        output.write(command)
        output.flush()

def communicate(ser, sample_time_sec):
    try:
        stopPump(ser)

        now = datetime.now()
        curr_time = now.strftime("%y%m%d_%H%M%S")
        directory = os.path.join(DIRECTORY_PATH, curr_time)
        os.makedirs(directory, exist_ok=True)

        write_command(f"SaveToDirectory({directory})")
        wait_for_file_response("0", "savetodirectory", 16)

        startPump(ser)

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
            for _ in range(num_samples):
                time.sleep(15)
                safe_serial_write(ser, "T\n")
                now = datetime.now()
                curr_time = now.strftime("%y-%m-%d_%H:%M:%S")
                for _ in range(10):
                    if ser.in_waiting:
                        temp_data = safe_serial_readline(ser)
                        if temp_data.isdigit():
                            temperatures.append(int(temp_data))
                            break
                    time.sleep(0.5)

            min_temp = min(temperatures)
            max_temp = max(temperatures)
            average_temp = sum(temperatures) / len(temperatures)
            writer.writerow([curr_time, min_temp, max_temp, average_temp])

        write_command("StopSampleCollection()")
        wait_for_file_response("0", "stopsamplecollection", 21)

        safe_serial_write(ser, "D\n")

    except Exception as e:
        print(f"Error during communication: {e}")
        if ser and ser.is_open:
            ser.close()

def controlPump(ser, command_name, expected_ack):
    try:
        write_command(command_name)
        wait_for_file_response("0", expected_ack, len(expected_ack))
        safe_serial_write(ser, "D\n")
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
        epoch_time = get_pacific_unix_epoch()
        print(f"Sending epoch time: {epoch_time}")
        safe_serial_write(ser, str(epoch_time) + "\n")
        return epoch_time
    except Exception as e:
        print(f"Error sending epoch time: {e}")
        return None

if __name__ == "__main__":
    ser = setup()
    terminal = TerminalInterface()
    terminal.start()

    while ser is None:
        time.sleep(1)
        print("Unable to set up serial connection. Retrying...")
        ser = setup()

    print("[NORA TERMINAL] > ", end="", flush=True)

    while True:
        if ser is None or not ser.is_open:
            print("Serial disconnected. Reconnecting...")
            ser = setup()
            continue

        cmd = terminal.get_command()
        if cmd:
            handleTerminalInput(ser, cmd)
            print("[NORA TERMINAL] > ", end="", flush=True)

        try:
            write_to = safe_serial_readline(ser) if ser.in_waiting else None
        except:
            ser = None
            continue

        if not write_to:
            continue

        if write_to == TIDE_LEVEL_QUERY_TYPE:
            tide_level = queryForWaterLevel()
            safe_serial_write(ser, "W" + str(tide_level) + "\n")

        elif write_to == SAMPLE_MESSAGE_TYPE:
            time_line = None
            start = time.time()
            while time.time() - start < 2:
                try:
                    if ser.in_waiting:
                        time_line = safe_serial_readline(ser)
                        break
                except:
                    ser = None
                    break
                time.sleep(0.1)

            if time_line and time_line.isdigit():
                sample_time_sec = int(time_line)
                communicate(ser, sample_time_sec)
            else:
                print("Warning: Expected sample time value after 'S' but none received.")

        elif write_to == STOP_PUMP_MESSAGE_TYPE:
            stopPump(ser)

        elif write_to == START_PUMP_MESSAGE_TYPE:
            startPump(ser)

        elif write_to == EPOCH_TIME_QUERY_TYPE:
            sendEpochTime(ser)

        elif (len(write_to) == 2 and write_to[0] == 'E'):
            reportErr(write_to)

        else:
            print(f"ERR: RECEIVED UNKNOWN COMMAND {write_to}")
