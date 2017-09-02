/******************************Module*Header**********************************\
*
* Module Name: d3dumddi.h
*
* Content: longhorn display driver user mode interfaces
*
* Copyright (c) 2003 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include <winapifamily.h>

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#ifndef _D3DUMDDI_H_
#define _D3DUMDDI_H_

#include "d3dukmdt.h"
#include <winerror.h>


#pragma warning(push)
#pragma warning(disable:4201) // anonymous unions warning

typedef struct _D3DDDIARG_SETDISPLAYMODE
{
    HANDLE                      hResource;              // Source surface
    UINT                        SubResourceIndex;       // Index of surface level
} D3DDDIARG_SETDISPLAYMODE;

typedef struct _D3DDDI_PRESENTFLAGS
{
    union
    {
        struct
        {
            UINT    Blt                 : 1;        // 0x00000001
            UINT    ColorFill           : 1;        // 0x00000002
            UINT    Flip                : 1;        // 0x00000004
            UINT    Reserved            :29;        // 0xFFFFFFF8
        };
        UINT    Value;
    };
} D3DDDI_PRESENTFLAGS;

typedef struct _D3DDDIARG_PRESENT
{
    HANDLE                      hSrcResource;           // in: Source surface
    UINT                        SrcSubResourceIndex;    // in: Index of surface level
    HANDLE                      hDstResource;           // in: if non-zero, it's the destination of the present
    UINT                        DstSubResourceIndex;    // in: Index of surface level
    D3DDDI_PRESENTFLAGS         Flags;                  // in: Presentation flags.
    D3DDDI_FLIPINTERVAL_TYPE    FlipInterval;           // in: Presentation interval (flip only)
} D3DDDIARG_PRESENT;

typedef struct _D3DDDIBOX
{
    UINT                Left;
    UINT                Top;
    UINT                Right;
    UINT                Bottom;
    UINT                Front;
    UINT                Back;
} D3DDDIBOX;

typedef struct _D3DDDIRANGE
{
    UINT                Offset;
    UINT                Size;
} D3DDDIRANGE;

typedef enum _D3DDDIRENDERSTATETYPE
{
    D3DDDIRS_ZENABLE                    = 7,    /* D3DZBUFFERTYPE (or TRUE/FALSE for legacy) */
    D3DDDIRS_FILLMODE                   = 8,    /* D3DFILLMODE */
    D3DDDIRS_SHADEMODE                  = 9,    /* D3DSHADEMODE */
    D3DDDIRS_LINEPATTERN                = 10,
    D3DDDIRS_ZWRITEENABLE               = 14,   /* TRUE to enable z writes */
    D3DDDIRS_ALPHATESTENABLE            = 15,   /* TRUE to enable alpha tests */
    D3DDDIRS_LASTPIXEL                  = 16,   /* TRUE for last-pixel on lines */
    D3DDDIRS_SRCBLEND                   = 19,   /* D3DBLEND */
    D3DDDIRS_DESTBLEND                  = 20,   /* D3DBLEND */
    D3DDDIRS_CULLMODE                   = 22,   /* D3DCULL */
    D3DDDIRS_ZFUNC                      = 23,   /* D3DCMPFUNC */
    D3DDDIRS_ALPHAREF                   = 24,   /* D3DFIXED */
    D3DDDIRS_ALPHAFUNC                  = 25,   /* D3DCMPFUNC */
    D3DDDIRS_DITHERENABLE               = 26,   /* TRUE to enable dithering */
    D3DDDIRS_ALPHABLENDENABLE           = 27,   /* TRUE to enable alpha blending */
    D3DDDIRS_FOGENABLE                  = 28,   /* TRUE to enable fog blending */
    D3DDDIRS_SPECULARENABLE             = 29,   /* TRUE to enable specular */
    D3DDDIRS_ZVISIBLE                   = 30,
    D3DDDIRS_FOGCOLOR                   = 34,   /* D3DCOLOR */
    D3DDDIRS_FOGTABLEMODE               = 35,   /* D3DFOGMODE */
    D3DDDIRS_FOGSTART                   = 36,   /* Fog start (for both vertex and pixel fog) */
    D3DDDIRS_FOGEND                     = 37,   /* Fog end      */
    D3DDDIRS_FOGDENSITY                 = 38,   /* Fog density  */
    D3DDDIRS_EDGEANTIALIAS              = 40,
    D3DDDIRS_COLORKEYENABLE             = 41,
    D3DDDIRS_OLDALPHABLENDENABLE        = 42,
    D3DDDIRS_ZBIAS                      = 47,
    D3DDDIRS_RANGEFOGENABLE             = 48,   /* Enables range-based fog */
    D3DDDIRS_TRANSLUCENTSORTINDEPENDENT = 51,
    D3DDDIRS_STENCILENABLE              = 52,   /* BOOL enable/disable stenciling */
    D3DDDIRS_STENCILFAIL                = 53,   /* D3DSTENCILOP to do if stencil test fails */
    D3DDDIRS_STENCILZFAIL               = 54,   /* D3DSTENCILOP to do if stencil test passes and Z test fails */
    D3DDDIRS_STENCILPASS                = 55,   /* D3DSTENCILOP to do if both stencil and Z tests pass */
    D3DDDIRS_STENCILFUNC                = 56,   /* D3DCMPFUNC fn.  Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
    D3DDDIRS_STENCILREF                 = 57,   /* Reference value used in stencil test */
    D3DDDIRS_STENCILMASK                = 58,   /* Mask value used in stencil test */
    D3DDDIRS_STENCILWRITEMASK           = 59,   /* Write mask applied to values written to stencil buffer */
    D3DDDIRS_TEXTUREFACTOR              = 60,   /* D3DCOLOR used for multi-texture blend */
    D3DDDIRS_SCENECAPTURE               = 62,   // DDI only to replace SceneCapture
    D3DDDIRS_STIPPLEPATTERN00           = 64,
    D3DDDIRS_STIPPLEPATTERN01           = 65,
    D3DDDIRS_STIPPLEPATTERN02           = 66,
    D3DDDIRS_STIPPLEPATTERN03           = 67,
    D3DDDIRS_STIPPLEPATTERN04           = 68,
    D3DDDIRS_STIPPLEPATTERN05           = 69,
    D3DDDIRS_STIPPLEPATTERN06           = 70,
    D3DDDIRS_STIPPLEPATTERN07           = 71,
    D3DDDIRS_STIPPLEPATTERN08           = 72,
    D3DDDIRS_STIPPLEPATTERN09           = 73,
    D3DDDIRS_STIPPLEPATTERN10           = 74,
    D3DDDIRS_STIPPLEPATTERN11           = 75,
    D3DDDIRS_STIPPLEPATTERN12           = 76,
    D3DDDIRS_STIPPLEPATTERN13           = 77,
    D3DDDIRS_STIPPLEPATTERN14           = 78,
    D3DDDIRS_STIPPLEPATTERN15           = 79,
    D3DDDIRS_STIPPLEPATTERN16           = 80,
    D3DDDIRS_STIPPLEPATTERN17           = 81,
    D3DDDIRS_STIPPLEPATTERN18           = 82,
    D3DDDIRS_STIPPLEPATTERN19           = 83,
    D3DDDIRS_STIPPLEPATTERN20           = 84,
    D3DDDIRS_STIPPLEPATTERN21           = 85,
    D3DDDIRS_STIPPLEPATTERN22           = 86,
    D3DDDIRS_STIPPLEPATTERN23           = 87,
    D3DDDIRS_STIPPLEPATTERN24           = 88,
    D3DDDIRS_STIPPLEPATTERN25           = 89,
    D3DDDIRS_STIPPLEPATTERN26           = 90,
    D3DDDIRS_STIPPLEPATTERN27           = 91,
    D3DDDIRS_STIPPLEPATTERN28           = 92,
    D3DDDIRS_STIPPLEPATTERN29           = 93,
    D3DDDIRS_STIPPLEPATTERN30           = 94,
    D3DDDIRS_STIPPLEPATTERN31           = 95,
    D3DDDIRS_WRAP0                      = 128,  /* wrap for 1st texture coord. set */
    D3DDDIRS_WRAP1                      = 129,  /* wrap for 2nd texture coord. set */
    D3DDDIRS_WRAP2                      = 130,  /* wrap for 3rd texture coord. set */
    D3DDDIRS_WRAP3                      = 131,  /* wrap for 4th texture coord. set */
    D3DDDIRS_WRAP4                      = 132,  /* wrap for 5th texture coord. set */
    D3DDDIRS_WRAP5                      = 133,  /* wrap for 6th texture coord. set */
    D3DDDIRS_WRAP6                      = 134,  /* wrap for 7th texture coord. set */
    D3DDDIRS_WRAP7                      = 135,  /* wrap for 8th texture coord. set */
    D3DDDIRS_CLIPPING                   = 136,
    D3DDDIRS_LIGHTING                   = 137,
    D3DDDIRS_AMBIENT                    = 139,
    D3DDDIRS_FOGVERTEXMODE              = 140,
    D3DDDIRS_COLORVERTEX                = 141,
    D3DDDIRS_LOCALVIEWER                = 142,
    D3DDDIRS_NORMALIZENORMALS           = 143,
    D3DDDIRS_COLORKEYBLENDENABLE        = 144,
    D3DDDIRS_DIFFUSEMATERIALSOURCE      = 145,
    D3DDDIRS_SPECULARMATERIALSOURCE     = 146,
    D3DDDIRS_AMBIENTMATERIALSOURCE      = 147,
    D3DDDIRS_EMISSIVEMATERIALSOURCE     = 148,
    D3DDDIRS_VERTEXBLEND                = 151,
    D3DDDIRS_CLIPPLANEENABLE            = 152,
    D3DDDIRS_SOFTWAREVERTEXPROCESSING   = 153,
    D3DDDIRS_POINTSIZE                  = 154,   /* float point size */
    D3DDDIRS_POINTSIZE_MIN              = 155,   /* float point size min threshold */
    D3DDDIRS_POINTSPRITEENABLE          = 156,   /* BOOL point texture coord control */
    D3DDDIRS_POINTSCALEENABLE           = 157,   /* BOOL point size scale enable */
    D3DDDIRS_POINTSCALE_A               = 158,   /* float point attenuation A value */
    D3DDDIRS_POINTSCALE_B               = 159,   /* float point attenuation B value */
    D3DDDIRS_POINTSCALE_C               = 160,   /* float point attenuation C value */
    D3DDDIRS_MULTISAMPLEANTIALIAS       = 161,  // BOOL - set to do FSAA with multisample buffer
    D3DDDIRS_MULTISAMPLEMASK            = 162,  // DWORD - per-sample enable/disable
    D3DDDIRS_PATCHEDGESTYLE             = 163,  // Sets whether patch edges will use float style tessellation
    D3DDDIRS_PATCHSEGMENTS              = 164,
    D3DDDIRS_DEBUGMONITORTOKEN          = 165,  // DEBUG ONLY - token to debug monitor
    D3DDDIRS_POINTSIZE_MAX              = 166,   /* float point size max threshold */
    D3DDDIRS_INDEXEDVERTEXBLENDENABLE   = 167,
    D3DDDIRS_COLORWRITEENABLE           = 168,  // per-channel write enable
    D3DDDIRS_DELETERTPATCH              = 169,
    D3DDDIRS_TWEENFACTOR                = 170,   // float tween factor
    D3DDDIRS_BLENDOP                    = 171,   // D3DBLENDOP setting
    D3DDDIRS_POSITIONDEGREE             = 172,   // NPatch position interpolation degree. D3DDEGREE_LINEAR or D3DDEGREE_CUBIC (default)
    D3DDDIRS_NORMALDEGREE               = 173,   // NPatch normal interpolation degree. D3DDEGREE_LINEAR (default) or D3DDEGREE_QUADRATIC
    D3DDDIRS_SCISSORTESTENABLE          = 174,
    D3DDDIRS_SLOPESCALEDEPTHBIAS        = 175,
    D3DDDIRS_ANTIALIASEDLINEENABLE      = 176,
    D3DDDIRS_MINTESSELLATIONLEVEL       = 178,
    D3DDDIRS_MAXTESSELLATIONLEVEL       = 179,
    D3DDDIRS_ADAPTIVETESS_X             = 180,
    D3DDDIRS_ADAPTIVETESS_Y             = 181,
    D3DDDIRS_ADAPTIVETESS_Z             = 182,
    D3DDDIRS_ADAPTIVETESS_W             = 183,
    D3DDDIRS_ENABLEADAPTIVETESSELLATION = 184,
    D3DDDIRS_TWOSIDEDSTENCILMODE        = 185,   /* BOOL enable/disable 2 sided stenciling */
    D3DDDIRS_CCW_STENCILFAIL            = 186,   /* D3DSTENCILOP to do if ccw stencil test fails */
    D3DDDIRS_CCW_STENCILZFAIL           = 187,   /* D3DSTENCILOP to do if ccw stencil test passes and Z test fails */
    D3DDDIRS_CCW_STENCILPASS            = 188,   /* D3DSTENCILOP to do if both ccw stencil and Z tests pass */
    D3DDDIRS_CCW_STENCILFUNC            = 189,   /* D3DCMPFUNC fn.  ccw Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
    D3DDDIRS_COLORWRITEENABLE1          = 190,   /* Additional ColorWriteEnables for the devices that support D3DPMISCCAPS_INDEPENDENTWRITEMASKS */
    D3DDDIRS_COLORWRITEENABLE2          = 191,   /* Additional ColorWriteEnables for the devices that support D3DPMISCCAPS_INDEPENDENTWRITEMASKS */
    D3DDDIRS_COLORWRITEENABLE3          = 192,   /* Additional ColorWriteEnables for the devices that support D3DPMISCCAPS_INDEPENDENTWRITEMASKS */
    D3DDDIRS_BLENDFACTOR                = 193,   /* D3DCOLOR used for a constant blend factor during alpha blending for devices that support D3DPBLENDCAPS_BLENDFACTOR */
    D3DDDIRS_SRGBWRITEENABLE            = 194,   /* Enable rendertarget writes to be DE-linearized to SRGB (for formats that expose D3DUSAGE_QUERY_SRGBWRITE) */
    D3DDDIRS_DEPTHBIAS                  = 195,
    D3DDDIRS_WRAP8                      = 198,   /* Additional wrap states for vs_3_0+ attributes with D3DDECLUSAGE_TEXCOORD */
    D3DDDIRS_WRAP9                      = 199,
    D3DDDIRS_WRAP10                     = 200,
    D3DDDIRS_WRAP11                     = 201,
    D3DDDIRS_WRAP12                     = 202,
    D3DDDIRS_WRAP13                     = 203,
    D3DDDIRS_WRAP14                     = 204,
    D3DDDIRS_WRAP15                     = 205,
    D3DDDIRS_SEPARATEALPHABLENDENABLE   = 206,  /* TRUE to enable a separate blending function for the alpha channel */
    D3DDDIRS_SRCBLENDALPHA              = 207,  /* SRC blend factor for the alpha channel when D3DDDIRS_SEPARATEDESTALPHAENABLE is TRUE */
    D3DDDIRS_DESTBLENDALPHA             = 208,  /* DST blend factor for the alpha channel when D3DDDIRS_SEPARATEDESTALPHAENABLE is TRUE */
    D3DDDIRS_BLENDOPALPHA               = 209,  /* Blending operation for the alpha channel when D3DDDIRS_SEPARATEDESTALPHAENABLE is TRUE */

    D3DDDIRS_FORCE_DWORD                = 0x7fffffff, /* force 32-bit size enum */
} D3DDDIRENDERSTATETYPE;

typedef struct _D3DDDIARG_RENDERSTATE
{
    D3DDDIRENDERSTATETYPE  State;
    UINT                   Value;
} D3DDDIARG_RENDERSTATE;

typedef struct _D3DDDIARG_WINFO
{
    FLOAT               WNear;
    FLOAT               WFar;
} D3DDDIARG_WINFO;

typedef struct _D3DDDIARG_VALIDATETEXTURESTAGESTATE
{
    UINT                NumPasses;      // out: Number of passes the hardware
                                        //      can perform the operation in
} D3DDDIARG_VALIDATETEXTURESTAGESTATE;

typedef enum _D3DDDITEXTURESTAGESTATETYPE
{
    D3DDDITSS_TEXTUREMAP             =  0,
    D3DDDITSS_COLOROP                =  1, /* D3DTEXTUREOP - per-stage blending controls for color channels */
    D3DDDITSS_COLORARG1              =  2, /* D3DTA_* (texture arg) */
    D3DDDITSS_COLORARG2              =  3, /* D3DTA_* (texture arg) */
    D3DDDITSS_ALPHAOP                =  4, /* D3DTEXTUREOP - per-stage blending controls for alpha channel */
    D3DDDITSS_ALPHAARG1              =  5, /* D3DTA_* (texture arg) */
    D3DDDITSS_ALPHAARG2              =  6, /* D3DTA_* (texture arg) */
    D3DDDITSS_BUMPENVMAT00           =  7, /* float (bump mapping matrix) */
    D3DDDITSS_BUMPENVMAT01           =  8, /* float (bump mapping matrix) */
    D3DDDITSS_BUMPENVMAT10           =  9, /* float (bump mapping matrix) */
    D3DDDITSS_BUMPENVMAT11           = 10, /* float (bump mapping matrix) */
    D3DDDITSS_TEXCOORDINDEX          = 11, /* identifies which set of texture coordinates index this texture */
    D3DDDITSS_ADDRESSU               = 13,
    D3DDDITSS_ADDRESSV               = 14,
    D3DDDITSS_BORDERCOLOR            = 15,
    D3DDDITSS_MAGFILTER              = 16,
    D3DDDITSS_MINFILTER              = 17,
    D3DDDITSS_MIPFILTER              = 18,
    D3DDDITSS_MIPMAPLODBIAS          = 19,
    D3DDDITSS_MAXMIPLEVEL            = 20,
    D3DDDITSS_MAXANISOTROPY          = 21,
    D3DDDITSS_BUMPENVLSCALE          = 22, /* float scale for bump map luminance */
    D3DDDITSS_BUMPENVLOFFSET         = 23, /* float offset for bump map luminance */
    D3DDDITSS_TEXTURETRANSFORMFLAGS  = 24, /* D3DTEXTURETRANSFORMFLAGS controls texture transform */
    D3DDDITSS_ADDRESSW               = 25,
    D3DDDITSS_COLORARG0              = 26, /* D3DTA_* third arg for triadic ops */
    D3DDDITSS_ALPHAARG0              = 27, /* D3DTA_* third arg for triadic ops */
    D3DDDITSS_RESULTARG              = 28, /* D3DTA_* arg for result (CURRENT or TEMP) */
    D3DDDITSS_SRGBTEXTURE            = 29,
    D3DDDITSS_ELEMENTINDEX           = 30,
    D3DDDITSS_DMAPOFFSET             = 31,
    D3DDDITSS_CONSTANT               = 32, /* Per-stage constant D3DTA_CONSTANT */
    D3DDDITSS_DISABLETEXTURECOLORKEY = 33,
    D3DDDITSS_TEXTURECOLORKEYVAL     = 34,

    D3DDDITSS_FORCE_DWORD            = 0x7fffffff, /* force 32-bit size enum */
} D3DDDITEXTURESTAGESTATETYPE;

typedef struct _D3DDDIARG_TEXTURESTAGESTATE
{
    UINT                        Stage;
    D3DDDITEXTURESTAGESTATETYPE State;
    UINT                        Value;
} D3DDDIARG_TEXTURESTAGESTATE;

typedef struct _D3DDDIARG_SETPIXELSHADERCONST
{
    UINT    Register;   // Const register to start copying
    UINT    Count;      // Number of 4-float vectors to copy for D3DDP2OP_SETPIXELSHADERCONST
                        // Number of 4-integer vectors to copy for D3DDP2OP_SETPIXELSHADERCONSTI
                        // Number of BOOL values to copy for D3DDP2OP_SETPIXELSHADERCONSTB
                        // Data follows
} D3DDDIARG_SETPIXELSHADERCONST;

typedef D3DDDIARG_SETPIXELSHADERCONST D3DDDIARG_SETPIXELSHADERCONSTI;
typedef D3DDDIARG_SETPIXELSHADERCONST D3DDDIARG_SETPIXELSHADERCONSTB;

typedef struct _D3DDDIARG_SETSTREAMSOURCEUM
{
    UINT    Stream;     // Stream index, starting from zero
    UINT    Stride;     // Vertex size in bytes
} D3DDDIARG_SETSTREAMSOURCEUM;

typedef struct _D3DDDIARG_SETINDICES
{
    HANDLE  hIndexBuffer;   // Index buffer handle
    UINT    Stride;         // Index size in bytes (2 or 4)
} D3DDDIARG_SETINDICES;

typedef struct _D3DDDIARG_DRAWPRIMITIVE
{
    D3DPRIMITIVETYPE PrimitiveType;
    UINT    VStart;
    UINT    PrimitiveCount;
} D3DDDIARG_DRAWPRIMITIVE;

typedef struct _D3DDDIARG_DRAWINDEXEDPRIMITIVE
{
    D3DPRIMITIVETYPE PrimitiveType;
    INT     BaseVertexIndex;    // Vertex which corresponds to index 0
    UINT    MinIndex;           // Min vertex index in the vertex buffer
    UINT    NumVertices;        // Number of vertices starting from MinIndex
    UINT    StartIndex;         // Start index in the index buffer
    UINT    PrimitiveCount;
} D3DDDIARG_DRAWINDEXEDPRIMITIVE;

typedef struct _D3DDDIARG_DRAWRECTPATCH
{
    UINT    Handle;
} D3DDDIARG_DRAWRECTPATCH;

typedef struct _D3DDDIARG_DRAWTRIPATCH
{
    UINT    Handle;
} D3DDDIARG_DRAWTRIPATCH;

typedef struct _D3DDDIARG_DRAWPRIMITIVE2
{
    D3DPRIMITIVETYPE PrimitiveType;
    UINT    FirstVertexOffset;    // Offset in bytes in the stream 0
    UINT    PrimitiveCount;
} D3DDDIARG_DRAWPRIMITIVE2;

typedef struct _D3DDDIARG_DRAWINDEXEDPRIMITIVE2
{
    D3DPRIMITIVETYPE PrimitiveType;
    INT   BaseVertexOffset;     // Stream 0 offset of the vertex which
                                // corresponds to index 0. This offset could be
                                // negative, but when an index is added to the
                                // offset the result is positive
    UINT  MinIndex;             // Min vertex index in the vertex buffer
    UINT  NumVertices;          // Number of vertices starting from MinIndex
    UINT  StartIndexOffset;     // Offset of the start index in the index buffer
    UINT  PrimitiveCount;       // Number of triangles (points, lines)
} D3DDDIARG_DRAWINDEXEDPRIMITIVE2;

typedef struct _D3DDDIARG_VOLUMEBLT
{
    HANDLE      hDstResource;   // dest resource
    HANDLE      hSrcResource;   // src resource
    UINT        DstX;           // dest X (width)
    UINT        DstY;           // dest Y (height)
    UINT        DstZ;           // dest Z (depth)
    D3DDDIBOX   SrcBox;         // src box
} D3DDDIARG_VOLUMEBLT;

typedef struct _D3DDDIARG_BUFFERBLT
{
    HANDLE      hDstResource;   // dest resource
    HANDLE      hSrcResource;   // src resource
    UINT        Offset;         // Offset in the dest surface (in BYTES)
    D3DDDIRANGE SrcRange;       // src range
} D3DDDIARG_BUFFERBLT;

typedef struct _D3DDDIARG_TEXBLT
{
    HANDLE      hDstResource;   // dest resource
    HANDLE      hSrcResource;   // src resource
    UINT        CubeMapFace;
    POINT       DstPoint;
    RECT        SrcRect;        // src rect
} D3DDDIARG_TEXBLT;

typedef struct _D3DDDIARG_STATESET
{
    UINT                Operation;  // in: D3DHAL_STATESET* enum
                                    // D3DHAL_STATESETDELETE and D3DHAL_STATESETCAPTURE
    D3DSTATEBLOCKTYPE   StateBlockType;     // in: Type use with D3DHAL_STATESETBEGIN/END
    HANDLE              hStateSet;  // out: Device handle returned from D3DHAL_STATESETBEGIN and
                                    //      D3DHAL_STATESETCREATE
                                    // in: State set handle passed with D3DHAL_STATESETEXECUTE,
} D3DDDIARG_STATESET;

typedef struct _D3DDDIARG_SETPRIORITY
{
    HANDLE  hResource;
    UINT    Priority;
} D3DDDIARG_SETPRIORITY;

typedef struct _D3DDDIARG_CLEAR
{
    // Flags can contain D3DCLEAR_TARGET, D3DCLEAR_ZBUFFER, D3DCLEAR_STENCIL and/or D3DCLEAR_COMPUTERECTS
    UINT                Flags;          // in:  surfaces to clear
    UINT                FillColor;      // in:  Color value for rtarget
    FLOAT               FillDepth;      // in:  Depth value for Z buffer (0.0-1.0)
    UINT                FillStencil;    // in:  value used to clear stencil buffer
} D3DDDIARG_CLEAR;

typedef struct _D3DDDIARG_UPDATEPALETTE
{
    UINT    PaletteHandle;
    UINT    StartIndex;
    UINT    NumEntries;
} D3DDDIARG_UPDATEPALETTE;

#define D3DDDISETPALETTE_256        0x00000001l
#define D3DDDISETPALETTE_ALLOW256   0x00000200l
#define D3DDDISETPALETTE_ALPHA      0x00002000l

typedef struct _D3DDDIARG_SETPALETTE
{
    UINT    PaletteHandle;
    UINT    PaletteFlags;
    HANDLE  hResource;
} D3DDDIARG_SETPALETTE;

// Used with all types of vertex shader constants
typedef struct _D3DDDIARG_SETVERTEXSHADERCONST
{
    UINT    Register;   // Const register to start copying
    UINT    Count;      // Number of 4-float vectors to copy for D3DDP2OP_SETVERTEXSHADERCONST
                        // Number of 4-integer vectors to copy for D3DDP2OP_SETVERTEXSHADERCONSTI
                        // Number of BOOL values to copy for D3DDP2OP_SETVERTEXSHADERCONSTB
                        // Data follows
} D3DDDIARG_SETVERTEXSHADERCONST;

typedef D3DDDIARG_SETVERTEXSHADERCONST D3DDDIARG_SETVERTEXSHADERCONSTI;
typedef D3DDDIARG_SETVERTEXSHADERCONST D3DDDIARG_SETVERTEXSHADERCONSTB;

typedef struct _D3DDDIARG_MULTIPLYTRANSFORM
{
    D3DTRANSFORMSTATETYPE   TransformType;
    D3DMATRIX               Matrix;
} D3DDDIARG_MULTIPLYTRANSFORM;

typedef struct _D3DDDIARG_SETTRANSFORM
{
    D3DTRANSFORMSTATETYPE   TransformType;
    D3DMATRIX               Matrix;
} D3DDDIARG_SETTRANSFORM;

typedef struct _D3DDDIARG_VIEWPORTINFO
{
    UINT    X;
    UINT    Y;
    UINT    Width;
    UINT    Height;
} D3DDDIARG_VIEWPORTINFO;

typedef struct _D3DDDIARG_ZRANGE
{
    FLOAT       MinZ;
    FLOAT       MaxZ;
} D3DDDIARG_ZRANGE;

typedef struct _D3DDDIARG_SETMATERIAL
{
    D3DCOLORVALUE   Diffuse;        /* Diffuse color RGBA */
    D3DCOLORVALUE   Ambient;        /* Ambient color RGB */
    D3DCOLORVALUE   Specular;       /* Specular 'shininess' */
    D3DCOLORVALUE   Emissive;       /* Emissive color RGB */
    FLOAT           Power;          /* Sharpness if specular highlight */
} D3DDDIARG_SETMATERIAL;

typedef enum _D3DDDI_SETLIGHT_TYPE
{
    // Values for DataType in D3DDDIARG_SETLIGHT
    D3DDDI_SETLIGHT_ENABLE  =   0,
    D3DDDI_SETLIGHT_DISABLE =   1,
    // If this is set, light data will be passed in after the
    // D3DLIGHT7 structure
    D3DDDI_SETLIGHT_DATA    =   2,
} D3DDDI_SETLIGHT_TYPE;

typedef struct _D3DDDIARG_SETLIGHT
{
    UINT                    Index;
    D3DDDI_SETLIGHT_TYPE    DataType;
} D3DDDIARG_SETLIGHT;

typedef struct _D3DDDI_LIGHT
{
    D3DLIGHTTYPE    Type;            /* Type of light source */
    D3DCOLORVALUE   Diffuse;         /* Diffuse color of light */
    D3DCOLORVALUE   Specular;        /* Specular color of light */
    D3DCOLORVALUE   Ambient;         /* Ambient color of light */
    D3DVECTOR       Position;         /* Position in world space */
    D3DVECTOR       Direction;        /* Direction in world space */
    FLOAT           Range;            /* Cutoff range */
    FLOAT           Falloff;          /* Falloff */
    FLOAT           Attenuation0;     /* Constant attenuation */
    FLOAT           Attenuation1;     /* Linear attenuation */
    FLOAT           Attenuation2;     /* Quadratic attenuation */
    FLOAT           Theta;            /* Inner angle of spotlight cone */
    FLOAT           Phi;              /* Outer angle of spotlight cone */
} D3DDDI_LIGHT;

typedef struct _D3DDDIARG_CREATELIGHT
{
    UINT        Index;
} D3DDDIARG_CREATELIGHT;

typedef struct _D3DDDIARG_DESTROYLIGHT
{
    UINT        Index;
} D3DDDIARG_DESTROYLIGHT;

typedef struct _D3DDDIARG_SETCLIPPLANE
{
    UINT        Index;
    FLOAT       Plane[4];
} D3DDDIARG_SETCLIPPLANE;

