#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define GPIO_CHIP "/dev/gpiochip0"
#define GPIO_LINE 24  // BCM 24번 핀 (DATA)
#define MAX_TIMINGS 85

int data[5] = {0, 0, 0, 0, 0};

long get_microseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (ts.tv_sec * 1000000L + ts.tv_nsec / 1000L);
}

void busy_wait_us(int delay_us) {
    long start = get_microseconds();
    while (get_microseconds() - start < delay_us);
}

int read_dht11(int *temperature, int *humidity) {
    struct gpiod_chip *chip = gpiod_chip_open(GPIO_CHIP);
    struct gpiod_line *line = gpiod_chip_get_line(chip, GPIO_LINE);
    int laststate = 1, counter = 0, j = 0;

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    gpiod_line_request_output(line, "dht", 0);
    gpiod_line_set_value(line, 0);
    usleep(18000);
    gpiod_line_set_value(line, 1);
    busy_wait_us(40);

    gpiod_line_release(line);
    gpiod_line_request_input(line, "dht");

    for (int i = 0; i < MAX_TIMINGS; i++) {
        counter = 0;
        while (gpiod_line_get_value(line) == laststate) {
            counter++;
            busy_wait_us(1);
            if (counter == 255) break;
        }
        laststate = gpiod_line_get_value(line);

        if (counter == 255) break;

        if ((i >= 4) && (i % 2 == 0)) {
            data[j / 8] <<= 1;
            if (counter > 50) data[j / 8] |= 1;
            j++;
        }
    }

    gpiod_line_release(line);
    gpiod_chip_close(chip);

    if ((j >= 40) &&
        (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        *humidity = data[0];
        *temperature = data[2];
        return 1;
    } else {
        return 0;
    }
}

int main() {
    int temp = 0, humi = 0;

    while (1) {
        if (read_dht11(&temp, &humi)) {
            printf("🌡 temperature: %d°C  💧 humidity: %d%%\n", temp, humi);
        } else {
            printf("❌ 센서 읽기 실패. 다시 시도합니다...\n");
        }
        sleep(2);
    }

    return 0;
}
