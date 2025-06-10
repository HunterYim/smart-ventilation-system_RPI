#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // for time_sleep, sleep
#include <signal.h>     // for signal handling
#include <pigpiod_if2.h> // pigpio C interface (for pigpiod daemon)
#include "DHTXXD.h"      // DHT sensor related header

// --- User configurable settings ---
#define RELAY_PIN 24        // GPIO pin for relay control (BCM)
#define DHT_SENSOR_GPIO 27  // GPIO pin for DHT sensor data (BCM)
#define DHT_SENSOR_MODEL DHT11 // Or DHT22, etc., as defined in DHTXXD.h (e.g., 0=auto, 1=DHT11, 2=DHT22/AM2302)

// Relay trigger type (assuming Low Level Trigger)
#define IS_LOW_LEVEL_TRIGGER 1 // 1: Low Level, 0: High Level

#if IS_LOW_LEVEL_TRIGGER
    #define RELAY_ON_SIGNAL  PI_LOW
    #define RELAY_OFF_SIGNAL PI_HIGH
#else
    #define RELAY_ON_SIGNAL  PI_HIGH
    #define RELAY_OFF_SIGNAL PI_LOW
#endif

// Fan operation conditions
#define TEMPERATURE_THRESHOLD_HIGH 30.0f
#define HUMIDITY_THRESHOLD_HIGH    70.0f // Not used in current logic, but kept for potential future use
#define TEMPERATURE_THRESHOLD_LOW  27.0f

#define READ_INTERVAL_SECONDS 5 // Sensor reading and control interval (seconds)

// pigpio daemon connection handle
int pi_handle = -1;
DHTXXD_t *dht_sensor_handle = NULL; // Handle for the DHT sensor object

// Global variables to store the latest sensor data from callback
volatile float current_temperature = -999.0f; // Initialize with an unlikely value
volatile float current_humidity = -999.0f;
volatile int sensor_data_ready = 0; // Flag to indicate new data is available

void cleanup_gpio_and_sensor() {
    if (pi_handle >= 0) {
        // Ensure relay is turned OFF before exiting
        if (set_mode(pi_handle, RELAY_PIN, PI_OUTPUT) == 0) { // Ensure mode is output before writing
             gpio_write(pi_handle, RELAY_PIN, RELAY_OFF_SIGNAL);
             printf("Relay on GPIO %d turned OFF before exit.\n", RELAY_PIN);
        } else {
            fprintf(stderr, "Warning: Failed to set GPIO %d mode to OUTPUT during cleanup.\n", RELAY_PIN);
        }

        if (dht_sensor_handle != NULL) {
            DHTXXD_cancel(dht_sensor_handle); // Cancel DHTXXD operations
            dht_sensor_handle = NULL;
            printf("DHTXXD sensor operations cancelled.\n");
        }
        pigpio_stop(pi_handle); // Disconnect from pigpio daemon
        pi_handle = -1;
        printf("pigpio connection stopped.\n");
    }
}

void handle_exit_signals(int signum) {
    printf("\nCaught signal %d, cleaning up and exiting...\n", signum);
    cleanup_gpio_and_sensor();
    exit(signum);
}

// Callback function to receive DHT sensor data
void dht_sensor_callback(DHTXXD_data_t data) {
    if (data.status == PI_DHT_GOOD) {
        current_temperature = data.temperature;
        current_humidity = data.humidity;
        sensor_data_ready = 1; // Set flag that new data is available
        // printf("Callback: Temp=%.1fC, Hum=%.1f%%\n", data.temperature, data.humidity); // Optional: for debugging callback
    } else {
        fprintf(stderr, "Callback: Failed to read DHT sensor. Error code: %d\n", data.status);
        // Optionally, set sensor_data_ready = 0 or handle error appropriately
        sensor_data_ready = 0; // Indicate data is not good
    }
}

