// LDD_ROG 2026.9.7

#ifndef GAME_H
#define GAME_H

#include <string>
#include <vector>
#include <utility>
#include <chrono>
#include <ctime>
#include <sstream>

enum class SceneType {
    FOREST,   
    POND,      
    PASTURE,   
    FIELD      
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


// 番茄钟
class PomodoroTimer {
    public:
        enum class State { IDLE, FOCUSING, RESTING };

        PomodoroTimer();

        void startFocus(int minutes);
        void startRest(int minutes);
        void stop();
        void tick();
        void pauseTick();
        void restore(State state, int totalSeconds, int remainingSeconds);

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


// 场景中的物体计算
struct SceneObject {
    GrowthStage stage = GrowthStage::SEEDLING;
    int typeIndex = 0;
    double accumulatedHours = 0.0;       // 累计专注小时数
    static constexpr double HOURS_TO_GROW = 12.0;
    static constexpr double HOURS_TO_MATURE = 24.0;

    bool isMature() const { return stage == GrowthStage::MATURE; }
};


struct FocusRecord {
    std::chrono::system_clock::time_point time;
    double hours = 0.0;
    std::string sceneName;
};


//场景
class Scene {
public:
    Scene(SceneType type, const std::string &name);

    SceneType type() const { return m_type; }
    const std::string &name() const { return m_name; }
    bool locked() const { return m_locked; }
    void setLocked(bool v) { m_locked = v; }
    void setName(const std::string &name) { m_name = name; }

    const std::vector<SceneObject> &objects() const { return m_objects; }
    std::vector<SceneObject> &objects() { return m_objects; }
    const SceneObject* currentObject() const;
    bool canPlant() const;
    void plant(int typeIndex);

    int pestCount() const { return m_pestCount; }
    bool removePest();

    void addFocusHours(double hours);

    void updatePests();

    void setPestInterval(double hours) { m_pestIntervalHours = hours; }

    int width() const { return m_width; }
    int height() const { return m_height; }
    void expandGrid();

    bool removeLastObject();

    static constexpr double MIN_FOCUS_FOR_PEST_RESET = 20.0 / 60.0;

    std::string pestEmoji() const;

    // 序列化/反序列化
    std::string serialize() const;
    void deserialize(const std::string &id, const std::string &data);

private:
    SceneType m_type;
    std::string m_name;
    bool m_locked = false;
    std::vector<SceneObject> m_objects;
    int m_pestCount = 0;
    int m_width = 3;
    int m_height = 2;
    std::chrono::system_clock::time_point m_lastPestCheck;
    double m_pestIntervalHours = 48.0;
    double m_overflowHours = 0.0;  // 专注溢出时长，下次种植时自动加给新物体
};


// 仓库
class Warehouse {
public:
    int wood() const { return m_wood; }
    int fish() const { return m_fish; }
    int meat() const { return m_meat; }
    int crop() const { return m_crop; }
    int moon() const { return m_moon; }

    void addWood(int n = 1) { m_wood += n; }
    void addFish(int n = 1) { m_fish += n; }
    void addMeat(int n = 1) { m_meat += n; }
    void addCrop(int n = 1) { m_crop += n; }
    void addMoon(int n = 1) { m_moon += n; }

    bool consumeWood(int n);
    bool consumeFish(int n);
    bool consumeMeat(int n);
    bool consumeCrop(int n);
    bool consumeMoon(int n);

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
    int m_moon = 0;
    int m_herbicide = 0;
    int m_pesticide = 0;
    int m_snakeRepellent = 0;
    int m_cableTie = 0;
};



class Game {
public:
    static Game &instance();

    void run();
    void stop();

    // 持久化
    void saveToFile();
    void loadFromFile();
    void resetAll();

    std::vector<Scene> &forests() { return m_forests; }
    std::vector<Scene> &ponds() { return m_ponds; }
    std::vector<Scene> &pastures() { return m_pastures; }
    std::vector<Scene> &fields() { return m_fields; }
    Warehouse &warehouse() { return m_warehouse; }

    SceneType currentSceneType() const { return m_currentSceneType; }
    int currentSceneIndex() const { return m_currentSceneIndex; }
    Scene* currentScene();
    Scene* getScene(SceneType type, int index);
    std::string pageLabel();

    PomodoroTimer &timer() { return m_timer; }

    void addMessage(const std::string &msg);
    std::vector<std::string> &messages() { return m_messages; }

    bool inMarket() const { return m_inMarket; }
    bool inWarehouse() const { return m_inWarehouse; }
    bool inMainMenu() const { return m_inMainMenu; }
    bool inHistory() const { return m_inHistory; }

    const std::vector<FocusRecord>& focusHistory() const { return m_focusHistory; }
    int focusStreakDays() const;
    double totalFocusHours() const;

    int inputMode() const { return m_inputMode; }
    // 重命名模式（inputMode==6）下正在编辑的新名称
    const std::string& renameBuffer() const { return m_renameBuffer; }

    void setPestInterval(double hours);
    void cyclePestInterval();
    std::string pestIntervalLabel() const;

