#include "game.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif

// ======================== 终端工具函数 ========================

void clearScreen() {
    // ANSI escape: move cursor to home (0,0) and clear to end of screen
    // Much faster than FillConsoleOutputCharacterA, eliminates flickering
    std::cout << "\033[H\033[J";
}

void hideCursor() {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
#else
    std::cout << "\033[?25l";
#endif
}

void showCursor() {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
#else
    std::cout << "\033[?25h";
#endif
}

std::string colorText(const std::string& text, int color) {
    return "\033[" + std::to_string(color) + "m" + text + "\033[0m";
}

std::string boldText(const std::string& text) {
    return "\033[1m" + text + "\033[0m";
}

// ======================== Emoji 映射 ========================

std::string getTreeEmoji(TreeType t) {
    switch (t) {
        case TreeType::PINE: return "\xf0\x9f\x8c\xb2";       // 🌲
        case TreeType::DECIDUOUS: return "\xf0\x9f\x8c\xb3";  // 🌳
        case TreeType::PALM: return "\xf0\x9f\x8c\xb4";       // 🌴
    }
    return "\xf0\x9f\x8c\xb2";
}

std::string getTreeEmoji(TreeType t, GrowthStage s) {
    switch (s) {
        case GrowthStage::SEEDLING: return "\xf0\x9f\x8c\xb1"; // 🌱
        case GrowthStage::GROWING: return "\xf0\x9f\x8d\x83";  // 🍃
        case GrowthStage::MATURE: return getTreeEmoji(t);
    }
    return "\xf0\x9f\x8c\xb1";
}

std::string getFishEmoji(FishType t) {
    switch (t) {
        case FishType::NORMAL: return "\xf0\x9f\x90\x9f";     // 🐟
        case FishType::TROPICAL: return "\xf0\x9f\x90\xa0";   // 🐠
        case FishType::PUFFER: return "\xf0\x9f\x90\xa1";     // 🐡
    }
    return "\xf0\x9f\x90\x9f";
}

std::string getFishEmoji(FishType t, GrowthStage s) {
    switch (s) {
        case GrowthStage::SEEDLING: return "\xf0\x9f\xab\xa7"; // 🫧
        case GrowthStage::GROWING: return "\xf0\x9f\xa6\x90";  // 🦐
        case GrowthStage::MATURE: return getFishEmoji(t);
    }
    return "\xf0\x9f\xab\xa7";
}

std::string getAnimalEmoji(AnimalType t) {
    switch (t) {
        case AnimalType::COW: return "\xf0\x9f\x90\x84";      // 🐄
        case AnimalType::SHEEP: return "\xf0\x9f\x90\x91";    // 🐑
        case AnimalType::PIG: return "\xf0\x9f\x90\x96";      // 🐖
        case AnimalType::CHICKEN: return "\xf0\x9f\x90\x93";  // 🐓
    }
    return "\xf0\x9f\x90\x84";
}

std::string getAnimalEmoji(AnimalType t, GrowthStage s) {
    switch (s) {
        case GrowthStage::SEEDLING: return "\xf0\x9f\xa5\x9a"; // 🥚
        case GrowthStage::GROWING: return "\xf0\x9f\x90\xa3";  // 🐣
        case GrowthStage::MATURE: return getAnimalEmoji(t);
    }
    return "\xf0\x9f\xa5\x9a";
}

std::string getCropEmoji(CropType t) {
    switch (t) {
        case CropType::WHEAT: return "\xf0\x9f\x8c\xbe";      // 🌾
        case CropType::CORN: return "\xf0\x9f\x8c\xbd";       // 🌽
        case CropType::CARROT: return "\xf0\x9f\xa5\x95";     // 🥕
        case CropType::TOMATO: return "\xf0\x9f\x8d\x85";     // 🍅
        case CropType::STRAWBERRY: return "\xf0\x9f\x8d\x93"; // 🍓
        case CropType::EGGPLANT: return "\xf0\x9f\x8d\x86";   // 🍆
        case CropType::CUCUMBER: return "\xf0\x9f\xa5\x92";   // 🥒
    }
    return "\xf0\x9f\x8c\xbe";
}

std::string getCropEmoji(CropType t, GrowthStage s) {
    switch (s) {
        case GrowthStage::SEEDLING: return "\xf0\x9f\x8c\xb0"; // 🌰
        case GrowthStage::GROWING: return "\xf0\x9f\x8c\xbf";  // 🌿
        case GrowthStage::MATURE: return getCropEmoji(t);
    }
    return "\xf0\x9f\x8c\xb0";
}

// ======================== PomodoroTimer ========================

PomodoroTimer::PomodoroTimer() {
    m_lastTick = std::chrono::steady_clock::now();
}

void PomodoroTimer::startFocus(int minutes) {
    if (minutes < 5) minutes = 5;
    if (minutes > 120) minutes = 120;
    m_total = minutes * 60;
    m_remaining = m_total;
    m_state = State::FOCUSING;
    m_justFinished = false;
    m_finishedAs = State::IDLE;
    m_lastTick = std::chrono::steady_clock::now();
}

void PomodoroTimer::startRest(int minutes) {
    if (minutes < 5) minutes = 5;
    if (minutes > 20) minutes = 20;
    m_total = minutes * 60;
    m_remaining = m_total;
    m_state = State::RESTING;
    m_justFinished = false;
    m_finishedAs = State::IDLE;
    m_lastTick = std::chrono::steady_clock::now();
}

void PomodoroTimer::stop() {
    m_state = State::IDLE;
    m_remaining = 0;
    m_total = 0;
    m_finishedAs = State::IDLE;
}

void PomodoroTimer::tick() {
    if (m_state == State::IDLE) return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastTick);

    if (elapsed.count() >= 1) {
        int secs = static_cast<int>(elapsed.count());
        m_remaining -= secs;
        if (m_remaining < 0) m_remaining = 0;
        m_lastTick = now;

        if (m_remaining <= 0) {
            m_finishedAs = m_state;
            m_justFinished = true;
            m_state = State::IDLE;
        }
    }
}

std::string PomodoroTimer::stateStr() const {
    switch (m_state) {
        case State::IDLE: return "\xe5\xbe\x85\xe6\x9c\xba";       // 待机
        case State::FOCUSING: return "\xe4\xb8\x93\xe6\xb3\xa8\xe4\xb8\xad"; // 专注中
        case State::RESTING: return "\xe4\xbc\x91\xe6\x81\xaf\xe4\xb8\xad";   // 休息中
    }
    return "\xe6\x9c\xaa\xe7\x9f\xa5";
}

std::string PomodoroTimer::timeStr() const {
    int mins = m_remaining / 60;
    int secs = m_remaining % 60;
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << mins
        << ":" << std::setw(2) << secs;
    return oss.str();
}

// ======================== Scene ========================

Scene::Scene(SceneType type, const std::string& name)
    : m_type(type), m_name(name) {
    m_lastFocusTime = std::chrono::system_clock::now();
    m_lastPestCheck = std::chrono::system_clock::now();
}

const SceneObject* Scene::currentObject() const {
    if (m_objects.empty()) return nullptr;
    return &m_objects.back();
}

bool Scene::canPlant() const {
    if (m_objects.empty()) return true;
    return m_objects.back().isMature();
}

void Scene::plant(int typeIndex) {
    if (!canPlant()) return;
    SceneObject obj;
    obj.typeIndex = typeIndex;
    obj.stage = GrowthStage::SEEDLING;
    obj.accumulatedHours = 0.0;
    m_objects.push_back(obj);
    m_lastFocusTime = std::chrono::system_clock::now();
    m_lastPestCheck = std::chrono::system_clock::now();
    expandGrid();
}

SceneObject& Scene::currentOrLastObject() {
    if (m_objects.empty()) {
        m_objects.push_back(SceneObject());
    }
    return m_objects.back();
}

void Scene::addFocusHours(double hours) {
    if (m_objects.empty()) return;

    SceneObject& obj = m_objects.back();
    if (obj.isMature()) return;

    // 只取当前阶段需要的量，溢出留给下一个物体
    double needed = SceneObject::HOURS_TO_MATURE - obj.accumulatedHours;
    double toAdd = hours;
    if (toAdd > needed) toAdd = needed;

    obj.accumulatedHours += toAdd;

    if (obj.accumulatedHours >= SceneObject::HOURS_TO_MATURE) {
        obj.stage = GrowthStage::MATURE;
        obj.accumulatedHours = SceneObject::HOURS_TO_MATURE;
    } else if (obj.accumulatedHours >= SceneObject::HOURS_TO_GROW) {
        obj.stage = GrowthStage::GROWING;
    }

    m_lastFocusTime = std::chrono::system_clock::now();
    if (hours >= MIN_FOCUS_FOR_PEST_RESET) {
        m_lastPestCheck = std::chrono::system_clock::now();
    }
}

