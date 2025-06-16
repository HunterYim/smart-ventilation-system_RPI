#ifndef DHT11_DRIVER_H
#define DHT11_DRIVER_H

#include "gui.h" // SharedData 구조체를 사용하기 위함

int dht11_init(SharedData *data); // 센서 초기화
void dht11_trigger_read(); // 센서 값 읽기 요청
void dht11_cleanup(); // 센서 리소스 정리

#endif