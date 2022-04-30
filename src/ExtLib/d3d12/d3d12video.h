

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0626 */
/* at Mon Jan 18 22:14:07 2038
 */
/* Compiler settings for d3d12video.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0626 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __d3d12video_h__
#define __d3d12video_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if _CONTROL_FLOW_GUARD_XFG
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

#ifndef __ID3D12VideoDevice_FWD_DEFINED__
#define __ID3D12VideoDevice_FWD_DEFINED__
typedef interface ID3D12VideoDevice ID3D12VideoDevice;

#endif 	/* __ID3D12VideoDevice_FWD_DEFINED__ */


#ifndef __ID3D12VideoDecoder_FWD_DEFINED__
#define __ID3D12VideoDecoder_FWD_DEFINED__
typedef interface ID3D12VideoDecoder ID3D12VideoDecoder;

#endif 	/* __ID3D12VideoDecoder_FWD_DEFINED__ */


#ifndef __ID3D12VideoDecoderHeap_FWD_DEFINED__
#define __ID3D12VideoDecoderHeap_FWD_DEFINED__
typedef interface ID3D12VideoDecoderHeap ID3D12VideoDecoderHeap;

#endif 	/* __ID3D12VideoDecoderHeap_FWD_DEFINED__ */


#ifndef __ID3D12VideoDecodeCommandList_FWD_DEFINED__
#define __ID3D12VideoDecodeCommandList_FWD_DEFINED__
typedef interface ID3D12VideoDecodeCommandList ID3D12VideoDecodeCommandList;

#endif 	/* __ID3D12VideoDecodeCommandList_FWD_DEFINED__ */


/* header files for imported files */
#include "d3d12.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_d3d12video_0000_0000 */
/* [local] */ 

typedef 
enum D3D12_FEATURE_VIDEO
    {
        D3D12_FEATURE_VIDEO_DECODE_SUPPORT	= 0,
        D3D12_FEATURE_VIDEO_DECODE_PROFILES	= 1,
        D3D12_FEATURE_VIDEO_DECODE_FORMATS	= 2,
        D3D12_FEATURE_VIDEO_DECODE_CONVERSION_SUPPORT	= 3,
        D3D12_FEATURE_VIDEO_PROCESS_SUPPORT	= 5,
        D3D12_FEATURE_VIDEO_PROCESS_MAX_INPUT_STREAMS	= 6,
        D3D12_FEATURE_VIDEO_PROCESS_REFERENCE_INFO	= 7,
        D3D12_FEATURE_VIDEO_DECODER_HEAP_SIZE	= 8,
        D3D12_FEATURE_VIDEO_PROCESSOR_SIZE	= 9,
        D3D12_FEATURE_VIDEO_DECODE_PROFILE_COUNT	= 10,
        D3D12_FEATURE_VIDEO_DECODE_FORMAT_COUNT	= 11,
        D3D12_FEATURE_VIDEO_ARCHITECTURE	= 17,
        D3D12_FEATURE_VIDEO_DECODE_HISTOGRAM	= 18,
        D3D12_FEATURE_VIDEO_FEATURE_AREA_SUPPORT	= 19,
        D3D12_FEATURE_VIDEO_MOTION_ESTIMATOR	= 20,
        D3D12_FEATURE_VIDEO_MOTION_ESTIMATOR_SIZE	= 21,
        D3D12_FEATURE_VIDEO_EXTENSION_COMMAND_COUNT	= 22,
        D3D12_FEATURE_VIDEO_EXTENSION_COMMANDS	= 23,
        D3D12_FEATURE_VIDEO_EXTENSION_COMMAND_PARAMETER_COUNT	= 24,
        D3D12_FEATURE_VIDEO_EXTENSION_COMMAND_PARAMETERS	= 25,
        D3D12_FEATURE_VIDEO_EXTENSION_COMMAND_SUPPORT	= 26,
        D3D12_FEATURE_VIDEO_EXTENSION_COMMAND_SIZE	= 27,
        D3D12_FEATURE_VIDEO_DECODE_PROTECTED_RESOURCES	= 28,
        D3D12_FEATURE_VIDEO_PROCESS_PROTECTED_RESOURCES	= 29,
        D3D12_FEATURE_VIDEO_MOTION_ESTIMATOR_PROTECTED_RESOURCES	= 30,
        D3D12_FEATURE_VIDEO_DECODER_HEAP_SIZE1	= 31,
        D3D12_FEATURE_VIDEO_PROCESSOR_SIZE1	= 32
    } 	D3D12_FEATURE_VIDEO;

typedef 
enum D3D12_BITSTREAM_ENCRYPTION_TYPE
    {
        D3D12_BITSTREAM_ENCRYPTION_TYPE_NONE	= 0
    } 	D3D12_BITSTREAM_ENCRYPTION_TYPE;

typedef 
enum D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE
    {
        D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE	= 0,
        D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_FIELD_BASED	= ( D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE + 1 ) 
    } 	D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE;

typedef 
enum D3D12_VIDEO_PROCESS_ALPHA_FILL_MODE
    {
        D3D12_VIDEO_PROCESS_ALPHA_FILL_MODE_OPAQUE	= 0,
        D3D12_VIDEO_PROCESS_ALPHA_FILL_MODE_BACKGROUND	= ( D3D12_VIDEO_PROCESS_ALPHA_FILL_MODE_OPAQUE + 1 ) ,
        D3D12_VIDEO_PROCESS_ALPHA_FILL_MODE_DESTINATION	= ( D3D12_VIDEO_PROCESS_ALPHA_FILL_MODE_BACKGROUND + 1 ) ,
        D3D12_VIDEO_PROCESS_ALPHA_FILL_MODE_SOURCE_STREAM	= ( D3D12_VIDEO_PROCESS_ALPHA_FILL_MODE_DESTINATION + 1 ) 
    } 	D3D12_VIDEO_PROCESS_ALPHA_FILL_MODE;

