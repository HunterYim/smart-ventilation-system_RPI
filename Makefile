# 컴파일러
CC = gcc

# 실행 파일 이름
TARGET = smart_ventilation

# 소스 파일이 있는 디렉토리
SRC_DIR = control

# 빌드 결과물이 생성될 디렉토리 (옵션)
BUILD_DIR = build

# 소스 파일 목록
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/gui.c \
       $(SRC_DIR)/control_logic.c \
       $(SRC_DIR)/dht11_driver.c \
       $(SRC_DIR)/motor_driver.c \
       $(SRC_DIR)/DHTXXD.c \
       $(SRC_DIR)/lcd_driver.c \
       $(SRC_DIR)/buzzer_driver.c

# 오브젝트 파일 목록 (빌드 디렉토리에 생성되도록 설정)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# 컴파일러 플래그 (GTK 라이브러리 경로 포함)
# -I$(SRC_DIR) : 헤더 파일(.h)을 control 폴더 안에서 찾도록 경로 추가
CFLAGS = -Wall -I$(SRC_DIR) `pkg-config --cflags gtk+-3.0`

# 링커 플래그 (필요한 라이브러리들 링크)
LIBS = `pkg-config --libs gtk+-3.0` -lpthread -lpigpiod_if2 -lrt

# 기본 빌드 룰
all: $(BUILD_DIR) $(TARGET)

# 실행 파일 생성 룰
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

# 오브젝트 파일 생성 룰
# $@: 룰의 타겟 (e.g., build/main.o)
# $<: 룰의 첫 번째 의존성 파일 (e.g., control/main.c)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 빌드 디렉토리 생성
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 정리 룰
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
