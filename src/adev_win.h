#pragma once

#ifdef _WIN32
#include <windows.h>
#include <wrl.h>
using namespace Microsoft::WRL;
using namespace Microsoft;

#define XAUDIO2_HELPER_FUNCTIONS
#include <XAudio2.h>

#pragma comment(lib, "XAudio2.lib")

#include "adev.h"

namespace ZPlayer {
    // XAudio2 Callback Class
    struct BufferContext {
        void* handle;
        int index;
        uint8_t* data;
        int len;
        int64_t pts;
    };
    
    class XAudio2Callback : public IXAudio2VoiceCallback {
    public:
        HANDLE _bufferEndEvent;
        XAudio2Callback() : _bufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL)) {}
        ~XAudio2Callback() { CloseHandle(_bufferEndEvent); }

        // called when the voice has finished playing a buffer
        void OnBufferEnd(void* pBufferContext) override;
        void OnVoiceError(void* pBufferContext, HRESULT error) override;

        // unused
        void OnVoiceProcessingPassStart(UINT32) override {}
        void OnVoiceProcessingPassEnd() override {}
        void OnStreamEnd() override {}
        void OnBufferStart(void* pBufferContext) override {}
        void OnLoopEnd(void* pBufferContext) override {}
    };


    class Adev_win : public Adev {
    public:
        int init(int sample_rate, int channels, int bytes_per_sample) override;
        int release() override;
        void setvolumePercent(float volumePercent) override;
        float getvolumePercent() override;
        void setMute(int channel, bool isMute) override;
        void setAllMute(bool isMute) override;
        int render(uint8_t* data, int len, int64_t pts) override;

        void addAvailableIndex(int index) { _available_index_q.push(index); }
    private:
        ComPtr<IXAudio2> _xaudio2 = nullptr;
        IXAudio2MasteringVoice* _masteringVoice = nullptr;
        IXAudio2SourceVoice* _sourceVoice = nullptr;

        XAudio2Callback _callback;

        std::vector<XAUDIO2_BUFFER> _buffers;
        std::vector<BufferContext> _bufferContexts;
        std::queue<int> _available_index_q;
        std::mutex _mutex;

        float _volume = 1.0f; // XAudio2AmplitudeRatio

        FILE* _fp = nullptr;
    };



}
#endif