typedef struct _D3DDDI_LOCKFLAGS
{
    union
    {
        struct
        {
            UINT    ReadOnly        : 1;        // 0x00000001
            UINT    WriteOnly       : 1;        // 0x00000002
            UINT    NoOverwrite     : 1;        // 0x00000004
            UINT    Discard         : 1;        // 0x00000008
            UINT    RangeValid      : 1;        // 0x00000010
            UINT    AreaValid       : 1;        // 0x00000020
            UINT    BoxValid        : 1;        // 0x00000040
            UINT    NotifyOnly      : 1;        // 0x00000080
            UINT    MightDrawFromLocked : 1;    // 0x00000100
            UINT    DoNotWait       : 1;        // 0x00000200
            UINT    Reserved        :22;        // 0xFFFFFC00
        };
        UINT Value;
    };
} D3DDDI_LOCKFLAGS;

typedef struct _D3DDDIARG_LOCK
{
    HANDLE              hResource;          // in: resource to lock, used by Lock to driver
    UINT                SubResourceIndex;   // in: zero based subresource index
    union
    {
        D3DDDIRANGE     Range;              // in: range being locked
        RECT            Area;               // in: area being locked
        D3DDDIBOX       Box;                // in: volume being locked
    };
    VOID*               pSurfData;          // out: pointer to memory
    UINT                Pitch;              // out: pitch of memory
    UINT                SlicePitch;         // out: slice pitch of memory
    D3DDDI_LOCKFLAGS    Flags;              // in: flags
} D3DDDIARG_LOCK;

typedef struct _D3DDDI_UNLOCKFLAGS
{
    union
    {
        struct
        {
            UINT    NotifyOnly      : 1;        // 0x00000001
            UINT    Reserved        :31;        // 0xFFFFFFFE
        };
        UINT Value;
    };
} D3DDDI_UNLOCKFLAGS;

typedef struct _D3DDDIARG_UNLOCK
{
    HANDLE              hResource;          // in: resource to lock, used by Lock to driver
    UINT                SubResourceIndex;   // in: zero based subresource index
    D3DDDI_UNLOCKFLAGS  Flags;              // in: flags
} D3DDDIARG_UNLOCK;

typedef struct _D3DDDI_OPENRESOURCEFLAGS
{
    union
    {
        struct
        {
            UINT    Fullscreen      : 1;        // 0x00000001
            UINT    AlphaOverride   : 1;        // 0x00000002
            UINT    Reserved        :30;        // 0xFFFFFFFC
        };
        UINT Value;
    };
} D3DDDI_OPENRESOURCEFLAGS;

typedef struct _D3DDDI_LOCKASYNCFLAGS
{
    union
    {
        struct
        {
            UINT    NoOverwrite          : 1;           // 0x00000001
            UINT    Discard              : 1;           // 0x00000002
            UINT    RangeValid           : 1;           // 0x00000004
            UINT    AreaValid            : 1;           // 0x00000008
            UINT    BoxValid             : 1;           // 0x00000010
            UINT    NoExistingReferences : 1;           // 0x00000020
            UINT    NotifyOnly           : 1;           // 0x00000040
            UINT    Reserved             : 25;          // 0xFFFFFF80
        };
        UINT Value;
    };
} D3DDDI_LOCKASYNCFLAGS;

typedef struct _D3DDDIARG_LOCKASYNC
{
    HANDLE                hResource;        // in: resource to lock
    UINT                  SubResourceIndex; // in: zero based subresource index
    D3DDDI_LOCKASYNCFLAGS Flags;            // in: lock flags
    union
    {
        D3DDDIRANGE     Range;              // in: range being locked
        RECT            Area;               // in: area being locked
        D3DDDIBOX       Box;                // in: volume being locked
    };
    HANDLE                hCookie;          // out: A handle representing the renamed surface (only valid if Discard flag is set)
    VOID*                 pSurfData;        // out: pointer to surface memory
    UINT                  Pitch;            // out: pitch of memory
    UINT                  SlicePitch;       // out: slice pitch of memory
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN7)
    D3DGPU_VIRTUAL_ADDRESS GpuVirtualAddress; // out: GPU Virtual address of the allocation locked. _ADVSCH_
#endif // D3D_UMD_INTERFACE_VERSION
} D3DDDIARG_LOCKASYNC;

typedef struct _D3DDDI_UNLOCKASYNCFLAGS
{
    union
    {
        struct
        {
            UINT    NotifyOnly      : 1;        // 0x00000001
            UINT    Reserved        :31;        // 0xFFFFFFFE
        };
        UINT Value;
    };
} D3DDDI_UNLOCKASYNCFLAGS;

typedef struct _D3DDDIARG_UNLOCKASYNC
{
    HANDLE                  hResource;         // in: resource to unlock
    UINT                    SubResourceIndex;  // in: zero based subresource index
    D3DDDI_UNLOCKASYNCFLAGS Flags;             // in: lock flags
} D3DDDIARG_UNLOCKASYNC;

typedef struct _D3DDDIARG_RENAME
{
    HANDLE        hResource;        // in: resource
    UINT          SubResourceIndex; // in: zero based subresource index
    HANDLE        hCookie;          // in: handle returned by LockAsync
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN7)
    D3DGPU_VIRTUAL_ADDRESS GpuVirtualAddress; // in: GPU Virtual address of the allocation renamed. _ADVSCH_
#endif // D3D_UMD_INTERFACE_VERSION
} D3DDDIARG_RENAME;

typedef struct _D3DDDIARG_OPENRESOURCE
{
    UINT                        NumAllocations;             // in : Number of open allocation structs
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN7)
    union {
        D3DDDI_OPENALLOCATIONINFO*  pOpenAllocationInfo;    // in : Array of open allocation structs : WDDM v1
        D3DDDI_OPENALLOCATIONINFO2* pOpenAllocationInfo2;   // in : Array of open allocation structs : WDDM v2 _ADVSCH_
    };
#else
    D3DDDI_OPENALLOCATIONINFO*  pOpenAllocationInfo;        // in : Array of open allocation structs
#endif // D3D_UMD_INTERFACE_VERSION
    D3DKMT_HANDLE               hKMResource;                // in : Kernel resource handle
    VOID*                       pPrivateDriverData;         // in : Ptr to per reosurce PrivateDriverData
    UINT                        PrivateDriverDataSize;      // in : Size of resource pPrivateDriverData
    HANDLE                      hResource;                  // in/out : D3D runtime handle / UM driver handle
    D3DDDI_ROTATION             Rotation;                   // in: The orientation of the resource. (0, 90, 180, 270)
    D3DDDI_OPENRESOURCEFLAGS    Flags;                      // in: Flags
}D3DDDIARG_OPENRESOURCE;

typedef struct _D3DDDIARG_CREATEVERTEXSHADERDECL
{
    UINT    NumVertexElements;              // in:  Number of vertex elements
    HANDLE  ShaderHandle;                   // out: Shader function handle
} D3DDDIARG_CREATEVERTEXSHADERDECL;

typedef struct _D3DDDIARG_CREATEVERTEXSHADERFUNC
{
    UINT    Size;           // in:  Shader function size in bytes
    HANDLE  ShaderHandle;   // out: Shader function handle
} D3DDDIARG_CREATEVERTEXSHADERFUNC;

typedef struct _D3DDDIARG_SETSTREAMSOURCE
{
    UINT    Stream;         // Stream index, starting from zero
    HANDLE  hVertexBuffer;  // Vertex buffer handle
    UINT    Offset;         // Offset of the first vertex size in bytes
    UINT    Stride;         // Vertex size in bytes
} D3DDDIARG_SETSTREAMSOURCE;

typedef struct _D3DDDIARG_SETSTREAMSOURCEFREQ
{
    UINT    Stream;         // Stream index, starting from zero
    UINT    Divider;        // Stream source divider
} D3DDDIARG_SETSTREAMSOURCEFREQ;

typedef struct _D3DDDIARG_SETCONVOLUTIONKERNELMONO
{
    UINT    Width;          // Kernel width
    UINT    Height;         // Kernel height
    FLOAT*  pKernelRow;     // Row weights
    FLOAT*  pKernelCol;     // Column weights
} D3DDDIARG_SETCONVOLUTIONKERNELMONO;

typedef enum _D3DDDI_COMPOSERECTSOP
{
    D3DDDICOMPOSERECTS_COPY        = 1,
    D3DDDICOMPOSERECTS_OR          = 2,
    D3DDDICOMPOSERECTS_AND         = 3,
    D3DDDICOMPOSERECTS_NEG         = 4,
    D3DDDICOMPOSERECTS_FORCE_UINT  =0x7fffffff
} D3DDDI_COMPOSERECTSOP;

typedef struct _D3DDDIARG_COMPOSERECTS
{
    HANDLE                hSrcResource;
    UINT                  SrcSubResourceIndex;
    HANDLE                hDstResource;
    UINT                  DstSubResourceIndex;
    HANDLE                hSrcRectDescsVB;
    UINT                  NumRects;
    HANDLE                hDstRectDescsVB;
    D3DDDI_COMPOSERECTSOP Operation;
    INT                   XOffset;
    INT                   YOffset;
} D3DDDIARG_COMPOSERECTS;

typedef struct _D3DDDI_BLTFLAGS
{
    union
    {
        struct
        {
            UINT    Point                : 1;// 0x00000001
            UINT    Linear               : 1;// 0x00000002
            UINT    SrcColorKey          : 1;// 0x00000004
            UINT    DstColorKey          : 1;// 0x00000008
            UINT    MirrorLeftRight      : 1;// 0x00000010
            UINT    MirrorUpDown         : 1;// 0x00000020
            UINT    LinearToSrgb         : 1;// 0x00000040
            UINT    Rotate               : 1;// 0x00000080
            UINT    BeginPresentToDwm    : 1;// 0x00000100
            UINT    ContinuePresentToDwm : 1;// 0x00000200
            UINT    EndPresentToDwm      : 1;// 0x00000400
#if (D3D_UMD_INTERFACE_VERSION < D3D_UMD_INTERFACE_VERSION_WIN8)
            UINT    Reserved             :21;// 0xFFFFF800
#else
            UINT    Discard              : 1;// 0x00000800
            UINT    NoOverwrite          : 1;// 0x00001000
            UINT    Tileable             : 1;// 0x00002000
            UINT    Reserved             :18;// 0xFFFFC000
#endif
        };
        UINT        Value;
    };
} D3DDDI_BLTFLAGS;

typedef struct _D3DDDIARG_BLT
{
    HANDLE  hSrcResource;           // Source surface
    UINT    SrcSubResourceIndex;    // Index of surface level
    RECT    SrcRect;                // Source rectangle
    HANDLE  hDstResource;           // Dest surface
    UINT    DstSubResourceIndex;    // Index of surface level
    RECT    DstRect;                // Dest rectangle
    UINT    ColorKey;               // Colorkey value
    D3DDDI_BLTFLAGS Flags;          // Flags
} D3DDDIARG_BLT;

typedef struct _D3DDDI_COLORFILLFLAGS
{
    union
    {
        struct
        {
            UINT    PresentToDwm :  1;// 0x00000001
            UINT    Reserved     : 31;// 0xFFFFFFFE
        };
        UINT        Value;
    };
} D3DDDI_COLORFILLFLAGS;

typedef struct _D3DDDIARG_COLORFILL
{
    HANDLE                hResource;          // Surface getting filled
    UINT                  SubResourceIndex;   // Index of surface level
    RECT                  DstRect;            // Surface dimensions to fill
    D3DCOLOR              Color;              // A8R8G8B8 fill color
    D3DDDI_COLORFILLFLAGS Flags;              // Flags
} D3DDDIARG_COLORFILL;

typedef struct _D3DDDIARG_DEPTHFILL
{
    HANDLE      hResource;          // Surface getting filled
    UINT        SubResourceIndex;   // Index of surface level
    RECT        DstRect;            // Surface dimensions to fill
    UINT        Depth;              // Native depth value
} D3DDDIARG_DEPTHFILL;

typedef enum _D3DDDIQUERYTYPE
{
    D3DDDIQUERYTYPE_VCACHE                   = 4, /* D3DISSUE_END */
    D3DDDIQUERYTYPE_RESOURCEMANAGER          = 5, /* D3DISSUE_END */
    D3DDDIQUERYTYPE_VERTEXSTATS              = 6, /* D3DISSUE_END */
    D3DDDIQUERYTYPE_DDISTATS                 = 7, /* D3DISSUE_END */
    D3DDDIQUERYTYPE_EVENT                    = 8, /* D3DISSUE_END */
    D3DDDIQUERYTYPE_OCCLUSION                = 9, /* D3DISSUE_BEGIN, D3DISSUE_END */
    D3DDDIQUERYTYPE_TIMESTAMP                = 10, /* D3DISSUE_END */
    D3DDDIQUERYTYPE_TIMESTAMPDISJOINT        = 11, /* D3DISSUE_BEGIN, D3DISSUE_END */
    D3DDDIQUERYTYPE_TIMESTAMPFREQ            = 12, /* D3DISSUE_END */
    D3DDDIQUERYTYPE_PIPELINETIMINGS          = 13, /* D3DISSUE_BEGIN, D3DISSUE_END */
    D3DDDIQUERYTYPE_INTERFACETIMINGS         = 14, /* D3DISSUE_BEGIN, D3DISSUE_END */
    D3DDDIQUERYTYPE_VERTEXTIMINGS            = 15, /* D3DISSUE_BEGIN, D3DISSUE_END */
    D3DDDIQUERYTYPE_PIXELTIMINGS             = 16, /* D3DISSUE_BEGIN, D3DISSUE_END */
    D3DDDIQUERYTYPE_BANDWIDTHTIMINGS         = 17, /* D3DISSUE_BEGIN, D3DISSUE_END */
    D3DDDIQUERYTYPE_CACHEUTILIZATION         = 18, /* D3DISSUE_BEGIN, D3DISSUE_END */
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // M1
    D3DDDIQUERYTYPE_COUNTER_DEVICE_DEPENDENT = 0x40000000, /* Start of "device-dependent counters" */
#endif
} D3DDDIQUERYTYPE;

typedef struct _D3DDDIARG_CREATEQUERY
{
    D3DDDIQUERYTYPE QueryType;
    HANDLE          hQuery;
} D3DDDIARG_CREATEQUERY;

typedef struct _D3DDDI_ISSUEQUERYFLAGS
{
    union
    {
        struct
        {
            UINT        Begin    :  1;
            UINT        End      :  1;
            UINT        Reserved : 30;
        };
        UINT Value;
    };
} D3DDDI_ISSUEQUERYFLAGS;

typedef struct _D3DDDIARG_ISSUEQUERY
{
    HANDLE          hQuery;
    D3DDDI_ISSUEQUERYFLAGS Flags;
} D3DDDIARG_ISSUEQUERY;

typedef struct _D3DDDIARG_GETQUERYDATA
{
    HANDLE          hQuery;
    VOID*           pData;
} D3DDDIARG_GETQUERYDATA;

typedef struct _D3DDDIARG_SETRENDERTARGET
{
    UINT        RenderTargetIndex;
    HANDLE      hRenderTarget;
    UINT        SubResourceIndex;
} D3DDDIARG_SETRENDERTARGET;

typedef struct _D3DDDIARG_SETDEPTHSTENCIL
{
    HANDLE      hZBuffer;
} D3DDDIARG_SETDEPTHSTENCIL;

typedef enum _D3DDDITEXTUREFILTERTYPE
{
    D3DDDITEXF_NONE            = 0,    // filtering disabled (valid for mip filter only)
    D3DDDITEXF_POINT           = 1,    // nearest
    D3DDDITEXF_LINEAR          = 2,    // linear interpolation
    D3DDDITEXF_ANISOTROPIC     = 3,    // anisotropic
    D3DDDITEXF_PYRAMIDALQUAD   = 6,    // 4-sample tent
    D3DDDITEXF_GAUSSIANQUAD    = 7,    // 4-sample gaussian
    D3DDDITEXF_FORCE_UINT      = 0x7fffffff,   // force 32-bit size enum
} D3DDDITEXTUREFILTERTYPE;

typedef struct _D3DDDIARG_GENERATEMIPSUBLEVELS
{
    HANDLE                  hResource;
    D3DDDITEXTUREFILTERTYPE Filter;
} D3DDDIARG_GENERATEMIPSUBLEVELS;

typedef struct _D3DDDIVERTEXELEMENT
{
    USHORT  Stream;     // Stream index
    USHORT  Offset;     // Offset in the stream in bytes
    UCHAR   Type;       // Data type
    UCHAR   Method;     // Processing method
    UCHAR   Usage;      // Semantics
    UCHAR   UsageIndex; // Semantic index
} D3DDDIVERTEXELEMENT;

typedef enum _D3DDDIBASISTYPE
{
    D3DDDIBASIS_BEZIER      = 0,
    D3DDDIBASIS_BSPLINE     = 1,
    D3DDDIBASIS_CATMULL_ROM = 2, /* In D3D8 this used to be D3DBASIS_INTERPOLATE */
    D3DDDIBASIS_FORCE_UINT  = 0x7fffffff,
} D3DDDIBASISTYPE;

typedef enum _D3DDDIDEGREETYPE
{
   D3DDDIDEGREE_LINEAR      = 1,
   D3DDDIDEGREE_QUADRATIC   = 2,
   D3DDDIDEGREE_CUBIC       = 3,
   D3DDDIDEGREE_QUINTIC     = 5,
   D3DDDIDEGREE_FORCE_UINT  = 0x7fffffff,
} D3DDDIDEGREETYPE;

typedef struct _D3DDDIRECTPATCH_INFO
{
    UINT                StartVertexOffsetWidth;
    UINT                StartVertexOffsetHeight;
    UINT                Width;
    UINT                Height;
    UINT                Stride;
    D3DDDIBASISTYPE     Basis;
    D3DDDIDEGREETYPE    Degree;
} D3DDDIRECTPATCH_INFO;

typedef struct _D3DDDITRIPATCH_INFO
{
    UINT                StartVertexOffset;
    UINT                NumVertices;
    D3DDDIBASISTYPE     Basis;
    D3DDDIDEGREETYPE    Degree;
} D3DDDITRIPATCH_INFO;

typedef struct _D3DDDIARG_CREATEPIXELSHADER
{
    UINT    CodeSize;       // in:  Shader code size in bytes
    HANDLE  ShaderHandle;   // out: Shader handle
} D3DDDIARG_CREATEPIXELSHADER;

typedef struct _DXVADDI_EXTENDEDFORMAT
{
    union {
        struct {
            UINT SampleFormat : 8;           // See DXVADDI_SAMPLEFORMAT
            UINT VideoChromaSubsampling : 4; // See DXVADDI_VIDEOCHROMASUBSAMPLING
            UINT NominalRange : 3;           // See DXVADDI_NOMINALRANGE
            UINT VideoTransferMatrix : 3;    // See DXVADDI_VIDEOTRANSFERMATRIX
            UINT VideoLighting : 4;          // See DXVADDI_VIDEOLIGHTING
            UINT VideoPrimaries : 5;         // See DXVADDI_VIDEOPRIMARIES
            UINT VideoTransferFunction : 5;  // See DXVADDI_VIDEOTRANSFERFUNCTION
        };
        UINT Value;
    };
} DXVADDI_EXTENDEDFORMAT;

typedef enum _DXVADDI_SAMPLEFORMAT
{
    DXVADDI_SampleFormatMask = 0x00FF,   // 8 bits used for DXVA Sample format
    DXVADDI_SampleUnknown = 0,
    DXVADDI_SampleProgressiveFrame = 2,
    DXVADDI_SampleFieldInterleavedEvenFirst = 3,
    DXVADDI_SampleFieldInterleavedOddFirst = 4,
    DXVADDI_SampleFieldSingleEven = 5,
    DXVADDI_SampleFieldSingleOdd = 6,
    DXVADDI_SampleSubStream = 7
} DXVADDI_SAMPLEFORMAT;

typedef enum _DXVADDI_VIDEOCHROMASUBSAMPLING
{
    DXVADDI_VideoChromaSubsamplingMask = 0x0F,
    DXVADDI_VideoChromaSubsampling_Unknown = 0,
    DXVADDI_VideoChromaSubsampling_ProgressiveChroma = 0x8,
    DXVADDI_VideoChromaSubsampling_Horizontally_Cosited = 0x4,
    DXVADDI_VideoChromaSubsampling_Vertically_Cosited = 0x2,
    DXVADDI_VideoChromaSubsampling_Vertically_AlignedChromaPlanes = 0x1,
    // 4:2:0 variations
    DXVADDI_VideoChromaSubsampling_MPEG2  = DXVADDI_VideoChromaSubsampling_Horizontally_Cosited |
                                            DXVADDI_VideoChromaSubsampling_Vertically_AlignedChromaPlanes,

    DXVADDI_VideoChromaSubsampling_MPEG1  = DXVADDI_VideoChromaSubsampling_Vertically_AlignedChromaPlanes,

    DXVADDI_VideoChromaSubsampling_DV_PAL  =DXVADDI_VideoChromaSubsampling_Horizontally_Cosited |
                                            DXVADDI_VideoChromaSubsampling_Vertically_Cosited,
    // 4:4:4, 4:2:2, 4:1:1
    DXVADDI_VideoChromaSubsampling_Cosited =DXVADDI_VideoChromaSubsampling_Horizontally_Cosited |
                                            DXVADDI_VideoChromaSubsampling_Vertically_Cosited |
                                            DXVADDI_VideoChromaSubsampling_Vertically_AlignedChromaPlanes

} DXVADDI_VIDEOCHROMASUBSAMPLING;

typedef enum _DXVADDI_NOMINALRANGE
{
    DXVADDI_NominalRangeMask = 0x07,
    DXVADDI_NominalRange_Unknown = 0,
    DXVADDI_NominalRange_Normal = 1,
    DXVADDI_NominalRange_Wide = 2,
    // explicit range forms
    DXVADDI_NominalRange_0_255 = 1,
    DXVADDI_NominalRange_16_235 = 2,
    DXVADDI_NominalRange_48_208 = 3
} DXVADDI_NOMINALRANGE;

typedef enum _DXVADDI_VIDEOTRANSFERMATRIX
{
    DXVADDI_VideoTransferMatrixMask = 0x07,
    DXVADDI_VideoTransferMatrix_Unknown = 0,
    DXVADDI_VideoTransferMatrix_BT709 = 1,
    DXVADDI_VideoTransferMatrix_BT601 = 2,
    DXVADDI_VideoTransferMatrix_SMPTE240M = 3
} DXVADDI_VIDEOTRANSFERMATRIX;

typedef enum _DXVADDI_VIDEOLIGHTING
{
    DXVADDI_VideoLightingMask = 0x0F,
    DXVADDI_VideoLighting_Unknown = 0,
    DXVADDI_VideoLighting_bright = 1,
    DXVADDI_VideoLighting_office = 2,
    DXVADDI_VideoLighting_dim = 3,
    DXVADDI_VideoLighting_dark = 4
} DXVADDI_VIDEOLIGHTING;

typedef enum _DXVADDI_VIDEOPRIMARIES
{
    DXVADDI_VideoPrimariesMask = 0x001f,
    DXVADDI_VideoPrimaries_Unknown = 0,
    DXVADDI_VideoPrimaries_reserved = 1,
    DXVADDI_VideoPrimaries_BT709 = 2,
    DXVADDI_VideoPrimaries_BT470_2_SysM = 3,
    DXVADDI_VideoPrimaries_BT470_2_SysBG = 4,
    DXVADDI_VideoPrimaries_SMPTE170M = 5,
    DXVADDI_VideoPrimaries_SMPTE240M = 6,
    DXVADDI_VideoPrimaries_EBU3213 = 7,
    DXVADDI_VideoPrimaries_SMPTE_C = 8
} DXVADDI_VIDEOPRIMARIES;

typedef enum _DXVADDI_VIDEOTRANSFERFUNCTION
{
    DXVADDI_VideoTransFuncMask = 0x001f,
    DXVADDI_VideoTransFunc_Unknown = 0,
    DXVADDI_VideoTransFunc_10 = 1,
    DXVADDI_VideoTransFunc_18 = 2,
    DXVADDI_VideoTransFunc_20 = 3,
    DXVADDI_VideoTransFunc_22 = 4,
    DXVADDI_VideoTransFunc_709  = 5,
    DXVADDI_VideoTransFunc_240M = 6,
    DXVADDI_VideoTransFunc_sRGB = 7,
    DXVADDI_VideoTransFunc_28 = 8
} DXVADDI_VIDEOTRANSFERFUNCTION;

#define DXVADDI_VideoTransFunc_22_709       DXVADDI_VideoTransFunc_709
#define DXVADDI_VideoTransFunc_22_240M      DXVADDI_VideoTransFunc_240M
#define DXVADDI_VideoTransFunc_22_8bit_sRGB DXVADDI_VideoTransFunc_sRGB

typedef struct _DXVADDI_FREQUENCY
{
    UINT              Numerator;
    UINT              Denominator;
} DXVADDI_FREQUENCY;

typedef struct _DXVADDI_VIDEODESC
{
    UINT                   SampleWidth;
    UINT                   SampleHeight;
    DXVADDI_EXTENDEDFORMAT SampleFormat;
    D3DDDIFORMAT           Format;
    DXVADDI_FREQUENCY      InputSampleFreq;
    DXVADDI_FREQUENCY      OutputFrameFreq;
    UINT                   UABProtectionLevel;
    UINT                   Reserved;
} DXVADDI_VIDEODESC;

typedef struct _DXVADDI_CONFIGPICTUREDECODE
{
    GUID        guidConfigBitstreamEncryption;
    GUID        guidConfigMBcontrolEncryption;
    GUID        guidConfigResidDiffEncryption;
    UINT        ConfigBitstreamRaw;
    UINT        ConfigMBcontrolRasterOrder;
    UINT        ConfigResidDiffHost;
    UINT        ConfigSpatialResid8;
    UINT        ConfigResid8Subtraction;
    UINT        ConfigSpatialHost8or9Clipping;
    UINT        ConfigSpatialResidInterleaved;
    UINT        ConfigIntraResidUnsigned;
    UINT        ConfigResidDiffAccelerator;
    UINT        ConfigHostInverseScan;
    UINT        ConfigSpecificIDCT;
    UINT        Config4GroupedCoefs;
    USHORT      ConfigMinRenderTargetBuffCount;
    USHORT      ConfigDecoderSpecific;
} DXVADDI_CONFIGPICTUREDECODE;

typedef struct _DXVADDI_PRIVATEDATA
{
    VOID*             pData;
    UINT              DataSize;
} DXVADDI_PRIVATEDATA;

typedef struct _D3DDDIARG_CREATEDECODEDEVICE
{
    CONST GUID*                  pGuid;
    DXVADDI_VIDEODESC            VideoDesc;
    DXVADDI_CONFIGPICTUREDECODE* pConfig;
    HANDLE                       hDecode;
} D3DDDIARG_CREATEDECODEDEVICE;

typedef struct _D3DDDIARG_SETDECODERENDERTARGET
{
    HANDLE      hDecode;
    HANDLE      hRenderTarget;
    UINT        SubResourceIndex;
} D3DDDIARG_SETDECODERENDERTARGET;

typedef struct DECLSPEC_ALIGN(16) _DXVADDI_PVP_BLOCK128
{
    BYTE        Data[16];
} DXVADDI_PVP_BLOCK128, DXVADDI_PVP_KEY128;

typedef struct _DXVADDI_PVP_SETKEY
{
    DXVADDI_PVP_KEY128  ContentKey;
} DXVADDI_PVP_SETKEY;

typedef struct _D3DDDIARG_DECODEBEGINFRAME
{
    HANDLE              hDecode;
    DXVADDI_PVP_SETKEY* pPVPSetKey;
} D3DDDIARG_DECODEBEGINFRAME;

typedef struct _D3DDDIARG_DECODEENDFRAME
{
    HANDLE      hDecode;
    HANDLE*     pHandleComplete;        // reserved
} D3DDDIARG_DECODEENDFRAME;

typedef struct DECLSPEC_ALIGN(16) _DXVADDI_PVP_HW_IV
{
    ULONGLONG   IV;         // Big-Endian IV
    ULONGLONG   Count;      // Big-Endian Block Count
} DXVADDI_PVP_HW_IV;

typedef struct _DXVADDI_DECODEBUFFERDESC
{
    HANDLE              hBuffer;
    D3DDDIFORMAT        CompressedBufferType;
    UINT                BufferIndex;    // reserved
    UINT                DataOffset;
    UINT                DataSize;
    UINT                FirstMBaddress;
    UINT                NumMBsInBuffer;
    UINT                Width;          // reserved
    UINT                Height;         // reserved
    UINT                Stride;         // reserved
    UINT                ReservedBits;
    DXVADDI_PVP_HW_IV   *pCipherCounter;
} DXVADDI_DECODEBUFFERDESC;

typedef struct _D3DDDIARG_DECODEEXECUTE
{
    HANDLE                          hDecode;
    UINT                            NumCompBuffers;
    DXVADDI_DECODEBUFFERDESC*       pCompressedBuffers;
} D3DDDIARG_DECODEEXECUTE;

typedef struct _DXVADDI_PRIVATEBUFFER
{
    HANDLE                 hResource;
    UINT                   SubResourceIndex;
    UINT                   DataOffset;
    UINT                   DataSize;
} DXVADDI_PRIVATEBUFFER;

typedef struct _D3DDDIARG_DECODEEXTENSIONEXECUTE
{
    HANDLE                 hDecode;
    UINT                   Function;
    DXVADDI_PRIVATEDATA*   pPrivateInput;
    DXVADDI_PRIVATEDATA*   pPrivateOutput;
    UINT                   NumBuffers;
    DXVADDI_PRIVATEBUFFER* pBuffers;
} D3DDDIARG_DECODEEXTENSIONEXECUTE;

typedef struct _D3DDDIARG_CREATEVIDEOPROCESSDEVICE
{
    CONST GUID*       pVideoProcGuid;
    DXVADDI_VIDEODESC VideoDesc;
    D3DDDIFORMAT      RenderTargetFormat;
    UINT              MaxSubStreams;
    HANDLE            hVideoProcess;
} D3DDDIARG_CREATEVIDEOPROCESSDEVICE;

