#pragma once

#include <iostream>

namespace ZPlayer {
    class WriteFile {
        public:
            void start(std::string file);
            void stop();
            void write(uint8_t* data, int len);
        private:
            FILE* _fp = nullptr;
    };
}