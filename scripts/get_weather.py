# LDD_ROG 2026.9.28

import sys
import os
import json
import urllib.error
import urllib.request
import urllib.parse

# UApiPro, consumes credits with 2/time, accepts your API changes
API_URL = "https://uapis.cn/api/v1/misc/weather"

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(BASE_DIR)
CITY_FILE = os.path.join(PROJECT_DIR, "focusfarm_weather_city.txt")


def load_city():
    try:
        with open(CITY_FILE, "r", encoding="utf-8") as f:
            return f.read().strip()
    except Exception:
        return ""


def fetch_weather(city):
    params = {"lang": "zh"}
    if city:
        params["city"] = city
    url = API_URL + "?" + urllib.parse.urlencode(params)
    req = urllib.request.Request(url, headers={"User-Agent": "FocusFarm/1.0"})
    with urllib.request.urlopen(req, timeout=10) as resp:
        return json.loads(resp.read().decode("utf-8"))


def is_match(query, data):
    q = (query or "").strip()
    if not q:
        return True
    fields = [data.get("city", ""), data.get("province", ""), data.get("district", "")]
    for raw in fields:
        f = str(raw or "")
        if not f:
            continue
        if q in f or f in q:
            return True
        for i in range(len(q)):
            for j in range(i + 2, len(q) + 1):
                if q[i:j] in f:
                    return True
    return False


def build_result(query, data):
    wind = (str(data.get("wind_direction", "")) + " " + str(data.get("wind_power", ""))).strip()
    return {
        "city": str(data.get("city", "未知")),
        "district": str(data.get("district", "")),
        "country": str(data.get("province", "未知")),
        "weather": str(data.get("weather", "未知")),
        "temp_c": str(data.get("temperature", "N/A")),
        "humidity": str(data.get("humidity", "N/A")),
        "wind_speed": wind or "N/A",
        "query": query or "",
        "matched": "1" if is_match(query, data) else "0",
    }


if __name__ == "__main__":
    city = load_city()
    try:
        print(json.dumps(build_result(city, fetch_weather(city)), ensure_ascii=False))
    except urllib.error.HTTPError as e:
        msg = "请求过于频繁，请稍后再试" if e.code == 429 else ("HTTP %s" % e.code)
        print(json.dumps({"error": msg}, ensure_ascii=False))
        sys.exit(1)
    except Exception as e:
        print(json.dumps({"error": str(e)}, ensure_ascii=False))
        sys.exit(1)
