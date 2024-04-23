#pragma once

#include <iostream>
#include <codecvt>
#include <locale>
#include <string>

namespace ZPlayer {
    void sleep(int ms);

    std::string convertToUTF8(const std::string& str);

    int64_t get_current_timestamp();

    std::string get_current_path();
}