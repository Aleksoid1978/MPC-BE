/*
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#pragma once

#define DXGI_SWAP_FRAME_COUNT   3

#include "d3d12.h"
#include "d3d12video.h"
#include <vector>

class CD3D12Commands
{
public:
    CD3D12Commands(ID3D12Device* d3d_dev);
    ~CD3D12Commands();
    

    ID3D12GraphicsCommandList* GetCommandBuffer();
    void Reset(ID3D12GraphicsCommandList* cmd);
    
    

    void ResourceBarrier(ID3D12GraphicsCommandList *cmd, ID3D12Resource *res,
        D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
    void ResourceBarrier(ID3D12Resource *res, D3D12_RESOURCE_STATES before,
        D3D12_RESOURCE_STATES after);
    void Submit(const std::vector<ID3D12GraphicsCommandList*>& cmds);
    void Init(ID3D12Device* d3d_dev);
    void GPUSync();
private:
    ID3D12Fence *m_GPUSyncFence;
    HANDLE m_GPUSyncHandle;
    UINT64 m_GPUSyncCounter;
    ID3D12Device* m_pD3DDevice;
    ID3D12CommandAllocator* m_pCommandAllocator;
    ID3D12CommandQueue *m_pCommandQueue;
    ID3D12GraphicsCommandList *m_pDebugList;
    std::vector<ID3D12GraphicsCommandList*> m_pFreeGraphicsCommand;
    std::vector<ID3D12VideoDecodeCommandList*> m_pFreeVideoDecodeCommand;
    std::vector<std::pair<ID3D12GraphicsCommandList*, UINT64>> m_pPendingCommandBuffers;

    ID3D12DescriptorHeap *m_pRTV, *m_pDSV, *m_pCBVUAVSRV, *m_pClear, *m_pSampler;
protected:

};

