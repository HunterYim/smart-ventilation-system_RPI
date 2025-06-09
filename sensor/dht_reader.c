#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define GPIO_CHIP "/dev/gpiochip0"
#define GPIO_LINE 24  // BCM 24번 (DHT11 DATA 핀)
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
    if (!chip) {
        perror("❌ Failed to open GPIO chip");
        return 0;
    }

    struct gpiod_line *line = gpiod_chip_get_line(chip, GPIO_LINE);
    if (!line) {
        perror("❌ Failed to get GPIO line");
        gpiod_chip_close(chip);
        return 0;
    }

    int laststate = 1, counter = 0, j = 0;
    memset(data, 0, sizeof(data));

    gpiod_line_request_output(line, "dht11", 0);
    gpiod_line_set_value(line, 0);
    usleep(18000);  // 18ms start signal
    gpiod_line_set_value(line, 1);
    busy_wait_us(40);

    gpiod_line_release(line);
    gpiod_line_request_input(line, "dht11");

    for (int i = 0; i < MAX_TIMINGS; i++) {
        counter = 0;
        while (gpiod_line_get_value(line) == laststate) {
            counter++;
            busy_wait_us(1);
            if (counter > 255) break;
        }
        laststate = gpiod_line_get_value(line);

        if (counter > 255) break;

        if ((i >= 4) && (i % 2 == 0)) {
            data[j / 8] <<= 1;
            if (counter > 50)
                data[j / 8] |= 1;
            j++;
        }
    }

    gpiod_line_release(line);
    gpiod_chip_close(chip);

    if (j >= 40) {
        int checksum = data[0] + data[1] + data[2] + data[3];
        if ((data[4] & 0xFF) == (checksum & 0xFF)) {
            *humidity = data[0];
            *temperature = data[2];
            return 1;
        } else {
            printf("❌ Checksum mismatch: got %d, expected %d\n", data[4], checksum & 0xFF);
            return 0;
        }
    } else {
        printf("❌ Failed to read full 40 bits (got %d bits)\n", j);
        return 0;
    }
}

int main() {
    int temp, humi;

    while (1) {
        if (read_dht11(&temp, &humi)) {
            printf("temperature: %d°C   humidity: %d%%\n", temp, humi);
        } else {
            printf("Retrying in 2 seconds...\n");
        }
        sleep(2);
    }

    return 0;
}
