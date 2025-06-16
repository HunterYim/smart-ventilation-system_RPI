from flask import Flask, render_template
import RPi.GPIO as GPIO
import Adafruit_DHT
import atexit

app = Flask(__name__)

FAN_PIN = 26
DHT_PIN = 24

GPIO.setmode(GPIO.BCM)
GPIO.setup(FAN_PIN, GPIO.OUT)

def cleanup():
    GPIO.cleanup()

atexit.register(cleanup)

@app.route("/")
def index():
    humidity, temperature = Adafruit_DHT.read_retry(Adafruit_DHT.DHT11, DHT_PIN)
    return render_template("index.html", temperature=temperature, humidity=humidity)

@app.route("/fan/on")
def fan_on():
    GPIO.output(FAN_PIN, GPIO.HIGH)
    return "<h1>✅ 팬 켜짐</h1><a href='/'>돌아가기</a>"

@app.route("/fan/off")
def fan_off():
    GPIO.output(FAN_PIN, GPIO.LOW)
    return "<h1>🛑 팬 꺼짐</h1><a href='/'>돌아가기</a>"

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
