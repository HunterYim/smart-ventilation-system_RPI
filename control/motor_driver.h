#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

int init_pigpio(); // pigpio 라이브러리 연결
int get_pi_handle(); // 다른 모듈에서 pigpio 핸들을 쓰기 위함
int setup_gpio(); // 릴레이 핀 초기 설정
void ventilation_on(); // 팬 켜기
void ventilation_off(); // 팬 끄기
void cleanup_pigpio(); // pigpio 연결 해제 및 정리

#endif