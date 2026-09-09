// LDD_ROG 2026.9.9

#include "game.h"
#include "ui.h"
#include "weather.h"

void title(Game &game, std::ostringstream &ui) 
{
    if (isNightTime()) {
        ui << "🌃 夜深了，去睡个觉吧···\n\n";
    }

    ui << boldText("( ‘-ωก̀ ) 专注农场") << "\n";

    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_info;
    localtime_s(&tm_info, &now_c);
    ui << std::put_time(&tm_info, "%Y年%m月%d日 %H:%M:%S") << "\n";
    ui << "当前你在：" << game.pageLabel() << "\n";
    ui << seasonLabel() << "\n";

    auto w = weather();
    if (w.locationAvailable) 
    {
        ui << "📍 " << w.city << ", " << w.country << "\n";
        ui << "⛅ " << w.weather << "  🌡 " << w.temp << "°C  💧 " << w.humidity << "%  💨 " << w.wind << "\n";
    } 
    else
        ui << "⚠ 无法获取位置信息，请检查Windows定位设置\n";
    ui << "\n";

    auto& t = game.timer();
    ui << "🍅 番茄钟: " << t.stateStr();
    if (game.nightPaused()) {
        ui << "  ⏸ 已暂停（等待确认继续）";
    } else if (t.state() != PomodoroTimer::State::IDLE) {
        ui << "  ⏱ " << t.timeStr();
        int total = t.totalSeconds();
        int remaining = t.remainingSeconds();
        int progress = total > 0 ? (total - remaining) * 20 / total : 0;
        ui << "  [";
        for (int i = 0; i < 20; i++) ui << (i < progress ? "█" : "░");
        ui << "]";
    }
    ui << "\n\n";
}

void renderMainMenu(Game& game, std::ostringstream& ui) 
{
    ui << "🗺 地图  杂草生成速度: " << game.pestIntervalLabel() << "\n";
    int dMoon = game.daysSinceMoonAward();
    if (dMoon >= 0 && dMoon < 3) {
        ui << "你上周获得了一个🌙~~再接再厉\n";
    }
    ui << "\n";

    auto renderCard = [&](SceneType type, const std::string& emoji, const std::string& label,
                          const std::string& key, bool selected) {
        ui << (selected ? "▶ " : "  ") << "[" << key << "] " << emoji << " " << label << "\n";
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
                ui << "     " << (subSelected ? "→ " : "  ") << s.name()
                   << (s.locked() ? " 🔒" : "");
                if (s.pestCount() > 0) ui << " " << s.pestEmoji() << "x" << s.pestCount();
                if ((int)s.objects().size() > 0) ui << " (" << (int)s.objects().size() << "个)";
                ui << "\n";
            }
        }
    };

    SceneType curType = game.currentSceneType();
    renderCard(SceneType::FOREST, "··", "森林", "1", curType == SceneType::FOREST);
    ui << "\n";
    renderCard(SceneType::POND, "··", "鱼塘", "2", curType == SceneType::POND);
    ui << "\n";
    renderCard(SceneType::PASTURE, "··", "牧场", "3", curType == SceneType::PASTURE);
    ui << "\n";
    renderCard(SceneType::FIELD, "··", "稻田", "4", curType == SceneType::FIELD);
}

void renderSceneGrid(Game& game, Scene& scene, std::ostringstream& ui) 
{
    (void)game;
    const auto& objects = scene.objects();
    int count = (int)objects.size();
    int w = scene.width();
    int h = scene.height();
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    for (int y = 0; y < h; y++) {
        ui << "  ";
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            if (idx < count) {
                const auto& obj = objects[idx];
                std::string emoji;
                switch (scene.type()) {
                    case SceneType::FOREST: emoji = getTreeEmoji(static_cast<TreeType>(obj.typeIndex), obj.stage); break;
                    case SceneType::POND: emoji = getFishEmoji(static_cast<FishType>(obj.typeIndex), obj.stage); break;
                    case SceneType::PASTURE: emoji = getAnimalEmoji(static_cast<AnimalType>(obj.typeIndex), obj.stage); break;
                    case SceneType::FIELD: emoji = getCropEmoji(static_cast<CropType>(obj.typeIndex), obj.stage); break;
                    default: emoji = "?"; break;
                }
                ui << " " << emoji << " ";
            } else {
                ui << " · ";
            }
        }
        ui << "\n";
    }

    if (scene.pestCount() > 0) {
        ui << "  " << scene.pestEmoji() << " x" << scene.pestCount() << "\n";
    }
}