    double focusInLast24h() const;
    void collectMature(Scene* sc);

    // 荣誉系统：月亮（连续5天满上限奖励）
    void recordFocus(double hours);
    void checkDayRollover();
    int daysSinceMoonAward() const;

    // 月神系统：集齐3个月亮召唤
    bool summonMoonGod();
    bool showMoonGod() const { return m_showMoonGod; }
    void dismissMoonGod() { m_showMoonGod = false; m_uiDirty = true; }
    int moonGodSummons() const { return m_moonGodSummons; }
    void tickMoonGodAnim() { if (m_showMoonGod && m_moonGodAnimFrame < MOON_GOD_ANIM_TOTAL) m_moonGodAnimFrame++; }
    int moonGodAnimFrame() const { return m_moonGodAnimFrame; }
    bool moonGodAnimDone() const { return m_moonGodAnimFrame >= MOON_GOD_ANIM_TOTAL; }
    int moonGodAnimTotal() const { return MOON_GOD_ANIM_TOTAL; }
    bool nightPaused() const { return m_nightPaused; }

private:
    Game();
    ~Game();

    void mainLoop();
    void readInput();
    void processInput();
    void updateGame();
    void renderUI();

    void addFocusRecord(double hours, const std::string& sceneName);

    std::vector<Scene> m_forests;
    std::vector<Scene> m_ponds;
    std::vector<Scene> m_pastures;
    std::vector<Scene> m_fields;
    Warehouse m_warehouse;

    SceneType m_currentSceneType = SceneType::FOREST;
    int m_currentSceneIndex = 0;
    SceneType m_focusSceneType = SceneType::FOREST;  // 专注开始时所在的场景类型
    int m_focusSceneIndex = 0;                       // 专注开始时所在的子场景索引
    bool m_inMarket = false;
    bool m_inWarehouse = false;
    bool m_inMainMenu = true;
    bool m_inHistory = false;

    PomodoroTimer m_timer;
    std::vector<std::string> m_messages;

    std::chrono::steady_clock::time_point m_lastPestCheck;

    double m_pestIntervalHours = 48.0;
    std::vector<FocusRecord> m_focusHistory;
    double m_totalFocusHours = 0.0;
    mutable int m_streakCache = -1;
    mutable std::string m_streakCacheDay;
    mutable size_t m_streakCacheSize = 0;

    // 荣誉系统状态
    std::vector<std::pair<std::string, double>> m_dailyFocus; // 每日专注: 日期 -> 小时
    std::string m_lastDateKey;      // 上次处理日期 YYYY-MM-DD
    std::string m_moonAwardDate;    // 上次获得月亮日期 YYYY-MM-DD
    int m_fullStreak = 0;           // 当前连续满额天数

    // 月神系统状态
    int m_moonGodSummons = 0;       // 月神召唤次数
    bool m_showMoonGod = false;     // 是否展示月神降临画面
    int m_moonGodAnimFrame = 0;     // 月神动画当前帧
    static constexpr int MOON_GOD_ANIM_TOTAL = 50; // 动画总帧数（约5秒）

    bool m_running = false;

    int m_inputMode = 0;
    std::string m_inputBuffer;      // 按键接收缓冲（processInput 后立即清空）
    std::string m_renameBuffer;     // 重命名时正在编辑的新名称（独立，避免与按键缓冲职责冲突）
    bool m_nightPaused = false;     // 夜间暂停，等待用户确认恢复
    bool m_resumePrompted = false;  // 是否已提示恢复询问

    bool m_uiDirty = true;
    int m_lastRenderSecond = -1;

    static constexpr int PEST_PUNISH_THRESHOLD = 7;
    static constexpr const char* SAVE_FILE = "focusfarm_save.dat";
};


// UI渲染函数声明
void title(Game &game, std::ostringstream &ui);
void renderMainMenu(Game &game, std::ostringstream &ui);
void renderSceneUI(Game &game, std::ostringstream &ui);
void renderSceneGrid(Game &game, Scene &scene, std::ostringstream &ui);
void renderMarketUI(Game &game, std::ostringstream &ui);
void renderWarehouseUI(Game &game, std::ostringstream &ui);
void renderHistoryUI(Game &game, std::ostringstream &ui);
void renderFooter(Game &game, std::ostringstream &ui);
void renderMoonGodUI(Game &game, std::ostringstream &ui);

// emoji工具声明
std::string getTreeEmoji(TreeType t);
std::string getTreeEmoji(TreeType t, GrowthStage s);
std::string getFishEmoji(FishType t);
std::string getFishEmoji(FishType t, GrowthStage s);
std::string getAnimalEmoji(AnimalType t);
std::string getAnimalEmoji(AnimalType t, GrowthStage s);
std::string getCropEmoji(CropType t);
std::string getCropEmoji(CropType t, GrowthStage s);

// 终端工具声明
void clearScreen();
void scrollToEnd();
void hideCursor();
void showCursor();

// 时间工具
bool isNightTime();
std::string todayDateKey();
std::string seasonLabel();
std::string boldText(const std::string &text);

#endif