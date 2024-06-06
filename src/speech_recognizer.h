#pragma once

#include "pocketsphinx.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>

#include "zplayer_define.h"

namespace ZPlayer {
    class SpeechRecognizer {
    public:
        SpeechRecognizer();
        ~SpeechRecognizer();

        void start();
        void stop();
        void flush();

        void recognize(std::shared_ptr<Pcm> pcm);
    private:
        void process();
    private:
        ps_config_t* _config = nullptr;
        ps_decoder_t* _decoder = nullptr;

        std::atomic_bool _isRunning = {false};
        std::thread _thread;
        std::mutex _mutex;

        std::condition_variable _cv;

        std::queue<std::shared_ptr<Pcm>> _pcmQueue;

        int _recognizeFrameSize = 0;
    };
}