#include "ff_decoder.h"

using namespace ZPlayer;

void FFDecoder::init(AVStream* stream) {
    if (!stream) {
        return;
    }
    
    auto codec = avcodec_find_decoder(stream->codecpar->codec_id);
    _codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(_codecContext, stream->codecpar);
    avcodec_open2(_codecContext, codec, nullptr);

    _isInit = _codecContext == nullptr? false : true;
}

void FFDecoder::release() {
    if (!_isInit) {
        return;
    }

    if (_codecContext) {
        avcodec_free_context(&_codecContext);
        _codecContext = nullptr;
    }

    _isInit = false;
}

int FFDecoder::send_packet(AVPacket* pkt) {
    if (!_isInit || !pkt || !pkt->data || !pkt->size) {
        return -1;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    return avcodec_send_packet(_codecContext, pkt);
}

int FFDecoder::receive_frame(AVFrame* frame) {
    if (!_isInit || !frame) {
        return -1;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    return avcodec_receive_frame(_codecContext, frame);
}