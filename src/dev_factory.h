#pragma once

#ifdef _WIN32
#include "vdev_win.h"
#include "adev_win.h"
#endif

namespace ZPlayer {
    class Dev_Factory {
    public:
        static std::shared_ptr<Vdev> createVdev() {
            #ifdef _WIN32
                return std::make_shared<Vdev_win>();
            #else
                return nullptr;
            #endif
        }
        static std::shared_ptr<Adev> createAdev() {
            #ifdef _WIN32
                return std::make_shared<Adev_win>();
            #else
                return nullptr;
            #endif
        }
    };
}