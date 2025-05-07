import time
from datetime import datetime
import pytz

def get_pacific_unix_epoch():
    """
    Get a modified Unix epoch time that will display the current Pacific Time
    when interpreted directly without timezone conversion.
    
    This function returns the Unix timestamp that is offset from UTC by the
    Pacific Time zone difference (including DST adjustments).
    
    Returns:
        int: Modified Unix epoch that will show as Pacific Time when
             converted directly to a datetime without timezone handling
    """
    # Get current UTC time
    utc_now = datetime.now(pytz.UTC)
    
    # Get the Pacific timezone offset in seconds (including DST)
    pacific_tz = pytz.timezone('America/Los_Angeles')
    pacific_now = utc_now.astimezone(pacific_tz)
    offset_seconds = pacific_now.utcoffset().total_seconds()
    
    # Get the standard Unix epoch time (UTC-based)
    utc_epoch = int(time.time())
    
    # Subtract the offset to get a timestamp that will display as Pacific Time
    # when interpreted directly
    pacific_epoch = utc_epoch - int(offset_seconds)
    
    return pacific_epoch

# Example usage
if __name__ == "__main__":
    # Standard UTC epoch
    utc_epoch = int(time.time())
    
    # Modified epoch that will display as Pacific time
    pacific_epoch = get_pacific_unix_epoch()
    
    # Demonstrate the difference
    print(f"Standard UTC epoch: {utc_epoch}")
    print(f"Modified epoch for Pacific: {pacific_epoch}")
    
    # Calculate the timezone offset
    pacific_tz = pytz.timezone('America/Los_Angeles')
    offset_hours = pacific_tz.utcoffset(datetime.now()).total_seconds() / 3600
    
    # Show what happens when we interpret directly (no timezone)
    print(f"\nTimezone offset: {offset_hours:.1f} hours from UTC")
    print(f"Difference in seconds: {utc_epoch - pacific_epoch}")
    
    # Convert both to show the actual time difference
    utc_display = datetime.fromtimestamp(utc_epoch)
    pacific_display = datetime.fromtimestamp(pacific_epoch)
    
    print(f"\nStandard epoch displays as: {utc_display} (local time of machine)")
    print(f"Modified epoch displays as: {pacific_display} (should be Pacific time)")
    
    # For verification, show the actual current Pacific time
    current_pacific = datetime.now(pacific_tz).replace(tzinfo=None)
    print(f"Actual Pacific time now: {current_pacific}")