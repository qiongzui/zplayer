#include "vrender.h"
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include "dev_factory.h"
#include "zlog.h"
#include "ztools.h"

using namespace ZPlayer;

VRender::VRender(void* surface)
: _surface(surface) {
    _dev = Dev_Factory::createVdev();
    _dev->setSurface(_surface);
}

VRender::~VRender() {
    _dev->stop();
    _dev->release();

    if (_renderFrame) {
        av_frame_free(&_renderFrame);
        _renderFrame = nullptr;
    }

    if (_filterFrame) {
        av_frame_free(&_filterFrame);
        _filterFrame = nullptr;
    }

    if (_textFrame) {
        av_frame_free(&_textFrame);
        _textFrame = nullptr;
    }
}

int VRender::init() {
    if (!_dev) {
        return -1;
    }
    auto ret = _dev->init();
    if (ret != 0) {
        loge("init render failed");
        return -1;
    }

    _renderFrame = av_frame_alloc();

    _filterFrame = av_frame_alloc();
    _textFrame = av_frame_alloc();
    av_image_alloc(_renderFrame->data, _renderFrame->linesize, _dev->getWidth(), _dev->getHeight(), AV_PIX_FMT_RGBA, 1);
    _renderFrame->width = _dev->getWidth();
    _renderFrame->height = _dev->getHeight();
    _renderFrame->format = AV_PIX_FMT_RGBA;

    _layout = std::make_unique<Layout>();
    if (_layout) {
        _layout->setupLayoutJson(R"(C:\Users\51917\Desktop\test\screen_recorder.json)", _dev->getWidth(), _dev->getHeight(), 2, "������");
    }

    ret = _dev->start();
    if (ret != 0) {
        loge("start render failed");
        return -1;
    }
    return 0;
}

void VRender::setSyncHandler(std::shared_ptr<AVSync> avSync) {
    _dev->setSyncHandler(avSync);
}

void VRender::render(AVFrame* frame) {
    if (_isUseXml) {
        // if (!_videoFilter) {
        //     _videoFilter = std::make_unique<FF_VideoFilter>();
        //     if (_videoFilter) {
        //         char filter_descr[256];
        //         snprintf(filter_descr, sizeof(filter_descr), "drawbox=x=0:y=0:w=%d:h=%d:color=green@1:thickness=10", frame->width, frame->height);
        //         if (_videoFilter->init(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format), filter_descr) != 0) {
        //             loge("init video filter failed");
        //             _videoFilter = nullptr;
        //             return;
        //         }
        //     }
        // }
    }
    if (!_isUseXml || !_layout) {
        _dev->asyncRender(frame);
    } else {
        std::vector<std::shared_ptr<Image>> thumbImages;
        std::shared_ptr<Image> frameImage = std::make_shared<Image>();
        std::shared_ptr<Image> image = std::make_shared<Image>();
        image->data[0] = frame->data[0];
        image->data[1] = frame->data[1];
        image->data[2] = frame->data[2];
        image->stride[0] = frame->linesize[0];
        image->stride[1] = frame->linesize[1];
        image->stride[2] = frame->linesize[2];
        image->width = frame->width;
        image->height = frame->height;
        image->format = frame->format;

        // static int cnt = 0;
        // static bool isShow = false;
        // if (cnt++ % 120 == 0) {
        //     isShow = !isShow;
        // }

        
        if (1) {
            thumbImages.emplace_back(image);
            thumbImages.emplace_back(image);
            thumbImages.emplace_back(image);
            thumbImages.emplace_back(image);
            // thumbImages.emplace_back(image);
            // thumbImages.emplace_back(image);
            // thumbImages.emplace_back(image);
        }

        std::shared_ptr<Image> mainImage = image;
        std::shared_ptr<Image> outImage = std::make_shared<Image>();
        outImage->data[0] = _renderFrame->data[0];
        outImage->data[1] = _renderFrame->data[1];
        outImage->data[2] = _renderFrame->data[2];
        outImage->stride[0] = _renderFrame->linesize[0];
        outImage->stride[1] = _renderFrame->linesize[1];
        outImage->stride[2] = _renderFrame->linesize[2];
        outImage->width = _renderFrame->width;
        outImage->height = _renderFrame->height;
        outImage->format = _renderFrame->format;

        _layout->fillOutputImage(outImage, mainImage, image, 0.1, 0.3, thumbImages);
        // _textFilter->filter(frame, _textFrame);

        _renderFrame->pts = frame->pts;

        _dev->asyncRender(_renderFrame);
        // av_frame_unref(_textFrame);
    }
}

void VRender::screenShot(std::string file) {
#ifdef _WIN32
    // _dev->screenShot(file);
#endif
}