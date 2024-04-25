#ifdef _WIN32
#include "vdev_win.h"
#include "zlog.h"
#include "ztools.h"

#include <iostream>

using namespace ZPlayer;

LPCWSTR stringToLPCWSTR(std::string orig)
{
	size_t origsize = orig.length() + 1;
	const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t *wcstring = (wchar_t *)malloc(sizeof(wchar_t)*(orig.length() - 1));
	mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);

	return wcstring;
}

void Vdev_win::setSurface(void* surface) {
    if (surface) {
        _hwnd = reinterpret_cast<HWND>(surface);
        RECT rect = {0};
        ::GetWindowRect(_hwnd, &rect);
        _width = rect.right - rect.left;
        _height = rect.bottom - rect.top;

        _viewport.TopLeftX = 0.0f;
        _viewport.TopLeftY = 0.0f;
        _viewport.Width = static_cast<float>(_width);
        _viewport.Height = static_cast<float>(_height);

        _scissorRect.left = 0;
        _scissorRect.top = 0;
        _scissorRect.right = _width;
        _scissorRect.bottom = _height;

		_aspectRatio = static_cast<float>(_width) / static_cast<float>(_height);
    }
    
}
int Vdev_win::init() {
    Vdev::init();
    if (initPipeline() != 0) {
        loge("initPipeline failed\n");
        return -1;
    }

    if (initResource() != 0) {
        loge("initResource failed\n");
        return -1;
    }
    return 0;
}
int Vdev_win::release() {
    waitForPreviousFrame();
    CloseHandle(_fenceEvent);

    Vdev::release();
    return 0;
}

int Vdev_win::render(uint8_t* data, int len) {
    auto hr = populateCommandList(data, len);
    if (hr != S_OK) {
        loge("populateCommandList failed, error: %s", win_error(GetLastError()));
        return -1;
    }

    ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
    _commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    hr = _swapChain->Present(1, 0);
    if (hr != S_OK) {
        loge("present failed, error: %s", win_error(GetLastError()));
        return -1;
    }
    waitForPreviousFrame();
    return 0;
}

int Vdev_win::initPipeline() {
    HRESULT hr = S_OK;
    UINT dxgiFactoryFlags = 0U;
    ComPtr<IDXGIFactory6> factory = nullptr;
    hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
    if (hr != S_OK) {
        loge("createDXGIFactory failed, error: %s", win_error(GetLastError()));
        return -1;
    }

    // enumerate adapters
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &_dxgiAdapter); ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc = {};
        _dxgiAdapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }
        
        hr = D3D12CreateDevice(_dxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr);
        if (hr == S_OK) {
            break;
        }
    }

    // create device
    hr = D3D12CreateDevice(_dxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&_device));
    if (hr != S_OK) {
        loge("D3D12CreateDevice failed, error: %s", win_error(GetLastError()));
        return -1;
    }

    // create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = _device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue));
    if (hr != S_OK) {
        loge("createCommandQueue failed, error: %s", win_error(GetLastError()));
        return -1;
    }

    // create swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = _frameBackBufCount;
    swapChainDesc.Width = _width;
    swapChainDesc.Height = _height;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain = nullptr;
    hr = factory->CreateSwapChainForHwnd(_commandQueue.Get(), _hwnd, &swapChainDesc, nullptr, nullptr, &swapChain);
    if (hr != S_OK) {
        loge("createSwapChainForHwnd failed, error: %s", win_error(GetLastError()));
        return -1;
    }

    hr = factory->MakeWindowAssociation(_hwnd, DXGI_MWA_NO_ALT_ENTER);
    if (hr != S_OK) {
        loge("makeWindowAssociation failed, error: %s", win_error(GetLastError()));
        return -1;
    }

    swapChain.As(&_swapChain);
    _frameIndex = _swapChain->GetCurrentBackBufferIndex();

    // create render target view heap
    {    
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = _frameBackBufCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        hr = _device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvHeap));
        if (hr != S_OK) {
            loge("createDescriptorHeap failed, error: %s", win_error(GetLastError()));
            return -1;
        }
        // create srv heap
        D3D12_DESCRIPTOR_HEAP_DESC stSrvHeapDesc = {};
        stSrvHeapDesc.NumDescriptors = 1;
        stSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        stSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        hr = _device->CreateDescriptorHeap(&stSrvHeapDesc, IID_PPV_ARGS(&_srvHeap));
        if (hr != S_OK) {
            loge("createDescriptorHeap failed, error: %s", win_error(GetLastError()));
            return -1;
        }

        _rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // create frame resources
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < _frameBackBufCount; ++i) {
        hr = _swapChain->GetBuffer(i, IID_PPV_ARGS(&_renderTargets[i]));
        if (hr != S_OK) {
            loge("getBuffer failed, error: %s", win_error(GetLastError()));
            return -1;
        }
        _device->CreateRenderTargetView(_renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, _rtvDescriptorSize);
    }



    // create command allocator
    hr = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAllocator));
    if (hr != S_OK) {
        loge("createCommandAllocator failed, error: %s", win_error(GetLastError()));
        return -1;
    }
    return 0;
}

