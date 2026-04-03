"""Send CPU load + RAM usage to ESP32 via USB Serial."""

import time
import sys
import glob
import os
import subprocess

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

def find_port():
    """Smart port detection for Windows."""
    if os.name == 'nt':
        # Пробуем найти через WMI
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
        
        # Перебор портов
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
                    cpu = psutil.cpu_percent(interval=0.5)
                    ram = psutil.virtual_memory().percent
                    line = f"CPU:{int(cpu)}|RAM:{int(ram)}\n"
                    ser.write(line.encode())
                    print(f"Sent: CPU {int(cpu)}% | RAM {int(ram)}%", end='\r')
                    time.sleep(UPDATE_INTERVAL)
        except (serial.SerialException, OSError):
            print("\n[!] Disconnected! Waiting for reconnection...")
            time.sleep(2)
        except KeyboardInterrupt:
            print("\nStopped.")
            break

if __name__ == "__main__":
    main()
