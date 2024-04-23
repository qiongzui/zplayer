#ifdef _WIN32
#pragma once

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

#include <wrl.h>
using namespace Microsoft::WRL;
using namespace Microsoft;
#include <dxgi1_6.h>
#include <DirectXMath.h>
using namespace DirectX;

#include <d3d12.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

#if defined(DEBUG)
#include <dxgidebug.h>
#endif

#include "d3dx12.h"

#include "vdev.h"

#define GRS_WND_TITLE _T("ZPlayer")
#define GRS_WND_CLASS_NAME _T("ZPlayer")

namespace ZPlayer {
    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT2 uv;
    };

    class Vdev_win : public Vdev {
        public:
            void setSurface(void* surface) override;
            int init() override;
            int release() override;
            int render(uint8_t* data, int len) override;
        private:
            int initPipeline();
            int initResource();
            std::vector<UINT8> generateTextureData();
            int waitForPreviousFrame();
            int populateCommandList(uint8_t* frame, int size);
        private:
            const static UINT _frameBackBufCount = 3u;
            int _textureWidth = 0;
            int _textureHeight = 0;
            int _textureChannel = 0;

            UINT _frameIndex = 0;
            UINT _frameCnt = 0;

            
            UINT _rtvDescriptorSize = 0U;

            HWND _hwnd = nullptr;
            MSG _msg = { 0 };

            float _aspectRatio = 3.0f;

            D3D12_VERTEX_BUFFER_VIEW _vertexBufferView = {};

            UINT64 _fenceValue = 0ui64;
            HANDLE _fenceEvent = nullptr;

            CD3DX12_VIEWPORT _viewport = {};
            CD3DX12_RECT _scissorRect = {};

            
            ComPtr<IDXGIAdapter1> _dxgiAdapter = nullptr;
            ComPtr<IDXGISwapChain3> _swapChain = nullptr;
            ComPtr<ID3D12Device4> _device = nullptr;
            ComPtr<ID3D12CommandQueue> _commandQueue = nullptr;
            ComPtr<ID3D12CommandAllocator> _commandAllocator = nullptr;
            ComPtr<ID3D12GraphicsCommandList> _commandList = nullptr;
            ComPtr<ID3D12DescriptorHeap> _rtvHeap = nullptr;
            ComPtr<ID3D12DescriptorHeap> _srvHeap = nullptr;
            ComPtr<ID3D12Resource2> _renderTargets[_frameBackBufCount] = {};
            ComPtr<ID3D12RootSignature> _rootSignature = nullptr;
            ComPtr<ID3D12PipelineState> _pipelineState = nullptr;
            ComPtr<ID3D12Resource> _vertexBuffer = nullptr;
            ComPtr<ID3D12Fence1> _fence = nullptr;
            ComPtr<ID3D12Resource> _texcute = nullptr;
            ComPtr<ID3D12Resource> _textureUpload = nullptr;

            // render params

    };
}
#endif