double Scene::hoursSinceLastFocus() const {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastFocusTime);
    return duration.count() / 3600.0;
}

void Scene::resetLastFocusTime() {
    m_lastFocusTime = std::chrono::system_clock::now();
}

bool Scene::removePest() {
    if (m_pestCount > 0) {
        m_pestCount--;
        return true;
    }
    return false;
}

void Scene::updatePests() {
    if (m_locked) return;

    auto now = std::chrono::system_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastPestCheck);
    double hours = delta.count() / 3600.0;
    int newPests = static_cast<int>(hours / m_pestIntervalHours);
    if (newPests > 0) {
        m_pestCount += newPests;
        m_lastPestCheck += std::chrono::seconds(static_cast<long long>(newPests * m_pestIntervalHours * 3600));
    }
}

void Scene::expandGrid() {
    int count = static_cast<int>(m_objects.size());
    // 确保网格能容纳所有物体
    while (count > m_width * m_height) {
        if (m_width <= m_height) {
            m_width++;
        } else {
            m_height++;
        }
    }
}

bool Scene::removeLastObject() {
    if (m_objects.empty()) return false;
    m_objects.pop_back();
    return true;
}

std::string Scene::pestEmoji() const {
    switch (m_type) {
        case SceneType::FOREST: return "\xf0\x9f\x8c\xbf";  // 🌿 杂草
        case SceneType::POND: return "\xf0\x9f\xaa\xb8";    // 🪸 珊瑚
        case SceneType::PASTURE: return "\xf0\x9f\x90\x8d"; // 🐍 蛇
        case SceneType::FIELD: return "\xf0\x9f\x90\x9b";   // 🐛 虫子
        default: return "?";
    }
}

std::string Scene::sceneEmoji() const {
    switch (m_type) {
        case SceneType::FOREST: return "\xf0\x9f\x8c\xb2";
        case SceneType::POND: return "\xf0\x9f\x90\x9f";
        case SceneType::PASTURE: return "\xf0\x9f\x90\x84";
        case SceneType::FIELD: return "\xf0\x9f\x8c\xbe";
        default: return "?";
    }
}

std::string Scene::serialize() const {
    std::ostringstream oss;
    oss << "name=" << m_name << "\n";
    oss << "locked=" << (m_locked ? "1" : "0") << "\n";
    oss << "pests=" << m_pestCount << "\n";
    oss << "width=" << m_width << "\n";
    oss << "height=" << m_height << "\n";
    oss << "pestInterval=" << m_pestIntervalHours << "\n";
    oss << "lastFocusTime=" << std::chrono::duration_cast<std::chrono::seconds>(
        m_lastFocusTime.time_since_epoch()).count() << "\n";
    oss << "lastPestCheck=" << std::chrono::duration_cast<std::chrono::seconds>(
        m_lastPestCheck.time_since_epoch()).count() << "\n";
    oss << "objects=";
    for (size_t i = 0; i < m_objects.size(); i++) {
        if (i > 0) oss << ";";
        oss << static_cast<int>(m_objects[i].stage) << ","
            << m_objects[i].typeIndex << ","
            << m_objects[i].accumulatedHours;
    }
    oss << "\n";
    return oss.str();
}

void Scene::deserialize(const std::string& id, const std::string& data) {
    // id is the scene type index string, e.g. "FOREST:0"
    auto getVal = [&](const std::string& key) -> std::string {
        size_t pos = data.find(key + "=");
        if (pos == std::string::npos) return "";
        pos += key.length() + 1;
        size_t end = data.find('\n', pos);
        if (end == std::string::npos) end = data.length();
        return data.substr(pos, end - pos);
    };

    std::string nameVal = getVal("name");
    if (!nameVal.empty()) m_name = nameVal;
    m_locked = (getVal("locked") == "1");
    m_pestCount = std::stoi(getVal("pests").empty() ? "0" : getVal("pests"));
    m_width = std::stoi(getVal("width").empty() ? "3" : getVal("width"));
    m_height = std::stoi(getVal("height").empty() ? "2" : getVal("height"));
    m_pestIntervalHours = std::stod(getVal("pestInterval").empty() ? "48.0" : getVal("pestInterval"));

    std::string lft = getVal("lastFocusTime");
    if (!lft.empty()) {
        m_lastFocusTime = std::chrono::system_clock::time_point(
            std::chrono::seconds(std::stoll(lft)));
    }

    std::string lpc = getVal("lastPestCheck");
    if (!lpc.empty()) {
        m_lastPestCheck = std::chrono::system_clock::time_point(
            std::chrono::seconds(std::stoll(lpc)));
    }

    // Parse objects
    std::string objs = getVal("objects");
    m_objects.clear();
    if (!objs.empty()) {
        size_t start = 0;
        while (start < objs.length()) {
            size_t end = objs.find(';', start);
            if (end == std::string::npos) end = objs.length();
            std::string part = objs.substr(start, end - start);
            // Format: stage,typeIndex,accumulatedHours
            size_t c1 = part.find(',');
            size_t c2 = part.find(',', c1 + 1);
            if (c1 != std::string::npos && c2 != std::string::npos) {
                SceneObject obj;
                obj.stage = static_cast<GrowthStage>(std::stoi(part.substr(0, c1)));
                obj.typeIndex = std::stoi(part.substr(c1 + 1, c2 - c1 - 1));
                obj.accumulatedHours = std::stod(part.substr(c2 + 1));
                m_objects.push_back(obj);
            }
            start = end + 1;
        }
    }
}

// ======================== Warehouse ========================

bool Warehouse::consumeWood(int n) {
    if (m_wood >= n) { m_wood -= n; return true; }
    return false;
}
bool Warehouse::consumeFish(int n) {
    if (m_fish >= n) { m_fish -= n; return true; }
    return false;
}
bool Warehouse::consumeMeat(int n) {
    if (m_meat >= n) { m_meat -= n; return true; }
    return false;
}
bool Warehouse::consumeCrop(int n) {
    if (m_crop >= n) { m_crop -= n; return true; }
    return false;
}

bool Warehouse::useHerbicide() {
    if (m_herbicide > 0) { m_herbicide--; return true; }
    return false;
}
bool Warehouse::usePesticide() {
    if (m_pesticide > 0) { m_pesticide--; return true; }
    return false;
}
bool Warehouse::useSnakeRepellent() {
    if (m_snakeRepellent > 0) { m_snakeRepellent--; return true; }
    return false;
}
bool Warehouse::useCableTie() {
    if (m_cableTie > 0) { m_cableTie--; return true; }
    return false;
}

// ======================== Game ========================

Game& Game::instance() {
    static Game game;
    return game;
}

Game::Game() {
    // 初始化默认场景
    m_forests.push_back(Scene(SceneType::FOREST, "\xe9\xab\x98\xe7\xad\x89\xe6\x95\xb0\xe5\xad\xa6"));     // 高等数学
    m_forests.push_back(Scene(SceneType::FOREST, "\xe5\xa4\xa7\xe5\xad\xa6\xe8\x8b\xb1\xe8\xaf\xad"));     // 大学英语
    m_forests.push_back(Scene(SceneType::FOREST, "\xe7\xba\xbf\xe6\x80\xa7\xe4\xbb\xa3\xe6\x95\xb0"));     // 线性代数

    m_ponds.push_back(Scene(SceneType::POND, "\xe6\x96\x87\xe5\xad\xa6\xe9\x98\x85\xe8\xaf\xbb"));         // 文学阅读
    m_ponds.push_back(Scene(SceneType::POND, "\xe4\xb8\x93\xe4\xb8\x9a\xe4\xb9\xa6\xe7\xb1\x8d"));         // 专业书籍

    m_pastures.push_back(Scene(SceneType::PASTURE, "C++\xe7\xbc\x96\xe7\xa8\x8b"));                         // C++编程
    m_pastures.push_back(Scene(SceneType::PASTURE, "Python\xe5\xbc\x80\xe5\x8f\x91"));                       // Python开发

    m_fields.push_back(Scene(SceneType::FIELD, "\xe8\xaf\xbe\xe7\xa8\x8b\xe8\xae\xbe\xe8\xae\xa1"));         // 课程设计
    m_fields.push_back(Scene(SceneType::FIELD, "\xe5\xbc\x80\xe6\xba\x90\xe9\xa1\xb9\xe7\x9b\xae"));         // 开源项目

    m_lastWeatherRefresh = std::chrono::steady_clock::now();
    m_lastPestCheck = std::chrono::steady_clock::now();
}

