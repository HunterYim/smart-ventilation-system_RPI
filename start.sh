#!/bin/bash

# --- Check for Root Privileges ---
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root (with sudo)." 1>&2
   exit 1
fi

echo "--- Starting Smart Ventilation System ---"

# --- 1. Start pigpio Daemon ---
echo "[1/3] Starting pigpio daemon..."
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
echo "[2/3] Loading FPGA kernel modules..."
sudo insmod ${MODULE_PATH}/fpga_interface_driver.ko
sudo insmod ${MODULE_PATH}/fpga_buzzer_driver.ko
sudo mknod /dev/fpga_buzzer c 264 0
sudo insmod ${MODULE_PATH}/fpga_text_lcd_driver.ko
sudo mknod /dev/fpga_text_lcd c 263 0
# Verify that the modules are loaded
echo "Verifying loaded modules:"
lsmod | grep "fpga"
echo "Kernel module loading process finished."

# --- 3. Run the Compiled Application ---
echo "[3/3] Running the Smart Ventilation System GUI application..."
sudo ./smart_ventilation