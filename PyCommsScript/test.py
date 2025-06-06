import time
from datetime import datetime
import pytz

def get_pacific_unix_epoch():
    """
    Get a Unix timestamp that, when directly interpreted by a device with no timezone awareness,
    will display the current Pacific Time.
    
    Returns:
        int: Unix timestamp adjusted to show Pacific Time
    """
    # Get current time in Pacific timezone
    pacific_tz = pytz.timezone('America/Los_Angeles')
    now_pacific = datetime.now(pacific_tz)
    
    # Remove timezone info to get a naive datetime object
    naive_pacific = now_pacific.replace(tzinfo=None)
    
    # Calculate seconds since epoch for this naive datetime
    # This is the timestamp that, when interpreted directly without timezone knowledge,
    # will show the current Pacific time
    pacific_epoch = int(naive_pacific.timestamp())
    
    return pacific_epoch

# Example usage
if __name__ == "__main__":
    # Standard UTC epoch
    standard_epoch = int(time.time())
    
    # Our adjusted epoch for Pacific time
    pacific_epoch = get_pacific_unix_epoch()
    
    print(f"Standard Unix epoch (UTC): {standard_epoch}")
    print(f"Adjusted epoch for Pacific: {pacific_epoch}")
    print(f"Difference: {standard_epoch - pacific_epoch} seconds")
    
    # VERIFICATION: Show how these would be interpreted directly
    # This is what a microcontroller would show
    standard_display = datetime.fromtimestamp(standard_epoch, tz=None)
    pacific_display = datetime.fromtimestamp(pacific_epoch, tz=None)
    
    # Also get the actual current Pacific time for comparison
    pacific_tz = pytz.timezone('America/Los_Angeles')
    actual_pacific = datetime.now(pacific_tz).replace(tzinfo=None)
    
    print(f"\nStandard epoch displays as: {standard_display} (local time of machine)")
    print(f"Adjusted epoch displays as: {pacific_display} (should be Pacific time)")
    print(f"Actual Pacific time now: {actual_pacific}")
