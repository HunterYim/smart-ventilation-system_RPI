#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

// pigpio 초기화
int init_pigpio();

// pigpio 핸들 반환 (DHT 드라이버에서 공유하기 위함)
int get_pi_handle();

// GPIO 초기화
int setup_gpio();

// 환기 시스템 켜기
void ventilation_on();

// 환기 시스템 끄기
void ventilation_off();

// 리소스 정리
void cleanup_pigpio();

#endif