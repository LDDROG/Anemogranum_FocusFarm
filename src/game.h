#ifndef GAME_H
#define GAME_H

#include <string>
#include <vector>
#include <utility>
#include <chrono>
#include <ctime>
#include <sstream>
#include <atomic>

// ======================== 基础枚举类型 ========================

enum class SceneType {
    FOREST,    // 森林 - 学科
    POND,      // 鱼塘 - 读书
    PASTURE,   // 牧场 - 技术栈
    FIELD,     // 稻田 - 实践项目
    MARKET     // 集市
};

enum class GrowthStage {
    SEEDLING,  // 幼苗 🌱
    GROWING,   // 成长中 🍃
    MATURE     // 成熟 🌲
};

enum class TreeType {
    PINE = 0,      // 🌲 松树
    DECIDUOUS = 1, // 🌳 落叶树
    PALM = 2       // 🌴 棕榈树
};

enum class FishType {
    NORMAL = 0,    // 🐟 普通鱼
    TROPICAL = 1,  // 🐠 热带鱼
    PUFFER = 2     // 🐡 河豚
};

enum class AnimalType {
    COW = 0,       // 🐄 牛
    SHEEP = 1,     // 🐑 羊
    PIG = 2,       // 🐖 猪
    CHICKEN = 3    // 🐓 鸡
};

enum class CropType {
    WHEAT = 0,     // 🌾 小麦
    CORN = 1,      // 🌽 玉米
    CARROT = 2,    // 🥕 胡萝卜
    TOMATO = 3,    // 🍅 西红柿
    STRAWBERRY = 4,// 🍓 草莓
    EGGPLANT = 5,  // 🍆 茄子
    CUCUMBER = 6   // 🥒 黄瓜
};

// ======================== 天气信息 ========================

struct WeatherInfo {
    std::string city = "Unknown";
    std::string country = "Unknown";
    std::string weather = "Unknown";
    std::string temp = "N/A";
    std::string humidity = "N/A";
    std::string wind = "N/A";
    bool locationAvailable = true;
};

// ======================== 番茄钟 ========================

class PomodoroTimer {
public:
    enum class State { IDLE, FOCUSING, RESTING };

    PomodoroTimer();

    void startFocus(int minutes);
    void startRest(int minutes);
    void stop();
    void tick();

    State state() const { return m_state; }
    int remainingSeconds() const { return m_remaining; }
    int totalSeconds() const { return m_total; }
    int elapsedSeconds() const { return m_total - m_remaining; }
    bool justFinished() const { return m_justFinished; }
    void clearJustFinished() { m_justFinished = false; }
    State finishedAs() const { return m_finishedAs; }

    std::string stateStr() const;
    std::string timeStr() const;

private:
    State m_state = State::IDLE;
    int m_remaining = 0;
    int m_total = 0;
    bool m_justFinished = false;
    State m_finishedAs = State::IDLE;
    std::chrono::steady_clock::time_point m_lastTick;
};

// ======================== 场景物体 ========================

struct SceneObject {
    GrowthStage stage = GrowthStage::SEEDLING;
    int typeIndex = 0;
    double accumulatedHours = 0.0;       // 累计专注小时数
    static constexpr double HOURS_TO_GROW = 12.0;
    static constexpr double HOURS_TO_MATURE = 24.0;

    bool isMature() const { return stage == GrowthStage::MATURE; }
};

// ======================== 场景 ========================

class Scene {
public:
    Scene(SceneType type, const std::string& name);

    SceneType type() const { return m_type; }
    const std::string& name() const { return m_name; }
    bool locked() const { return m_locked; }
    void setLocked(bool v) { m_locked = v; }
    void setName(const std::string& name) { m_name = name; }

    const std::vector<SceneObject>& objects() const { return m_objects; }
    std::vector<SceneObject>& objects() { return m_objects; }
    const SceneObject* currentObject() const;
    bool canPlant() const;
    void plant(int typeIndex);
    SceneObject& currentOrLastObject();