typedef struct _D3DDDIARG_SETVIDEOPROCESSRENDERTARGET
{
    HANDLE      hVideoProcess;
    HANDLE      hRenderTarget;
    UINT        SubResourceIndex;
} D3DDDIARG_SETVIDEOPROCESSRENDERTARGET;

typedef struct _D3DDDIARG_VIDEOPROCESSENDFRAME
{
    HANDLE      hVideoProcess;
    HANDLE*     pHandleComplete;        // reserved
} D3DDDIARG_VIDEOPROCESSENDFRAME;

typedef struct _DXVADDI_AYUVSAMPLE8
{
    UCHAR     Cr;      // V
    UCHAR     Cb;      // U
    UCHAR     Y;
    UCHAR     Alpha;
} DXVADDI_AYUVSAMPLE8;

typedef struct _DXVADDI_AYUVSAMPLE16
{
    USHORT     Cr;      // V
    USHORT     Cb;      // U
    USHORT     Y;
    USHORT     Alpha;
} DXVADDI_AYUVSAMPLE16;

typedef struct _DXVADDI_FIXED32
{
    union {
        struct {
            USHORT      Fraction;
            SHORT       Value;
        };
        LONG ll;
    };
} DXVADDI_FIXED32;

typedef struct _DXVADDI_PROCAMPVALUES
{
    DXVADDI_FIXED32    Brightness;
    DXVADDI_FIXED32    Contrast;
    DXVADDI_FIXED32    Hue;
    DXVADDI_FIXED32    Saturation;
} DXVADDI_PROCAMPVALUES;

typedef LONGLONG REFERENCE_TIME;

#define DXVADDI_SAMPLEDATA_RFF                  0x0001
#define DXVADDI_SAMPLEDATA_TFF                  0x0002
#define DXVADDI_SAMPLEDATA_RFF_TFF_PRESENT      0x0004

typedef struct _DXVADDI_VIDEOSAMPLEFLAGS
{
    union
    {
        struct
        {
            UINT    PaletteChanged      : 1;    // 0x00000001
            UINT    SrcRectChanged      : 1;    // 0x00000002
            UINT    DstRectChanged      : 1;    // 0x00000004
            UINT    ColorDataChanged    : 1;    // 0x00000008
            UINT    PlanarAlphaChanged  : 1;    // 0x00000010
            UINT    Reserved            :11;    // 0x0000FFE0
            UINT    SampleData          :16;    // 0xFFFF0000
        };
        UINT        Value;
    };
} DXVADDI_VIDEOSAMPLEFLAGS;

typedef struct _DXVADDI_VIDEOSAMPLE
{
    REFERENCE_TIME          Start;
    REFERENCE_TIME          End;
    DXVADDI_EXTENDEDFORMAT  SampleFormat;
    DXVADDI_VIDEOSAMPLEFLAGS SampleFlags;
    HANDLE                  SrcResource;
    UINT                    SrcSubResourceIndex;
    RECT                    SrcRect;
    RECT                    DstRect;
    DXVADDI_AYUVSAMPLE8     Pal[16];
    DXVADDI_FIXED32         PlanarAlpha;
} DXVADDI_VIDEOSAMPLE;

typedef struct _DXVADDI_FILTERVALUES
{
    DXVADDI_FIXED32 Level;
    DXVADDI_FIXED32 Threshold;
    DXVADDI_FIXED32 Radius;
} DXVADDI_FILTERVALUES;

#define DXVADDI_DESTDATA_RFF                    0x0001
#define DXVADDI_DESTDATA_TFF                    0x0002
#define DXVADDI_DESTDATA_RFF_TFF_PRESENT        0x0004

typedef struct _DXVADDI_VIDEOPROCESSBLTFLAGS
{
    union
    {
        struct
        {
            UINT    BackgroundChanged   : 1;    // 0x00000001
            UINT    TargetRectChanged   : 1;    // 0x00000002
            UINT    ColorDataChanged    : 1;    // 0x00000004
            UINT    AlphaChanged        : 1;    // 0x00000008
            UINT    Reserved            :12;    // 0x0000FFF0
            UINT    DestData            :16;    // 0xFFFF0000
        };
        UINT        Value;
    };
} DXVADDI_VIDEOPROCESSBLTFLAGS;

typedef struct _D3DDDIARG_VIDEOPROCESSBLT
{
    REFERENCE_TIME          TargetFrame;
    HANDLE                  hVideoProcess;
    RECT                    TargetRect;
    SIZE                    ConstrictionSize;
    UINT                    StreamingFlags;
    DXVADDI_AYUVSAMPLE16    BackgroundColor;
    DXVADDI_EXTENDEDFORMAT  DestFormat;
    DXVADDI_VIDEOPROCESSBLTFLAGS DestFlags;
    DXVADDI_PROCAMPVALUES   ProcAmpValues;
    DXVADDI_FIXED32         Alpha;
    DXVADDI_FILTERVALUES    NoiseFilterLuma;
    DXVADDI_FILTERVALUES    NoiseFilterChroma;
    DXVADDI_FILTERVALUES    DetailFilterLuma;
    DXVADDI_FILTERVALUES    DetailFilterChroma;
    DXVADDI_VIDEOSAMPLE*    pSrcSurfaces;
    UINT                    NumSrcSurfaces;
} D3DDDIARG_VIDEOPROCESSBLT;

typedef struct _D3DDDIARG_CREATEEXTENSIONDEVICE
{
    CONST GUID*                  pGuid;
    DXVADDI_PRIVATEDATA*         pPrivate;
    HANDLE                       hExtension;
} D3DDDIARG_CREATEEXTENSIONDEVICE;

typedef struct _D3DDDIARG_EXTENSIONEXECUTE
{
    HANDLE                       hExtension;
    UINT                         Function;
    DXVADDI_PRIVATEDATA*         pPrivateInput;
    DXVADDI_PRIVATEDATA*         pPrivateOutput;
    UINT                         NumBuffers;
    DXVADDI_PRIVATEBUFFER*       pBuffers;
} D3DDDIARG_EXTENSIONEXECUTE;

typedef struct _D3DDDI_OVERLAYINFOFLAGS
{
   union
   {
       struct
       {
           UINT          DstColorKey     : 1; // 0x00000001
           UINT          DstColorKeyRange: 1; // 0x00000002
           UINT          SrcColorKey     : 1; // 0x00000004
           UINT          SrcColorKeyRange: 1; // 0x00000008
           UINT          Bob             : 1; // 0x00000010
           UINT          Interleaved     : 1; // 0x00000020
           UINT          MirrorLeftRight : 1; // 0x00000040
           UINT          MirrorUpDown    : 1; // 0x00000080
           UINT          Deinterlace     : 1; // 0x00000100
           UINT          LimitedRGB      : 1; // 0x00000200
           UINT          YCbCrBT709      : 1; // 0x00000400
           UINT          YCbCrxvYCC      : 1; // 0x00000800
           UINT          Reserved        :20; // 0xFFFFF000
       };
       UINT Value;
    };
} D3DDDI_OVERLAYINFOFLAGS;

typedef struct _D3DDDI_OVERLAYINFO
{
    HANDLE            hResource;        // in: Resource to be displayed
    UINT              SubResourceIndex; // in: Sub resource index
    RECT              DstRect;          // in: Dest rect
    RECT              SrcRect;          // in: Source rect
    UINT              DstColorKeyLow;   // in: Low dest colorkey value
    UINT              DstColorKeyHigh;  // in: High dest colorkey value
    UINT              SrcColorKeyLow;   // in: Low source colorkey value
    UINT              SrcColorKeyHigh;  // in: High source colorkey value
    D3DDDI_OVERLAYINFOFLAGS Flags;      // in: Flags
} D3DDDI_OVERLAYINFO;

typedef struct _D3DDDIARG_CREATEOVERLAY
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;      // in
    D3DDDI_OVERLAYINFO              OverlayInfo;        // in
    HANDLE                          hOverlay;           // out: Driver overlay handle
} D3DDDIARG_CREATEOVERLAY;

typedef struct _D3DDDIARG_UPDATEOVERLAY
{
    HANDLE             hOverlay;           // in: Driver overlay handle
    D3DDDI_OVERLAYINFO OverlayInfo;        // in
} D3DDDIARG_UPDATEOVERLAY;

typedef struct _D3DDDI_FLIPOVERLAYFLAGS
{
    union
    {
        struct
        {
            UINT          Even             : 1; // 0x00000001
            UINT          Odd              : 1; // 0x00000002
            UINT          Reserved         :30; // 0xFFFFFFFC
        };
        UINT Value;
    };
} D3DDDI_FLIPOVERLAYFLAGS;

typedef struct _D3DDDIARG_FLIPOVERLAY
{
    HANDLE            hOverlay;             // in: Driver overlay handle
    HANDLE            hSource;              // in: Resource currently displayed
    UINT              SourceIndex;          // in: Sub resource index
    D3DDDI_FLIPOVERLAYFLAGS Flags;          // in: Flags
} D3DDDIARG_FLIPOVERLAY;

typedef struct _D3DDDI_OVERLAYCOLORCONTROLSFLAGS
{
    union
    {
        struct
        {
            UINT           Brightness      :  1;
            UINT           Contrast        :  1;
            UINT           Hue             :  1;
            UINT           Saturation      :  1;
            UINT           Sharpness       :  1;
            UINT           Gamma           :  1;
            UINT           ColorEnable     :  1;
            UINT           Reserved        : 25;
        };
        UINT Value;
    };
} D3DDDI_OVERLAYCOLORCONTROLSFLAGS;

typedef struct _D3DDDI_OVERLAYCOLORCONTROLS
{
    INT                BrightnessSetting;
    INT                ContrastSetting;
    INT                HueSetting;
    INT                SaturationSetting;
    INT                SharpnessSetting;
    INT                GammaSetting;
    INT                ColorEnableSetting;
    D3DDDI_OVERLAYCOLORCONTROLSFLAGS Flags;
} D3DDDI_OVERLAYCOLORCONTROLS;

typedef struct _D3DDDIARG_GETOVERLAYCOLORCONTROLS
{
    HANDLE                      hOverlay;         // in: Driver overlay handle
    HANDLE                      hResource;        // in: Resource associated with the overlay
    D3DDDI_OVERLAYCOLORCONTROLS ColorControls;    // out: Current color controls
} D3DDDIARG_GETOVERLAYCOLORCONTROLS;

typedef struct _D3DDDIARG_SETOVERLAYCOLORCONTROLS
{
    HANDLE                      hOverlay;         // in: Driver overlay handle
    HANDLE                      hResource;        // in: Resource associated with the overlay
    D3DDDI_OVERLAYCOLORCONTROLS ColorControls;    // in: Current color controls
} D3DDDIARG_SETOVERLAYCOLORCONTROLS;

typedef struct _D3DDDIARG_DESTROYOVERLAY
{
    HANDLE             hOverlay;            // in: Driver overlay handle
} D3DDDIARG_DESTROYOVERLAY;

typedef struct _D3DDDIARG_QUERYRESOURCERESIDENCY
{
    CONST HANDLE*   pHandleList;            // in: Driver resource handles
    UINT            NumResources;           // in: Number of resource handles
} D3DDDIARG_QUERYRESOURCERESIDENCY;

typedef struct _D3DDDIARG_GETCAPTUREALLOCATIONHANDLE
{
    HANDLE              hResource;          // in:  Driver resource handle
    D3DKMT_HANDLE       hAllocation;        // out: Kernel mode allocation handle
} D3DDDIARG_GETCAPTUREALLOCATIONHANDLE;

typedef struct _D3DDDIARG_CAPTURETOSYSMEM
{
    HANDLE              hSrcResource;        // in: Capture buffer resource handle
    RECT                SrcRect;             // in: Source rect
    HANDLE              hDstResource;        // in: Sysmem resource handle
    UINT                DstSubResourceIndex; // in: Sysmem resource index
    RECT                DstRect;             // in: Destination rect
} D3DDDIARG_CAPTURETOSYSMEM;

typedef enum _DXVAHDDDI_FRAME_FORMAT
{
    DXVAHDDDI_FRAME_FORMAT_PROGRESSIVE                   = 0,
    DXVAHDDDI_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST    = 1,
    DXVAHDDDI_FRAME_FORMAT_INTERLACED_BOTTOM_FIELD_FIRST = 2
} DXVAHDDDI_FRAME_FORMAT;

typedef enum _DXVAHDDDI_DEVICE_USAGE
{
    DXVAHDDDI_DEVICE_USAGE_PLAYBACK_NORMAL = 0,
    DXVAHDDDI_DEVICE_USAGE_OPTIMAL_SPEED   = 1,
    DXVAHDDDI_DEVICE_USAGE_OPTIMAL_QUALITY = 2
} DXVAHDDDI_DEVICE_USAGE;

typedef enum _DXVAHDDDI_DEVICE_CAPS
{
    DXVAHDDDI_DEVICE_CAPS_LINEAR_SPACE            = 0x1,
    DXVAHDDDI_DEVICE_CAPS_xvYCC                   = 0x2,
    DXVAHDDDI_DEVICE_CAPS_RGB_RANGE_CONVERSION    = 0x4,
    DXVAHDDDI_DEVICE_CAPS_YCbCr_MATRIX_CONVERSION = 0x8,
    DXVAHDDDI_DEVICE_CAPS_NOMINAL_RANGE           = 0x10
} DXVAHDDDI_DEVICE_CAPS;

typedef enum _DXVAHDDDI_FEATURE_CAPS
{
    DXVAHDDDI_FEATURE_CAPS_ALPHA_FILL    = 0x1,
    DXVAHDDDI_FEATURE_CAPS_CONSTRICTION  = 0x2,
    DXVAHDDDI_FEATURE_CAPS_LUMA_KEY      = 0x4,
    DXVAHDDDI_FEATURE_CAPS_ALPHA_PALETTE = 0x8,
    DXVAHDDDI_FEATURE_CAPS_ROTATION      = 0x10
} DXVAHDDDI_FEATURE_CAPS;

typedef enum _DXVAHDDDI_FILTER_CAPS
{
    DXVAHDDDI_FILTER_CAPS_BRIGHTNESS         = 0x1,
    DXVAHDDDI_FILTER_CAPS_CONTRAST           = 0x2,
    DXVAHDDDI_FILTER_CAPS_HUE                = 0x4,
    DXVAHDDDI_FILTER_CAPS_SATURATION         = 0x8,
    DXVAHDDDI_FILTER_CAPS_NOISE_REDUCTION    = 0x10,
    DXVAHDDDI_FILTER_CAPS_EDGE_ENHANCEMENT   = 0x20,
    DXVAHDDDI_FILTER_CAPS_ANAMORPHIC_SCALING = 0x40
} DXVAHDDDI_FILTER_CAPS;

typedef enum _DXVAHDDDI_INPUT_FORMAT_CAPS
{
    DXVAHDDDI_INPUT_FORMAT_CAPS_RGB_INTERLACED     = 0x1,
    DXVAHDDDI_INPUT_FORMAT_CAPS_RGB_PROCAMP        = 0x2,
    DXVAHDDDI_INPUT_FORMAT_CAPS_RGB_LUMA_KEY       = 0x4,
    DXVAHDDDI_INPUT_FORMAT_CAPS_PALETTE_INTERLACED = 0x8
} DXVAHDDDI_INPUT_FORMAT_CAPS;

typedef enum _DXVAHDDDI_PROCESSOR_CAPS
{
    DXVAHDDDI_PROCESSOR_CAPS_DEINTERLACE_BLEND               = 0x1,
    DXVAHDDDI_PROCESSOR_CAPS_DEINTERLACE_BOB                 = 0x2,
    DXVAHDDDI_PROCESSOR_CAPS_DEINTERLACE_ADAPTIVE            = 0x4,
    DXVAHDDDI_PROCESSOR_CAPS_DEINTERLACE_MOTION_COMPENSATION = 0x8,
    DXVAHDDDI_PROCESSOR_CAPS_INVERSE_TELECINE                = 0x10,
    DXVAHDDDI_PROCESSOR_CAPS_FRAME_RATE_CONVERSION           = 0x20
} DXVAHDDDI_PROCESSOR_CAPS;

typedef enum _DXVAHDDDI_ITELECINE_CAPS
{
    DXVAHDDDI_ITELECINE_CAPS_32           = 0x1,
    DXVAHDDDI_ITELECINE_CAPS_22           = 0x2,
    DXVAHDDDI_ITELECINE_CAPS_2224         = 0x4,
    DXVAHDDDI_ITELECINE_CAPS_2332         = 0x8,
    DXVAHDDDI_ITELECINE_CAPS_32322        = 0x10,
    DXVAHDDDI_ITELECINE_CAPS_55           = 0x20,
    DXVAHDDDI_ITELECINE_CAPS_64           = 0x40,
    DXVAHDDDI_ITELECINE_CAPS_87           = 0x80,
    DXVAHDDDI_ITELECINE_CAPS_222222222223 = 0x100,
    DXVAHDDDI_ITELECINE_CAPS_OTHER        = 0x80000000
} DXVAHDDDI_ITELECINE_CAPS;

typedef enum _DXVAHDDDI_FILTER
{
    DXVAHDDDI_FILTER_BRIGHTNESS         = 0,
    DXVAHDDDI_FILTER_CONTRAST           = 1,
    DXVAHDDDI_FILTER_HUE                = 2,
    DXVAHDDDI_FILTER_SATURATION         = 3,
    DXVAHDDDI_FILTER_NOISE_REDUCTION    = 4,
    DXVAHDDDI_FILTER_EDGE_ENHANCEMENT   = 5,
    DXVAHDDDI_FILTER_ANAMORPHIC_SCALING = 6
} DXVAHDDDI_FILTER;

typedef enum _DXVAHDDDI_BLT_STATE
{
    DXVAHDDDI_BLT_STATE_TARGET_RECT        = 0,
    DXVAHDDDI_BLT_STATE_BACKGROUND_COLOR   = 1,
    DXVAHDDDI_BLT_STATE_OUTPUT_COLOR_SPACE = 2,
    DXVAHDDDI_BLT_STATE_ALPHA_FILL         = 3,
    DXVAHDDDI_BLT_STATE_CONSTRICTION       = 4,
    DXVAHDDDI_BLT_STATE_PRIVATE            = 1000
} DXVAHDDDI_BLT_STATE;

typedef enum _DXVAHDDDI_ALPHA_FILL_MODE
{
    DXVAHDDDI_ALPHA_FILL_MODE_OPAQUE        = 0,
    DXVAHDDDI_ALPHA_FILL_MODE_BACKGROUND    = 1,
    DXVAHDDDI_ALPHA_FILL_MODE_DESTINATION   = 2,
    DXVAHDDDI_ALPHA_FILL_MODE_SOURCE_STREAM = 3
} DXVAHDDDI_ALPHA_FILL_MODE;

typedef enum _DXVAHDDDI_ROTATION
{
    DXVAHDDDI_ROTATION_IDENTITY = 0,
    DXVAHDDDI_ROTATION_90       = 1,
    DXVAHDDDI_ROTATION_180      = 2,
    DXVAHDDDI_ROTATION_270      = 3
} DXVAHDDDI_ROTATION;

typedef enum _DXVAHDDDI_STREAM_STATE
{
    DXVAHDDDI_STREAM_STATE_FRAME_FORMAT              = 1,
    DXVAHDDDI_STREAM_STATE_INPUT_COLOR_SPACE         = 2,
    DXVAHDDDI_STREAM_STATE_OUTPUT_RATE               = 3,
    DXVAHDDDI_STREAM_STATE_SOURCE_RECT               = 4,
    DXVAHDDDI_STREAM_STATE_DESTINATION_RECT          = 5,
    DXVAHDDDI_STREAM_STATE_ALPHA                     = 6,
    DXVAHDDDI_STREAM_STATE_PALETTE                   = 7,
    DXVAHDDDI_STREAM_STATE_LUMA_KEY                  = 8,
    DXVAHDDDI_STREAM_STATE_ASPECT_RATIO              = 9,
    DXVAHDDDI_STREAM_STATE_ROTATION                  = 10,
    DXVAHDDDI_STREAM_STATE_FILTER_BRIGHTNESS         = 100,
    DXVAHDDDI_STREAM_STATE_FILTER_CONTRAST           = 101,
    DXVAHDDDI_STREAM_STATE_FILTER_HUE                = 102,
    DXVAHDDDI_STREAM_STATE_FILTER_SATURATION         = 103,
    DXVAHDDDI_STREAM_STATE_FILTER_NOISE_REDUCTION    = 104,
    DXVAHDDDI_STREAM_STATE_FILTER_EDGE_ENHANCEMENT   = 105,
    DXVAHDDDI_STREAM_STATE_FILTER_ANAMORPHIC_SCALING = 106,
    DXVAHDDDI_STREAM_STATE_PRIVATE                   = 1000
} DXVAHDDDI_STREAM_STATE;

typedef enum _DXVAHDDDI_OUTPUT_RATE
{
    DXVAHDDDI_OUTPUT_RATE_NORMAL = 0,
    DXVAHDDDI_OUTPUT_RATE_HALF   = 1,
    DXVAHDDDI_OUTPUT_RATE_CUSTOM = 2
} DXVAHDDDI_OUTPUT_RATE;

typedef struct _DXVAHDDDI_RATIONAL
{
    UINT Numerator;
    UINT Denominator;
} DXVAHDDDI_RATIONAL;

typedef struct _DXVAHDDDI_COLOR_RGBA
{
    FLOAT R;
    FLOAT G;
    FLOAT B;
    FLOAT A;
} DXVAHDDDI_COLOR_RGBA;

typedef struct _DXVAHDDDI_COLOR_YCbCrA
{
    FLOAT Y;
    FLOAT Cb;
    FLOAT Cr;
    FLOAT A;
} DXVAHDDDI_COLOR_YCbCrA;

typedef union _DXVAHDDDI_COLOR
{
    DXVAHDDDI_COLOR_RGBA   RGB;
    DXVAHDDDI_COLOR_YCbCrA YCbCr;
} DXVAHDDDI_COLOR;

typedef struct _DXVAHDDDI_BLT_STATE_TARGET_RECT_DATA
{
    BOOL Enable;
    RECT TargetRect;
} DXVAHDDDI_BLT_STATE_TARGET_RECT_DATA;

typedef struct _DXVAHDDDI_BLT_STATE_BACKGROUND_COLOR_DATA
{
    BOOL            YCbCr;
    DXVAHDDDI_COLOR BackgroundColor;
} DXVAHDDDI_BLT_STATE_BACKGROUND_COLOR_DATA;

typedef enum _DXVAHDDDI_NOMINAL_RANGE
{
    DXVAHDDDI_NOMINAL_RANGE_UNDEFINED         = 0,
    DXVAHDDDI_NOMINAL_RANGE_16_235            = 1,
    DXVAHDDDI_NOMINAL_RANGE_0_255             = 2
} DXVAHDDDI_NOMINAL_RANGE;

typedef struct _DXVAHDDDI_BLT_STATE_OUTPUT_COLOR_SPACE_DATA
{
    union
    {
        struct
        {
            UINT Usage        :  1; // 0:Playback,     1:Processing
            UINT RGB_Range    :  1; // 0:Full(0-255),  1:Limited(16-235)
            UINT YCbCr_Matrix :  1; // 0:BT.601(SDTV), 1:BT.709(HDTV)
            UINT YCbCr_xvYCC  :  1; // 0:Conventional, 1:Extended(xvYCC)
            UINT Nominal_Range : 2; // DXVAHDDDI_NOMINAL_RANGE
            UINT Reserved     : 26;
        };

        UINT Value;
    };
} DXVAHDDDI_BLT_STATE_OUTPUT_COLOR_SPACE_DATA;

typedef struct _DXVAHDDDI_BLT_STATE_ALPHA_FILL_DATA
{
    DXVAHDDDI_ALPHA_FILL_MODE Mode;
    UINT                      StreamNumber;
} DXVAHDDDI_BLT_STATE_ALPHA_FILL_DATA;

typedef struct _DXVAHDDDI_BLT_STATE_CONSTRICTION_DATA
{
    BOOL Enable;
    SIZE Size;
} DXVAHDDDI_BLT_STATE_CONSTRICTION_DATA;

typedef struct _DXVAHDDDI_BLT_STATE_PRIVATE_DATA
{
    GUID  Guid;
    UINT  DataSize;
    VOID* pData;
} DXVAHDDDI_BLT_STATE_PRIVATE_DATA;

typedef struct _DXVAHDDDI_STREAM_STATE_FRAME_FORMAT_DATA
{
    DXVAHDDDI_FRAME_FORMAT FrameFormat;
} DXVAHDDDI_STREAM_STATE_FRAME_FORMAT_DATA;

typedef struct _DXVAHDDDI_STREAM_STATE_INPUT_COLOR_SPACE_DATA
{
    union
    {
        struct
        {
            UINT Type         :  1; // 0:Video,        1:Graphics
            UINT RGB_Range    :  1; // 0:Full(0-255),  1:Limited(16-235)
            UINT YCbCr_Matrix :  1; // 0:BT.601(SDTV), 1:BT.709(HDTV)
            UINT YCbCr_xvYCC  :  1; // 0:Conventional, 1:Extended(xvYCC)
            UINT Nominal_Range : 2; // DXVAHDDDI_NOMINAL_RANGE
            UINT Reserved     : 26;
        };

        UINT Value;
    };
} DXVAHDDDI_STREAM_STATE_INPUT_COLOR_SPACE_DATA;

typedef struct _DXVAHDDDI_STREAM_STATE_OUTPUT_RATE_DATA
{
    BOOL                  RepeatFrame;
    DXVAHDDDI_OUTPUT_RATE OutputRate;
    DXVAHDDDI_RATIONAL    CustomRate;
} DXVAHDDDI_STREAM_STATE_OUTPUT_RATE_DATA;

typedef struct _DXVAHDDDI_STREAM_STATE_SOURCE_RECT_DATA
{
    BOOL Enable;
    RECT SourceRect;
} DXVAHDDDI_STREAM_STATE_SOURCE_RECT_DATA;

typedef struct _DXVAHDDDI_STREAM_STATE_DESTINATION_RECT_DATA
{
    BOOL Enable;
    RECT DestinationRect;
} DXVAHDDDI_STREAM_STATE_DESTINATION_RECT_DATA;

typedef struct _DXVAHDDDI_STREAM_STATE_ALPHA_DATA
{
    BOOL  Enable;
    FLOAT Alpha;
} DXVAHDDDI_STREAM_STATE_ALPHA_DATA;

typedef struct _DXVAHDDDI_STREAM_STATE_PALETTE_DATA
{
    UINT      Count;
    D3DCOLOR* pEntries;
} DXVAHDDDI_STREAM_STATE_PALETTE_DATA;

typedef struct _DXVAHDDDI_STREAM_STATE_LUMA_KEY_DATA
{
    BOOL  Enable;
    FLOAT Lower;
    FLOAT Upper;
} DXVAHDDDI_STREAM_STATE_LUMA_KEY_DATA;

typedef struct _DXVAHDDDI_STREAM_STATE_ASPECT_RATIO_DATA
{
    BOOL               Enable;
    DXVAHDDDI_RATIONAL SourceAspectRatio;
    DXVAHDDDI_RATIONAL DestinationAspectRatio;
} DXVAHDDDI_STREAM_STATE_ASPECT_RATIO_DATA;

typedef struct _DXVAHDDDI_STREAM_STATE_FILTER_DATA
{
    BOOL Enable;
    INT  Level;
} DXVAHDDDI_STREAM_STATE_FILTER_DATA;

typedef struct _DXVAHDDDI_STREAM_STATE_ROTATION_DATA
{
    BOOL               Enable;
    DXVAHDDDI_ROTATION Rotation;
} DXVAHDDDI_STREAM_STATE_ROTATION_DATA;

typedef struct _DXVAHDDDI_STREAM_STATE_PRIVATE_DATA
{
    GUID  Guid;
    UINT  DataSize;
    VOID* pData;
} DXVAHDDDI_STREAM_STATE_PRIVATE_DATA;

typedef struct _DXVAHDDDI_SURFACE
{
    HANDLE hResource;
    UINT   SubResourceIndex;
} DXVAHDDDI_SURFACE;

typedef struct _DXVAHDDDI_STREAM_DATA
{
    BOOL               Enable;
    UINT               OutputIndex;
    UINT               InputFrameOrField;
    UINT               PastFrames;
    UINT               FutureFrames;
    DXVAHDDDI_SURFACE* pPastSurfaces;
    DXVAHDDDI_SURFACE  InputSurface;
    DXVAHDDDI_SURFACE* pFutureSurfaces;
} DXVAHDDDI_STREAM_DATA;

DEFINE_GUID(DXVAHDDDI_STREAM_STATE_PRIVATE_IVTC, 0x9c601e3c,0x0f33,0x414c,0xa7,0x39,0x99,0x54,0x0e,0xe4,0x2d,0xa5);

typedef struct _DXVAHDDDI_STREAM_STATE_PRIVATE_IVTC_DATA
{
    BOOL Enable;
    UINT ITelecineFlags; // DXVAHDDDI_ITELECINE_CAPS
    UINT Frames;
    UINT InputField;
} DXVAHDDDI_STREAM_STATE_PRIVATE_IVTC_DATA;

typedef struct _D3DDDIARG_DXVAHD_CREATEVIDEOPROCESSOR
{
    CONST GUID* pVPGuid;                                  // in
    HANDLE      hVideoProcessor;                          // out
} D3DDDIARG_DXVAHD_CREATEVIDEOPROCESSOR;

typedef struct _D3DDDIARG_DXVAHD_SETVIDEOPROCESSBLTSTATE
{
    HANDLE              hVideoProcessor;                  // in
    DXVAHDDDI_BLT_STATE State;                            // in
    UINT                DataSize;                         // in
    CONST VOID*         pData;                            // in
} D3DDDIARG_DXVAHD_SETVIDEOPROCESSBLTSTATE;

