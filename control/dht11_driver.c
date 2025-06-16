#include "dht11_driver.h"
#include "motor_driver.h" // get_pi_handle() 사용
#include "DHTXXD.h"
#include <stdio.h>

#define DHT_SENSOR_GPIO 27
#define DHT_SENSOR_MODEL DHT11

static DHTXXD_t *dht_sensor_handle = NULL;
static SharedData *g_shared_data_for_callback = NULL;

// 센서 데이터 수신 콜백 함수
void dht_sensor_callback(DHTXXD_data_t data) {
    if (g_shared_data_for_callback && (data.status == DHT_GOOD || data.status == DHT_BAD_DATA)) {
        g_mutex_lock(&g_shared_data_for_callback->mutex);
        g_shared_data_for_callback->temperature = data.temperature;
        g_shared_data_for_callback->humidity = data.humidity;
        g_shared_data_for_callback->new_data_available = TRUE; // 새 데이터 플래그 설정
        g_mutex_unlock(&g_shared_data_for_callback->mutex);
    }
}

int dht11_init(SharedData *data) {
    g_shared_data_for_callback = data;
    int pi = get_pi_handle();
    if (pi < 0) return -1;

    dht_sensor_handle = DHTXXD(pi, DHT_SENSOR_GPIO, DHT_SENSOR_MODEL, dht_sensor_callback);
    if (dht_sensor_handle == NULL) {
        fprintf(stderr, "Failed to initialize DHT sensor on GPIO %d.\n", DHT_SENSOR_GPIO);
        return -1;
    }
    return 0;
}

void dht11_trigger_read() {
    if (dht_sensor_handle) DHTXXD_manual_read(dht_sensor_handle);
}

void dht11_cleanup() {
    if (dht_sensor_handle) {
        DHTXXD_cancel(dht_sensor_handle);
        dht_sensor_handle = NULL;
    }
}