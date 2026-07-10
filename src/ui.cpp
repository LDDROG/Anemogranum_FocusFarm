#include "game.h"
#include <iomanip>
#include <algorithm>

// ======================== 顶部横幅 ========================

void renderHeader(Game& game, std::ostringstream& ui) {
    ui << "╔══════════════════════════════════════════════════════════════════════╗\n";
    ui << "║" << boldText("  🌻 专注农场 - Focus Farm 🌻") << "                                  ║\n";
    ui << "╚══════════════════════════════════════════════════════════════════════╝\n\n";

    // 时间
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_info;
#ifdef _WIN32
    localtime_s(&tm_info, &now_c);
#else
    localtime_r(&now_c, &tm_info);
#endif

    ui << "┌─────────────────────────────────────────────────────────────────────┐\n";
    ui << "│ 📅 " << std::put_time(&tm_info, "%Y年%m月%d日 %H:%M:%S");
    ui << "                                                  │\n";

    auto& w = game.weather();
    if (w.locationAvailable) {
        ui << "│ 📍 " << w.city << ", " << w.country;
        // 填充空格
        int pad = 55 - (int)(w.city.length() + w.country.length());
        for (int i = 0; i < pad; i++) ui << " ";
        ui << "│\n";
        ui << "│ 🌤 " << w.weather << "  🌡 " << w.temp << "°C  💧 " << w.humidity
           << "%  💨 " << w.wind << "km/h";
        pad = 55 - (int)(w.weather.length() + w.temp.length() + w.humidity.length() + w.wind.length());
        for (int i = 0; i < pad; i++) ui << " ";
        ui << "│\n";
    } else {
        ui << "│ ⚠ 无法获取位置信息，请检查Windows位置设置是否开启                    │\n";
    }
    ui << "└─────────────────────────────────────────────────────────────────────┘\n\n";

    // 番茄钟
    auto& t = game.timer();
    ui << "┌────────────────────── 番茄钟 ──────────────────────────┐\n";
    ui << "│ 状态: " << t.stateStr();
    if (t.state() != PomodoroTimer::State::IDLE) {
        ui << "  ⏱ " << t.timeStr();
        int total = t.totalSeconds();
        int remaining = t.remainingSeconds();
        int progress = total > 0 ? (total - remaining) * 20 / total : 0;
        ui << "  [";
        for (int i = 0; i < 20; i++) {
            ui << (i < progress ? "█" : "░");
        }
        ui << "]";
    }
    ui << "        │\n";
    ui << "└───────────────────────────────────────────────────────┘\n\n";
}

// ======================== 主菜单 ========================

void renderMainMenu(Game& game, std::ostringstream& ui) {
    ui << "┌────────────────────── 🗺 地图界面 ────────────────────────┐\n";
    ui << "│ 杂草生成速度: " << game.pestIntervalLabel();
    int pad = 51 - (int)game.pestIntervalLabel().length();
    for (int i = 0; i < pad; i++) ui << " ";
    ui << "│\n";
    ui << "│                                                            │\n";

    // 四个场景卡片
    auto renderCard = [&](SceneType type, const std::string& emoji, const std::string& label,
                          const std::string& key, bool selected) {
        std::string prefix = selected ? "▶ " : "  ";
        ui << "│ " << prefix << "[" << key << "] " << emoji << " " << label;
        // 填充
        int pad = 48 - (int)label.length();
        for (int i = 0; i < pad; i++) ui << " ";
        ui << "│\n";

        // 显示子场景列表
        std::vector<Scene>* scenes = nullptr;
        switch (type) {
            case SceneType::FOREST: scenes = &game.forests(); break;
            case SceneType::POND: scenes = &game.ponds(); break;
            case SceneType::PASTURE: scenes = &game.pastures(); break;
            case SceneType::FIELD: scenes = &game.fields(); break;
            default: break;
        }
        if (scenes) {
            for (int i = 0; i < (int)scenes->size(); i++) {
                auto& s = (*scenes)[i];
                bool subSelected = selected && (i == game.currentSceneIndex());
                ui << "│    " << (subSelected ? "→ " : "  ") << s.name()
                   << (s.locked() ? " 🔒" : "");
                if (s.pestCount() > 0) {
                    ui << " " << s.pestEmoji() << "x" << s.pestCount();
                }
                int objCount = (int)s.objects().size();
                if (objCount > 0) {
                    ui << " (" << objCount << "个)";
                }
                int pad2 = 44 - (int)s.name().length();
                for (int j = 0; j < pad2; j++) ui << " ";
                ui << "│\n";
            }
        }
    };

    SceneType curType = game.currentSceneType();
    renderCard(SceneType::FOREST, "🌲", "森林 - 学科知识", "1", curType == SceneType::FOREST);
    ui << "│                                                            │\n";
    renderCard(SceneType::POND, "🐟", "鱼塘 - 阅读积累", "2", curType == SceneType::POND);
    ui << "│                                                            │\n";
    renderCard(SceneType::PASTURE, "🐄", "牧场 - 技术栈", "3", curType == SceneType::PASTURE);
    ui << "│                                                            │\n";
    renderCard(SceneType::FIELD, "🌾", "稻田 - 实践项目", "4", curType == SceneType::FIELD);

    ui << "│                                                            │\n";
    ui << "└────────────────────────────────────────────────────────────┘\n";
}

