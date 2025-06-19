#!/bin/bash

# --- 1. Compile the C Application ---
echo "Compiling the C application..."
make clean
make
if [ $? -ne 0 ]; then
    echo "Error: Compilation failed."
    exit 1
fi
echo "Compilation successful."

echo "--- Script finished ---"