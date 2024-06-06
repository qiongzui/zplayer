#pragma once

#include <iostream>

#include "SoundTouch/SoundTouchDLL.h"

namespace ZPlayer {
    class AudioFilter {
    public:
        void init();
        void release();
    };
}