typedef 
enum D3D12_VIDEO_PROCESS_FILTER_FLAGS
    {
        D3D12_VIDEO_PROCESS_FILTER_FLAG_NONE	= 0,
        D3D12_VIDEO_PROCESS_FILTER_FLAG_BRIGHTNESS	= ( D3D12_VIDEO_PROCESS_FILTER_FLAG_NONE + 1 ) ,
        D3D12_VIDEO_PROCESS_FILTER_FLAG_CONTRAST	= ( D3D12_VIDEO_PROCESS_FILTER_FLAG_BRIGHTNESS + 1 ) ,
        D3D12_VIDEO_PROCESS_FILTER_FLAG_HUE	= ( D3D12_VIDEO_PROCESS_FILTER_FLAG_CONTRAST + 1 ) ,
        D3D12_VIDEO_PROCESS_FILTER_FLAG_SATURATION	= ( D3D12_VIDEO_PROCESS_FILTER_FLAG_HUE + 1 ) ,
        D3D12_VIDEO_PROCESS_FILTER_FLAG_NOISE_REDUCTION	= ( D3D12_VIDEO_PROCESS_FILTER_FLAG_SATURATION + 1 ) ,
        D3D12_VIDEO_PROCESS_FILTER_FLAG_EDGE_ENHANCEMENT	= ( D3D12_VIDEO_PROCESS_FILTER_FLAG_NOISE_REDUCTION + 1 ) ,
        D3D12_VIDEO_PROCESS_FILTER_FLAG_ANAMORPHIC_SCALING	= ( D3D12_VIDEO_PROCESS_FILTER_FLAG_EDGE_ENHANCEMENT + 1 ) ,
        D3D12_VIDEO_PROCESS_FILTER_FLAG_STEREO_ADJUSTMENT	= ( D3D12_VIDEO_PROCESS_FILTER_FLAG_ANAMORPHIC_SCALING + 1 ) 
    } 	D3D12_VIDEO_PROCESS_FILTER_FLAGS;

typedef 
enum D3D12_VIDEO_FRAME_STEREO_FORMAT
    {
        D3D12_VIDEO_FRAME_STEREO_FORMAT_NONE	= 0,
        D3D12_VIDEO_FRAME_STEREO_FORMAT_MONO	= ( D3D12_VIDEO_FRAME_STEREO_FORMAT_NONE + 1 ) ,
        D3D12_VIDEO_FRAME_STEREO_FORMAT_HORIZONTAL	= ( D3D12_VIDEO_FRAME_STEREO_FORMAT_MONO + 1 ) ,
        D3D12_VIDEO_FRAME_STEREO_FORMAT_VERTICAL	= ( D3D12_VIDEO_FRAME_STEREO_FORMAT_HORIZONTAL + 1 ) ,
        D3D12_VIDEO_FRAME_STEREO_FORMAT_SEPARATE	= ( D3D12_VIDEO_FRAME_STEREO_FORMAT_VERTICAL + 1 ) 
    } 	D3D12_VIDEO_FRAME_STEREO_FORMAT;

typedef 
enum D3D12_VIDEO_FIELD_TYPE
    {
        D3D12_VIDEO_FIELD_TYPE_NONE	= 0,
        D3D12_VIDEO_FIELD_TYPE_INTERLACED_TOP_FIELD_FIRST	= ( D3D12_VIDEO_FIELD_TYPE_NONE + 1 ) ,
        D3D12_VIDEO_FIELD_TYPE_INTERLACED_BOTTOM_FIELD_FIRST	= ( D3D12_VIDEO_FIELD_TYPE_INTERLACED_TOP_FIELD_FIRST + 1 ) 
    } 	D3D12_VIDEO_FIELD_TYPE;

typedef 
enum D3D12_VIDEO_PROCESS_DEINTERLACE_FLAGS
    {
        D3D12_VIDEO_PROCESS_DEINTERLACE_FLAG_NONE	= 0,
        D3D12_VIDEO_PROCESS_DEINTERLACE_FLAG_BOB	= ( D3D12_VIDEO_PROCESS_DEINTERLACE_FLAG_NONE + 1 ) ,
        D3D12_VIDEO_PROCESS_DEINTERLACE_FLAG_CUSTOM	= ( D3D12_VIDEO_PROCESS_DEINTERLACE_FLAG_BOB + 1 ) 
    } 	D3D12_VIDEO_PROCESS_DEINTERLACE_FLAGS;

typedef 
enum D3D12_VIDEO_DECODE_ARGUMENT_TYPE
    {
        D3D12_VIDEO_DECODE_ARGUMENT_TYPE_PICTURE_PARAMETERS	= 0,
        D3D12_VIDEO_DECODE_ARGUMENT_TYPE_INVERSE_QUANTIZATION_MATRIX	= 1,
        D3D12_VIDEO_DECODE_ARGUMENT_TYPE_SLICE_CONTROL	= 2,
        D3D12_VIDEO_DECODE_ARGUMENT_TYPE_MAX_VALID	= 3
    } 	D3D12_VIDEO_DECODE_ARGUMENT_TYPE;

typedef 
enum D3D12_VIDEO_DECODE_SUPPORT_FLAGS
    {
        D3D12_VIDEO_DECODE_SUPPORT_FLAG_NONE	= 0,
        D3D12_VIDEO_DECODE_SUPPORT_FLAG_SUPPORTED	= 0x1
    } 	D3D12_VIDEO_DECODE_SUPPORT_FLAGS;

typedef 
enum D3D12_VIDEO_DECODE_CONFIGURATION_FLAGS
    {
        D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_NONE	= 0,
        D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_HEIGHT_ALIGNMENT_MULTIPLE_32_REQUIRED	= 0x1,
        D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_POST_PROCESSING_SUPPORTED	= 0x2,
        D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_REFERENCE_ONLY_ALLOCATIONS_REQUIRED	= 0x4,
        D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_ALLOW_RESOLUTION_CHANGE_ON_NON_KEY_FRAME	= 0x8
    } 	D3D12_VIDEO_DECODE_CONFIGURATION_FLAGS;

