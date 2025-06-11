#include <pthread.h>
#include <signal.h>
#include "gui.h"
#include "control_logic.h"
#include "dht11_driver.h"
#include "motor_driver.h"

// 리소스 정리를 위한 전역 포인터
static SharedData *g_main_shared_data = NULL;

void cleanup_resources() {
    printf("\nCleaning up resources...\n");
    dht11_cleanup();
    cleanup_pigpio(); // 이 함수 안에서 팬 off 및 pigpio stop이 호출됨
    if (g_main_shared_data) {
        g_mutex_clear(&g_main_shared_data->mutex);
    }
}

void handle_exit_signals(int signum) {
    cleanup_resources();
    exit(signum);
}

int main(int argc, char *argv[]) {
    // 종료 시그널 핸들러 등록
    signal(SIGINT, handle_exit_signals);
    signal(SIGTERM, handle_exit_signals);

    // 1. pigpio 초기화
    if (init_pigpio() < 0) {
        return 1;
    }

    // 2. GPIO 핀 설정
    if (setup_gpio() != 0) {
        cleanup_pigpio();
        return 1;
    }

    // GTK 애플리케이션 생성
    GtkApplication *app = gtk_application_new("com.example.smartvent.pigpio", G_APPLICATION_FLAGS_NONE);

    // 공유 데이터 및 위젯 구조체 생성
    SharedData shared_data;
    GuiWidgets widgets;
    g_main_shared_data = &shared_data; // 전역 포인터에 할당

    // 공유 데이터 초기화
    shared_data.temperature = -999.0f;
    shared_data.humidity = -999.0f;
    shared_data.mode = AUTOMATIC;
    shared_data.is_running = FALSE;
    shared_data.new_data_available = FALSE;
    shared_data.widgets = &widgets;
    g_mutex_init(&shared_data.mutex);

    // 3. DHT 센서 초기화 (공유 데이터 주소 전달)
    if (dht11_init(&shared_data) != 0) {
        cleanup_pigpio();
        return 1;
    }

    // 4. 워커 스레드 생성
    pthread_t worker_thread;
    if (pthread_create(&worker_thread, NULL, worker_thread_func, &shared_data) != 0) {
        perror("pthread_create failed");
        cleanup_resources();
        return -1;
    }
    
    // 5. GTK GUI 시작
    g_signal_connect(app, "activate", G_CALLBACK(create_gui), &shared_data);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    
    // 6. 애플리케이션 종료 및 리소스 정리
    pthread_cancel(worker_thread); // 스레드 종료
    pthread_join(worker_thread, NULL);
    g_object_unref(app);
    cleanup_resources(); // 모든 리소스 정리

    return status;
}