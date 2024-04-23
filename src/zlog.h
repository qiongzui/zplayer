#pragma once

#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace ZPlayer {
    void logd(const char* fmt, ...);
    void logi(const char* fmt, ...);
    void logw(const char* fmt, ...);
    void loge(const char* fmt, ...);

	char* ff_error(int errnum);
#ifdef _WIN32
    const char* win_error(DWORD errnum);
#endif // _WIN32
}