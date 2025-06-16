#ifndef BUZZER_DRIVER_H
#define BUZZER_DRIVER_H

// 버저 디바이스 파일이 사용 가능한지 확인하는 함수
int buzzer_init();

// 버저를 켜는 함수
void buzzer_on();

// 버저를 끄는 함수
void buzzer_off();

#endif