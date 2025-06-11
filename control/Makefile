# 컴파일러
CC = gcc

# 실행 파일 이름
TARGET = smart_ventilation

# 컴파일할 소스 파일 목록
SRCS = main.c gui.c control_logic.c dht11_driver.c motor_driver.c DHTXXD.c

# 오브젝트 파일 (자동 생성)
OBJS = $(SRCS:.c=.o)

# 컴파일러 플래그 (GTK 라이브러리 경로 포함)
CFLAGS = -Wall `pkg-config --cflags gtk+-3.0`

# 링커 플래그 (필요한 라이브러리들 링크)
# GTK, pthread(스레드), pigpiod_if2(GPIO), rt(시간함수)
LIBS = `pkg-config --libs gtk+-3.0` -lpthread -lpigpiod_if2 -lrt

# 기본 빌드 룰
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 정리 룰
clean:
	rm -f $(OBJS) $(TARGET)