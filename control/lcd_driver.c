#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "lcd_driver.h" 

void lcd_display_update(float temp, float humi)
{
   // Text LCD 디바이스 파일을 염
   int fd = open("/dev/fpga_text_lcd", O_WRONLY);
   if (fd < 0) {
      // 파일 열기에 실패하면 오류 메시지를 출력하고 조용히 종료
      // 프로그램 전체를 중단시키지 않는 것이 중요
      perror("lcd_driver: open /dev/fpga_text_lcd failed");
      return;
   }

   char line1[17], line2[17], lcd_data[33] = {0};

   if (temp >= 28.0 || humi >= 70.0) {
      snprintf(line1, sizeof(line1), "FAN ON NOW!"); 
      snprintf(line2, sizeof(line2), "T:%.1fC H:%.0f%%", temp, humi);  
   } else {
      snprintf(line1, sizeof(line1), "Temp: %.1f C", temp);
      snprintf(line2, sizeof(line2), "Humi: %.1f %%", humi);
   }

   // 두 라인을 합쳐서 32바이트 데이터로 만듦
   snprintf(lcd_data, sizeof(lcd_data), "%-16s%-16s", line1, line2);

   // LCD 디바이스에 데이터를 씀
   write(fd, lcd_data, 32);

   // 파일을 닫음
   close(fd);
}