    int pestCount() const { return m_pestCount; }
    void addPest() { m_pestCount++; }
    bool removePest();

    void addFocusHours(double hours);
    double hoursSinceLastFocus() const;
    void resetLastFocusTime();

    void updatePests();

    void setPestInterval(double hours) { m_pestIntervalHours = hours; }
    double pestInterval() const { return m_pestIntervalHours; }

    int width() const { return m_width; }
    int height() const { return m_height; }
    void expandGrid();

    bool removeLastObject();

    static constexpr double MIN_FOCUS_FOR_PEST_RESET = 20.0 / 60.0;

    std::string pestEmoji() const;
    std::string sceneEmoji() const;

    // 序列化/反序列化
    std::string serialize() const;
    void deserialize(const std::string& id, const std::string& data);

private:
    SceneType m_type;
    std::string m_name;
    bool m_locked = false;
    std::vector<SceneObject> m_objects;
    int m_pestCount = 0;
    int m_width = 3;
    int m_height = 2;
    std::chrono::system_clock::time_point m_lastFocusTime;
    std::chrono::system_clock::time_point m_lastPestCheck;
    double m_pestIntervalHours = 48.0;
};

// ======================== 仓库 ========================

class Warehouse {
public:
    int wood() const { return m_wood; }
    int fish() const { return m_fish; }
    int meat() const { return m_meat; }
    int crop() const { return m_crop; }

    void addWood(int n = 1) { m_wood += n; }
    void addFish(int n = 1) { m_fish += n; }
    void addMeat(int n = 1) { m_meat += n; }
    void addCrop(int n = 1) { m_crop += n; }

    bool consumeWood(int n);
    bool consumeFish(int n);
    bool consumeMeat(int n);
    bool consumeCrop(int n);

    int herbicide() const { return m_herbicide; }
    int pesticide() const { return m_pesticide; }
    int snakeRepellent() const { return m_snakeRepellent; }
    int cableTie() const { return m_cableTie; }

    void addHerbicide(int n = 1) { m_herbicide += n; }
    void addPesticide(int n = 1) { m_pesticide += n; }
    void addSnakeRepellent(int n = 1) { m_snakeRepellent += n; }
    void addCableTie(int n = 1) { m_cableTie += n; }

    bool useHerbicide();
    bool usePesticide();
    bool useSnakeRepellent();
    bool useCableTie();

private:
    int m_wood = 0;
    int m_fish = 0;
    int m_meat = 0;
    int m_crop = 0;
    int m_herbicide = 0;
    int m_pesticide = 0;
    int m_snakeRepellent = 0;
    int m_cableTie = 0;
};

// ======================== 游戏主类 ========================

class Game {
public:
    static Game& instance();

    void run();
    void stop();

    // 持久化
    void saveToFile();
    void loadFromFile();
    void resetAll();

    std::vector<Scene>& forests() { return m_forests; }
    std::vector<Scene>& ponds() { return m_ponds; }
    std::vector<Scene>& pastures() { return m_pastures; }
    std::vector<Scene>& fields() { return m_fields; }
    Warehouse& warehouse() { return m_warehouse; }
    WeatherInfo& weather() { return m_weather; }

    SceneType currentSceneType() const { return m_currentSceneType; }
    int currentSceneIndex() const { return m_currentSceneIndex; }
    Scene* currentScene();
    void setCurrentScene(SceneType type, int index);

    PomodoroTimer& timer() { return m_timer; }

    void addMessage(const std::string& msg);
    std::vector<std::string>& messages() { return m_messages; }

    void refreshWeather();

