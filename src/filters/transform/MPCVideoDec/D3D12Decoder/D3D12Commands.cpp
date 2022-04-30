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


#include "stdafx.h"
#include "D3D12Commands.h"


CD3D12Commands::CD3D12Commands(ID3D12Device* d3d_dev)
{
    m_GPUSyncHandle = NULL;
    m_GPUSyncCounter = 1;
    Init(d3d_dev);
}
CD3D12Commands::~CD3D12Commands()
{
    SafeRelease(&m_pD3DDevice);
    SafeRelease(&m_pCommandAllocator);
    SafeRelease(&m_pCommandQueue);

}

void CD3D12Commands::Init(ID3D12Device* d3d_dev)
{
    m_pD3DDevice = d3d_dev;
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    m_pD3DDevice->CreateCommandQueue(&desc, __uuidof(ID3D12CommandQueue), (void**)&m_pCommandQueue);
    //was using the D3D12_FENCE_FLAG_NONE
    m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_SHARED, __uuidof(ID3D12Fence), (void**)&m_GPUSyncFence);
    //we were using CreateEvent(NULL, TRUE, FALSE, NULL) its manual reset which was true
    m_GPUSyncHandle = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    m_GPUSyncFence->SetName(L"GPUSync fence");

    
    CHECK_HR(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
        __uuidof(ID3D12CommandAllocator), (void**)&m_pCommandAllocator));

    m_pCommandAllocator->SetName(L"Command allocator");

    CHECK_HR(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator, NULL,
        __uuidof(ID3D12GraphicsCommandList), (void**)&m_pDebugList));

    // command buffers are allocated opened, close it immediately.
    m_pDebugList->Close();

    m_pDebugList->SetName(L"Debug command list");


    

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heap_desc.NodeMask = 1;
    heap_desc.NumDescriptors = 32;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    CHECK_HR(m_pD3DDevice->CreateDescriptorHeap(&heap_desc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pRTV));

    m_pRTV->SetName(L"RTV heap");

    heap_desc.NumDescriptors = 1;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    CHECK_HR(m_pD3DDevice->CreateDescriptorHeap(&heap_desc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pDSV));

    m_pDSV->SetName(L"DSV heap");

    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    heap_desc.NumDescriptors = 8;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

    CHECK_HR(m_pD3DDevice->CreateDescriptorHeap(&heap_desc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pSampler));

    m_pSampler->SetName(L"Sampler heap");

    heap_desc.NumDescriptors = 1030;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    
    CHECK_HR(m_pD3DDevice->CreateDescriptorHeap(&heap_desc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pCBVUAVSRV));

    m_pCBVUAVSRV->SetName(L"CBV/UAV/SRV heap");

    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    CHECK_HR(m_pD3DDevice->CreateDescriptorHeap(&heap_desc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pClear));
    m_pClear->SetName(L"UAV clear heap");
    return;
done:
    assert(0);
    
}

ID3D12GraphicsCommandList* CD3D12Commands::GetCommandBuffer()
{
    if (m_pFreeGraphicsCommand.empty())
    {
        ID3D12GraphicsCommandList* list = NULL;
        CHECK_HR(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator, NULL,
            __uuidof(ID3D12GraphicsCommandList), (void**)&list));
        // list starts opened, close it
        list->Close();
        m_pFreeGraphicsCommand.push_back(list);
    }

    ID3D12GraphicsCommandList* ret = m_pFreeGraphicsCommand.back();
    m_pFreeGraphicsCommand.pop_back();

    return ret;
done:
    return nullptr;
}

void CD3D12Commands::Reset(ID3D12GraphicsCommandList* cmd)
{
    m_pCommandAllocator->Reset();
    cmd->Reset(m_pCommandAllocator, nullptr);
    
}

void CD3D12Commands::ResourceBarrier(ID3D12GraphicsCommandList *cmd, ID3D12Resource *res,
    D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = res;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    cmd->ResourceBarrier(1, &barrier);
}

void CD3D12Commands::ResourceBarrier(ID3D12Resource *res, D3D12_RESOURCE_STATES before,
    D3D12_RESOURCE_STATES after)

{
    ID3D12GraphicsCommandList *cmd = GetCommandBuffer();

    Reset(cmd);
    ResourceBarrier(cmd, res, before, after);
    cmd->Close();

    Submit({ cmd });
}

void CD3D12Commands::Submit(const std::vector<ID3D12GraphicsCommandList*>& cmds)
{
    std::vector<ID3D12CommandList*> submits;

    m_GPUSyncCounter++;

    for (int i = 0 ; i<cmds.size();i++)//std::vector< ID3D12GraphicsCommandList*>::iterator it; it != cmds.end(); it++)
    {
        
        m_pPendingCommandBuffers.push_back(std::make_pair(cmds.at(i), m_GPUSyncCounter));
        submits.push_back(cmds.at(i));
    }

    m_pCommandQueue->ExecuteCommandLists((UINT)submits.size(), submits.data());
    m_pCommandQueue->Signal(m_GPUSyncFence, m_GPUSyncCounter);
}

void CD3D12Commands::GPUSync()
{
    m_GPUSyncCounter++;

    m_pCommandQueue->Signal(m_GPUSyncFence, m_GPUSyncCounter);
    m_GPUSyncFence->SetEventOnCompletion(m_GPUSyncCounter, m_GPUSyncHandle);
    WaitForSingleObject(m_GPUSyncHandle, 10000);

}