typedef 
enum D3D12_VIDEO_DECODE_TIER
    {
        D3D12_VIDEO_DECODE_TIER_NOT_SUPPORTED	= 0,
        D3D12_VIDEO_DECODE_TIER_1	= ( D3D12_VIDEO_DECODE_TIER_NOT_SUPPORTED + 1 ) ,
        D3D12_VIDEO_DECODE_TIER_2	= ( D3D12_VIDEO_DECODE_TIER_1 + 1 ) ,
        D3D12_VIDEO_DECODE_TIER_3	= ( D3D12_VIDEO_DECODE_TIER_2 + 1 ) 
    } 	D3D12_VIDEO_DECODE_TIER;


typedef struct D3D12_VIDEO_DECODE_CONFIGURATION
    {
    GUID DecodeProfile;
    D3D12_BITSTREAM_ENCRYPTION_TYPE BitstreamEncryption;
    D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE InterlaceType;
    } 	D3D12_VIDEO_DECODE_CONFIGURATION;

typedef struct D3D12_VIDEO_DECODER_DESC
    {
    UINT NodeMask;
    D3D12_VIDEO_DECODE_CONFIGURATION Configuration;
    } 	D3D12_VIDEO_DECODER_DESC;

typedef struct D3D12_VIDEO_DECODER_HEAP_DESC
    {
    UINT NodeMask;
    D3D12_VIDEO_DECODE_CONFIGURATION Configuration;
    UINT DecodeWidth;
    UINT DecodeHeight;
    DXGI_FORMAT Format;
    DXGI_RATIONAL FrameRate;
    UINT BitRate;
    UINT MaxDecodePictureBufferCount;
    } 	D3D12_VIDEO_DECODER_HEAP_DESC;

typedef struct D3D12_VIDEO_PROCESS_OUTPUT_STREAM_DESC
    {
    DXGI_FORMAT Format;
    DXGI_COLOR_SPACE_TYPE ColorSpace;
    D3D12_VIDEO_PROCESS_ALPHA_FILL_MODE AlphaFillMode;
    UINT AlphaFillModeSourceStreamIndex;
    FLOAT BackgroundColor[ 4 ];
    DXGI_RATIONAL FrameRate;
    BOOL EnableStereo;
    } 	D3D12_VIDEO_PROCESS_OUTPUT_STREAM_DESC;

typedef struct D3D12_VIDEO_SIZE_RANGE
    {
    UINT MaxWidth;
    UINT MaxHeight;
    UINT MinWidth;
    UINT MinHeight;
    } 	D3D12_VIDEO_SIZE_RANGE;

typedef struct D3D12_VIDEO_PROCESS_LUMA_KEY
    {
    BOOL Enable;
    FLOAT Lower;
    FLOAT Upper;
    } 	D3D12_VIDEO_PROCESS_LUMA_KEY;

typedef struct D3D12_VIDEO_PROCESS_INPUT_STREAM_DESC
    {
    DXGI_FORMAT Format;
    DXGI_COLOR_SPACE_TYPE ColorSpace;
    DXGI_RATIONAL SourceAspectRatio;
    DXGI_RATIONAL DestinationAspectRatio;
    DXGI_RATIONAL FrameRate;
    D3D12_VIDEO_SIZE_RANGE SourceSizeRange;
    D3D12_VIDEO_SIZE_RANGE DestinationSizeRange;
    BOOL EnableOrientation;
    D3D12_VIDEO_PROCESS_FILTER_FLAGS FilterFlags;
    D3D12_VIDEO_FRAME_STEREO_FORMAT StereoFormat;
    D3D12_VIDEO_FIELD_TYPE FieldType;
    D3D12_VIDEO_PROCESS_DEINTERLACE_FLAGS DeinterlaceMode;
    BOOL EnableAlphaBlending;
    D3D12_VIDEO_PROCESS_LUMA_KEY LumaKey;
    UINT NumPastFrames;
    UINT NumFutureFrames;
    BOOL EnableAutoProcessing;
    } 	D3D12_VIDEO_PROCESS_INPUT_STREAM_DESC;

typedef struct D3D12_FEATURE_DATA_VIDEO_DECODE_PROFILE_COUNT
    {
    UINT NodeIndex;
    UINT ProfileCount;
    } 	D3D12_FEATURE_DATA_VIDEO_DECODE_PROFILE_COUNT;

typedef struct D3D12_FEATURE_DATA_VIDEO_DECODE_PROFILES
    {
    UINT NodeIndex;
    UINT ProfileCount;
    GUID *pProfiles;
    } 	D3D12_FEATURE_DATA_VIDEO_DECODE_PROFILES;

typedef struct D3D12_FEATURE_DATA_VIDEO_DECODE_FORMAT_COUNT
    {
    UINT NodeIndex;
    D3D12_VIDEO_DECODE_CONFIGURATION Configuration;
    UINT FormatCount;
    } 	D3D12_FEATURE_DATA_VIDEO_DECODE_FORMAT_COUNT;

typedef struct D3D12_FEATURE_DATA_VIDEO_DECODE_FORMATS
    {
    UINT NodeIndex;
    D3D12_VIDEO_DECODE_CONFIGURATION Configuration;
    UINT FormatCount;
    DXGI_FORMAT *pOutputFormats;
    } 	D3D12_FEATURE_DATA_VIDEO_DECODE_FORMATS;

typedef struct D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT
    {
    UINT NodeIndex;
    D3D12_VIDEO_DECODE_CONFIGURATION Configuration;
    UINT Width;
    UINT Height;
    DXGI_FORMAT DecodeFormat;
    DXGI_RATIONAL FrameRate;
    UINT BitRate;
    D3D12_VIDEO_DECODE_SUPPORT_FLAGS SupportFlags;
    D3D12_VIDEO_DECODE_CONFIGURATION_FLAGS ConfigurationFlags;
    D3D12_VIDEO_DECODE_TIER DecodeTier;
    } 	D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT;