typedef struct _D3DDDIARG_DXVAHD_GETVIDEOPROCESSBLTSTATEPRIVATE
{
    HANDLE                            hVideoProcessor;    // in
    DXVAHDDDI_BLT_STATE_PRIVATE_DATA* pData;              // in/out
} D3DDDIARG_DXVAHD_GETVIDEOPROCESSBLTSTATEPRIVATE;

typedef struct _D3DDDIARG_DXVAHD_SETVIDEOPROCESSSTREAMSTATE
{
    HANDLE                 hVideoProcessor;               // in
    UINT                   StreamNumber;                  // in
    DXVAHDDDI_STREAM_STATE State;                         // in
    UINT                   DataSize;                      // in
    CONST VOID*            pData;                         // in
} D3DDDIARG_DXVAHD_SETVIDEOPROCESSSTREAMSTATE;

typedef struct _D3DDDIARG_DXVAHD_GETVIDEOPROCESSSTREAMSTATEPRIVATE
{
    HANDLE                               hVideoProcessor; // in
    UINT                                 StreamNumber;    // in
    DXVAHDDDI_STREAM_STATE_PRIVATE_DATA* pData;           // in/out
} D3DDDIARG_DXVAHD_GETVIDEOPROCESSSTREAMSTATEPRIVATE;

typedef struct _D3DDDIARG_DXVAHD_VIDEOPROCESSBLTHD
{
    HANDLE                       hVideoProcessor;         // in
    DXVAHDDDI_SURFACE            OutputSurface;           // in
    UINT                         OutputFrame;             // in
    UINT                         StreamCount;             // in
    CONST DXVAHDDDI_STREAM_DATA* pStreams;                // in
} D3DDDIARG_DXVAHD_VIDEOPROCESSBLTHD;

typedef enum _DDIAUTHENTICATEDCHANNELTYPE
{
    DDIAUTHENTICATEDCHANNEL_DRIVER_SOFTWARE = 2,
    DDIAUTHENTICATEDCHANNEL_DRIVER_HARDWARE = 3,
} DDIAUTHENTICATEDCHANNELTYPE;

typedef struct _D3DDDIARG_CREATEAUTHENICATEDCHANNEL
{
    DDIAUTHENTICATEDCHANNELTYPE  ChannelType;             //in
    HANDLE                       hChannel;                // out
} D3DDDIARG_CREATEAUTHENTICATEDCHANNEL;

typedef struct _D3DDDIARG_AUTHENTICATEDCHANNELKEYEXCHANGE
{
    HANDLE                       hChannel;
    UINT                         DataSize;
    VOID*                        pData;
} D3DDDIARG_AUTHENTICATEDCHANNELKEYEXCHANGE;

typedef struct _D3DDDIARG_QUERYAUTHENICATEDCHANNEL
{
    // The pInputData buffer are defined identically to the
    // IDirect3DAuthenticatedChannel::Query intput buffers
    UINT                         InputSize;
    const VOID*                  pInputData;

    // The pOutputData buffer are defined identically to the
    // IDirect3DAuthenticatedChannel::Query output buffers
    UINT                         OutputSize;
    VOID*                        pOutputData;
} D3DDDIARG_QUERYAUTHENTICATEDCHANNEL;

typedef struct _D3DDDIARG_CONFIGUREAUTHENICATEDCHANNEL
{
    // pInputData buffer are defined identically to the
    // IDirect3DAuthenticatedChannel::Configure input buffers
    UINT                         InputSize;
    const VOID*                  pInputData;

    VOID*                        pOutputData;
} D3DDDIARG_CONFIGUREAUTHENTICATEDCHANNEL;

typedef struct _D3DDDIARG_DESTROYAUTHENICATEDCHANNEL
{
    HANDLE                       hChannel;
} D3DDDIARG_DESTROYAUTHENTICATEDCHANNEL;

typedef struct _D3DDDIARG_CREATECRYPTOSESSION
{
    GUID                         CryptoType;
    GUID                         DecodeProfile;
    HANDLE                       hCryptoSession;
} D3DDDIARG_CREATECRYPTOSESSION;

typedef struct _D3DDDIARG_CRYPTOSESSIONKEYEXCHANGE
{
    HANDLE                       hCryptoSession;
    UINT                         DataSize;
    VOID*                        pData;
} D3DDDIARG_CRYPTOSESSIONKEYEXCHANGE;

typedef struct _D3DDDIARG_DESTROYCRYPTOSESSION
{
    HANDLE                       hCryptoSession;
} D3DDDIARG_DESTROYCRYPTOSESSION;

typedef struct _D3DDDIARG_ENCRYPTIONBLT
{
    HANDLE                       hCryptoSession;
    HANDLE                       hSrcResource;
    UINT                         SrcSubResourceIndex;
    HANDLE                       hDstResource;
    UINT                         DstSubResourceIndex;
    UINT                         DstResourceSize;
    VOID*                        pIV;
} D3DDDIARG_ENCRYPTIONBLT;

typedef struct _D3DDDIENCRYPTED_BLOCK_INFO
{
    UINT NumEncryptedBytesAtBeginning;
    UINT NumBytesInSkipPattern;
    UINT NumBytesInEncryptPattern;
} D3DDDIENCRYPTED_BLOCK_INFO;

typedef struct _D3DDDIARG_DECRYPTIONBLT
{
    HANDLE                       hCryptoSession;
    HANDLE                       hSrcResource;
    UINT                         SrcSubResourceIndex;
    HANDLE                       hDstResource;
    UINT                         DstSubResourceIndex;
    UINT                         SrcResourceSize;
    D3DDDIENCRYPTED_BLOCK_INFO*  pEncryptedBlockInfo;
    VOID*                        pContentKey;
    VOID*                        pIV;
} D3DDDIARG_DECRYPTIONBLT;

typedef struct _D3DDDIARG_GETPITCH
{
    HANDLE                       hCryptoSession;
    HANDLE                       hResource;
    UINT                         SubResourceIndex;
    UINT                         Pitch;
} D3DDDIARG_GETPITCH;

typedef struct _D3DDDIARG_STARTSESSIONKEYREFRESH
{
    HANDLE                       hCryptoSession;
    VOID*                        pRandomNumber;
    UINT                         RandomNumberSize;
} D3DDDIARG_STARTSESSIONKEYREFRESH;

typedef struct _D3DDDIARG_FINISHSESSIONKEYREFRESH
{
    HANDLE                       hCryptoSession;
} D3DDDIARG_FINISHSESSIONKEYREFRESH;

typedef struct _GETENCRYPTIONBLTKEY
{
    HANDLE                       hCryptoSession;
    VOID*                        pReadBackKey;
    UINT                         KeySize;
} D3DDDIARG_GETENCRYPTIONBLTKEY;

typedef struct _D3DDDIARG_RESOLVESHAREDRESOURCE
{
    HANDLE                       hResource;
} D3DDDIARG_RESOLVESHAREDRESOURCE;

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8)
typedef enum D3DDDI_COPY_FLAGS
{
    D3DDDI_COPY_NO_OVERWRITE = 0x00000001,
    D3DDDI_COPY_DISCARD      = 0x00000002
} D3DDDI_COPY_FLAGS;

typedef struct _D3DDDIARG_DISCARD
{
    HANDLE      hResource;
    UINT        FirstSubResource;
    UINT        NumSubResources;
#if (D3D_UMD_INTERFACE_VERSION > D3D_UMD_INTERFACE_VERSION_WIN8_RC)
    const RECT* pRects;
    UINT        NumRects;
#endif
} D3DDDIARG_DISCARD;

typedef struct _D3DDDIARG_VOLUMEBLT1
{
    HANDLE      hDstResource;   // dest resource
    HANDLE      hSrcResource;   // src resource
    UINT        DstX;           // dest X (width)
    UINT        DstY;           // dest Y (height)
    UINT        DstZ;           // dest Z (depth)
    D3DDDIBOX   SrcBox;         // src box
    UINT        CopyFlags;      // D3DDDI_COPY_FLAGS
} D3DDDIARG_VOLUMEBLT1;

typedef struct _D3DDDIARG_BUFFERBLT1
{
    HANDLE      hDstResource;   // dest resource
    HANDLE      hSrcResource;   // src resource
    UINT        Offset;         // Offset in the dest surface (in BYTES)
    D3DDDIRANGE SrcRange;       // src range
    UINT        CopyFlags;      // D3DDDI_COPY_FLAGS
} D3DDDIARG_BUFFERBLT1;

typedef struct _D3DDDIARG_TEXBLT1
{
    HANDLE      hDstResource;   // dest resource
    HANDLE      hSrcResource;   // src resource
    UINT        CubeMapFace;
    POINT       DstPoint;
    RECT        SrcRect;        // src rect
    UINT        CopyFlags;      // D3DDDI_COPY_FLAGS
} D3DDDIARG_TEXBLT1;

typedef struct _D3DDDIARG_OFFERRESOURCES {
    _In_reads_(Resources) const HANDLE*   pResources;  // in:  array of resource handles.
    _In_ UINT                              Resources;   // in:  number of elements in pResources.
    _In_ D3DDDI_OFFER_PRIORITY             Priority;    // in:  priority with which to offer the resources
} D3DDDIARG_OFFERRESOURCES;

typedef struct _D3DDDIARG_RECLAIMRESOURCES {
    _In_reads_(Resources) HANDLE*         pResources;  // in:  array of resource handles.
    _Out_writes_all_opt_(Resources) BOOL* pDiscarded;  // out: optional array of booleans specifying whether each resource was discarded
    _In_ UINT                              Resources;   // in:  number of elements in pResources and pDiscarded
} D3DDDIARG_RECLAIMRESOURCES;

typedef enum D3DDDI_CHECK_DIRECT_FLIP_FLAGS
{
    D3DDDI_CHECKDIRECTFLIP_IMMEDIATE = 0x00000001,
} D3DDDI_CHECK_DIRECT_FLIP_FLAGS;

typedef struct _D3DDDIARG_CHECKDIRECTFLIPSUPPORT {
    HANDLE      hAppSwapchainResource;    // in:
    HANDLE      hDWMSwapchainResource;    // in:
    UINT        CheckDirectFlipFlags;     // in
    BOOL        Supported;                // out:
} D3DDDIARG_CHECKDIRECTFLIPSUPPORT;
#endif // D3D_UMD_INTERFACE_VERSION

typedef enum _D3DDDICAPS_TYPE
{
    D3DDDICAPS_DDRAW                                    = 1,
    D3DDDICAPS_DDRAW_MODE_SPECIFIC                      = 2,
    D3DDDICAPS_GETFORMATCOUNT                           = 3,
    D3DDDICAPS_GETFORMATDATA                            = 4,
    D3DDDICAPS_GETMULTISAMPLEQUALITYLEVELS              = 5,
    D3DDDICAPS_GETD3DQUERYCOUNT                         = 6,
    D3DDDICAPS_GETD3DQUERYDATA                          = 7,
    D3DDDICAPS_GETD3D3CAPS                              = 8,
    D3DDDICAPS_GETD3D5CAPS                              = 9,
    D3DDDICAPS_GETD3D6CAPS                              =10,
    D3DDDICAPS_GETD3D7CAPS                              =11,
    D3DDDICAPS_GETD3D8CAPS                              =12,
    D3DDDICAPS_GETD3D9CAPS                              =13,
    D3DDDICAPS_GETDECODEGUIDCOUNT                       =14,
    D3DDDICAPS_GETDECODEGUIDS                           =15,
    D3DDDICAPS_GETDECODERTFORMATCOUNT                   =16,
    D3DDDICAPS_GETDECODERTFORMATS                       =17,
    D3DDDICAPS_GETDECODECOMPRESSEDBUFFERINFOCOUNT       =18,
    D3DDDICAPS_GETDECODECOMPRESSEDBUFFERINFO            =19,
    D3DDDICAPS_GETDECODECONFIGURATIONCOUNT              =20,
    D3DDDICAPS_GETDECODECONFIGURATIONS                  =21,
    D3DDDICAPS_GETVIDEOPROCESSORDEVICEGUIDCOUNT         =22,
    D3DDDICAPS_GETVIDEOPROCESSORDEVICEGUIDS             =23,
    D3DDDICAPS_GETVIDEOPROCESSORRTFORMATCOUNT           =24,
    D3DDDICAPS_GETVIDEOPROCESSORRTFORMATS               =25,
    D3DDDICAPS_GETVIDEOPROCESSORRTSUBSTREAMFORMATCOUNT  =26,
    D3DDDICAPS_GETVIDEOPROCESSORRTSUBSTREAMFORMATS      =27,
    D3DDDICAPS_GETVIDEOPROCESSORCAPS                    =28,
    D3DDDICAPS_GETPROCAMPRANGE                          =29,
    D3DDDICAPS_FILTERPROPERTYRANGE                      =30,
    D3DDDICAPS_GETEXTENSIONGUIDCOUNT                    =31,
    D3DDDICAPS_GETEXTENSIONGUIDS                        =32,
    D3DDDICAPS_GETEXTENSIONCAPS                         =33,
    D3DDDICAPS_GETGAMMARAMPCAPS                         =34,
    D3DDDICAPS_CHECKOVERLAYSUPPORT                      =35,
    D3DDDICAPS_DXVAHD_GETVPDEVCAPS                      =36,
    D3DDDICAPS_DXVAHD_GETVPOUTPUTFORMATS                =37,
    D3DDDICAPS_DXVAHD_GETVPINPUTFORMATS                 =38,
    D3DDDICAPS_DXVAHD_GETVPCAPS                         =39,
    D3DDDICAPS_DXVAHD_GETVPCUSTOMRATES                  =40,
    D3DDDICAPS_DXVAHD_GETVPFILTERRANGE                  =41,
    D3DDDICAPS_GETCONTENTPROTECTIONCAPS                 =42,
    D3DDDICAPS_GETCERTIFICATESIZE                       =43,
    D3DDDICAPS_GETCERTIFICATE                           =44,
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8)
    D3DDDICAPS_GET_ARCHITECTURE_INFO                    =45,
    D3DDDICAPS_GET_SHADER_MIN_PRECISION_SUPPORT         =46,
    D3DDDICAPS_GET_MULTIPLANE_OVERLAY_CAPS              =47,
    D3DDDICAPS_GET_MULTIPLANE_OVERLAY_FILTER_RANGE      =48,
#endif // D3D_UMD_INTERFACE_VERSION
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // MP
    D3DDDICAPS_GET_MULTIPLANE_OVERLAY_GROUP_CAPS        =49,
    D3DDDICAPS_GET_SIMPLE_INSTANCING_SUPPORT            =50,
    D3DDDICAPS_GET_MARKER_CAPS                          =51,
#endif // D3D_UMD_INTERFACE_VERSION
} D3DDDICAPS_TYPE;

typedef struct _D3DDDIARG_GETCAPS
{
    D3DDDICAPS_TYPE     Type;
    VOID*               pInfo;
    VOID*               pData;
    UINT                DataSize;
} D3DDDIARG_GETCAPS;

//D3DDDICAPS_DDRAW

    typedef struct _DDRAW_CAPS
    {
        UINT   Caps;
        UINT   Caps2;
        UINT   CKeyCaps;
        UINT   FxCaps;
        UINT   MaxVideoPorts;
    } DDRAW_CAPS;

    // Caps
    #define DDRAW_CAPS_ZBLTS             0x00000001
    #define DDRAW_CAPS_COLORKEY          0x00000002
    #define DDRAW_CAPS_BLTDEPTHFILL      0x00000004

    // Caps2
    #define DDRAW_CAPS2_CANDROPZ16BIT    0x00000002
    #define DDRAW_CAPS2_FLIPINTERVAL     0x00000004
    #define DDRAW_CAPS2_FLIPNOVSYNC      0x00000008
    #define DDRAW_CAPS2_DYNAMICTEXTURES  0x00000010

    // CKeyCaps
    #define DDRAW_CKEYCAPS_SRCBLT        0x00000001
    #define DDRAW_CKEYCAPS_DESTBLT       0x00000002

    // FxCaps
    #define DDRAW_FXCAPS_BLTMIRRORLEFTRIGHT  0x00000001
    #define DDRAW_FXCAPS_BLTMIRRORUPDOWN     0x00000002

//D3DDDICAPS_DDRAW_MODE_SPECIFIC

    typedef struct _DDRAW_MODE_SPECIFIC_CAPS
    {
        UINT    Head;
        UINT    Caps;
        UINT    CKeyCaps;
        UINT    FxCaps;
        UINT    MaxVisibleOverlays;
        UINT    MinOverlayStretch;
        UINT    MaxOverlayStretch;
    } DDRAW_MODE_SPECIFIC_CAPS;

    // Caps
    #define MODE_CAPS_OVERLAY                     0x00000001
    #define MODE_CAPS_OVERLAYSTRETCH              0x00000002
    #define MODE_CAPS_CANBOBINTERLEAVED           0x00000004
    #define MODE_CAPS_CANBOBNONINTERLEAVED        0x00000008
    #define MODE_CAPS_CANFLIPODDEVEN              0x00000010
    #define MODE_CAPS_READSCANLINE                0x00000020
    #define MODE_CAPS_COLORCONTROLOVERLAY         0x00000040

    // CKeyCaps
    #define MODE_CKEYCAPS_DESTOVERLAY             0x00000001
    #define MODE_CKEYCAPS_DESTOVERLAYYUV          0x00000002
    #define MODE_CKEYCAPS_SRCOVERLAY              0x00000004
    #define MODE_CKEYCAPS_SRCOVERLAYCLRSPACE      0x00000008
    #define MODE_CKEYCAPS_SRCOVERLAYCLRSPACEYUV   0x00000010
    #define MODE_CKEYCAPS_SRCOVERLAYYUV           0x00000020

    // FxCaps
    #define MODE_FXCAPS_OVERLAYSHRINKX            0x00000001
    #define MODE_FXCAPS_OVERLAYSHRINKY            0x00000002
    #define MODE_FXCAPS_OVERLAYSTRETCHX           0x00000004
    #define MODE_FXCAPS_OVERLAYSTRETCHY           0x00000008
    #define MODE_FXCAPS_OVERLAYMIRRORLEFTRIGHT    0x00000010
    #define MODE_FXCAPS_OVERLAYMIRRORUPDOWN       0x00000020
    #define MODE_FXCAPS_OVERLAYDEINTERLACE        0x00000040

//D3DDDICAPS_GETFORMATCOUNT

//D3DDDICAPS_GETFORMATDATA

    typedef struct _FORMATOP
    {
        D3DDDIFORMAT  Format;
        UINT          Operations;
        UINT          FlipMsTypes;
        UINT          BltMsTypes;
        UINT          PrivateFormatBitCount;
    } FORMATOP;

    #define FORMATOP_TEXTURE                    0x00000001L
    #define FORMATOP_VOLUMETEXTURE              0x00000002L
    #define FORMATOP_CUBETEXTURE                0x00000004L
    #define FORMATOP_OFFSCREEN_RENDERTARGET     0x00000008L
    #define FORMATOP_SAME_FORMAT_RENDERTARGET   0x00000010L
    #define FORMATOP_ZSTENCIL                   0x00000040L
    #define FORMATOP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH 0x00000080L
    #define FORMATOP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET 0x00000100L
    #define FORMATOP_DISPLAYMODE                0x00000400L
    #define FORMATOP_3DACCELERATION             0x00000800L
    #define FORMATOP_PIXELSIZE                  0x00001000L
    #define FORMATOP_CONVERT_TO_ARGB            0x00002000L
    #define FORMATOP_OFFSCREENPLAIN             0x00004000L
    #define FORMATOP_SRGBREAD                   0x00008000L
    #define FORMATOP_BUMPMAP                    0x00010000L
    #define FORMATOP_DMAP                       0x00020000L
    #define FORMATOP_NOFILTER                   0x00040000L
    #define FORMATOP_MEMBEROFGROUP_ARGB         0x00080000L
    #define FORMATOP_SRGBWRITE                  0x00100000L
    #define FORMATOP_NOALPHABLEND               0x00200000L
    #define FORMATOP_AUTOGENMIPMAP              0x00400000L
    #define FORMATOP_VERTEXTEXTURE              0x00800000L
    #define FORMATOP_NOTEXCOORDWRAPNORMIP       0x01000000L
    #define FORMATOP_PLANAR                     0x02000000L
    #define FORMATOP_OVERLAY                    0x04000000L
    #define FORMATOP_CAPTURE                    0x08000000L
    #define FORMATOP_VIDEO_ENCODER              0x10000000L
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8)
    #define FORMATOP_MULTIPLANE_OVERLAY         0x20000000L
#endif // D3D_UMD_INTERFACE_VERSION

//D3DDDICAPS_GETMULTISAMPLEQUALITYLEVELS

    typedef struct _DDIMULTISAMPLEQUALITYLEVELSDATA
    {
        D3DDDIFORMAT            Format;
        BOOL                    Flip;
        D3DDDIMULTISAMPLE_TYPE  MsType;
        UINT                    QualityLevels;
    } DDIMULTISAMPLEQUALITYLEVELSDATA;

//D3DDDICAPS_GETD3DQUERYCOUNT
//D3DDDICAPS_GETD3DQUERYDATA
//D3DDDICAPS_GETD3D3CAPS
//D3DDDICAPS_GETD3D5CAPS
//D3DDDICAPS_GETD3D6CAPS
//D3DDDICAPS_GETD3D7CAPS
//D3DDDICAPS_GETD3D8CAPS
//D3DDDICAPS_GETD3D9CAPS

