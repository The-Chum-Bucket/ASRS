# import threading
# import queue
# import time

# isSampling = True
# hours = 8
# minutes = 0

# class TerminalInterface:
#     def __init__(self):
#         self.input_queue = queue.Queue()
#         self.output_queue = queue.Queue()
#         self.thread = threading.Thread(target=self._terminal_input_loop, daemon=True)
#         self.running = False

#     def start(self):
#         """Starts the terminal input thread."""
#         self.running = True
#         self.thread.start()

#     def _terminal_input_loop(self):
#         """Reads user input and puts the full command as a list of strings into the input queue."""
#         while self.running:
#             try:
#                 line = input("").strip()
#                 if not line:
#                     continue
#                 tokens = line.split()
#                 # Instead of splitting into (command, args), put the entire tokens list
#                 self.input_queue.put(tokens)
#             except EOFError:
#                 break
#             except Exception as e:
#                 self.output_queue.put(f"[ERROR] Terminal input thread encountered: {e}")


#     def get_command(self):
#         """Returns the next terminal command as a list of strings, or None if queue empty."""
#         try:
#             return self.input_queue.get_nowait()
#         except queue.Empty:
#             return None


#     def send_output(self, message):
#         """Puts a message in the output queue to be printed by the main thread."""
#         self.output_queue.put(message)

# def handleTerminalInput(ser, terminalCommand):
#     global isSampling, hours, minutes

#     match terminalCommand[0]:
#         case "set-interval":
#             if len(terminalCommand) != 3:
#                 print("ERR: Invalid set-interval usage!\n"
#                       "  Usage: set-interval <hours> <minutes>\n")
#             else:
#                 hours = int(terminalCommand[1])
#                 minutes = int(terminalCommand[2])
                
#                 hour_str = "hour" if hours == 1 else "hours"
#                 minute_str = "minute" if minutes == 1 else "minutes"
                
#                 print(f"Setting sampling interval to {hours} {hour_str} and {minutes} {minute_str}...")
#                 print("Success")
#                 if not isSampling:
#                     print("WARNING: ASRS is not currently interval sampling!\n")
#                 else:
#                     print("")

#         case "stop-sampling":
#             if isSampling:
#                 print("Stopping interval sampling...")
#                 isSampling = False
#                 print("Success\n")
#             else:
#                 print("ASRS already has interval sampling enabled.\n")
#         case "start-sampling":
#             if not isSampling:
#                 print("Starting interval sampling...")
#                 isSampling = True
#                 print("Success\n")
#             else:
#                 print("ASRS already has interval sampling disabled.\n")
#         case "status":
#             status_string = "enabled" if isSampling else "disabled"
#             hour_str = " hour" if hours == 1 else " hours"
#             minute_str = " minute" if minutes == 1 else " minutes"
#             print(f"ASRS has interval sampling {status_string}, sampling every {hours}{hour_str} and {minutes}{minute_str}.\n")
            
#         case "help":
#             print("Known commands:\n"
#                 "  status                              — View the current status of the ASRS\n" #Q0, recv 1HxxMxx for sampling, or 0HxxMxx for not sampling
#                 "  set-interval <hours> <minutes>      — Set the sampling interval (hours and minutes)\n" #Q1HxxMxx
#                 "  start-sampling                      — Enable interval sampling\n" #Q2
#                 "  stop-sampling                       — Disable interval sampling\n" #Q3
#                 "  run-sample                          — Start a sample manually" #Q4
#                 "  read-temps                          — Returns the temperatures of all system RTDs") #Q5, recv 0R1XX.XXR2xx.xxR3xx.xx
#         case _:
#             print(
#                 f"ERR: UNKNOWN COMMAND {terminalCommand[0]}\n"
#                   "Type \"help\" to view all commands\n"
#             )