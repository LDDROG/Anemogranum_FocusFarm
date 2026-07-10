#include "game.h"
#include <iostream>
#include <locale>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
static BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    switch (dwCtrlType) {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            // 阻止直接关闭，必须先按 Q 退出
            // 对于 LOGOFF/SHUTDOWN，系统会强制关闭，但我们先保存
            if (dwCtrlType == CTRL_LOGOFF_EVENT || dwCtrlType == CTRL_SHUTDOWN_EVENT) {
                Game::instance().stop();
                return TRUE;
            }
            // 显示提示信息
            std::cout << "\n⚠ 请按 Q 键正常退出程序，而非直接关闭窗口\n";
            std::cout << "按 Q 退出，按其他键继续..." << std::flush;
            return TRUE; // 阻止事件
        default:
            return FALSE;
    }
}
#endif

int main() {
#ifdef _WIN32
    // 设置控制台为 UTF-8 以支持 emoji 和中文字符
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 启用 ANSI 转义序列支持（Windows 10+）
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        dwMode |= ENABLE_PROCESSED_OUTPUT;
        SetConsoleMode(hOut, dwMode);
    }

    // 设置控制台标题
    SetConsoleTitleW(L"🌻 专注农场 - Focus Farm");

    // 注册控制台事件处理器，阻止直接关闭窗口
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
#endif

    try {
        std::locale::global(std::locale(""));
    } catch (const std::runtime_error&) {
        // MinGW on Windows may not support empty locale name
        try {
            std::locale::global(std::locale("C"));
        } catch (...) {}
    }

    clearScreen();

    std::cout << R"(
╔══════════════════════════════════════════════════════════════════════╗
║                                                                      ║
║              🌻  欢迎来到专注农场 - Focus Farm  🌻                  ║
║                                                                      ║
║         用专注浇灌你的农场，让每一分努力都开花结果                    ║
║                                                                      ║
║   📖 森林 = 学科知识    🐟 鱼塘 = 阅读积累                          ║
║   🐄 牧场 = 技术栈      🌾 稻田 = 实践项目                          ║
║   🏪 集市 = 物品兑换                                                ║
║                                                                      ║
║   操作说明：                                                         ║
║   · 方向键浏览场景，Enter 进入                                       ║
║   · 在场景中按 1 开始专注，2 停止专注                                ║
║   · 按 3 种植新物体，4 清除有害物品，D 铲除物体                      ║
║   · M 集市，W 仓库，X 重置系统，Q 保存并退出                         ║
║   · ⚠ 请勿直接关闭窗口，请按 Q 正常退出                             ║
║                                                                      ║
╚══════════════════════════════════════════════════════════════════════╝
    )" << std::endl;

    std::cout << "\n  按 Enter 键开始..." << std::endl;
    std::cin.get();

    Game::instance().run();

    clearScreen();
    std::cout << R"(
╔══════════════════════════════════════════════════════════════════════╗
║                                                                      ║
║                  再见！记得常回来看看你的农场哦 🌻                   ║
║                                                                      ║
╚══════════════════════════════════════════════════════════════════════╝
    )" << std::endl;

    return 0;
}