typedef struct D3D12_VIDEO_DECODE_CONVERSION_ARGUMENTS
    {
    BOOL Enable;
    ID3D12Resource *pReferenceTexture2D;
    UINT ReferenceSubresource;
    DXGI_COLOR_SPACE_TYPE OutputColorSpace;
    DXGI_COLOR_SPACE_TYPE DecodeColorSpace;
    } 	D3D12_VIDEO_DECODE_CONVERSION_ARGUMENTS;

typedef struct D3D12_VIDEO_DECODE_OUTPUT_STREAM_ARGUMENTS
    {
    ID3D12Resource *pOutputTexture2D;
    UINT OutputSubresource;
    D3D12_VIDEO_DECODE_CONVERSION_ARGUMENTS ConversionArguments;
    } 	D3D12_VIDEO_DECODE_OUTPUT_STREAM_ARGUMENTS;

typedef struct D3D12_VIDEO_DECODE_FRAME_ARGUMENT
    {
    D3D12_VIDEO_DECODE_ARGUMENT_TYPE Type;
    UINT Size;
    void *pData;
    } 	D3D12_VIDEO_DECODE_FRAME_ARGUMENT;

typedef struct D3D12_VIDEO_DECODE_REFERENCE_FRAMES
    {
    UINT NumTexture2Ds;
    ID3D12Resource **ppTexture2Ds;
    UINT *pSubresources;
    ID3D12VideoDecoderHeap **ppHeaps;
    } 	D3D12_VIDEO_DECODE_REFERENCE_FRAMES;

typedef struct D3D12_VIDEO_DECODE_COMPRESSED_BITSTREAM
    {
    ID3D12Resource *pBuffer;
    UINT64 Offset;
    UINT64 Size;
    } 	D3D12_VIDEO_DECODE_COMPRESSED_BITSTREAM;

typedef struct D3D12_VIDEO_DECODE_INPUT_STREAM_ARGUMENTS
    {
    UINT NumFrameArguments;
    D3D12_VIDEO_DECODE_FRAME_ARGUMENT FrameArguments[ 10 ];
    D3D12_VIDEO_DECODE_REFERENCE_FRAMES ReferenceFrames;
    D3D12_VIDEO_DECODE_COMPRESSED_BITSTREAM CompressedBitstream;
    ID3D12VideoDecoderHeap *pHeap;
    } 	D3D12_VIDEO_DECODE_INPUT_STREAM_ARGUMENTS;



extern RPC_IF_HANDLE __MIDL_itf_d3d12video_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_d3d12video_0000_0000_v0_0_s_ifspec;

#ifndef __ID3D12VideoDevice_INTERFACE_DEFINED__
#define __ID3D12VideoDevice_INTERFACE_DEFINED__

/* interface ID3D12VideoDevice */
/* [unique][local][object][uuid] */ 