int Vdev_win::initResource() {
    HRESULT hr = S_OK;
    // create root signature
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        hr = _device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
        if (hr != S_OK) {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 stDSPRanges[1];
        stDSPRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        CD3DX12_ROOT_PARAMETER1 stRootParameters[1];
        stRootParameters[0].InitAsDescriptorTable(1, &stDSPRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_STATIC_SAMPLER_DESC stStaticSampler = {};
        stStaticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;//D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        stStaticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;//D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        stStaticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;//D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        stStaticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;//D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        stStaticSampler.MipLODBias = 0.0f;
        stStaticSampler.MaxAnisotropy = 0;
        stStaticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		stStaticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        stStaticSampler.MinLOD = 0.0f;
        stStaticSampler.MaxLOD = D3D12_FLOAT32_MAX;
        stStaticSampler.ShaderRegister = 0;
        stStaticSampler.RegisterSpace = 0;
        stStaticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(stRootParameters), stRootParameters, 1, &stStaticSampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;

        hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
        if (hr != S_OK) {
            loge("D3D12SerializeVersionedRootSignature failed, error: %s", win_error(GetLastError()));
            return -1;
        }
        hr = _device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
        if (hr != S_OK) {
            loge("createRootSignature failed, error: %s", win_error(GetLastError()));
            return -1;
        }
    }

    // create pipeline state, which includes commpiling and loading shaders
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;
        #ifdef _DEBUG
            UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        #else
            UINT compileFlags = 0;
        #endif
        auto shaderPath = get_current_path() + "\\shader\\texture.hlsl";
        hr = D3DCompileFromFile(stringToLPCWSTR(shaderPath.c_str()), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
        if (hr != S_OK) {
            loge("D3DCompileFromFile failed, error: %s", win_error(GetLastError()));
            return -1;
        }
        hr = D3DCompileFromFile(stringToLPCWSTR(shaderPath.c_str()), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);
        if (hr != S_OK) {
            loge("D3DCompileFromFile failed, error: %s", win_error(GetLastError()));
            return -1;
        }
        
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };

        // create pipeline state
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
        pipelineStateDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        pipelineStateDesc.pRootSignature = _rootSignature.Get();
        pipelineStateDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        pipelineStateDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        pipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        pipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        pipelineStateDesc.DepthStencilState.DepthEnable = FALSE;
        pipelineStateDesc.DepthStencilState.StencilEnable = FALSE;
        pipelineStateDesc.SampleMask = UINT_MAX;
        pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateDesc.NumRenderTargets = 1;
        pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pipelineStateDesc.SampleDesc.Count = 1;
        hr = _device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&_pipelineState));
        if (hr != S_OK) {
            loge("createGraphicsPipelineState failed, error: %s", win_error(GetLastError()));
            return -1;
        }
    }

    // create command list
    hr = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocator.Get(), _pipelineState.Get(), IID_PPV_ARGS(&_commandList));
    if (hr != S_OK) {
        loge("createCommandList failed, error: %s", win_error(GetLastError()));
        return -1;
    }

    // create the vertex buffer
    {
        Vertex stTriangleVertices[] = {
            {{-1.f, -1.f, 0.0f}, {0.0f, 1.0f}},
            {{-1.f, 1.f, 0.0f}, {0.0f, 0.0f}},
            {{1.f,-1.f, 0.0f}, {1.0f, 1.0f}},
            {{1.f, 1.f, 0.0f}, {1.0f, 0.0f}},
        };

        const UINT vertexBufferSize = sizeof(stTriangleVertices);
		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        hr = _device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&_vertexBuffer)
        );
        if (hr != S_OK) {
            loge("createCommittedResource failed, error: %s", win_error(GetLastError()));
            return -1;
        }

        UINT8* vertexDataBegin = nullptr;
        CD3DX12_RANGE readRange(0, 0);
        hr = _vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin));
        if (hr != S_OK) {
            loge("map failed, error: %s", win_error(GetLastError()));
            return -1;
        }
        memcpy(vertexDataBegin, stTriangleVertices, sizeof(stTriangleVertices));
        _vertexBuffer->Unmap(0, nullptr);
        
        _vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
        _vertexBufferView.StrideInBytes = sizeof(Vertex);
        _vertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // create the texture
    {
        D3D12_RESOURCE_DESC stTextureDesc = {};
        stTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        stTextureDesc.Alignment = 0;
        stTextureDesc.Width = _width;
        stTextureDesc.Height = _height;
        stTextureDesc.DepthOrArraySize = 1;
        stTextureDesc.MipLevels = 1;
        stTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        stTextureDesc.SampleDesc.Count = 1;
        stTextureDesc.SampleDesc.Quality = 0;
        // stTextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        stTextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        hr = _device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &stTextureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&_texcute));
        if (hr != S_OK) {
            loge("createCommittedResource failed, error: %s", win_error(GetLastError()));
            return -1;
        }

        UINT64 uploadBufferSize = GetRequiredIntermediateSize(_texcute.Get(), 0, 1);

        heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
        hr = _device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&_textureUpload));
        if (hr != S_OK) {
            loge("createCommittedResource failed, error: %s", win_error(GetLastError()));
            return -1;
        }

        std::vector<UINT8> texture = generateTextureData();

        D3D12_SUBRESOURCE_DATA textureData = {};
        textureData.pData = &texture[0];
        textureData.RowPitch = _width * 4;
        textureData.SlicePitch = textureData.RowPitch * _height;
        //         FILE* fp = nullptr;
        // fopen_s(&fp, R"(C:\Users\51917\Desktop\test\texture1.rgba)", "wb");
        // fwrite(textureData.pData, 1, textureData.SlicePitch, fp);
        // fclose(fp);

        int size = UpdateSubresources(_commandList.Get(), _texcute.Get(), _textureUpload.Get(), 0, 0, 1, &textureData);
        if (size == 0) {
            loge("UpdateSubresources failed, error: %s", win_error(GetLastError()));
            return -1;
        }

		auto urceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_texcute.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        _commandList->ResourceBarrier(1, &urceBarrier);
    
        D3D12_SHADER_RESOURCE_VIEW_DESC stSrvDesc = {};
        stSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        stSrvDesc.Format = stTextureDesc.Format;
        stSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        stSrvDesc.Texture2D.MipLevels = 1;
        stSrvDesc.Texture2D.MostDetailedMip = 0;
        _device->CreateShaderResourceView(_texcute.Get(), &stSrvDesc, _srvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    // close the command list and execute it to begin the initial GPU setup
    hr = _commandList->Close();
    if (hr != S_OK) {
        loge("close command list failed, error: %s", win_error(GetLastError()));
        return -1;
    }

    ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
    _commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    {
        // create fence
        hr = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
        if (hr != S_OK) {
            loge("createFence failed, error: %s", win_error(GetLastError()));
            return -1;
        }
        _fenceValue = 1;

        // create fence event
        _fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (_fenceEvent == nullptr) {
            loge("createEvent failed");
            return -1;
        }

        waitForPreviousFrame();
    }

}

