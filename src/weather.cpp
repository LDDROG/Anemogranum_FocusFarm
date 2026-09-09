// LDD_ROG 2026.9.8

#include "weather.h"
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>

static constexpr int WEATHER_REFRESH_SEC = 900;
static WeatherInfo g_weather;
static std::mutex g_mutex;
static std::atomic<bool> g_running{false};
static std::thread g_thread;

struct PipeCloser {
    FILE* p = nullptr;
    ~PipeCloser() { if (p) _pclose(p); }
    PipeCloser(const PipeCloser&) = delete;
    PipeCloser& operator=(const PipeCloser&) = delete;
};

static WeatherInfo fetchOnce() {
    WeatherInfo w;

    _putenv("PYTHONIOENCODING=utf-8");

    std::string cmd = "python scripts/get_weather.py 2>&1";
    PipeCloser pipe{_popen(cmd.c_str(), "r")};
    if (!pipe.p) {
        w.locationAvailable = false;
        w.city = "无法获取";
        w.weather = "没安装Python？";
        return w;
    }

    char buffer[4096];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe.p)) {
        result += buffer;
    }

    auto findValue = [&](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\": \"";
        size_t pos = result.find(search);
        if (pos == std::string::npos) {
            search = "\"" + key + "\":\"";
            pos = result.find(search);
        }
        if (pos != std::string::npos) {
            pos += search.length();
            size_t end = result.find("\"", pos);
            if (end != std::string::npos) {
                return result.substr(pos, end - pos);
            }
        }
        return "N/A";
    };

    if (result.find("\"error\"") != std::string::npos) {
        w.locationAvailable = false;
        w.city = "获取失败";
        w.weather = "请检查网络或Windows定位设置";
    } else if (result.empty()) {
        w.locationAvailable = false;
        w.city = "无响应";
        w.weather = "请检查Python和网络";
    } else {
        w.city = findValue("city");
        w.country = findValue("country");
        w.weather = findValue("weather");
        w.temp = findValue("temp_c");
        w.humidity = findValue("humidity");
        w.wind = findValue("wind_speed");
        w.locationAvailable = true;
    }
    return w;
}

WeatherInfo weather() {
    std::lock_guard<std::mutex> lk(g_mutex);
    return g_weather;
}

void startWeatherService() {
    if (g_running.exchange(true)) return;
    g_thread = std::thread([]() {
        while (g_running) {
            try {
                WeatherInfo w = fetchOnce();
                {
                    std::lock_guard<std::mutex> lk(g_mutex);
                    g_weather = w;
                }
            } catch (...) {}   // 失败时保留上一次的天气数据
            for (int i = 0; i < WEATHER_REFRESH_SEC && g_running; i++) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    });
}

void stopWeatherService() 
{
    g_running = false;
    if (g_thread.joinable()) g_thread.join();
}