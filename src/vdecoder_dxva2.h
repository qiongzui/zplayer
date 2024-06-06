#ifdef _WIN32
#pragma once

#include <iostream>
#include <dxva2api.h>
#include <d3d12video.h>


namespace ZPlayer {
    class VDecoder_DXVA2 {
    public:
        void init(int width, int height, int fps);
        void release();

    private:
        HRESULT createDeviceManager();

    };
}

#endif