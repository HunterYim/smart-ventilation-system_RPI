#ifndef DHT11_DRIVER_H
#define DHT11_DRIVER_H

#include "gui.h" // SharedData 구조체를 사용하기 위함

// DHT 센서 초기화 (SharedData 포인터를 받아 콜백에서 사용)
int dht11_init(SharedData *data);

// 수동으로 센서 읽기 트리거
void dht11_trigger_read();

// DHT 센서 관련 리소스 정리
void dht11_cleanup();

#endif