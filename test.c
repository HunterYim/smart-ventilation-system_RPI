#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>           // open()
#include <pigpiod_if2.h>     // DHTXXD 라이브러리
#include "DHTXXD.h"

void fatal(char *fmt, ...)
{
   char buf[128];
   va_list ap;

   va_start(ap, fmt);
   vsnprintf(buf, sizeof(buf), fmt, ap);
   va_end(ap);

   fprintf(stderr, "%s\n", buf);
   fflush(stderr);
   exit(EXIT_FAILURE);
}

void usage()
{
   fprintf(stderr, "\n" \
      "Usage: DHTXXD [OPTION] ...\n" \
      "   -g value, gpio, 0-31,                       default 4\n" \
      "   -i value, reading interval in seconds\n" \
      "             0=single reading,                 default 0\n" \
      "   -m value, model 0=auto, 1=DHT11, 2=other,   default auto\n" \
      "   -h string, host name,                       default NULL\n" \
      "   -p value, socket port, 1024-32000,          default 8888\n\n");
}

int optGPIO     = 24;
char *optHost   = NULL;
char *optPort   = NULL;
int optModel    = DHTAUTO;
int optInterval = 0;

// Text LCD에 온습도 문자열 출력
void write_to_lcd(float temp, float humi)
{
   int fd = open("/dev/fpga_text_lcd", O_WRONLY);
   if (fd < 0) {
      perror("open /dev/fpga_text_lcd");
      return;
   }

   char line1[17], line2[17], lcd_data[33] = {0};
   snprintf(line1, sizeof(line1), "Temp: %.1f C", temp);
   snprintf(line2, sizeof(line2), "Humi: %.1f %%", humi);
   snprintf(lcd_data, sizeof(lcd_data), "%-16s%-16s", line1, line2);

   write(fd, lcd_data, 32);
   close(fd);
}

// DHT 센서 값 읽을 때마다 호출됨
void cbf(DHTXXD_data_t r)
{
   if (r.status == 0) {
      printf("✅ Temp: %.1f C | Humi: %.1f %%\n", r.temperature, r.humidity);
      write_to_lcd(r.temperature, r.humidity);
   } else {
      printf("❌ Sensor read failed. status=%d\n", r.status);
   }
}

static uint64_t getNum(char *str, int *err)
{
   uint64_t val;
   char *endptr;

   *err = 0;
   val = strtoll(str, &endptr, 0);
   if (*endptr) {*err = 1; val = -1;}
   return val;
}

static void initOpts(int argc, char *argv[])
{
   int opt, err, i;

   while ((opt = getopt(argc, argv, "g:h:i:m:p:")) != -1)
   {
      switch (opt)
      {
         case 'g':
            i = getNum(optarg, &err);
            if ((i >= 0) && (i <= 31)) optGPIO = i;
            else fatal("invalid -g option (%d)", i);
            break;

         case 'h':
            optHost = malloc(sizeof(optarg)+1);
            if (optHost) strcpy(optHost, optarg);
            break;

         case 'i':
            i = getNum(optarg, &err);
            if ((i>=0) && (i<=86400)) optInterval = i;
            else fatal("invalid -i option (%d)", i);
            break;

         case 'm':
            i = getNum(optarg, &err);
            if ((i >= DHTAUTO) && (i <= DHTXX)) optModel = i;
            else fatal("invalid -m option (%d)", i);
            break;

         case 'p':
            optPort = malloc(sizeof(optarg)+1);
            if (optPort) strcpy(optPort, optarg);
            break;

        default:
           usage();
           exit(-1);
        }
    }
}

int main(int argc, char *argv[])
{
   int pi;
   DHTXXD_t *dht;

   initOpts(argc, argv);

   pi = pigpio_start(optHost, optPort);
   if (pi >= 0)
   {
      dht = DHTXXD(pi, optGPIO, optModel, cbf);

      if (optInterval)
      {
         DHTXXD_auto_read(dht, optInterval);
         while (1) time_sleep(60);
      }
      else
      {
         DHTXXD_manual_read(dht);
      }

      DHTXXD_cancel(dht);
      pigpio_stop(pi);
   }
   return 0;
}