void renderSceneUI(Game& game, std::ostringstream& ui) 
{
    Scene* scene = game.currentScene();
    if (!scene) {
        ui << "  场景不存在\n";
        return;
    }

    std::string sceneIcon;
    switch (scene->type()) {
        case SceneType::FOREST: sceneIcon = "🌲"; break;
        case SceneType::POND: sceneIcon = "🐟"; break;
        case SceneType::PASTURE: sceneIcon = "🐔"; break;
        case SceneType::FIELD: sceneIcon = "🌾"; break;
        default: sceneIcon = "?"; break;
    }

    if (game.inputMode() == 6)   // 重命名状态
    {
        std::time_t nc = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm* ti = std::localtime(&nc);
        bool cursorOn = (ti && (ti->tm_sec % 2 == 0));
        ui << boldText(sceneIcon + " " + game.renameBuffer())
           << (cursorOn ? "▌" : " ") << "  ✏ 重命名中\n";
    } else {
        ui << boldText(sceneIcon + " " + scene->name()) << "\n";
    }
    ui << "🔒 状态: " << (scene->locked() ? "已封锁" : "活跃中") << "\n";
    double f24 = game.focusInLast24h();
    ui << "⏱ 24h内专注: " << static_cast<int>(f24 * 60) << "分钟 / 15小时上限\n";

    const auto *obj = scene->currentObject();
    if (obj) {
        double progress = obj->accumulatedHours / SceneObject::HOURS_TO_MATURE * 100.0;
        if (progress > 100.0) progress = 100.0;

        std::string stageName;
        switch (obj->stage) {
            case GrowthStage::SEEDLING: stageName = "幼苗 🌱"; break;
            case GrowthStage::GROWING: stageName = "成长中 🌿"; break;
            case GrowthStage::MATURE: stageName = "已成熟 ✓"; break;
        }

        ui << "阶段: " << stageName << "\n";
        ui << "进度: " << std::fixed << std::setprecision(1) << progress << "%  [";
        int bar = (int)(progress / 5.0);
        for (int i = 0; i < 20; i++) ui << (i < bar ? "█" : "░");
        ui << "]\n";
        ui << "累计: " << std::fixed << std::setprecision(1)
           << obj->accumulatedHours << " / " << SceneObject::HOURS_TO_MATURE << " 小时\n";
    } else {
        ui << "当前: 空地（请先种植！）\n";
    }

    ui << scene->pestEmoji() << " 有害物: " << scene->pestCount() << " 个\n\n";

    std::vector<Scene>* scenes = nullptr;
    switch (scene->type()) {
        case SceneType::FOREST: scenes = &game.forests(); break;
        case SceneType::POND: scenes = &game.ponds(); break;
        case SceneType::PASTURE: scenes = &game.pastures(); break;
        case SceneType::FIELD: scenes = &game.fields(); break;
        default: break;
    }
    ui << "子场景: ";
    if (scenes) {
        for (int i = 0; i < (int)scenes->size(); i++) {
            if (i == game.currentSceneIndex()) ui << "[" << (*scenes)[i].name() << "] ";
            else ui << (*scenes)[i].name() << " ";
        }
    }
    ui << "\n";
    ui << "↑↓ 切换子场景  ←→ 切换场景类型\n\n";

    renderSceneGrid(game, *scene, ui);
}