Game::~Game() {
    stop();
}

Scene* Game::currentScene() {
    switch (m_currentSceneType) {
        case SceneType::FOREST:
            if (m_currentSceneIndex >= 0 && m_currentSceneIndex < (int)m_forests.size())
                return &m_forests[m_currentSceneIndex];
            break;
        case SceneType::POND:
            if (m_currentSceneIndex >= 0 && m_currentSceneIndex < (int)m_ponds.size())
                return &m_ponds[m_currentSceneIndex];
            break;
        case SceneType::PASTURE:
            if (m_currentSceneIndex >= 0 && m_currentSceneIndex < (int)m_pastures.size())
                return &m_pastures[m_currentSceneIndex];
            break;
        case SceneType::FIELD:
            if (m_currentSceneIndex >= 0 && m_currentSceneIndex < (int)m_fields.size())
                return &m_fields[m_currentSceneIndex];
            break;
        default:
            break;
    }
    return nullptr;
}

void Game::setCurrentScene(SceneType type, int index) {
    m_currentSceneType = type;
    m_currentSceneIndex = index;
    m_inMarket = false;
    m_inWarehouse = false;
    m_inMainMenu = false;
}

void Game::addMessage(const std::string& msg) {
    m_messages.push_back(msg);
    if (m_messages.size() > 20) {
        m_messages.erase(m_messages.begin());
    }
}

void Game::refreshWeather() {
    // 使用Python脚本获取天气
    std::string cmd = "python scripts/get_weather.py 2>&1";
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) {
        m_weather.locationAvailable = false;
        m_weather.city = "无法获取";
        m_weather.weather = "请检查Python是否安装";
        return;
    }

    char buffer[4096];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    _pclose(pipe);

    // 简单JSON解析
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
        m_weather.locationAvailable = false;
        m_weather.city = "获取失败";
        m_weather.weather = "请检查网络或Windows位置设置";
    } else if (result.empty()) {
        m_weather.locationAvailable = false;
        m_weather.city = "无响应";
        m_weather.weather = "请检查Python和网络";
    } else {
        m_weather.city = findValue("city");
        m_weather.country = findValue("country");
        m_weather.weather = findValue("weather");
        m_weather.temp = findValue("temp_c");
        m_weather.humidity = findValue("humidity");
        m_weather.wind = findValue("wind_speed");
        m_weather.locationAvailable = true;
    }
}

// ======================== Game 主循环 ========================

void Game::run() {
    m_running = true;
    m_lastWeatherRefresh = std::chrono::steady_clock::now();
    m_lastPestCheck = std::chrono::steady_clock::now();

    hideCursor();

    // 加载存档
    loadFromFile();

    // 追赶离线时间：更新所有场景的有害物品
    auto catchUpPests = [&](std::vector<Scene>& scenes) {
        for (auto& s : scenes) {
            s.updatePests();
        }
    };
    catchUpPests(m_forests);
    catchUpPests(m_ponds);
    catchUpPests(m_pastures);
    catchUpPests(m_fields);

    // 首次刷新天气
    refreshWeather();
    addMessage("🌻 欢迎来到专注农场！");

    mainLoop();
}

void Game::stop() {
    saveToFile();
    m_running = false;
    showCursor();
}

