#pragma once

#include <iostream>

namespace ZPlayer {
    enum class ZPlayer_State {
        Idle,
		Prepared,
		Playing,
		Stopped,
		Paused
    };

    enum class ZPlayer_CodecId {
        H264,
        H265,
        AAC,
        PCM
    };

    enum class ZPlayer_PixelFormat {
        YUV420P,
        RGBA
    };

    enum class ZPlayer_SampleFormat {
        U8,   // packed
        S16,
        F32,
        S16P, // planner
        F32P
    };
}