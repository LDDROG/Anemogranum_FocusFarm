#include "game.h"
#include <cstdio>
#include <cstdlib>

// 天气刷新功能已在 game.cpp 的 Game::refreshWeather() 中实现
// 本文件保留用于未来扩展：
// - 直接使用 WinHTTP API 获取天气（无需 Python 依赖）
// - 使用 Windows Location API 获取设备位置
// - 备用天气 API 源

// 当前实现通过调用 Python 脚本 get_weather.py 从 wttr.in 获取天气信息