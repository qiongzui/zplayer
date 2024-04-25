#include "demuxer.h"
#include "zlog.h"

using namespace ZPlayer;



void ZDemuxer::init() {
    if (_url.empty()) {
        return;
    }

    auto ret = avformat_open_input(&_formatContext, _url.c_str(), nullptr, nullptr);
    if (ret != 0) {
        loge("avformat_open_input failed: %s", ff_error(ret));
        return;
    }

    if (avformat_find_stream_info(_formatContext, nullptr) < 0) {
        return;
    }

    ret = av_find_best_stream(_formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (ret >= 0) {
        _videoStreamIndex = ret;
    }
    
    ret = av_find_best_stream(_formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (ret >= 0) {
        _audioStreamIndex = ret;
    }

    _packet = av_packet_alloc();

    _durationMs = _formatContext->duration / AV_TIME_BASE * 1000;
    if (_videoStreamIndex >= 0) {
        auto fps = av_guess_frame_rate(_formatContext, _formatContext->streams[_videoStreamIndex], nullptr);
        _frameRate = fps.num / fps.den;
        logi("video fps: %d", _frameRate);
    }
}

void ZDemuxer::release() {
    if (_packet) {
        av_packet_free(&_packet);
        _packet = nullptr;
    }
    
    if (_formatContext) {
        avformat_close_input(&_formatContext);
        _formatContext = nullptr;
    }

    _videoStreamIndex = -1;
    _audioStreamIndex = -1;
}

void ZDemuxer::readPacket(AVPacket* pkt) {
    if (_formatContext) {
        if (av_read_frame(_formatContext, pkt) < 0) {
            return;
        }
    }
}

void ZDemuxer::seek(int64_t posMs) {
    if (_formatContext) {
        int64_t pos = posMs * _formatContext->streams[0]->time_base.num / _formatContext->streams[0]->time_base.den;
        _seekTimestampMs = posMs;
        av_seek_frame(_formatContext, 0, pos, AVSEEK_FLAG_BACKWARD);
    }
}

AVStream* ZDemuxer::getVideoStream() {
    if (_videoStreamIndex < 0) {
        return nullptr;
    }
    return _formatContext->streams[_videoStreamIndex];
}

AVStream* ZDemuxer::getAudioStream() {
    if (_audioStreamIndex < 0) {
        return nullptr;
    }
    return _formatContext->streams[_audioStreamIndex];
}