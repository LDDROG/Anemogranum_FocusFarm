# LDD_ROG 2026.9.8

import sys
import json
import urllib.request
import urllib.parse

# UApiPro, consumes credits with 2/time, accepts your API changes
API_URL = "https://uapis.cn/api/v1/misc/weather"

# allow definate position, NONE for automatic
DEFAULT_CITY = ""

def fetch_weather(city=None):
    params = {"lang": "zh"}
    city = city or DEFAULT_CITY
    if city:
        params["city"] = city
    url = API_URL + "?" + urllib.parse.urlencode(params)
    req = urllib.request.Request(url, headers={"User-Agent": "FocusFarm/1.0"})
    with urllib.request.urlopen(req, timeout=10) as resp:
        return json.loads(resp.read().decode("utf-8"))

if __name__ == "__main__":
    try:
        arg_city = sys.argv[1] if len(sys.argv) > 1 else None
        d = fetch_weather(arg_city)
        wind = (str(d.get("wind_direction", "")) + " " + str(d.get("wind_power", ""))).strip()
        result = {
            "city": str(d.get("city", "未知")),
            "country": str(d.get("province", "未知")),
            "weather": str(d.get("weather", "未知")),
            "temp_c": str(d.get("temperature", "N/A")),
            "humidity": str(d.get("humidity", "N/A")),
            "wind_speed": wind or "N/A",}
        print(json.dumps(result, ensure_ascii=False))
    except Exception as e:
        print(json.dumps({"error": str(e)}, ensure_ascii=False))
        sys.exit(1)