std::vector<UINT8> Vdev_win::generateTextureData()
{
    const UINT rowPitch = _width * 4;
    const UINT cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
    const UINT cellHeight = _height >> 3;    // The height of a cell in the checkerboard texture.
    const UINT textureSize = rowPitch * _height;

    std::vector<UINT8> data(textureSize);
    UINT8* pData = &data[0];

    for (UINT n = 0; n < textureSize; n += 4)
    {
        UINT x = n % rowPitch;
        UINT y = n / rowPitch;
        UINT i = x / cellPitch;
        UINT j = y / cellHeight;

        if (i % 2 != j % 2)
        {
            pData[n] = 0x00;        // R
            pData[n + 1] = 0x00;    // G
            pData[n + 2] = 0x00;    // B
            pData[n + 3] = 0xff;    // A
        }
        else
        {
            pData[n] = 0xff;        // R
            pData[n + 1] = 0xff;    // G
            pData[n + 2] = 0xff;    // B
            pData[n + 3] = 0xff;    // A
        }
    }

    return data;
}

int Vdev_win::waitForPreviousFrame() {
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = _fenceValue;
    auto hr = _commandQueue->Signal(_fence.Get(), fence);
    if (FAILED(hr)) {
        loge("signal failed, error: %s", win_error(GetLastError()));
        return -1;
    }
    _fenceValue++;

    // Wait until the previous frame is finished.
    if (_fence->GetCompletedValue() < fence) {
        hr = _fence->SetEventOnCompletion(fence, _fenceEvent);
        if (FAILED(hr)) {
            loge("setEventOnCompletion failed, error: %s", win_error(GetLastError()));
            return -1;
        }

        WaitForSingleObject(_fenceEvent, INFINITE);
    }

    _frameIndex = _swapChain->GetCurrentBackBufferIndex();
    return 0;
}

