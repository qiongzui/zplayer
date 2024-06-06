#include "ztools.h"

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#else
#include <sys/time.h>
#endif

void ZPlayer::sleep(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

std::string ZPlayer::convertToUTF8(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
    std::wstring wide = converter.from_bytes(str);
    
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf8conv;
    return utf8conv.to_bytes(wide);
}

int64_t ZPlayer::get_current_timestamp() {
#ifdef _WIN32
    return GetTickCount64();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

tm* ZPlayer::get_current_wall_time() {
    time_t t = time(0);
    tm* now = localtime(&t);
    return now;
}

std::string ZPlayer::get_current_path() {
    char buffer[MAX_PATH];
    _getcwd(buffer, MAX_PATH);
    return std::string(buffer);
}

void ZPlayer::Timer::start(int interval, std::function<void()> callback) {
    running_ = true;
    auto startTime = std::chrono::steady_clock::now();
    while (running_) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();

        if (elapsedTime >= interval) {
            callback();
            startTime = std::chrono::steady_clock::now();
        }
    }
}

void ZPlayer::Timer::stop() {
    running_ = false;
}