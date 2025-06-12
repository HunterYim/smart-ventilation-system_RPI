#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "buzzer_driver.h"

// FPGA 버저 디바이스 파일 경로
#define BUZZER_DEVICE "/dev/fpga_buzzer"

// 프로그램 시작 시 버저 디바이스가 사용 가능한지 확인합니다.
int buzzer_init() {
    int dev = open(BUZZER_DEVICE, O_WRONLY);
    if (dev < 0) {
        fprintf(stderr, "[Error] Buzzer device %s open failed!\n", BUZZER_DEVICE);
        fprintf(stderr, "Please check if the kernel module (fpga_buzzer_driver.ko) is loaded.\n");
        return -1;
    }
    // 확인 후 바로 닫아줍니다.
    close(dev);
    printf("[Init] Buzzer device %s found.\n", BUZZER_DEVICE);
    return 0;
}

// 버저를 켭니다 (디바이스에 '1'을 씀)
void buzzer_on() {
    int dev = open(BUZZER_DEVICE, O_WRONLY);
    if (dev < 0) {
        // 초기화 때 확인했더라도, 런타임 중에 문제가 생길 수 있으므로 방어 코드 추가
        perror("buzzer_on: device open failed");
        return;
    }
    
    unsigned char data = 1;
    write(dev, &data, 1);
    
    close(dev);
}

// 버저를 끕니다 (디바이스에 '0'을 씀)
void buzzer_off() {
    int dev = open(BUZZER_DEVICE, O_WRONLY);
    if (dev < 0) {
        perror("buzzer_off: device open failed");
        return;
    }

    unsigned char data = 0;
    write(dev, &data, 1);

    close(dev);
}