#pragma once

#include <iostream>
#include "adev.h"
#include "av_sync.h"
extern "C" {
    #include "libswresample/swresample.h"
    #include "libavutil/opt.h"
}
namespace ZPlayer {
    class ARender {
    public:
        ARender() = delete;
        ARender(int sample_rate, int channels, int bitDepth);
        ~ARender();
        void render(AVFrame* frame);
        int init();
        int release();
        void setSyncHandler(std::shared_ptr<AVSync> avSync) { _avSync = avSync; }
        void setMute(int channel, bool isMute);
        void setAllMute(bool isMute);
        void setvolumePercent(float volumePercent);
        float getvolumePercent();
        void seek(int timestamp) { _seekTimestampMs = timestamp; }
    private:
        std::shared_ptr<Adev> _dev = nullptr;
        int _sampleRate = 0;
        int _channels = 0;
        int _bytes_per_sample = 0;
        bool _isMute = false;
        int _seekTimestampMs = -1;
        SwrContext* _swrContext = nullptr;

        uint8_t** _swrframe = nullptr;
        int _swrlineSize = 0;
        std::shared_ptr<AVSync> _avSync = nullptr;
    };
}