//D3DDDICAPS_GETDECODEGUIDCOUNT
//D3DDDICAPS_GETDECODEGUIDS

    DEFINE_GUID(DXVADDI_ModeMPEG2_MoComp, 0xe6a9f44b, 0x61b0, 0x4563,0x9e,0xa4,0x63,0xd2,0xa3,0xc6,0xfe,0x66);
    DEFINE_GUID(DXVADDI_ModeMPEG2_IDCT,   0xbf22ad00, 0x03ea, 0x4690,0x80,0x77,0x47,0x33,0x46,0x20,0x9b,0x7e);
    DEFINE_GUID(DXVADDI_ModeMPEG2_VLD,    0xee27417f, 0x5e28, 0x4e65,0xbe,0xea,0x1d,0x26,0xb5,0x08,0xad,0xc9);
    DEFINE_GUID(DXVADDI_ModeMPEG1_VLD,    0x6f3ec719, 0x3735, 0x42cc,0x80,0x63,0x65,0xcc,0x3c,0xb3,0x66,0x16);
    DEFINE_GUID(DXVADDI_ModeMPEG2and1_VLD,0x86695f12, 0x340e, 0x4f04,0x9f,0xd3,0x92,0x53,0xdd,0x32,0x74,0x60);

    DEFINE_GUID(DXVADDI_ModeH264_A,  0x1b81be64, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
    DEFINE_GUID(DXVADDI_ModeH264_B,  0x1b81be65, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
    DEFINE_GUID(DXVADDI_ModeH264_C,  0x1b81be66, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
    DEFINE_GUID(DXVADDI_ModeH264_D,  0x1b81be67, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
    DEFINE_GUID(DXVADDI_ModeH264_E,  0x1b81be68, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
    DEFINE_GUID(DXVADDI_ModeH264_F,  0x1b81be69, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
    DEFINE_GUID(DXVADDI_ModeH264_VLD_WithFMOASO_NoFGT,  0xd5f04ff9, 0x3418,0x45d8,0x95,0x61,0x32,0xa7,0x6a,0xae,0x2d,0xdd);
    DEFINE_GUID(DXVADDI_ModeH264_VLD_Stereo_Progressive_NoFGT,     0xd79be8da, 0x0cf1, 0x4c81,0xb8,0x2a,0x69,0xa4,0xe2,0x36,0xf4,0x3d);
    DEFINE_GUID(DXVADDI_ModeH264_VLD_Stereo_NoFGT,                 0xf9aaccbb, 0xc2b6, 0x4cfc,0x87,0x79,0x57,0x07,0xb1,0x76,0x05,0x52);
    DEFINE_GUID(DXVADDI_ModeH264_VLD_Multiview_NoFGT,              0x705b9d82, 0x76cf, 0x49d6,0xb7,0xe6,0xac,0x88,0x72,0xdb,0x01,0x3c);

    DEFINE_GUID(DXVADDI_ModeWMV8_A,  0x1b81be80, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
    DEFINE_GUID(DXVADDI_ModeWMV8_B,  0x1b81be81, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);

    DEFINE_GUID(DXVADDI_ModeWMV9_A,  0x1b81be90, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
    DEFINE_GUID(DXVADDI_ModeWMV9_B,  0x1b81be91, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
    DEFINE_GUID(DXVADDI_ModeWMV9_C,  0x1b81be94, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);

    DEFINE_GUID(DXVADDI_ModeVC1_A,   0x1b81beA0, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
    DEFINE_GUID(DXVADDI_ModeVC1_B,   0x1b81beA1, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
    DEFINE_GUID(DXVADDI_ModeVC1_C,   0x1b81beA2, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
    DEFINE_GUID(DXVADDI_ModeVC1_D,   0x1b81beA3, 0xa0c7, 0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);
    DEFINE_GUID(DXVADDI_ModeVC1_D2010,0x1b81beA4, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);

    DEFINE_GUID(DXVADDI_ModeMPEG4pt2_VLD_Simple,           0xefd64d74, 0xc9e8, 0x41d7,0xa5,0xe9,0xe9,0xb0,0xe3,0x9f,0xa3,0x19);
    DEFINE_GUID(DXVADDI_ModeMPEG4pt2_VLD_AdvSimple_NoGMC,  0xed418a9f, 0x010d, 0x4eda,0x9a,0xe3,0x9a,0x65,0x35,0x8d,0x8d,0x2e);
    DEFINE_GUID(DXVADDI_ModeMPEG4pt2_VLD_AdvSimple_GMC,    0xab998b5b, 0x4258, 0x44a9,0x9f,0xeb,0x94,0xe5,0x97,0xa6,0xba,0xae);

    DEFINE_GUID(DXVADDI_ModeHEVC_VLD_Main,    0x5b11d51b, 0x2f4c, 0x4452,0xbc,0xc3,0x09,0xf2,0xa1,0x16,0x0c,0xc0);
    DEFINE_GUID(DXVADDI_ModeHEVC_VLD_Main10,  0x107af0e0, 0xef1a, 0x4d19,0xab,0xa8,0x67,0xa1,0x63,0x07,0x3d,0x13);

    DEFINE_GUID(DXVADDI_ModeVP9_VLD_Profile0,       0x463707f8, 0xa1d0, 0x4585, 0x87, 0x6d, 0x83, 0xaa, 0x6d, 0x60, 0xb8, 0x9e);
    DEFINE_GUID(DXVADDI_ModeVP9_VLD_10bit_Profile2, 0xa4c749ef, 0x6ecf, 0x48aa, 0x84, 0x48, 0x50, 0xa7, 0xa1, 0x16, 0x5f, 0xf7);
    DEFINE_GUID(DXVADDI_ModeVP8_VLD,                0x90b899ea, 0x3a62, 0x4705, 0x88, 0xb3, 0x8d, 0xf0, 0x4b, 0x27, 0x44, 0xe7);



    #define DXVADDI_ModeMPEG2_MOCOMP        DXVADDI_ModeMPEG2_MoComp

    #define DXVADDI_ModeWMV8_PostProc       DXVADDI_ModeWMV8_A
    #define DXVADDI_ModeWMV8_MoComp         DXVADDI_ModeWMV8_B

    #define DXVADDI_ModeWMV9_PostProc       DXVADDI_ModeWMV9_A
    #define DXVADDI_ModeWMV9_MoComp         DXVADDI_ModeWMV9_B
    #define DXVADDI_ModeWMV9_IDCT           DXVADDI_ModeWMV9_C

    #define DXVADDI_ModeVC1_PostProc        DXVADDI_ModeVC1_A
    #define DXVADDI_ModeVC1_MoComp          DXVADDI_ModeVC1_B
    #define DXVADDI_ModeVC1_IDCT            DXVADDI_ModeVC1_C
    #define DXVADDI_ModeVC1_VLD             DXVADDI_ModeVC1_D

    #define DXVADDI_ModeH264_MoComp_NoFGT   DXVADDI_ModeH264_A
    #define DXVADDI_ModeH264_MoComp_FGT     DXVADDI_ModeH264_B
    #define DXVADDI_ModeH264_IDCT_NoFGT     DXVADDI_ModeH264_C
    #define DXVADDI_ModeH264_IDCT_FGT       DXVADDI_ModeH264_D
    #define DXVADDI_ModeH264_VLD_NoFGT      DXVADDI_ModeH264_E
    #define DXVADDI_ModeH264_VLD_FGT        DXVADDI_ModeH264_F

//D3DDDICAPS_GETDECODERTFORMATCOUNT
//D3DDDICAPS_GETDECODERTFORMATS
//D3DDDICAPS_GETDECODECOMPRESSEDBUFFERINFOCOUNT
//D3DDDICAPS_GETDECODECOMPRESSEDBUFFERINFO

    typedef struct _DXVADDI_DECODEINPUT
    {
        CONST GUID*        pGuid;
        DXVADDI_VIDEODESC  VideoDesc;
    } DXVADDI_DECODEINPUT;

    typedef struct _DXVADDI_DECODEBUFFERINFO
    {
        D3DDDIFORMAT       CompressedBufferType;
        UINT               CreationWidth;
        UINT               CreationHeight;
        D3DDDI_POOL        CreationPool;
    } DXVADDI_DECODEBUFFERINFO;

//D3DDDICAPS_GETDECODECONFIGURATIONCOUNT
//D3DDDICAPS_GETDECODECONFIGURATIONS

    DEFINE_GUID(DXVADDI_NoEncrypt,  0x1b81beD0, 0xa0c7,0x11d3,0xb9,0x84,0x00,0xc0,0x4f,0x2e,0x73,0xc5);

//D3DDDICAPS_GETVIDEOPROCESSORDEVICEGUIDCOUNT
//D3DDDICAPS_GETVIDEOPROCESSORDEVICEGUIDS

    DEFINE_GUID(DXVADDI_VideoProcProgressiveDevice, 0x5a54a0c9,0xc7ec,0x4bd9,0x8e,0xde,0xf3,0xc7,0x5d,0xc4,0x39,0x3b);
    DEFINE_GUID(DXVADDI_VideoProcBobDevice,         0x335aa36e,0x7884,0x43a4,0x9c,0x91,0x7f,0x87,0xfa,0xf3,0xe3,0x7e);

//D3DDDICAPS_GETVIDEOPROCESSORRTFORMATCOUNT
//D3DDDICAPS_GETVIDEOPROCESSORRTFORMATS
//D3DDDICAPS_GETVIDEOPROCESSORRTSUBSTREAMFORMATCOUNT
//D3DDDICAPS_GETVIDEOPROCESSORRTSUBSTREAMFORMATS
//D3DDDICAPS_GETVIDEOPROCESSORCAPS

    typedef struct _DXVADDI_VIDEOPROCESSORCAPS
    {
        D3DDDI_POOL              InputPool;
        UINT                     NumForwardRefSamples;
        UINT                     NumBackwardRefSamples;
        D3DDDIFORMAT             OutputFormat;
        UINT                     DeinterlaceTechnology;
        UINT                     ProcAmpControlCaps;
        UINT                     VideoProcessorOperations;
        UINT                     NoiseFilterTechnology;
        UINT                     DetailFilterTechnology;
    } DXVADDI_VIDEOPROCESSORCAPS;

    #define DXVADDI_DEINTERLACETECH_UNKNOWN                         0x0000
    #define DXVADDI_DEINTERLACETECH_BOBLINEREPLICATE                0x0001
    #define DXVADDI_DEINTERLACETECH_BOBVERTICALSTRETCH              0x0002
    #define DXVADDI_DEINTERLACETECH_BOBVERTICALSTRETCH4TAP          0x0004
    #define DXVADDI_DEINTERLACETECH_MEDIANFILTERING                 0x0008
    #define DXVADDI_DEINTERLACETECH_EDGEFILTERING                   0x0010
    #define DXVADDI_DEINTERLACETECH_FIELDADAPTIVE                   0x0020
    #define DXVADDI_DEINTERLACETECH_PIXELADAPTIVE                   0x0040
    #define DXVADDI_DEINTERLACETECH_MOTIONVECTORSTEERED             0x0080
    #define DXVADDI_DEINTERLACETECH_INVERSETELECINE                 0x0100

    #define DXVADDI_PROCAMP_NONE                                    0x0000
    #define DXVADDI_PROCAMP_BRIGHTNESS                              0x0001
    #define DXVADDI_PROCAMP_CONTRAST                                0x0002
    #define DXVADDI_PROCAMP_HUE                                     0x0004
    #define DXVADDI_PROCAMP_SATURATION                              0x0008

    #define DXVADDI_VIDEOPROCESS_NONE                               0x0000
    #define DXVADDI_VIDEOPROCESS_YUV2RGB                            0x0001
    #define DXVADDI_VIDEOPROCESS_STRETCHX                           0x0002
    #define DXVADDI_VIDEOPROCESS_STRETCHY                           0x0004
    #define DXVADDI_VIDEOPROCESS_ALPHABLEND                         0x0008
    #define DXVADDI_VIDEOPROCESS_SUBRECTS                           0x0010
    #define DXVADDI_VIDEOPROCESS_SUBSTREAMS                         0x0020
    #define DXVADDI_VIDEOPROCESS_SUBSTREAMSEXTENDED                 0x0040
    #define DXVADDI_VIDEOPROCESS_YUV2RGBEXTENDED                    0x0080
    #define DXVADDI_VIDEOPROCESS_ALPHABLENDEXTENDED                 0x0100
    #define DXVADDI_VIDEOPROCESS_CONSTRICTION                       0x0200
    #define DXVADDI_VIDEOPROCESS_NOISEFILTER                        0x0400
    #define DXVADDI_VIDEOPROCESS_DETAILFILTER                       0x0800
    #define DXVADDI_VIDEOPROCESS_PLANARALPHA                        0x1000
    #define DXVADDI_VIDEOPROCESS_LINEARSCALING                      0x2000
    #define DXVADDI_VIDEOPROCESS_GAMMACOMPENSATED                   0x4000
    #define DXVADDI_VIDEOPROCESS_MAINTAINSORIGINALFIELDDATA         0x8000

    #define DXVADDI_NOISEFILTERTECH_UNSUPPORTED                     0x0000
    #define DXVADDI_NOISEFILTERTECH_UNKNOWN                         0x0001
    #define DXVADDI_NOISEFILTERTECH_MEDIAN                          0x0002
    #define DXVADDI_NOISEFILTERTECH_TEMPORAL                        0x0004
    #define DXVADDI_NOISEFILTERTECH_BLOCKNOISE                      0x0008
    #define DXVADDI_NOISEFILTERTECH_MOSQUITONOISE                   0x0010

    #define DXVADDI_DETAILFILTERTECH_UNSUPPORTED                    0x0000
    #define DXVADDI_DETAILFILTERTECH_UNKNOWN                        0x0001
    #define DXVADDI_DETAILFILTERTECH_EDGE                           0x0002
    #define DXVADDI_DETAILFILTERTECH_SHARPENING                     0x0004

    typedef struct _DXVADDI_VIDEOPROCESSORINPUT
    {
        CONST GUID*              pVideoProcGuid;
        DXVADDI_VIDEODESC        VideoDesc;
        D3DDDIFORMAT             RenderTargetFormat;
    } DXVADDI_VIDEOPROCESSORINPUT;

//D3DDDICAPS_GETPROCAMPRANGE

    typedef struct _DXVADDI_QUERYPROCAMPINPUT
    {
        CONST GUID*              pVideoProcGuid;
        DXVADDI_VIDEODESC        VideoDesc;
        D3DDDIFORMAT             RenderTargetFormat;
        UINT                     ProcAmpCap;
    } DXVADDI_QUERYPROCAMPINPUT;

    typedef struct _DXVADDI_VALUERANGE
    {
        DXVADDI_FIXED32          MinValue;
        DXVADDI_FIXED32          MaxValue;
        DXVADDI_FIXED32          DefaultValue;
        DXVADDI_FIXED32          StepSize;
    } DXVADDI_VALUERANGE;

//D3DDDICAPS_FILTERPROPERTYRANGE

    #define DXVADDI_NOISEFILTER_LUMALEVEL                           1
    #define DXVADDI_NOISEFILTER_LUMATHREASHOLD                      2
    #define DXVADDI_NOISEFILTER_LUMARADIUS                          3
    #define DXVADDI_NOISEFILTER_CHROMALEVEL                         4
    #define DXVADDI_NOISEFILTER_CHROMATHREASHOLD                    5
    #define DXVADDI_NOISEFILTER_CHROMARADIUS                        6
    #define DXVADDI_DETAILFILTER_LUMALEVEL                          7
    #define DXVADDI_DETAILFILTER_LUMATHREASHOLD                     8
    #define DXVADDI_DETAILFILTER_LUMARADIUS                         9
    #define DXVADDI_DETAILFILTER_CHROMALEVEL                        10
    #define DXVADDI_DETAILFILTER_CHROMATHREASHOLD                   11
    #define DXVADDI_DETAILFILTER_CHROMARADIUS                       12

    typedef struct _DXVADDI_QUERYFILTERPROPERTYRANGEINPUT
    {
        CONST GUID*              pVideoProcGuid;
        DXVADDI_VIDEODESC        VideoDesc;
        D3DDDIFORMAT             RenderTargetFormat;
        UINT                     FilterSetting;
    } DXVADDI_QUERYFILTERPROPERTYRANGEINPUT;

//D3DDDICAPS_GETEXTENSIONGUIDCOUNT
//D3DDDICAPS_GETEXTENSIONGUIDS
//D3DDDICAPS_GETEXTENSIONCAPS

    #define DXVADDI_EXTENSION_CATEGORY_DECODER                      0x0001
    #define DXVADDI_EXTENSION_CATEGORY_ENCODER                      0x0002
    #define DXVADDI_EXTENSION_CATEGORY_PROCESSOR                    0x0004
    #define DXVADDI_EXTENSION_CATEGORY_ALL                          0x0007

    #define DXVADDI_EXTENSION_CAPTYPE_MIN                           300
    #define DXVADDI_EXTENSION_CAPTYPE_MAX                           400

    typedef struct _DXVADDI_QUERYEXTENSIONCAPSINPUT
    {
        CONST GUID*              pGuid;
        UINT                     CapType;
        DXVADDI_PRIVATEDATA*     pPrivate;
    } DXVADDI_QUERYEXTENSIONCAPSINPUT;

//D3DDDICAPS_GETGAMMARAMPCAPS

    typedef struct _DDIGAMMACAPS
    {
        UINT                     GammaCaps;
    } DDIGAMMACAPS;

    // Caps
    #define GAMMA_CAP_RGB256x3x16           0x00000001    // Standard GDI gamma ramps (256 entries, 3 channels per entry, 16 bits per channel)

//D3DDDICAPS_CHECKOVERLAYSUPPORT

    typedef struct _DDICHECKOVERLAYSUPPORTINPUT
    {
        UINT                    OverlayWidth;
        UINT                    OverlayHeight;
        D3DDDIFORMAT            OverlayFormat;
        UINT                    DisplayWidth;
        UINT                    DisplayHeight;
        UINT                    DisplayRefreshRate;
        D3DDDIFORMAT            DisplayFormat;
        D3DDDI_SCANLINEORDERING DisplayScanLineOrdering;
        D3DDDI_ROTATION         DisplayRotation;
    } DDICHECKOVERLAYSUPPORTINPUT;

//D3DDDICAPS_DXVAHD_GETVPDEVCAPS
//D3DDDICAPS_DXVAHD_GETVPOUTPUTFORMATS
//D3DDDICAPS_DXVAHD_GETVPINPUTFORMATS
//D3DDDICAPS_DXVAHD_GETVPCAPS

    typedef struct _DXVAHDDDI_CONTENT_DESC
    {
        DXVAHDDDI_FRAME_FORMAT InputFrameFormat;
        DXVAHDDDI_RATIONAL     InputFrameRate;
        UINT                   InputWidth;
        UINT                   InputHeight;
        DXVAHDDDI_RATIONAL     OutputFrameRate;
        UINT                   OutputWidth;
        UINT                   OutputHeight;
    } DXVAHDDDI_CONTENT_DESC;

    typedef struct _DXVAHDDDI_DEVICE_DESC
    {
        DXVAHDDDI_CONTENT_DESC* pContentDesc;
        DXVAHDDDI_DEVICE_USAGE  Usage;
    } DXVAHDDDI_DEVICE_DESC;

    typedef struct _DXVAHDDDI_VPDEVCAPS
    {
        UINT        Reserved;
        UINT        DeviceCaps;          // DXVAHDDDI_DEVICE_CAPS
        UINT        FeatureCaps;         // DXVAHDDDI_FEATURE_CAPS
        UINT        FilterCaps;          // DXVAHDDDI_FILTER_CAPS
        UINT        InputFormatCaps;     // DXVAHDDDI_INPUT_FORMAT_CAPS
        D3DDDI_POOL InputPool;
        UINT        OutputFormatCount;
        UINT        InputFormatCount;
        UINT        VideoProcessorCount;
        UINT        MaxInputStreams;
        UINT        MaxStreamStates;
    } DXVAHDDDI_VPDEVCAPS;

    typedef struct _DXVAHDDDI_VPCAPS
    {
        GUID VPGuid;
        UINT PastFrames;
        UINT FutureFrames;
        UINT ProcessorCaps;   // DXVAHDDDI_PROCESSOR_CAPS
        UINT ITelecineCaps;   // DXVAHDDDI_ITELECINE_CAPS
        UINT CustomRateCount;
    } DXVAHDDDI_VPCAPS;

//D3DDDICAPS_DXVAHD_GETVPCUSTOMRATES

    typedef struct _DXVAHDDDI_CUSTOM_RATE_DATA
    {
        DXVAHDDDI_RATIONAL CustomRate;
        UINT               OutputFrames;
        BOOL               InputInterlaced;
        UINT               InputFramesOrFields;
    } DXVAHDDDI_CUSTOM_RATE_DATA;

//D3DDDICAPS_DXVAHD_GETVPFILTERRANGE

    typedef struct _DXVAHDDDI_FILTER_RANGE_DATA
    {
        INT   Minimum;
        INT   Maximum;
        INT   Default;
        FLOAT Multiplier;
    } DXVAHDDDI_FILTER_RANGE_DATA;

//D3DDDICAPS_GETCONTENTPROTECTIONCAPS

    typedef struct _DDICONTENTPROTECTIONCAPS
    {
        GUID CryptoType;
        GUID DecodeProfile;
    } DDICONTENTPROTECTIONCAPS;

//D3DDDICAPS_GETCERTIFICATESIZE
//D3DDDICAPS_GETCERTIFICATE

    typedef enum _D3DDDI_CERTIFICATETYPE
    {
        D3DDDI_CERTTYPE_AUTHENTICATED_CHANNEL = 1,
        D3DDDI_CERTTYPE_CRYPTOSESSION         = 2
    } D3DDDI_CERTIFICATETYPE;

    typedef struct _DDICERTIFICATEINFO
    {
        D3DDDI_CERTIFICATETYPE CertificateType;
        DDIAUTHENTICATEDCHANNELTYPE ChannelType;
        GUID CryptoSessionType;
    } DDICERTIFICATEINFO;

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8)
//D3DDDICAPS_GET_ARCHITECTURE_INFO

    typedef struct D3DDDICAPS_ARCHITECTURE_INFO
    {
        BOOL TileBasedDeferredRenderer;
    } D3DDDICAPS_ARCHITECTURE_INFO;

    typedef enum D3DDDICAPS_SHADER_MIN_PRECISION
    {
        D3DDDICAPS_SHADER_MIN_PRECISION_10_BIT = 0x1,
        D3DDDICAPS_SHADER_MIN_PRECISION_16_BIT = 0x2
    } D3DDDICAPS_SHADER_MIN_PRECISION;

    typedef struct D3DDDICAPS_SHADER_MIN_PRECISION_SUPPORT
    {
        UINT VertexShaderMinPrecision; // D3DDDICAPS_SHADER_MIN_PRECISION for VS
        UINT PixelShaderMinPrecision; // D3DDDICAPS_SHADER_MIN_PRECISION for PS
    } D3DDDICAPS_SHADER_MIN_PRECISION_SUPPORT;

// D3DDDICAPS_GET_MULTIPLANE_OVERLAY_CAPS

    typedef struct D3DDDI_MULTIPLANE_OVERLAY_CAPS
    {
        UINT MaxPlanes;
        UINT NumCapabilityGroups;    
    } D3DDDI_MULTIPLANE_OVERLAY_CAPS;

// D3DDDICAPS_GET_MULTIPLANE_OVERLAY_GROUP_CAPS

    typedef struct D3DDDI_MULTIPLANE_OVERLAY_GROUP_CAPS_INPUT
    {
        D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
        UINT                           GroupIndex;
    } D3DDDI_MULTIPLANE_OVERLAY_GROUP_CAPS_INPUT;

    typedef enum _D3DDDI_MULTIPLANE_OVERLAY_FEATURE_CAPS
    {
        D3DDDI_MULTIPLANE_OVERLAY_FEATURE_CAPS_ROTATION          = 0x1,
        D3DDDI_MULTIPLANE_OVERLAY_FEATURE_CAPS_VERTICAL_FLIP     = 0x2,
        D3DDDI_MULTIPLANE_OVERLAY_FEATURE_CAPS_HORIZONTAL_FLIP   = 0x4,
        D3DDDI_MULTIPLANE_OVERLAY_FEATURE_CAPS_DEINTERLACE       = 0x8, 
        D3DDDI_MULTIPLANE_OVERLAY_FEATURE_CAPS_RGB               = 0x10,
        D3DDDI_MULTIPLANE_OVERLAY_FEATURE_CAPS_YUV               = 0x20,
        D3DDDI_MULTIPLANE_OVERLAY_FEATURE_CAPS_BILINEAR_FILTER   = 0x40,
        D3DDDI_MULTIPLANE_OVERLAY_FEATURE_CAPS_HIGH_FILTER       = 0x100,
    } D3DDDI_MULTIPLANE_OVERLAY_FEATURE_CAPS;

    typedef struct D3DDDI_MULTIPLANE_OVERLAY_GROUP_CAPS
    {
        UINT  NumPlanes;
        float MaxStretchFactor;
        float MaxShrinkFactor;
        UINT  OverlayCaps;        // D3DDDI_MULTIPLANE_OVERLAY_FEATURE_CAPS
    } D3DDDI_MULTIPLANE_OVERLAY_GROUP_CAPS;

#endif // D3D_UMD_INTERFACE_VERSION

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // MP 

// D3DDDICAPS_GET_SIMPLE_INSTANCING_SUPPORT

    typedef struct D3DDDICAPS_SIMPLE_INSTANCING_SUPPORT
    {
        BOOL SimpleInstancingSupported;
    } D3DDDICAPS_SIMPLE_INSTANCING_SUPPORT;

// D3DDDICAPS_GET_MARKER_CAPS
    typedef enum D3DDDI_MARKERTYPE
    {
        D3DDDIMT_NONE,
        D3DDDIMT_PROFILE,
    } D3DDDI_MARKERTYPE;
#endif // D3D_UMD_INTERFACE_VERSION

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8)

//
// Hardware Multi Plane Overlay DDI
//

#define D3DDDI_MAX_MULTIPLANE_OVERLAY_ALLOCATIONS 16

typedef enum _D3DDDI_MULTIPLANE_OVERLAY_FLAGS
{
    D3DDDI_MULTIPLANE_OVERLAY_FLAG_VERTICAL_FLIP   = 0x1,
    D3DDDI_MULTIPLANE_OVERLAY_FLAG_HORIZONTAL_FLIP = 0x2,
} D3DDDI_MULTIPLANE_OVERLAY_FLAGS;

typedef enum _D3DDDI_MULTIPLANE_OVERLAY_BLEND
{
    D3DDDI_MULTIPLANE_OVERLAY_BLEND_OPAQUE     = 0x0,
    D3DDDI_MULTIPLANE_OVERLAY_BLEND_ALPHABLEND = 0x1,
} D3DDDI_MULTIPLANE_OVERLAY_BLEND;

typedef enum D3DDDI_MULTIPLANE_OVERLAY_VIDEO_FRAME_FORMAT
{
    D3DDDI_MULIIPLANE_OVERLAY_VIDEO_FRAME_FORMAT_PROGRESSIVE  = 0,
    D3DDDI_MULTIPLANE_OVERLAY_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST   = 1,
    D3DDDI_MULTIPLANE_OVERLAY_VIDEO_FRAME_FORMAT_INTERLACED_BOTTOM_FIELD_FIRST    = 2
} D3DDDI_MULTIPLANE_OVERLAY_VIDEO_FRAME_FORMAT;

typedef enum D3DDDI_MULTIPLANE_OVERLAY_YCbCr_FLAGS
{
    D3DDDI_MULTIPLANE_OVERLAY_YCbCr_FLAG_NOMINAL_RANGE = 0x1, // 16 - 235 vs. 0 - 255
    D3DDDI_MULTIPLANE_OVERLAY_YCbCr_FLAG_BT709         = 0x2, // BT.709 vs. BT.601
    D3DDDI_MULTIPLANE_OVERLAY_YCbCr_FLAG_xvYCC         = 0x4, // xvYCC vs. conventional YCbCr
} D3DDDI_MULTIPLANE_OVERLAY_YCbCr_FLAGS;

typedef enum D3DDDI_MULTIPLANE_OVERLAY_STRETCH_QUALITY
{
    D3DDDI_MULTIPLANE_OVERLAY_STRETCH_QUALITY_BILINEAR        = 0x1,  // Bilinear
    D3DDDI_MULTIPLANE_OVERLAY_STRETCH_QUALITY_HIGH            = 0x2,  // Maximum
} D3DDDI_MULTIPLANE_OVERLAY_STRETCH_QUALITY;

typedef struct _D3DDDI_MULTIPLANE_OVERLAY_ATTRIBUTES
{
    UINT                                 Flags;  // D3DDDI_MULTIPLANE_OVERLAY_FLAGS
    RECT                                 SrcRect;
    RECT                                 DstRect;
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // M1
    RECT                                 ClipRect;
#endif
    D3DDDI_ROTATION                      Rotation;
    D3DDDI_MULTIPLANE_OVERLAY_BLEND      Blend;
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // MP
    UINT                                 DirtyRectCount;
    RECT*                                pDirtyRects;
#else
    UINT                                 NumFilters;
    void*                                pFilters;
#endif
    D3DDDI_MULTIPLANE_OVERLAY_VIDEO_FRAME_FORMAT VideoFrameFormat;
    UINT                                 YCbCrFlags; // D3DDDI_MULTIPLANE_OVERLAY_YCbCr_FLAGS
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // MP
    D3DDDI_MULTIPLANE_OVERLAY_STRETCH_QUALITY StretchQuality;
#endif
} D3DDDI_MULTIPLANE_OVERLAY_ATTRIBUTES;

typedef struct D3DDDI_CHECK_MULTIPLANE_OVERLAY_SUPPORT_PLANE_INFO
{
    HANDLE                                  hResource;
    UINT                                    SubResourceIndex;
    D3DDDI_MULTIPLANE_OVERLAY_ATTRIBUTES    PlaneAttributes;
} D3DDDI_CHECK_MULTIPLANE_OVERLAY_SUPPORT_PLANE_INFO;

typedef struct _D3DDDIARG_CHECKMULTIPLANEOVERLAYSUPPORT {
    D3DDDI_VIDEO_PRESENT_SOURCE_ID                      VidPnSourceId; // in:
    UINT                                                NumPlanes;     // in:
    D3DDDI_CHECK_MULTIPLANE_OVERLAY_SUPPORT_PLANE_INFO* pPlanes;       // in:
    BOOL                                                Supported;     // out:
} D3DDDIARG_CHECKMULTIPLANEOVERLAYSUPPORT;

typedef struct _D3DDDI_PRESENT_MULTIPLANE_OVERLAY
{
    UINT                                 LayerIndex;
    BOOL                                 Enabled;
    HANDLE                               hResource;
    UINT                                 SubResourceIndex;
    D3DDDI_MULTIPLANE_OVERLAY_ATTRIBUTES PlaneAttributes;
} D3DDDI_PRESENT_MULTIPLANE_OVERLAY;

typedef struct D3DDDIARG_PRESENTMULTIPLANEOVERLAY
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID       VidPnSourceId;
    D3DDDI_PRESENTFLAGS                  Flags;
    D3DDDI_FLIPINTERVAL_TYPE             FlipInterval;
    UINT                                 PresentPlaneCount;
    D3DDDI_PRESENT_MULTIPLANE_OVERLAY*   pPresentPlanes;
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // M1
    UINT                                 Reserved;
#endif
} D3DDDIARG_PRESENTMULTIPLANEOVERLAY;

#endif // D3D_UMD_INTERFACE_VERSION

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // M1

typedef enum D3DDDI_FLUSH_FLAGS
{
    D3DDDI_TRIM_MEMORY = 0x00000002, // 0x2 is used to align with the 10+ DDI version
} D3DDDI_FLUSH_FLAGS;

typedef enum D3DDDI_COUNTER_TYPE
{
    D3DDDI_COUNTER_TYPE_FLOAT32,
    D3DDDI_COUNTER_TYPE_UINT16,
    D3DDDI_COUNTER_TYPE_UINT32,
    D3DDDI_COUNTER_TYPE_UINT64,
} D3DDDI_COUNTER_TYPE;

typedef struct D3DDDIARG_COUNTER_INFO
{
    D3DDDIQUERYTYPE LastDeviceDependentCounter;
    UINT NumSimultaneousCounters;
} D3DDDIARG_COUNTER_INFO;

typedef struct D3DDDIARG_COPYFLAGS
{
    union
    {
        struct
        {
            UINT    NoOverwrite     : 1;        // 0x00000001
            UINT    Discard         : 1;        // 0x00000002
            UINT    Reserved1       :22;        // 0x00FFFFFC
            UINT    BoxValid        : 1;        // 0x01000000
            UINT    Reserved2       : 7;        // 0xFE000000
        };
        UINT Value;
    };
} D3DDDIARG_COPYFLAGS;

typedef struct D3DDDIARG_UPDATESUBRESOURCEUP
{
    HANDLE                hResource;          // Surface getting updated
    UINT                  SubResourceIndex;   // Index of surface level
    D3DDDIBOX             DstBox;             // Surface dimensions to fill
    CONST VOID*           pSysMemUP;          // Data to copy
    UINT                  RowPitch;
    UINT                  DepthPitch;
    D3DDDIARG_COPYFLAGS   Flags;              // Flags
} D3DDDIARG_UPDATESUBRESOURCEUP;

typedef struct D3DDDIARG_CHECKPRESENTDURATIONSUPPORT {
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;             // in:
    UINT                            DesiredPresentDuration;    // in:
    UINT                            ClosestSmallerDuration;    // out:
    UINT                            ClosestLargerDuration;     // out:
} D3DDDIARG_CHECKPRESENTDURATIONSUPPORT;

typedef struct D3DDDIARG_PRESENTSURFACE
{
    HANDLE hResource;        // in: Source surface
    UINT   SubResourceIndex; // in: Index of surface level
} D3DDDIARG_PRESENTSURFACE;

typedef struct _D3DDDIARG_PRESENT1
{
    CONST D3DDDIARG_PRESENTSURFACE* phSrcResources;      // in: Array of Source Resources
    UINT                            SrcResources;        // in: Number of Source Resources
    HANDLE                          hDstResource;        // in: if non-zero, it's the destination of the present
    UINT                            DstSubResourceIndex; // in: Index of surface level
    D3DDDI_PRESENTFLAGS             Flags;               // in: Presentation flags.
    D3DDDI_FLIPINTERVAL_TYPE        FlipInterval;        // in: Presentation interval (flip only)
    UINT                            Reserved;
    CONST RECT*                     pDirtyRects;         // in: Array of dirty rects
    UINT                            DirtyRects;          // in: Number of dirty rects
#if D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_2
    UINT                            BackBufferMultiplicity; // out: Number of back buffer allocations per resource
#endif
} D3DDDIARG_PRESENT1;

#endif // D3D_UMD_INTERFACE_VERSION

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // MP
#define D3DDDI_SETMARKERMODE_CUSTOMDRIVEREVENTS 0x1

typedef enum D3DDDI_MARKERLOGTYPE
{
    D3DDDIMLT_NONE,
    D3DDDIMLT_PROFILE,
    D3DDDIMLT_FT_PROFILE,
} D3DDDI_MARKERLOGTYPE;
#endif

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0)

typedef struct D3DDDIARG_TRIMRESIDENCYSET {
    D3DDDI_TRIMRESIDENCYSET_FLAGS TrimFlags;
    UINT64                        NumBytesToTrim;
} D3DDDIARG_TRIMRESIDENCYSET;

#endif

//
// GetInfo IDs and structures
//
#define D3DDDIDEVINFOID_VCACHE  4

typedef struct _D3DDDIDEVINFO_VCACHE
{
    UINT    Pattern;            // out: bit pattern, return value must be FOUR_CC('C', 'A', 'C', 'H')
    UINT    OptMethod;          // out: optimization method 0 means longest strips, 1 means vertex cache based
    UINT    CacheSize;          // out: cache size to optimize for (only required if type is 1)
    UINT    MagicNumber;        // out: used to determine when to restart strips (only required if type is 1)
} D3DDDIDEVINFO_VCACHE;

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_2)

typedef struct _D3DDDIARG_SYNCTOKEN
{
    HANDLE hResource;
    HANDLE hSyncToken;
} D3DDDIARG_SYNCTOKEN;

#endif


typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETRENDERSTATE)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_RENDERSTATE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_UPDATEWINFO)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_WINFO*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_VALIDATEDEVICE)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_VALIDATETEXTURESTAGESTATE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETTEXTURESTAGESTATE)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_TEXTURESTAGESTATE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETTEXTURE)(
        _In_ HANDLE hDevice, _In_ UINT, _In_ HANDLE);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETPIXELSHADER)(
        _In_ HANDLE hDevice, _In_ HANDLE);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETPIXELSHADERCONST)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETPIXELSHADERCONST*, _In_ CONST FLOAT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETSTREAMSOURCEUM)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETSTREAMSOURCEUM*, _In_ CONST VOID*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETINDICES)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETINDICES*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETINDICESUM)(
        _In_ HANDLE hDevice, _In_ UINT, _In_ CONST VOID*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DRAWPRIMITIVE)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_DRAWPRIMITIVE*, _In_opt_ CONST UINT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DRAWINDEXEDPRIMITIVE)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_DRAWINDEXEDPRIMITIVE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DRAWRECTPATCH)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_DRAWRECTPATCH*, _In_ CONST D3DDDIRECTPATCH_INFO*, _In_ CONST FLOAT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DRAWTRIPATCH)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_DRAWTRIPATCH*, _In_ CONST D3DDDITRIPATCH_INFO*, _In_ CONST FLOAT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DRAWPRIMITIVE2)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_DRAWPRIMITIVE2*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DRAWINDEXEDPRIMITIVE2)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_DRAWINDEXEDPRIMITIVE2*, _In_ UINT, _In_ CONST VOID*, _In_opt_ CONST UINT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_VOLBLT)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_VOLUMEBLT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_BUFBLT)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_BUFFERBLT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_TEXBLT)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_TEXBLT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_STATESET)(
        _In_ HANDLE hDevice, _In_ D3DDDIARG_STATESET*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETPRIORITY)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETPRIORITY*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CLEAR)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_CLEAR*, _In_ UINT, _In_ CONST RECT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_UPDATEPALETTE)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_UPDATEPALETTE*, _In_ CONST PALETTEENTRY*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETPALETTE)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETPALETTE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETVERTEXSHADERCONST)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETVERTEXSHADERCONST*, _In_ CONST VOID*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_MULTIPLYTRANSFORM)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_MULTIPLYTRANSFORM*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETTRANSFORM)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETTRANSFORM*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETVIEWPORT)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_VIEWPORTINFO*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETZRANGE)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_ZRANGE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETMATERIAL)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETMATERIAL*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETLIGHT)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETLIGHT*, _In_ CONST D3DDDI_LIGHT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CREATELIGHT)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_CREATELIGHT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DESTROYLIGHT)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_DESTROYLIGHT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETCLIPPLANE)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETCLIPPLANE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_GETINFO)(
        _In_ HANDLE hDevice, _In_ UINT, _Out_writes_bytes_(DevInfoSize)VOID*, _In_ UINT DevInfoSize);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_LOCK)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_LOCK*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_UNLOCK)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_UNLOCK*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_LOCKASYNC)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_LOCKASYNC*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_UNLOCKASYNC)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_UNLOCKASYNC*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_RENAME)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_RENAME*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CREATERESOURCE)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CREATERESOURCE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DESTROYRESOURCE)(
        _In_ HANDLE hDevice, _In_ HANDLE);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETDISPLAYMODE)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETDISPLAYMODE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_PRESENT)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_PRESENT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_FLUSH)(
        _In_ HANDLE hDevice);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CREATEVERTEXSHADERDECL)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CREATEVERTEXSHADERDECL*, _In_ CONST D3DDDIVERTEXELEMENT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETVERTEXSHADERDECL)(
        _In_ HANDLE hDevice, _In_ HANDLE);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DELETEVERTEXSHADERDECL)(
        _In_ HANDLE hDevice, _In_ HANDLE);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CREATEVERTEXSHADERFUNC)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CREATEVERTEXSHADERFUNC*, _In_ CONST UINT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETVERTEXSHADERFUNC)(
        _In_ HANDLE hDevice, _In_ HANDLE);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DELETEVERTEXSHADERFUNC)(
        _In_ HANDLE hDevice, _In_ HANDLE);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETVERTEXSHADERCONSTI)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETVERTEXSHADERCONSTI*, _In_ CONST INT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETVERTEXSHADERCONSTB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETVERTEXSHADERCONSTB*, _In_ CONST BOOL*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETSCISSORRECT)(
        _In_ HANDLE hDevice, _In_ CONST RECT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETSTREAMSOURCE)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETSTREAMSOURCE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETSTREAMSOURCEFREQ)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETSTREAMSOURCEFREQ*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETCONVOLUTIONKERNELMONO)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETCONVOLUTIONKERNELMONO*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_COMPOSERECTS)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_COMPOSERECTS*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_BLT)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_BLT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_COLORFILL)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_COLORFILL*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DEPTHFILL)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_DEPTHFILL*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CREATEQUERY)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CREATEQUERY*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DESTROYQUERY)(
        _In_ HANDLE hDevice, _In_ CONST HANDLE);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_ISSUEQUERY)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_ISSUEQUERY*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_GETQUERYDATA)(
        _In_ HANDLE hDevice, _Inout_ CONST D3DDDIARG_GETQUERYDATA*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETRENDERTARGET)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETRENDERTARGET*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETDEPTHSTENCIL)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETDEPTHSTENCIL*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_GENERATEMIPSUBLEVELS)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_GENERATEMIPSUBLEVELS*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETPIXELSHADERCONSTI)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETPIXELSHADERCONSTI*, _In_ CONST INT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETPIXELSHADERCONSTB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETPIXELSHADERCONSTB*, _In_ CONST BOOL*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CREATEPIXELSHADER)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CREATEPIXELSHADER*, _In_ CONST UINT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DELETEPIXELSHADER)(
        _In_ HANDLE hDevice, _In_ HANDLE);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CREATEDECODEDEVICE)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CREATEDECODEDEVICE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DESTROYDECODEDEVICE)(
        _In_ HANDLE hDevice, _In_ HANDLE);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETDECODERENDERTARGET) (
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETDECODERENDERTARGET*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DECODEBEGINFRAME) (
        _In_ HANDLE hDevice, _In_ D3DDDIARG_DECODEBEGINFRAME*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DECODEENDFRAME) (
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_DECODEENDFRAME*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DECODEEXECUTE)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_DECODEEXECUTE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DECODEEXTENSIONEXECUTE)(
        _In_ HANDLE hDevice, _Inout_ CONST D3DDDIARG_DECODEEXTENSIONEXECUTE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CREATEVIDEOPROCESSDEVICE) (
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CREATEVIDEOPROCESSDEVICE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DESTROYVIDEOPROCESSDEVICE) (
        _In_ HANDLE hDevice, _In_ HANDLE hVideoProcessor);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_VIDEOPROCESSBEGINFRAME) (
        _In_ HANDLE hDevice, _In_ HANDLE hVideoProcess);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_VIDEOPROCESSENDFRAME) (
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_VIDEOPROCESSENDFRAME*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETVIDEOPROCESSRENDERTARGET) (
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETVIDEOPROCESSRENDERTARGET*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_VIDEOPROCESSBLT) (
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_VIDEOPROCESSBLT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CREATEEXTENSIONDEVICE) (
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CREATEEXTENSIONDEVICE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DESTROYEXTENSIONDEVICE) (
        _In_ HANDLE hDevice, _In_ HANDLE hExtension);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_EXTENSIONEXECUTE) (
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_EXTENSIONEXECUTE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DESTROYDEVICE)(
        _In_ HANDLE hDevice);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CREATEOVERLAY) (
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CREATEOVERLAY*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_UPDATEOVERLAY) (
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_UPDATEOVERLAY*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_FLIPOVERLAY) (
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_FLIPOVERLAY*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_GETOVERLAYCOLORCONTROLS) (
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_GETOVERLAYCOLORCONTROLS*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETOVERLAYCOLORCONTROLS) (
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SETOVERLAYCOLORCONTROLS*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DESTROYOVERLAY) (
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_DESTROYOVERLAY*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_QUERYRESOURCERESIDENCY) (
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_QUERYRESOURCERESIDENCY*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_OPENRESOURCE) (
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_OPENRESOURCE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_GETCAPTUREALLOCATIONHANDLE)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_GETCAPTUREALLOCATIONHANDLE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CAPTURETOSYSMEM)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_CAPTURETOSYSMEM*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DXVAHD_CREATEVIDEOPROCESSOR)(
        _In_ HANDLE, _Inout_ D3DDDIARG_DXVAHD_CREATEVIDEOPROCESSOR*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DXVAHD_SETVIDEOPROCESSBLTSTATE)(
        _In_ HANDLE, _In_ CONST D3DDDIARG_DXVAHD_SETVIDEOPROCESSBLTSTATE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DXVAHD_GETVIDEOPROCESSBLTSTATEPRIVATE)(
        _In_ HANDLE, _Inout_ D3DDDIARG_DXVAHD_GETVIDEOPROCESSBLTSTATEPRIVATE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DXVAHD_SETVIDEOPROCESSSTREAMSTATE)(
        _In_ HANDLE, _In_ CONST D3DDDIARG_DXVAHD_SETVIDEOPROCESSSTREAMSTATE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DXVAHD_GETVIDEOPROCESSSTREAMSTATEPRIVATE)(
        _In_ HANDLE, _Inout_ D3DDDIARG_DXVAHD_GETVIDEOPROCESSSTREAMSTATEPRIVATE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DXVAHD_VIDEOPROCESSBLTHD)(
        _In_ HANDLE, _In_ CONST D3DDDIARG_DXVAHD_VIDEOPROCESSBLTHD*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DXVAHD_DESTROYVIDEOPROCESSOR)(
        _In_ HANDLE, _In_ HANDLE);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CREATEAUTHENTICATEDCHANNEL)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CREATEAUTHENTICATEDCHANNEL*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_AUTHENTICATEDCHANNELKEYEXCHANGE)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_AUTHENTICATEDCHANNELKEYEXCHANGE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_QUERYAUTHENTICATEDCHANNEL)(
        _In_ HANDLE hDevice, _Inout_ CONST D3DDDIARG_QUERYAUTHENTICATEDCHANNEL*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CONFIGUREAUTHENICATEDCHANNEL)(
        _In_ HANDLE hDevice, _Inout_ CONST D3DDDIARG_CONFIGUREAUTHENTICATEDCHANNEL*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DESTROYAUTHENTICATEDCHANNEL)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_DESTROYAUTHENTICATEDCHANNEL*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CREATECRYPTOSESSION)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CREATECRYPTOSESSION*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CRYPTOSESSIONKEYEXCHANGE)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CRYPTOSESSIONKEYEXCHANGE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DESTROYCRYPTOSESSION)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_DESTROYCRYPTOSESSION*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_ENCRYPTIONBLT)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_ENCRYPTIONBLT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_GETPITCH)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_GETPITCH*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_STARTSESSIONKEYREFRESH)(
        _In_ HANDLE hDevice, _Inout_ CONST D3DDDIARG_STARTSESSIONKEYREFRESH*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_FINISHSESSIONKEYREFRESH)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_FINISHSESSIONKEYREFRESH*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_GETENCRYPTIONBLTKEY)(
        _In_ HANDLE hDevice, _Inout_ CONST D3DDDIARG_GETENCRYPTIONBLTKEY*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DECRYPTIONBLT)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_DECRYPTIONBLT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_RESOLVESHAREDRESOURCE)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_RESOLVESHAREDRESOURCE*);
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8)
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_VOLBLT1)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_VOLUMEBLT1*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_BUFBLT1)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_BUFFERBLT1*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_TEXBLT1)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_TEXBLT1*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_DISCARD)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_DISCARD*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_OFFERRESOURCES)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_OFFERRESOURCES*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_RECLAIMRESOURCES)(
        _In_ HANDLE hDevice, _Inout_ CONST D3DDDIARG_RECLAIMRESOURCES*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CHECKDIRECTFLIPSUPPORT)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CHECKDIRECTFLIPSUPPORT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CREATERESOURCE2)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CREATERESOURCE2*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CHECKMULTIPLANEOVERLAYSUPPORT)(
        _In_ HANDLE hDevice, _Inout_ D3DDDIARG_CHECKMULTIPLANEOVERLAYSUPPORT*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_PRESENTMULTIPLANEOVERLAY)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_PRESENTMULTIPLANEOVERLAY*);