void Game::mainLoop() {
    while (m_running) {
        // 处理输入
#ifdef _WIN32
        while (_kbhit()) {
            int ch = _getch();
            // 处理扩展键（方向键等）
            if (ch == 0 || ch == 224) {
                int ext = _getch();
                m_inputBuffer += (char)(ch);
                m_inputBuffer += (char)(ext);
            } else {
                m_inputBuffer += (char)(ch);
            }
        }
        if (!m_inputBuffer.empty()) {
            processInput();
        }
#else
        // Unix 系统
        struct timeval tv = {0, 0};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
            char ch;
            if (read(STDIN_FILENO, &ch, 1) > 0) {
                m_inputBuffer += ch;
                processInput();
            }
        }
#endif

        updateGame();
        renderUI();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// 获取场景列表和数量的辅助函数
static int getSceneCount(Game& game, SceneType type) {
    switch (type) {
        case SceneType::FOREST: return (int)game.forests().size();
        case SceneType::POND: return (int)game.ponds().size();
        case SceneType::PASTURE: return (int)game.pastures().size();
        case SceneType::FIELD: return (int)game.fields().size();
        default: return 0;
    }
}

void Game::processInput() {
    std::string buf = m_inputBuffer;
    m_inputBuffer.clear();

    for (size_t i = 0; i < buf.size(); i++) {
        unsigned char ch = (unsigned char)buf[i];

        // 处理方向键 (Windows: 224 + code)
        if (ch == 224 && i + 1 < buf.size()) {
            int code = (unsigned char)buf[i + 1];
            i++;

            if (m_inMainMenu) {
                // 主菜单方向键：切换场景类型
                int typeIdx = static_cast<int>(m_currentSceneType);
                if (code == 75) { // 左箭头
                    typeIdx--;
                    if (typeIdx < 0) typeIdx = 3;
                } else if (code == 77) { // 右箭头
                    typeIdx++;
                    if (typeIdx > 3) typeIdx = 0;
                } else if (code == 72) { // 上箭头
                    // 上一个子场景
                } else if (code == 80) { // 下箭头
                    // 下一个子场景
                }
                m_currentSceneType = static_cast<SceneType>(typeIdx);
                m_currentSceneIndex = 0;
                continue;
            }

            if (!m_inMarket && !m_inWarehouse && !m_inMainMenu) {
                // 场景内方向键：切换子场景
                SceneType type = m_currentSceneType;
                int count = getSceneCount(*this, type);
                if (code == 72) { // 上箭头
                    m_currentSceneIndex--;
                    if (m_currentSceneIndex < 0) m_currentSceneIndex = count - 1;
                } else if (code == 80) { // 下箭头
                    m_currentSceneIndex++;
                    if (m_currentSceneIndex >= count) m_currentSceneIndex = 0;
                } else if (code == 75) { // 左箭头 - 切换场景类型
                    int typeIdx = static_cast<int>(type);
                    typeIdx--;
                    if (typeIdx < 0) typeIdx = 3;
                    m_currentSceneType = static_cast<SceneType>(typeIdx);
                    m_currentSceneIndex = 0;
                } else if (code == 77) { // 右箭头 - 切换场景类型
                    int typeIdx = static_cast<int>(type);
                    typeIdx++;
                    if (typeIdx > 3) typeIdx = 0;
                    m_currentSceneType = static_cast<SceneType>(typeIdx);
                    m_currentSceneIndex = 0;
                }
            }
            continue;
        }

        // 全局快捷键
        if (ch == 'q' || ch == 'Q') {
            saveToFile();
            m_running = false;
            return;
        }

        // 主菜单操作
        if (m_inMainMenu) {
            switch (ch) {
                case '\r': case '\n': case ' ': {
                    // 进入选中的场景
                    m_inMainMenu = false;
                    addMessage("\xf0\x9f\x94\x8d \xe8\xbf\x9b\xe5\x85\xa5\xe5\x9c\xba\xe6\x99\xaf"); // 🔍 进入场景
                    break;
                }
                case 'm': case 'M':
                    m_inMarket = true;
                    m_inMainMenu = false;
                    break;
                case 'w': case 'W':
                    m_inWarehouse = true;
                    m_inMainMenu = false;
                    break;
                case 's': case 'S':
                    cyclePestInterval();
                    addMessage("\xe2\x9a\x99 \xe6\x9d\x82\xe8\x8d\x89\xe7\x94\x9f\xe6\x88\x90\xe9\x80\x9f\xe5\xba\xa6\xe5\xb7\xb2\xe8\xae\xbe\xe4\xb8\xba: " + pestIntervalLabel());
                    break;
                case 'x': case 'X':
                    addMessage("⚠ 确认重置整个系统？所有数据将丢失！[Y]确认 [其他键]取消");
                    m_inputMode = 9;
                    break;
                case '1': case '2': case '3': case '4': {
                    // 快速选择场景类型
                    int idx = ch - '1';
                    if (idx < 4) {
                        m_currentSceneType = static_cast<SceneType>(idx);
                        m_currentSceneIndex = 0;
                    }
                    break;
                }
            }
            continue;
        }

        // 集市模式
        if (m_inMarket) {
            switch (ch) {
                case 'b': case 'B':
                    m_inMarket = false;
                    m_inMainMenu = true;
                    break;
                case '1':
                    if (m_warehouse.consumeWood(3)) {
                        m_warehouse.addHerbicide();
                        addMessage("\xf0\x9f\x9b\x92 \xe7\x94\xa8 3\xf0\x9f\xaa\xb5 \xe6\x8d\xa2\xe5\x8f\x96\xe4\xba\x86 1\xf0\x9f\xa7\xaa \xe9\x99\xa4\xe8\x8d\x89\xe5\x89\x82");
                    } else addMessage("\xe2\x9d\x8c \xe6\x9c\xa8\xe5\xa4\xb4\xe4\xb8\x8d\xe8\xb6\xb3\xef\xbc\x81\xe9\x9c\x80\xe8\xa6\x81 3\xf0\x9f\xaa\xb5");
                    break;
                case '2':
                    if (m_warehouse.consumeCrop(3)) {
                        m_warehouse.addPesticide();
                        addMessage("\xf0\x9f\x9b\x92 \xe7\x94\xa8 3\xf0\x9f\x8c\xbd \xe6\x8d\xa2\xe5\x8f\x96\xe4\xba\x86 1\xf0\x9f\x92\x8a \xe5\x86\x9c\xe8\x8d\xaf");
                    } else addMessage("\xe2\x9d\x8c \xe4\xbd\x9c\xe7\x89\xa9\xe4\xb8\x8d\xe8\xb6\xb3\xef\xbc\x81\xe9\x9c\x80\xe8\xa6\x81 3\xf0\x9f\x8c\xbd");
                    break;
                case '3':
                    if (m_warehouse.consumeMeat(3)) {
                        m_warehouse.addSnakeRepellent();
                        addMessage("\xf0\x9f\x9b\x92 \xe7\x94\xa8 3\xf0\x9f\xa5\xa9 \xe6\x8d\xa2\xe5\x8f\x96\xe4\xba\x86 1\xf0\x9f\xaa\xa4 \xe9\xa9\xb1\xe8\x9b\x87\xe5\x89\x82");
                    } else addMessage("\xe2\x9d\x8c \xe8\x82\x89\xe4\xb8\x8d\xe8\xb6\xb3\xef\xbc\x81\xe9\x9c\x80\xe8\xa6\x81 3\xf0\x9f\xa5\xa9");
                    break;
                case '4':
                    if (m_warehouse.consumeFish(3)) {
                        m_warehouse.addCableTie();
                        addMessage("\xf0\x9f\x9b\x92 \xe7\x94\xa8 3\xf0\x9f\x90\x9f \xe6\x8d\xa2\xe5\x8f\x96\xe4\xba\x86 1\xf0\x9f\x94\x97 \xe8\xbd\xa7\xe5\xb8\xa6");
                    } else addMessage("\xe2\x9d\x8c \xe9\xb1\xbc\xe4\xb8\x8d\xe8\xb6\xb3\xef\xbc\x81\xe9\x9c\x80\xe8\xa6\x81 3\xf0\x9f\x90\x9f");
                    break;
            }
            continue;
        }

        // 仓库模式
        if (m_inWarehouse) {
            switch (ch) {
                case 'b': case 'B':
                    m_inWarehouse = false;
                    m_inMainMenu = true;
                    break;
                case '1':
                    if (m_warehouse.useHerbicide()) {
                        for (auto& f : m_forests) {
                            if (f.pestCount() > 0) {
                                f.removePest();
                                addMessage("\xf0\x9f\xa7\xaa \xe4\xbd\xbf\xe7\x94\xa8\xe9\x99\xa4\xe8\x8d\x89\xe5\x89\x82\xe6\xb8\x85\xe9\x99\xa4\xe4\xba\x86\xe4\xb8\x80\xe4\xb8\xaa\xf0\x9f\x8c\xbf");
                                break;
                            }
                        }
                    } else addMessage("\xe2\x9d\x8c \xe6\xb2\xa1\xe6\x9c\x89\xe9\x99\xa4\xe8\x8d\x89\xe5\x89\x82\xef\xbc\x81");
                    break;
                case '2':
                    if (m_warehouse.usePesticide()) {
                        for (auto& f : m_fields) {
                            if (f.pestCount() > 0) {
                                f.removePest();
                                addMessage("\xf0\x9f\x92\x8a \xe4\xbd\xbf\xe7\x94\xa8\xe5\x86\x9c\xe8\x8d\xaf\xe6\xb8\x85\xe9\x99\xa4\xe4\xba\x86\xe4\xb8\x80\xe4\xb8\xaa\xf0\x9f\x90\x9b");
                                break;
                            }
                        }
                    } else addMessage("\xe2\x9d\x8c \xe6\xb2\xa1\xe6\x9c\x89\xe5\x86\x9c\xe8\x8d\xaf\xef\xbc\x81");
                    break;
                case '3':
                    if (m_warehouse.useSnakeRepellent()) {
                        for (auto& p : m_pastures) {
                            if (p.pestCount() > 0) {
                                p.removePest();
                                addMessage("\xf0\x9f\xaa\xa4 \xe4\xbd\xbf\xe7\x94\xa8\xe9\xa9\xb1\xe8\x9b\x87\xe5\x89\x82\xe6\xb8\x85\xe9\x99\xa4\xe4\xba\x86\xe4\xb8\x80\xe4\xb8\xaa\xf0\x9f\x90\x8d");
                                break;
                            }
                        }
                    } else addMessage("\xe2\x9d\x8c \xe6\xb2\xa1\xe6\x9c\x89\xe9\xa9\xb1\xe8\x9b\x87\xe5\x89\x82\xef\xbc\x81");
                    break;
                case '4':
                    if (m_warehouse.useCableTie()) {
                        for (auto& p : m_ponds) {
                            if (p.pestCount() > 0) {
                                p.removePest();
                                addMessage("\xf0\x9f\x94\x97 \xe4\xbd\xbf\xe7\x94\xa8\xe8\xbd\xa7\xe5\xb8\xa6\xe6\xb8\x85\xe9\x99\xa4\xe4\xba\x86\xe4\xb8\x80\xe4\xb8\xaa\xf0\x9f\xaa\xb8");
                                break;
                            }
                        }
                    } else addMessage("\xe2\x9d\x8c \xe6\xb2\xa1\xe6\x9c\x89\xe8\xbd\xa7\xe5\xb8\xa6\xef\xbc\x81");
                    break;
            }
            continue;
        }

        // 场景内操作
        Scene* sc = currentScene();
        if (!sc) continue;

        switch (ch) {
            case 'b': case 'B':
                // 返回主菜单
                m_inMainMenu = true;
                break;
            case 'm': case 'M':
                m_inMarket = true;
                break;
            case 'w': case 'W':
                m_inWarehouse = true;
                break;
            case '1': {
                // 开始专注
                if (m_timer.state() == PomodoroTimer::State::RESTING) {
                    m_timer.stop();
                    addMessage("\xe2\x8f\xb9 \xe5\xb7\xb2\xe5\x81\x9c\xe6\xad\xa2\xe4\xbc\x91\xe6\x81\xaf");
                }
                if (m_timer.state() == PomodoroTimer::State::FOCUSING) {
                    addMessage("\xe2\x9d\x8c \xe7\x95\xaa\xe8\x8c\x84\xe9\x92\x9f\xe5\xb7\xb2\xe5\x9c\xa8\xe8\xbf\x90\xe8\xa1\x8c\xe4\xb8\xad");
                    break;
                }
                if (!sc->currentObject() && !sc->canPlant()) {
                    addMessage("\xe2\x9d\x8c \xe8\xaf\xb7\xe5\x85\x88\xe7\xa7\x8d\xe6\xa4\x8d\xe4\xb8\x80\xe4\xb8\xaa\xe7\x89\xa9\xe4\xbd\x93");
                    break;
                }
                if (sc->currentObject() && sc->currentObject()->isMature()) {
                    addMessage("\xe2\x9d\x8c \xe5\xbd\x93\xe5\x89\x8d\xe7\x89\xa9\xe4\xbd\x93\xe5\xb7\xb2\xe6\x88\x90\xe7\x86\x9f\xef\xbc\x8c\xe8\xaf\xb7\xe5\x85\x88\xe7\xa7\x8d\xe6\xa4\x8d\xe6\x96\xb0\xe7\x89\xa9\xe4\xbd\x93");
                    break;
                }
                // 15h/24h 限制检查
                double f24 = focusInLast24h();
                if (f24 >= 15.0) {
                    addMessage("\xe2\x9d\x8c 24\xe5\xb0\x8f\xe6\x97\xb6\xe5\x86\x85\xe5\xb7\xb2\xe4\xb8\x93\xe6\xb3\xa8 " +
                        std::to_string(static_cast<int>(f24 * 60)) + " 分钟，已达到上限15小时，请休息后再来");
                    break;
                }
                // 预设专注时长：10, 20, 30, 40, 60, 90, 120分钟
                addMessage("\xe2\x8f\xb1 \xe8\xaf\xb7\xe9\x80\x89\xe6\x8b\xa9\xe4\xb8\x93\xe6\xb3\xa8\xe6\x97\xb6\xe9\x95\xbf: [A]10min [B]20min [C]30min [D]40min [E]1h [F]1.5h [G]2h");
                m_inputMode = 1;
                break;
            }
            case '2': {
                // 停止专注或休息
                if (m_timer.state() == PomodoroTimer::State::IDLE) {
                    addMessage("\xe2\x9d\x8c \xe6\xb2\xa1\xe6\x9c\x89\xe5\x9c\xa8\xe8\xbf\x9b\xe8\xa1\x8c\xe7\x9a\x84\xe8\xae\xa1\xe6\x97\xb6");
                    break;
                }
                int elapsed = m_timer.elapsedSeconds();
                double hours = elapsed / 3600.0;
                if (hours > 0 && m_timer.state() == PomodoroTimer::State::FOCUSING) {
                    sc->addFocusHours(hours);
                    m_focusHistory.push_back({std::chrono::system_clock::now(), hours});
                    addMessage("\xe2\x8f\xb9 \xe6\x89\x8b\xe5\x8a\xa8\xe5\x81\x9c\xe6\xad\xa2\xe4\xb8\x93\xe6\xb3\xa8\xef\xbc\x8c\xe5\xb7\xb2\xe7\xb4\xaf\xe7\xa7\xaf " +
                        std::to_string(static_cast<int>(hours * 60)) + " \xe5\x88\x86\xe9\x92\x9f");
                    if (hours >= Scene::MIN_FOCUS_FOR_PEST_RESET) {
                        addMessage("\xf0\x9f\x8c\xbf " + sc->name() + " \xe6\x9d\x82\xe8\x8d\x89\xe8\xae\xa1\xe6\x97\xb6\xe5\x99\xa8\xe5\xb7\xb2\xe9\x87\x8d\xe7\xbd\xae");
                    } else {
                        addMessage("⚠ 专注不足20分钟，杂草计时器未重置");
                    }
                    addMessage("😴 要休息一下吗？ [A]5min [B]10min [C]15min [D]20min 或任意键跳过");
                    m_inputMode = 7;
                } else if (m_timer.state() == PomodoroTimer::State::RESTING) {
                    addMessage("\xe2\x8f\xb9 \xe6\x89\x8b\xe5\x8a\xa8\xe5\x81\x9c\xe6\xad\xa2\xe4\xbc\x91\xe6\x81\xaf");
                }
                m_timer.stop();
                break;
            }
            case '0': {
                // 手动开始休息
                if (m_timer.state() == PomodoroTimer::State::FOCUSING) {
                    int elapsed = m_timer.elapsedSeconds();
                    double hours = elapsed / 3600.0;
                    if (hours > 0 && sc) {
                        sc->addFocusHours(hours);
                        m_focusHistory.push_back({std::chrono::system_clock::now(), hours});
                        addMessage("\xe2\x8f\xb9 \xe5\xb7\xb2\xe5\x81\x9c\xe6\xad\xa2\xe4\xb8\x93\xe6\xb3\xa8\xef\xbc\x8c\xe5\xb7\xb2\xe7\xb4\xaf\xe7\xa7\xaf " +
                            std::to_string(static_cast<int>(hours * 60)) + " \xe5\x88\x86\xe9\x92\x9f");
                        if (hours >= Scene::MIN_FOCUS_FOR_PEST_RESET) {
                            addMessage("\xf0\x9f\x8c\xbf " + sc->name() + " \xe6\x9d\x82\xe8\x8d\x89\xe8\xae\xa1\xe6\x97\xb6\xe5\x99\xa8\xe5\xb7\xb2\xe9\x87\x8d\xe7\xbd\xae");
                        } else {
                            addMessage("⚠ 专注不足20分钟，杂草计时器未重置");
                        }
                    }
                    m_timer.stop();
                }
                if (m_timer.state() == PomodoroTimer::State::RESTING) {
                    addMessage("\xe2\x9d\x8c \xe5\xb7\xb2\xe5\x9c\xa8\xe4\xbc\x91\xe6\x81\xaf\xe4\xb8\xad");
                    break;
                }
                addMessage("\xf0\x9f\x98\xb4 \xe8\xaf\xb7\xe9\x80\x89\xe6\x8b\xa9\xe4\xbc\x91\xe6\x81\xaf\xe6\x97\xb6\xe9\x95\xbf: [A]5min [B]10min [C]15min [D]20min \xe6\x88\x96\xe4\xbb\xbb\xe6\x84\x8f\xe9\x94\xae\xe8\xb7\xb3\xe8\xbf\x87");
                m_inputMode = 7;
                break;
            }
            case '3': {
                // 种新物体
                if (!sc->canPlant()) {
                    addMessage("\xe2\x9d\x8c \xe5\xbd\x93\xe5\x89\x8d\xe7\x89\xa9\xe4\xbd\x93\xe8\xbf\x98\xe6\x9c\xaa\xe5\xae\x8c\xe5\x85\xa8\xe9\x95\xbf\xe5\xa4\xa7\xef\xbc\x81");
                    break;
                }
                switch (sc->type()) {
                    case SceneType::FOREST:
                        addMessage("\xf0\x9f\x8c\xb1 \xe9\x80\x89\xe6\x8b\xa9\xe6\xa0\x91\xe7\xa7\x8d: [A]\xf0\x9f\x8c\xb2\xe6\x9d\xbe [B]\xf0\x9f\x8c\xb3\xe8\x90\xbd\xe5\x8f\xb6 [C]\xf0\x9f\x8c\xb4\xe6\xa3\x95\xe6\xa6\x88");
                        m_inputMode = 2;
                        break;
                    case SceneType::POND:
                        addMessage("\xf0\x9f\x90\x9f \xe9\x80\x89\xe6\x8b\xa9\xe9\xb1\xbc\xe7\xa7\x8d: [A]\xf0\x9f\x90\x9f\xe6\x99\xae\xe9\x80\x9a [B]\xf0\x9f\x90\xa0\xe7\x83\xad\xe5\xb8\xa6 [C]\xf0\x9f\x90\xa1\xe6\xb2\xb3\xe8\xb1\x9a");
                        m_inputMode = 3;
                        break;
                    case SceneType::PASTURE:
                        addMessage("\xf0\x9f\x90\x84 \xe9\x80\x89\xe6\x8b\xa9\xe7\x89\xb2\xe7\x95\x9c: [A]\xf0\x9f\x90\x84\xe7\x89\x9b [B]\xf0\x9f\x90\x91\xe7\xbe\x8a [C]\xf0\x9f\x90\x96\xe7\x8c\xaa [D]\xf0\x9f\x90\x93\xe9\xb8\xa1");
                        m_inputMode = 4;
                        break;
                    case SceneType::FIELD:
                        addMessage("\xf0\x9f\x8c\xbe \xe9\x80\x89\xe6\x8b\xa9\xe4\xbd\x9c\xe7\x89\xa9: [A]\xf0\x9f\x8c\xbe\xe5\xb0\x8f\xe9\xba\xa6 [B]\xf0\x9f\x8c\xbd\xe7\x8e\x89\xe7\xb1\xb3 [C]\xf0\x9f\xa5\x95\xe8\x83\xa1\xe8\x90\x9d\xe5\x8d\x9c [D]\xf0\x9f\x8d\x85\xe8\xa5\xbf\xe7\xba\xa2\xe6\x9f\xbf [E]\xf0\x9f\x8d\x93\xe8\x8d\x89\xe8\x8e\x93 [F]\xf0\x9f\x8d\x86\xe8\x8c\x84\xe5\xad\x90 [G]\xf0\x9f\xa5\x92\xe9\xbb\x84\xe7\x93\x9c");
                        m_inputMode = 5;
                        break;
                    default: break;
                }
                break;
            }
            case '4': {
                // 清除有害物品（自然方式）
                if (sc->currentObject() && sc->currentObject()->isMature() && sc->pestCount() > 0) {
                    sc->removePest();
                    addMessage("\xf0\x9f\xa7\xb9 \xe6\xb8\x85\xe9\x99\xa4\xe4\xba\x86\xe4\xb8\x80\xe4\xb8\xaa" + sc->pestEmoji());
                } else if (!sc->currentObject() || !sc->currentObject()->isMature()) {
                    addMessage("\xe2\x9d\x8c \xe5\xbd\x93\xe5\x89\x8d\xe7\x89\xa9\xe4\xbd\x93\xe6\x9c\xaa\xe6\x88\x90\xe7\x86\x9f\xef\xbc\x8c\xe6\x97\xa0\xe6\xb3\x95\xe6\xb8\x85\xe9\x99\xa4");
                } else {
                    addMessage("\xe2\x9d\x8c \xe6\xb2\xa1\xe6\x9c\x89\xe6\x9c\x89\xe5\xae\xb3\xe7\x89\xa9\xe5\x93\x81\xef\xbc\x81");
                }
                break;
            }
            case 'l': case 'L': {
                // 封锁/解锁
                sc->setLocked(!sc->locked());
                addMessage(sc->locked() ? "\xf0\x9f\x94\x92 \xe5\xb7\xb2\xe5\xb0\x81\xe9\x94\x81 " + sc->name()
                                        : "\xf0\x9f\x94\x93 \xe5\xb7\xb2\xe8\xa7\xa3\xe9\x94\x81 " + sc->name());
                break;
            }
            case 'r': case 'R': {
                // 重命名场景
                addMessage("✏ 请输入新名称，按 Enter 确认:");
                m_inputBuffer.clear();
                m_inputMode = 6;
                break;
            }
            case 'd': case 'D': {
                // 铲除当前物体
                if (sc->objects().empty()) {
                    addMessage("❌ 当前场景没有物体可铲除！");
                    break;
                }
                addMessage("⚠ 确认铲除 " + sc->name() + " 的最后一个物体？[Y]确认 [其他键]取消");
                m_inputMode = 8;
                break;
            }
            case 'x': case 'X': {
                // 系统重置
                addMessage("⚠ 确认重置整个系统？所有数据将丢失！[Y]确认 [其他键]取消");
                m_inputMode = 9;
                break;
            }
        }

        // 输入模式处理
        if (m_inputMode > 0) {
            switch (m_inputMode) {
                case 1: { // 专注时长选择
                    int minutes = 0;
                    switch (ch) {
                        case 'a': case 'A': minutes = 10; break;
                        case 'b': case 'B': minutes = 20; break;
                        case 'c': case 'C': minutes = 30; break;
                        case 'd': case 'D': minutes = 40; break;
                        case 'e': case 'E': minutes = 60; break;
                        case 'f': case 'F': minutes = 90; break;
                        case 'g': case 'G': minutes = 120; break;
                        default: break;
                    }
                    if (minutes > 0) {
                        m_timer.startFocus(minutes);
                        addMessage("\xe2\x96\xb6 \xe5\xbc\x80\xe5\xa7\x8b\xe4\xb8\x93\xe6\xb3\xa8 " +
                            std::to_string(minutes) + " \xe5\x88\x86\xe9\x92\x9f");
                        m_inputMode = 0;
                    }
                    break;
                }
                case 2: { // 森林树木类型
                    int typeIdx = -1;
                    switch (ch) {
                        case 'a': case 'A': typeIdx = 0; break; // 松树
                        case 'b': case 'B': typeIdx = 1; break; // 落叶树
                        case 'c': case 'C': typeIdx = 2; break; // 棕榈树
                        default: break;
                    }
                    if (typeIdx >= 0 && sc) {
                        sc->plant(typeIdx);
                        addMessage("\xf0\x9f\x8c\xb1 \xe5\x9c\xa8 " + sc->name() + " \xe7\xa7\x8d\xe4\xb8\x8b\xe4\xba\x86\xe4\xb8\x80\xe6\xa3\xb5\xe6\xa0\x91");
                        m_inputMode = 0;
                    }
                    break;
                }
                case 3: { // 鱼塘鱼类型
                    int typeIdx = -1;
                    switch (ch) {
                        case 'a': case 'A': typeIdx = 0; break;
                        case 'b': case 'B': typeIdx = 1; break;
                        case 'c': case 'C': typeIdx = 2; break;
                        default: break;
                    }
                    if (typeIdx >= 0 && sc) {
                        sc->plant(typeIdx);
                        addMessage("\xf0\x9f\x90\x9f \xe5\x9c\xa8 " + sc->name() + " \xe6\x94\xbe\xe5\x85\xa5\xe4\xba\x86\xe4\xb8\x80\xe6\x9d\xa1\xe9\xb1\xbc");
                        m_inputMode = 0;
                    }
                    break;
                }
                case 4: { // 牧场动物类型
                    int typeIdx = -1;
                    switch (ch) {
                        case 'a': case 'A': typeIdx = 0; break;
                        case 'b': case 'B': typeIdx = 1; break;
                        case 'c': case 'C': typeIdx = 2; break;
                        case 'd': case 'D': typeIdx = 3; break;
                        default: break;
                    }
                    if (typeIdx >= 0 && sc) {
                        sc->plant(typeIdx);
                        addMessage("\xf0\x9f\x90\x84 \xe5\x9c\xa8 " + sc->name() + " \xe5\xbc\x80\xe5\xa7\x8b\xe5\x85\xbb\xe6\xae\x96\xe4\xba\x86\xe4\xb8\x80\xe5\x8f\xaa\xe5\x8a\xa8\xe7\x89\xa9");
                        m_inputMode = 0;
                    }
                    break;
                }
                case 5: { // 稻田作物类型
                    int typeIdx = -1;
                    switch (ch) {
                        case 'a': case 'A': typeIdx = 0; break;
                        case 'b': case 'B': typeIdx = 1; break;
                        case 'c': case 'C': typeIdx = 2; break;
                        case 'd': case 'D': typeIdx = 3; break;
                        case 'e': case 'E': typeIdx = 4; break;
                        case 'f': case 'F': typeIdx = 5; break;
                        case 'g': case 'G': typeIdx = 6; break;
                        default: break;
                    }
                    if (typeIdx >= 0 && sc) {
                        sc->plant(typeIdx);
                        addMessage("\xf0\x9f\x8c\xbe \xe5\x9c\xa8 " + sc->name() + " \xe6\x92\xad\xe7\xa7\x8d\xe4\xba\x86\xe4\xb8\x80\xe6\xa3\xb5\xe4\xbd\x9c\xe7\x89\xa9");
                        m_inputMode = 0;
                    }
                    break;
                }
                case 6: { // 重命名场景
                    if (ch == '\r' || ch == '\n') {
                        if (!m_inputBuffer.empty() && sc) {
                            std::string oldName = sc->name();
                            sc->setName(m_inputBuffer);
                            addMessage("\xe2\x9c\x8f \xe5\xb7\xb2\xe5\xb0\x86 " + oldName + " \xe9\x87\x8d\xe5\x91\xbd\xe5\x90\x8d\xe4\xb8\xba " + m_inputBuffer);
                        }
                        m_inputMode = 0;
                        m_inputBuffer.clear();
                    } else if (ch == '\b' || ch == 127) {
                        if (!m_inputBuffer.empty()) {
                            m_inputBuffer.pop_back();
                        }
                    } else if (ch >= 32 && ch < 127) {
                        m_inputBuffer += ch;
                    }
                    break;
                }
                case 7: { // 休息时长选择
                    int minutes = 0;
                    switch (ch) {
                        case 'a': case 'A': minutes = 5; break;
                        case 'b': case 'B': minutes = 10; break;
                        case 'c': case 'C': minutes = 15; break;
                        case 'd': case 'D': minutes = 20; break;
                        default: m_inputMode = 0; break;
                    }
                    if (minutes > 0) {
                        m_timer.startRest(minutes);
                        addMessage("😴 开始休息 " +
                            std::to_string(minutes) + " 分钟");
                        m_inputMode = 0;
                    }
                    break;
                }
                case 8: { // 删除确认
                    if (ch == 'y' || ch == 'Y') {
                        if (sc && sc->removeLastObject()) {
                            addMessage("🗑 已铲除 " + sc->name() + " 的最后一个物体");
                        }
                    } else {
                        addMessage("❌ 已取消铲除");
                    }
                    m_inputMode = 0;
                    break;
                }
                case 9: { // 重置确认
                    if (ch == 'y' || ch == 'Y') {
                        resetAll();
                    } else {
                        addMessage("❌ 已取消重置");
                    }
                    m_inputMode = 0;
                    break;
                }
            }
        }
    }
}

void Game::updateGame() {
    auto now = std::chrono::steady_clock::now();

    // 番茄钟计时
    m_timer.tick();

    // 番茄钟完成处理
    if (m_timer.justFinished()) {
        m_timer.clearJustFinished();
        Scene* sc = currentScene();
        if (sc) {
            double hours = m_timer.totalSeconds() / 3600.0;
            if (hours > 0 && m_timer.finishedAs() == PomodoroTimer::State::FOCUSING) {
                sc->addFocusHours(hours);
                m_focusHistory.push_back({std::chrono::system_clock::now(), hours});
                addMessage("\xe2\x9c\x85 \xe4\xb8\x93\xe6\xb3\xa8\xe5\xae\x8c\xe6\x88\x90\xef\xbc\x81" +
                    sc->name() + " +" + std::to_string(static_cast<int>(hours * 60)) + "\xe5\x88\x86\xe9\x92\x9f");

                if (hours >= Scene::MIN_FOCUS_FOR_PEST_RESET) {
                    addMessage("\xf0\x9f\x8c\xbf " + sc->name() + " \xe6\x9d\x82\xe8\x8d\x89\xe8\xae\xa1\xe6\x97\xb6\xe5\x99\xa8\xe5\xb7\xb2\xe9\x87\x8d\xe7\xbd\xae");
                } else {
                    addMessage("⚠ 专注不足20分钟，杂草计时器未重置");
                }

                // 检查是否成熟并入库
                if (sc->currentObject() && sc->currentObject()->isMature()) {
                    switch (sc->type()) {
                        case SceneType::FOREST:
                            m_warehouse.addWood();
                            addMessage("\xf0\x9f\xaa\xb5 \xe4\xbb\x93\xe5\xba\x93\xe6\x9c\xa8\xe5\xa4\xb4 +1\xef\xbc\x81");
                            break;
                        case SceneType::POND:
                            m_warehouse.addFish();
                            addMessage("\xf0\x9f\x90\x9f \xe4\xbb\x93\xe5\xba\x93\xe9\xb1\xbc +1\xef\xbc\x81");
                            break;
                        case SceneType::PASTURE:
                            m_warehouse.addMeat();
                            addMessage("\xf0\x9f\xa5\xa9 \xe4\xbb\x93\xe5\xba\x93\xe8\x82\x89 +1\xef\xbc\x81");
                            break;
                        case SceneType::FIELD:
                            m_warehouse.addCrop();
                            addMessage("\xf0\x9f\x8c\xbd \xe4\xbb\x93\xe5\xba\x93\xe4\xbd\x9c\xe7\x89\xa9 +1\xef\xbc\x81");
                            break;
                        default: break;
                    }
                }

                m_timer.stop();
                addMessage("\xf0\x9f\x98\xb4 \xe8\xa6\x81\xe4\xbc\x91\xe6\x81\xaf\xe4\xb8\x80\xe4\xb8\x8b\xe5\x90\x97\xef\xbc\x9f [A]5min [B]10min [C]15min [D]20min \xe6\x88\x96\xe4\xbb\xbb\xe6\x84\x8f\xe9\x94\xae\xe8\xb7\xb3\xe8\xbf\x87");
                m_inputMode = 7;
            } else if (m_timer.finishedAs() == PomodoroTimer::State::RESTING) {
                m_timer.stop();
                addMessage("\xf0\x9f\x94\x94 \xe4\xbc\x91\xe6\x81\xaf\xe6\x97\xb6\xe9\x97\xb4\xe7\xbb\x93\xe6\x9d\x9f\xef\xbc\x81");
                addMessage("\xe2\x8f\xb1 \xe7\xbb\xa7\xe7\xbb\xad\xe4\xb8\x93\xe6\xb3\xa8\xef\xbc\x9f [A]10min [B]20min [C]30min [D]40min [E]1h [F]1.5h [G]2h \xe6\x88\x96\xe4\xbb\xbb\xe6\x84\x8f\xe9\x94\xae\xe8\xb7\xb3\xe8\xbf\x87");
                m_inputMode = 1;
            }
        }
    }

    // 每60秒检查有害物品
    auto pestElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastPestCheck);
    if (pestElapsed.count() >= 60) {
        m_lastPestCheck = now;

        // 清理超过24小时的专注历史记录
        auto sysNow = std::chrono::system_clock::now();
        auto cutoff = sysNow - std::chrono::hours(24);
        m_focusHistory.erase(
            std::remove_if(m_focusHistory.begin(), m_focusHistory.end(),
                [&](const auto& e) { return e.first < cutoff; }),
            m_focusHistory.end());

        auto checkPests = [&](std::vector<Scene>& scenes) {
            for (auto& s : scenes) {
                s.updatePests();
            }
        };
        checkPests(m_forests);
        checkPests(m_ponds);
        checkPests(m_pastures);
        checkPests(m_fields);

        // 有害物品惩罚检查
        auto checkPunish = [this](std::vector<Scene>& scenes, const std::string& typeName) {
            for (auto& s : scenes) {
                if (s.pestCount() > PEST_PUNISH_THRESHOLD) {
                    int excess = s.pestCount() - PEST_PUNISH_THRESHOLD;
                    int matureCount = 0;
                    for (auto& obj : s.objects()) {
                        if (obj.isMature()) matureCount++;
                    }
                    int toRemove = (std::min)(excess, matureCount);
                    if (toRemove > 0) {
                        addMessage("\xe2\x9a\xa0\xe2\x9a\xa0\xe2\x9a\xa0 " + s.name() +
                            " \xe6\x9c\x89\xe5\xae\xb3\xe7\x89\xa9\xe5\x93\x81\xe8\xbf\x87\xe5\xa4\x9a\xef\xbc\x81\xe6\x8d\x9f\xe5\xa4\xb1\xe4\xba\x86 " +
                            std::to_string(toRemove) + " \xe4\xb8\xaa" + typeName + "\xef\xbc\x81");
                        int removed = 0;
                        for (int i = (int)s.objects().size() - 1; i >= 0 && removed < toRemove; i--) {
                            if (s.objects()[i].isMature()) {
                                s.objects().erase(s.objects().begin() + i);
                                removed++;
                            }
                        }
                    }
                }
            }
        };
        checkPunish(m_forests, "\xe6\xa0\x91\xe6\x9c\xa8");
        checkPunish(m_ponds, "\xe9\xb1\xbc");
        checkPunish(m_pastures, "\xe7\x89\xb2\xe7\x95\x9c");
        checkPunish(m_fields, "\xe4\xbd\x9c\xe7\x89\xa9");
    }
}

