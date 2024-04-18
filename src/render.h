#pragma once

#include <iostream>

extern "C" {
    #include "libswscale/swscale.h"
}

namespace ZPlayer {
    class ZRender{
    public:
        void init(void* surface);
        void release();
        void render(AVFrame* frame);
        void screenShot(std::string file);
        void seek(int timestamp) { _seekTimestampMs = timestamp; }
    private:
        int _seekTimestampMs = -1;
        bool _isInit = false;
    };
}