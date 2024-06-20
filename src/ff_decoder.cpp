#include "ff_decoder.h"
#include "zlog.h"
#include "ztools.h"

using namespace ZPlayer;

void FFDecoder::init(AVStream* stream, bool isVideo) {
    if (!stream) {
        return;
    }
    _isVideo = isVideo;
    
    auto codec = avcodec_find_decoder(stream->codecpar->codec_id);
    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    
    if (codec->type == AVMEDIA_TYPE_VIDEO) {
        type = av_hwdevice_find_type_by_name("d3d12va");
        
        for (int i = 0;; ++i) {
            auto config = avcodec_get_hw_config(codec, i);
            if (!config) {
                logw("not support dxva2 decoder");
                break;
            }
            if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type) {
                _hw_pix_fmt = config->pix_fmt;
                _hw_frame = av_frame_alloc();
                break;
            }
        }
    }

    _codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(_codecContext, stream->codecpar);

    _renderFrame = av_frame_alloc();
    if (_hw_pix_fmt != AV_PIX_FMT_NONE) {
        auto ret = av_hwdevice_ctx_create(&_hw_device_ctx, type, nullptr, nullptr, 0);
        if (ret < 0) {
            loge("av_hwdevice_ctx_create failed, error: %s", ff_error(ret));
        } else {
            _codecContext->hw_device_ctx = av_buffer_ref(_hw_device_ctx);
        }

        // auto hw_frames_ref = av_hwframe_ctx_alloc(_hw_device_ctx);
        // av_hwframe_ctx_init(hw_frames_ref);
        // auto hw_frames_ctx = reinterpret_cast<AVHWFramesContext*>(hw_frames_ref->buffer);
        // hw_frames_ctx->format = AV_PIX_FMT_D3D11VA_VLD;
        // hw_frames_ctx->width = 1280;
        // hw_frames_ctx->height = 735;
        // av_hwframe_get_buffer(hw_frames_ref, _renderFrame, 0);

        // _codecContext->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
        _isHwDecoder = true;
    }

    avcodec_open2(_codecContext, codec, nullptr);

    _isInit = _codecContext == nullptr? false : true;
}

int FFDecoder::start() {
    _isRunning = true;
    _decodeThread = std::thread(&FFDecoder::decodeThread, this);
    return 0;
}

int FFDecoder::stop() {
    std::lock_guard<std::mutex> lock(_mutex);
    _isRunning = false;
    _cv.notify_all();
    return 0;
}

void FFDecoder::release() {
    if (!_isInit) {
        return;
    }

    stop();

    if (_decodeThread.joinable()) {
        _decodeThread.join();
    }

    if (_codecContext) {
        avcodec_free_context(&_codecContext);
        _codecContext = nullptr;
    }

    if (_hw_device_ctx) {
        av_buffer_unref(&_hw_device_ctx);
        _hw_device_ctx = nullptr;
    }
    
    if (_hw_frame) {
        av_frame_free(&_hw_frame);
    }

    if (_renderFrame) {
        av_frame_free(&_renderFrame);
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
        auto tmpFrame = _hw_pix_fmt != AV_PIX_FMT_NONE? _hw_frame : frame;
        ret = avcodec_receive_frame(_codecContext, tmpFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            loge("avcodec_receive_frame failed, error: %s", ff_error(ret));
            return -1;
        }

        if (_hw_device_ctx && tmpFrame->format == _hw_pix_fmt) {
            av_hwframe_transfer_data(frame, _hw_frame, 0);
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
    _packet_q.emplace(pkt);
    //logi("video: %d, packet_q.size = %d", _isVideo, _packet_q.size());
    _cv.notify_all();
    return 0;
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

void FFDecoder::decodeThread() {
    auto tmpFrame = _isHwDecoder? _hw_frame : _renderFrame;
    int ret = 0;
    AVPacket* packet = nullptr;
    while (_isRunning) {
        {        
            std::unique_lock<std::mutex> lk(_mutex);
            _cv.wait(lk, [&]() {
                return _packet_q.size() > 0 || !_isRunning;
            });

            if (!_isRunning) {
                break;
            }
            
            packet = _packet_q.front();
            _packet_q.pop();
        }

        ret = avcodec_send_packet(_codecContext, packet);

        while (ret > -1) {
            ret = avcodec_receive_frame(_codecContext, tmpFrame);
            if (ret < 0) {
                break;;
            }

            if (_isHwDecoder) {
                av_hwframe_transfer_data(_renderFrame, tmpFrame, 0);
            }

            if (_renderFrame->format == 8) {
                continue;
            }
            if (_callback && _callback->renderCb) {
                logi("renderFrame format: %d", _renderFrame->format);
                _callback->renderCb(_renderFrame);
            }
            av_frame_unref(_renderFrame);
        }
        av_packet_free(&packet);
    }
    
}