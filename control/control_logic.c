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

            // 2. 버저 제어 로직 (상태 변수 사용)
            // 경고 조건이 발생했을 때
            if (data->temperature >= WARNING_TEMP_THRESHOLD || data->humidity >= WARNING_HUMI_THRESHOLD) {
                // 아직 경고가 활성화되지 않았다면,
                if (!data->is_alert_active) {
                    data->is_alert_active = TRUE; // 상태를 '활성'으로 변경하고
                    buzzer_on();                  // 버저를 켠다.
                    printf("[Alert] Buzzer ON\n");
                }
            } 
            // 경고 조건이 해제되었을 때
            else {
                // 이전에 경고가 활성화 상태였다면,
                if (data->is_alert_active) {
                    data->is_alert_active = FALSE; // 상태를 '비활성'으로 변경하고
                    buzzer_off();                  // 버저를 끈다.
                    printf("[Alert] Buzzer OFF\n");
                }
            }

            // 3. 자동 팬 제어 (기존 로직)
            if (data->mode == AUTOMATIC) {
                if (data->temperature > TEMPERATURE_THRESHOLD || data->humidity > HUMIDITY_THRESHOLD) {
                    if (!data->is_running) {
                        data->is_running = TRUE;
                        ventilation_on();
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