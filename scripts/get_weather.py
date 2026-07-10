import sys
import json
import urllib.request
import urllib.parse
import urllib.error

def get_weather(city=""):
    try:
        if city:
            url = "https://wttr.in/{}?format=j1".format(urllib.parse.quote(city))
        else:
            url = "https://wttr.in/?format=j1"

        req = urllib.request.Request(url, headers={"User-Agent": "FocusFarm/1.0"})
        with urllib.request.urlopen(req, timeout=8) as resp:
            data = json.loads(resp.read().decode("utf-8"))

        current = data.get("current_condition", [{}])[0]
        nearest = data.get("nearest_area", [{}])[0]

        result = {
            "city": nearest.get("areaName", [{}])[0].get("value", "Unknown"),
            "country": nearest.get("country", [{}])[0].get("value", "Unknown"),
            "weather": current.get("weatherDesc", [{}])[0].get("value", "Unknown"),
            "temp_c": current.get("temp_C", "N/A"),
            "humidity": current.get("humidity", "N/A"),
            "wind_speed": current.get("windspeedKmph", "N/A"),
        }
        print(json.dumps(result, ensure_ascii=False))
    except Exception as e:
        print(json.dumps({"error": str(e)}, ensure_ascii=False))
        sys.exit(1)

if __name__ == "__main__":
    city = sys.argv[1] if len(sys.argv) > 1 else ""
    get_weather(city)