#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>

// GUI 위젯들의 포인터를 담을 구조체
typedef struct {
    GtkWidget *window;
    GtkWidget *lbl_temp;
    GtkWidget *lbl_humidity;
    GtkWidget *lbl_status;
    GtkWidget *switch_mode;
    GtkWidget *btn_manual_on;
    GtkWidget *btn_manual_off;
} GuiWidgets;

// 시스템의 작동 모드
typedef enum { AUTOMATIC, MANUAL } SystemMode;

// 스레드 간에 공유될 데이터 구조체
typedef struct {
    float temperature;
    float humidity;
    SystemMode mode;
    gboolean is_running;          // 팬 작동 여부
    gboolean is_alert_active;     // 경고 활성화 상태
    gboolean new_data_available;  // 새 센서 데이터 수신 플래그
    GuiWidgets *widgets;          // GUI 위젯 포인터
    GMutex mutex;                 // 데이터 보호를 위한 뮤텍스
} SharedData;

// GUI 생성 함수 프로토타입
void create_gui(GtkApplication *app, gpointer user_data);

// gui.c에 정의된 콜백 함수를 다른 파일에서도 알 수 있도록 공개적으로 선언
void on_mode_switch_state_set(GtkSwitch *sw, gboolean state, gpointer user_data);

#endif