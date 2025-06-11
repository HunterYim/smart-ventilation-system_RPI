#include "control_logic.hh"
#include "gui.h"
#include "dht11_driver.h"
#include "motor_driver.h"
#include <unistd.h>
#include <stdio.h>

#define TEMPERATURE_THRESHOLD_HIGH 33.0f
#define TEMPERATURE_THRESHOLD_LOW  29.0f
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
        dht11_trigger_read();
        sleep(1); // 콜백이 데이터를 처리할 시간

        g_mutex_lock(&data->mutex);
        if (data->new_data_available) {
            if (data->mode == AUTOMATIC) {
                if (data->is_running) { // 팬이 켜져 있을 때
                    if (data->temperature < TEMPERATURE_THRESHOLD_LOW) {
                        data->is_running = FALSE;
                        ventilation_off();
                    }
                } else { // 팬이 꺼져 있을 때
                    if (data->temperature > TEMPERATURE_THRESHOLD_HIGH) {
                        data->is_running = TRUE;
                        ventilation_on();
                    }
                }
            }
            data->new_data_available = FALSE; // 플래그 리셋
        }
        g_mutex_unlock(&data->mutex);

        g_idle_add(update_gui_callback, data); // GUI 업데이트 요청

        sleep(READ_INTERVAL_SECONDS - 1); // 다음 주기까지 대기
    }
    return NULL;
}