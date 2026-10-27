// LDD_ROG 2026.9.9

#include "game.h"
#include <iostream>
#include <locale>

#include <windows.h>

static BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    switch (dwCtrlType) {
        case CTRL_CLOSE_EVENT:
            // 关闭终端自动保存
            Game::instance().stop();
            return FALSE; 
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            // 阻止 Ctrl+C / Ctrl+Break
            std::cout << "\n⚠ 退出请按 Q 键或关闭终端窗口\n";
            std::cout << "按 Q 退出，按其他键继续..." << std::flush;
            return TRUE;
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            Game::instance().stop();
            return FALSE;
        default:
            return FALSE;
    }
}


int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 单实例：两个进程会各自加载存档并轮流覆盖，必然丢进度
    HANDLE hInstanceMutex = CreateMutexA(NULL, TRUE, "Local\\FocusFarm_SingleInstance");
    if (hInstanceMutex != NULL && GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cout << "⚠ 已经有一个 FocusFarm 在运行。" << std::endl;
        std::cout << "  同时开两个会互相覆盖存档、丢失进度，本次已退出。" << std::endl;
        std::cout << "  请先关闭已打开的窗口再试。" << std::endl;
        std::cout << std::endl << "  按任意键退出..." << std::flush;
        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        if (hIn != INVALID_HANDLE_VALUE) {
            FlushConsoleInputBuffer(hIn);
            INPUT_RECORD rec;
            DWORD read = 0;
            while (ReadConsoleInputW(hIn, &rec, 1, &read) && read > 0) {
                if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) break;
            }
        }
        CloseHandle(hInstanceMutex);
        return 1;
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        dwMode |= ENABLE_PROCESSED_OUTPUT;
        SetConsoleMode(hOut, dwMode);
    }

    // 控制台标题
    SetConsoleTitleW(L"🍃 风种子");

    // 注册控制台事件处理器，阻止直接关闭窗口
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    try 
    {
        std::locale::global(std::locale(""));
    } 
    catch (const std::runtime_error&) 
    {
        try {
            std::locale::global(std::locale("C"));
        } catch (...) {}
    }
    clearScreen();


    
    std::cout << "风带来故事的种子，时间使之发芽。\n\n";
    std::cout << "欢迎回来\n\n";
    std::cout << "按 Enter 键进入..." << std::endl;
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    FlushConsoleInputBuffer(hIn);
    while (true) {
        INPUT_RECORD rec;
        DWORD read = 0;
        if (!ReadConsoleInputW(hIn, &rec, 1, &read) || read == 0) break;
        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown &&
            rec.Event.KeyEvent.uChar.UnicodeChar == 13) break;
    }
    FlushConsoleInputBuffer(hIn);  // 清空残留

    Game::instance().run();

    clearScreen();
    std::cout << R"(
╔══════════════════════════════════════════════════════════════════════╗
║                                                                      
║                  再见！记得常回来看看你的农场哦 🍃                     
║                                                                      
╚══════════════════════════════════════════════════════════════════════╝
    )" << std::endl;
    return 0;
}