    bool inMarket() const { return m_inMarket; }
    bool inWarehouse() const { return m_inWarehouse; }
    bool inMainMenu() const { return m_inMainMenu; }
    void setInMarket(bool v) { m_inMarket = v; m_inWarehouse = false; m_inMainMenu = false; }
    void setInWarehouse(bool v) { m_inWarehouse = v; m_inMarket = false; m_inMainMenu = false; }
    void setInMainMenu(bool v) { m_inMainMenu = v; m_inMarket = false; m_inWarehouse = false; }

    // 输入模式: 0=无, 1=专注时长, 2=树木类型, 3=鱼类型, 4=动物类型, 5=作物类型
    int inputMode() const { return m_inputMode; }
    void setInputMode(int mode) { m_inputMode = mode; }
    std::string& inputBuffer() { return m_inputBuffer; }

    double pestIntervalHours() const { return m_pestIntervalHours; }
    void setPestInterval(double hours);
    void cyclePestInterval();
    std::string pestIntervalLabel() const;

    double focusInLast24h() const;

private:
    Game();
    ~Game();

    void mainLoop();
    void processInput();
    void updateGame();
    void renderUI();

    std::vector<Scene> m_forests;
    std::vector<Scene> m_ponds;
    std::vector<Scene> m_pastures;
    std::vector<Scene> m_fields;
    Warehouse m_warehouse;
    WeatherInfo m_weather;

    SceneType m_currentSceneType = SceneType::FOREST;
    int m_currentSceneIndex = 0;
    bool m_inMarket = false;
    bool m_inWarehouse = false;
    bool m_inMainMenu = true;

    PomodoroTimer m_timer;
    std::vector<std::string> m_messages;

    std::chrono::steady_clock::time_point m_lastWeatherRefresh;
    std::chrono::steady_clock::time_point m_lastPestCheck;

    double m_pestIntervalHours = 48.0;
    std::vector<std::pair<std::chrono::system_clock::time_point, double>> m_focusHistory;

    bool m_running = false;

    // 输入模式: 0=无, 1=专注时长, 2=树木类型, 3=鱼类型, 4=动物类型, 5=作物类型, 6=重命名, 7=休息时长, 8=删除确认, 9=重置确认
    int m_inputMode = 0;
    std::string m_inputBuffer;

    static constexpr int WEATHER_REFRESH_SEC = 5;
    static constexpr int PEST_PUNISH_THRESHOLD = 7;
    static constexpr const char* SAVE_FILE = "focusfarm_save.dat";
};

// ======================== UI 渲染函数声明 ========================

void renderHeader(Game& game, std::ostringstream& ui);
void renderMainMenu(Game& game, std::ostringstream& ui);
void renderSceneUI(Game& game, std::ostringstream& ui);
void renderSceneGrid(Game& game, Scene& scene, std::ostringstream& ui);
void renderMarketUI(Game& game, std::ostringstream& ui);
void renderWarehouseUI(Game& game, std::ostringstream& ui);
void renderFooter(Game& game, std::ostringstream& ui);

// ======================== emoji 工具函数 ========================

std::string getTreeEmoji(TreeType t);
std::string getTreeEmoji(TreeType t, GrowthStage s);
std::string getFishEmoji(FishType t);
std::string getFishEmoji(FishType t, GrowthStage s);
std::string getAnimalEmoji(AnimalType t);
std::string getAnimalEmoji(AnimalType t, GrowthStage s);
std::string getCropEmoji(CropType t);
std::string getCropEmoji(CropType t, GrowthStage s);

// ======================== 终端工具函数 ========================

void clearScreen();
void hideCursor();
void showCursor();
std::string colorText(const std::string& text, int color);
std::string boldText(const std::string& text);

constexpr int COLOR_RESET = 0;
constexpr int COLOR_GREEN = 32;
constexpr int COLOR_YELLOW = 33;
constexpr int COLOR_RED = 31;
constexpr int COLOR_CYAN = 36;
constexpr int COLOR_BLUE = 34;
constexpr int COLOR_MAGENTA = 35;

#endif // GAME_H