#endif // D3D_UMD_INTERFACE_VERSION
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // M1
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_FLUSH1)(
        _In_ HANDLE hDevice, UINT /*D3DDDI_FLUSH_FLAGS*/ FlushFlags);
typedef VOID ( APIENTRY* PFND3DDDI_CHECKCOUNTERINFO )(
        _In_ HANDLE hDevice, _Out_ D3DDDIARG_COUNTER_INFO* );
typedef _Check_return_ HRESULT ( APIENTRY* PFND3DDDI_CHECKCOUNTER )(
        _In_ HANDLE hDevice, _In_ D3DDDIQUERYTYPE, _Out_ D3DDDI_COUNTER_TYPE*, _Out_ UINT*,
        _Out_writes_to_opt_(*pNameLength, *pNameLength) LPSTR,
        _Inout_opt_ UINT* pNameLength,
        _Out_writes_to_opt_(*pUnitsLength, *pUnitsLength) LPSTR,
        _Inout_opt_ UINT* pUnitsLength,
        _Out_writes_to_opt_(*pDescriptionLength, *pDescriptionLength) LPSTR,
        _Inout_opt_ UINT* pDescriptionLength);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_UPDATESUBRESOURCEUP)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_UPDATESUBRESOURCEUP*);

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_4)
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_PRESENT1)(
        _In_ HANDLE hDevice, _In_ D3DDDIARG_PRESENT1*);
#else
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_PRESENT1)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_PRESENT1*);
#endif

typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CHECKPRESENTDURATIONSUPPORT)(
        _In_ HANDLE hDevice, _In_ D3DDDIARG_CHECKPRESENTDURATIONSUPPORT*);
#endif // D3D_UMD_INTERFACE_VERSION
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // MP
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETMARKERMODE)(
        _In_ HANDLE hDevice, _In_ D3DDDI_MARKERTYPE Type, /*D3DDDI_SETMARKERMODE*/ UINT Flags );
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_SETMARKER)(
        _In_ HANDLE hDevice);
#endif // D3D_UMD_INTERFACE_VERSION

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0)
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_TRIMRESIDENCYSET)(
        _In_ HANDLE hDevice, _In_ D3DDDIARG_TRIMRESIDENCYSET*);
#endif // D3D_UMD_INTERFACE_VERSION

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_2)
typedef VOID ( APIENTRY* PFND3DDDI_SYNCTOKEN )(
        _In_ HANDLE hDevice, _In_ CONST D3DDDIARG_SYNCTOKEN*);
#endif // D3D_UMD_INTERFACE_VERSION

typedef struct _D3DDDI_DEVICEFUNCS
{
    PFND3DDDI_SETRENDERSTATE                pfnSetRenderState;
    PFND3DDDI_UPDATEWINFO                   pfnUpdateWInfo;
    PFND3DDDI_VALIDATEDEVICE                pfnValidateDevice;
    PFND3DDDI_SETTEXTURESTAGESTATE          pfnSetTextureStageState;
    PFND3DDDI_SETTEXTURE                    pfnSetTexture;
    PFND3DDDI_SETPIXELSHADER                pfnSetPixelShader;
    PFND3DDDI_SETPIXELSHADERCONST           pfnSetPixelShaderConst;
    PFND3DDDI_SETSTREAMSOURCEUM             pfnSetStreamSourceUm;
    PFND3DDDI_SETINDICES                    pfnSetIndices;
    PFND3DDDI_SETINDICESUM                  pfnSetIndicesUm;
    PFND3DDDI_DRAWPRIMITIVE                 pfnDrawPrimitive;
    PFND3DDDI_DRAWINDEXEDPRIMITIVE          pfnDrawIndexedPrimitive;
    PFND3DDDI_DRAWRECTPATCH                 pfnDrawRectPatch;
    PFND3DDDI_DRAWTRIPATCH                  pfnDrawTriPatch;
    PFND3DDDI_DRAWPRIMITIVE2                pfnDrawPrimitive2;
    PFND3DDDI_DRAWINDEXEDPRIMITIVE2         pfnDrawIndexedPrimitive2;
    PFND3DDDI_VOLBLT                        pfnVolBlt;
    PFND3DDDI_BUFBLT                        pfnBufBlt;
    PFND3DDDI_TEXBLT                        pfnTexBlt;
    PFND3DDDI_STATESET                      pfnStateSet;
    PFND3DDDI_SETPRIORITY                   pfnSetPriority;
    PFND3DDDI_CLEAR                         pfnClear;
    PFND3DDDI_UPDATEPALETTE                 pfnUpdatePalette;
    PFND3DDDI_SETPALETTE                    pfnSetPalette;
    PFND3DDDI_SETVERTEXSHADERCONST          pfnSetVertexShaderConst;
    PFND3DDDI_MULTIPLYTRANSFORM             pfnMultiplyTransform;
    PFND3DDDI_SETTRANSFORM                  pfnSetTransform;
    PFND3DDDI_SETVIEWPORT                   pfnSetViewport;
    PFND3DDDI_SETZRANGE                     pfnSetZRange;
    PFND3DDDI_SETMATERIAL                   pfnSetMaterial;
    PFND3DDDI_SETLIGHT                      pfnSetLight;
    PFND3DDDI_CREATELIGHT                   pfnCreateLight;
    PFND3DDDI_DESTROYLIGHT                  pfnDestroyLight;
    PFND3DDDI_SETCLIPPLANE                  pfnSetClipPlane;
    PFND3DDDI_GETINFO                       pfnGetInfo;
    PFND3DDDI_LOCK                          pfnLock;
    PFND3DDDI_UNLOCK                        pfnUnlock;
    PFND3DDDI_CREATERESOURCE                pfnCreateResource;
    PFND3DDDI_DESTROYRESOURCE               pfnDestroyResource;
    PFND3DDDI_SETDISPLAYMODE                pfnSetDisplayMode;
    PFND3DDDI_PRESENT                       pfnPresent;
    PFND3DDDI_FLUSH                         pfnFlush;
    PFND3DDDI_CREATEVERTEXSHADERFUNC        pfnCreateVertexShaderFunc;
    PFND3DDDI_DELETEVERTEXSHADERFUNC        pfnDeleteVertexShaderFunc;
    PFND3DDDI_SETVERTEXSHADERFUNC           pfnSetVertexShaderFunc;
    PFND3DDDI_CREATEVERTEXSHADERDECL        pfnCreateVertexShaderDecl;
    PFND3DDDI_DELETEVERTEXSHADERDECL        pfnDeleteVertexShaderDecl;
    PFND3DDDI_SETVERTEXSHADERDECL           pfnSetVertexShaderDecl;
    PFND3DDDI_SETVERTEXSHADERCONSTI         pfnSetVertexShaderConstI;
    PFND3DDDI_SETVERTEXSHADERCONSTB         pfnSetVertexShaderConstB;
    PFND3DDDI_SETSCISSORRECT                pfnSetScissorRect;
    PFND3DDDI_SETSTREAMSOURCE               pfnSetStreamSource;
    PFND3DDDI_SETSTREAMSOURCEFREQ           pfnSetStreamSourceFreq;
    PFND3DDDI_SETCONVOLUTIONKERNELMONO      pfnSetConvolutionKernelMono;
    PFND3DDDI_COMPOSERECTS                  pfnComposeRects;
    PFND3DDDI_BLT                           pfnBlt;
    PFND3DDDI_COLORFILL                     pfnColorFill;
    PFND3DDDI_DEPTHFILL                     pfnDepthFill;
    PFND3DDDI_CREATEQUERY                   pfnCreateQuery;
    PFND3DDDI_DESTROYQUERY                  pfnDestroyQuery;
    PFND3DDDI_ISSUEQUERY                    pfnIssueQuery;
    PFND3DDDI_GETQUERYDATA                  pfnGetQueryData;
    PFND3DDDI_SETRENDERTARGET               pfnSetRenderTarget;
    PFND3DDDI_SETDEPTHSTENCIL               pfnSetDepthStencil;
    PFND3DDDI_GENERATEMIPSUBLEVELS          pfnGenerateMipSubLevels;
    PFND3DDDI_SETPIXELSHADERCONSTI          pfnSetPixelShaderConstI;
    PFND3DDDI_SETPIXELSHADERCONSTB          pfnSetPixelShaderConstB;
    PFND3DDDI_CREATEPIXELSHADER             pfnCreatePixelShader;
    PFND3DDDI_DELETEPIXELSHADER             pfnDeletePixelShader;
    PFND3DDDI_CREATEDECODEDEVICE            pfnCreateDecodeDevice;
    PFND3DDDI_DESTROYDECODEDEVICE           pfnDestroyDecodeDevice;
    PFND3DDDI_SETDECODERENDERTARGET         pfnSetDecodeRenderTarget;
    PFND3DDDI_DECODEBEGINFRAME              pfnDecodeBeginFrame;
    PFND3DDDI_DECODEENDFRAME                pfnDecodeEndFrame;
    PFND3DDDI_DECODEEXECUTE                 pfnDecodeExecute;
    PFND3DDDI_DECODEEXTENSIONEXECUTE        pfnDecodeExtensionExecute;
    PFND3DDDI_CREATEVIDEOPROCESSDEVICE      pfnCreateVideoProcessDevice;
    PFND3DDDI_DESTROYVIDEOPROCESSDEVICE     pfnDestroyVideoProcessDevice;
    PFND3DDDI_VIDEOPROCESSBEGINFRAME        pfnVideoProcessBeginFrame;
    PFND3DDDI_VIDEOPROCESSENDFRAME          pfnVideoProcessEndFrame;
    PFND3DDDI_SETVIDEOPROCESSRENDERTARGET   pfnSetVideoProcessRenderTarget;
    PFND3DDDI_VIDEOPROCESSBLT               pfnVideoProcessBlt;
    PFND3DDDI_CREATEEXTENSIONDEVICE         pfnCreateExtensionDevice;
    PFND3DDDI_DESTROYEXTENSIONDEVICE        pfnDestroyExtensionDevice;
    PFND3DDDI_EXTENSIONEXECUTE              pfnExtensionExecute;
    PFND3DDDI_CREATEOVERLAY                 pfnCreateOverlay;
    PFND3DDDI_UPDATEOVERLAY                 pfnUpdateOverlay;
    PFND3DDDI_FLIPOVERLAY                   pfnFlipOverlay;
    PFND3DDDI_GETOVERLAYCOLORCONTROLS       pfnGetOverlayColorControls;
    PFND3DDDI_SETOVERLAYCOLORCONTROLS       pfnSetOverlayColorControls;
    PFND3DDDI_DESTROYOVERLAY                pfnDestroyOverlay;
    PFND3DDDI_DESTROYDEVICE                 pfnDestroyDevice;
    PFND3DDDI_QUERYRESOURCERESIDENCY        pfnQueryResourceResidency;
    PFND3DDDI_OPENRESOURCE                  pfnOpenResource;
    PFND3DDDI_GETCAPTUREALLOCATIONHANDLE    pfnGetCaptureAllocationHandle;
    PFND3DDDI_CAPTURETOSYSMEM               pfnCaptureToSysMem;
    PFND3DDDI_LOCKASYNC                     pfnLockAsync;
    PFND3DDDI_UNLOCKASYNC                   pfnUnlockAsync;
    PFND3DDDI_RENAME                        pfnRename;
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN7)
    PFND3DDDI_DXVAHD_CREATEVIDEOPROCESSOR              pfnCreateVideoProcessor;
    PFND3DDDI_DXVAHD_SETVIDEOPROCESSBLTSTATE           pfnSetVideoProcessBltState;
    PFND3DDDI_DXVAHD_GETVIDEOPROCESSBLTSTATEPRIVATE    pfnGetVideoProcessBltStatePrivate;
    PFND3DDDI_DXVAHD_SETVIDEOPROCESSSTREAMSTATE        pfnSetVideoProcessStreamState;
    PFND3DDDI_DXVAHD_GETVIDEOPROCESSSTREAMSTATEPRIVATE pfnGetVideoProcessStreamStatePrivate;
    PFND3DDDI_DXVAHD_VIDEOPROCESSBLTHD                 pfnVideoProcessBltHD;
    PFND3DDDI_DXVAHD_DESTROYVIDEOPROCESSOR             pfnDestroyVideoProcessor;
    PFND3DDDI_CREATEAUTHENTICATEDCHANNEL               pfnCreateAuthenticatedChannel;
    PFND3DDDI_AUTHENTICATEDCHANNELKEYEXCHANGE          pfnAuthenticatedChannelKeyExchange;
    PFND3DDDI_QUERYAUTHENTICATEDCHANNEL                pfnQueryAuthenticatedChannel;
    PFND3DDDI_CONFIGUREAUTHENICATEDCHANNEL             pfnConfigureAuthenticatedChannel;
    PFND3DDDI_DESTROYAUTHENTICATEDCHANNEL              pfnDestroyAuthenticatedChannel;
    PFND3DDDI_CREATECRYPTOSESSION                      pfnCreateCryptoSession;
    PFND3DDDI_CRYPTOSESSIONKEYEXCHANGE                 pfnCryptoSessionKeyExchange;
    PFND3DDDI_DESTROYCRYPTOSESSION                     pfnDestroyCryptoSession;
    PFND3DDDI_ENCRYPTIONBLT                            pfnEncryptionBlt;
    PFND3DDDI_GETPITCH                                 pfnGetPitch;
    PFND3DDDI_STARTSESSIONKEYREFRESH                   pfnStartSessionKeyRefresh;
    PFND3DDDI_FINISHSESSIONKEYREFRESH                  pfnFinishSessionKeyRefresh;
    PFND3DDDI_GETENCRYPTIONBLTKEY                      pfnGetEncryptionBltKey;
    PFND3DDDI_DECRYPTIONBLT                            pfnDecryptionBlt;
    PFND3DDDI_RESOLVESHAREDRESOURCE                    pfnResolveSharedResource;
#endif // D3D_UMD_INTERFACE_VERSION
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8)
    PFND3DDDI_VOLBLT1                                  pfnVolBlt1;
    PFND3DDDI_BUFBLT1                                  pfnBufBlt1;
    PFND3DDDI_TEXBLT1                                  pfnTexBlt1;
    PFND3DDDI_DISCARD                                  pfnDiscard;
    PFND3DDDI_OFFERRESOURCES                           pfnOfferResources;
    PFND3DDDI_RECLAIMRESOURCES                         pfnReclaimResources;
    PFND3DDDI_CHECKDIRECTFLIPSUPPORT                   pfnCheckDirectFlipSupport;
    PFND3DDDI_CREATERESOURCE2                          pfnCreateResource2;
    PFND3DDDI_CHECKMULTIPLANEOVERLAYSUPPORT            pfnCheckMultiPlaneOverlaySupport;
    PFND3DDDI_PRESENTMULTIPLANEOVERLAY                 pfnPresentMultiPlaneOverlay;
#endif // D3D_UMD_INTERFACE_VERSION
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // M1
    void*                                              pfnReserved1;
    PFND3DDDI_FLUSH1                                   pfnFlush1;
    PFND3DDDI_CHECKCOUNTERINFO                         pfnCheckCounterInfo;
    PFND3DDDI_CHECKCOUNTER                             pfnCheckCounter;
    PFND3DDDI_UPDATESUBRESOURCEUP                      pfnUpdateSubresourceUP;
    PFND3DDDI_PRESENT1                                 pfnPresent1;
    PFND3DDDI_CHECKPRESENTDURATIONSUPPORT              pfnCheckPresentDurationSupport;
#endif
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // MP
    PFND3DDDI_SETMARKER                                pfnSetMarker;
    PFND3DDDI_SETMARKERMODE                            pfnSetMarkerMode;
#endif
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0)
    PFND3DDDI_TRIMRESIDENCYSET                         pfnTrimResidencySet;
#endif
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_2)
    PFND3DDDI_SYNCTOKEN                                pfnAcquireResource;
    PFND3DDDI_SYNCTOKEN                                pfnReleaseResource;
#endif
} D3DDDI_DEVICEFUNCS;

typedef struct _D3DDDICB_QUERYADAPTERINFO
{
    VOID*  pPrivateDriverData;
    UINT   PrivateDriverDataSize;
} D3DDDICB_QUERYADAPTERINFO;

typedef struct _D3DDDICB_GETMULTISAMPLEMETHODLIST
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;  // in: adapter's VidPN source ID
    UINT                            Width;          // in:
    UINT                            Height;         // in:
    D3DDDIFORMAT                    Format;         // in:
    D3DDDI_MULTISAMPLINGMETHOD*     pMethodList;    // out:
    UINT                            MethodCount;    // in/out:
} D3DDDICB_GETMULTISAMPLEMETHODLIST;

typedef struct _D3DDDICB_ALLOCATE
{
    CONST VOID*     pPrivateDriverData;
    UINT            PrivateDriverDataSize;
    HANDLE          hResource;              //in:  D3D runtime resource handle
    D3DKMT_HANDLE   hKMResource;            //out: kernel resource handle
    UINT            NumAllocations;         //in:  number of allocation handles
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN7)
    union {
        D3DDDI_ALLOCATIONINFO*  pAllocationInfo;  //in:  arrary of allocation structs : WDDM v1
        D3DDDI_ALLOCATIONINFO2* pAllocationInfo2; //in:  arrary of allocation structs : WDDM v2 _ADVSCH_
    };
#else
    D3DDDI_ALLOCATIONINFO* pAllocationInfo; //in:  arrary of allocation structs
#endif // D3D_UMD_INTERFACE_VERSION
} D3DDDICB_ALLOCATE;

typedef struct _D3DDDICB_DEALLOCATE
{
    HANDLE                  hResource;          //in: D3D runtime resource handle
    UINT                    NumAllocations;     //in: number of allocation handles,ignored if hResource != NULL
    CONST D3DKMT_HANDLE*  HandleList; //in: arrary of allocation handles,ignored if hResource != NULL
} D3DDDICB_DEALLOCATE;

typedef struct _D3DDDICB_SETPRIORITY
{
    HANDLE                  hResource;          //in: D3D runtime resource handle
    UINT                    NumAllocations;     //in: number of allocation handles,ignored if hResource != NULL
    CONST D3DKMT_HANDLE*    HandleList;         //in: arrray of allocation handles,ignored if hResource != NULL
    CONST UINT*             pPriorities;            //in: priority to set.
} D3DDDICB_SETPRIORITY;

typedef enum _D3DDDI_RESIDENCYSTATUS
{
    D3DDDI_RESIDENCYSTATUS_RESIDENTINGPUMEMORY=1,
    D3DDDI_RESIDENCYSTATUS_RESIDENTINSHAREDMEMORY=2,
    D3DDDI_RESIDENCYSTATUS_NOTRESIDENT=3,
} D3DDDI_RESIDENCYSTATUS;

typedef struct _D3DDDICB_QUERYRESIDENCY
{
    HANDLE                              hResource;          //in: D3D runtime resource handle
    UINT                                NumAllocations;     //in: number of allocation handles,ignored if hResource != NULL
    CONST D3DKMT_HANDLE*                HandleList;         //in: arrray of allocation handles,ignored if hResource != NULL
    D3DDDI_RESIDENCYSTATUS*             pResidencyStatus;   //out: Residency status of the specified allocation.
} D3DDDICB_QUERYRESIDENCY;

typedef struct _D3DDDICB_LOCK
{
    D3DKMT_HANDLE       hAllocation;        // in: allocation to lock, used by LockCb from driver.
                                            // out: New handle representing the allocation.
    UINT                PrivateDriverData;  // in: from UMD to KMD's AcquireAperture
    UINT                NumPages;
    CONST UINT*         pPages;
    VOID*               pData;              // out: pointer to memory
    D3DDDICB_LOCKFLAGS  Flags;
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN7)
    D3DGPU_VIRTUAL_ADDRESS GpuVirtualAddress; // out: GPU Virtual address of the allocation locked. _ADVSCH_
