#!/usr/bin/env python3
"""Send CPU load + RAM usage to ESP32 (esp32_oled.ino) via USB Serial."""

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
        try:
            import wmi
            c = wmi.WMI()
            for item in c.Win32_PnPEntity():
                if item.Name and ("USB-SERIAL" in item.Name.upper() or "ESP" in item.Name.upper() or "CP210" in item.Name.upper()):
                    import re
                    match = re.search(r"\(COM(\d+)\)", item.Name)
                    if match:
                        return f"COM{match.group(1)}"
        except: pass
        
        for i in range(1, 10):
            port = f"COM{i}"
            try:
                with serial.Serial(port, BAUD, timeout=0.5) as s:
                    return port
            except: continue
    else:
        for dev in sorted(glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")):
            if os.path.exists(dev): return dev
    return None

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else find_port()
    if not port:
        print("Error: No USB-Serial port found.")
        sys.exit(1)

    while True:
        try:
            with serial.Serial(port, BAUD, timeout=1) as ser:
                time.sleep(2)
                ser.reset_input_buffer()
                print(f"Connected on {port} @ {BAUD} baud")
                
                while True:
                    cpu = psutil.cpu_percent(interval=0.5)
                    ram = psutil.virtual_memory().percent
                    line = f"CPU:{int(cpu)}|RAM:{int(ram)}\n"
                    ser.write(line.encode())
                    
                    if ser.in_waiting:
                        print(f"  {line.strip()} -> {ser.readline().decode().strip()}")
                    
                    time.sleep(UPDATE_INTERVAL)
        except (serial.SerialException, OSError):
            print("\n[!] Disconnected! Waiting for reconnection...")
            time.sleep(2)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nStopped.")
