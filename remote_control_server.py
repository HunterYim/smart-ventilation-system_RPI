import os
from flask import Flask, render_template_string, request

# C 코드에서 정의한 FIFO 경로와 동일해야 함
FIFO_PATH = "/tmp/smart_vent_fifo"

app = Flask(__name__)

# 웹 페이지의 HTML 코드
HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>Smart Ventilation Remote</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; }
        .container { max-width: 400px; margin: 50px auto; padding: 20px; background: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1 { color: #333; }
        .btn { display: block; width: 80%; padding: 20px; margin: 20px auto; font-size: 1.2em; color: white; border: none; border-radius: 5px; cursor: pointer; }
        .btn-on { background-color: #28a745; }
        .btn-off { background-color: #dc3545; }
        .btn-auto { background-color: #007bff; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Smart Ventilation Control</h1>
        <button class="btn btn-on" onclick="sendCommand('REMOTE_ON')">Turn Fan ON</button>
        <button class="btn btn-off" onclick="sendCommand('REMOTE_OFF')">Turn Fan OFF</button>
        <button class="btn btn-auto" onclick="sendCommand('REMOTE_AUTO')">Set to AUTO Mode</button>
    </div>

    <script>
        function sendCommand(cmd) {
            fetch('/command/' + cmd, { method: 'POST' })
                .then(response => response.text())
                .then(data => console.log(data))
                .catch(error => console.error('Error:', error));
        }
    </script>
</body>
</html>
"""

# FIFO에 명령을 쓰는 함수
def write_to_fifo(command):
    try:
        # 'w' 모드로 열기 위해 os 모듈 사용
        fd = os.open(FIFO_PATH, os.O_WRONLY)
        os.write(fd, command.encode())
        os.close(fd)
        return f"Command '{command}' sent successfully."
    except Exception as e:
        return f"Error sending command: {e}"

# 루트 URL 접속 시 HTML 페이지를 보여줌
@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)

# 버튼 클릭 시 명령을 처리하는 URL
@app.route('/command/<string:cmd>', methods=['POST'])
def command(cmd):
    valid_commands = ["REMOTE_ON", "REMOTE_OFF", "REMOTE_AUTO"]
    if cmd in valid_commands:
        return write_to_fifo(cmd)
    else:
        return "Invalid command", 400

if __name__ == '__main__':
    # 외부에서 접속 가능하도록 host='0.0.0.0'으로 설정
    app.run(host='0.0.0.0', port=5000)