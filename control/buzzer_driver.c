#include <stdio.h>
#include <pigpiod_if2.h>
#include "buzzer_driver.h"
#include "motor_driver.h" // get_pi_handle() 함수를 사용하기 위함

#define BUZZER_PIN 18 // 버저 GPIO 핀 번호 (BCM)

// 버저 GPIO 핀을 출력으로 설정하고 초기 상태를 OFF로 만듭니다.
int buzzer_init() {
    int pi = get_pi_handle();
    if (pi < 0) {
        fprintf(stderr, "Buzzer init failed: Invalid pigpio handle.\n");
        return -1;
    }

    if (set_mode(pi, BUZZER_PIN, PI_OUTPUT) != 0) {
        fprintf(stderr, "Failed to set GPIO %d to OUTPUT.\n", BUZZER_PIN);
        return -1;
    }
    
    // 초기 상태는 꺼짐 (LOW 신호)
    gpio_write(pi, BUZZER_PIN, PI_LOW);
    printf("[Init] Buzzer on GPIO %d initialized.\n", BUZZER_PIN);
    return 0;
}

// 버저를 켭니다 (HIGH 신호)
void buzzer_on() {
    int pi = get_pi_handle();
    if (pi >= 0) {
        gpio_write(pi, BUZZER_PIN, PI_HIGH);
    }
}

// 버저를 끕니다 (LOW 신호)
void buzzer_off() {
    int pi = get_pi_handle();
    if (pi >= 0) {
        gpio_write(pi, BUZZER_PIN, PI_LOW);
    }
}