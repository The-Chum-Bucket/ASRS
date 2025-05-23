import threading
import queue
import time

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

def handleTerminalInput(terminalCommand):
    match terminalCommand[0]:
        case "set-interval":
            if len(terminalCommand) != 3:
                print("ERR: Invalid set-interval usage!\n"
                      "  Usage: set-interval <hours> <minutes>\n")
            else:
                hours = terminalCommand[1]
                minutes = terminalCommand[2]
                
                hour_str = "hour" if hours == 1 else "hours"
                minute_str = "minute" if minutes == 1 else "minutes"
                
                print(f"Setting sampling interval to {hours} {hour_str} and {minutes} {minute_str}...")
                print("Success")

        case "stop-sampling":
            print("Stopping interval sampling...")
            print("Success")
        case "start-sampling":
            print("Starting interval sampling...")
            print("Success")
        case "help":
            print("Valid commands:\n"
                "  set-interval <hours> <minutes>      — Set the update interval (hours and minutes)\n"
                "  start-sampling                      — Stop sampling process\n"
                "  stop-sampling                       — Start sampling process")
        case _:
            print(
                f"ERR: UNKNOWN COMMAND {terminalCommand[0]}\n"
                  "Type \"help\" to view all commands"
            )