#ifndef WEATHER_H
#define WEATHER_H

#include <string>
struct WeatherInfo {
    std::string city = "Unknown";
    std::string country = "Unknown";
    std::string weather = "Unknown";
    std::string temp = "N/A";
    std::string humidity = "N/A";
    std::string wind = "N/A";
    bool locationAvailable = true;
};
void startWeatherService();
void stopWeatherService();
WeatherInfo weather();

#endif