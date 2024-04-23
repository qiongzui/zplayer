#pragma once

#include <iostream>
#include "vdev_win.h"

extern "C" {
    #include "libswscale/swscale.h"
}

namespace ZPlayer {
    class ZRender{
    public:
        int init(void* surface);
        int release();
        void render(AVFrame* frame);
        void screenShot(std::string file);
        void seek(int timestamp) { _seekTimestampMs = timestamp; }
    private:
        std::shared_ptr<Vdev> _dev = nullptr;
        int _seekTimestampMs = -1;
        bool _isInit = false;
    };
}