/*EXTERN_C const IID IID_ID3D12VideoDevice;*/
DEFINE_GUID(IID_ID3D12VideoDevice, 0x1F052807, 0x0B46, 0x4ACC, 0x8A,0x89, 0x36,0x4F,0x79,0x37,0x18,0xA4);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1F052807-0B46-4ACC-8A89-364F793718A4")
    ID3D12VideoDevice : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport( 
            D3D12_FEATURE_VIDEO FeatureVideo,
            void *pFeatureSupportData,
            UINT FeatureSupportDataSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateVideoDecoder( 
            const D3D12_VIDEO_DECODER_DESC *pDesc,
            REFIID riid,
            void **ppVideoDecoder) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateVideoDecoderHeap( 
            const D3D12_VIDEO_DECODER_HEAP_DESC *pVideoDecoderHeapDesc,
            REFIID riid,
            void **ppVideoDecoderHeap) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateVideoProcessor( 
            UINT NodeMask,
            const D3D12_VIDEO_PROCESS_OUTPUT_STREAM_DESC *pOutputStreamDesc,
            UINT NumInputStreamDescs,
            const D3D12_VIDEO_PROCESS_INPUT_STREAM_DESC *pInputStreamDescs,
            REFIID riid,
            void **ppVideoProcessor) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D12VideoDeviceVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D12VideoDevice * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D12VideoDevice * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D12VideoDevice * This);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDevice, CheckFeatureSupport)
        HRESULT ( STDMETHODCALLTYPE *CheckFeatureSupport )( 
            ID3D12VideoDevice * This,
            D3D12_FEATURE_VIDEO FeatureVideo,
            void *pFeatureSupportData,
            UINT FeatureSupportDataSize);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDevice, CreateVideoDecoder)
        HRESULT ( STDMETHODCALLTYPE *CreateVideoDecoder )( 
            ID3D12VideoDevice * This,
            const D3D12_VIDEO_DECODER_DESC *pDesc,
            REFIID riid,
            void **ppVideoDecoder);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDevice, CreateVideoDecoderHeap)
        HRESULT ( STDMETHODCALLTYPE *CreateVideoDecoderHeap )( 
            ID3D12VideoDevice * This,
            const D3D12_VIDEO_DECODER_HEAP_DESC *pVideoDecoderHeapDesc,
            REFIID riid,
            void **ppVideoDecoderHeap);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDevice, CreateVideoProcessor)
        HRESULT ( STDMETHODCALLTYPE *CreateVideoProcessor )( 
            ID3D12VideoDevice * This,
            UINT NodeMask,
            const D3D12_VIDEO_PROCESS_OUTPUT_STREAM_DESC *pOutputStreamDesc,
            UINT NumInputStreamDescs,
            const D3D12_VIDEO_PROCESS_INPUT_STREAM_DESC *pInputStreamDescs,
            REFIID riid,
            void **ppVideoProcessor);
        
        END_INTERFACE
    } ID3D12VideoDeviceVtbl;

    interface ID3D12VideoDevice
    {
        CONST_VTBL struct ID3D12VideoDeviceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D12VideoDevice_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12VideoDevice_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12VideoDevice_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12VideoDevice_CheckFeatureSupport(This,FeatureVideo,pFeatureSupportData,FeatureSupportDataSize)	\
    ( (This)->lpVtbl -> CheckFeatureSupport(This,FeatureVideo,pFeatureSupportData,FeatureSupportDataSize) ) 

#define ID3D12VideoDevice_CreateVideoDecoder(This,pDesc,riid,ppVideoDecoder)	\
    ( (This)->lpVtbl -> CreateVideoDecoder(This,pDesc,riid,ppVideoDecoder) ) 

#define ID3D12VideoDevice_CreateVideoDecoderHeap(This,pVideoDecoderHeapDesc,riid,ppVideoDecoderHeap)	\
    ( (This)->lpVtbl -> CreateVideoDecoderHeap(This,pVideoDecoderHeapDesc,riid,ppVideoDecoderHeap) ) 

#define ID3D12VideoDevice_CreateVideoProcessor(This,NodeMask,pOutputStreamDesc,NumInputStreamDescs,pInputStreamDescs,riid,ppVideoProcessor)	\
    ( (This)->lpVtbl -> CreateVideoProcessor(This,NodeMask,pOutputStreamDesc,NumInputStreamDescs,pInputStreamDescs,riid,ppVideoProcessor) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12VideoDevice_INTERFACE_DEFINED__ */


#ifndef __ID3D12VideoDecoder_INTERFACE_DEFINED__
#define __ID3D12VideoDecoder_INTERFACE_DEFINED__

/* interface ID3D12VideoDecoder */
/* [unique][local][object][uuid] */ 


/*EXTERN_C const IID IID_ID3D12VideoDecoder;*/
DEFINE_GUID(IID_ID3D12VideoDecoder, 0xC59B6BDC, 0x7720, 0x4074, 0xA1,0x36, 0x17,0xA1,0x56,0x03,0x74,0x70);
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C59B6BDC-7720-4074-A136-17A156037470")
    ID3D12VideoDecoder : public ID3D12Pageable
    {
    public:
        virtual D3D12_VIDEO_DECODER_DESC STDMETHODCALLTYPE GetDesc( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D12VideoDecoderVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D12VideoDecoder * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D12VideoDecoder * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D12VideoDecoder * This);
        
        DECLSPEC_XFGVIRT(ID3D12Object, GetPrivateData)
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D12VideoDecoder * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        DECLSPEC_XFGVIRT(ID3D12Object, SetPrivateData)
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D12VideoDecoder * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        DECLSPEC_XFGVIRT(ID3D12Object, SetPrivateDataInterface)
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D12VideoDecoder * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        DECLSPEC_XFGVIRT(ID3D12Object, SetName)
        HRESULT ( STDMETHODCALLTYPE *SetName )( 
            ID3D12VideoDecoder * This,
            /* [annotation] */ 
            _In_z_  LPCWSTR Name);
        
        DECLSPEC_XFGVIRT(ID3D12DeviceChild, GetDevice)
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D12VideoDecoder * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_opt_  void **ppvDevice);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecoder, GetDesc)
        D3D12_VIDEO_DECODER_DESC ( STDMETHODCALLTYPE *GetDesc )( 
            ID3D12VideoDecoder * This);
        
        END_INTERFACE
    } ID3D12VideoDecoderVtbl;

    interface ID3D12VideoDecoder
    {
        CONST_VTBL struct ID3D12VideoDecoderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D12VideoDecoder_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12VideoDecoder_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12VideoDecoder_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12VideoDecoder_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12VideoDecoder_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12VideoDecoder_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12VideoDecoder_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12VideoDecoder_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 



#define ID3D12VideoDecoder_GetDesc(This)	\
    ( (This)->lpVtbl -> GetDesc(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */



D3D12_VIDEO_DECODER_DESC STDMETHODCALLTYPE ID3D12VideoDecoder_GetDesc_Proxy( 
    ID3D12VideoDecoder * This);


void __RPC_STUB ID3D12VideoDecoder_GetDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ID3D12VideoDecoder_INTERFACE_DEFINED__ */


#ifndef __ID3D12VideoDecoderHeap_INTERFACE_DEFINED__
#define __ID3D12VideoDecoderHeap_INTERFACE_DEFINED__

/* interface ID3D12VideoDecoderHeap */
/* [unique][local][object][uuid] */ 


//EXTERN_C const IID IID_ID3D12VideoDecoderHeap;
DEFINE_GUID(IID_ID3D12VideoDecoderHeap,0x0946B7C9,0xEBF6,0x4047,0xBB,0x73,0x86,0x83,0xE2,0x7D,0xBB,0x1F);
#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0946B7C9-EBF6-4047-BB73-8683E27DBB1F")
    ID3D12VideoDecoderHeap : public ID3D12Pageable
    {
    public:
        virtual D3D12_VIDEO_DECODER_HEAP_DESC STDMETHODCALLTYPE GetDesc( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D12VideoDecoderHeapVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D12VideoDecoderHeap * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D12VideoDecoderHeap * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D12VideoDecoderHeap * This);
        
        DECLSPEC_XFGVIRT(ID3D12Object, GetPrivateData)
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D12VideoDecoderHeap * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        DECLSPEC_XFGVIRT(ID3D12Object, SetPrivateData)
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D12VideoDecoderHeap * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        DECLSPEC_XFGVIRT(ID3D12Object, SetPrivateDataInterface)
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D12VideoDecoderHeap * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        DECLSPEC_XFGVIRT(ID3D12Object, SetName)
        HRESULT ( STDMETHODCALLTYPE *SetName )( 
            ID3D12VideoDecoderHeap * This,
            /* [annotation] */ 
            _In_z_  LPCWSTR Name);
        
        DECLSPEC_XFGVIRT(ID3D12DeviceChild, GetDevice)
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D12VideoDecoderHeap * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_opt_  void **ppvDevice);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecoderHeap, GetDesc)
        D3D12_VIDEO_DECODER_HEAP_DESC ( STDMETHODCALLTYPE *GetDesc )( 
            ID3D12VideoDecoderHeap * This);
        
        END_INTERFACE
    } ID3D12VideoDecoderHeapVtbl;

    interface ID3D12VideoDecoderHeap
    {
        CONST_VTBL struct ID3D12VideoDecoderHeapVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D12VideoDecoderHeap_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12VideoDecoderHeap_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12VideoDecoderHeap_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12VideoDecoderHeap_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12VideoDecoderHeap_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12VideoDecoderHeap_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12VideoDecoderHeap_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12VideoDecoderHeap_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 



#define ID3D12VideoDecoderHeap_GetDesc(This)	\
    ( (This)->lpVtbl -> GetDesc(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */



D3D12_VIDEO_DECODER_HEAP_DESC STDMETHODCALLTYPE ID3D12VideoDecoderHeap_GetDesc_Proxy( 
    ID3D12VideoDecoderHeap * This);


void __RPC_STUB ID3D12VideoDecoderHeap_GetDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ID3D12VideoDecoderHeap_INTERFACE_DEFINED__ */


#ifndef __ID3D12VideoDecodeCommandList_INTERFACE_DEFINED__
#define __ID3D12VideoDecodeCommandList_INTERFACE_DEFINED__

/* interface ID3D12VideoDecodeCommandList */
/* [unique][local][object][uuid] */ 


DEFINE_GUID(IID_ID3D12VideoDecodeCommandList,0x3B60536E,0xAD29,0x4E64,0xA2,0x69,0xF8,0x53,0x83,0x7E,0x5E,0x53);
//EXTERN_C const IID IID_ID3D12VideoDecodeCommandList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3B60536E-AD29-4E64-A269-F853837E5E53")
    ID3D12VideoDecodeCommandList : public ID3D12CommandList
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( 
            ID3D12CommandAllocator *pAllocator) = 0;
        
        virtual void STDMETHODCALLTYPE ClearState( void) = 0;
        
        virtual void STDMETHODCALLTYPE ResourceBarrier( 
            UINT NumBarriers,
            const D3D12_RESOURCE_BARRIER *pBarriers) = 0;
        
        virtual void STDMETHODCALLTYPE DiscardResource( 
            ID3D12Resource *pResource,
            const D3D12_DISCARD_REGION *pRegion) = 0;
        
        virtual void STDMETHODCALLTYPE BeginQuery( 
            ID3D12QueryHeap *pQueryHeap,
            D3D12_QUERY_TYPE Type,
            UINT Index) = 0;
        
        virtual void STDMETHODCALLTYPE EndQuery( 
            ID3D12QueryHeap *pQueryHeap,
            D3D12_QUERY_TYPE Type,
            UINT Index) = 0;
        
        virtual void STDMETHODCALLTYPE ResolveQueryData( 
            ID3D12QueryHeap *pQueryHeap,
            D3D12_QUERY_TYPE Type,
            UINT StartIndex,
            UINT NumQueries,
            ID3D12Resource *pDestinationBuffer,
            UINT64 AlignedDestinationBufferOffset) = 0;
        
        virtual void STDMETHODCALLTYPE SetPredication( 
            ID3D12Resource *pBuffer,
            UINT64 AlignedBufferOffset,
            D3D12_PREDICATION_OP Operation) = 0;
        
        virtual void STDMETHODCALLTYPE SetMarker( 
            UINT Metadata,
            const void *pData,
            UINT Size) = 0;
        
        virtual void STDMETHODCALLTYPE BeginEvent( 
            UINT Metadata,
            const void *pData,
            UINT Size) = 0;
        
        virtual void STDMETHODCALLTYPE EndEvent( void) = 0;
        
        virtual void STDMETHODCALLTYPE DecodeFrame( 
            ID3D12VideoDecoder *pDecoder,
            const D3D12_VIDEO_DECODE_OUTPUT_STREAM_ARGUMENTS *pOutputArguments,
            const D3D12_VIDEO_DECODE_INPUT_STREAM_ARGUMENTS *pInputArguments) = 0;
        
        virtual void STDMETHODCALLTYPE WriteBufferImmediate( 
            UINT Count,
            const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER *pParams,
            const D3D12_WRITEBUFFERIMMEDIATE_MODE *pModes) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ID3D12VideoDecodeCommandListVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ID3D12VideoDecodeCommandList * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ID3D12VideoDecodeCommandList * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ID3D12VideoDecodeCommandList * This);
        
        DECLSPEC_XFGVIRT(ID3D12Object, GetPrivateData)
        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
            ID3D12VideoDecodeCommandList * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _Inout_  UINT *pDataSize,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);
        
        DECLSPEC_XFGVIRT(ID3D12Object, SetPrivateData)
        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
            ID3D12VideoDecodeCommandList * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData);
        
        DECLSPEC_XFGVIRT(ID3D12Object, SetPrivateDataInterface)
        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
            ID3D12VideoDecodeCommandList * This,
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_opt_  const IUnknown *pData);
        
        DECLSPEC_XFGVIRT(ID3D12Object, SetName)
        HRESULT ( STDMETHODCALLTYPE *SetName )( 
            ID3D12VideoDecodeCommandList * This,
            /* [annotation] */ 
            _In_z_  LPCWSTR Name);
        
        DECLSPEC_XFGVIRT(ID3D12DeviceChild, GetDevice)
        HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
            ID3D12VideoDecodeCommandList * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_opt_  void **ppvDevice);
        
        DECLSPEC_XFGVIRT(ID3D12CommandList, GetType)
        D3D12_COMMAND_LIST_TYPE ( STDMETHODCALLTYPE *GetType )( 
            ID3D12VideoDecodeCommandList * This);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecodeCommandList, Close)
        HRESULT ( STDMETHODCALLTYPE *Close )( 
            ID3D12VideoDecodeCommandList * This);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecodeCommandList, Reset)
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ID3D12VideoDecodeCommandList * This,
            ID3D12CommandAllocator *pAllocator);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecodeCommandList, ClearState)
        void ( STDMETHODCALLTYPE *ClearState )( 
            ID3D12VideoDecodeCommandList * This);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecodeCommandList, ResourceBarrier)
        void ( STDMETHODCALLTYPE *ResourceBarrier )( 
            ID3D12VideoDecodeCommandList * This,
            UINT NumBarriers,
            const D3D12_RESOURCE_BARRIER *pBarriers);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecodeCommandList, DiscardResource)
        void ( STDMETHODCALLTYPE *DiscardResource )( 
            ID3D12VideoDecodeCommandList * This,
            ID3D12Resource *pResource,
            const D3D12_DISCARD_REGION *pRegion);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecodeCommandList, BeginQuery)
        void ( STDMETHODCALLTYPE *BeginQuery )( 
            ID3D12VideoDecodeCommandList * This,
            ID3D12QueryHeap *pQueryHeap,
            D3D12_QUERY_TYPE Type,
            UINT Index);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecodeCommandList, EndQuery)
        void ( STDMETHODCALLTYPE *EndQuery )( 
            ID3D12VideoDecodeCommandList * This,
            ID3D12QueryHeap *pQueryHeap,
            D3D12_QUERY_TYPE Type,
            UINT Index);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecodeCommandList, ResolveQueryData)
        void ( STDMETHODCALLTYPE *ResolveQueryData )( 
            ID3D12VideoDecodeCommandList * This,
            ID3D12QueryHeap *pQueryHeap,
            D3D12_QUERY_TYPE Type,
            UINT StartIndex,
            UINT NumQueries,
            ID3D12Resource *pDestinationBuffer,
            UINT64 AlignedDestinationBufferOffset);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecodeCommandList, SetPredication)
        void ( STDMETHODCALLTYPE *SetPredication )( 
            ID3D12VideoDecodeCommandList * This,
            ID3D12Resource *pBuffer,
            UINT64 AlignedBufferOffset,
            D3D12_PREDICATION_OP Operation);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecodeCommandList, SetMarker)
        void ( STDMETHODCALLTYPE *SetMarker )( 
            ID3D12VideoDecodeCommandList * This,
            UINT Metadata,
            const void *pData,
            UINT Size);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecodeCommandList, BeginEvent)
        void ( STDMETHODCALLTYPE *BeginEvent )( 
            ID3D12VideoDecodeCommandList * This,
            UINT Metadata,
            const void *pData,
            UINT Size);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecodeCommandList, EndEvent)
        void ( STDMETHODCALLTYPE *EndEvent )( 
            ID3D12VideoDecodeCommandList * This);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecodeCommandList, DecodeFrame)
        void ( STDMETHODCALLTYPE *DecodeFrame )( 
            ID3D12VideoDecodeCommandList * This,
            ID3D12VideoDecoder *pDecoder,
            const D3D12_VIDEO_DECODE_OUTPUT_STREAM_ARGUMENTS *pOutputArguments,
            const D3D12_VIDEO_DECODE_INPUT_STREAM_ARGUMENTS *pInputArguments);
        
        DECLSPEC_XFGVIRT(ID3D12VideoDecodeCommandList, WriteBufferImmediate)
        void ( STDMETHODCALLTYPE *WriteBufferImmediate )( 
            ID3D12VideoDecodeCommandList * This,
            UINT Count,
            const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER *pParams,
            const D3D12_WRITEBUFFERIMMEDIATE_MODE *pModes);
        
        END_INTERFACE
    } ID3D12VideoDecodeCommandListVtbl;

    interface ID3D12VideoDecodeCommandList
    {
        CONST_VTBL struct ID3D12VideoDecodeCommandListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ID3D12VideoDecodeCommandList_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ID3D12VideoDecodeCommandList_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ID3D12VideoDecodeCommandList_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ID3D12VideoDecodeCommandList_GetPrivateData(This,guid,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,guid,pDataSize,pData) ) 

