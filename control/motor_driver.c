#include "motor_driver.h"
#include <stdio.h>
#include <pigpiod_if2.h>

#define RELAY_PIN 24
#define RELAY_ON_SIGNAL  PI_LOW
#define RELAY_OFF_SIGNAL PI_HIGH

static int pi_handle = -1;

int init_pigpio() {
    pi_handle = pigpio_start(NULL, NULL);
    if (pi_handle < 0) {
        fprintf(stderr, "Failed to connect to pigpiod daemon. (sudo pigpiod)\n");
    }
    return pi_handle;
}

int get_pi_handle() {
    return pi_handle;
}

int setup_gpio() {
    if (pi_handle < 0) return -1;
    if (set_mode(pi_handle, RELAY_PIN, PI_OUTPUT) != 0) {
        fprintf(stderr, "Failed to set GPIO %d to OUTPUT.\n", RELAY_PIN);
        return -1;
    }
    gpio_write(pi_handle, RELAY_PIN, RELAY_OFF_SIGNAL); // 초기 상태: OFF
    return 0;
}

void ventilation_on() {
    if (pi_handle >= 0) gpio_write(pi_handle, RELAY_PIN, RELAY_ON_SIGNAL);
}

void ventilation_off() {
    if (pi_handle >= 0) gpio_write(pi_handle, RELAY_PIN, RELAY_OFF_SIGNAL);
}

void cleanup_pigpio() {
    if (pi_handle >= 0) {
        gpio_write(pi_handle, RELAY_PIN, RELAY_OFF_SIGNAL); // 종료 전 팬 끄기
        pigpio_stop(pi_handle);
        pi_handle = -1;
    }
}