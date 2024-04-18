#pragma once

#include <iostream>
#include <queue>
extern "C" {
#include "libavcodec/avcodec.h"
}

namespace ZPlayer {
    class Packet_Queue {
    public:
        void init(int size = 5);
        void flush();
        void release();

        AVPacket* empty_dequeue();
        void empty_enqueue(AVPacket* packet);

        void video_filled_enqueue(AVPacket* packet);
        AVPacket* video_filled_dequeue();

        void audio_filled_enqueue(AVPacket* packet);
        AVPacket* audio_filled_dequeue();

    private:
        std::queue<AVPacket*> _empty_queue;
        std::queue<AVPacket*> _video_queue;
        std::queue<AVPacket*> _audio_queue;
    };

    class Frame_Queue {
    public:
        void init(int size = 5);
        void flush();
        void release();

        AVFrame* empty_dequeue();
        void empty_enqueue(AVFrame* frame);

        void filled_enqueue(AVFrame* frame);
        AVFrame* filled_dequeue();

    private:
        std::queue<AVFrame*> _empty_queue;
        std::queue<AVFrame*> _filled_queue;
    };
}

