#include "control_logic.h"
#include "gui.h"
#include "dht11_driver.h"
#include "motor_driver.h"
#include <unistd.h>
#include <stdio.h>

// #define TEMPERATURE_THRESHOLD_HIGH 33.0f
// #define TEMPERATURE_THRESHOLD_LOW  29.0f
#define TEMPERATURE_THRESHOLD 27.0f
#define HUMIDITY_THRESHOLD    70.0f
#define READ_INTERVAL_SECONDS 5

// GUI 업데이트를 수행할 콜백 (메인 GTK 스레드에서 안전하게 실행됨)
static gboolean update_gui_callback(gpointer user_data) {
    SharedData *data = (SharedData*)user_data;

    g_mutex_lock(&data->mutex);
    char temp_str[32], hum_str[32];
    sprintf(temp_str, "temperature: %.1f °C", data->temperature);
    sprintf(hum_str, "humidity: %.1f %%", data->humidity);

    gtk_label_set_text(GTK_LABEL(data->widgets->lbl_temp), temp_str);
    gtk_label_set_text(GTK_LABEL(data->widgets->lbl_humidity), hum_str);

    if (data->mode == AUTOMATIC) {
        gtk_label_set_text(GTK_LABEL(data->widgets->lbl_status),
            data->is_running ? "fan on (auto)" : "fan off (auto)");
    }
    g_mutex_unlock(&data->mutex);
    return G_SOURCE_REMOVE;
}

// 백그라운드 워커 스레드
void* worker_thread_func(void* user_data) {
    SharedData *data = (SharedData*)user_data;

    while (1) {
        // 1. 센서 읽기 요청
        dht11_trigger_read();

        // 2. 콜백이 데이터를 처리할 시간 제공
        sleep(1);

        // 3. 공유 데이터에 접근하여 로직 수행 (뮤텍스로 보호)
        g_mutex_lock(&data->mutex);

        if (data->new_data_available) {
            // 자동 모드일 때만 제어 로직 실행
            if (data->mode == AUTOMATIC) {
                // ★★★★★ 새로운 조건 로직 ★★★★★
                // 조건: 온도가 25도 이상이거나, 습도가 60% 이상인가?
                if (data->temperature >= TEMPERATURE_THRESHOLD || data->humidity >= HUMIDITY_THRESHOLD) {
                    // 조건 만족 시: 팬을 켭니다 (단, 이미 켜져있으면 아무것도 안 함).
                    if (!data->is_running) {
                        data->is_running = TRUE;
                        ventilation_on();
                        // printf 로그는 필요 시 추가
                    }
                } else {
                    // 조건 불만족 시: 팬을 끕니다 (단, 이미 꺼져있으면 아무것도 안 함).
                    if (data->is_running) {
                        data->is_running = FALSE;
                        ventilation_off();
                        // printf 로그는 필요 시 추가
                    }
                }
            }
            data->new_data_available = FALSE; // 플래그 리셋
        }

        g_mutex_unlock(&data->mutex);

        // 4. GUI 업데이트를 메인 스레드에 요청
        g_idle_add(update_gui_callback, data);

        // 5. 다음 주기를 위해 대기
        sleep(4);
    }
    return NULL;
}