void renderMarketUI(Game& game, std::ostringstream& ui) 
{
    auto& wh = game.warehouse();
    ui << "🏪 集市  兑换比例 3:1\n\n";
    ui << "  [1] 🪵 木头 x3  →  🧪 除草剂 x1  (清除🌿杂草)  仓库木头: " << wh.wood() << " 个\n";
    ui << "  [2] 🌽 作物 x3  →  💊 农药 x1    (清除🐛虫子)  仓库作物: " << wh.crop() << " 个\n";
    ui << "  [3] 🥩 肉 x3    →  🪤 驱蛇剂 x1  (清除🐍蛇)    仓库肉: " << wh.meat() << " 个\n";
    ui << "  [4] 🐟 鱼 x3    →  🔗 轧带 x1    (清除🪸珊瑚)  仓库鱼: " << wh.fish() << " 个\n\n";
    ui << "  当前持有: 🧪 除草剂 x" << wh.herbicide()
       << "  💊 农药 x" << wh.pesticide()
       << "  🪤 驱蛇剂 x" << wh.snakeRepellent()
       << "  🔗 轧带 x" << wh.cableTie() << "\n";
}

void renderWarehouseUI(Game& game, std::ostringstream& ui) 
{
    auto& wh = game.warehouse();
    ui << "🏠 仓库\n\n";
    ui << "  🪵 木头  x" << wh.wood() << "\n";
    ui << "  🐟 鱼    x" << wh.fish() << "\n";
    ui << "  🥩 肉    x" << wh.meat() << "\n";
    ui << "  🌽 作物  x" << wh.crop() << "\n";
    ui << "  🌙 月亮(荣誉)  x" << wh.moon() << "  已召唤月神 " << game.moonGodSummons() << " 次\n\n";
    ui << "  除害物品:\n";
    ui << "  🧪 除草剂 x" << wh.herbicide() << "  [1] 使用(清除🌿杂草)\n";
    ui << "  💊 农药   x" << wh.pesticide() << "  [2] 使用(清除🐛虫子)\n";
    ui << "  🪤 驱蛇剂 x" << wh.snakeRepellent() << "  [3] 使用(清除🐍蛇)\n";
    ui << "  🔗 轧带   x" << wh.cableTie() << "  [4] 使用(清除🪸珊瑚)\n";
}

void renderHistoryUI(Game& game, std::ostringstream& ui) 
{
    const auto& hist = game.focusHistory();

    ui << "📖 专注记录\n\n";

    int totalMins = (int)(game.totalFocusHours() * 60 + 0.5);
    ui << "🎯 累计专注   " << hist.size() << " 次\n";
    ui << "⏱ 累计时长   " << totalMins / 60 << " 小时 " << totalMins % 60 << " 分钟\n";
    ui << "🔥 连续专注   " << game.focusStreakDays() << " 天\n\n";

    ui << "── 最近记录 ──\n";
    if (hist.empty()) 
    {
        ui << "  还没有专注记录，去场景里按 1 开始吧！\n";
        return;
    }

    const int MAX_SHOW = 12;
    int count = (int)hist.size();
    int begin = (std::max)(0, count - MAX_SHOW);

    for (int i = count - 1; i >= begin; i--) 
    {
        const auto& r = hist[i];
        std::time_t t = std::chrono::system_clock::to_time_t(r.time);
        std::tm tm_info;
        localtime_s(&tm_info, &t);

        char buf[32];
        std::snprintf(buf, sizeof(buf), "%02d-%02d %02d:%02d",
                      tm_info.tm_mon + 1, tm_info.tm_mday,
                      tm_info.tm_hour, tm_info.tm_min);

        int mins = (int)(r.hours * 60 + 0.5);
        ui << "  " << buf << "   " << std::setw(3) << mins << " 分钟";
        if (!r.sceneName.empty()) ui << "   " << r.sceneName;
        ui << "\n";
    }

    if (count > MAX_SHOW) 
        ui << "\n  （仅显示最近 " << MAX_SHOW << " 条，共 " << count << " 条）\n";
}

