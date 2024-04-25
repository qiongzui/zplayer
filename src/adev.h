#pragma once

#include <iostream>
#include <thread>
#include <condition_variable>
#include <atomic>
#include "zqueue.h"

namespace ZPlayer {
    class Adev {
    public:
        virtual int init(int sample_rate, int channels, int bytes_per_sample);
        virtual int release();
        virtual void setvolumePercent(float volumePercent) = 0;
        virtual float getvolumePercent() = 0;
        virtual void setMute(int channel, bool isMute) = 0;
        virtual void setAllMute(bool isMute) = 0;
        virtual int render(uint8_t* data, int len, int64_t pts) = 0;
    protected:
        int _sampleRate = 0;
        int _channels = 0;
        int _bytes_per_sample = 0;
        std::atomic_bool _isInit = false;
    };
} // namespace ZPlayer