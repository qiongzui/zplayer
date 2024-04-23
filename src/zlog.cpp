#include "zlog.h"

#include <stdarg.h>
#include <stdio.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

extern "C" {
#include "libavutil/error.h"
}

#if defined(_MSC_VER)
static char av_error[AV_ERROR_MAX_STRING_SIZE] = { 0 };
#define z_err2str(errnum) \
    av_make_error_string(av_error, AV_ERROR_MAX_STRING_SIZE, errnum)
#elif
#define av_err2str(errnum) \
    av_make_error_string((char[AV_ERROR_MAX_STRING_SIZE]){0}, AV_ERROR_MAX_STRING_SIZE, errnum)
#endif

#ifdef _WIN32
#include <windows.h>
auto logger = spdlog::basic_logger_mt("zplayer", R"(log\zplayer_log.txt)", true);
#endif

void ZPlayer::logd(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    spdlog::debug(buffer);
#ifdef _WIN32
    logger->debug(buffer);
#endif
    va_end(args);
}

void ZPlayer::logi(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    spdlog::info(buffer);
#ifdef _WIN32
    logger->info(buffer);
#endif
    va_end(args);
}

void ZPlayer::logw(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    spdlog::warn(buffer);
#ifdef _WIN32
    logger->warn(buffer);
#endif
    va_end(args);
}

void ZPlayer::loge(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    spdlog::error(buffer);
#ifdef _WIN32
    logger->error(buffer);
#endif
    va_end(args);
}

char* ZPlayer::ff_error(int errnum) {
    return z_err2str(errnum);
}

#ifdef _WIN32
const char* ZPlayer::win_error(DWORD errnum) {
    return std::to_string(long long (errnum)).c_str();
}
#endif // _WIN32
