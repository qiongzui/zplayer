#include "speech_recognizer.h"
#include "zlog.h"

using namespace ZPlayer;

void ps_callback(void* user_data, err_lvl_t level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (level >= ERR_ERROR) {
        loge("ps_error: %s", buffer);
    }
    va_end(args);
}

SpeechRecognizer::SpeechRecognizer() {
    _isRunning = false;
    err_set_callback(ps_callback, nullptr);
    
    _config = ps_config_init(nullptr);
    if (!_config) {
        logw("ps_config_init failed");
        return;
    }
    ps_config_set_str(_config, "hmm", R"(D:\code\myProject\ZPlayer\ZPlayer\deps\cmusphinx-zh-cn-5.2\zh_cn.cd_cont_5000)");
    ps_config_set_str(_config, "lm", R"(D:\code\myProject\ZPlayer\ZPlayer\deps\cmusphinx-zh-cn-5.2\zh_cn.lm.bin)");
    ps_config_set_str(_config, "dict", R"(D:\code\myProject\ZPlayer\ZPlayer\deps\cmusphinx-zh-cn-5.2\zh_cn.dic)");

    _decoder = ps_init(_config);
    if (!_decoder) {
        logw("ps_init failed");
    }
}

SpeechRecognizer::~SpeechRecognizer() {
    if (_decoder) {
        ps_free(_decoder);
        _decoder = nullptr;
    }
    if (_config) {
        ps_config_free(_config);
        _config = nullptr;
    }
}

void SpeechRecognizer::start() {
    if (!_decoder) {
		return;
    }
    _isRunning = true;
    _thread = std::thread(&SpeechRecognizer::process, this);
}

void SpeechRecognizer::stop() {
    if (!_decoder) {
        return;
    }
    _isRunning = false;
    _cv.notify_all();
    if (_thread.joinable()) {
        _thread.join();
    }
}

void SpeechRecognizer::flush() {
    if (!_decoder) {
        return;
    }
    std::unique_lock<std::mutex> lock(_mutex);
    while (_pcmQueue.size() > 0) {
        _pcmQueue.pop();
    }
}

void SpeechRecognizer::recognize(std::shared_ptr<Pcm> pcm) {
    if (!_decoder) {
        return;
    }
    std::unique_lock<std::mutex> lock(_mutex);
    _pcmQueue.push(pcm);
    _cv.notify_all();
}

void SpeechRecognizer::process() {
    std::shared_ptr<Pcm> pcm = nullptr;
    while (_isRunning) {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait(lock, [&]() {
                return !_isRunning || _pcmQueue.size() > 0;
                });
            if (!_isRunning) {
                break;
            }

            pcm = _pcmQueue.front();
            _pcmQueue.pop();
        }
        if (!pcm) {
            continue;
        }

        if (_recognizeFrameSize == 0) {
            ps_start_utt(_decoder);
        }
        
        ps_process_raw(_decoder, reinterpret_cast<int16_t*>(pcm->data), pcm->sample_count, true, false);
        _recognizeFrameSize++;

        if (_recognizeFrameSize == 13) {
            ps_end_utt(_decoder);
            _recognizeFrameSize = 0;
            auto result = ps_get_hyp(_decoder, nullptr);
            if (result) {
                logi("result: %s", result);
            }
        }
    }
}