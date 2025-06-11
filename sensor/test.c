#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd;
    char line1[17], line2[17];
    int temp = 26; // 여기에 네 DHT 센서 값 넣어
    int humi = 60;

    // 출력 문자열 준비 (16자 이내 권장)
    snprintf(line1, sizeof(line1), "Temp: %d C", temp);
    snprintf(line2, sizeof(line2), "Humi: %d %%", humi);

    // LCD 장치 열기
    fd = open("/dev/fpga_text_lcd", O_WRONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    // 32자 = 16자(line1) + 16자(line2)
    char lcd_data[33] = {0};
    snprintf(lcd_data, sizeof(lcd_data), "%-16s%-16s", line1, line2); // 왼쪽 정렬로 32자 구성

    // LCD에 쓰기
    write(fd, lcd_data, 32);
    close(fd);

    return 0;
}
