#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // for sleep
#include <pigpio.h>     // pigpio library header

// --- User configurable settings ---
#define DHT_SENSOR_PIN  24  // BCM GPIO pin number to use
#define DHT_SENSOR_TYPE 11  // DHT sensor type (11 or 22)
// ------------------------------

// Must include the DHT sensor related header file provided by pigpio examples.
// For example, if the file name is "DHTXXD.h", include it as below.
// This file must be obtained from the official pigpio examples.
#include "DHTXXD.h" // <<< This file should be in the project directory, or its path specified during compilation.

int main(void) {
    DHTXXD_data_t dht_data; // Data structure defined in DHTXXD.h
    int status;
    int attempts = 5; // Number of attempts to read data

    // 1. Initialize pigpio library
    if (gpioInitialise() < 0) {
        fprintf(stderr, "Failed to initialize pigpio library.\n");
        fprintf(stderr, "Check if pigpiod daemon is running (e.g., sudo pigpiod).\n");
        return 1;
    }

    printf("Starting to read DHT%d sensor data (BCM GPIO %d)...\n", DHT_SENSOR_TYPE, DHT_SENSOR_PIN);
    printf("Waiting a few seconds for sensor to stabilize...\n");
    sleep(2); // Sensor stabilization time (optional)

    while(attempts--) {
        // 2. Read DHT sensor data (using function defined in DHTXXD.h)
        // The DHTXXD function reads data by specifying a GPIO pin and sensor type.
        // The third argument is the sensor type (0: auto-detect, 11: DHT11, 22: DHT22)
        // The function signature might vary depending on the DHTXXD example version,
        // so check the downloaded DHTXXD.h file.
        // Common form: status = DHTXXD(gpio_pin, dht_type_or_auto, &dht_data_struct);
        // Or status = DHT(gpio_pin, &dht_data_struct); // Older example
        status = DHTXXD(DHT_SENSOR_PIN, DHT_SENSOR_TYPE, &dht_data);

        if (status == PI_DHT_GOOD) {
            // Data read successfully
            printf("------------------------------------\n");
            printf("Temperature: %.1f °C\n", dht_data.temperature);
            printf("Humidity: %.1f %%\n", dht_data.humidity);
            // If DHTXXD.h has a timestamp field
            if (dht_data.timestamp > 0) { // Check if timestamp is valid (simple check)
                 printf("Last read: %u seconds ago\n", (unsigned)(time(NULL) - dht_data.timestamp));
            }
            printf("------------------------------------\n");
            break; // Succeeded, so break the loop
        } else {
            // Failed to read data
            fprintf(stderr, "Failed to read DHT sensor. ");
            switch (status) {
                case PI_DHT_BAD_DATA:
                    fprintf(stderr, "Data error (checksum mismatch, etc.)\n");
                    break;
                case PI_DHT_TIMEOUT:
                    fprintf(stderr, "Timeout occurred\n");
                    break;
                case PI_INIT_FAILED: // This case might not be directly returned by DHTXXD, but good to be aware of
                    fprintf(stderr, "pigpio initialization failed (recheck gpioInitialise)\n");
                    break;
                default:
                    fprintf(stderr, "Unknown error code: %d\n", status);
                    break;
            }
            if (attempts > 0) {
                printf("Retrying in 2 seconds... (Attempts remaining: %d)\n", attempts);
                sleep(2); // DHT sensor should be read at intervals of at least 1-2 seconds
            }
        }
    }

    if (status != PI_DHT_GOOD) {
        fprintf(stderr, "Ultimately failed to read DHT sensor data.\n");
        fprintf(stderr, "  - Check BCM GPIO %d pin connection.\n", DHT_SENSOR_PIN);
        fprintf(stderr, "  - Check DHT11 sensor power and GND connection.\n");
        fprintf(stderr, "  - Check if a pull-up resistor is connected to the data pin (may be built into the module).\n");
        fprintf(stderr, "  - Check if the pigpiod daemon is running.\n");
    }

    // 3. Terminate pigpio library
    gpioTerminate();

    return (status == PI_DHT_GOOD) ? 0 : 1;
}
