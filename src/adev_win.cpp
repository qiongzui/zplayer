#include "adev_win.h"
#include "zlog.h"
#include "ztools.h"

using namespace ZPlayer;

#ifdef _XBOX // big endian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT  'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif

#ifndef _XBOX // little endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT  ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

#define MAX_DECIBELS 144.5f
#define MAX_BUFFER_QUEUED 5

int Adev_win::init(int sample_rate, int channels, int bytes_per_sample) {
    Adev::init(sample_rate, channels, bytes_per_sample);

    HRESULT hr;
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        loge("CoInitializeEx failed, error: 0x%x", hr);
        return -1;
    }

    if (FAILED(hr = XAudio2Create(&_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        loge("XAudio2Create failed, error: 0x%x", hr);
        return -1;
    }

    if (FAILED(hr = _xaudio2->CreateMasteringVoice(&_masteringVoice))) {
        loge("CreateMasteringVoice failed, error: 0x%x", hr);
        return -1;
    }

    WAVEFORMATEX wfx = { 0 };
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = _channels;
    wfx.nSamplesPerSec = _sampleRate;
    wfx.wBitsPerSample = bytes_per_sample * 8;
    wfx.nBlockAlign = wfx.nChannels * bytes_per_sample;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (FAILED(hr = _xaudio2->CreateSourceVoice(&_sourceVoice, &wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &_callback, nullptr, nullptr))) {
        loge("CreateSourceVoice failed, error: 0x%x",hr);
        return -1;
    }

    logi("createSourceVoice success, params: sample_rate: %d, channels: %d, bits_per_sample: %d, block_align: %d, avg_bytes_per_sec: %d",
     wfx.nSamplesPerSec, wfx.nChannels, wfx.wBitsPerSample, wfx.nBlockAlign, wfx.nAvgBytesPerSec);

    _sourceVoice->Start(0);

    _buffers.resize(MAX_BUFFER_QUEUED);
    _bufferContexts.resize(MAX_BUFFER_QUEUED);
    for (int i = 0; i < MAX_BUFFER_QUEUED; ++i) {
        _available_index_q.push(i);
        _bufferContexts[i].handle = this;
        _bufferContexts[i].index = i;
    }

    _isInit = true;
    return 0;
}

int Adev_win::release() {
    _isInit = false;
    if (_sourceVoice) {
        _sourceVoice->Stop();
        _sourceVoice->DestroyVoice();
        _sourceVoice = nullptr;
    }
    if (_xaudio2) {
        _xaudio2->Release();
        _xaudio2 = nullptr;
    }

    for (auto bufferCtx : _bufferContexts) {
        delete[] bufferCtx.data;
        bufferCtx.data = nullptr;
    }

    CoUninitialize();
    return 0;
}

int Adev_win::render(uint8_t* data, int len, int64_t pts) {
    if (!_isInit) {
        return -1;
    }
    while (_available_index_q.empty()) {
        WaitForSingleObjectEx(_callback._bufferEndEvent, INFINITE, TRUE);
    }

    std::lock_guard<std::mutex> lock(_mutex);
    auto index = _available_index_q.front();
    _available_index_q.pop();

    if (_bufferContexts[index].data == nullptr) {
        _bufferContexts[index].data = new uint8_t[len];
    }

    memcpy(_bufferContexts[index].data, data, len);

    auto buffer = _buffers[index];
    buffer.AudioBytes = len;
    buffer.pAudioData = _bufferContexts[index].data;
    buffer.Flags = 0;

    _bufferContexts[index].pts = pts;
    buffer.pContext = &_bufferContexts[index];

    if (FAILED(_sourceVoice->SubmitSourceBuffer(&buffer))) {
        loge("SubmitSourceBuffer failed");
        return -1;
    }
    

    // XAUDIO2_VOICE_STATE state;
    // while (_sourceVoice->GetState(&state), state.BuffersQueued >= MAX_BUFFER_QUEUED) {
    //     WaitForSingleObjectEx(_callback._bufferEndEvent, INFINITE, TRUE);
    // }
    
    // logi("buffer queued: %d", state.BuffersQueued);
    return 0;
}

void Adev_win::setvolumePercent(float volumePercent) {
    volumePercent = std::fmax(std::fmin(volumePercent, 1.f), 0.0f);

    float volume = MAX_DECIBELS * volumePercent;
    if (_sourceVoice) {
        _volume = XAudio2DecibelsToAmplitudeRatio(volume);
        _sourceVoice->SetVolume(_volume);
    }
}

float Adev_win::getvolumePercent() {
    if (_sourceVoice) {
        return XAudio2AmplitudeRatioToDecibels(_volume) / MAX_DECIBELS;
    }
    return 0.0f;
}

void Adev_win::setMute(int channel, bool isMute) {
    if (_sourceVoice) {
        if (isMute) {
            float volume = 0.f;
            _sourceVoice->SetChannelVolumes(channel, &volume);
        } else {
            _sourceVoice->SetChannelVolumes(channel, &_volume);
        }
    }
}

void Adev_win::setAllMute(bool isMute) {
    if (_sourceVoice) {
        if (isMute) {
            float volume = 0.f;
            _sourceVoice->SetVolume(volume);
        } else {
            _sourceVoice->SetVolume(_volume);
        }
    }
}

void XAudio2Callback::OnBufferEnd(void *pBufferContext) {
    SetEvent(_bufferEndEvent);

    if (pBufferContext) {
        auto buf_ctx = *reinterpret_cast<BufferContext*>(pBufferContext);
        reinterpret_cast<Adev_win*>(buf_ctx.handle)->addAvailableIndex(buf_ctx.index);
        // logi("OnBufferEnd: pts: %lld", buf_ctx.pts);
    }
}

void XAudio2Callback::OnVoiceError(void *pBufferContext, HRESULT error) {
    if (pBufferContext) {
        loge("OnVoiceError: pts: %lld, %s", (*reinterpret_cast<BufferContext*>(pBufferContext)).pts, error);
    } else {
        loge("OnVoiceError: %s", win_error(error));
    }
}