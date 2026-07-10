cmake_minimum_required(VERSION 3.16)
project(FocusFarm VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
endif()

add_executable(FocusFarm
    src/main.cpp
    src/game.cpp
    src/ui.cpp
    src/weather.cpp
)

target_include_directories(FocusFarm PRIVATE ${CMAKE_SOURCE_DIR}/src)

if(WIN32)
    target_link_libraries(FocusFarm PRIVATE winhttp ws2_32)
endif()