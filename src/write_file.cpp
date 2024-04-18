#include "write_file.h"

using namespace ZPlayer;

void WriteFile::start(std::string file) {
    _fp = fopen(file.c_str(), "wb+");
}

void WriteFile::stop() {
    if (_fp) {
        fclose(_fp);
        _fp = nullptr;
    }
}

void WriteFile::write(uint8_t* data, int len) {
    fwrite(data, 1, len, _fp);
}