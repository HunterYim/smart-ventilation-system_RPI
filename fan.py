import RPi.GPIO as GPIO
import time

# --- 설정 값 ---
RELAY_PIN = 24        # 릴레이 제어에 사용할 GPIO 핀 번호 (BCM 모드 기준)
ON_TIME_SECONDS = 5    # 릴레이가 켜져 있는 시간 (초)
OFF_TIME_SECONDS = 10   # 릴레이가 꺼져 있는 시간 (초)

# 릴레이 트리거 방식 설정 (True: High Level Trigger, False: Low Level Trigger)
IS_HIGH_LEVEL_TRIGGER = True 

# --- GPIO 설정 ---
GPIO.setmode(GPIO.BCM)      # BCM 핀 번호 체계 사용
GPIO.setwarnings(False)     # 불필요한 경고 메시지 비활성화 (선택 사항)
GPIO.setup(RELAY_PIN, GPIO.OUT) # 릴레이 제어핀을 출력으로 설정

# --- 초기 릴레이 상태 설정 (꺼진 상태로 시작) ---
if IS_HIGH_LEVEL_TRIGGER:
    GPIO.output(RELAY_PIN, GPIO.LOW)  # High Level Trigger: LOW로 보내면 꺼짐
    print(f"OFF (GPIO {RELAY_PIN}'s pin LOW)")
else:
    GPIO.output(RELAY_PIN, GPIO.HIGH) # Low Level Trigger: HIGH로 보내면 꺼짐
    print(f"OFF (GPIO {RELAY_PIN}'s pin HIGH)")

time.sleep(1) # 잠시 대기

# --- 메인 루프 ---
try:
    print("test start.")
    while True:
        # 릴레이 켜기
        if IS_HIGH_LEVEL_TRIGGER:
            GPIO.output(RELAY_PIN, GPIO.HIGH)
            print(f"ON (GPIO {RELAY_PIN} pin HIGH)")
        else:
            GPIO.output(RELAY_PIN, GPIO.LOW)
            print(f"ON (GPIO {RELAY_PIN} pin LOW)")
        
        time.sleep(ON_TIME_SECONDS)

        # 릴레이 끄기
        if IS_HIGH_LEVEL_TRIGGER:
            GPIO.output(RELAY_PIN, GPIO.LOW)
            print(f"OFF (GPIO {RELAY_PIN} pin LOW)")
        else:
            GPIO.output(RELAY_PIN, GPIO.HIGH)
            print(f"OFF (GPIO {RELAY_PIN} pin HIGH)")

        time.sleep(OFF_TIME_SECONDS)

except KeyboardInterrupt:
    print("\n...")

finally:
    # 프로그램 종료 시 GPIO 정리
    print("GPIO...")
    GPIO.cleanup()
    print("GPIO ")
