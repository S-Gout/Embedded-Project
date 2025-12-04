import os
import io
import wave
import requests
from flask import Flask, request

app = Flask(__name__)

OPENAI_API_KEY = os.getenv("OPENAI_API_KEY")
OPENAI_API_URL = "https://api.openai.com/v1/audio/transcriptions"
OPENAI_MODEL = "whisper-1"  # โมเดลแปลงเสียงเป็นข้อความ

if not OPENAI_API_KEY:
    print("WARNING: OPENAI_API_KEY is not set! Please set it in your environment.")

@app.route("/audio", methods=["POST"])
def audio():
    raw = request.data
    if not raw:
        print("No data received")
        return "none", 200, {"Content-Type": "text/plain"}

    print(f"Got {len(raw)} bytes from ESP32", flush=True)

    # 1) แปลง raw PCM 16-bit 16kHz -> wav ในหน่วยความจำ
    wav_bytes = io.BytesIO()
    with wave.open(wav_bytes, "wb") as wf:
        wf.setnchannels(1)       # mono
        wf.setsampwidth(2)       # 16-bit = 2 bytes
        wf.setframerate(16000)   # 16 kHz
        wf.writeframes(raw)

    wav_bytes.seek(0)

    # 2) เรียก OpenAI Audio API (Speech-to-Text)
    headers = {
        "Authorization": f"Bearer {OPENAI_API_KEY}",
    }

    files = {
        "file": ("audio.wav", wav_bytes, "audio/wav"),
    }

    data = {
        "model": OPENAI_MODEL,
        "language": "en",   # เราจะพูดคำสั่งภาษาอังกฤษ: turn on / turn off
    }

    try:
        print("Sending audio to OpenAI API...", flush=True)
        resp = requests.post(OPENAI_API_URL, headers=headers, files=files, data=data)
        resp.raise_for_status()
        result = resp.json()
    except Exception as e:
        print("Error calling OpenAI:", e, flush=True)
        return "none", 200, {"Content-Type": "text/plain"}

    text = result.get("text", "").lower().strip()
    print(f"Heard text: '{text}'", flush=True)

    # ทำให้ข้อความสะอาดขึ้นนิดหน่อย
    t = text.replace("-", " ").replace("_", " ")

    command = "none"

    wake_ok = ("hey" in t)

    if wake_ok and "open" in t:
        command = "on"
    elif wake_ok and "close" in t:
        command = "off"
    elif wake_ok and "clothes" in t:
        command = "off"

    print(f"Return command: {command}", flush=True)
    return command, 200, {"Content-Type": "text/plain"}

if __name__ == "__main__":
    print("=== SERVER API (Whisper STT) STARTED ===", flush=True)
    app.run(host="0.0.0.0", port=5000)