// ======================== 场景网格 ========================

void renderSceneGrid(Game& game, Scene& scene, std::ostringstream& ui) {
    const auto& objects = scene.objects();
    int count = (int)objects.size();
    int w = scene.width();
    int h = scene.height();

    // 确保网格至少 1x1
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    ui << "  ┌";
    for (int x = 0; x < w * 3 + 1; x++) ui << "─";
    ui << "┐\n";

    for (int y = 0; y < h; y++) {
        ui << "  │";
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            if (idx < count) {
                const auto& obj = objects[idx];
                std::string emoji;
                switch (scene.type()) {
                    case SceneType::FOREST:
                        emoji = getTreeEmoji(static_cast<TreeType>(obj.typeIndex), obj.stage);
                        break;
                    case SceneType::POND:
                        emoji = getFishEmoji(static_cast<FishType>(obj.typeIndex), obj.stage);
                        break;
                    case SceneType::PASTURE:
                        emoji = getAnimalEmoji(static_cast<AnimalType>(obj.typeIndex), obj.stage);
                        break;
                    case SceneType::FIELD:
                        emoji = getCropEmoji(static_cast<CropType>(obj.typeIndex), obj.stage);
                        break;
                    default: emoji = "?";
                }
                ui << " " << emoji << " ";
            } else {
                ui << " · ";
            }
        }
        ui << "│\n";
    }

    ui << "  └";
    for (int x = 0; x < w * 3 + 1; x++) ui << "─";
    ui << "┘\n";

    // 有害物品显示
    if (scene.pestCount() > 0) {
        ui << "  " << scene.pestEmoji() << " x" << scene.pestCount() << "\n";
    }
}

// ======================== 场景界面 ========================

void renderSceneUI(Game& game, std::ostringstream& ui) {
    Scene* scene = game.currentScene();
    if (!scene) {
        ui << "  场景不存在\n";
        return;
    }

    // 场景标题
    std::string sceneIcon;
    switch (scene->type()) {
        case SceneType::FOREST: sceneIcon = "🌲"; break;
        case SceneType::POND: sceneIcon = "🐟"; break;
        case SceneType::PASTURE: sceneIcon = "🐄"; break;
        case SceneType::FIELD: sceneIcon = "🌾"; break;
        default: sceneIcon = "?"; break;
    }

    ui << "┌────────────────────── " << sceneIcon << " " << scene->name() << " ────────────────────────┐\n";
    ui << "│ 🔒 状态: " << (scene->locked() ? "已封锁" : "活跃中");
    ui << "                                                  │\n";
    double f24 = game.focusInLast24h();
    ui << "│ ⏱ 24h内专注: " << static_cast<int>(f24 * 60) << "分钟 / 15小时上限";
    int pad = 36 - (int)std::to_string(static_cast<int>(f24 * 60)).length();
    for (int i = 0; i < pad; i++) ui << " ";
    ui << "│\n";

    const auto* obj = scene->currentObject();
    if (obj) {
        double progress = obj->accumulatedHours / SceneObject::HOURS_TO_MATURE * 100.0;
        if (progress > 100.0) progress = 100.0;

        std::string stageName;
        switch (obj->stage) {
            case GrowthStage::SEEDLING: stageName = "幼苗 🌱"; break;
            case GrowthStage::GROWING: stageName = "成长中 🌿"; break;
            case GrowthStage::MATURE: stageName = "已成熟 ✓"; break;
        }

        ui << "│ 🌱 阶段: " << stageName;
        int pad = 49 - (int)stageName.length();
        for (int i = 0; i < pad; i++) ui << " ";
        ui << "│\n";

        ui << "│ ⏳ 进度: " << std::fixed << std::setprecision(1) << progress << "%  [";
        int bar = (int)(progress / 5.0);
        for (int i = 0; i < 20; i++) ui << (i < bar ? "█" : "░");
        ui << "]     │\n";

        ui << "│ 🕐 累计: " << std::fixed << std::setprecision(1)
           << obj->accumulatedHours << " / " << SceneObject::HOURS_TO_MATURE << " 小时";
        ui << "                               │\n";
    } else {
        ui << "│ 🌱 当前: 空地（请先种植！）                                    │\n";
    }

    ui << "│ " << scene->pestEmoji() << " 有害物: " << scene->pestCount() << " 个";
    ui << "                                                  │\n";

    // 场景切换信息
    ui << "│                                                              │\n";
    ui << "│ 子场景: ";
    std::vector<Scene>* scenes = nullptr;
    switch (scene->type()) {
        case SceneType::FOREST: scenes = &game.forests(); break;
        case SceneType::POND: scenes = &game.ponds(); break;
        case SceneType::PASTURE: scenes = &game.pastures(); break;
        case SceneType::FIELD: scenes = &game.fields(); break;
        default: break;
    }
    if (scenes) {
        for (int i = 0; i < (int)scenes->size(); i++) {
            if (i == game.currentSceneIndex()) {
                ui << "[" << (*scenes)[i].name() << "] ";
            } else {
                ui << (*scenes)[i].name() << " ";
            }
        }
    }
    ui << "    │\n";
    ui << "│ ↑↓ 切换子场景  ←→ 切换场景类型                                │\n";
    ui << "└──────────────────────────────────────────────────────────────┘\n\n";

    // 场景网格
    renderSceneGrid(game, *scene, ui);
}

