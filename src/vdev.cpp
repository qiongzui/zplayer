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
    AVFrame* swsFrame = nullptr;
    {
        std::unique_lock<std::mutex> lock(_empty_mutex);
        _empty_cv.wait(lock, [&]() {
            return _frameQueue->get_empty_queue_size() > 0 || !_isRunning;
        });
        if (!_isRunning) {
            return -1;
        }

        swsFrame = _frameQueue->empty_dequeue();
    }

    if (!swsFrame) {
        return -1;
    }

    swsFrame->pts = frame->pts;
    int bufferSize = 0;
    if (frame->width != _width || frame->height != _height) {
        if (!_swsContext) {
            _swsContext = sws_getContext(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                _width, _height, AV_PIX_FMT_RGBA,
                SWS_BICUBIC, NULL, NULL, NULL);
        }

        if (_swsContext) {
            if (swsFrame && !swsFrame->data[0]) {
                bufferSize = av_image_alloc(swsFrame->data, swsFrame->linesize, _width, _height, AV_PIX_FMT_RGBA, 1);
            }
            if (swsFrame && swsFrame->data) {
                sws_scale(_swsContext, frame->data, frame->linesize, 0, frame->height, swsFrame->data, swsFrame->linesize);
            }
        }
    } else {
        if (swsFrame && !swsFrame->data[0]) {
            av_image_alloc(swsFrame->data, swsFrame->linesize, _width, _height, AV_PIX_FMT_RGBA, 1);
        }
        if (swsFrame && swsFrame->data) {
            memcpy_s(swsFrame->data[0], swsFrame->linesize[0], frame->data[0], frame->linesize[0]);
        }
    }
    auto start = get_current_timestamp();
    std::unique_lock<std::mutex> lock(_mutex);
    _cv.wait(lock, [&]() {
        return _frameQueue->get_full_queue_size() < 20 || !_isRunning;
    });

    if (!_isRunning) {
        return 0;
    }
    _frameQueue->full_enqueue(swsFrame);
    // _render_q.emplace(_renderFrame);
    _cv.notify_all();

    auto end = get_current_timestamp();
    logi("enqueue time cost: %lld, q_size: %d", end - start, _frameQueue->get_full_queue_size());
    return 0;
}

void Vdev::renderThread() {
    int64_t curPlayTime = 0;
    AVFrame* pFrame = nullptr;
    // _timer->start(10, [&](){
    //     pFrame = _frameQueue->full_dequeue();
    //     _cv.notify_all();
    //     if (pFrame) {
    //         int64_t delay = _avSync->getVideoDelay(pFrame->pts);
    //         if (delay > 0) {
    //             logi("frame pts: %lld, sleep: %lld", pFrame->pts, delay);
    //             // std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    //         }
    //         render(pFrame->data[0], pFrame->linesize[0] * _height, pFrame->pts);
    //     }
    //     _frameQueue->empty_enqueue(pFrame);
    //     _empty_cv.notify_all();
    // });
    while (_isRunning) {
        curPlayTime = _avSync->getMasterClock();
        {        
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait(lock, [&]() {
                return _frameQueue->get_full_queue_size() > 0 || !_isRunning;
            });

            if (!_isRunning) {
                break;
            }

            int64_t delta = _frameQueue->get_full_current_frame_timestamp() - curPlayTime - 2;
            if (delta > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            // pFrame = _render_q.front();
            // _render_q.pop();
            pFrame = _frameQueue->full_dequeue();
            _cv.notify_all();
        }

        // auto pFrame = _frameQueue->full_dequeue();
        if (pFrame) {
            int64_t delay = _avSync->getVideoDelay(pFrame->pts);
            if (delay > 0) {
                logi("frame pts: %lld, sleep: %lld", pFrame->pts, delay);
                // std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
            render(pFrame->data[0], pFrame->linesize[0] * _height, pFrame->pts);
        }
        _frameQueue->empty_enqueue(pFrame);
        _empty_cv.notify_all();
        // av_frame_unref(pFrame);
    }
    logi("renderThread exit");
}