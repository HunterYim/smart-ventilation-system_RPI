#include "control_logic.h"
#include "gui.h"
#include "dht11_driver.h"
#include "motor_driver.h"
#include "lcd_driver.h"
#include "buzzer_driver.h"
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define FIFO_PATH "/tmp/smart_vent_fifo" // Flask와 통신할 파이프 경로
#define STATUS_FILE_PATH "/tmp/smart_vent_status.json" // 웹 통신용 상태 파일

// 새롭게 정의된 임계값
#define TEMPERATURE_THRESHOLD 27.0f
#define HUMIDITY_THRESHOLD    70.0f

// LCD 및 버저 경고 임계값
#define WARNING_TEMP_THRESHOLD 27.0f
#define WARNING_HUMI_THRESHOLD 70.0f

#define READ_INTERVAL_SECONDS 3

// 현재 상태를 JSON 파일로 쓰는 함수
static void write_status_to_file(SharedData *data) {
    FILE *fp = fopen(STATUS_FILE_PATH, "w");
    if (fp == NULL) {
        perror("[Error] Failed to open status file");
        return;
    }
    // "auto" 또는 "manual" 문자열 결정
    const char* mode_str = (data->mode == AUTOMATIC) ? "auto" : "manual";
    // JSON 형식으로 파일에 씀
    fprintf(fp, "{\"temperature\": %.1f, \"humidity\": %.1f, \"fan_on\": %s, \"mode\": \"%s\"}",
            data->temperature,
            data->humidity,
            data->is_running ? "true" : "false",
            mode_str);
    fclose(fp);
}

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
    } else {
        gtk_label_set_text(GTK_LABEL(data->widgets->lbl_status),
            data->is_running ? "Fan ON (Manual)" : "Fan OFF (Manual)");
    }
    g_mutex_unlock(&data->mutex);
    return G_SOURCE_REMOVE;
}

// 백그라운드 워커 스레드
void* worker_thread_func(void* user_data) {
    SharedData *data = (SharedData*)user_data;
    int fifo_fd;
    char command_buf[64];

    // FIFO 파이프 생성 (모든 사용자가 쓸 수 있도록 0777 권한)
    if (mkfifo(FIFO_PATH, 0777) == -1 && errno != EEXIST) {
        perror("[Error] mkfifo failed");
    }

    while (1) {
        // 원격 제어 명령 확인
        fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
        if (fifo_fd != -1) {
            int bytes_read = read(fifo_fd, command_buf, sizeof(command_buf) - 1);
            if (bytes_read > 0) {
                command_buf[bytes_read] = '\0';
                printf("[Remote] Command received: %s\n", command_buf);
                g_mutex_lock(&data->mutex);
                if (strncmp(command_buf, "REMOTE_ON", 9) == 0) {
                    data->mode = MANUAL;
                    data->is_running = TRUE;
                    ventilation_on();
                    gtk_switch_set_active(GTK_SWITCH(data->widgets->switch_mode), TRUE);
                } else if (strncmp(command_buf, "REMOTE_OFF", 10) == 0) {
                    data->mode = MANUAL;
                    data->is_running = FALSE;
                    ventilation_off();
                    gtk_switch_set_active(GTK_SWITCH(data->widgets->switch_mode), TRUE);
                } else if (strncmp(command_buf, "REMOTE_AUTO", 11) == 0) {
                    data->mode = AUTOMATIC;
                    gtk_switch_set_active(GTK_SWITCH(data->widgets->switch_mode), FALSE);
                }
                g_mutex_unlock(&data->mutex);
            }
            close(fifo_fd);
        }

        dht11_trigger_read();
        sleep(1); 

        g_mutex_lock(&data->mutex);
        if (data->new_data_available) {
            // 디버그 메시지
            printf("[Debug Logic] New data processed -> Temp: %.1f C, Humi: %.1f %%\n", 
                   data->temperature, data->humidity);
            
            // 1. Text LCD 업데이트
            lcd_display_update(data->temperature, data->humidity);
            write_status_to_file(data);

            // 2. 버저 제어 로직
            // 현재 센서 값 기준으로 경고 상태인지 판단
            bool current_warning_state = (data->temperature >= WARNING_TEMP_THRESHOLD || data->humidity >= WARNING_HUMI_THRESHOLD);

            // 상태가 OFF에서 ON으로 바뀌는 '순간'을 감지
            // 현재는 경고 상태이지만, 직전까지는 경고 상태가 아니었을 때
            if (current_warning_state && !data->is_alert_active) {
                
                // 뮤텍스를 잠시 풀고 버저를 제어
                g_mutex_unlock(&data->mutex);
                
                printf("[Alert] Warning condition met. Sounding buzzer for 5 seconds...\n");
                buzzer_on();
                sleep(5); // 5초 동안 대기
                buzzer_off();
                printf("[Alert] Buzzer stopped.\n");

                // 다시 뮤텍스를 잠그고 루프를 계속 진행
                g_mutex_lock(&data->mutex);
            }

            // 다음 루프를 위해 현재 상태를 저장
            data->is_alert_active = current_warning_state;


            // 자동 팬 제어 (기존 로직)
            if (data->mode == AUTOMATIC) {
                // 팬 제어 로직은 경고 조건과 별개로 작동합니다.
                bool fan_on_condition = (data->temperature > TEMPERATURE_THRESHOLD || data->humidity > HUMIDITY_THRESHOLD);
                if (fan_on_condition) {
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