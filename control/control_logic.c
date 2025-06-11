#include "control_logic.h"
#include "dht11_driver.h"
#include "motor_driver.h"
#include <unistd.h>
#include <stdio.h>

// 제공된 코드의 임계값 사용
#define TEMPERATURE_THRESHOLD_HIGH 33.0f
#define TEMPERATURE_THRESHOLD_LOW  29.0f

// GUI 업데이트 콜백 (이전과 동일)
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
        
        // 2. 콜백이 데이터를 처리할 시간을 줌 (약 1초)
        sleep(1);

        // 3. 공유 데이터에 접근하여 로직 수행
        g_mutex_lock(&data->mutex);

        // 새 데이터가 들어왔을 때만 자동 제어 로직 실행
        if (data->new_data_available) {
            if (data->mode == AUTOMATIC) {
                // Hysteresis(이력) 로직: 급격한 On/Off 방지
                if (data->is_running) { // 현재 팬이 켜져 있다면
                    if (data->temperature < TEMPERATURE_THRESHOLD_LOW) {
                        data->is_running = FALSE;
                        ventilation_off();
                    }
                } else { // 현재 팬이 꺼져 있다면
                    if (data->temperature > TEMPERATURE_THRESHOLD_HIGH) {
                        data->is_running = TRUE;
                        ventilation_on();
                    }
                }
            }
            data->new_data_available = FALSE; // 플래그 리셋
        }
        
        g_mutex_unlock(&data->mutex);

        // 4. GUI 업데이트를 메인 스레드에 요청
        g_idle_add(update_gui_callback, data);

        // 5. 다음 주기를 위해 대기 (전체 주기는 약 5초)
        sleep(4);
    }
    return NULL;
}