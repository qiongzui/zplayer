#include "vdev.h"
#include "zlog.h"
#include "ztools.h"

using namespace ZPlayer;

void Vdev::setOutputSize(int w, int h) {
    _width = w;
    _height = h;
}

int Vdev::init() {
    _frameQueue = std::make_shared<Frame_Queue>(5);
    return 0;
}

int Vdev::start() {
    _isRunning = true;
    _thread = std::thread(&Vdev::renderThread, this);
    return 0;
}

int Vdev::stop() {
    _isRunning = false;
    _cv.notify_all();
    return 0;
}

int Vdev::release() {
    if (_thread.joinable()) {
        _thread.join();
    }

    if (_swsContext) {
        sws_freeContext(_swsContext);
        _swsContext = nullptr;
    }

    _frameQueue->release();

    return 0;
}

int Vdev::asyncRender(AVFrame* frame) {
    if (_frameQueue->get_empty_queue_size() == 0) {
        return -1;
    }

    int bufferSize = 0;
    if (frame->width != _width || frame->height != _height) {
        if (!_swsContext) {
            _swsContext = sws_getContext(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                _width, _height, AV_PIX_FMT_RGBA,
                SWS_BICUBIC, NULL, NULL, NULL);
        }

        if (_swsContext) {
            auto pFrame = _frameQueue->empty_dequeue();
            if (pFrame && !pFrame->data[0]) {
                bufferSize = av_image_alloc(pFrame->data, pFrame->linesize, _width, _height, AV_PIX_FMT_RGBA, 1);
            }
            if (pFrame && pFrame->data) {
                sws_scale(_swsContext, frame->data, frame->linesize, 0, frame->height, pFrame->data, pFrame->linesize);
                std::lock_guard<std::mutex> lock(_mutex);
                _frameQueue->full_enqueue(pFrame);
                _cv.notify_all();
            }
        }
    } else {
        auto pFrame = _frameQueue->empty_dequeue();
        if (pFrame && !pFrame->data[0]) {
            av_image_alloc(pFrame->data, pFrame->linesize, _width, _height, AV_PIX_FMT_RGBA, 1);
        }
        if (pFrame && pFrame->data) {
            memcpy_s(pFrame->data[0], pFrame->linesize[0], frame->data[0], frame->linesize[0]);
            std::lock_guard<std::mutex> lock(_mutex);
            _frameQueue->full_enqueue(pFrame);
            _cv.notify_all();
        }
    }
    return 0;
}

void Vdev::renderThread() {
    while (_isRunning) {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [&]() {
            return _frameQueue->get_full_queue_size() || !_isRunning;
            });
        if (!_isRunning) {
            break;
        }
        auto pFrame = _frameQueue->full_dequeue();
        if (pFrame) {
            render(pFrame->data[0], pFrame->linesize[0] * _height);
        }
        _frameQueue->empty_enqueue(pFrame);
    }
    logi("renderThread exit");
}