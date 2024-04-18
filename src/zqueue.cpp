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

void Packet_Queue::video_filled_enqueue(AVPacket* packet) {
    _video_queue.push(packet);
}

AVPacket* Packet_Queue::video_filled_dequeue() {
    if (!_video_queue.empty()) {
        auto packet = _video_queue.front();
        _video_queue.pop();
        return packet;
    }
    return nullptr;
}

void Packet_Queue::audio_filled_enqueue(AVPacket* packet) {
    _audio_queue.push(packet);
}

AVPacket* Packet_Queue::audio_filled_dequeue() {
    if (!_audio_queue.empty()) {
        auto packet = _audio_queue.front();
        _audio_queue.pop();
        return packet;
    }
    return nullptr;
}

void Frame_Queue::init(int size) {
    for (size_t i = 0; i < size; i++) {
        _empty_queue.push(av_frame_alloc());
    }
}
void Frame_Queue::flush() {
    while (!_filled_queue.empty()) {
        auto frame = _filled_queue.front();
        _empty_queue.push(frame);
        _filled_queue.pop();
    }
}

void Frame_Queue::release() {
    while (!_empty_queue.empty()) {
        auto frame = _empty_queue.front();
        av_frame_free(&frame);
        _empty_queue.pop();
    }

    while (!_filled_queue.empty()) {
        auto frame = _filled_queue.front();
        av_frame_free(&frame);
        _filled_queue.pop();
    }
}

AVFrame* Frame_Queue::empty_dequeue() {
    if (!_empty_queue.empty()) {
        auto frame = _empty_queue.front();
        _empty_queue.pop();
        return frame;
    }
    return nullptr;
}

void Frame_Queue::empty_enqueue(AVFrame* frame) {
    _empty_queue.push(frame);
}

void Frame_Queue::filled_enqueue(AVFrame* frame) {
    _filled_queue.push(frame);
}

AVFrame* Frame_Queue::filled_dequeue() {
    if (!_filled_queue.empty()) {
        auto frame = _filled_queue.front();
        _filled_queue.pop();
        return frame;
    }
    return nullptr;
}