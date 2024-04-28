#pragma once

#include <iostream>

#include "zplayer_define.h"

namespace ZPlayer {
    class VDecoder {
    public:
        void init(int width, int height, ZPlayer_CodecId codecId, ZPlayer_PixelFormat pixelFormat);
        void release();

        // int decode(AVPacket* pkt, AVFrame* frame);
        // int send_packet(AVPacket* pkt);
        // int receive_frame(AVFrame* frame);
        // bool isInit() { return _isInit; }
    };
}