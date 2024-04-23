#include "render.h"
#include "vdev_factory.h"
#include "zlog.h"

using namespace ZPlayer;

int ZRender::init(void* surface) {
    _dev = Vdev_Factory::createVdev();
    _dev->setSurface(surface);
    auto ret = _dev->init();
    if (ret != 0) {
        loge("init render failed");
        return ret;
    }
    ret = _dev->start();
    if (ret != 0) {
        loge("start render failed");
        return ret;
    }
    return 0;
}

int ZRender::release() {
    _dev->stop();
    return _dev->release();
}

void ZRender::render(AVFrame* frame) {
    _dev->asyncRender(frame);
}

void ZRender::screenShot(std::string file) {
#ifdef _WIN32
    // _dev->screenShot(file);
#endif
}