#endif // D3D_UMD_INTERFACE_VERSION
} D3DDDICB_LOCK;

typedef struct _D3DDDICB_UNLOCK
{
    UINT                    NumAllocations;
    CONST D3DKMT_HANDLE*    phAllocations;
} D3DDDICB_UNLOCK;

typedef struct _D3DDDICB_SETDISPLAYMODE
{
    D3DKMT_HANDLE               hPrimaryAllocation;             // in: The primary allocation for scanning
    UINT                        PrivateDriverFormatAttribute;   // out: Private Format Attribute of the current primary surface if SetDsiplayModeCb returns STATUS_GRAPHICS_INCOMPATIBLE_PRIVATE_FORMAT
} D3DDDICB_SETDISPLAYMODE;

typedef struct _D3DDDICB_SETDISPLAYPRIVATEDRIVERFORMAT
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;                  // in: Identifies which VidPn we are changing the private driver format attribute of
    UINT                            PrivateDriverFormatAttribute;   // in: Private Format Attribute to set for the given VidPn
} D3DDDICB_SETDISPLAYPRIVATEDRIVERFORMAT;

typedef struct _D3DDDICB_PRESENT
{
    D3DKMT_HANDLE               hSrcAllocation;                                 // in: The allocation of which content will be presented
    D3DKMT_HANDLE               hDstAllocation;                                 // in: if non-zero, it's the destination allocation of the present
    HANDLE                      hContext;                                       // in: Context being submitted to.
    UINT                        BroadcastContextCount;                          // in: Specifies the number of context
                                                                                //     to broadcast this present operation to.
                                                                                //     Only supported for flip operation.
    HANDLE                      BroadcastContext[D3DDDI_MAX_BROADCAST_CONTEXT]; // in: Specifies the handle of the context to
                                                                                //     broadcast to.
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0)
    _Field_size_(BroadcastContextCount)
    D3DKMT_HANDLE*              BroadcastSrcAllocation;                         // in: LDA
    _Field_size_opt_(BroadcastContextCount)
    D3DKMT_HANDLE*              BroadcastDstAllocation;                         // in: LDA
    UINT                        PrivateDriverDataSize;                          // in:
    _Field_size_bytes_(PrivateDriverDataSize)
    PVOID                       pPrivateDriverData;                             // in: Private driver data to pass to DdiPresent and DdiSetVidPnSourceAddress
    BOOLEAN                     bOptimizeForComposition;                        // out: DWM is involved in composition
#endif // (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0)

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_2_2)
    BOOL                        SyncIntervalOverrideValid;                      // in: Override app sync interval
    D3DDDI_FLIPINTERVAL_TYPE    SyncIntervalOverride;                           // in: Override app sync interval
#endif // (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_2_2)
} D3DDDICB_PRESENT;

typedef struct _D3DDDICB_RENDERFLAGS
{
    union
    {
        struct
        {
            UINT    ResizeCommandBuffer      : 1;     // 0x00000001
            UINT    ResizeAllocationList     : 1;     // 0x00000002
            UINT    ResizePatchLocationList  : 1;     // 0x00000004
            UINT    NullRendering            : 1;     // 0x00000008
            UINT    Reserved                 :28;     // 0xFFFFFFF0
        };
        UINT Value;
    };
} D3DDDICB_RENDERFLAGS;

typedef struct _D3DDDICB_RENDER
{
    UINT                        CommandLength;              // in:  Number of byte to process in the command buffer.
    UINT                        CommandOffset;              // in:  Offset of the command to be process in the
                                                            //      current command buffer.
    UINT                        NumAllocations;             // in: Number of allocation in allocations list.
    UINT                        NumPatchLocations;          // in: Number of patch location in patch allocations list.
    VOID*                       pNewCommandBuffer;          // out: Pointer to the next command buffer to use.
    UINT                        NewCommandBufferSize;       // in: Size requested for the next command buffer.
                                                            // out: Size of the next command buffer to use.
    D3DDDI_ALLOCATIONLIST*      pNewAllocationList;         // out: Pointer to the next allocation list to use.
    UINT                        NewAllocationListSize;      // in: Size requested for the next allocation list.
                                                            // out: Size of the new allocation list.
    D3DDDI_PATCHLOCATIONLIST*   pNewPatchLocationList;      // out: Pointer to the next patch location list.
    UINT                        NewPatchLocationListSize;   // in: Size requested for the next patch location list.
                                                            // out: Size of the new patch location list.
    D3DDDICB_RENDERFLAGS Flags;                             // in: Flags
    HANDLE                      hContext;                   // in: Context being submitted to and owning the specified command buffer.
    UINT                        BroadcastContextCount;                          // in: Specifies the number of context
                                                                                //     to broadcast this command buffer to.
    HANDLE                      BroadcastContext[D3DDDI_MAX_BROADCAST_CONTEXT]; // in: Specifies the handle of the context to
                                                                                //     broadcast to.
    ULONG                       QueuedBufferCount;          // out: Number of DMA buffer queued to this context after this submission.
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN7)
    D3DGPU_VIRTUAL_ADDRESS      NewCommandBuffer;           // out: GPU Virtual address to the next command buffer to use. _ADVSCH_
    VOID*                       pPrivateDriverData;         // in: Pointer to driver private data. _ADVSCH_
    UINT                        PrivateDriverDataSize;      // in: Size of the driver private data. _ADVSCH_
#endif // D3D_UMD_INTERFACE_VERSION
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // MP
    D3DDDI_MARKERLOGTYPE        MarkerLogType;              // in
    UINT                        RenderCBSequence;           // in
    UINT                        FirstAPISequenceNumberHigh; // in
    UINT                        CompletedAPISequenceNumberLow0Size; // in
    UINT                        CompletedAPISequenceNumberLow1Size; // in
    UINT                        BegunAPISequenceNumberLow0Size; // in
    UINT                        BegunAPISequenceNumberLow1Size; // in
    CONST UINT*                 pCompletedAPISequenceNumberLow0; // in
    CONST UINT*                 pCompletedAPISequenceNumberLow1; // in
    CONST UINT*                 pBegunAPISequenceNumberLow0; // in
    CONST UINT*                 pBegunAPISequenceNumberLow1; // in
#endif
} D3DDDICB_RENDER;

typedef struct _D3DDDICB_ESCAPE
{
    HANDLE             hDevice;                // in: optional device
    D3DDDI_ESCAPEFLAGS Flags;                  // in: flags
    VOID*              pPrivateDriverData;     // in/out: data to/from KMD
    UINT               PrivateDriverDataSize;  // in: size of the data
    HANDLE             hContext;               // in: optional context
} D3DDDICB_ESCAPE;

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3)
typedef enum _D3DDDI_DEVICEEXECUTION_STATE
{
    D3DDDI_DEVICEEXECUTION_ACTIVE               = 1,
    D3DDDI_DEVICEEXECUTION_RESET                = 2,
    D3DDDI_DEVICEEXECUTION_HUNG                 = 3,
    D3DDDI_DEVICEEXECUTION_STOPPED              = 4,
    D3DDDI_DEVICEEXECUTION_ERROR_OUTOFMEMORY    = 5,
    D3DDDI_DEVICEEXECUTION_ERROR_DMAFAULT       = 6,
} D3DDDI_DEVICEEXECUTION_STATE;

typedef struct _D3DDDI_EXECUTIONSTATEESCAPE
{
    D3DDDI_DEVICEEXECUTION_STATE State;        // out: state of the device
} D3DDDI_EXECUTIONSTATEESCAPE;

typedef struct _D3DDDI_FRAMELATENCYESCAPE
{
    UINT RequestedLatency;                     // in: driver-requested frame latency
} D3DDDI_FRAMELATENCYESCAPE;
#endif // D3D_UMD_INTERFACE_VERSION

typedef struct _D3DDDICB_CREATEOVERLAY
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;      // in
    D3DDDI_KERNELOVERLAYINFO        OverlayInfo;       // in
    D3DKMT_HANDLE                   hKernelOverlay;    // out: Kernel overlay handle
} D3DDDICB_CREATEOVERLAY;

typedef struct _D3DDDICB_UPDATEOVERLAY
{
    D3DKMT_HANDLE            hKernelOverlay;    // in: Kernel overlay handle
    D3DDDI_KERNELOVERLAYINFO OverlayInfo;       // in
} D3DDDICB_UPDATEOVERLAY;

typedef struct _D3DDDICB_FLIPOVERLAY
{
    D3DKMT_HANDLE        hKernelOverlay;        // in: Kernel overlay handle
    D3DKMT_HANDLE        hSource;               // in: Allocation currently displayed
    VOID*                pPrivateDriverData;    // in: Private driver data
    UINT                 PrivateDriverDataSize; // in: Size of private driver data
} D3DDDICB_FLIPOVERLAY;

typedef struct _D3DDDICB_DESTROYOVERLAY
{
    D3DKMT_HANDLE        hKernelOverlay;        // in: Kernel overlay handle
} D3DDDICB_DESTROYOVERLAY;

typedef struct _D3DDDICB_CREATECONTEXT
{
    UINT                        NodeOrdinal;                    // in:  Specify the node ordinal this context is targeted to.
    UINT                        EngineAffinity;                 // in:  Specify the engine affinity within the node.
    D3DDDI_CREATECONTEXTFLAGS   Flags;                          // in:  Context creation flags.
    VOID*                       pPrivateDriverData;             // in:  Pointer to private driver data
    UINT                        PrivateDriverDataSize;          // in:  Size of private driver data
    HANDLE                      hContext;                       // out: Handle to the created context.
    VOID*                       pCommandBuffer;                 // out: Pointer to the first command buffer for this context.
    UINT                        CommandBufferSize;              // out: Command buffer size (bytes).
    D3DDDI_ALLOCATIONLIST*      pAllocationList;                // out: Pointer to the first allocation list for this context.
    UINT                        AllocationListSize;             // out: Allocation list size (elements).
    D3DDDI_PATCHLOCATIONLIST*   pPatchLocationList;             // out: Pointer to the first patch location list for this device.
    UINT                        PatchLocationListSize;          // out: Patch location list size (elements).
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN7)
    D3DGPU_VIRTUAL_ADDRESS      CommandBuffer;                  // out: GPU Virtual address to the command buffer to use. _ADVSCH_
#endif // D3D_UMD_INTERFACE_VERSION
} D3DDDICB_CREATECONTEXT;

typedef struct _D3DDDICB_DESTROYCONTEXT
{
    HANDLE          hContext;                       // in: Handle to the context to destroy.
} D3DDDICB_DESTROYCONTEXT;

typedef struct _D3DDDICB_CREATESYNCHRONIZATIONOBJECT
{
    D3DDDI_SYNCHRONIZATIONOBJECTINFO        Info;           // in:  Attributes of the synchronization object to create.
    D3DKMT_HANDLE                           hSyncObject;    // out: Handle to the created object.
} D3DDDICB_CREATESYNCHRONIZATIONOBJECT;

typedef struct _D3DDDICB_DESTROYSYNCHRONIZATIONOBJECT
{
    D3DKMT_HANDLE          hSyncObject;                     // in: Handle to the synchronization object to destroy.
} D3DDDICB_DESTROYSYNCHRONIZATIONOBJECT;

typedef struct _D3DDDICB_WAITFORSYNCHRONIZATIONOBJECT
{
    HANDLE                 hContext;                                        // in: Specify the context that should be waiting.
    UINT                   ObjectCount;                                     // in: Number of object to wait on.
    D3DKMT_HANDLE          ObjectHandleArray[D3DDDI_MAX_OBJECT_WAITED_ON];  // in: Handle to synchronization objects to wait on.
} D3DDDICB_WAITFORSYNCHRONIZATIONOBJECT;

typedef struct _D3DDDICB_SIGNALSYNCHRONIZATIONOBJECT
{
    HANDLE                  hContext;                                       // in: Specify the context that should signal the objets.
    UINT                    ObjectCount;                                    // in: Number of object to signal.
    D3DKMT_HANDLE           ObjectHandleArray[D3DDDI_MAX_OBJECT_SIGNALED];  // in: Handle to the synchronization object to signal.
    D3DDDICB_SIGNALFLAGS    Flags;                                          // in: Specify signal behavior.
} D3DDDICB_SIGNALSYNCHRONIZATIONOBJECT;

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8)
typedef struct _D3DDDICB_OFFERALLOCATIONS
{
    _In_reads_(NumAllocations) CONST HANDLE*        pResources;            // in: array of D3D runtime resource handles.
    _In_reads_(NumAllocations) CONST D3DKMT_HANDLE* HandleList;            // in: array of allocation handles. If non-NULL, pResources must be NULL.
    _In_ UINT                                       NumAllocations;        // in: number of items in whichever of pResources or HandleList is non-NULL.
    _In_ D3DDDI_OFFER_PRIORITY                      Priority;              // in: priority with which to offer the allocations
} D3DDDICB_OFFERALLOCATIONS;

typedef struct _D3DDDICB_RECLAIMALLOCATIONS
{
    _In_reads_(NumAllocations) CONST HANDLE*        pResources;            // in:  array of D3D runtime resource handles.
    _In_reads_(NumAllocations) CONST D3DKMT_HANDLE* HandleList;            // in:  array of allocation handles. If non-NULL, pResources must be NULL.
    _Out_writes_all_opt_(NumAllocations) BOOL*      pDiscarded;            // out: optional array of booleans specifying whether each resource or allocation was discarded.
    _In_ UINT                                       NumAllocations;        // in:  number of items in pDiscarded and whichever of pResources or HandleList is non-NULL.
} D3DDDICB_RECLAIMALLOCATIONS;

typedef struct _D3DDDICB_RECLAIMALLOCATIONS2
{
    _In_ D3DKMT_HANDLE                              PagingQueue;           // in:  The paging queue, supplied by the UMD, to page in the allocation list
    _In_ UINT                                       NumAllocations;        // in:  number of items in pDiscarded and whichever of pResources or HandleList is non-NULL.
    _In_reads_(NumAllocations) CONST HANDLE*        pResources;            // in:  array of D3D runtime resource handles.
    _In_reads_(NumAllocations) CONST D3DKMT_HANDLE* HandleList;            // in:  array of allocation handles.
    _Out_writes_all_opt_(NumAllocations) BOOL*      pDiscarded;            // out: optional array of booleans specifying whether each resource or allocation was discarded.
    _Out_ UINT64                                    PagingFenceValue;      // out: The paging fence to synchronize against before submitting work to the GPU which
                                                                           //      references any of the resources or allocations in the provided arrays
} D3DDDICB_RECLAIMALLOCATIONS2;

typedef struct _D3D12DDICB_OFFERALLOCATIONS
{
    _In_ UINT                                       NumAllocations;        // in:  number of items in pDiscarded and whichever of pResources or HandleList is non-NULL.
    _In_reads_(NumAllocations) CONST D3DKMT_HANDLE* HandleList;            // in:  array of allocation handles.
    _In_ D3DDDI_OFFER_PRIORITY                      Priority;              // in:  priority with which to offer the allocations
} D3D12DDICB_OFFERALLOCATIONS;

typedef struct _D3D12DDICB_RECLAIMALLOCATIONS2
{
    _In_ UINT                                       NumAllocations;        // in:  number of items in pDiscarded and whichever of pResources or HandleList is non-NULL.
    _In_reads_(NumAllocations) CONST D3DKMT_HANDLE* HandleList;            // in:  array of allocation handles. If non-NULL, pResources must be NULL.
    _Out_writes_all_opt_(NumAllocations) BOOL*      pDiscarded;            // out: optional array of booleans specifying whether each resource or allocation was discarded.
    _Out_ UINT64                                    PagingFenceValue;      // out: The paging fence to synchronize against before submitting work to the GPU which
                                                                           //      references any of the resources or allocations in the provided arrays
} D3D12DDICB_RECLAIMALLOCATIONS2;

typedef struct _D3DDDICB_CREATESYNCHRONIZATIONOBJECT2
{
    D3DDDI_SYNCHRONIZATIONOBJECTINFO2       Info;           // in/out:  Attributes of the synchronization object to create.
    D3DKMT_HANDLE                           hSyncObject;    // out:     Handle to the created object.
} D3DDDICB_CREATESYNCHRONIZATIONOBJECT2;

typedef struct _D3DDDICB_WAITFORSYNCHRONIZATIONOBJECT2
{
    HANDLE                 hContext;                                        // in: Specify the context that should be waiting.
    UINT                   ObjectCount;                                     // in: Number of object to wait on.
    D3DKMT_HANDLE          ObjectHandleArray[D3DDDI_MAX_OBJECT_WAITED_ON];  // in: Handle to synchronization objects to wait on.
    UINT64                 FenceValue;                                      // in: fence value to be waited.
} D3DDDICB_WAITFORSYNCHRONIZATIONOBJECT2;

typedef struct _D3DDDICB_SIGNALSYNCHRONIZATIONOBJECT2
{
    HANDLE                  hContext;                                       // in: Specify the context that should signal the objets.
    UINT                    ObjectCount;                                    // in: Number of object to signal.
    D3DKMT_HANDLE           ObjectHandleArray[D3DDDI_MAX_OBJECT_SIGNALED];  // in: Handle to the synchronization object to signal.
    D3DDDICB_SIGNALFLAGS    Flags;                                          // in: Specify signal behavior.
    ULONG                   BroadcastContextCount;                          // in: Specifies the number of context to broadcast this signal buffer to.
    HANDLE                  BroadcastContext[D3DDDI_MAX_BROADCAST_CONTEXT]; // in: Specifies the handle of the context to broadcast to.
    union
    {
        UINT64              FenceValue;                                     // in: fence value to be signaled;
        HANDLE              CpuEventHandle;                                 // in: handle of a CPU event to be signaled
    };
} D3DDDICB_SIGNALSYNCHRONIZATIONOBJECT2;

typedef struct D3DDDI_MULTIPLANE_OVERLAY_ALLOCATION_INFO
{
    D3DKMT_HANDLE PresentAllocation;
    UINT SubResourceIndex;
} D3DDDI_MULTIPLANE_ALLOCATION_INFO;

typedef struct D3DDDICB_PRESENTMULTIPLANEOVERLAY
{
    HANDLE          hContext;
    UINT            BroadcastContextCount;
    HANDLE          BroadcastContext[D3DDDI_MAX_BROADCAST_CONTEXT];
    UINT            AllocationInfoCount;
    D3DDDI_MULTIPLANE_ALLOCATION_INFO AllocationInfo[D3DDDI_MAX_MULTIPLANE_OVERLAY_ALLOCATIONS];
} D3DDDICB_PRESENTMULTIPLANEOVERLAY;

#endif // (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8)

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // MP
typedef struct D3DDDICB_LOGUMDMARKER
{
    HANDLE hContext;    
    UINT64 APISequenceNumber;
    INT Index;
    INT StringIndex;
    LPCWSTR Info;
} D3DDDICB_LOGUMDMARKER; 
#endif // D3D_UMD_INTERFACE_VERSION

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0)

typedef struct D3DDDICB_EVICT
{
    UINT                        NumAllocations;     // in: number of allocation handles
    CONST D3DKMT_HANDLE*        AllocationList;     // in: an array of NumAllocations allocation handles
    D3DDDI_EVICT_FLAGS          Flags;              // in: eviction flags
    UINT64                      NumBytesToTrim;     // out: This value indicates how much to trim in order to satisfy the new budget.
} D3DDDICB_EVICT;

typedef struct D3DDDICB_WAITFORSYNCHRONIZATIONOBJECTFROMCPU
{
    UINT                    ObjectCount;                    // in: Number of objects to wait on.
    _Field_size_(ObjectCount)
    const D3DKMT_HANDLE*    ObjectHandleArray;              // in: Handle to monitored fence synchronization objects to wait on.
    _Field_size_(ObjectCount)
    const UINT64*           FenceValueArray;                // in: Fence values to be waited on.
    HANDLE                  hAsyncEvent;                    // in: Event to be signaled when the wait condition is satisfied.
                                                            // When set to NULL, the call will not return until the wait condition is satisfied.
    D3DDDI_WAITFORSYNCHRONIZATIONOBJECTFROMCPU_FLAGS Flags; // in: Flags that specify the wait mode.
} D3DDDICB_WAITFORSYNCHRONIZATIONOBJECTFROMCPU;

typedef struct D3DDDICB_SIGNALSYNCHRONIZATIONOBJECTFROMCPU
{
    UINT                    ObjectCount;        // in: Number of objects to signal.
    _Field_size_(ObjectCount)
    const D3DKMT_HANDLE*    ObjectHandleArray;  // in: Handle to monitored fence synchronization objects to signal.
    _Field_size_(ObjectCount)
    const UINT64*           FenceValueArray;    // in: Fence values to be signaled.
} D3DDDICB_SIGNALSYNCHRONIZATIONOBJECTFROMCPU;

typedef struct D3DDDICB_WAITFORSYNCHRONIZATIONOBJECTFROMGPU
{
    HANDLE                  hContext;                   // in: Specify the context that should be waiting.
    UINT                    ObjectCount;                // in: Number of object to wait on.

    _Field_size_(ObjectCount)
    const D3DKMT_HANDLE*    ObjectHandleArray;          // in: Handles to synchronization objects to wait on.

    union
    {
        _Field_size_(ObjectCount)
        const UINT64*       MonitoredFenceValueArray;   // in: monitored fence values to be waited.

        UINT64              FenceValue;                 // in: fence value to be waited.

        UINT64 Reserved[8];
    };
} D3DDDICB_WAITFORSYNCHRONIZATIONOBJECTFROMGPU;

typedef struct D3DDDICB_SIGNALSYNCHRONIZATIONOBJECTFROMGPU
{
    HANDLE                  hContext;           // in: Identifies the context that the signal is being submitted to.
    UINT                    ObjectCount;        // in: Specifies the number of objects to signal.

    _Field_size_(ObjectCount)
    const D3DKMT_HANDLE*    ObjectHandleArray;  // in: Specifies the objects to signal.

    union
    {
        _Field_size_(ObjectCount)
        const UINT64*   MonitoredFenceValueArray; // in: monitored fence values to be signaled

        UINT64 Reserved[8];
    };
} D3DDDICB_SIGNALSYNCHRONIZATIONOBJECTFROMGPU;

typedef struct D3DDDICB_SIGNALSYNCHRONIZATIONOBJECTFROMGPU2
{
    UINT                    ObjectCount;            // in: Specifies the number of objects to signal.

    _Field_size_(ObjectCount)
    const D3DKMT_HANDLE*    ObjectHandleArray;      // in: Specifies the objects to signal.

    D3DDDICB_SIGNALFLAGS    Flags;                  // in: Specifies signal behavior.

    ULONG                   BroadcastContextCount;  // in: Specifies the number of contexts to broadcast this signal to.

    _Field_size_(BroadcastContextCount)
    const HANDLE*           BroadcastContextArray;  // in: Specifies context handles to broadcast to.

    union
    {
        UINT64              FenceValue;             // in: fence value to be signaled

        HANDLE              CpuEventHandle;         // in: handle of a CPU event to be signaled if Flags.EnqueueCpuEvent flag is set.

        _Field_size_(ObjectCount)
        const UINT64*       MonitoredFenceValueArray; // in: monitored fence values to be signaled

        UINT64              Reserved[8];
    };
} D3DDDICB_SIGNALSYNCHRONIZATIONOBJECTFROMGPU2;

typedef struct D3DDDICB_CREATEPAGINGQUEUE
{
    D3DDDI_PAGINGQUEUE_PRIORITY Priority;                       // in: scheduling priority relative to other paging queues on this device
    D3DKMT_HANDLE               hPagingQueue;                   // out: handle to the paging queue used to synchronize paging operations for this device.
    D3DKMT_HANDLE               hSyncObject;                    // out: handle to the monitored fence object used to synchronize paging operations for this paging queue.
    VOID*                       FenceValueCPUVirtualAddress;    // out: Read-only mapping of the fence value for the CPU
    UINT                        PhysicalAdapterIndex;           // in: Physical adapter index (engine ordinal) for the queue.
} D3DDDICB_CREATEPAGINGQUEUE;

typedef struct _D3DDDICB_LOCK2
{
    D3DKMT_HANDLE       hAllocation;        // in: Allocation to lock, used by Lock2Cb from driver.
    D3DDDICB_LOCK2FLAGS Flags;
    PVOID               pData;              // out: Virtual address of the locked allocation
} D3DDDICB_LOCK2;

typedef struct _D3DDDICB_UNLOCK2
{
    D3DKMT_HANDLE       hAllocation;        // in: Allocation to unlock
} D3DDDICB_UNLOCK2;

typedef struct _D3DDDICB_INVALIDATECACHE
{
    D3DKMT_HANDLE   hAllocation;
    SIZE_T          Offset;
    SIZE_T          Length;
} D3DDDICB_INVALIDATECACHE;

typedef struct _D3DDDICB_FREEGPUVIRTUALADDRESS
{
    D3DGPU_VIRTUAL_ADDRESS          BaseAddress;                    // in: Base virtual address to free
    D3DGPU_SIZE_T                   Size;                           // in: Size is bytes to free
} D3DDDICB_FREEGPUVIRTUALADDRESS;

typedef struct _D3DDDICB_UPDATEGPUVIRTUALADDRESS
{
    HANDLE                                      hContext;           // in:
    D3DKMT_HANDLE                               hFenceObject;       // in:
    UINT                                        NumOperations;      // in:
    D3DDDI_UPDATEGPUVIRTUALADDRESS_OPERATION*   Operations;         // in:
    UINT                                        Reserved0;          // in:
    UINT64                                      Reserved1;          // in:
    UINT64                                      FenceValue;         // in:
    union
    {
       struct
       {
           UINT  DoNotWait  :  1;
           UINT  Reserved   : 31;
       };
       UINT Value;
    } Flags;
} D3DDDICB_UPDATEGPUVIRTUALADDRESS;

typedef struct _D3DDDICB_CREATECONTEXTVIRTUAL
{
    UINT                        NodeOrdinal;                        // in:
    UINT                        EngineAffinity;                     // in:
    D3DDDI_CREATECONTEXTFLAGS   Flags;                              // in:
    VOID*                       pPrivateDriverData;                 // in:
    UINT                        PrivateDriverDataSize;              // in:
    HANDLE                      hContext;                           // out:
} D3DDDICB_CREATECONTEXTVIRTUAL;

typedef struct _D3DDDICB_SUBMITCOMMANDFLAGS
{
    union
    {
        struct
        {
            UINT    NullRendering            : 1;     // 0x00000001
            UINT    Reserved                 :31;     // 0xFFFFFFFE
        };
        UINT Value;
    };
} D3DDDICB_SUBMITCOMMANDFLAGS;

typedef struct _D3DDDICB_SUBMITCOMMAND
{
    D3DGPU_VIRTUAL_ADDRESS      Commands;
    UINT                        CommandLength;
    D3DDDICB_SUBMITCOMMANDFLAGS Flags;
    UINT                        BroadcastContextCount;
    HANDLE                      BroadcastContext[D3DDDI_MAX_BROADCAST_CONTEXT];
    VOID*                       pPrivateDriverData;
    UINT                        PrivateDriverDataSize;
    UINT                        NumPrimaries;
    D3DKMT_HANDLE               WrittenPrimaries[D3DDDI_MAX_WRITTEN_PRIMARIES];
    D3DDDI_MARKERLOGTYPE        MarkerLogType;              
    UINT                        RenderCBSequence;           
    UINT                        FirstAPISequenceNumberHigh; 
    UINT                        CompletedAPISequenceNumberLow0Size; 
    UINT                        CompletedAPISequenceNumberLow1Size; 
    UINT                        BegunAPISequenceNumberLow0Size; 
    UINT                        BegunAPISequenceNumberLow1Size; 
    CONST UINT*                 pCompletedAPISequenceNumberLow0;
    CONST UINT*                 pCompletedAPISequenceNumberLow1;
    CONST UINT*                 pBegunAPISequenceNumberLow0;
    CONST UINT*                 pBegunAPISequenceNumberLow1;
    UINT                        Reserved;
    UINT                        NumHistoryBuffers;
    D3DKMT_HANDLE*              HistoryBufferArray;
#if (D3D_UMD_INTERFACE_VERSION == D3D_UMD_INTERFACE_VERSION_WDDM2_1_2)
    HANDLE                      hSyncToken; // Used in conjunction with pfnReleaseResource, should be 0 otherwise
#elif (D3D_UMD_INTERFACE_VERSION > D3D_UMD_INTERFACE_VERSION_WDDM2_1_2)
    void*                       pReserved; // Set to null
#endif
} D3DDDICB_SUBMITCOMMAND;

typedef struct _D3DDDICB_DEALLOCATE2
{
    HANDLE                  hResource;          //in: D3D runtime resource handle
    UINT                    NumAllocations;     //in: number of allocation handles,ignored if hResource != NULL
    CONST D3DKMT_HANDLE*  HandleList; //in: arrary of allocation handles,ignored if hResource != NULL
    D3DDDICB_DESTROYALLOCATION2FLAGS Flags;
} D3DDDICB_DEALLOCATE2;

#endif // (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0)

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_1)

typedef struct D3DDDICB_OFFERALLOCATIONS2
{
    _In_reads_(NumAllocations) CONST HANDLE*          pResources; // in: array of D3D runtime resource handles.
    _In_reads_(NumAllocations) CONST D3DKMT_HANDLE*   HandleList; // in: array of allocation handles. If non-NULL, pResources must be NULL.
    _In_ UINT                  NumAllocations;                    // in: number of items in whichever of pResources or HandleList is non-NULL.
    _In_ D3DDDI_OFFER_PRIORITY Priority;                          // in: priority with which to offer the allocations
    _In_ D3DDDI_OFFER_FLAGS    Flags;                             // in: flags specifying additional behaviors on offer
} D3DDDICB_OFFERALLOCATIONS2;

#endif // (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_1)

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_2)

