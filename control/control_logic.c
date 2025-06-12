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
        // DHTXXD_auto_read를 사용하므로 이 함수는 더 이상 필요 없습니다.
        // dht11_trigger_read();
        
        // auto_read가 주기적으로 콜백을 호출하므로, 스레드는 잠시 대기하며 확인만 합니다.
        sleep(1); 

        g_mutex_lock(&data->mutex);
        if (data->new_data_available) {
            
            // 1. Text LCD 업데이트 (올바른 함수 호출 방식으로 수정)
            // LCD 드라이버에 모든 상태 정보를 넘겨줍니다.
            update_lcd_display(data);

            // 자동 모드일 때만 팬과 부저를 제어합니다.
            if (data->mode == AUTOMATIC) {
                // 2. 팬 제어 로직
                // 조건: 온도가 27도 이상이거나 습도가 70% 이상인가?
                if (data->temperature >= TEMPERATURE_THRESHOLD || data->humidity >= HUMIDITY_THRESHOLD) {
                    if (!data->is_running) {
                        data->is_running = TRUE;
                        ventilation_on();
                        printf("[Auto Control] Fan ON (T:%.1f, H:%.1f)\n", data->temperature, data->humidity);
                    }
                } else { // 조건에 해당하지 않으면 팬을 끈다
                    if (data->is_running) {
                        data->is_running = FALSE;
                        ventilation_off();
                        printf("[Auto Control] Fan OFF (T:%.1f, H:%.1f)\n", data->temperature, data->humidity);
                    }
                }
                
                // 3. 부저 제어 로직
                // 조건: 온도가 30도 이상이고 습도가 80% 이상인가?
                if (data->temperature >= WARNING_TEMP_THRESHOLD && data->humidity >= WARNING_HUMI_THRESHOLD) {
                     if (!data->is_alert_active) {
                        data->is_alert_active = TRUE;
                        buzzer_on();
                    }
                } else {
                    if (data->is_alert_active) {
                        data->is_alert_active = FALSE;
                        buzzer_off();
                    }
                }
            } else { // 수동 모드일 때는 자동 제어 로직을 건너뜁니다.
                // 수동 모드로 전환될 때 경고 상태도 꺼주는 것이 좋습니다.
                if (data->is_alert_active) {
                    data->is_alert_active = FALSE;
                    buzzer_off();
                }
            }
            
            // 처리 완료 후 플래그 리셋
            data->new_data_available = FALSE;
        }
        g_mutex_unlock(&data->mutex);

        // GUI 업데이트 요청
        g_idle_add(update_gui_callback, data);

        // ※ 참고: DHTXXD_auto_read를 사용하므로, 
        // 이 스레드의 sleep 주기는 제어 로직의 반응 속도만 결정합니다.
        // 1초 정도로 유지하는 것이 좋습니다.
    }
    return NULL;
}