#define ID3D12VideoDecodeCommandList_SetPrivateData(This,guid,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,guid,DataSize,pData) ) 

#define ID3D12VideoDecodeCommandList_SetPrivateDataInterface(This,guid,pData)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,guid,pData) ) 

#define ID3D12VideoDecodeCommandList_SetName(This,Name)	\
    ( (This)->lpVtbl -> SetName(This,Name) ) 


#define ID3D12VideoDecodeCommandList_GetDevice(This,riid,ppvDevice)	\
    ( (This)->lpVtbl -> GetDevice(This,riid,ppvDevice) ) 


#define ID3D12VideoDecodeCommandList_GetType(This)	\
    ( (This)->lpVtbl -> GetType(This) ) 


#define ID3D12VideoDecodeCommandList_Close(This)	\
    ( (This)->lpVtbl -> Close(This) ) 

#define ID3D12VideoDecodeCommandList_Reset(This,pAllocator)	\
    ( (This)->lpVtbl -> Reset(This,pAllocator) ) 

#define ID3D12VideoDecodeCommandList_ClearState(This)	\
    ( (This)->lpVtbl -> ClearState(This) ) 

#define ID3D12VideoDecodeCommandList_ResourceBarrier(This,NumBarriers,pBarriers)	\
    ( (This)->lpVtbl -> ResourceBarrier(This,NumBarriers,pBarriers) ) 

