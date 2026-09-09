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

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        dwMode |= ENABLE_PROCESSED_OUTPUT;
        SetConsoleMode(hOut, dwMode);
    }

    // 控制台标题
    SetConsoleTitleW(L"🍃 风种子 - 专注农场");

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


    
    std::cout << R"(                                                           
║                                                                      
║        
║        风带来故事的种子，时间使之发芽···                    
║     
║   四种专注场景，可用于不同类别的专注：                                                           
║           📖 森林      🐟 鱼塘                          
║           🐄 牧场      🌾 稻田
║   查看已拥有的作物和物品需要前往： 🏠 仓库
║   可以在以下场景使用作物兑换物品： 🏪 集市                                                                                
║                                                                      
║   操作说明：                                                         
║   · ↑↓←→ 浏览场景，Enter 进入                                       
║   · 按 1 开始专注，按 2 停止专注                                
║   · 按 3 种植作物，按 4 除害，按 D 铲除作物                      
║   · M 集市，W 仓库，X 重置系统，Q 保存并退出                                                   
║   
║                                                                   
    )" << std::endl;

    std::cout << "\n  按 Enter 键开始..." << std::endl;
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