void Game::setPestInterval(double hours) {
    m_pestIntervalHours = hours;
    for (auto& f : m_forests) f.setPestInterval(hours);
    for (auto& p : m_ponds) p.setPestInterval(hours);
    for (auto& pa : m_pastures) pa.setPestInterval(hours);
    for (auto& fi : m_fields) fi.setPestInterval(hours);
}

void Game::cyclePestInterval() {
    if (m_pestIntervalHours == 24.0) {
        setPestInterval(48.0);
    } else if (m_pestIntervalHours == 48.0) {
        setPestInterval(72.0);
    } else {
        setPestInterval(24.0);
    }
}

std::string Game::pestIntervalLabel() const {
    if (m_pestIntervalHours == 24.0) return "24h (\xe9\xab\x98\xe5\x8e\x8b\xe5\x8a\x9b)";
    if (m_pestIntervalHours == 48.0) return "48h (\xe4\xb8\xad\xe7\xad\x89\xe5\x8e\x8b\xe5\x8a\x9b)";
    return "72h (\xe4\xbd\x8e\xe5\x8e\x8b\xe5\x8a\x9b)";
}

double Game::focusInLast24h() const {
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - std::chrono::hours(24);
    double total = 0.0;
    for (const auto& entry : m_focusHistory) {
        if (entry.first >= cutoff) {
            total += entry.second;
        }
    }
    return total;
}

