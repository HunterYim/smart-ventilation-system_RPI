#include "control_logic.h"
#include "gui.h"
#include "dht11_driver.h"
#include "motor_driver.h"
#include "lcd_driver.h"
#include "buzzer_driver.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// 새롭게 정의된 임계값
#define TEMPERATURE_THRESHOLD 27.0f
#define HUMIDITY_THRESHOLD    70.0f

// LCD 및 버저 경고 임계값
#define WARNING_TEMP_THRESHOLD 27.0f
#define WARNING_HUMI_THRESHOLD 70.0f

#define READ_INTERVAL_SECONDS 3

// FIFO 경로 정의
#define FIFO_PATH "/tmp/smart_vent_fifo"

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
    int fifo_fd;
    char command_buf[64];

    // 스레드 시작 시 FIFO 생성
    mkfifo(FIFO_PATH, 0666);

    while (1) {
        // 1. 원격 제어 명령 확인
        // FIFO를 Non-blocking 모드로 열기
        fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
        if (fifo_fd != -1) {
            int bytes_read = read(fifo_fd, command_buf, sizeof(command_buf) - 1);
            if (bytes_read > 0) {
                command_buf[bytes_read] = '\0'; // 문자열 끝 처리
                printf("[Remote] Command received: %s\n", command_buf);

                g_mutex_lock(&data->mutex);
                // 수동 모드로 강제 전환 후 팬 켜기
                if (strncmp(command_buf, "REMOTE_ON", 9) == 0) {
                    data->mode = MANUAL;
                    data->is_running = TRUE;
                    ventilation_on();
                    gtk_label_set_text(GTK_LABEL(data->widgets->lbl_status), "Fan ON (Remote)");
                    gtk_switch_set_active(GTK_SWITCH(data->widgets->switch_mode), TRUE);
                } 
                // 수동 모드로 강제 전환 후 팬 끄기
                else if (strncmp(command_buf, "REMOTE_OFF", 10) == 0) {
                    data->mode = MANUAL;
                    data->is_running = FALSE;
                    ventilation_off();
                    gtk_label_set_text(GTK_LABEL(data->widgets->lbl_status), "Fan OFF (Remote)");
                    gtk_switch_set_active(GTK_SWITCH(data->widgets->switch_mode), TRUE);
                }
                // 자동 모드로 전환
                else if (strncmp(command_buf, "REMOTE_AUTO", 11) == 0) {
                    data->mode = AUTOMATIC;
                    gtk_label_set_text(GTK_LABEL(data->widgets->lbl_status), "Mode set to Auto");
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
            data->new_data_available = FALSE;
        }
        g_mutex_unlock(&data->mutex);

        g_idle_add(update_gui_callback, data);

        sleep(READ_INTERVAL_SECONDS - 1);
    }
    return NULL;
}