typedef struct _D3DDDICB_RECLAIMALLOCATIONS3
{
    _In_ D3DKMT_HANDLE                                      PagingQueue;      // in:  The paging queue, supplied by the UMD, to page in the allocation list
    _In_ UINT                                               NumAllocations;   // in:  number of items in pDiscarded and whichever of pResources or HandleList is non-NULL.
    _In_reads_(NumAllocations) CONST HANDLE*                pResources;       // in:  array of D3D runtime resource handles.
    _In_reads_(NumAllocations) CONST D3DKMT_HANDLE*         HandleList;       // in:  array of allocation handles.
    _Out_writes_all_(NumAllocations) D3DDDI_RECLAIM_RESULT* pResults;         // out: array of result enums specifying whether each resource or allocation was either successfully
                                                                              //      reclaimed, discarded, or has no commitment.
    _Out_ UINT64                                            PagingFenceValue; // out: The paging fence to synchronize against before submitting work to the GPU which
                                                                              //      references any of the resources or allocations in the provided arrays
} D3DDDICB_RECLAIMALLOCATIONS3;

typedef struct _D3DDDICB_SYNCTOKEN
{
    HANDLE hSyncToken;
    UINT BroadcastContextCount;

    _Field_size_(BroadcastContextCount)
    CONST HANDLE* BroadcastContextArray;
} D3DDDICB_SYNCTOKEN;

// Only provide this typedef for WDDM2.1.2 drivers - new drivers should just use the SYNCTOKEN struct directly.
#if (D3D_UMD_INTERFACE_VERSION == D3D_UMD_INTERFACE_VERSION_WDDM2_1_2)
typedef D3DDDICB_SYNCTOKEN D3DDDICB_ACQUIRERESOURCE;
#endif

#endif // (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_2)

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_2_1)

typedef struct _D3DDDICB_CREATEHWCONTEXT
{
    UINT                        NodeOrdinal;                    // in:  Specify the node ordinal this context is targeted to.
    UINT                        EngineAffinity;                 // in:  Specify the engine affinity within the node.
    D3DDDI_CREATEHWCONTEXTFLAGS Flags;                          // in:  Context creation flags.
    UINT                        PrivateDriverDataSize;          // in:  Size of private driver data
    _Inout_
    _Field_size_bytes_         (PrivateDriverDataSize)
    VOID*                       pPrivateDriverData;             // in/out:  Pointer to private driver data
    HANDLE                      hHwContext;                     // out: Handle to the created context.
} D3DDDICB_CREATEHWCONTEXT;

typedef struct _D3DDDICB_DESTROYHWCONTEXT
{
    HANDLE  hHwContext;                       // in: Handle to the context to destroy.
} D3DDDICB_DESTROYHWCONTEXT;

typedef struct _D3DDDICB_CREATEHWQUEUE
{
    HANDLE                                  hHwContext;                     // in: Handle to the context the queue is created for.
    D3DDDI_CREATEHWQUEUEFLAGS               Flags;                          // in: Queue creation flags.
    UINT                                    PrivateDriverDataSize;          // in:  Size of private driver data
    _Inout_
    _Field_size_bytes_                     (PrivateDriverDataSize)
    VOID*                                   pPrivateDriverData;             // in/out:  Pointer to private driver data
    HANDLE                                  hHwQueue;                       // out: Handle to the created queue.
    D3DKMT_HANDLE                           hHwQueueProgressFence;          // out: Handle to the hardware queue progress fence object.
    VOID*                                   HwQueueProgressFenceCPUVirtualAddress;  // out: Read-only mapping of the fence value for the CPU
    D3DGPU_VIRTUAL_ADDRESS                  HwQueueProgressFenceGPUVirtualAddress;  // out: Read/write mapping of the fence value for the GPU
} D3DDDICB_CREATEHWQUEUE;

typedef struct _D3DDDICB_DESTROYHWQUEUE
{
    HANDLE  hHwQueue;                       // in: Handle to the queue to destroy.
} D3DDDICB_DESTROYHWQUEUE;

typedef struct _D3DDDICB_SUBMITCOMMANDTOHWQUEUEFLAGS
{
    union
    {
        struct
        {
            UINT    Reserved                 :32;     // 0xFFFFFFFF
        };
        UINT Value;
    };
} D3DDDICB_SUBMITCOMMANDTOHWQUEUEFLAGS;

typedef struct _D3DDDICB_SUBMITCOMMANDTOHWQUEUE
{
    HANDLE                                  hHwQueue;
    UINT64                                  HwQueueProgressFenceId;

    D3DGPU_VIRTUAL_ADDRESS                  Commands;
    UINT                                    CommandLength;
    D3DDDICB_SUBMITCOMMANDTOHWQUEUEFLAGS    Flags;

    UINT                                    PrivateDriverDataSize;
    _Field_size_bytes_                     (PrivateDriverDataSize)
    VOID*                                   pPrivateDriverData;

    UINT                                    NumPrimaries;
    _Field_size_                           (NumPrimaries)
    D3DKMT_HANDLE*                          WrittenPrimaries;
} D3DDDICB_SUBMITCOMMANDTOHWQUEUE;

typedef struct _D3DDDICB_SUBMITWAITFORSYNCOBJECTSTOHWQUEUE
{
    HANDLE                  hHwQueue;               // in: Hardware queue to queue the wait on.

    UINT                    ObjectCount;            // in: Number of objects to wait on.

    _Field_size_           (ObjectCount)
    const D3DKMT_HANDLE*    ObjectHandleArray;      // in: Handles to monitored fence synchronization objects to wait on.

    _Field_size_(ObjectCount)
    const UINT64*           FenceValueArray;        // in: monitored fence values to be waited.
} D3DDDICB_SUBMITWAITFORSYNCOBJECTSTOHWQUEUE;

typedef struct _D3DDDICB_SUBMITSIGNALSYNCOBJECTSTOHWQUEUE
{
    D3DDDICB_SIGNALFLAGS    Flags;                  // in: Specifies signal behavior.

    ULONG                   BroadcastHwQueueCount;  // in: Specifies the number of hardware queues to broadcast this signal to.

    _Field_size_           (BroadcastHwQueueCount)
    const HANDLE*           BroadcastHwQueueArray;  // in: Specifies hardware queue handles to broadcast to.

    UINT                    ObjectCount;            // in: Number of objects to signal.

    _Field_size_           (ObjectCount)
    const D3DKMT_HANDLE*    ObjectHandleArray;      // in: Handles to monitored fence synchronization objects to signal.

    _Field_size_           (ObjectCount)
    const UINT64*           FenceValueArray;        // in: monitored fence values to signal.
} D3DDDICB_SUBMITSIGNALSYNCOBJECTSTOHWQUEUE;

#endif // (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_2_1)

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_ALLOCATECB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_ALLOCATE*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_DEALLOCATECB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_DEALLOCATE*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_SETPRIORITYCB)(
        _In_ HANDLE hDevice, _In_ D3DDDICB_SETPRIORITY*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_QUERYRESIDENCYCB)(
        _In_ HANDLE hDevice, _Inout_ CONST D3DDDICB_QUERYRESIDENCY*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_SETDISPLAYMODECB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_SETDISPLAYMODE*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_SETDISPLAYPRIVATEDRIVERFORMATCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_SETDISPLAYPRIVATEDRIVERFORMAT*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_PRESENTCB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_PRESENT*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_RENDERCB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_RENDER*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_LOCKCB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_LOCK*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_UNLOCKCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_UNLOCK*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_ESCAPECB)(
        _In_ HANDLE hAdapter, _Inout_ CONST D3DDDICB_ESCAPE*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_CREATEOVERLAYCB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_CREATEOVERLAY*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_UPDATEOVERLAYCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_UPDATEOVERLAY*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_FLIPOVERLAYCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_FLIPOVERLAY*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_DESTROYOVERLAYCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_DESTROYOVERLAY*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_CREATECONTEXTCB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_CREATECONTEXT*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_DESTROYCONTEXTCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_DESTROYCONTEXT*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_CREATESYNCHRONIZATIONOBJECTCB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_CREATESYNCHRONIZATIONOBJECT*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_DESTROYSYNCHRONIZATIONOBJECTCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_DESTROYSYNCHRONIZATIONOBJECT*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_WAITFORSYNCHRONIZATIONOBJECTCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_WAITFORSYNCHRONIZATIONOBJECT*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_SIGNALSYNCHRONIZATIONOBJECTCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_SIGNALSYNCHRONIZATIONOBJECT*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_SETASYNCCALLBACKSCB)(
        _In_ HANDLE hDevice, _In_ BOOL Enable);

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8)
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_CREATESYNCHRONIZATIONOBJECT2CB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_CREATESYNCHRONIZATIONOBJECT2*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_WAITFORSYNCHRONIZATIONOBJECT2CB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_WAITFORSYNCHRONIZATIONOBJECT2*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_SIGNALSYNCHRONIZATIONOBJECT2CB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_SIGNALSYNCHRONIZATIONOBJECT2*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_OFFERALLOCATIONSCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_OFFERALLOCATIONS*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_RECLAIMALLOCATIONSCB)(
        _In_ HANDLE hDevice, _Inout_ CONST D3DDDICB_RECLAIMALLOCATIONS*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_PRESENTMULTIPLANEOVERLAYCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_PRESENTMULTIPLANEOVERLAY*);
#endif

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // MP
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_LOGUMDMARKERCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_LOGUMDMARKER*);
#endif // D3D_UMD_INTERFACE_VERSION

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0)

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_MAKERESIDENTCB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDI_MAKERESIDENT*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_EVICTCB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_EVICT*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_WAITFORSYNCHRONIZATIONOBJECTFROMCPUCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_WAITFORSYNCHRONIZATIONOBJECTFROMCPU*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_SIGNALSYNCHRONIZATIONOBJECTFROMCPUCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_SIGNALSYNCHRONIZATIONOBJECTFROMCPU*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_WAITFORSYNCHRONIZATIONOBJECTFROMGPUCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_WAITFORSYNCHRONIZATIONOBJECTFROMGPU*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_SIGNALSYNCHRONIZATIONOBJECTFROMGPUCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_SIGNALSYNCHRONIZATIONOBJECTFROMGPU*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_SIGNALSYNCHRONIZATIONOBJECTFROMGPU2CB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_SIGNALSYNCHRONIZATIONOBJECTFROMGPU2*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_CREATEPAGINGQUEUECB)(
        _In_ HANDLE hDevice, _Out_ D3DDDICB_CREATEPAGINGQUEUE*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_DESTROYPAGINGQUEUECB)(
        _In_ HANDLE hDevice, _In_ const D3DDDI_DESTROYPAGINGQUEUE*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_LOCK2CB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_LOCK2*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_UNLOCK2CB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_UNLOCK2*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_INVALIDATECACHECB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_INVALIDATECACHE*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_MAPGPUVIRTUALADDRESSCB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDI_MAPGPUVIRTUALADDRESS*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_FREEGPUVIRTUALADDRESSCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_FREEGPUVIRTUALADDRESS*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_RESERVEGPUVIRTUALADDRESSCB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDI_RESERVEGPUVIRTUALADDRESS*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_UPDATEGPUVIRTUALADDRESSCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_UPDATEGPUVIRTUALADDRESS*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_GETRESOURCEPRESENTPRIVATEDRIVERDATACB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDI_GETRESOURCEPRESENTPRIVATEDRIVERDATA*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_CREATECONTEXTVIRTUALCB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_CREATECONTEXTVIRTUAL*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_SUBMITCOMMANDCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_SUBMITCOMMAND*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_DEALLOCATE2CB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_DEALLOCATE2*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_RECLAIMALLOCATIONS2CB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_RECLAIMALLOCATIONS2*);

#endif // (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0)

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_1)

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_UPDATEALLOCATIONPROPERTYCB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDI_UPDATEALLOCPROPERTY*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_OFFERALLOCATIONS2CB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_OFFERALLOCATIONS2*);

#endif // (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_1)

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_2)

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_RECLAIMALLOCATIONS3CB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_RECLAIMALLOCATIONS3*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_SYNCTOKENCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_SYNCTOKEN*);

#if (D3D_UMD_INTERFACE_VERSION == D3D_UMD_INTERFACE_VERSION_WDDM2_1_2)
typedef PFND3DDDI_SYNCTOKENCB PFND3DDDI_ACQUIRERESOURCECB;
#endif

#endif // (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_2)

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_2_1)

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_CREATEHWCONTEXTCB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_CREATEHWCONTEXT*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_DESTROYHWCONTEXTCB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_DESTROYHWCONTEXT*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_CREATEHWQUEUECB)(
        _In_ HANDLE hDevice, _Inout_ D3DDDICB_CREATEHWQUEUE*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_DESTROYHWQUEUECB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_DESTROYHWQUEUE*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_SUBMITCOMMANDTOHWQUEUECB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_SUBMITCOMMANDTOHWQUEUE*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_SUBMITWAITFORSYNCOBJECTSTOHWQUEUECB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_SUBMITWAITFORSYNCOBJECTSTOHWQUEUE*);

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_SUBMITSIGNALSYNCOBJECTSTOHWQUEUECB)(
        _In_ HANDLE hDevice, _In_ CONST D3DDDICB_SUBMITSIGNALSYNCOBJECTSTOHWQUEUE*);

#endif // (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_2_1)

typedef struct _D3DDDI_DEVICECALLBACKS
{
    PFND3DDDI_ALLOCATECB                            pfnAllocateCb;
    PFND3DDDI_DEALLOCATECB                          pfnDeallocateCb;
    PFND3DDDI_SETPRIORITYCB                         pfnSetPriorityCb;
    PFND3DDDI_QUERYRESIDENCYCB                      pfnQueryResidencyCb;
    PFND3DDDI_SETDISPLAYMODECB                      pfnSetDisplayModeCb;
    PFND3DDDI_PRESENTCB                             pfnPresentCb;
    PFND3DDDI_RENDERCB                              pfnRenderCb;
    PFND3DDDI_LOCKCB                                pfnLockCb;
    PFND3DDDI_UNLOCKCB                              pfnUnlockCb;
    PFND3DDDI_ESCAPECB                              pfnEscapeCb;
    PFND3DDDI_CREATEOVERLAYCB                       pfnCreateOverlayCb;
    PFND3DDDI_UPDATEOVERLAYCB                       pfnUpdateOverlayCb;
    PFND3DDDI_FLIPOVERLAYCB                         pfnFlipOverlayCb;
    PFND3DDDI_DESTROYOVERLAYCB                      pfnDestroyOverlayCb;
    PFND3DDDI_CREATECONTEXTCB                       pfnCreateContextCb;
    PFND3DDDI_DESTROYCONTEXTCB                      pfnDestroyContextCb;
    PFND3DDDI_CREATESYNCHRONIZATIONOBJECTCB         pfnCreateSynchronizationObjectCb;
    PFND3DDDI_DESTROYSYNCHRONIZATIONOBJECTCB        pfnDestroySynchronizationObjectCb;
    PFND3DDDI_WAITFORSYNCHRONIZATIONOBJECTCB        pfnWaitForSynchronizationObjectCb;
    PFND3DDDI_SIGNALSYNCHRONIZATIONOBJECTCB         pfnSignalSynchronizationObjectCb;
    PFND3DDDI_SETASYNCCALLBACKSCB                   pfnSetAsyncCallbacksCb;
    PFND3DDDI_SETDISPLAYPRIVATEDRIVERFORMATCB       pfnSetDisplayPrivateDriverFormatCb;
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8)
    PFND3DDDI_OFFERALLOCATIONSCB                    pfnOfferAllocationsCb;
    PFND3DDDI_RECLAIMALLOCATIONSCB                  pfnReclaimAllocationsCb;
    PFND3DDDI_CREATESYNCHRONIZATIONOBJECT2CB        pfnCreateSynchronizationObject2Cb;
    PFND3DDDI_WAITFORSYNCHRONIZATIONOBJECT2CB       pfnWaitForSynchronizationObject2Cb;
    PFND3DDDI_SIGNALSYNCHRONIZATIONOBJECT2CB        pfnSignalSynchronizationObject2Cb;
    PFND3DDDI_PRESENTMULTIPLANEOVERLAYCB            pfnPresentMultiPlaneOverlayCb;
#endif // (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN8)
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // MP
    PFND3DDDI_LOGUMDMARKERCB                        pfnLogUMDMarkerCb;
#endif // D3D_UMD_INTERFACE_VERSION
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0)
    PFND3DDDI_MAKERESIDENTCB                        pfnMakeResidentCb;
    PFND3DDDI_EVICTCB                               pfnEvictCb;
    PFND3DDDI_WAITFORSYNCHRONIZATIONOBJECTFROMCPUCB pfnWaitForSynchronizationObjectFromCpuCb;
    PFND3DDDI_SIGNALSYNCHRONIZATIONOBJECTFROMCPUCB  pfnSignalSynchronizationObjectFromCpuCb;
    PFND3DDDI_WAITFORSYNCHRONIZATIONOBJECTFROMGPUCB pfnWaitForSynchronizationObjectFromGpuCb;
    PFND3DDDI_SIGNALSYNCHRONIZATIONOBJECTFROMGPUCB  pfnSignalSynchronizationObjectFromGpuCb;
    PFND3DDDI_CREATEPAGINGQUEUECB                   pfnCreatePagingQueueCb;
    PFND3DDDI_DESTROYPAGINGQUEUECB                  pfnDestroyPagingQueueCb;
    PFND3DDDI_LOCK2CB                               pfnLock2Cb;
    PFND3DDDI_UNLOCK2CB                             pfnUnlock2Cb;
    PFND3DDDI_INVALIDATECACHECB                     pfnInvalidateCacheCb;
    PFND3DDDI_RESERVEGPUVIRTUALADDRESSCB            pfnReserveGpuVirtualAddressCb;
    PFND3DDDI_MAPGPUVIRTUALADDRESSCB                pfnMapGpuVirtualAddressCb;
    PFND3DDDI_FREEGPUVIRTUALADDRESSCB               pfnFreeGpuVirtualAddressCb;
    PFND3DDDI_UPDATEGPUVIRTUALADDRESSCB             pfnUpdateGpuVirtualAddressCb;
    PFND3DDDI_CREATECONTEXTVIRTUALCB                pfnCreateContextVirtualCb;
    PFND3DDDI_SUBMITCOMMANDCB                       pfnSubmitCommandCb;
    PFND3DDDI_DEALLOCATE2CB                         pfnDeallocate2Cb;
    PFND3DDDI_SIGNALSYNCHRONIZATIONOBJECTFROMGPU2CB pfnSignalSynchronizationObjectFromGpu2Cb;
    PFND3DDDI_RECLAIMALLOCATIONS2CB                 pfnReclaimAllocations2Cb;
    PFND3DDDI_GETRESOURCEPRESENTPRIVATEDRIVERDATACB pfnGetResourcePresentPrivateDriverDataCb;
#endif // D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_0
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_1)
    PFND3DDDI_UPDATEALLOCATIONPROPERTYCB            pfnUpdateAllocationPropertyCb;
    PFND3DDDI_OFFERALLOCATIONS2CB                   pfnOfferAllocations2Cb;
#endif // D3D_UMD_INTERFACE_VERSION
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_2)
    PFND3DDDI_RECLAIMALLOCATIONS3CB                 pfnReclaimAllocations3Cb;
    PFND3DDDI_SYNCTOKENCB                           pfnAcquireResourceCb;
#endif // D3D_UMD_INTERFACE_VERSION
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_1_3)
    PFND3DDDI_SYNCTOKENCB                           pfnReleaseResourceCb;
#endif // D3D_UMD_INTERFACE_VERSION
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM2_2_1)
    PFND3DDDI_CREATEHWCONTEXTCB                     pfnCreateHwContextCb;
    PFND3DDDI_DESTROYHWCONTEXTCB                    pfnDestroyHwContextCb;
    PFND3DDDI_CREATEHWQUEUECB                       pfnCreateHwQueueCb;
    PFND3DDDI_DESTROYHWQUEUECB                      pfnDestroyHwQueueCb;
    PFND3DDDI_SUBMITCOMMANDTOHWQUEUECB              pfnSubmitCommandToHwQueueCb;
    PFND3DDDI_SUBMITWAITFORSYNCOBJECTSTOHWQUEUECB   pfnSubmitWaitForSyncObjectsToHwQueueCb;
    PFND3DDDI_SUBMITSIGNALSYNCOBJECTSTOHWQUEUECB    pfnSubmitSignalSyncObjectsToHwQueueCb;
#endif // D3D_UMD_INTERFACE_VERSION
} D3DDDI_DEVICECALLBACKS;

typedef struct _D3DDDI_CREATEDEVICEFLAGS
{
    union
    {
        struct
        {
            UINT    AllowMultithreading     : 1;    // 0x00000001
            UINT    AllowFlipBatching       : 1;    // 0x00000002
            UINT    Reserved                :30;    // 0xFFFFFFFD
        };
        UINT Value;
    };
} D3DDDI_CREATEDEVICEFLAGS;

typedef struct _D3DDDIARG_CREATEDEVICE
{
    HANDLE                          hDevice;                // in:  Runtime handle/out: Driver handle
    UINT                            Interface;              // in:  Interface version
    UINT                            Version;                // in:  Runtime Version
    CONST D3DDDI_DEVICECALLBACKS*   pCallbacks;             // in:  Pointer to runtime callbacks
    VOID*                           pCommandBuffer;         // in:  Pointer to the first command buffer to use.
    UINT                            CommandBufferSize;      // in:  Size of the first command buffer to use.
    D3DDDI_ALLOCATIONLIST*          pAllocationList;        // out: Pointer to the first allocation list to use.
    UINT                            AllocationListSize;     // in:  Size of the allocation list that will be available
                                                            //      when the first command buffer is submitted.
    D3DDDI_PATCHLOCATIONLIST*       pPatchLocationList;     // out: Pointer to the first patch location list to use.
    UINT                            PatchLocationListSize;  // in:  Size of the patch location list that will be available
                                                            //      when the first command buffer is submitted.
    D3DDDI_DEVICEFUNCS*             pDeviceFuncs;           // out: Driver function table
    D3DDDI_CREATEDEVICEFLAGS        Flags;                  // in:  Flags
#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WIN7)
    D3DGPU_VIRTUAL_ADDRESS          CommandBuffer;          // out: GPU Virtual address to the command buffer to use. _ADVSCH_
#endif // D3D_UMD_INTERFACE_VERSION
} D3DDDIARG_CREATEDEVICE;

typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_QUERYADAPTERINFOCB)(
        _In_ HANDLE hAdapter, _Inout_ CONST D3DDDICB_QUERYADAPTERINFO*);
typedef _Check_return_ HRESULT (APIENTRY CALLBACK *PFND3DDDI_GETMULTISAMPLEMETHODLISTCB)(
        _In_ HANDLE hAdapter, _Inout_ D3DDDICB_GETMULTISAMPLEMETHODLIST*);

typedef struct _D3DDDI_ADAPTERCALLBACKS
{
    PFND3DDDI_QUERYADAPTERINFOCB            pfnQueryAdapterInfoCb;
    PFND3DDDI_GETMULTISAMPLEMETHODLISTCB    pfnGetMultisampleMethodListCb;
} D3DDDI_ADAPTERCALLBACKS;

typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_GETCAPS)(
        _In_ HANDLE hAdapter, _Inout_ CONST D3DDDIARG_GETCAPS*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CREATEDEVICE)(
        _In_ HANDLE hAdapter, _Inout_ D3DDDIARG_CREATEDEVICE*);
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_CLOSEADAPTER)(
        _In_ HANDLE hAdapter);

typedef struct _D3DDDI_ADAPTERFUNCS
{
    PFND3DDDI_GETCAPS                       pfnGetCaps;
    PFND3DDDI_CREATEDEVICE                  pfnCreateDevice;
    PFND3DDDI_CLOSEADAPTER                  pfnCloseAdapter;
} D3DDDI_ADAPTERFUNCS;

typedef struct _D3DDDIARG_OPENADAPTER
{
    HANDLE                         hAdapter;           // in/out:  Runtime handle/out: Driver handle
    UINT                           Interface;          // in:  Interface version
    UINT                           Version;            // in:  Runtime version
    CONST D3DDDI_ADAPTERCALLBACKS* pAdapterCallbacks;  // in:  Pointer to runtime callbacks
    D3DDDI_ADAPTERFUNCS*           pAdapterFuncs;      // out: Driver function table
    UINT                           DriverVersion;      // out: D3D UMD interface version the
                                                       //      driver was compiled with. Use
                                                       //      D3D_UMD_INTERFACE_VERSION.
} D3DDDIARG_OPENADAPTER;

typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_OPENADAPTER)(
        _Inout_ D3DDDIARG_OPENADAPTER*);

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // MP
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDICB_LOGSTRINGTABLEENTRY)(
    HANDLE hLog, UINT StringIndex, LPCWSTR Info );

typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_LOGSTRINGTABLE)(
    HANDLE hLog, PFND3DDDICB_LOGSTRINGTABLEENTRY pfnLogStringTableEntryCb );

#define D3DDDI_LOGMARKERSTRINGTABLE_PROCNAME "LogMarkerStringTable"
#endif

#define D3DDDI_VERSION64_MAJOR16_MASK 0xFFFF000000000000ui64
#define D3DDDI_VERSION64_MINOR16_MASK 0x0000FFFF00000000ui64
#define D3DDDI_VERSION64_BUILD16_MASK 0x00000000FFFF0000ui64
#define D3DDDI_VERSION64_REVISION16_MASK 0x000000000000FFFFui64
#define D3DDDI_VERSION64_INTERFACE32_MASK (D3DDDI_VERSION64_MAJOR16_MASK | D3DDDI_VERSION64_MINOR16_MASK)
#define D3DDDI_VERSION64_VERSION32_MASK (D3DDDI_VERSION64_BUILD16_MASK | D3DDDI_VERSION64_REVISION16_MASK)
#define D3DDDI_MAJOR16_FROM_VERSION64( v ) ((UINT16)(v >> 48))
#define D3DDDI_MINOR16_FROM_VERSION64( v ) ((UINT16)(v >> 32))
#define D3DDDI_BUILD16_FROM_VERSION64( v ) ((UINT16)(v >> 16))
#define D3DDDI_REVISION16_FROM_VERSION64( v ) ((UINT16)(v))
#define D3DDDI_INTERFACE32_FROM_VERSION64( v ) ((UINT32)(v >> 32))
#define D3DDDI_VERSION32_FROM_VERSION64( v ) ((UINT32)(v))
#define D3DDDI_MAJOR16_FROM_INTERFACE32( v ) ((UINT16)(v >> 16))
#define D3DDDI_MINOR16_FROM_INTERFACE32( v ) ((UINT16)(v))
#define D3DDDI_BUILD16_FROM_VERSION32( v ) ((UINT16)(v >> 16))
#define D3DDDI_REVISION16_FROM_VERSION32( v ) ((UINT16)(v))
#define D3DDDI_VERSION64_FROM16( maj, min, bld, rev ) \
    ((((UINT64)(UINT16)maj) << 48) | (((UINT64)(UINT16)min) << 32) | (((UINT64)(UINT16)bld) << 16) | ((UINT64)(UINT16)rev))
#define D3DDDI_VERSION64_FROM32( interface, version ) \
    ((((UINT64)(UINT32)interface) << 32) | ((UINT64)(UINT32)version))


#define _FACD3DDDI  0x876
#define MAKE_D3DDDIHRESULT(code)  MAKE_HRESULT(1, _FACD3DDDI, code)

#define D3DDDIERR_OUTOFVIDEOMEMORY              MAKE_D3DDDIHRESULT(380)
#define D3DDDIERR_WASSTILLDRAWING               MAKE_D3DDDIHRESULT(540)
#define D3DDDIERR_NOTAVAILABLE                  MAKE_D3DDDIHRESULT(2154)
#define D3DDDIERR_INVALIDCALL                   MAKE_D3DDDIHRESULT(2156)
#define D3DDDIERR_DEVICEREMOVED                 MAKE_D3DDDIHRESULT(2160)
#define D3DDDIERR_PRIVILEGEDINSTRUCTION         MAKE_D3DDDIHRESULT(2161)
#define D3DDDIERR_INVALIDINSTRUCTION            MAKE_D3DDDIHRESULT(2162)
#define D3DDDIERR_INVALIDHANDLE                 MAKE_D3DDDIHRESULT(2163)
#define D3DDDIERR_CANTEVICTPINNEDALLOCATION     ERROR_GRAPHICS_CANT_EVICT_PINNED_ALLOCATION
#define D3DDDIERR_CANTRENDERLOCKEDALLOCATION    ERROR_GRAPHICS_CANT_RENDER_LOCKED_ALLOCATION
#define D3DDDIERR_INVALIDUSERBUFFER             MAKE_D3DDDIHRESULT(2169)
#define D3DDDIERR_INCOMPATIBLEPRIVATEFORMAT     MAKE_D3DDDIHRESULT(2170)
#define D3DDDIERR_UNSUPPORTEDOVERLAY            MAKE_D3DDDIHRESULT(2171)
#define D3DDDIERR_UNSUPPORTEDOVERLAYFORMAT      MAKE_D3DDDIHRESULT(2172)
#define D3DDDIERR_UNSUPPORTEDCRYPTO             MAKE_D3DDDIHRESULT(2174)
#define D3DDDIERR_APPLICATIONERROR              MAKE_D3DDDIHRESULT(2181)
#define D3DDDIERR_OUTOFHWPROTECTEDMEMORY        MAKE_D3DDDIHRESULT(2183)

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // M1
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_QUERYDLISTFORAPPLICATION)(
        _Out_ BOOL*);
#endif

#if (D3D_UMD_INTERFACE_VERSION >= D3D_UMD_INTERFACE_VERSION_WDDM1_3) // MP
typedef _Check_return_ HRESULT (APIENTRY *PFND3DDDI_QUERYDLISTFORAPPLICATION1)(
        _Out_ BOOL*, _In_ HANDLE, _In_ PFND3DDDI_ESCAPECB);
#endif

#pragma warning(pop)

#endif /* _D3DUMDDI_H */

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion



