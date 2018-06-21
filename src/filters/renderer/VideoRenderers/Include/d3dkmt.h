#ifndef _D3DKMT_H
#define _D3DKMT_H

#include <winternl.h>
#include "d3dumddi.h"

// D3D definitions

typedef struct _D3DKMT_OPENADAPTERFROMDEVICENAME
{
    _In_ PCWSTR pDeviceName;
    _Out_ D3DKMT_HANDLE hAdapter;
    _Out_ LUID AdapterLuid;
} D3DKMT_OPENADAPTERFROMDEVICENAME;

typedef struct _D3DKMT_OPENADAPTERFROMHDC {
    _In_ HDC hDc;
    _Out_ D3DKMT_HANDLE hAdapter;
    _Out_ LUID AdapterLuid;
    _Out_ D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_OPENADAPTERFROMHDC;

typedef struct _D3DKMT_CLOSEADAPTER
{
    _In_ D3DKMT_HANDLE hAdapter;
} D3DKMT_CLOSEADAPTER;

typedef enum _D3DKMT_QUERYRESULT_PREEMPTION_ATTEMPT_RESULT
{
    D3DKMT_PreemptionAttempt = 0,
    D3DKMT_PreemptionAttemptSuccess = 1,
    D3DKMT_PreemptionAttemptMissNoCommand = 2,
    D3DKMT_PreemptionAttemptMissNotEnabled = 3,
    D3DKMT_PreemptionAttemptMissNextFence = 4,
    D3DKMT_PreemptionAttemptMissPagingCommand = 5,
    D3DKMT_PreemptionAttemptMissSplittedCommand = 6,
    D3DKMT_PreemptionAttemptMissFenceCommand= 7,
    D3DKMT_PreemptionAttemptMissRenderPendingFlip = 8,
    D3DKMT_PreemptionAttemptMissNotMakingProgress = 9,
    D3DKMT_PreemptionAttemptMissLessPriority = 10,
    D3DKMT_PreemptionAttemptMissRemainingQuantum = 11,
    D3DKMT_PreemptionAttemptMissRemainingPreemptionQuantum = 12,
    D3DKMT_PreemptionAttemptMissAlreadyPreempting = 13,
    D3DKMT_PreemptionAttemptMissGlobalBlock = 14,
    D3DKMT_PreemptionAttemptMissAlreadyRunning = 15,
    D3DKMT_PreemptionAttemptStatisticsMax
} D3DKMT_QUERYRESULT_PREEMPTION_ATTEMPT_RESULT;

typedef enum _D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE
{
    D3DKMT_ClientRenderBuffer = 0,
    D3DKMT_ClientPagingBuffer = 1,
    D3DKMT_SystemPagingBuffer = 2,
    D3DKMT_SystemPreemptionBuffer = 3,
    D3DKMT_DmaPacketTypeMax
} D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE;

typedef enum _D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE
{
    D3DKMT_RenderCommandBuffer = 0,
    D3DKMT_DeferredCommandBuffer = 1,
    D3DKMT_SystemCommandBuffer = 2,
    D3DKMT_MmIoFlipCommandBuffer = 3,
    D3DKMT_WaitCommandBuffer = 4,
    D3DKMT_SignalCommandBuffer = 5,
    D3DKMT_DeviceCommandBuffer = 6,
    D3DKMT_SoftwareCommandBuffer = 7,
    D3DKMT_QueuePacketTypeMax
} D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE;

typedef enum _D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS
{
    D3DKMT_AllocationPriorityClassMinimum = 0,
    D3DKMT_AllocationPriorityClassLow = 1,
    D3DKMT_AllocationPriorityClassNormal = 2,
    D3DKMT_AllocationPriorityClassHigh = 3,
    D3DKMT_AllocationPriorityClassMaximum = 4,
    D3DKMT_MaxAllocationPriorityClass
} D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS;

#define D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX 5

typedef struct _D3DKMT_QUERYSTATISTICS_COUNTER
{
    ULONG Count;
    ULONGLONG Bytes;
} D3DKMT_QUERYSTATISTICS_COUNTER;

typedef struct _D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION
{
    ULONG PacketSubmited;
    ULONG PacketCompleted;
    ULONG PacketPreempted;
    ULONG PacketFaulted;
} D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION
{
    ULONG PacketSubmited;
    ULONG PacketCompleted;
} D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION
{
    D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION QueuePacket[D3DKMT_QueuePacketTypeMax];
    D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION DmaPacket[D3DKMT_DmaPacketTypeMax];
} D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION
{
    ULONG PreemptionCounter[D3DKMT_PreemptionAttemptStatisticsMax];
} D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION
{
    LARGE_INTEGER RunningTime; // 100ns
    ULONG ContextSwitch;
    D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION PreemptionStatistics;
    D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION PacketStatistics;
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_NODE_INFORMATION
{
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION GlobalInformation; // global
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION SystemInformation; // system thread
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_NODE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION
{
    ULONG Frame;
    ULONG CancelledFrame;
    ULONG QueuedPresent;
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION
{
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION GlobalInformation; // global
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION SystemInformation; // system thread
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATSTICS_REFERENCE_DMA_BUFFER
{
    ULONG NbCall;
    ULONG NbAllocationsReferenced;
    ULONG MaxNbAllocationsReferenced;
    ULONG NbNULLReference;
    ULONG NbWriteReference;
    ULONG NbRenamedAllocationsReferenced;
    ULONG NbIterationSearchingRenamedAllocation;
    ULONG NbLockedAllocationReferenced;
    ULONG NbAllocationWithValidPrepatchingInfoReferenced;
    ULONG NbAllocationWithInvalidPrepatchingInfoReferenced;
    ULONG NbDMABufferSuccessfullyPrePatched;
    ULONG NbPrimariesReferencesOverflow;
    ULONG NbAllocationWithNonPreferredResources;
    ULONG NbAllocationInsertedInMigrationTable;
} D3DKMT_QUERYSTATSTICS_REFERENCE_DMA_BUFFER;

typedef struct _D3DKMT_QUERYSTATSTICS_RENAMING
{
    ULONG NbAllocationsRenamed;
    ULONG NbAllocationsShrinked;
    ULONG NbRenamedBuffer;
    ULONG MaxRenamingListLength;
    ULONG NbFailuresDueToRenamingLimit;
    ULONG NbFailuresDueToCreateAllocation;
    ULONG NbFailuresDueToOpenAllocation;
    ULONG NbFailuresDueToLowResource;
    ULONG NbFailuresDueToNonRetiredLimit;
} D3DKMT_QUERYSTATSTICS_RENAMING;

typedef struct _D3DKMT_QUERYSTATSTICS_PREPRATION
{
    ULONG BroadcastStall;
    ULONG NbDMAPrepared;
    ULONG NbDMAPreparedLongPath;
    ULONG ImmediateHighestPreparationPass;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsTrimmed;
} D3DKMT_QUERYSTATSTICS_PREPRATION;

typedef struct _D3DKMT_QUERYSTATSTICS_PAGING_FAULT
{
    D3DKMT_QUERYSTATISTICS_COUNTER Faults;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsFirstTimeAccess;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsReclaimed;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsMigration;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsIncorrectResource;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsLostContent;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsEvicted;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsMEM_RESET;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsUnresetSuccess;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsUnresetFail;
    ULONG AllocationsUnresetSuccessRead;
    ULONG AllocationsUnresetFailRead;

    D3DKMT_QUERYSTATISTICS_COUNTER Evictions;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToPreparation;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToLock;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToClose;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToPurge;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToSuspendCPUAccess;
} D3DKMT_QUERYSTATSTICS_PAGING_FAULT;

typedef struct _D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER
{
    ULONGLONG BytesFilled;
    ULONGLONG BytesDiscarded;
    ULONGLONG BytesMappedIntoAperture;
    ULONGLONG BytesUnmappedFromAperture;
    ULONGLONG BytesTransferredFromMdlToMemory;
    ULONGLONG BytesTransferredFromMemoryToMdl;
    ULONGLONG BytesTransferredFromApertureToMemory;
    ULONGLONG BytesTransferredFromMemoryToAperture;
} D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER;

typedef struct _D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE
{
    ULONG NbRangesAcquired;
    ULONG NbRangesReleased;
} D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE;

typedef struct _D3DKMT_QUERYSTATSTICS_LOCKS
{
    ULONG NbLocks;
    ULONG NbLocksWaitFlag;
    ULONG NbLocksDiscardFlag;
    ULONG NbLocksNoOverwrite;
    ULONG NbLocksNoReadSync;
    ULONG NbLocksLinearization;
    ULONG NbComplexLocks;
} D3DKMT_QUERYSTATSTICS_LOCKS;

typedef struct _D3DKMT_QUERYSTATSTICS_ALLOCATIONS
{
    D3DKMT_QUERYSTATISTICS_COUNTER Created;
    D3DKMT_QUERYSTATISTICS_COUNTER Destroyed;
    D3DKMT_QUERYSTATISTICS_COUNTER Opened;
    D3DKMT_QUERYSTATISTICS_COUNTER Closed;
    D3DKMT_QUERYSTATISTICS_COUNTER MigratedSuccess;
    D3DKMT_QUERYSTATISTICS_COUNTER MigratedFail;
    D3DKMT_QUERYSTATISTICS_COUNTER MigratedAbandoned;
} D3DKMT_QUERYSTATSTICS_ALLOCATIONS;

typedef struct _D3DKMT_QUERYSTATSTICS_TERMINATIONS
{
    D3DKMT_QUERYSTATISTICS_COUNTER TerminatedShared;
    D3DKMT_QUERYSTATISTICS_COUNTER TerminatedNonShared;
    D3DKMT_QUERYSTATISTICS_COUNTER DestroyedShared;
    D3DKMT_QUERYSTATISTICS_COUNTER DestroyedNonShared;
} D3DKMT_QUERYSTATSTICS_TERMINATIONS;

typedef struct _D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION
{
    ULONG NbSegments;
    ULONG NodeCount;
    ULONG VidPnSourceCount;

    ULONG VSyncEnabled;
    ULONG TdrDetectedCount;

    LONGLONG ZeroLengthDmaBuffers;
    ULONGLONG RestartedPeriod;

    D3DKMT_QUERYSTATSTICS_REFERENCE_DMA_BUFFER ReferenceDmaBuffer;
    D3DKMT_QUERYSTATSTICS_RENAMING Renaming;
    D3DKMT_QUERYSTATSTICS_PREPRATION Preparation;
    D3DKMT_QUERYSTATSTICS_PAGING_FAULT PagingFault;
    D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER PagingTransfer;
    D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE SwizzlingRange;
    D3DKMT_QUERYSTATSTICS_LOCKS Locks;
    D3DKMT_QUERYSTATSTICS_ALLOCATIONS Allocations;
    D3DKMT_QUERYSTATSTICS_TERMINATIONS Terminations;

    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY
{
    ULONGLONG BytesAllocated;
    ULONGLONG BytesReserved;
    ULONG SmallAllocationBlocks;
    ULONG LargeAllocationBlocks;
    ULONGLONG WriteCombinedBytesAllocated;
    ULONGLONG WriteCombinedBytesReserved;
    ULONGLONG CachedBytesAllocated;
    ULONGLONG CachedBytesReserved;
    ULONGLONG SectionBytesAllocated;
    ULONGLONG SectionBytesReserved;
} D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION
{
    ULONG NodeCount;
    ULONG VidPnSourceCount;

    D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY SystemMemory;

    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_DMA_BUFFER
{
    D3DKMT_QUERYSTATISTICS_COUNTER Size;
    ULONG AllocationListBytes;
    ULONG PatchLocationListBytes;
} D3DKMT_QUERYSTATISTICS_DMA_BUFFER;

typedef struct _D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA
{
    ULONG64 TotalBytesEvictedFromProcess;
    ULONG64 BytesBySegmentPreference[D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX];
} D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA;

typedef struct _D3DKMT_QUERYSTATISTICS_POLICY
{
    ULONGLONG PreferApertureForRead[D3DKMT_MaxAllocationPriorityClass];
    ULONGLONG PreferAperture[D3DKMT_MaxAllocationPriorityClass];
    ULONGLONG MemResetOnPaging;
    ULONGLONG RemovePagesFromWorkingSetOnPaging;
    ULONGLONG MigrationEnabled;
} D3DKMT_QUERYSTATISTICS_POLICY;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION
{
    ULONG NbSegments;
    ULONG NodeCount;
    ULONG VidPnSourceCount;

    ULONG VirtualMemoryUsage;

    D3DKMT_QUERYSTATISTICS_DMA_BUFFER DmaBuffer;
    D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA CommitmentData;
    D3DKMT_QUERYSTATISTICS_POLICY _Policy;

    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_MEMORY
{
    ULONGLONG TotalBytesEvicted;
    ULONG AllocsCommitted;
    ULONG AllocsResident;
} D3DKMT_QUERYSTATISTICS_MEMORY;

typedef struct _D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1
{
    ULONG CommitLimit;
    ULONG BytesCommitted;
    ULONG BytesResident;

    D3DKMT_QUERYSTATISTICS_MEMORY Memory;

    ULONG Aperture; // boolean

    ULONGLONG TotalBytesEvictedByPriority[D3DKMT_MaxAllocationPriorityClass];

    ULONG64 SystemMemoryEndAddress;
    struct
    {
        ULONG64 PreservedDuringStandby : 1;
        ULONG64 PreservedDuringHibernate : 1;
        ULONG64 PartiallyPreservedDuringHibernate : 1;
        ULONG64 Reserved : 61;
    } PowerFlags;

    ULONG64 Reserved[7];
} D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1;

typedef struct _D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION
{
    ULONGLONG CommitLimit;
    ULONGLONG BytesCommitted;
    ULONGLONG BytesResident;

    D3DKMT_QUERYSTATISTICS_MEMORY Memory;

    ULONG Aperture; // boolean

    ULONGLONG TotalBytesEvictedByPriority[D3DKMT_MaxAllocationPriorityClass];

    ULONG64 SystemMemoryEndAddress;
    struct
    {
        ULONG64 PreservedDuringStandby : 1;
        ULONG64 PreservedDuringHibernate : 1;
        ULONG64 PartiallyPreservedDuringHibernate : 1;
        ULONG64 Reserved : 61;
    } PowerFlags;

    ULONG64 Reserved[6];
} D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY
{
    ULONG AllocsCommitted;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocsResidentInP[D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX];
    D3DKMT_QUERYSTATISTICS_COUNTER AllocsResidentInNonPreferred;
    ULONGLONG TotalBytesEvictedDueToPreparation;
} D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY
{
    ULONGLONG UseMRU;
} D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION
{
    ULONGLONG BytesCommitted;
    ULONGLONG MaximumWorkingSet;
    ULONGLONG MinimumWorkingSet;

    ULONG NbReferencedAllocationEvictedInPeriod;

    D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY VideoMemory;
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY _Policy;

    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION;

typedef enum _D3DKMT_QUERYSTATISTICS_TYPE
{
    D3DKMT_QUERYSTATISTICS_ADAPTER = 0,
    D3DKMT_QUERYSTATISTICS_PROCESS = 1,
    D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER = 2,
    D3DKMT_QUERYSTATISTICS_SEGMENT = 3,
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT = 4,
    D3DKMT_QUERYSTATISTICS_NODE = 5,
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE = 6,
    D3DKMT_QUERYSTATISTICS_VIDPNSOURCE = 7,
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE = 8
} D3DKMT_QUERYSTATISTICS_TYPE;

typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT
{
    ULONG SegmentId;
} D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT;

typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_NODE
{
    ULONG NodeId;
} D3DKMT_QUERYSTATISTICS_QUERY_NODE;

typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE
{
    ULONG VidPnSourceId;
} D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE;

typedef union _D3DKMT_QUERYSTATISTICS_RESULT
{
    D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION AdapterInformation;
    D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1 SegmentInformationV1; // WIN7
    D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION SegmentInformation; // WIN8
    D3DKMT_QUERYSTATISTICS_NODE_INFORMATION NodeInformation;
    D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION VidPnSourceInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION ProcessInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION ProcessAdapterInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION ProcessSegmentInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION ProcessNodeInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION ProcessVidPnSourceInformation;
} D3DKMT_QUERYSTATISTICS_RESULT;

typedef struct _D3DKMT_QUERYSTATISTICS
{
    _In_ D3DKMT_QUERYSTATISTICS_TYPE Type;
    _In_ LUID AdapterLuid;
    _In_opt_ HANDLE hProcess;
    _Out_ D3DKMT_QUERYSTATISTICS_RESULT QueryResult;

    union
    {
        _In_ D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT QuerySegment;
        _In_ D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT QueryProcessSegment;
        _In_ D3DKMT_QUERYSTATISTICS_QUERY_NODE QueryNode;
        _In_ D3DKMT_QUERYSTATISTICS_QUERY_NODE QueryProcessNode;
        _In_ D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE QueryVidPnSource;
        _In_ D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE QueryProcessVidPnSource;
    };
} D3DKMT_QUERYSTATISTICS;

typedef enum _KMTQUERYADAPTERINFOTYPE
{
    KMTQAITYPE_UMDRIVERPRIVATE = 0,
    KMTQAITYPE_UMDRIVERNAME = 1, // D3DKMT_UMDFILENAMEINFO
    KMTQAITYPE_UMOPENGLINFO = 2, // D3DKMT_OPENGLINFO
    KMTQAITYPE_GETSEGMENTSIZE = 3, // D3DKMT_SEGMENTSIZEINFO
    KMTQAITYPE_ADAPTERGUID = 4, // GUID
    KMTQAITYPE_FLIPQUEUEINFO = 5, // D3DKMT_FLIPQUEUEINFO
    KMTQAITYPE_ADAPTERADDRESS = 6, // D3DKMT_ADAPTERADDRESS
    KMTQAITYPE_SETWORKINGSETINFO = 7, // D3DKMT_WORKINGSETINFO
    KMTQAITYPE_ADAPTERREGISTRYINFO = 8, // D3DKMT_ADAPTERREGISTRYINFO
    KMTQAITYPE_CURRENTDISPLAYMODE = 9, // D3DKMT_CURRENTDISPLAYMODE
    KMTQAITYPE_MODELIST = 10, // D3DKMT_DISPLAYMODE (array)
    KMTQAITYPE_CHECKDRIVERUPDATESTATUS = 11,
    KMTQAITYPE_VIRTUALADDRESSINFO = 12, // D3DKMT_VIRTUALADDRESSINFO
    KMTQAITYPE_DRIVERVERSION = 13, // D3DKMT_DRIVERVERSION
    KMTQAITYPE_ADAPTERTYPE = 15, // D3DKMT_ADAPTERTYPE // since WIN8
    KMTQAITYPE_OUTPUTDUPLCONTEXTSCOUNT = 16, // D3DKMT_OUTPUTDUPLCONTEXTSCOUNT
    KMTQAITYPE_WDDM_1_2_CAPS = 17, // D3DKMT_WDDM_1_2_CAPS
    KMTQAITYPE_UMD_DRIVER_VERSION = 18, // D3DKMT_UMD_DRIVER_VERSION
    KMTQAITYPE_DIRECTFLIP_SUPPORT = 19, // D3DKMT_DIRECTFLIP_SUPPORT
    KMTQAITYPE_MULTIPLANEOVERLAY_SUPPORT = 20, // D3DKMT_MULTIPLANEOVERLAY_SUPPORT // since WDDM1_3
    KMTQAITYPE_DLIST_DRIVER_NAME = 21, // D3DKMT_DLIST_DRIVER_NAME
    KMTQAITYPE_WDDM_1_3_CAPS = 22, // D3DKMT_WDDM_1_3_CAPS
    KMTQAITYPE_MULTIPLANEOVERLAY_HUD_SUPPORT = 23, // D3DKMT_MULTIPLANEOVERLAY_HUD_SUPPORT
    KMTQAITYPE_WDDM_2_0_CAPS = 24, // D3DKMT_WDDM_2_0_CAPS // since WDDM2_0
    KMTQAITYPE_NODEMETADATA = 25, // D3DKMT_NODEMETADATA
    KMTQAITYPE_CPDRIVERNAME = 26, // D3DKMT_CPDRIVERNAME
    KMTQAITYPE_XBOX = 27, // D3DKMT_XBOX
    KMTQAITYPE_INDEPENDENTFLIP_SUPPORT = 28, // D3DKMT_INDEPENDENTFLIP_SUPPORT
    KMTQAITYPE_MIRACASTCOMPANIONDRIVERNAME = 29, // D3DKMT_MIRACASTCOMPANIONDRIVERNAME
    KMTQAITYPE_PHYSICALADAPTERCOUNT = 30, // D3DKMT_PHYSICAL_ADAPTER_COUNT
    KMTQAITYPE_PHYSICALADAPTERDEVICEIDS = 31, // D3DKMT_QUERY_DEVICE_IDS
    KMTQAITYPE_DRIVERCAPS_EXT = 32, // D3DKMT_DRIVERCAPS_EXT
    KMTQAITYPE_QUERY_MIRACAST_DRIVER_TYPE = 33, // D3DKMT_QUERY_MIRACAST_DRIVER_TYPE
    KMTQAITYPE_QUERY_GPUMMU_CAPS = 34, // D3DKMT_QUERY_GPUMMU_CAPS
    KMTQAITYPE_QUERY_MULTIPLANEOVERLAY_DECODE_SUPPORT = 35, // D3DKMT_MULTIPLANEOVERLAY_DECODE_SUPPORT
    KMTQAITYPE_QUERY_HW_PROTECTION_TEARDOWN_COUNT = 36, // UINT32
    KMTQAITYPE_QUERY_ISBADDRIVERFORHWPROTECTIONDISABLED = 37, // D3DKMT_ISBADDRIVERFORHWPROTECTIONDISABLED
    KMTQAITYPE_MULTIPLANEOVERLAY_SECONDARY_SUPPORT = 38, // D3DKMT_MULTIPLANEOVERLAY_SECONDARY_SUPPORT
    KMTQAITYPE_INDEPENDENTFLIP_SECONDARY_SUPPORT = 39, // D3DKMT_INDEPENDENTFLIP_SECONDARY_SUPPORT
    KMTQAITYPE_PANELFITTER_SUPPORT = 40, // D3DKMT_PANELFITTER_SUPPORT // since WDDM2_1
    KMTQAITYPE_PHYSICALADAPTERPNPKEY = 41, // D3DKMT_QUERY_PHYSICAL_ADAPTER_PNP_KEY // since WDDM2_2
    KMTQAITYPE_GETSEGMENTGROUPSIZE = 42, // D3DKMT_SEGMENTGROUPSIZEINFO
    KMTQAITYPE_MPO3DDI_SUPPORT = 43, // D3DKMT_MPO3DDI_SUPPORT
    KMTQAITYPE_HWDRM_SUPPORT = 44, // D3DKMT_HWDRM_SUPPORT
    KMTQAITYPE_MPOKERNELCAPS_SUPPORT = 45, // D3DKMT_MPOKERNELCAPS_SUPPORT
    KMTQAITYPE_MULTIPLANEOVERLAY_STRETCH_SUPPORT = 46, // D3DKMT_MULTIPLANEOVERLAY_STRETCH_SUPPORT
    KMTQAITYPE_GET_DEVICE_VIDPN_OWNERSHIP_INFO = 47, // D3DKMT_GET_DEVICE_VIDPN_OWNERSHIP_INFO
    KMTQAITYPE_QUERYREGISTRY = 48, // D3DDDI_QUERYREGISTRY_INFO // since WDDM2_4
    KMTQAITYPE_KMD_DRIVER_VERSION = 49, // D3DKMT_KMD_DRIVER_VERSION
    KMTQAITYPE_BLOCKLIST_KERNEL = 50, // D3DKMT_BLOCKLIST_INFO ??
    KMTQAITYPE_BLOCKLIST_RUNTIME = 51, // D3DKMT_BLOCKLIST_INFO ??
    KMTQAITYPE_ADAPTERGUID_RENDER = 52, // GUID
    KMTQAITYPE_ADAPTERADDRESS_RENDER = 53, // D3DKMT_ADAPTERADDRESS
    KMTQAITYPE_ADAPTERREGISTRYINFO_RENDER = 54, // D3DKMT_ADAPTERREGISTRYINFO
    KMTQAITYPE_CHECKDRIVERUPDATESTATUS_RENDER = 55,
    KMTQAITYPE_DRIVERVERSION_RENDER = 56, // D3DKMT_DRIVERVERSION
    KMTQAITYPE_ADAPTERTYPE_RENDER = 57, // D3DKMT_ADAPTERTYPE
    KMTQAITYPE_WDDM_1_2_CAPS_RENDER = 58, // D3DKMT_WDDM_1_2_CAPS
    KMTQAITYPE_WDDM_1_3_CAPS_RENDER = 59, // D3DKMT_WDDM_1_3_CAPS
    KMTQAITYPE_QUERY_ADAPTER_UNIQUE_GUID = 60, // D3DKMT_QUERY_ADAPTER_UNIQUE_GUID
    KMTQAITYPE_NODEPERFDATA = 61, // D3DKMT_NODE_PERFDATA
    KMTQAITYPE_ADAPTERPERFDATA = 62, // D3DKMT_ADAPTER_PERFDATA
    KMTQAITYPE_ADAPTERPERFDATA_CAPS = 63, // D3DKMT_ADAPTER_PERFDATACAPS
    KMTQUITYPE_GPUVERSION = 64, // D3DKMT_GPUVERSION
} KMTQUERYADAPTERINFOTYPE;

typedef struct _D3DKMT_QUERYADAPTERINFO
{
    _In_ D3DKMT_HANDLE AdapterHandle;
    _In_ KMTQUERYADAPTERINFOTYPE Type;
    _Out_ PVOID PrivateDriverData;
    _Out_ UINT32 PrivateDriverDataSize;
} D3DKMT_QUERYADAPTERINFO;

// Indicates the type of engine on a GPU node.
typedef enum _DXGK_ENGINE_TYPE
{
    DXGK_ENGINE_TYPE_OTHER = 0, // This value is used for proprietary or unique functionality that is not exposed by typical adapters, as well as for an engine that performs work that doesn't fall under another category.
    DXGK_ENGINE_TYPE_3D = 1, // The adapter's 3-D processing engine. All adapters that are not a display-only device have one 3-D engine.
    DXGK_ENGINE_TYPE_VIDEO_DECODE = 2, // The engine that handles video decoding, including decompression of video frames from an input stream into typical YUV surfaces. The workload packets for an H.264 video codec workload test must appear on either the decode engine or the 3-D engine.
    DXGK_ENGINE_TYPE_VIDEO_ENCODE = 3, // The engine that handles video encoding, including compression of typical video frames into an encoded video format.
    DXGK_ENGINE_TYPE_VIDEO_PROCESSING = 4, // The engine that is responsible for any video processing that is done after a video input stream is decoded. Such processing can include RGB surface conversion, filtering, stretching, color correction, deinterlacing, or other steps that are required before the final image is rendered to the display screen. The workload packets for workload tests must appear on either the video processing engine or the 3-D engine.
    DXGK_ENGINE_TYPE_SCENE_ASSEMBLY = 5, // The engine that performs vertex processing of 3-D workloads as a preliminary pass prior to the remainder of the 3-D rendering. This engine also stores vertices in bins that tile-based rendering engines use.
    DXGK_ENGINE_TYPE_COPY = 6, // The engine that is a copy engine used for moving data. This engine can perform subresource updates, blitting, paging, or other similar data handling. The workload packets for calls to CopySubresourceRegion or UpdateSubResource methods of Direct3D 10 and Direct3D 11 must appear on either the copy engine or the 3-D engine.
    DXGK_ENGINE_TYPE_OVERLAY = 7, // The virtual engine that is used for synchronized flipping of overlays in Direct3D 9.
    DXGK_ENGINE_TYPE_CRYPTO,
    DXGK_ENGINE_TYPE_MAX
} DXGK_ENGINE_TYPE;

#define DXGK_MAX_METADATA_NAME_LENGTH 32

#include <pshpack1.h>
typedef struct _D3DKMT_NODEMETADATA
{
    union
    {
        _In_ UINT32 NodeOrdinalAndAdapterIndex;
        struct
        {
            UINT32 NodeOrdinal : 16;
            UINT32 AdapterIndex : 16;
        };
    };
    _Out_ DXGK_ENGINE_TYPE EngineType;
    _Out_ WCHAR FriendlyName[DXGK_MAX_METADATA_NAME_LENGTH];
    _Out_ UINT32 Reserved;
    _Out_ BOOLEAN GpuMmuSupported; // Indicates whether the graphics engines of the node support the GpuMmu model. // since WDDM2_0
    _Out_ BOOLEAN IoMmuSupported; // Indicates whether the graphics engines of the node support the SVM model.
} D3DKMT_NODEMETADATA;
#include <poppack.h>

// Function pointers

typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENADAPTERFROMDEVICENAME)(_Inout_ D3DKMT_OPENADAPTERFROMDEVICENAME *);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_OPENADAPTERFROMHDC)(_Inout_ D3DKMT_OPENADAPTERFROMHDC *);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_CLOSEADAPTER)(_In_ const D3DKMT_CLOSEADAPTER *);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYSTATISTICS)(_In_ const D3DKMT_QUERYSTATISTICS *);
typedef _Check_return_ NTSTATUS (APIENTRY *PFND3DKMT_QUERYADAPTERINFO)(_Inout_ const D3DKMT_QUERYADAPTERINFO *);

#endif