// ======================== 集市场景 ========================

void renderMarketUI(Game& game, std::ostringstream& ui) {
    auto& wh = game.warehouse();

    ui << "┌────────────────────── 🏪 集市 ──────────────────────────┐\n";
    ui << "│                                                            │\n";
    ui << "│  ┌──────────────────────────────────────────────────────┐  │\n";
    ui << "│  │              兑换列表（3:1 比例）                     │  │\n";
    ui << "│  ├──────────────────────────────────────────────────────┤  │\n";
    ui << "│  │ [1] 🪵 木头 x3  →  🧪 除草剂 x1    (清除🌿杂草)      │  │\n";
    ui << "│  │     仓库木头: " << wh.wood() << " 个                                  │  │\n";
    ui << "│  ├──────────────────────────────────────────────────────┤  │\n";
    ui << "│  │ [2] 🌽 作物 x3  →  💊 农药 x1      (清除🐛虫子)      │  │\n";
    ui << "│  │     仓库作物: " << wh.crop() << " 个                                  │  │\n";
    ui << "│  ├──────────────────────────────────────────────────────┤  │\n";
    ui << "│  │ [3] 🥩 肉 x3    →  🪤 驱蛇剂 x1    (清除🐍蛇)        │  │\n";
    ui << "│  │     仓库肉: " << wh.meat() << " 个                                    │  │\n";
    ui << "│  ├──────────────────────────────────────────────────────┤  │\n";
    ui << "│  │ [4] 🐟 鱼 x3    →  🔗 轧带 x1      (清除🪸珊瑚)      │  │\n";
    ui << "│  │     仓库鱼: " << wh.fish() << " 个                                    │  │\n";
    ui << "│  └──────────────────────────────────────────────────────┘  │\n";
    ui << "│                                                            │\n";
    ui << "│  当前持有除害物品:                                          │\n";
    ui << "│  🧪 除草剂 x" << wh.herbicide() << "  💊 农药 x" << wh.pesticide()
       << "  🪤 驱蛇剂 x" << wh.snakeRepellent() << "  🔗 轧带 x" << wh.cableTie();
    ui << "        │\n";
    ui << "└────────────────────────────────────────────────────────────┘\n";
}

// ======================== 仓库界面 ========================