int main(int argc, char *argv[]) {
    int fan_on = 0; // 0: OFF, 1: ON

    // Register signal handlers for graceful shutdown
    signal(SIGINT, handle_exit_signals);
    signal(SIGTERM, handle_exit_signals);

    // 1. Connect to pigpio daemon
    pi_handle = pigpio_start(NULL, NULL);
    if (pi_handle < 0) {
        fprintf(stderr, "Failed to connect to pigpiod daemon. Error code: %d\n", pi_handle);
        fprintf(stderr, "Ensure pigpiod daemon is running (e.g., sudo pigpiod).\n");
        return 1;
    }
    printf("Connected to pigpiod daemon (handle: %d).\n", pi_handle);

    // 2. Set relay GPIO pin to output mode and initial state to OFF
    if (set_mode(pi_handle, RELAY_PIN, PI_OUTPUT) != 0) {
        fprintf(stderr, "Failed to set GPIO %d mode to OUTPUT. Error: %s\n", RELAY_PIN, pigpio_error(pigpio_last_error(pi_handle)));
        cleanup_gpio_and_sensor();
        return 1;
    }
    gpio_write(pi_handle, RELAY_PIN, RELAY_OFF_SIGNAL);
    printf("GPIO %d set to OUTPUT. Initial relay state: OFF.\n", RELAY_PIN);

    // 3. Initialize DHT sensor using the callback mechanism
    //    The DHTXXD function signature from test_DHTXXD.c is:
    //    DHTXXD_t *DHTXXD(int pi, int gpio, int model, DHTXXD_callback_t callback)
    dht_sensor_handle = DHTXXD(pi_handle, DHT_SENSOR_GPIO, DHT_SENSOR_MODEL, dht_sensor_callback);
    if (dht_sensor_handle == NULL) { // DHTXXD might return NULL on failure (check its implementation)
        fprintf(stderr, "Failed to initialize DHT sensor on GPIO %d.\n", DHT_SENSOR_GPIO);
        cleanup_gpio_and_sensor();
        return 1;
    }
    printf("DHT sensor initialized on GPIO %d. Waiting for first reading...\n", DHT_SENSOR_GPIO);

    // 4. Main loop
    while (1) {
        // Trigger a manual read. The data will be delivered to dht_sensor_callback.
        if (dht_sensor_handle) {
            DHTXXD_manual_read(dht_sensor_handle);
        }

        // Wait a moment for the callback to potentially update the data
        time_sleep(1); // Give some time for the read and callback to occur

        if (sensor_data_ready) {
            printf("Main Loop - Sensor Data: Temp = %.1f*C, Humidity = %.1f%%\n", current_temperature, current_humidity);

            // Fan control logic using global variables updated by callback
            if (fan_on) { // If fan is currently ON
                if (current_temperature < TEMPERATURE_THRESHOLD_LOW && current_temperature > -50.0f) { // Check for valid temp
                    gpio_write(pi_handle, RELAY_PIN, RELAY_OFF_SIGNAL);
                    printf("Fan turned OFF (Temp: %.1fC < %.1fC)\n", current_temperature, TEMPERATURE_THRESHOLD_LOW);
                    fan_on = 0;
                }
            } else { // If fan is currently OFF
                if (current_temperature > TEMPERATURE_THRESHOLD_HIGH && current_temperature < 100.0f) { // Check for valid temp
                    gpio_write(pi_handle, RELAY_PIN, RELAY_ON_SIGNAL);
                    printf("Fan turned ON (Temp: %.1fC > %.1fC)\n", current_temperature, TEMPERATURE_THRESHOLD_HIGH);
                    fan_on = 1;
                }
            }
            sensor_data_ready = 0; // Reset flag after processing data
        } else {
            printf("Main Loop - No new sensor data or sensor read failed.\n");
        }

        time_sleep(READ_INTERVAL_SECONDS -1); // Subtract the 1 second already slept
                                             // Ensure total loop time is approx. READ_INTERVAL_SECONDS
    }

    cleanup_gpio_and_sensor(); // Should be called by signal handler
    return 0;
}