#define ID3D12VideoDecodeCommandList_DiscardResource(This,pResource,pRegion)	\
    ( (This)->lpVtbl -> DiscardResource(This,pResource,pRegion) ) 

#define ID3D12VideoDecodeCommandList_BeginQuery(This,pQueryHeap,Type,Index)	\
    ( (This)->lpVtbl -> BeginQuery(This,pQueryHeap,Type,Index) ) 

#define ID3D12VideoDecodeCommandList_EndQuery(This,pQueryHeap,Type,Index)	\
    ( (This)->lpVtbl -> EndQuery(This,pQueryHeap,Type,Index) ) 

#define ID3D12VideoDecodeCommandList_ResolveQueryData(This,pQueryHeap,Type,StartIndex,NumQueries,pDestinationBuffer,AlignedDestinationBufferOffset)	\
    ( (This)->lpVtbl -> ResolveQueryData(This,pQueryHeap,Type,StartIndex,NumQueries,pDestinationBuffer,AlignedDestinationBufferOffset) ) 

#define ID3D12VideoDecodeCommandList_SetPredication(This,pBuffer,AlignedBufferOffset,Operation)	\
    ( (This)->lpVtbl -> SetPredication(This,pBuffer,AlignedBufferOffset,Operation) ) 

#define ID3D12VideoDecodeCommandList_SetMarker(This,Metadata,pData,Size)	\
    ( (This)->lpVtbl -> SetMarker(This,Metadata,pData,Size) ) 

