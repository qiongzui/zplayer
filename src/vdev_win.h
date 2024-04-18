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

#define GRS_WND_TITLE _T("ZPlayer")
#define GRS_WND_CLASS_NAME _T("ZPlayer")

struct GRS_VERTEX {
    XMFLOAT3 position;
    XMFLOAT4 color;
};

class Vdec_win {
    private:
        const static UINT _frameBackBufCount = 3u;
        int _width = 0;
        int _height = 0;
        UINT _frameIndex = 0;
        UINT _frameCnt = 0;

        UINT _dxgiFactoryFlags = 0U;
        UINT _RTVDescriptorSize = 0U;

        HWND _hwnd = nullptr;
        MSG _msg = { 0 };

        float _aspectRatio = 3.0f;

        D3D12_VERTEX_BUFFER_VIEW _stVertexBufferView = {};

        UINT64 _fenceValue = 0ui64;
        HANDLE _fenceEvent = nullptr;

        CD3DX12_VIEWPORT _viewport = {};
        CD3DX12_RECT _scissorRect = {};

        ComPtr<IDXGIFactory6> _dxgiFactory = nullptr;
        ComPtr<IDXGIAdapter4> _dxgiAdapter = nullptr;
        ComPtr<IDXGISwapChain4> _dxgiSwapChain = nullptr;
        ComPtr<ID3D12Device4> _d3dDevice = nullptr;
        ComPtr<ID3D12CommandQueue> _d3dCommandQueue = nullptr;
        ComPtr<ID3D12CommandAllocator> _d3dCommandAllocator = nullptr;
        ComPtr<ID3D12GraphicsCommandList> _d3dCommandList = nullptr;
        ComPtr<ID3D12DescriptorHeap> _d3dRtvHeap = nullptr;
        ComPtr<ID3D12Resource2> _d3dRenderTargets[_frameBackBufCount] = {};
        ComPtr<ID3D12RootSignature> _d3dRootSignature = nullptr;
        ComPtr<ID3D12PipelineState> _d3dPipelineState = nullptr;
        ComPtr<ID3D12Resource> _d3dVertexBuffer = nullptr;
        ComPtr<ID3D12Fence1> _d3dFence = nullptr;

};
#endif
