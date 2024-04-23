#include "zqueue.h"

using namespace ZPlayer;

void Packet_Queue::init(int size) {
    for (size_t i = 0; i < size; i++) {
        _empty_queue.push(av_packet_alloc());
    }
}
void Packet_Queue::flush() {
    while (!_video_queue.empty()) {
        auto packet = _video_queue.front();
        _empty_queue.push(packet);
        _video_queue.pop();
    }

    while (!_audio_queue.empty()) {
        auto packet = _audio_queue.front();
        _empty_queue.push(packet);
        _audio_queue.pop();
    }
}

void Packet_Queue::release() {
    while (!_empty_queue.empty()) {
        auto packet = _empty_queue.front();
        av_packet_free(&packet);
        _empty_queue.pop();
    }

    while (!_video_queue.empty()) {
        auto packet = _video_queue.front();
        av_packet_free(&packet);
        _video_queue.pop();
    }

    while (!_audio_queue.empty()) {
        auto packet = _audio_queue.front();
        av_packet_free(&packet);
        _audio_queue.pop();
    }
}

AVPacket* Packet_Queue::empty_dequeue() {
    if (!_empty_queue.empty()) {
        auto packet = _empty_queue.front();
        _empty_queue.pop();
        return packet;
    }
    return nullptr;
}

void Packet_Queue::empty_enqueue(AVPacket* packet) {
    _empty_queue.push(packet);
}

void Packet_Queue::video_full_enqueue(AVPacket* packet) {
    _video_queue.push(packet);
}

AVPacket* Packet_Queue::video_full_dequeue() {
    if (!_video_queue.empty()) {
        auto packet = _video_queue.front();
        _video_queue.pop();
        return packet;
    }
    return nullptr;
}

void Packet_Queue::audio_full_enqueue(AVPacket* packet) {
    _audio_queue.push(packet);
}

AVPacket* Packet_Queue::audio_full_dequeue() {
    if (!_audio_queue.empty()) {
        auto packet = _audio_queue.front();
        _audio_queue.pop();
        return packet;
    }
    return nullptr;
}

Frame_Queue::Frame_Queue(int frame_cnt) {
    std::lock_guard<std::mutex> lock(_mutex);
    for (size_t i = 0; i < frame_cnt; i++) {
        _empty_queue.push(av_frame_alloc());
    }
    _size = frame_cnt;
    _empty_queue_size = _size;
    _full_queue_size = 0;
}
void Frame_Queue::flush() {
    std::lock_guard<std::mutex> lock(_mutex);
    while (!_full_queue.empty()) {
        auto frame = _full_queue.front();
        _empty_queue.push(frame);
        _full_queue.pop();
    }

    _empty_queue_size = _size;
    _full_queue_size = 0;
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

    _empty_queue_size = 0;
    _full_queue_size = 0;
    _size = 0;
}

AVFrame* Frame_Queue::empty_dequeue() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_empty_queue.empty()) {
        auto frame = _empty_queue.front();
        _empty_queue.pop();
        _empty_queue_size--;
        return frame;
    }
    return nullptr;
}

void Frame_Queue::empty_enqueue(AVFrame* frame) {
    std::lock_guard<std::mutex> lock(_mutex);
    _empty_queue.push(frame);
    _empty_queue_size++;
}

void Frame_Queue::full_enqueue(AVFrame* frame) {
    std::lock_guard<std::mutex> lock(_mutex);
    _full_queue.push(frame);
    _full_queue_size++;
}

AVFrame* Frame_Queue::full_dequeue() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_full_queue.empty()) {
        auto frame = _full_queue.front();
        _full_queue.pop();
        _full_queue_size--;
        return frame;
    }
    return nullptr;
}