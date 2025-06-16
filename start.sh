#!/bin/bash

# --- Check for Root Privileges ---
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root (with sudo)." 1>&2
   exit 1
fi

echo "--- Starting Smart Ventilation System ---"

# --- 1. Start pigpio Daemon ---
echo "[1/4] Starting pigpio daemon..."
# For a clean start, kill any existing instance first
killall pigpiod
sleep 1
pigpiod
if [ $? -ne 0 ]; then
    echo "Error: Failed to start pigpio daemon."
    exit 1
fi
echo "pigpio daemon started successfully."


# --- 2. Load FPGA Kernel Modules ---
MODULE_PATH="./drivers" 
echo "[2/4] Loading FPGA kernel modules..."
insmod ${MODULE_PATH}/fpga_interface_driver.ko
insmod ${MODULE_PATH}/fpga_buzzer_driver.ko
insmod ${MODULE_PATH}/fpga_text_lcd_driver.ko
# Verify that the modules are loaded
echo "Verifying loaded modules:"
lsmod | grep "fpga"
echo "Kernel module loading process finished."


# --- 3. Compile the C Application ---
echo "[3/4] Compiling the C application..."
make clean
make
if [ $? -ne 0 ]; then
    echo "Error: Compilation failed."
    exit 1
fi
echo "Compilation successful."


# --- 4. Run the Compiled Application ---
echo "[4/4] Running the Smart Ventilation System GUI application..."
sodo ./smart_ventilation

echo "--- Script finished ---"