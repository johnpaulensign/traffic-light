import subprocess
import time

def is_microphone_in_use():
    """
    Checks if the macOS microphone is in use by inspecting ioreg.
    Returns: True if in use, False otherwise.
    """
    try:
        # Run command to check active audio engine users
        output = subprocess.check_output(
            ["ioreg", "-l", "AppleHDAEngineInput"], 
            stderr=subprocess.DEVNULL
        ).decode("latin-1")
        
        # Look for the active client count
        for line in output.split('\n'):
            if "IOAudioEngineNumActiveUserClients" in line:
                count = int(line.split('=')[-1].strip())
                return count > 0
    except Exception as e:
        print(f"Error checking microphone status: {e}")
        return False
    
    return False

# Example usage
if __name__ == "__main__":
    attempts = 30

    for x in range(0, attempts):

        if is_microphone_in_use():
            print("Microphone is currently IN USE.")
        else:
            print("Microphone is IDLE.")

        time.sleep(1)

