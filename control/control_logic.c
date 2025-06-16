#include "control_logic.h"
#include "gui.h"
#include "dht11_driver.h"
#include "motor_driver.h"
#include "lcd_driver.h"
#include "buzzer_driver.h"
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

// 새롭게 정의된 임계값
#define TEMPERATURE_THRESHOLD 27.0f
#define HUMIDITY_THRESHOLD    70.0f

// LCD 및 버저 경고 임계값
#define WARNING_TEMP_THRESHOLD 27.0f
#define WARNING_HUMI_THRESHOLD 70.0f

#define READ_INTERVAL_SECONDS 3

// GUI 업데이트를 수행할 콜백 (메인 GTK 스레드에서 안전하게 실행됨)
static gboolean update_gui_callback(gpointer user_data) {
    SharedData *data = (SharedData*)user_data;

    // data나 widgets, 또는 특정 위젯이 아직 준비되지 않았다면 아무것도 하지 않고 종료
    if (!data || !data->widgets || !data->widgets->lbl_temp) {
        return G_SOURCE_REMOVE;
    }

    g_mutex_lock(&data->mutex);
    char temp_str[32], hum_str[32];
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
            
            // 디버그 메시지
            printf("[Debug Logic] New data processed -> Temp: %.1f C, Humi: %.1f %%\n", 
                   data->temperature, data->humidity);
            
            // 1. Text LCD 업데이트
            lcd_display_update(data->temperature, data->humidity);

            // 2. 수정된 버저 제어 로직
            
            // 현재 센서 값 기준으로 경고 상태인지 판단
            bool is_warning_condition = (data->temperature >= WARNING_TEMP_THRESHOLD || data->humidity >= WARNING_HUMI_THRESHOLD);

            // [핵심 로직] 경고 조건이 발생했고, 이전에는 경고 상태가 아니었을 때 (즉, 딱 한 번만 실행됨)
            if (is_warning_condition && !data->is_alert_active) {
                
                // 1. 경고 상태를 '활성'으로 변경하여 중복 실행 방지
                data->is_alert_active = TRUE;
                
                // 2. 뮤텍스를 잠시 풀고 버저를 제어 (sleep이 뮤텍스를 오래 잡고 있는 것을 방지)
                g_mutex_unlock(&data->mutex);
                
                printf("[Alert] Warning condition met. Sounding buzzer for 5 seconds...\n");
                buzzer_on();
                sleep(5); // 5초 동안 대기
                buzzer_off();
                printf("[Alert] Buzzer stopped.\n");

                // 3. 다시 뮤텍스를 잠그고 루프를 계속 진행
                g_mutex_lock(&data->mutex);

            } 
            // 경고 조건이 해제되었을 때 (온/습도가 정상으로 돌아왔을 때)
            else if (!is_warning_condition && data->is_alert_active) {
                // 경고 상태를 '비활성'으로 리셋하여 다음 경고를 준비
                data->is_alert_active = FALSE; 
                printf("[Alert] Warning condition cleared. System reset.\n");
            }

            // 3. 자동 팬 제어
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

        // 여기서의 sleep 시간은 버저 작동 시간에 영향을 받지 않음
        sleep(READ_INTERVAL_SECONDS - 1);
    }
    return NULL;
}