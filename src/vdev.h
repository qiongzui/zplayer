#pragma once

#include <iostream>
#include <thread>
#include <condition_variable>
#include <atomic>
#include "zqueue.h"
#include "av_sync.h"
#include "ztools.h"

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libswscale/swscale.h"
}

namespace ZPlayer {
    class Vdev {
    public:
        virtual void setOutputSize(int w, int h);
        virtual void setSurface(void* surface) = 0;
        virtual int init();
        virtual int release();
        virtual int start();
        virtual int stop();
        virtual int asyncRender(AVFrame* frame);

        void setSyncHandler(std::shared_ptr<AVSync> avSync) { _avSync = avSync; }

        int getWidth() { return _width; }
        int getHeight() { return _height; }
    protected:
        virtual int render(uint8_t* data, int len, int64_t pts) = 0;
        int _width = 0;
        int _height = 0;
        std::atomic_bool _isRunning = false;
    private:
        void renderThread();
        std::thread _thread;
        std::condition_variable _cv;
        std::mutex _mutex;
        std::condition_variable _empty_cv;
        std::mutex _empty_mutex;
        std::shared_ptr<Frame_Queue> _frameQueue = nullptr;

        SwsContext* _swsContext = nullptr;
        std::shared_ptr<AVSync> _avSync = nullptr;
    };
} // namespace ZPlayer