import os
import json
import traceback
from flask import Flask, render_template_string, jsonify

FIFO_PATH = "/tmp/smart_vent_fifo"
STATUS_FILE_PATH = "/tmp/smart_vent_status.json"

app = Flask(__name__)

# HTML 템플릿
HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Smart Ventilation Remote</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; background-color: #f4f7f9; margin: 0; }
        .container { width: 90%; max-width: 420px; background: white; border-radius: 12px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); padding: 25px; text-align: center; }
        h1 { font-size: 1.8em; color: #333; margin-bottom: 25px; }
        .status-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-bottom: 25px; }
        .status-box { background-color: #f9f9f9; padding: 15px; border-radius: 8px; }
        .status-box h2 { font-size: 1em; margin: 0 0 5px 0; color: #555; text-transform: uppercase; }
        .status-box p { font-size: 1.5em; margin: 0; color: #007bff; font-weight: bold; }
        .status-box p#fan_status.off { color: #dc3545; }
        .btn-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }
        .btn { padding: 15px; font-size: 1.1em; color: white; border: none; border-radius: 8px; cursor: pointer; transition: background-color 0.2s; }
        .btn-on { background-color: #28a745; }
        .btn-off { background-color: #dc3545; }
        .btn-auto { background-color: #007bff; grid-column: 1 / -1; } /* Full width */
    </style>
</head>
<body>
    <div class="container">
        <h1>Smart Ventilation Control</h1>
        <div class="status-grid">
            <div class="status-box">
                <h2>Temperature</h2>
                <p><span id="temp_val">--</span> °C</p>
            </div>
            <div class="status-box">
                <h2>Humidity</h2>
                <p><span id="humi_val">--</span> %</p>
            </div>
            <div class="status-box">
                <h2>Mode</h2>
                <p id="mode_status">--</p>
            </div>
            <div class="status-box">
                <h2>Fan</h2>
                <p id="fan_status">--</p>
            </div>
        </div>
        <div class="btn-grid">
            <button class="btn btn-on" onclick="sendCommand('REMOTE_ON')">Manual ON</button>
            <button class="btn btn-off" onclick="sendCommand('REMOTE_OFF')">Manual OFF</button>
            <button class="btn btn-auto" onclick="sendCommand('REMOTE_AUTO')">Set to AUTO</button>
        </div>
    </div>
<script>
    function sendCommand(cmd) {
        fetch('/command/' + cmd, { method: 'POST' });
    }
    function updateStatus() {
        fetch('/status')
            .then(response => response.json())
            .then(data => {
                document.getElementById('temp_val').innerText = data.temperature.toFixed(1);
                document.getElementById('humi_val').innerText = data.humidity.toFixed(1);
                document.getElementById('mode_status').innerText = data.mode.toUpperCase();
                const fanStatus = document.getElementById('fan_status');
                fanStatus.innerText = data.fan_on ? 'ON' : 'OFF';
                fanStatus.className = data.fan_on ? 'on' : 'off';
            })
            .catch(error => console.error('Error fetching status:', error));
    }
    // 페이지 로드 시 즉시 상태 업데이트하고, 3초마다 반복
    document.addEventListener('DOMContentLoaded', function() {
        updateStatus();
        setInterval(updateStatus, 3000);
    });
</script>
</body>
</html>
"""

def write_to_fifo(command):
    print(f"[Flask Debug] Attempting to write '{command}' to FIFO...")
    try:
        # FIFO가 존재하는지 먼저 확인
        if not os.path.exists(FIFO_PATH):
            print(f"[Flask Error] FIFO path '{FIFO_PATH}' does not exist!")
            return f"Error: FIFO path does not exist.", 500
        
        # O_NONBLOCK을 추가하여 쓰기가 즉시 실패하는지 확인
        fd = os.open(FIFO_PATH, os.O_WRONLY | os.O_NONBLOCK)
        os.write(fd, command.encode())
        os.close(fd)
        print(f"[Flask Debug] Command '{command}' sent successfully.")
        return "OK"
    except Exception as e:
        # 어떤 종류의 에러가 발생했는지, 상세한 내용을 터미널에 출력
        print(f"[Flask Error] Failed to write to FIFO.")
        print(f"[Flask Error] Exception Type: {type(e).__name__}")
        print(f"[Flask Error] Exception Details: {e}")
        traceback.print_exc() # 전체 에러 스택을 출력
        return f"Error: {e}", 500

# C가 만든 상태 파일을 읽어서 JSON으로 반환하는 API
@app.route('/status')
def get_status():
    try:
        with open(STATUS_FILE_PATH, 'r') as f:
            data = json.load(f)
        return jsonify(data)
    except (FileNotFoundError, json.JSONDecodeError):
        # 파일이 없거나 내용이 비어있을 때 기본값 반환
        return jsonify({"temperature": 0, "humidity": 0, "fan_on": False, "mode": "unknown"})

@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)

@app.route('/command/<string:cmd>', methods=['POST'])
def command(cmd):
    if cmd in ["REMOTE_ON", "REMOTE_OFF", "REMOTE_AUTO"]:
        return write_to_fifo(cmd)
    return "Invalid command", 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)