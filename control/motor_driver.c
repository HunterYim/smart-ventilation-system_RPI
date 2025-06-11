#include "motor_driver.h"
#include <stdio.h>
#include <pigpiod_if2.h>

#define RELAY_PIN 24 // BCM 핀 번호

// Low Level Trigger 기준
#define RELAY_ON_SIGNAL  PI_LOW
#define RELAY_OFF_SIGNAL PI_HIGH

// pigpio 핸들을 static으로 관리
static int pi_handle = -1;

int init_pigpio() {
    pi_handle = pigpio_start(NULL, NULL);
    if (pi_handle < 0) {
        fprintf(stderr, "Failed to connect to pigpiod daemon. Error code: %d\n", pi_handle);
        fprintf(stderr, "Ensure pigpiod daemon is running (e.g., sudo pigpiod).\n");
    }
    return pi_handle;
}

int get_pi_handle() {
    return pi_handle;
}

int setup_gpio() {
    if (pi_handle < 0) return -1;
    
    int result = set_mode(pi_handle, RELAY_PIN, PI_OUTPUT);
    if (result != 0) {
        fprintf(stderr, "Failed to set GPIO %d to OUTPUT. Error: %d\n", RELAY_PIN, result);
        return -1;
    }
    // 초기 상태를 OFF로 설정
    gpio_write(pi_handle, RELAY_PIN, RELAY_OFF_SIGNAL);
    printf("GPIO %d set to OUTPUT. Initial state: OFF\n", RELAY_PIN);
    return 0;
}

void ventilation_on() {
    if (pi_handle >= 0) {
        gpio_write(pi_handle, RELAY_PIN, RELAY_ON_SIGNAL);
        printf("Fan turned ON.\n");
    }
}

void ventilation_off() {
    if (pi_handle >= 0) {
        gpio_write(pi_handle, RELAY_PIN, RELAY_OFF_SIGNAL);
        printf("Fan turned OFF.\n");
    }
}

void cleanup_pigpio() {
    if (pi_handle >= 0) {
        // 종료 전 팬 끄기
        gpio_write(pi_handle, RELAY_PIN, RELAY_OFF_SIGNAL);
        pigpio_stop(pi_handle);
        pi_handle = -1;
        printf("pigpio connection stopped.\n");
    }
}