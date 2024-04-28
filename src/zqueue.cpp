#include "zqueue.h"
#include "ztools.h"

using namespace ZPlayer;

void Packet_Queue::init(int size) {
    for (size_t i = 0; i < size; i++) {
        _empty_queue.push(av_packet_alloc());
    }
    _isInit = true;
}
void Packet_Queue::flush() {
    if (!_isInit) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(_video_mutex);
        while (!_video_queue.empty()) {
            auto packet = _video_queue.front();
            _empty_queue.push(packet);
            _video_queue.pop();
        }
    }

    {
        std::lock_guard<std::mutex> lock(_audio_mutex);
        while (!_audio_queue.empty()) {
            auto packet = _audio_queue.front();
            _empty_queue.push(packet);
            _audio_queue.pop();
        }
    }
}

void Packet_Queue::release() {
    _isInit = false;
    {
        std::lock_guard<std::mutex> lock(_empty_mutex);
        while (!_empty_queue.empty()) {
            auto packet = _empty_queue.front();
            av_packet_free(&packet);
            _empty_queue.pop();
        }
    }

    {
        std::lock_guard<std::mutex> lock(_video_mutex);
        while (!_video_queue.empty()) {
            auto packet = _video_queue.front();
            av_packet_free(&packet);
            _video_queue.pop();
        }
    }

    {
        std::lock_guard<std::mutex> lock(_audio_mutex);
        while (!_audio_queue.empty()) {
            auto packet = _audio_queue.front();
            av_packet_free(&packet);
            _audio_queue.pop();
        }
    }
}

AVPacket* Packet_Queue::empty_dequeue() {
    if (!_isInit) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(_empty_mutex);
    if (!_empty_queue.empty()) {
        auto packet = _empty_queue.front();
        _empty_queue.pop();
        return packet;
    }
    return nullptr;
}

void Packet_Queue::empty_enqueue(AVPacket* packet) {
    if (!_isInit) {
        return;
    }

    std::lock_guard<std::mutex> lock(_empty_mutex);
    _empty_queue.push(packet);
}

void Packet_Queue::video_full_enqueue(AVPacket* packet) {
    if (!_isInit) {
        return;
    }

    if (_video_queue.size() > 100) {
        sleep(20);
    }
    std::lock_guard<std::mutex> lock(_video_mutex);
    auto pkt = av_packet_clone(packet);
    _video_queue.push(pkt);
}

AVPacket* Packet_Queue::video_full_dequeue() {
    if (!_isInit) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(_video_mutex);
    if (!_video_queue.empty()) {
        auto packet = _video_queue.front();
        _video_queue.pop();
        return packet;
    }
    return nullptr;
}

void Packet_Queue::audio_full_enqueue(AVPacket* packet) {
    if (!_isInit) {
        return;
    }

    if (_audio_queue.size() > 20) {
        // sleep(20);
    }
    std::lock_guard<std::mutex> lock(_audio_mutex);
    auto pkt = av_packet_clone(packet);
    _audio_queue.push(pkt);
}

AVPacket* Packet_Queue::audio_full_dequeue() {
    if (!_isInit) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(_audio_mutex);
    if (!_audio_queue.empty()) {
        auto packet = _audio_queue.front();
        _audio_queue.pop();
        return packet;
    }
    return nullptr;
}

int64_t Packet_Queue::get_video_current_packet_timestamp() {
    std::lock_guard<std::mutex> lock(_video_mutex);
    if (!_video_queue.empty()) {
        auto packet = _video_queue.front();
        return packet->dts;
    }
    return 0;
}

int64_t Packet_Queue::get_audio_current_packet_timestamp() {
    std::lock_guard<std::mutex> lock(_audio_mutex);
    if (!_audio_queue.empty()) {
        auto packet = _audio_queue.front();
        return packet->dts;
    }
    return 0;
}

Frame_Queue::Frame_Queue(int frame_cnt) {
    std::lock_guard<std::mutex> lock(_mutex);
    for (size_t i = 0; i < frame_cnt; i++) {
        _empty_queue.push(av_frame_alloc());
    }
    _size = frame_cnt;
}
void Frame_Queue::flush() {
    std::lock_guard<std::mutex> lock(_mutex);
    while (!_full_queue.empty()) {
        auto frame = _full_queue.front();
        _empty_queue.push(frame);
        _full_queue.pop();
    }
}

void Frame_Queue::release() {
    std::lock_guard<std::mutex> lock(_mutex);
    while (!_empty_queue.empty()) {
        auto frame = _empty_queue.front();
        av_frame_free(&frame);
        _empty_queue.pop();
    }

    while (!_full_queue.empty()) {
        auto frame = _full_queue.front();
        av_frame_free(&frame);
        _full_queue.pop();
    }
    _size = 0;
}

AVFrame* Frame_Queue::empty_dequeue() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_empty_queue.empty()) {
        auto frame = _empty_queue.front();
        _empty_queue.pop();
        return frame;
    }
    return nullptr;
}

void Frame_Queue::empty_enqueue(AVFrame* frame) {
    std::lock_guard<std::mutex> lock(_mutex);
    _empty_queue.push(frame);
}

void Frame_Queue::full_enqueue(AVFrame* frame) {
    std::lock_guard<std::mutex> lock(_mutex);
    _full_queue.push(frame);
}

AVFrame* Frame_Queue::full_dequeue() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_full_queue.empty()) {
        auto frame = _full_queue.front();
        _full_queue.pop();
        return frame;
    }
    return nullptr;
}

int64_t Frame_Queue::get_full_current_frame_timestamp() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_full_queue.empty()) {
        auto frame = _full_queue.front();
        return frame->pts;
    }
    return 0;
}