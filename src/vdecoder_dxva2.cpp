#ifdef _WIN32
#include "vdecoder_dxva2.h"

using namespace ZPlayer;

void VDecoder_DXVA2::init(int width, int height, int fps) {
    D3D12_VIDEO_DECODE_CONFIGURATION decodeConfig = {};
    decodeConfig.DecodeProfile = D3D12_VIDEO_DECODE_PROFILE_H264;
    decodeConfig.BitstreamEncryption = D3D12_BITSTREAM_ENCRYPTION_TYPE_NONE;
    decodeConfig.InterlaceType = D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE;

    D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT decodeSupport = {};
    decodeSupport.NodeIndex = 0;
    decodeSupport.Configuration = decodeConfig;
    decodeSupport.Width = width;
    decodeSupport.Height = height;
    decodeSupport.DecodeFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    decodeSupport.FrameRate = {1, static_cast<uint32_t>(fps)};
    decodeSupport.BitRate = 0;


}

void VDecoder_DXVA2::release() {
}

HRESULT VDecoder_DXVA2::createDeviceManager() {

    return S_OK;
}
#endif