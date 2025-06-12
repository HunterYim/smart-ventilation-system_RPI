#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "lcd_driver.h" // 방금 만든 헤더 파일을 포함

// 제공해주신 코드를 그대로 가져와 함수로 만듦
void lcd_display_update(float temp, float humi)
{
   // Text LCD 디바이스 파일을 엽니다.
   int fd = open("/dev/fpga_text_lcd", O_WRONLY);
   if (fd < 0) {
      // 파일 열기에 실패하면 오류 메시지를 출력하고 조용히 종료합니다.
      // 프로그램 전체를 중단시키지 않는 것이 중요합니다.
      perror("lcd_driver: open /dev/fpga_text_lcd failed");
      return;
   }

   char line1[17], line2[17], lcd_data[33] = {0};

   // 제공해주신 경고 조건 로직을 그대로 사용합니다.
   if (temp >= 24.0 || humi >= 50.0) {
      snprintf(line1, sizeof(line1), "Check Climate!"); // 첫 번째 라인
      snprintf(line2, sizeof(line2), "T:%.1fC H:%.0f%%", temp, humi);  // 두 번째 라인
   } else {
      snprintf(line1, sizeof(line1), "Temp: %.1f C", temp);
      snprintf(line2, sizeof(line2), "Humi: %.1f %%", humi);
   }

   // 두 라인을 합쳐서 32바이트 데이터로 만듭니다.
   snprintf(lcd_data, sizeof(lcd_data), "%-16s%-16s", line1, line2);

   // LCD 디바이스에 데이터를 씁니다.
   write(fd, lcd_data, 32);

   // 파일을 닫습니다.
   close(fd);
}