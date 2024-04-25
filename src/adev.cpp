#include "adev.h"
#include "zlog.h"
#include "ztools.h"

using namespace ZPlayer;

int Adev::init(int sample_rate, int channels, int bytes_per_sample) {
    _sampleRate = sample_rate;
    _channels = channels;
    _bytes_per_sample = bytes_per_sample;
    return 0;
}

int Adev::release() {
    return 0;
}