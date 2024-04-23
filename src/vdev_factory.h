#pragma once

#ifdef _WIN32
#include "vdev_win.h"
#endif

namespace ZPlayer {
    class Vdev_Factory {
    public:
        static std::shared_ptr<Vdev> createVdev() {
            #ifdef _WIN32
                return std::make_shared<Vdev_win>();
            #else
                return nullptr;
            #endif
        }
    };
}