int Vdev_win::populateCommandList(uint8_t* frame, int size)
{
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    auto hr = _commandAllocator->Reset();
    if (FAILED(hr)) {
        loge("reset command allocator failed, error: %s", win_error(GetLastError()));
        return -1;
    }

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    hr = _commandList->Reset(_commandAllocator.Get(), _pipelineState.Get());
    if (FAILED(hr)) {
        loge("reset command list failed, error: %s", win_error(GetLastError()));
        return -1;
    }

    // Set necessary state.
    _commandList->SetGraphicsRootSignature(_rootSignature.Get());
    
    ID3D12DescriptorHeap* ppHeaps[] = { _srvHeap.Get() };
    _commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    
    _commandList->SetGraphicsRootDescriptorTable(0, _srvHeap->GetGPUDescriptorHandleForHeapStart());
    
    _commandList->RSSetViewports(1, &_viewport);
    
    _commandList->RSSetScissorRects(1, &_scissorRect);
    // copy data to texture
    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = frame;
    textureData.RowPitch = _width * 4;
    textureData.SlicePitch = size;
    int len = UpdateSubresources(_commandList.Get(), _texcute.Get(), _textureUpload.Get(), 0, 0, 1, &textureData);
    if (len == 0) {
        loge("UpdateSubresources failed, error: %s", win_error(GetLastError()));
        return -1;
    }

    // Indicate that the back buffer will be used as a render target.
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
    _commandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart(), _frameIndex, _rtvDescriptorSize);
    _commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    _commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    _commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    _commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
    _commandList->DrawInstanced(4, 1, 0, 0);

    // Indicate that the back buffer will now be used to present.
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    _commandList->ResourceBarrier(1, &barrier);

    return _commandList->Close();
}
#endif