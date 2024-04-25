#include "arender.h"
#include "dev_factory.h"
#include "zlog.h"
#include "ztools.h"

using namespace ZPlayer;

ARender::ARender(int sample_rate, int channels, int bitDepth)
    : _sampleRate(sample_rate),
      _channels(channels),
      _bytes_per_sample(bitDepth) {

}

ARender::~ARender() {
}
FILE* fp;
int ARender::init() {
    fp = fopen(std::string(get_current_path() + "\\dump\\output.pcm").c_str(), "wb+");
    _dev = Dev_Factory::createAdev();
    if (!_dev) {
        loge("create adev failed");
        return -1;
    }
    auto ret = _dev->init(_sampleRate, _channels, _bytes_per_sample);
    if (ret != 0) {
        loge("init adev failed");
        return -1;
    }
    return 0;
}

int ARender::release() {
    fclose(fp);

    if (_swrframe) {
        av_freep(&_swrframe);
        _swrframe = nullptr;
    }

    if (_swrContext) {
        swr_free(&_swrContext);
        _swrContext = nullptr;
    }
    if (_dev) {
        _dev->release();
        _dev = nullptr;
    }
    return 0;
}

void ARender::render(AVFrame* frame) {
    if (!_dev) {
        return;
    }
    if (av_sample_fmt_is_planar(static_cast<AVSampleFormat>(frame->format))) {
        if (!_swrContext) {
            _swrContext = swr_alloc();
            if (!_swrContext) {
                loge("alloc swr context failed");
                return;
            }
            
            av_opt_set_chlayout(_swrContext, "in_chlayout", &frame->ch_layout, 0);
            av_opt_set_int(_swrContext, "in_sample_rate", frame->sample_rate, 0);
            av_opt_set_sample_fmt(_swrContext, "in_sample_fmt", static_cast<AVSampleFormat>(frame->format), 0);
            av_opt_set_chlayout(_swrContext, "out_chlayout", &frame->ch_layout, 0);
            av_opt_set_int(_swrContext, "out_sample_rate", _sampleRate, 0);
            av_opt_set_sample_fmt(_swrContext, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
            int ret = swr_init(_swrContext);
            if (ret < 0) {
                loge("init swr failed");
                return;
            }

			ret = av_samples_alloc_array_and_samples(&_swrframe, &_swrlineSize, frame->ch_layout.nb_channels, frame->nb_samples, AV_SAMPLE_FMT_S16, 0);
			if (ret < 0) {
				loge("alloc swr frame failed");
				return;
			}
        }

        if (_swrContext) {
            int ret = swr_convert(_swrContext, _swrframe, frame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
            if (ret < 0) {
                loge("swr convert failed");
                return;
            }

            // fwrite(_swrframe[0], 1, _swrlineSize, fp);
            _dev->render(_swrframe[0], _swrlineSize, frame->pts);
        } else {
            _dev->render(frame->data[0], frame->linesize[0], frame->pts);
        }
    }
}

void ARender::setMute(int channel, bool isMute) {
    if (_dev) {
        _dev->setMute(channel, isMute);
    }
}

void ARender::setAllMute(bool isMute) {
    if (_dev) {
        _dev->setAllMute(isMute);
    }
}
void ARender::setvolumePercent(float volumePercent) {
    if (_dev) {
        _dev->setvolumePercent(volumePercent);
    }
}

float ARender::getvolumePercent() {
    if (_dev) {
        return _dev->getvolumePercent();
    }
    return 0.0f;
}