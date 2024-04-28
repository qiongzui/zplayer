#pragma once

#include <iostream>
#include <queue>
#include <atomic>
#include <mutex>
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
}

namespace ZPlayer {
    class Packet_Queue {
    public:
        void init(int size = 5);
        void flush();
        void release();

        AVPacket* empty_dequeue();
        void empty_enqueue(AVPacket* packet);

        void video_full_enqueue(AVPacket* packet);
        AVPacket* video_full_dequeue();

        void audio_full_enqueue(AVPacket* packet);
        AVPacket* audio_full_dequeue();

        int get_empty_queue_size() const { return _empty_queue.size(); };
        int get_video_full_queue_size() const { return _video_queue.size(); };
        int get_audio_full_queue_size() const { return _audio_queue.size(); };

        int64_t get_video_current_packet_timestamp();
        int64_t get_audio_current_packet_timestamp();

    private:
        std::queue<AVPacket*> _empty_queue;
        std::queue<AVPacket*> _video_queue;
        std::queue<AVPacket*> _audio_queue;

        std::mutex _empty_mutex;
        std::mutex _video_mutex;
        std::mutex _audio_mutex;

        std::atomic_bool _isInit = false;
    };

    class Frame_Queue {
    public:
        Frame_Queue(int frame_cnt = 5);
        void flush();
        void release();

        AVFrame* empty_dequeue();
        void empty_enqueue(AVFrame* frame);
        int get_empty_queue_size() const { return _empty_queue.size(); };

        void full_enqueue(AVFrame* frame);
        AVFrame* full_dequeue();
        int get_full_queue_size() const { return _full_queue.size(); };

        int64_t get_full_current_frame_timestamp();

    private:
        std::mutex _mutex;
        int _size = 0;
        std::queue<AVFrame*> _empty_queue;
        std::queue<AVFrame*> _full_queue;
    };
}

