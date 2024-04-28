#include "av_sync.h"

using namespace ZPlayer;

void AVSync::init() {
    _time_base = -1;
    _master_clock = -1;
}

void AVSync::release() {

}

void AVSync::start() {
    _isRunning = true;
}

void AVSync::stop() {
    _isRunning = false;
    _cv.notify_all();
}

void AVSync::updateMasterClock(int64_t pts) {
    _master_clock = pts;
    if (_time_base < 0) {
        _time_base = pts;
    }
    _cv.notify_all();
}

int64_t AVSync::getAudioDelay(int64_t pts) {
    if (!_isRunning) {
        return -1;
    }
    std::unique_lock<std::mutex> lock(_mutex);
    _cv.wait(lock, [&]() {
        return !_isRunning || (_master_clock >= 0);
    });

    return pts - _master_clock;
}

int64_t AVSync::getVideoDelay(int64_t pts) {
    if (!_isRunning) {
        return -1;
    }
    std::unique_lock<std::mutex> lock(_mutex);
    _cv.wait(lock, [&]() {
        return !_isRunning || (_master_clock >= 0);
    });
    return pts - _master_clock;
}