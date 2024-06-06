#pragma once

#include <iostream>
#include "vdev.h"
#include "av_sync.h"
#include "layout.h"
#include "ff_video_filter.h"

extern "C" {
    #include "libavcodec/avcodec.h"
}

namespace ZPlayer {
    class VRender{
    public:
        VRender() = delete;
        VRender(void* surface);
        ~VRender();
        int init();
        void setSyncHandler(std::shared_ptr<AVSync> avSync);
        void render(AVFrame* frame);
        void screenShot(std::string file);
        void seek(int timestamp) { _seekTimestampMs = timestamp; }
    private:
        std::shared_ptr<Vdev> _dev = nullptr;
        int _seekTimestampMs = -1;
        bool _isInit = false;
        void* _surface = nullptr;
        AVFrame* _filterFrame = nullptr;
        AVFrame* _renderFrame = nullptr;
        AVFrame* _textFrame = nullptr;

        std::unique_ptr<Layout> _layout = nullptr;
        std::unique_ptr<FF_VideoFilter> _videoFilter = nullptr;
        std::unique_ptr<FF_VideoFilter> _textFilter = nullptr;

        bool _isUseXml = true;
    };
}