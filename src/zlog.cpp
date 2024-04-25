#include "zlog.h"

#include <stdarg.h>
#include <stdio.h>
#include <locale>
#include <codecvt>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"0

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

inline std::string unicode2utf8(const std::wstring& in)	// = unicode => utf-8
{
    std::string ret;
    try {
        std::wstring_convert< std::codecvt_utf8<wchar_t> > wcv;
        ret = wcv.to_bytes(in);
    }
    catch (const std::exception&) {
        ;
    }
    return ret;
}

const char* ZPlayer::win_error(DWORD errnum) {
    char messageBuffer[256];
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errnum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)messageBuffer, 0, NULL);
 
    return messageBuffer;
}
#endif // _WIN32
