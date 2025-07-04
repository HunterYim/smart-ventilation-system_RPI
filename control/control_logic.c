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
#include <sys/select.h>

#define FIFO_PATH "/tmp/smart_vent_fifo" // Flask와 통신할 파이프 경로
#define STATUS_FILE_PATH "/tmp/smart_vent_status.json" // 웹 통신용 상태 파일

// 새롭게 정의된 임계값
#define TEMPERATURE_THRESHOLD 28.0f
#define HUMIDITY_THRESHOLD    70.0f

// LCD 및 버저 경고 임계값
#define WARNING_TEMP_THRESHOLD 28.0f
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
    if (!data || !data->widgets || !data->widgets->lbl_temp) {
        return G_SOURCE_REMOVE;
    }

    g_mutex_lock(&data->mutex);
    
    // 1. 라벨 업데이트
    char temp_str[32], hum_str[32];
    sprintf(temp_str, "Temperature: %.1f C", data->temperature);
    sprintf(hum_str, "Humidity: %.1f %%", data->humidity);
    gtk_label_set_text(GTK_LABEL(data->widgets->lbl_temp), temp_str);
    gtk_label_set_text(GTK_LABEL(data->widgets->lbl_humidity), hum_str);

    // 2. 상태 라벨 및 스위치 상태 업데이트
    GtkSwitch *mode_switch = GTK_SWITCH(data->widgets->switch_mode);
    gboolean is_manual_mode = (data->mode == MANUAL);

    if (data->mode == AUTOMATIC) {
        gtk_label_set_text(GTK_LABEL(data->widgets->lbl_status),
            data->is_running ? "Fan ON (Auto)" : "Fan OFF (Auto)");
    } else { // MANUAL
        gtk_label_set_text(GTK_LABEL(data->widgets->lbl_status),
            data->is_running ? "Fan ON (Manual)" : "Fan OFF (Manual)");
    }

    // 3. GUI 스위치의 현재 상태와 데이터의 모드 상태가 다를 때만 업데이트
    // 이렇게 하여 무한 시그널 루프를 방지하고, 원격 제어 상태를 GUI에 정확히 반영
    if (gtk_switch_get_active(mode_switch) != is_manual_mode) {
        // 콜백이 또 호출되는 것을 막기 위해 잠시 시그널 핸들러를 비활성화
        g_signal_handlers_block_by_func(mode_switch, G_CALLBACK(on_mode_switch_state_set), data);
        gtk_switch_set_active(mode_switch, is_manual_mode);
        // 다시 시그널 핸들러 활성화
        g_signal_handlers_unblock_by_func(mode_switch, G_CALLBACK(on_mode_switch_state_set), data);
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
    if (chmod(FIFO_PATH, 0777) == -1) {
        perror("[Error] chmod for FIFO failed");
    }

    printf("[Logic] Opening FIFO for reading. Waiting for writer...\n");
    fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1) {
        perror("[Error] Failed to open FIFO permanently");
        return NULL; // 스레드 종료
    }
    printf("[Logic] FIFO opened successfully.\n");

    while (1) {
        fd_set fds;
        struct timeval tv;
        int ret;

        FD_ZERO(&fds);
        FD_SET(fifo_fd, &fds);
        
        tv.tv_sec = 0;
        tv.tv_usec = 10000; // 0.01초만 기다림

        ret = select(fifo_fd + 1, &fds, NULL, NULL, &tv);
        
        if (ret > 0 && FD_ISSET(fifo_fd, &fds)) {
            int bytes_read = read(fifo_fd, command_buf, sizeof(command_buf) - 1);
            if (bytes_read > 0) {
                command_buf[bytes_read] = '\0';
                printf("[Remote] Command received: %s\n", command_buf);
                g_mutex_lock(&data->mutex);
                // 모든 gtk_switch_set_active 호출을 제거
                if (strncmp(command_buf, "REMOTE_ON", 9) == 0) {
                    data->mode = MANUAL;
                    data->is_running = TRUE;
                    ventilation_on();
                } else if (strncmp(command_buf, "REMOTE_OFF", 10) == 0) {
                    data->mode = MANUAL;
                    data->is_running = FALSE;
                    ventilation_off();
                } else if (strncmp(command_buf, "REMOTE_AUTO", 11) == 0) {
                    data->mode = AUTOMATIC;
                }
                g_mutex_unlock(&data->mutex);
            }
        }

        // // 원격 제어 명령 확인
        // fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
        // if (fifo_fd != -1) {
        //     int bytes_read = read(fifo_fd, command_buf, sizeof(command_buf) - 1);
        //     if (bytes_read > 0) {
        //         command_buf[bytes_read] = '\0';
        //         printf("[Remote] Command received: %s\n", command_buf);
        //         g_mutex_lock(&data->mutex);
        //         if (strncmp(command_buf, "REMOTE_ON", 9) == 0) {
        //             data->mode = MANUAL;
        //             data->is_running = TRUE;
        //             ventilation_on();
        //             gtk_switch_set_active(GTK_SWITCH(data->widgets->switch_mode), TRUE);
        //         } else if (strncmp(command_buf, "REMOTE_OFF", 10) == 0) {
        //             data->mode = MANUAL;
        //             data->is_running = FALSE;
        //             ventilation_off();
        //             gtk_switch_set_active(GTK_SWITCH(data->widgets->switch_mode), TRUE);
        //         } else if (strncmp(command_buf, "REMOTE_AUTO", 11) == 0) {
        //             data->mode = AUTOMATIC;
        //             gtk_switch_set_active(GTK_SWITCH(data->widgets->switch_mode), FALSE);
        //         }
        //         g_mutex_unlock(&data->mutex);
        //     }
        //     close(fifo_fd);
        // }

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
                bool fan_on_condition = (data->temperature >= TEMPERATURE_THRESHOLD || data->humidity >= HUMIDITY_THRESHOLD);
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
    close(fifo_fd);
    return NULL;
}