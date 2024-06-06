#pragma once

#include <iostream>
extern "C" {
    #include "libavfilter/avfilter.h"
    #include "libavfilter/buffersink.h"
    #include "libavfilter/buffersrc.h"
}

#include "zplayer_define.h"

namespace ZPlayer {
    class FF_VideoFilter {
    public:
        FF_VideoFilter() = default;
        ~FF_VideoFilter();
        int init(int width, int height, AVPixelFormat format, std::string filter_descr);
        int release();
        int filter(AVFrame* frame, AVFrame* filter_frame);
        int filter(std::shared_ptr<Image> inImage, std::shared_ptr<Image> outImage);

    private:
        AVFilterGraph* _filterGraph = nullptr;
        AVFilterContext* _inFilter = nullptr;
        AVFilterContext* _outFilter = nullptr;

        AVFrame* _inFrame;
        AVFrame* _filterFrame;
    };
}