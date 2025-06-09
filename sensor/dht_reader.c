#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define GPIO_CHIP "/dev/gpiochip0"
#define GPIO_LINE 24   // BCM GPIO24
#define MAX_TIMINGS 85
#define TIMEOUT_US 100

int data[5] = {0, 0, 0, 0, 0};

long get_microseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (ts.tv_sec * 1000000L + ts.tv_nsec / 1000L);
}

void delay_us(unsigned int us) {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

int read_dht11(int *temperature, int *humidity) {
    struct gpiod_chip *chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip) {
        perror("chip open");
        return 0;
    }

    struct gpiod_line *line = gpiod_chip_get_line(chip, GPIO_LINE);
    if (!line) {
        perror("line get");
        gpiod_chip_close(chip);
        return 0;
    }

    memset(data, 0, sizeof(data));

    gpiod_line_request_output(line, "dht11", 0);
    gpiod_line_set_value(line, 0);
    delay_us(18000);  // 18ms
    gpiod_line_set_value(line, 1);
    delay_us(40);

    gpiod_line_release(line);
    gpiod_line_request_input(line, "dht11");

    int last_state = 1, counter = 0, j = 0;
    long start_time;

    for (int i = 0; i < MAX_TIMINGS; i++) {
        counter = 0;
        start_time = get_microseconds();
        while (gpiod_line_get_value(line) == last_state) {
            counter++;
            delay_us(1);
            if (get_microseconds() - start_time > TIMEOUT_US)
                break;
        }
        last_state = gpiod_line_get_value(line);
        if (counter == 0 || counter >= 255) break;

        if ((i >= 4) && (i % 2 == 0)) {
            data[j / 8] <<= 1;
            if (counter > 40)
                data[j / 8] |= 1;
            j++;
        }
    }

    gpiod_line_release(line);
    gpiod_chip_close(chip);

    if (j >= 40 && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
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
            printf("temperature: %d°C   humidity: %d%%\n", temp, humi);
        } else {
            printf("❌ Failed. Retry...\n");
        }
        sleep(2);
    }

    return 0;
}
