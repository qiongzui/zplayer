#pragma once

#include <iostream>
#include "vdev.h"
#include "av_sync.h"

extern "C" {
    #include "libswscale/swscale.h"
}

namespace ZPlayer {
    class VRender{
    public:
        VRender() = delete;
        VRender(void* surface);
        ~VRender();
        int init();
        void setSyncHandler(AVSync* avSync);
        void render(AVFrame* frame);
        void screenShot(std::string file);
        void seek(int timestamp) { _seekTimestampMs = timestamp; }
    private:
        std::shared_ptr<Vdev> _dev = nullptr;
        int _seekTimestampMs = -1;
        bool _isInit = false;
        void* _surface = nullptr;

        
    };
}