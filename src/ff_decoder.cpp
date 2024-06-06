#include "ff_decoder.h"
#include "zlog.h"

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

int FFDecoder::decode(AVPacket* pkt, AVFrame* frame) {
    if (!_isInit || !pkt || !pkt->data || !pkt->size || !frame) {
        return -1;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    auto ret = avcodec_send_packet(_codecContext, pkt);
    if (ret < 0) {
        loge("avcodec_send_packet failed, error: %s", ff_error(ret));
        return -1;
    }

    while (true) {
        ret = avcodec_receive_frame(_codecContext, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            loge("avcodec_receive_frame failed, error: %s", ff_error(ret));
            return -1;
        }
        return 0;
    }
    return 0;
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

int FFDecoder::flush() {
    avcodec_send_packet(_codecContext, nullptr);

    AVFrame* frame = av_frame_alloc();
    int ret = 0;
    while (ret >= 0) {
        ret = avcodec_receive_frame(_codecContext, frame);
        if (ret < 0) {
            // loge("adecoderThread: receive_frame failed, ret: %d", ret);
            break;
        }
    }

    avcodec_flush_buffers(_codecContext);
    av_frame_free(&frame);
    return ret;
}