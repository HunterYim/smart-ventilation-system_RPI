#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pigpio.h>

#define DHT_PIN 24  // BCM GPIO24

int read_dht11(int *temperature, int *humidity) {
    int data[5] = {0};
    uint32_t count = 0;

    gpioSetMode(DHT_PIN, PI_OUTPUT);
    gpioWrite(DHT_PIN, PI_LOW);
    gpioDelay(18000);  // 18ms
    gpioWrite(DHT_PIN, PI_HIGH);
    gpioDelay(40);

    gpioSetMode(DHT_PIN, PI_INPUT);

    // Wait for sensor response
    while (gpioRead(DHT_PIN) == PI_HIGH) {
        gpioDelay(1);
        if (++count > 100) return 0;
    }

    count = 0;
    while (gpioRead(DHT_PIN) == PI_LOW) {
        gpioDelay(1);
        if (++count > 100) return 0;
    }

    count = 0;
    while (gpioRead(DHT_PIN) == PI_HIGH) {
        gpioDelay(1);
        if (++count > 100) return 0;
    }

    // Read 40 bits
    for (int i = 0; i < 40; i++) {
        while (gpioRead(DHT_PIN) == PI_LOW);  // Start each bit

        gpioDelay(30);
        if (gpioRead(DHT_PIN) == PI_HIGH) {
            data[i / 8] <<= 1;
            data[i / 8] |= 1;
        } else {
            data[i / 8] <<= 1;
        }

        // Wait for end of bit
        count = 0;
        while (gpioRead(DHT_PIN) == PI_HIGH) {
            gpioDelay(1);
            if (++count > 100) break;
        }
    }

    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        *humidity = data[0];
        *temperature = data[2];
        return 1;
    }

    return 0;
}

int main() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio 초기화 실패\n");
        return 1;
    }

    int temp = 0, humi = 0;
    while (1) {
        if (read_dht11(&temp, &humi)) {
            printf("temperature: %d°C   humidity: %d%%\n", temp, humi);
        } else {
            printf("❌ Failed. Retry...\n");
        }
        sleep(2);
    }

    gpioTerminate();
    return 0;
}