#define ID3D12VideoDecodeCommandList_BeginEvent(This,Metadata,pData,Size)	\
    ( (This)->lpVtbl -> BeginEvent(This,Metadata,pData,Size) ) 

#define ID3D12VideoDecodeCommandList_EndEvent(This)	\
    ( (This)->lpVtbl -> EndEvent(This) ) 

#define ID3D12VideoDecodeCommandList_DecodeFrame(This,pDecoder,pOutputArguments,pInputArguments)	\
    ( (This)->lpVtbl -> DecodeFrame(This,pDecoder,pOutputArguments,pInputArguments) ) 

#define ID3D12VideoDecodeCommandList_WriteBufferImmediate(This,Count,pParams,pModes)	\
    ( (This)->lpVtbl -> WriteBufferImmediate(This,Count,pParams,pModes) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ID3D12VideoDecodeCommandList_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_MPEG2, 0xee27417f, 0x5e28, 0x4e65, 0xbe, 0xea, 0x1d, 0x26, 0xb5, 0x08, 0xad, 0xc9); 
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_MPEG1_AND_MPEG2, 0x86695f12, 0x340e, 0x4f04, 0x9f, 0xd3, 0x92, 0x53, 0xdd, 0x32, 0x74, 0x60); 
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_H264, 0x1b81be68, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_H264_STEREO_PROGRESSIVE, 0xd79be8da, 0x0cf1, 0x4c81, 0xb8, 0x2a, 0x69, 0xa4, 0xe2, 0x36, 0xf4, 0x3d);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_H264_STEREO, 0xf9aaccbb, 0xc2b6, 0x4cfc, 0x87, 0x79, 0x57, 0x07, 0xb1, 0x76, 0x05, 0x52);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_H264_MULTIVIEW, 0x705b9d82, 0x76cf, 0x49d6, 0xb7, 0xe6, 0xac, 0x88, 0x72, 0xdb, 0x01, 0x3c);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_VC1, 0x1b81beA3, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_VC1_D2010, 0x1b81beA4, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_MPEG4PT2_SIMPLE, 0xefd64d74, 0xc9e8,0x41d7,0xa5,0xe9,0xe9,0xb0,0xe3,0x9f,0xa3,0x19);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_MPEG4PT2_ADVSIMPLE_NOGMC, 0xed418a9f, 0x010d, 0x4eda, 0x9a, 0xe3, 0x9a, 0x65, 0x35, 0x8d, 0x8d, 0x2e);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN, 0x5b11d51b, 0x2f4c, 0x4452, 0xbc, 0xc3, 0x09, 0xf2, 0xa1, 0x16, 0x0c, 0xc0);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN10, 0x107af0e0, 0xef1a, 0x4d19, 0xab, 0xa8, 0x67, 0xa1, 0x63, 0x07, 0x3d, 0x13);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_VP9, 0x463707f8, 0xa1d0, 0x4585, 0x87, 0x6d, 0x83, 0xaa, 0x6d, 0x60, 0xb8, 0x9e);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_VP9_10BIT_PROFILE2, 0xa4c749ef, 0x6ecf, 0x48aa, 0x84, 0x48, 0x50, 0xa7, 0xa1, 0x16, 0x5f, 0xf7);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_VP8, 0x90b899ea, 0x3a62, 0x4705, 0x88, 0xb3, 0x8d, 0xf0, 0x4b, 0x27, 0x44, 0xe7);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_AV1_PROFILE0, 0xb8be4ccb, 0xcf53, 0x46ba, 0x8d, 0x59, 0xd6, 0xb8, 0xa6, 0xda, 0x5d, 0x2a);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_AV1_PROFILE1, 0x6936ff0f, 0x45b1, 0x4163, 0x9c, 0xc1, 0x64, 0x6e, 0xf6, 0x94, 0x61, 0x08);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_AV1_PROFILE2, 0x0c5f2aa1, 0xe541, 0x4089, 0xbb, 0x7b, 0x98, 0x11, 0x0a, 0x19, 0xd7, 0xc8);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_AV1_12BIT_PROFILE2, 0x17127009, 0xa00f, 0x4ce1, 0x99, 0x4e, 0xbf, 0x40, 0x81, 0xf6, 0xf3, 0xf0);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_AV1_12BIT_PROFILE2_420, 0x2d80bed6, 0x9cac, 0x4835, 0x9e, 0x91, 0x32, 0x7b, 0xbc, 0x4f, 0x9e, 0xe8);
/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