void Game::renderUI() {
    clearScreen();

    std::ostringstream ui;

    renderHeader(*this, ui);

    if (m_inMarket) {
        renderMarketUI(*this, ui);
    } else if (m_inWarehouse) {
        renderWarehouseUI(*this, ui);
    } else if (m_inMainMenu) {
        renderMainMenu(*this, ui);
    } else {
        renderSceneUI(*this, ui);
    }

    renderFooter(*this, ui);

    std::cout << ui.str();
    std::cout.flush();
}

// ======================== 持久化 ========================

void Game::saveToFile() {
    std::ofstream ofs(SAVE_FILE);
    if (!ofs) {
        addMessage("❌ 无法保存配置文件！");
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);

    ofs << "# FocusFarm 配置文件\n";
    ofs << "# 保存时间: " << std::ctime(&nowTime);
    ofs << "# 此文件包含所有游戏数据，可用于备份和迁移\n";
    ofs << "# 手动编辑请谨慎，格式错误可能导致数据丢失\n";
    ofs << "# ================================================\n\n";

    ofs << "[game]\n";
    ofs << "currentSceneType=" << static_cast<int>(m_currentSceneType) << "\n";
    ofs << "currentSceneIndex=" << m_currentSceneIndex << "\n";
    ofs << "pestIntervalHours=" << m_pestIntervalHours << "\n";
    ofs << "inMainMenu=" << (m_inMainMenu ? "1" : "0") << "\n";
    ofs << "saveTimestamp=" << std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count() << "\n\n";

    // Warehouse
    ofs << "[warehouse]\n";
    ofs << "wood=" << m_warehouse.wood() << "\n";
    ofs << "fish=" << m_warehouse.fish() << "\n";
    ofs << "meat=" << m_warehouse.meat() << "\n";
    ofs << "crop=" << m_warehouse.crop() << "\n";
    ofs << "herbicide=" << m_warehouse.herbicide() << "\n";
    ofs << "pesticide=" << m_warehouse.pesticide() << "\n";
    ofs << "snakeRepellent=" << m_warehouse.snakeRepellent() << "\n";
    ofs << "cableTie=" << m_warehouse.cableTie() << "\n\n";

    // Focus history
    ofs << "[focusHistory]\n";
    for (const auto& entry : m_focusHistory) {
        ofs << std::chrono::duration_cast<std::chrono::seconds>(
            entry.first.time_since_epoch()).count() << "=" << entry.second << "\n";
    }
    ofs << "\n";

    // Scenes
    auto saveScenes = [&](const std::string& prefix, const std::vector<Scene>& scenes) {
        for (size_t i = 0; i < scenes.size(); i++) {
            ofs << "[" << prefix << ":" << i << "]\n";
            ofs << scenes[i].serialize() << "\n";
        }
    };

    saveScenes("FOREST", m_forests);
    saveScenes("POND", m_ponds);
    saveScenes("PASTURE", m_pastures);
    saveScenes("FIELD", m_fields);

    ofs.close();
    addMessage("💾 配置已保存到 " + std::string(SAVE_FILE));
}

