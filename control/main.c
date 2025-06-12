#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include "gui.h"
#include "control_logic.h"
#include "dht11_driver.h"
#include "motor_driver.h"
#include "buzzer_driver.h"

// 프로그램 종료 시 리소스 정리를 위해 필요한 전역 포인터
static SharedData *g_main_shared_data_for_cleanup = NULL;
// GTK 애플리케이션 객체에 접근하기 위한 전역 포인터
static GtkApplication *g_app = NULL;

// 모든 하드웨어 및 소프트웨어 리소스를 정리하는 함수
void cleanup_all_resources() {
    printf("\n[Cleanup] Cleaning up all resources...\n");

    // 1. DHT 센서 리소스 정리
    dht11_cleanup();
    printf("[Cleanup] DHT sensor resources cleaned up.\n");

    // 2. pigpio 및 GPIO 리소스 정리 (이 함수에서 팬을 끕니다)
    cleanup_pigpio();
    printf("[Cleanup] pigpio and GPIO resources cleaned up.\n");

    // 3. 뮤텍스 정리
    if (g_main_shared_data_for_cleanup) {
        g_mutex_clear(&g_main_shared_data_for_cleanup->mutex);
        printf("[Cleanup] Mutex cleared.\n");
    }
    printf("[Cleanup] Cleanup finished.\n");
}

// Ctrl+C (SIGINT) 또는 종료 신호(SIGTERM)를 처리하는 핸들러
void handle_exit_signals(int signum) {
    printf("\n[Signal] Caught signal %d. Initiating graceful shutdown...\n", signum);

    // GTK 애플리케이션이 실행 중이라면 종료하도록 요청
    if (g_app) {
        g_application_quit(G_APPLICATION(g_app));
    } else {
        // GTK가 시작되기 전에 문제가 발생한 경우 직접 정리하고 종료
        cleanup_all_resources();
        exit(signum);
    }
}

int main(int argc, char *argv[]) {
    // 1. 종료 시그널 핸들러 등록
    signal(SIGINT, handle_exit_signals);
    signal(SIGTERM, handle_exit_signals);

    printf("[Main] Initializing hardware...\n");
    // 2. 하드웨어 초기화 (pigpio 연결 및 GPIO 설정)
    if (init_pigpio() < 0) return 1;
    if (setup_gpio() != 0) {
        cleanup_pigpio();
        return 1;
    }// --- 여기에 버저 초기화 호출을 추가합니다 ---
    if (buzzer_init() != 0) {
        cleanup_pigpio();
        return 1;
    }
    if (buzzer_init() != 0) {
        cleanup_pigpio();
        return 1;
    }

    printf("[Main] Hardware initialized successfully.\n");

    // 3. GTK 애플리케이션 생성
    g_app = gtk_application_new("com.rpi.smartvent", G_APPLICATION_FLAGS_NONE);

    // 4. 스레드 간 공유 데이터 및 GUI 위젯 구조체 생성
    SharedData shared_data;
    GuiWidgets widgets;
    g_main_shared_data_for_cleanup = &shared_data; // 정리 함수에서 사용할 수 있도록 전역 포인터에 할당

    // 공유 데이터 초기화
    shared_data.temperature = 0.0f;
    shared_data.humidity = 0.0f;
    shared_data.mode = AUTOMATIC;
    shared_data.is_running = FALSE;
    shared_data.new_data_available = FALSE;
    shared_data.widgets = &widgets;
    g_mutex_init(&shared_data.mutex);
    printf("[Main] Shared data initialized.\n");

    // 5. DHT 센서 초기화
    if (dht11_init(&shared_data) != 0) {
        cleanup_pigpio();
        return 1;
    }
    printf("[Main] DHT sensor initialized.\n");

    // 6. 제어 로직을 수행할 백그라운드 스레드 생성
    pthread_t worker_thread;
    if (pthread_create(&worker_thread, NULL, worker_thread_func, &shared_data) != 0) {
        perror("[Error] pthread_create failed");
        cleanup_all_resources();
        return -1;
    }
    printf("[Main] Worker thread created.\n");

    // 7. GTK 'activate' 시그널에 GUI 생성 함수 연결
    g_signal_connect(g_app, "activate", G_CALLBACK(create_gui), &shared_data);

    printf("[Main] Starting GTK application. GUI window will open now.\n");
    // 8. GTK 애플리케이션 실행 (GUI 창이 닫힐 때까지 이 함수에서 대기)
    int status = g_application_run(G_APPLICATION(g_app), argc, argv);
    
    // --- 이하는 GUI 창이 닫힌 후 실행되는 코드 ---
    printf("\n[Main] GTK application has been closed. Starting final cleanup routine...\n");
    
    // 9. 워커 스레드 종료
    pthread_cancel(worker_thread);
    pthread_join(worker_thread, NULL);
    printf("[Main] Worker thread terminated.\n");
    
    // 10. GTK 객체 참조 해제
    g_object_unref(g_app);
    printf("[Main] GTK application object unreferenced.\n");

    // 11. 모든 리소스 정리 (가장 중요)
    cleanup_all_resources();

    printf("[Main] Program finished with status %d. Exiting.\n", status);
    return status;
}