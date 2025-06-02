import Adafruit_DHT
import time

sensor = Adafruit_DHT.DHT11
gpio_pin = 17

def read_dht11():
    humidity, temperature = Adafruit_DHT.read_retry(sensor, gpio_pin)
    if humidity is not None and temperature is not None:
        return temperature, humidity
    else:
        raise RuntimeError("센서 값을 읽을 수 없습니다.")

if __name__ == "__main__":
    try:
        while True:
            temp, humi = read_dht11()
            print(f"🌡 온도: {temp:.1f}°C  💧 습도: {humi:.1f}%")
            time.sleep(2)
    except KeyboardInterrupt:
        print("종료됨.")
