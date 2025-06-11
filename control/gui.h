#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>

// ... (GuiWidgets 구조체는 이전과 동일) ...
typedef struct {
    GtkWidget *window;
    GtkWidget *lbl_temp;
    GtkWidget *lbl_humidity;
    GtkWidget *lbl_status;
    GtkWidget *switch_mode;
    GtkWidget *btn_manual_on;
    GtkWidget *btn_manual_off;
} GuiWidgets;


// 공유 데이터 구조체
typedef enum { AUTOMATIC, MANUAL } SystemMode;

typedef struct {
    float temperature;
    float humidity;
    SystemMode mode;
    gboolean is_running;
    gboolean new_data_available; // 워커 스레드용 플래그
    GuiWidgets *widgets;
    GMutex mutex;
} SharedData;

void create_gui(GtkApplication *app, gpointer user_data);

#endif