void Game::loadFromFile() {
    std::ifstream ifs(SAVE_FILE);
    if (!ifs) {
        // 首次运行，无配置文件
        return;
    }

    std::map<std::string, std::string> sections;
    std::string line;
    std::string currentSection;
    std::ostringstream currentData;

    while (std::getline(ifs, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;

        if (line[0] == '[' && line.back() == ']') {
            // Save previous section
            if (!currentSection.empty()) {
                sections[currentSection] = currentData.str();
            }
            currentSection = line.substr(1, line.length() - 2);
            currentData.str("");
            currentData.clear();
        } else if (!currentSection.empty()) {
            currentData << line << "\n";
        }
    }
    // Save last section
    if (!currentSection.empty()) {
        sections[currentSection] = currentData.str();
    }
    ifs.close();

    // Parse game section
    auto getGameVal = [&](const std::string& key) -> std::string {
        auto it = sections.find("game");
        if (it == sections.end()) return "";
        const std::string& data = it->second;
        size_t pos = data.find(key + "=");
        if (pos == std::string::npos) return "";
        pos += key.length() + 1;
        size_t end = data.find('\n', pos);
        if (end == std::string::npos) end = data.length();
        return data.substr(pos, end - pos);
    };

    std::string cst = getGameVal("currentSceneType");
    if (!cst.empty()) m_currentSceneType = static_cast<SceneType>(std::stoi(cst));
    std::string csi = getGameVal("currentSceneIndex");
    if (!csi.empty()) m_currentSceneIndex = std::stoi(csi);
    std::string pih = getGameVal("pestIntervalHours");
    if (!pih.empty()) setPestInterval(std::stod(pih));
    m_inMainMenu = (getGameVal("inMainMenu") == "1");

    // Parse warehouse
    auto parseWarehouse = [&]() {
        auto it = sections.find("warehouse");
        if (it == sections.end()) return;
        const std::string& data = it->second;
        auto getVal = [&](const std::string& key) -> int {
            size_t pos = data.find(key + "=");
            if (pos == std::string::npos) return 0;
            pos += key.length() + 1;
            size_t end = data.find('\n', pos);
            if (end == std::string::npos) end = data.length();
            return std::stoi(data.substr(pos, end - pos));
        };
        // Directly set warehouse values - we need to add setters
        // For now, just consume and add
        int w = getVal("wood");
        int f = getVal("fish");
        int m = getVal("meat");
        int c = getVal("crop");
        int h = getVal("herbicide");
        int p = getVal("pesticide");
        int sr = getVal("snakeRepellent");
        int ct = getVal("cableTie");
        for (int i = 0; i < w; i++) m_warehouse.addWood();
        for (int i = 0; i < f; i++) m_warehouse.addFish();
        for (int i = 0; i < m; i++) m_warehouse.addMeat();
        for (int i = 0; i < c; i++) m_warehouse.addCrop();
        for (int i = 0; i < h; i++) m_warehouse.addHerbicide();
        for (int i = 0; i < p; i++) m_warehouse.addPesticide();
        for (int i = 0; i < sr; i++) m_warehouse.addSnakeRepellent();
        for (int i = 0; i < ct; i++) m_warehouse.addCableTie();
    };
    parseWarehouse();

    // Parse focus history
    {
        auto it = sections.find("focusHistory");
        if (it != sections.end()) {
            m_focusHistory.clear();
            std::istringstream iss(it->second);
            std::string fline;
            while (std::getline(iss, fline)) {
                if (fline.empty()) continue;
                size_t eq = fline.find('=');
                if (eq != std::string::npos) {
                    auto tp = std::chrono::system_clock::time_point(
                        std::chrono::seconds(std::stoll(fline.substr(0, eq))));
                    double hours = std::stod(fline.substr(eq + 1));
                    m_focusHistory.push_back({tp, hours});
                }
            }
        }
    }

    // Parse scenes
    auto parseScenes = [&](const std::string& prefix, std::vector<Scene>& scenes) {
        // First, count how many scenes
        for (size_t i = 0; ; i++) {
            std::string key = prefix + ":" + std::to_string(i);
            auto it = sections.find(key);
            if (it == sections.end()) break;
            if (i < scenes.size()) {
                scenes[i].deserialize(key, it->second);
            }
        }
    };

    parseScenes("FOREST", m_forests);
    parseScenes("POND", m_ponds);
    parseScenes("PASTURE", m_pastures);
    parseScenes("FIELD", m_fields);

    addMessage("📂 配置已从 " + std::string(SAVE_FILE) + " 加载");
}

void Game::resetAll() {
    // Reset scenes
    m_forests.clear();
    m_ponds.clear();
    m_pastures.clear();
    m_fields.clear();

    // Reinitialize default scenes
    m_forests.push_back(Scene(SceneType::FOREST, "高等数学"));
    m_forests.push_back(Scene(SceneType::FOREST, "大学英语"));
    m_forests.push_back(Scene(SceneType::FOREST, "线性代数"));

    m_ponds.push_back(Scene(SceneType::POND, "文学阅读"));
    m_ponds.push_back(Scene(SceneType::POND, "专业书籍"));

    m_pastures.push_back(Scene(SceneType::PASTURE, "C++编程"));
    m_pastures.push_back(Scene(SceneType::PASTURE, "Python开发"));

    m_fields.push_back(Scene(SceneType::FIELD, "课程设计"));
    m_fields.push_back(Scene(SceneType::FIELD, "开源项目"));

    // Reset warehouse
    m_warehouse = Warehouse();

    // Reset game state
    m_currentSceneType = SceneType::FOREST;
    m_currentSceneIndex = 0;
    m_inMarket = false;
    m_inWarehouse = false;
    m_inMainMenu = true;
    m_pestIntervalHours = 48.0;
    setPestInterval(48.0);

    // Reset timer
    m_timer.stop();

    // Clear history
    m_focusHistory.clear();
    m_messages.clear();

    // Reset timestamps
    m_lastWeatherRefresh = std::chrono::steady_clock::now();
    m_lastPestCheck = std::chrono::steady_clock::now();

    // Delete save file
    std::remove(SAVE_FILE);

    addMessage("🔄 系统已重置为初始状态");
}