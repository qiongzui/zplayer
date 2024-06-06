#pragma once

#include <iostream>
#include <codecvt>
#include <locale>
#include <string>
#include <functional>
#include <chrono>

namespace ZPlayer {
    void sleep(int ms);

    std::string convertToUTF8(const std::string& str);

    int64_t get_current_timestamp();
    tm* get_current_wall_time();

    std::string get_current_path();

    class Timer {
    public:
    Timer() : running_(false) {}
    void start(int interval, std::function<void()> callback);
    void stop();

private:
    bool running_;
};
}