#pragma once

#include <iostream>
#include <mutex>

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
}

namespace ZPlayer {
    class FFDecoder {
    public:
        void init(AVStream* stream);
        void release();

        int decode(AVPacket* pkt, AVFrame* frame);
        int send_packet(AVPacket* pkt);
        int receive_frame(AVFrame* frame);
        bool isInit() { return _isInit; }
    private:
        AVCodecContext* _codecContext = nullptr;
        AVCodecParserContext* _parserContext = nullptr;
        bool _isInit = false;
        std::mutex _mutex;
    };
}