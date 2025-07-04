#include "gui.h"
#include "motor_driver.h"

static SharedData *g_shared_data = NULL;

// "수동 환기 시작" 버튼 콜백
static void on_manual_on_clicked(GtkButton *button, gpointer user_data) {
    g_mutex_lock(&g_shared_data->mutex);
    if (g_shared_data->mode == MANUAL) {
        g_shared_data->is_running = TRUE;
        ventilation_on();
        gtk_label_set_text(GTK_LABEL(g_shared_data->widgets->lbl_status), "Fan ON (Manual)");
    }
    g_mutex_unlock(&g_shared_data->mutex);
}

// "수동 환기 정지" 버튼 콜백
static void on_manual_off_clicked(GtkButton *button, gpointer user_data) {
    g_mutex_lock(&g_shared_data->mutex);
    if (g_shared_data->mode == MANUAL) {
        g_shared_data->is_running = FALSE;
        ventilation_off();
        gtk_label_set_text(GTK_LABEL(g_shared_data->widgets->lbl_status), "Fan OFF (Manual)");
    }
    g_mutex_unlock(&g_shared_data->mutex);
}

// "자동/수동" 스위치 콜백
void on_mode_switch_state_set(GtkSwitch *sw, gboolean state, gpointer user_data) {
    SharedData *data = (SharedData*)user_data;

    g_mutex_lock(&data->mutex);
    if (state) { // TRUE: 수동 모드
        data->mode = MANUAL;
        gtk_widget_set_sensitive(data->widgets->btn_manual_on, TRUE);
        gtk_widget_set_sensitive(data->widgets->btn_manual_off, TRUE);
        if (data->is_running) {
            data->is_running = FALSE;
            ventilation_off();
        }
        gtk_label_set_text(GTK_LABEL(data->widgets->lbl_status), "Fan OFF (Manual)");
    } else { // FALSE: 자동 모드
        data->mode = AUTOMATIC;
        gtk_widget_set_sensitive(data->widgets->btn_manual_on, FALSE);
        gtk_widget_set_sensitive(data->widgets->btn_manual_off, FALSE);
    }
    g_mutex_unlock(&data->mutex);
}

// GUI를 생성하고 표시하는 메인 함수
void create_gui(GtkApplication *app, gpointer user_data) {
    // 전역 포인터와 지역 포인터를 모두 설정
    g_shared_data = (SharedData*)user_data;
    SharedData *data = (SharedData*)user_data;
    GuiWidgets *widgets = data->widgets;

    widgets->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(widgets->window), "Smart Ventilation System");
    gtk_window_set_default_size(GTK_WINDOW(widgets->window), 480, 320);
    gtk_container_set_border_width(GTK_CONTAINER(widgets->window), 20);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 15);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 15);
    gtk_container_add(GTK_CONTAINER(widgets->window), grid);

    GtkWidget *lbl_title = gtk_label_new("Smart Ventilation System");
    PangoAttribute *attr = pango_attr_size_new(20 * PANGO_SCALE);
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, attr);
    gtk_label_set_attributes(GTK_LABEL(lbl_title), attrs);
    pango_attr_list_unref(attrs);

    widgets->lbl_temp = gtk_label_new("temperature: --.- °C");
    widgets->lbl_humidity = gtk_label_new("humidity: --.- %");
    widgets->lbl_status = gtk_label_new("status: reseting...");
    widgets->switch_mode = gtk_switch_new();
    GtkWidget *lbl_mode_auto = gtk_label_new("auto");
    GtkWidget *lbl_mode_manual = gtk_label_new("manual");
    widgets->btn_manual_on = gtk_button_new_with_label("manual ventilation start");
    widgets->btn_manual_off = gtk_button_new_with_label("manual ventilation stop");

    gtk_grid_attach(GTK_GRID(grid), lbl_title, 0, 0, 4, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->lbl_temp, 0, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->lbl_humidity, 2, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->lbl_status, 0, 2, 4, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_mode_auto, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->switch_mode, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_mode_manual, 2, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->btn_manual_on, 0, 4, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->btn_manual_off, 2, 4, 2, 1);

    // 모든 시그널 연결 시 세 번째 인자로 NULL 대신 'data' 포인터를 전달
    g_signal_connect(widgets->switch_mode, "state-set", G_CALLBACK(on_mode_switch_state_set), data);
    g_signal_connect(widgets->btn_manual_on, "clicked", G_CALLBACK(on_manual_on_clicked), data);
    g_signal_connect(widgets->btn_manual_off, "clicked", G_CALLBACK(on_manual_off_clicked), data);

    gtk_switch_set_active(GTK_SWITCH(widgets->switch_mode), FALSE);
    gtk_widget_set_sensitive(widgets->btn_manual_on, FALSE);
    gtk_widget_set_sensitive(widgets->btn_manual_off, FALSE);

    gtk_widget_show_all(widgets->window);
}