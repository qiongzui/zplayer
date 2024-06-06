#include "ff_video_filter.h"
#include "zlog.h"

using namespace ZPlayer;

FF_VideoFilter::~FF_VideoFilter() {
    release();
}
int FF_VideoFilter::init(int width, int height, AVPixelFormat format, std::string filter_descr) {
    char args[512];
    int ret = 0;
    const AVFilter* buffersrc = avfilter_get_by_name("buffer");
    const AVFilter* buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut* inputs = avfilter_inout_alloc();
    AVFilterInOut* outputs = avfilter_inout_alloc();

    _filterGraph = avfilter_graph_alloc();
    if (!inputs || !outputs || !_filterGraph) {
        goto end;
    }

    snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=1/15360:pixel_aspect=0/1",
            width, height, format);
    ret = avfilter_graph_create_filter(&_inFilter, buffersrc, "in", args, nullptr, _filterGraph);
    if (ret < 0) {
        goto end;
    }

    ret = avfilter_graph_create_filter(&_outFilter, buffersink, "out", nullptr, nullptr, _filterGraph);
    if (ret < 0) {
        goto end;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = _inFilter;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = _outFilter;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    ret = avfilter_graph_parse_ptr(_filterGraph, filter_descr.c_str(), &inputs, &outputs, nullptr);
    if (ret < 0) {
        goto end;
    }

    ret = avfilter_graph_config(_filterGraph, nullptr);
    if (ret < 0) {
        goto end;
    }
end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return ret;
}

int FF_VideoFilter::release() {
    if (_filterGraph) {
        avfilter_graph_free(&_filterGraph);
        _filterGraph = nullptr;
    }

    if (_inFrame) {
        av_frame_free(&_inFrame);
        _inFrame = nullptr;
    }

    if (_filterFrame) {
        av_frame_free(&_filterFrame);
        _filterFrame = nullptr;
    }
    
    _inFilter = nullptr;
    _outFilter = nullptr;
    return 0;
}

int FF_VideoFilter::filter(AVFrame* frame, AVFrame* filter_frame) {
    if (_inFilter == nullptr || _outFilter == nullptr) {
        return -1;
    }
    int ret = av_buffersrc_add_frame_flags(_inFilter, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0) {
        return ret;
    }

    while (true) {
        ret = av_buffersink_get_frame(_outFilter, filter_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }

        if (ret < 0) {
            loge("av_buffersink_get_frame error: %s", ff_error(ret));
            return ret;
        }
    }
    return 0;
}

int FF_VideoFilter::filter(std::shared_ptr<Image> inImage, std::shared_ptr<Image> outImage) {
    if (!_inFrame) {
        _inFrame = av_frame_alloc();
    }
    if (!_filterFrame) {
        _filterFrame = av_frame_alloc();
    }

    if (inImage->format == AV_PIX_FMT_YUV420P) {
        _inFrame->data[0] = inImage->data[0];
        _inFrame->data[1] = inImage->data[1];
        _inFrame->data[2] = inImage->data[2];

        _inFrame->linesize[0] = inImage->stride[0];
        _inFrame->linesize[1] = inImage->stride[1];
        _inFrame->linesize[2] = inImage->stride[2];

        _inFrame->width = inImage->width;
        _inFrame->height = inImage->height;
        _inFrame->format = inImage->format;
    } else {
        _inFrame->data[0] = inImage->data[0];
        _inFrame->linesize[0] = inImage->stride[0];
        _inFrame->width = inImage->width;
        _inFrame->height = inImage->height;
        _inFrame->format = inImage->format;
    }

    if (filter(_inFrame, _filterFrame) < 0) {
		return -1;
	}

	if (_filterFrame->format == AV_PIX_FMT_YUV420P) {
        if (outImage->data[0] && outImage->data[1] && outImage->data[2]) {
            memcpy(outImage->data[0], _filterFrame->data[0], _filterFrame->linesize[0] * _filterFrame->height);
            memcpy(outImage->data[1], _filterFrame->data[1], _filterFrame->linesize[1] * _filterFrame->height / 2);
            memcpy(outImage->data[2], _filterFrame->data[2], _filterFrame->linesize[2] * _filterFrame->height / 2);
        } else {
            outImage->data[0] = _filterFrame->data[0];
            outImage->data[1] = _filterFrame->data[1];
            outImage->data[2] = _filterFrame->data[2];
        }

		outImage->stride[0] = _filterFrame->linesize[0];
		outImage->stride[1] = _filterFrame->linesize[1];
		outImage->stride[2] = _filterFrame->linesize[2];

		outImage->width = _filterFrame->width;
		outImage->height = _filterFrame->height;
		outImage->format = _filterFrame->format;
	} else {
        if (outImage->data[0]) {
            memcpy(outImage->data[0], _filterFrame->data[0], _filterFrame->linesize[0] * _filterFrame->height);
        } else {
            outImage->data[0] = _filterFrame->data[0];
        }
		outImage->stride[0] = _filterFrame->linesize[0];
		outImage->width = _filterFrame->width;
		outImage->height = _filterFrame->height;
		outImage->format = _filterFrame->format;
	}
    av_frame_unref(_filterFrame);
}