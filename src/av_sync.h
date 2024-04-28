#pragma once

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace ZPlayer {
    enum class AVSync_Mode {
        Audio_Base,
        System_Base
    };
    class AVSync {
    public:
        void init();
        void release();

        void start();
        void stop();

        void updateMasterClock(int64_t pts);
        int64_t getMasterClock() { return _master_clock; }
        int64_t getAudioDelay(int64_t pts);
        int64_t getVideoDelay(int64_t pts);
    private:
        int64_t _time_base = -1;
        int64_t _master_clock = -1;

        std::atomic_bool _isRunning = false;

        std::mutex _mutex;
        std::condition_variable _cv;

    };
}