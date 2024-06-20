#pragma once

#include <iostream>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <queue>

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
}

struct DecoderCb {
    std::function<void(AVFrame* frame)> renderCb;
};
namespace ZPlayer {
    class FFDecoder {
    public:
        void init(AVStream* stream, bool isVideo);
        int start();
        int stop();
        void release();
        int flush();

        int decode(AVPacket* pkt, AVFrame* frame);
        int send_packet(AVPacket* pkt);
        bool isInit() { return _isInit; }

        void setCallBack(std::shared_ptr<DecoderCb> cb) { _callback = cb;}
    private:
        void decodeThread();

    private:
        bool _isVideo;

        AVCodecContext* _codecContext = nullptr;
        AVCodecParserContext* _parserContext = nullptr;
        bool _isInit = false;
        std::mutex _mutex;
        AVBufferRef* _hw_device_ctx = nullptr;
        AVPixelFormat _hw_pix_fmt = AV_PIX_FMT_NONE;
        bool _isHwDecoder = false;

        AVFrame* _renderFrame = nullptr;
        AVFrame* _hw_frame = nullptr;
        std::shared_ptr<DecoderCb> _callback = nullptr;

        std::atomic_bool _isRunning = {false};
        std::thread _decodeThread;
        std::condition_variable _cv;
        int _decodingPktCnt = 0;

        std::queue<AVPacket*> _packet_q;
    };
}