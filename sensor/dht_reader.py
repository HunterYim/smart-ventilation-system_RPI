import Adafruit_DHT
import time

sensor = Adafruit_DHT.DHT11
gpio_pin = 24   #EXT_GPIO0 = GPIO24

def read_dht11():
    humidity, temperature = Adafruit_DHT.read_retry(sensor, gpio_pin)
    if humidity is not None and temperature is not None:
        return temperature, humidity
    else:
        raise RuntimeError("RuntimeError")

if __name__ == "__main__":
    try:
        while True:
            temp, humi = read_dht11()
            print(f"🌡 temperature: {temp:.1f}°C  💧 humidity: {humi:.1f}%")
            time.sleep(2)
    except KeyboardInterrupt:
        print("finish")
