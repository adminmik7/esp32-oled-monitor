"""Send GPU load + RAM usage to ESP32 via USB Serial."""

import time
import sys
import glob
import os
import subprocess
import re

BAUD = 115200
UPDATE_INTERVAL = 1

try:
    import psutil
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "psutil", "--quiet"])
    import psutil

try:
    import serial
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pyserial", "--quiet"])
    import serial

def get_gpu_load():
    """Get GPU load from nvidia-smi."""
    try:
        result = subprocess.run(
            ["nvidia-smi", "--query-gpu=utilization.gpu", "--format=csv,noheader,nounits"],
            capture_output=True, text=True, timeout=5
        )
        if result.returncode == 0 and result.stdout.strip():
            lines = [l.strip() for l in result.stdout.strip().splitlines() if l.strip()]
            if lines:
                return int(float(lines[0]))
    except:
        pass
    return 0

def find_port():
    """Smart port detection for Windows."""
    if os.name == 'nt':
        # Пробуем найти через WMI (самый надежный способ для ESP)
        try:
            import wmi
            c = wmi.WMI()
            for item in c.Win32_PnPEntity():
                if item.Name and ("USB-SERIAL" in item.Name.upper() or "ESP" in item.Name.upper() or "CP210" in item.Name.upper()):
                    import re
                    match = re.search(r"\(COM(\d+)\)", item.Name)
                    if match:
                        return f"COM{match.group(1)}"
        except: 
            pass
        
        # Если WMI не сработал, перебираем порты
        for i in range(1, 20):
            port = f"COM{i}"
            try:
                with serial.Serial(port, BAUD, timeout=0.1) as s:
                    return port
            except: 
                continue
    else:
        # Для Linux/Mac
        for dev in sorted(glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")):
            if os.path.exists(dev): 
                return dev
    return None

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else find_port()
    if not port:
        print("Error: No USB-Serial port found.")
        input("Press Enter to exit...")
        sys.exit(1)

    print(f"Looking for ESP32 on {port}...")

    while True:
        try:
            with serial.Serial(port, BAUD, timeout=1) as ser:
                time.sleep(2) # Ждем перезагрузки ESP
                ser.reset_input_buffer()
                print(f"Connected on {port} @ {BAUD} baud")
                
                while True:
                    gpu = get_gpu_load()
                    ram = psutil.virtual_memory().percent
                    line = f"GPU:{int(gpu)}|RAM:{int(ram)}\n"
                    ser.write(line.encode())
                    
                    # Если хочешь видеть ответ от ESP, раскомментируй строки ниже:
                    # if ser.in_waiting:
                    #     print(f" {line.strip()} -> {ser.readline().decode().strip()}")
                    
                    time.sleep(UPDATE_INTERVAL)
        except (serial.SerialException, OSError):
            print("\n[!] Disconnected! Waiting for reconnection...")
            time.sleep(2)
        except KeyboardInterrupt:
            print("\nStopped.")
            break

if __name__ == "__main__":
    main()
