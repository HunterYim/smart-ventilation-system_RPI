CC = gcc
TARGET = smart_ventilation

# 소스 파일 목록에 DHTXXD.c 추가
SRCS = main.c gui.c control_logic.c dht11_driver.c motor_driver.c DHTXXD.c
OBJS = $(SRCS:.c=.o)

# 컴파일러 플래그 및 라이브러리 설정
# pigpiod_if2 라이브러리를 링크에 추가
CFLAGS = -Wall `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0` -lpthread -lpigpiod_if2 -lrt

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)