void renderMoonGodUI(Game& game, std::ostringstream& ui) 
{
    int f = game.moonGodAnimFrame();
    bool done = game.moonGodAnimDone();
    int total = game.moonGodAnimTotal();

    ui << "┌────────────────────────────────────────────────────────────┐\n";
    ui << "│                                                            │\n";
    ui << "│                                                            │\n";

    if (!done) {
        int MOVE = total * 7 / 10;  // 移动聚拢阶段占70%（约3.5秒），其余为合体闪烁
        if (f < MOVE) {
            float p = (float)f / (float)MOVE;
            int gap = 12 - (int)(12.0f * p);
            ui << "│";
            for (int i = 0; i < 27 - gap; i++) ui << ' ';
            ui << "🌙";
            for (int i = 0; i < gap; i++) ui << ' ';
            ui << "🌙";
            for (int i = 0; i < gap; i++) ui << ' ';
            ui << "🌙";
            ui << "│\n";
        } else {
            if ((f - MOVE) % 2 == 0) {
                ui << "│                       🌙🌙🌙                              │\n";
            } else {
                ui << "│                         🌕                                │\n";
            }
        }
    } else {
        ui << "│                       ✦   ✦                              │\n";
        ui << "│                     ✦   🌕   ✦                           │\n";
        ui << "│                       ✦   ✦                              │\n";
        ui << "│                                                            │\n";
        ui << "│                    —— 三月交辉之刻 ——                      │\n";
    }
    ui << "│                                                            │\n";
    ui << "│        月神已降临 " << game.moonGodSummons() << " 次";
    ui << "                             │\n";
    ui << "│                                                            │\n";
    ui << "└────────────────────────────────────────────────────────────┘\n";
}

void renderFooter(Game& game, std::ostringstream& ui) {
    ui << "\n── 操作 ──\n";
    int im = game.inputMode();
    if (im == 10) {
        ui << "  [Y]继续计时  [N]停止\n";
    } else if (im == 1) {
        ui << "  [A]10min [B]20min [C]30min [D]40min [E]1h [F]1.5h [G]2h\n";
    } else if (im == 7) {
        ui << "  [A]5min [B]10min [C]15min [D]20min 或任意键跳过\n";
    } else if (im == 6) {
        ui << "  [Enter]确认新名称  [Backspace]删除一个字\n";
    } else if (im == 8) {
        ui << "  [Y]确认铲除  [其他键]取消\n";
    } else if (im == 9) {
        ui << "  [Y]确认重置  [其他键]取消\n";
    } else if (im >= 2 && im <= 5) {
        ui << "  按 A-G 选择类型\n";
    } else if (game.showMoonGod()) {
        ui << "  [任意键] 返回\n";
    } else if (game.inMarket()) {
        ui << "  [1]木头→除草剂 [2]作物→农药 [3]肉→驱蛇剂 [4]鱼→轧带\n";
        ui << "  [B]返回主菜单  [Q]退出\n";
    } else if (game.inWarehouse()) {
        ui << "  [1]使用除草剂 [2]使用农药 [3]使用驱蛇剂 [4]使用轧带\n";
        ui << "  [B]返回主菜单  [Q]退出\n";
    } else if (game.inHistory()) {
        ui << "  [B]返回主菜单  [Q]退出\n";
    } else if (game.inMainMenu()) {
        ui << "  [1-4]选择场景 [Enter]进入 [M]集市 [W]仓库 [H]记录 [S]速度 [X]重置\n";
        ui << "  [Q]保存并退出\n";
    } else {
        Scene* sc = game.currentScene();
        ui << "  [1]专注 [2]停止 [3]种植 [4]除害 [D]铲除作物 [0]休息 [L]"
           << (sc && sc->locked() ? "解锁当前场景" : "封锁当前场景") << " [R]重命名\n";
        ui << "  [B]地图 [M]集市 [W]仓库 [X]重置 [↑↓←→]切换 [Q]保存退出\n";
    }

    ui << "\n── 消息 ──\n";
    auto& msgs = game.messages();
    int msgCount = (int)msgs.size();
    int start = (std::max)(0, msgCount - 5);
    for (int i = start; i < msgCount; i++) {
        ui << "  " << msgs[i] << "\n";
    }
    if (msgCount == 0) {
        ui << "  哒哒哒~ 按方向键浏览，Enter进入场景。\n";
    }
}