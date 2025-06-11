#include "dht11_driver.h"
#include "motor_driver.h" // get_pi_handle()을 사용하기 위해
#include "DHTXXD.h"
#include <stdio.h>

#define DHT_SENSOR_GPIO 27
#define DHT_SENSOR_MODEL DHT11

static DHTXXD_t *dht_sensor_handle = NULL;
static SharedData *g_shared_data = NULL; // 콜백에서 사용할 공유 데이터 포인터

// DHT 센서 데이터 수신 콜백 함수
void dht_sensor_callback(DHTXXD_data_t data) {
    // 유효한 데이터가 수신되면 SharedData 구조체를 업데이트
    // g_shared_data가 NULL이 아닌지 확인
    if (g_shared_data && (data.status == DHT_GOOD || data.status == DHT_BAD_DATA)) {
        // 스레드 충돌을 막기 위해 뮤텍스로 보호
        g_mutex_lock(&g_shared_data->mutex);
        
        g_shared_data->temperature = data.temperature;
        g_shared_data->humidity = data.humidity;
        g_shared_data->new_data_available = TRUE; // 새 데이터가 있음을 알림

        g_mutex_unlock(&g_shared_data->mutex);
    } else if (g_shared_data) {
        fprintf(stderr, "Callback: Failed to read DHT sensor. Error code: %d\n", data.status);
         g_mutex_lock(&g_shared_data->mutex);
         g_shared_data->new_data_available = FALSE;
         g_mutex_unlock(&g_shared_data->mutex);
    }
}

int dht11_init(SharedData *data) {
    g_shared_data = data; // 전역 포인터에 공유 데이터 주소 저장
    int pi = get_pi_handle();
    if (pi < 0) {
        fprintf(stderr, "DHT init failed: invalid pigpio handle.\n");
        return -1;
    }

    dht_sensor_handle = DHTXXD(pi, DHT_SENSOR_GPIO, DHT_SENSOR_MODEL, dht_sensor_callback);
    if (dht_sensor_handle == NULL) {
        fprintf(stderr, "Failed to initialize DHT sensor on GPIO %d.\n", DHT_SENSOR_GPIO);
        return -1;
    }
    printf("DHT sensor initialized on GPIO %d.\n", DHT_SENSOR_GPIO);
    return 0;
}

void dht11_trigger_read() {
    if (dht_sensor_handle) {
        DHTXXD_manual_read(dht_sensor_handle);
    }
}

void dht11_cleanup() {
    if (dht_sensor_handle) {
        DHTXXD_cancel(dht_sensor_handle);
        dht_sensor_handle = NULL;
        printf("DHTXXD sensor operations cancelled.\n");
    }
}