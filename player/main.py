import serial
from serial.tools import list_ports
import pygame
import time

# -----------------------
# Configuration
# -----------------------
BAUD_RATE = 115200
TRIGGER = "Opening"      # Message from Arduino
SOUND_FILE = "sound.mp3" # or sound.wav
# -----------------------

# List serial ports
ports = list(list_ports.comports())

if not ports:
    print("No serial ports found.")
    exit()

print("Available serial ports:")
for i, port in enumerate(ports):
    print(f"[{i}] {port.device} - {port.description}")

# User selects a port
while True:
    try:
        index = int(input("\nSelect port number: "))
        if 0 <= index < len(ports):
            break
    except ValueError:
        pass
    print("Invalid selection.")

port_name = ports[index].device

print(f"\nConnecting to {port_name}...\n")

ser = serial.Serial(port_name, BAUD_RATE, timeout=1)

pygame.mixer.init()
sound = pygame.mixer.Sound(SOUND_FILE)

print("Listening...\n")

try:
    while True:
        if ser.in_waiting:
            line = ser.readline().decode("utf-8", errors="ignore").strip()

            if line:
                print(line)

                if line == TRIGGER:
                    # Stop previous playback (if any) and play once
                    sound.stop()
                    sound.play()

        time.sleep(0.01)

except KeyboardInterrupt:
    print("\nExiting...")

finally:
    ser.close()
    pygame.quit()