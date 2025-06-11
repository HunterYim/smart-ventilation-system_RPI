#include <pthread.h>
#include <signal.h>
#include "gui.h"
#include "control_logic.h"
#include "dht11_driver.h"
#include "motor_driver.h"

static SharedData *g_main_shared_data_for_cleanup = NULL;

void cleanup_all_resources() {
    printf("\nCleaning up all resources...\n");
    dht11_cleanup();
    cleanup_pigpio();
    if (g_main_shared_data_for_cleanup) {
        g_mutex_clear(&g_main_shared_data_for_cleanup->mutex);
    }
}

void handle_exit_signals(int signum) {
    cleanup_all_resources();
    exit(signum);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_exit_signals);
    signal(SIGTERM, handle_exit_signals);

    if (init_pigpio() < 0) return 1;
    if (setup_gpio() != 0) {
        cleanup_pigpio();
        return 1;
    }

    GtkApplication *app = gtk_application_new("com.rpi.smartvent", G_APPLICATION_FLAGS_NONE);

    SharedData shared_data;
    GuiWidgets widgets;
    g_main_shared_data_for_cleanup = &shared_data;

    shared_data.temperature = 0.0f;
    shared_data.humidity = 0.0f;
    shared_data.mode = AUTOMATIC;
    shared_data.is_running = FALSE;
    shared_data.new_data_available = FALSE;
    shared_data.widgets = &widgets;
    g_mutex_init(&shared_data.mutex);

    if (dht11_init(&shared_data) != 0) {
        cleanup_pigpio();
        return 1;
    }

    pthread_t worker_thread;
    if (pthread_create(&worker_thread, NULL, worker_thread_func, &shared_data) != 0) {
        perror("pthread_create failed");
        cleanup_all_resources();
        return -1;
    }

    g_signal_connect(app, "activate", G_CALLBACK(create_gui), &shared_data);
    int status = g_application_run(G_APPLICATION(app), argc, argv);

    pthread_cancel(worker_thread);
    pthread_join(worker_thread, NULL);
    g_object_unref(app);
    cleanup_all_resources();

    return status;
}