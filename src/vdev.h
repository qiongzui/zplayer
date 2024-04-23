#pragma once

#include <iostream>
#include <thread>
#include <condition_variable>
#include <atomic>
#include "zqueue.h"

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
    protected:
        virtual int render(uint8_t* data, int len) = 0;
        int _width = 0;
        int _height = 0;
        std::atomic_bool _isRunning = false;
    private:
        void renderThread();
        std::thread _thread;
        std::condition_variable _cv;
        std::mutex _mutex;
        std::shared_ptr<Frame_Queue> _frameQueue = nullptr;

        SwsContext* _swsContext = nullptr;
    };
} // namespace ZPlayer