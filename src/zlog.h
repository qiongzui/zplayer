#pragma once

#include <iostream>

namespace ZPlayer {
    void logd(const char* fmt, ...);
    void logi(const char* fmt, ...);
    void logw(const char* fmt, ...);
    void loge(const char* fmt, ...);

	char* ff_error(int errnum);
}