void renderWarehouseUI(Game& game, std::ostringstream& ui) {
    auto& wh = game.warehouse();

    ui << "┌────────────────────── 🏠 仓库 ──────────────────────────┐\n";
    ui << "│                                                            │\n";
    ui << "│  ┌──────────────────── 库存 ────────────────────────┐     │\n";
    ui << "│  │  🪵 木头  x" << wh.wood();
    int pad = 45 - std::to_string(wh.wood()).length();
    for (int i = 0; i < pad; i++) ui << " ";
    ui << "│     │\n";
    ui << "│  │  🐟 鱼    x" << wh.fish();
    pad = 45 - std::to_string(wh.fish()).length();
    for (int i = 0; i < pad; i++) ui << " ";
    ui << "│     │\n";
    ui << "│  │  🥩 肉    x" << wh.meat();
    pad = 45 - std::to_string(wh.meat()).length();
    for (int i = 0; i < pad; i++) ui << " ";
    ui << "│     │\n";
    ui << "│  │  🌽 作物  x" << wh.crop();
    pad = 45 - std::to_string(wh.crop()).length();
    for (int i = 0; i < pad; i++) ui << " ";
    ui << "│     │\n";
    ui << "│  └──────────────────────────────────────────────────┘     │\n";
    ui << "│                                                            │\n";
    ui << "│  ┌────────────────── 除害物品 ──────────────────────┐     │\n";
    ui << "│  │  🧪 除草剂 x" << wh.herbicide() << "   [1] 使用（清除🌿杂草）";
    pad = 30 - std::to_string(wh.herbicide()).length();
    for (int i = 0; i < pad; i++) ui << " ";
    ui << "│     │\n";
    ui << "│  │  💊 农药   x" << wh.pesticide() << "   [2] 使用（清除🐛虫子）";
    pad = 30 - std::to_string(wh.pesticide()).length();
    for (int i = 0; i < pad; i++) ui << " ";
    ui << "│     │\n";
    ui << "│  │  🪤 驱蛇剂 x" << wh.snakeRepellent() << "   [3] 使用（清除🐍蛇）";
    pad = 30 - std::to_string(wh.snakeRepellent()).length();
    for (int i = 0; i < pad; i++) ui << " ";
    ui << "│     │\n";
    ui << "│  │  🔗 轧带   x" << wh.cableTie() << "   [4] 使用（清除🪸珊瑚）";
    pad = 30 - std::to_string(wh.cableTie()).length();
    for (int i = 0; i < pad; i++) ui << " ";
    ui << "│     │\n";
    ui << "│  └──────────────────────────────────────────────────┘     │\n";
    ui << "│                                                            │\n";
    ui << "└────────────────────────────────────────────────────────────┘\n";
}

// ======================== 底部操作提示 ========================

void renderFooter(Game& game, std::ostringstream& ui) {
    ui << "\n┌────────────────────── 操作提示 ──────────────────────────┐\n";

    if (game.inMarket()) {
        ui << "│ [1]木头→除草剂 [2]作物→农药 [3]肉→驱蛇剂 [4]鱼→轧带      │\n";
        ui << "│ [B]返回主菜单  [Q]退出                                    │\n";
    } else if (game.inWarehouse()) {
        ui << "│ [1]使用除草剂 [2]使用农药 [3]使用驱蛇剂 [4]使用轧带       │\n";
        ui << "│ [B]返回主菜单  [Q]退出                                    │\n";
    } else if (game.inMainMenu()) {
        ui << "│ [1-4]选择场景 [↑↓]切换子场景 [Enter]进入 [M]集市 [W]仓库 [S]速度 [X]重置 │\n";
        ui << "│ [Q]保存并退出                                               │\n";
    } else {
        ui << "│ [1]专注 [2]停止 [3]种植 [4]清除 [D]铲除 [0]休息 [L]封锁 [R]重命名 │\n";
        ui << "│ [B]地图 [M]集市 [W]仓库 [X]重置 [↑↓←→]切换 [Q]保存退出      │\n";
    }

    ui << "└──────────────────────────────────────────────────────────┘\n";

    // 消息日志
    ui << "\n┌────────────────────── 消息日志 ──────────────────────────┐\n";
    auto& msgs = game.messages();
    int msgCount = (int)msgs.size();
    int start = (std::max)(0, msgCount - 5);
    for (int i = start; i < msgCount; i++) {
        ui << "│ " << msgs[i];
        int len = (int)msgs[i].length();
        // 简单填充（不处理emoji宽度）
        for (int j = 0; j < 55 - len && j < 55; j++) ui << " ";
        ui << "│\n";
    }
    if (msgCount == 0) {
        ui << "│ 欢迎来到专注农场！按方向键浏览，Enter进入场景。            │\n";
    }
    ui << "└──────────────────────────────────────────────────────────┘\n";
}