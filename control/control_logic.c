#include "control_logic.h"
#include "gui.h"
#include "dht11_driver.h"
#include "motor_driver.h"
#include "lcd_driver.h"
#include "buzzer_driver.h"
#include <unistd.h>
#include <stdio.h>

// 새롭게 정의된 임계값
#define TEMPERATURE_THRESHOLD 27.0f
#define HUMIDITY_THRESHOLD    70.0f

// LCD 및 버저 경고 임계값
#define WARNING_TEMP_THRESHOLD 30.0f
#define WARNING_HUMI_THRESHOLD 80.0f

#define READ_INTERVAL_SECONDS 3

// GUI 업데이트를 수행할 콜백 (메인 GTK 스레드에서 안전하게 실행됨)
static gboolean update_gui_callback(gpointer user_data) {
    SharedData *data = (SharedData*)user_data;

    g_mutex_lock(&data->mutex);
    char temp_str[32], hum_str[32];
    // GUI 라벨 텍스트 수정
    sprintf(temp_str, "Temperature: %.1f C", data->temperature);
    sprintf(hum_str, "Humidity: %.1f %%", data->humidity);

    gtk_label_set_text(GTK_LABEL(data->widgets->lbl_temp), temp_str);
    gtk_label_set_text(GTK_LABEL(data->widgets->lbl_humidity), hum_str);

    if (data->mode == AUTOMATIC) {
        gtk_label_set_text(GTK_LABEL(data->widgets->lbl_status),
            data->is_running ? "Fan ON (Auto)" : "Fan OFF (Auto)");
    }
    g_mutex_unlock(&data->mutex);
    return G_SOURCE_REMOVE;
}

// 백그라운드 워커 스레드
void* worker_thread_func(void* user_data) {
    SharedData *data = (SharedData*)user_data;

    while (1) {
        dht11_trigger_read();
        sleep(1); 

        g_mutex_lock(&data->mutex);
        if (data->new_data_available) {
            
            // 1. Text LCD 업데이트
            lcd_display_update(data->temperature, data->humidity);

            // 2. 버저 제어 로직 추가
            if (data->temperature >= WARNING_TEMP_THRESHOLD || data->humidity >= WARNING_HUMI_THRESHOLD) {
                buzzer_on();
            } else {
                buzzer_off();
            }

            // 3. 자동 팬 제어
            if (data->mode == AUTOMATIC) {
                if (data->temperature > TEMPERATURE_THRESHOLD || data->humidity > HUMIDITY_THRESHOLD) {
                    if (!data->is_running) {
                        data->is_running = TRUE;
                        ventilation_on();
                        printf("[Auto Control] Fan ON (T:%.1f, H:%.1f)\n", data->temperature, data->humidity);
                    }
                } 
                // 조건에 해당하지 않으면 (온도와 습도 모두 임계값 이하)
                else {
                    // 팬이 켜져있다면 끈다
                    if (data->is_running) {
                        data->is_running = FALSE;
                        ventilation_off();
                        printf("[Auto Control] Fan OFF (T:%.1f, H:%.1f)\n", data->temperature, data->humidity);
                    }
                } else {
                    if (data->is_running) {
                        data->is_running = FALSE;
                        ventilation_off();
                    }
                }
            }
            data->new_data_available = FALSE;
        }
        g_mutex_unlock(&data->mutex);

        g_idle_add(update_gui_callback, data);

        sleep(READ_INTERVAL_SECONDS - 1);
    }
    return NULL;
}
