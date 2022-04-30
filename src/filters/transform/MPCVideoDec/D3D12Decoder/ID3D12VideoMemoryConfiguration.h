// -----------------------------------------------------------------
// ID3D12VideoMemoryConfiguration interface and data structure definitions
// -----------------------------------------------------------------
#pragma once

interface __declspec(uuid("8495AA1F-E090-4C85-9C34-3B082215DFA2")) ID3D12DecoderConfiguration : public IUnknown
{
    // Set the surface format the decoder is going to send.
    // If the renderer is not ready to accept this format, an error will be returned.
    virtual HRESULT STDMETHODCALLTYPE ActivateD3D12Decoding(ID3D12Device* pDevice,
        HANDLE hMutex, UINT nFlags) = 0;

    // Get the currently preferred D3D11 adapter index (to be used with IDXGIFactory1::EnumAdapters1)
    virtual UINT STDMETHODCALLTYPE GetD3D12AdapterIndex() = 0;
};


interface __declspec(uuid("48334841-69F8-43CD-9EDC-75AFE51FF96D")) IMediaSampleD3D12 : public IUnknown
{
    // Get the D3D11 texture for the specified view.
    // 2D images with only one view always use view 0. For 3D, view 0 specifies the base view, view 1 the extension
    // view.
    virtual HRESULT STDMETHODCALLTYPE GetD3D12Texture(ID3D12Resource** ppTexture, int* index) = 0;
};
