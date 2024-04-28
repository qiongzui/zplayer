#include "vrender.h"
#include "dev_factory.h"
#include "zlog.h"

using namespace ZPlayer;

VRender::VRender(void* surface)
: _surface(surface) {

}

VRender::~VRender() {
    _dev->stop();
    _dev->release();
}

int VRender::init() {
    _dev = Dev_Factory::createVdev();
    _dev->setSurface(_surface);
    auto ret = _dev->init();
    if (ret != 0) {
        loge("init render failed");
        return -1;
    }
    ret = _dev->start();
    if (ret != 0) {
        loge("start render failed");
        return -1;
    }
    return 0;
}

void VRender::setSyncHandler(AVSync* avSync) {
    _dev->setSyncHandler(avSync);
}

void VRender::render(AVFrame* frame) {
    _dev->asyncRender(frame);
}

void VRender::screenShot(std::string file) {
#ifdef _WIN32
    // _dev->screenShot(file);
#endif
}