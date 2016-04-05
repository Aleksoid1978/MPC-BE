/*
 * (C) 2015 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#if (_MSC_VER < 1900)
#include <dx/d3d.h>
#include <initguid.h>

DEFINE_GUID( IID_IDirectDraw7,       0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b );
#else

#include <d3d9.h>
#include <initguid.h>

DEFINE_GUID( IID_IDirect3D7,         0xf5049e77,0x4861,0x11d2,0xa4,0x07,0x00,0xa0,0xc9,0x60,0x29,0xa8 );
DEFINE_GUID( IID_IDirect3DHALDevice, 0x84E63dE0,0x46AA,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E );
DEFINE_GUID( IID_IDirect3DDevice7,   0xf5049e79,0x4861,0x11d2,0xa4,0x07,0x00,0xa0,0xc9,0x06,0x29,0xa8 );
DEFINE_GUID( IID_IDirectDraw7,       0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b );

struct IDirect3D7;
struct IDirect3DDevice7;
struct IDirect3DVertexBuffer7;
typedef struct IDirect3D7             *LPDIRECT3D7;
typedef struct IDirect3DDevice7       *LPDIRECT3DDEVICE7;
typedef struct IDirect3DVertexBuffer7 *LPDIRECT3DVERTEXBUFFER7;

typedef float D3DVALUE, *LPD3DVALUE;

typedef struct _D3DPrimCaps {
	DWORD dwSize;
	DWORD dwMiscCaps;                 /* Capability flags */
	DWORD dwRasterCaps;
	DWORD dwZCmpCaps;
	DWORD dwSrcBlendCaps;
	DWORD dwDestBlendCaps;
	DWORD dwAlphaCmpCaps;
	DWORD dwShadeCaps;
	DWORD dwTextureCaps;
	DWORD dwTextureFilterCaps;
	DWORD dwTextureBlendCaps;
	DWORD dwTextureAddressCaps;
	DWORD dwStippleWidth;             /* maximum width and height of */
	DWORD dwStippleHeight;            /* of supported stipple (up to 32x32) */
} D3DPRIMCAPS, *LPD3DPRIMCAPS;

typedef struct _D3DDeviceDesc7 {
	DWORD            dwDevCaps;              /* Capabilities of device */
	D3DPRIMCAPS      dpcLineCaps;
	D3DPRIMCAPS      dpcTriCaps;
	DWORD            dwDeviceRenderBitDepth; /* One of DDBB_8, 16, etc.. */
	DWORD            dwDeviceZBufferBitDepth;/* One of DDBD_16, 32, etc.. */

	DWORD       dwMinTextureWidth, dwMinTextureHeight;
	DWORD       dwMaxTextureWidth, dwMaxTextureHeight;

	DWORD       dwMaxTextureRepeat;
	DWORD       dwMaxTextureAspectRatio;
	DWORD       dwMaxAnisotropy;

	D3DVALUE    dvGuardBandLeft;
	D3DVALUE    dvGuardBandTop;
	D3DVALUE    dvGuardBandRight;
	D3DVALUE    dvGuardBandBottom;

	D3DVALUE    dvExtentsAdjust;
	DWORD       dwStencilCaps;

	DWORD       dwFVFCaps;
	DWORD       dwTextureOpCaps;
	WORD        wMaxTextureBlendStages;
	WORD        wMaxSimultaneousTextures;

	DWORD       dwMaxActiveLights;
	D3DVALUE    dvMaxVertexW;
	GUID        deviceGUID;

	WORD        wMaxUserClipPlanes;
	WORD        wMaxVertexBlendMatrices;

	DWORD       dwVertexProcessingCaps;

	DWORD       dwReserved1;
	DWORD       dwReserved2;
	DWORD       dwReserved3;
	DWORD       dwReserved4;
} D3DDEVICEDESC7, *LPD3DDEVICEDESC7;

typedef struct _D3DVERTEXBUFFERDESC {
	DWORD dwSize;
	DWORD dwCaps;
	DWORD dwFVF;
	DWORD dwNumVertices;
} D3DVERTEXBUFFERDESC, *LPD3DVERTEXBUFFERDESC;

#ifndef D3DRECT_DEFINED
typedef struct _D3DRECT {
	union {
		LONG x1;
		LONG lX1;
	};
	union {
		LONG y1;
		LONG lY1;
	};
	union {
		LONG x2;
		LONG lX2;
	};
	union {
		LONG y2;
		LONG lY2;
	};
} D3DRECT;
#define D3DRECT_DEFINED
#endif
typedef struct _D3DRECT *LPD3DRECT;

#ifndef D3DMATRIX_DEFINED
typedef struct _D3DMATRIX {
#if(DIRECT3D_VERSION >= 0x0500)
#if (defined __cplusplus) && (defined D3D_OVERLOADS)
	union {
		struct {
#endif

#endif /* DIRECT3D_VERSION >= 0x0500 */
			D3DVALUE        _11, _12, _13, _14;
			D3DVALUE        _21, _22, _23, _24;
			D3DVALUE        _31, _32, _33, _34;
			D3DVALUE        _41, _42, _43, _44;

#if(DIRECT3D_VERSION >= 0x0500)
#if (defined __cplusplus) && (defined D3D_OVERLOADS)
		};
		D3DVALUE m[4][4];
	};
	_D3DMATRIX() { }
	_D3DMATRIX(D3DVALUE _m00, D3DVALUE _m01, D3DVALUE _m02, D3DVALUE _m03,
		D3DVALUE _m10, D3DVALUE _m11, D3DVALUE _m12, D3DVALUE _m13,
		D3DVALUE _m20, D3DVALUE _m21, D3DVALUE _m22, D3DVALUE _m23,
		D3DVALUE _m30, D3DVALUE _m31, D3DVALUE _m32, D3DVALUE _m33
		)
	{
		m[0][0] = _m00; m[0][1] = _m01; m[0][2] = _m02; m[0][3] = _m03;
		m[1][0] = _m10; m[1][1] = _m11; m[1][2] = _m12; m[1][3] = _m13;
		m[2][0] = _m20; m[2][1] = _m21; m[2][2] = _m22; m[2][3] = _m23;
		m[3][0] = _m30; m[3][1] = _m31; m[3][2] = _m32; m[3][3] = _m33;
	}

	D3DVALUE& operator()(int iRow, int iColumn) { return m[iRow][iColumn]; }
	const D3DVALUE& operator()(int iRow, int iColumn) const { return m[iRow][iColumn]; }
#if(DIRECT3D_VERSION >= 0x0600)
	friend _D3DMATRIX operator* (const _D3DMATRIX&, const _D3DMATRIX&);
#endif /* DIRECT3D_VERSION >= 0x0600 */
#endif
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DMATRIX;
#define D3DMATRIX_DEFINED
#endif
typedef struct _D3DMATRIX *LPD3DMATRIX;

typedef struct _D3DVIEWPORT7 {
	DWORD       dwX;
	DWORD       dwY;            /* Viewport Top left */
	DWORD       dwWidth;
	DWORD       dwHeight;       /* Viewport Dimensions */
	D3DVALUE    dvMinZ;         /* Min/max of clip Volume */
	D3DVALUE    dvMaxZ;
} D3DVIEWPORT7, *LPD3DVIEWPORT7;

typedef struct _D3DMATERIAL7 {
	union {
		D3DCOLORVALUE   diffuse;        /* Diffuse color RGBA */
		D3DCOLORVALUE   dcvDiffuse;
	};
	union {
		D3DCOLORVALUE   ambient;        /* Ambient color RGB */
		D3DCOLORVALUE   dcvAmbient;
	};
	union {
		D3DCOLORVALUE   specular;       /* Specular 'shininess' */
		D3DCOLORVALUE   dcvSpecular;
	};
	union {
		D3DCOLORVALUE   emissive;       /* Emissive color RGB */
		D3DCOLORVALUE   dcvEmissive;
	};
	union {
		D3DVALUE        power;          /* Sharpness if specular highlight */
		D3DVALUE        dvPower;
	};
} D3DMATERIAL7, *LPD3DMATERIAL7;

typedef struct _D3DLIGHT7 {
	D3DLIGHTTYPE    dltType;            /* Type of light source */
	D3DCOLORVALUE   dcvDiffuse;         /* Diffuse color of light */
	D3DCOLORVALUE   dcvSpecular;        /* Specular color of light */
	D3DCOLORVALUE   dcvAmbient;         /* Ambient color of light */
	D3DVECTOR       dvPosition;         /* Position in world space */
	D3DVECTOR       dvDirection;        /* Direction in world space */
	D3DVALUE        dvRange;            /* Cutoff range */
	D3DVALUE        dvFalloff;          /* Falloff */
	D3DVALUE        dvAttenuation0;     /* Constant attenuation */
	D3DVALUE        dvAttenuation1;     /* Linear attenuation */
	D3DVALUE        dvAttenuation2;     /* Quadratic attenuation */
	D3DVALUE        dvTheta;            /* Inner angle of spotlight cone */
	D3DVALUE        dvPhi;              /* Outer angle of spotlight cone */
} D3DLIGHT7, *LPD3DLIGHT7;

typedef struct _D3DCLIPSTATUS {
	DWORD dwFlags; /* Do we set 2d extents, 3D extents or status */
	DWORD dwStatus; /* Clip status */
	float minx, maxx; /* X extents */
	float miny, maxy; /* Y extents */
	float minz, maxz; /* Z extents */
} D3DCLIPSTATUS, *LPD3DCLIPSTATUS;

typedef struct _D3DDP_PTRSTRIDE
{
	LPVOID lpvData;
	DWORD  dwStride;
} D3DDP_PTRSTRIDE;

typedef struct _D3DDRAWPRIMITIVESTRIDEDDATA
{
	D3DDP_PTRSTRIDE position;
	D3DDP_PTRSTRIDE normal;
	D3DDP_PTRSTRIDE diffuse;
	D3DDP_PTRSTRIDE specular;
	D3DDP_PTRSTRIDE textureCoords[D3DDP_MAXTEXCOORD];
} D3DDRAWPRIMITIVESTRIDEDDATA, *LPD3DDRAWPRIMITIVESTRIDEDDATA;

#ifndef D3DVECTOR_DEFINED
typedef struct _D3DVECTOR {
	union {
		D3DVALUE x;
		D3DVALUE dvX;
	};
	union {
		D3DVALUE y;
		D3DVALUE dvY;
	};
	union {
		D3DVALUE z;
		D3DVALUE dvZ;
	};
#if(DIRECT3D_VERSION >= 0x0500)
#if (defined __cplusplus) && (defined D3D_OVERLOADS)

public:

	// =====================================
	// Constructors
	// =====================================

	_D3DVECTOR() { }
	_D3DVECTOR(D3DVALUE f);
	_D3DVECTOR(D3DVALUE _x, D3DVALUE _y, D3DVALUE _z);
	_D3DVECTOR(const D3DVALUE f[3]);

	// =====================================
	// Access grants
	// =====================================

	const D3DVALUE&operator[](int i) const;
	D3DVALUE&operator[](int i);

	// =====================================
	// Assignment operators
	// =====================================

	_D3DVECTOR& operator += (const _D3DVECTOR& v);
	_D3DVECTOR& operator -= (const _D3DVECTOR& v);
	_D3DVECTOR& operator *= (const _D3DVECTOR& v);
	_D3DVECTOR& operator /= (const _D3DVECTOR& v);
	_D3DVECTOR& operator *= (D3DVALUE s);
	_D3DVECTOR& operator /= (D3DVALUE s);

	// =====================================
	// Unary operators
	// =====================================

	friend _D3DVECTOR operator + (const _D3DVECTOR& v);
	friend _D3DVECTOR operator - (const _D3DVECTOR& v);


	// =====================================
	// Binary operators
	// =====================================

	// Addition and subtraction
	friend _D3DVECTOR operator + (const _D3DVECTOR& v1, const _D3DVECTOR& v2);
	friend _D3DVECTOR operator - (const _D3DVECTOR& v1, const _D3DVECTOR& v2);
	// Scalar multiplication and division
	friend _D3DVECTOR operator * (const _D3DVECTOR& v, D3DVALUE s);
	friend _D3DVECTOR operator * (D3DVALUE s, const _D3DVECTOR& v);
	friend _D3DVECTOR operator / (const _D3DVECTOR& v, D3DVALUE s);
	// Memberwise multiplication and division
	friend _D3DVECTOR operator * (const _D3DVECTOR& v1, const _D3DVECTOR& v2);
	friend _D3DVECTOR operator / (const _D3DVECTOR& v1, const _D3DVECTOR& v2);

	// Vector dominance
	friend int operator < (const _D3DVECTOR& v1, const _D3DVECTOR& v2);
	friend int operator <= (const _D3DVECTOR& v1, const _D3DVECTOR& v2);

	// Bitwise equality
	friend int operator == (const _D3DVECTOR& v1, const _D3DVECTOR& v2);

	// Length-related functions
	friend D3DVALUE SquareMagnitude(const _D3DVECTOR& v);
	friend D3DVALUE Magnitude(const _D3DVECTOR& v);

	// Returns vector with same direction and unit length
	friend _D3DVECTOR Normalize(const _D3DVECTOR& v);

	// Return min/max component of the input vector
	friend D3DVALUE Min(const _D3DVECTOR& v);
	friend D3DVALUE Max(const _D3DVECTOR& v);

	// Return memberwise min/max of input vectors
	friend _D3DVECTOR Minimize(const _D3DVECTOR& v1, const _D3DVECTOR& v2);
	friend _D3DVECTOR Maximize(const _D3DVECTOR& v1, const _D3DVECTOR& v2);

	// Dot and cross product
	friend D3DVALUE DotProduct(const _D3DVECTOR& v1, const _D3DVECTOR& v2);
	friend _D3DVECTOR CrossProduct(const _D3DVECTOR& v1, const _D3DVECTOR& v2);

#endif
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DVECTOR;
#define D3DVECTOR_DEFINED
#endif
typedef struct _D3DVECTOR *LPD3DVECTOR;

typedef enum _D3DTEXTUREMAGFILTER
{
	D3DTFG_POINT = 1,    // nearest
	D3DTFG_LINEAR = 2,    // linear interpolation
	D3DTFG_FLATCUBIC = 3,    // cubic
	D3DTFG_GAUSSIANCUBIC = 4,   // different cubic kernel
	D3DTFG_ANISOTROPIC = 5,    //
	D3DTFG_FORCE_DWORD = 0x7fffffff,   // force 32-bit size enum
} D3DTEXTUREMAGFILTER;

typedef enum _D3DTEXTUREMINFILTER
{
	D3DTFN_POINT = 1,    // nearest
	D3DTFN_LINEAR = 2,    // linear interpolation
	D3DTFN_ANISOTROPIC = 3,    //
	D3DTFN_FORCE_DWORD = 0x7fffffff,   // force 32-bit size enum
} D3DTEXTUREMINFILTER;

typedef enum _D3DTEXTUREMIPFILTER
{
	D3DTFP_NONE = 1,    // mipmapping disabled (use MAG filter)
	D3DTFP_POINT = 2,    // nearest
	D3DTFP_LINEAR = 3,    // linear interpolation
	D3DTFP_FORCE_DWORD = 0x7fffffff,   // force 32-bit size enum
} D3DTEXTUREMIPFILTER;

#define D3DTSS_ADDRESS   (D3DTEXTURESTAGESTATETYPE)12
#define D3DTSS_MAGFILTER (D3DTEXTURESTAGESTATETYPE)16
#define D3DTSS_MINFILTER (D3DTEXTURESTAGESTATETYPE)17
#define D3DTSS_MIPFILTER (D3DTEXTURESTAGESTATETYPE)18

#define D3DDP_WAIT 0x00000001l

#define D3DRENDERSTATE_CULLMODE        D3DRS_CULLMODE
#define D3DRENDERSTATE_LIGHTING        D3DRS_LIGHTING
#define D3DRENDERSTATE_BLENDENABLE     D3DRS_ALPHABLENDENABLE
#define D3DRENDERSTATE_ALPHATESTENABLE D3DRS_ALPHATESTENABLE
#define D3DRENDERSTATE_SRCBLEND        D3DRS_SRCBLEND
#define D3DRENDERSTATE_DESTBLEND       D3DRS_DESTBLEND

typedef HRESULT(CALLBACK * LPD3DENUMDEVICESCALLBACK7)(LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC7, LPVOID);
typedef HRESULT(CALLBACK* LPD3DENUMPIXELFORMATSCALLBACK)(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext);

DECLARE_INTERFACE_(IDirect3D7, IUnknown)
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	/*** IDirect3D7 methods ***/
	STDMETHOD(EnumDevices)(THIS_ LPD3DENUMDEVICESCALLBACK7, LPVOID) PURE;
	STDMETHOD(CreateDevice)(THIS_ REFCLSID, LPDIRECTDRAWSURFACE7, LPDIRECT3DDEVICE7*) PURE;
	STDMETHOD(CreateVertexBuffer)(THIS_ LPD3DVERTEXBUFFERDESC, LPDIRECT3DVERTEXBUFFER7*, DWORD) PURE;
	STDMETHOD(EnumZBufferFormats)(THIS_ REFCLSID, LPD3DENUMPIXELFORMATSCALLBACK, LPVOID) PURE;
	STDMETHOD(EvictManagedTextures)(THIS) PURE;
};

DECLARE_INTERFACE_(IDirect3DDevice7, IUnknown)
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	/*** IDirect3DDevice7 methods ***/
	STDMETHOD(GetCaps)(THIS_ LPD3DDEVICEDESC7) PURE;
	STDMETHOD(EnumTextureFormats)(THIS_ LPD3DENUMPIXELFORMATSCALLBACK, LPVOID) PURE;
	STDMETHOD(BeginScene)(THIS) PURE;
	STDMETHOD(EndScene)(THIS) PURE;
	STDMETHOD(GetDirect3D)(THIS_ LPDIRECT3D7*) PURE;
	STDMETHOD(SetRenderTarget)(THIS_ LPDIRECTDRAWSURFACE7, DWORD) PURE;
	STDMETHOD(GetRenderTarget)(THIS_ LPDIRECTDRAWSURFACE7 *) PURE;
	STDMETHOD(Clear)(THIS_ DWORD, LPD3DRECT, DWORD, D3DCOLOR, D3DVALUE, DWORD) PURE;
	STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE, LPD3DMATRIX) PURE;
	STDMETHOD(GetTransform)(THIS_ D3DTRANSFORMSTATETYPE, LPD3DMATRIX) PURE;
	STDMETHOD(SetViewport)(THIS_ LPD3DVIEWPORT7) PURE;
	STDMETHOD(MultiplyTransform)(THIS_ D3DTRANSFORMSTATETYPE, LPD3DMATRIX) PURE;
	STDMETHOD(GetViewport)(THIS_ LPD3DVIEWPORT7) PURE;
	STDMETHOD(SetMaterial)(THIS_ LPD3DMATERIAL7) PURE;
	STDMETHOD(GetMaterial)(THIS_ LPD3DMATERIAL7) PURE;
	STDMETHOD(SetLight)(THIS_ DWORD, LPD3DLIGHT7) PURE;
	STDMETHOD(GetLight)(THIS_ DWORD, LPD3DLIGHT7) PURE;
	STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE, DWORD) PURE;
	STDMETHOD(GetRenderState)(THIS_ D3DRENDERSTATETYPE, LPDWORD) PURE;
	STDMETHOD(BeginStateBlock)(THIS) PURE;
	STDMETHOD(EndStateBlock)(THIS_ LPDWORD) PURE;
	STDMETHOD(PreLoad)(THIS_ LPDIRECTDRAWSURFACE7) PURE;
	STDMETHOD(DrawPrimitive)(THIS_ D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, DWORD) PURE;
	STDMETHOD(DrawIndexedPrimitive)(THIS_ D3DPRIMITIVETYPE, DWORD, LPVOID, DWORD, LPWORD, DWORD, DWORD) PURE;
	STDMETHOD(SetClipStatus)(THIS_ LPD3DCLIPSTATUS) PURE;
	STDMETHOD(GetClipStatus)(THIS_ LPD3DCLIPSTATUS) PURE;
	STDMETHOD(DrawPrimitiveStrided)(THIS_ D3DPRIMITIVETYPE, DWORD, LPD3DDRAWPRIMITIVESTRIDEDDATA, DWORD, DWORD) PURE;
	STDMETHOD(DrawIndexedPrimitiveStrided)(THIS_ D3DPRIMITIVETYPE, DWORD, LPD3DDRAWPRIMITIVESTRIDEDDATA, DWORD, LPWORD, DWORD, DWORD) PURE;
	STDMETHOD(DrawPrimitiveVB)(THIS_ D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, DWORD) PURE;
	STDMETHOD(DrawIndexedPrimitiveVB)(THIS_ D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, LPWORD, DWORD, DWORD) PURE;
	STDMETHOD(ComputeSphereVisibility)(THIS_ LPD3DVECTOR, LPD3DVALUE, DWORD, DWORD, LPDWORD) PURE;
	STDMETHOD(GetTexture)(THIS_ DWORD, LPDIRECTDRAWSURFACE7 *) PURE;
	STDMETHOD(SetTexture)(THIS_ DWORD, LPDIRECTDRAWSURFACE7) PURE;
	STDMETHOD(GetTextureStageState)(THIS_ DWORD, D3DTEXTURESTAGESTATETYPE, LPDWORD) PURE;
	STDMETHOD(SetTextureStageState)(THIS_ DWORD, D3DTEXTURESTAGESTATETYPE, DWORD) PURE;
	STDMETHOD(ValidateDevice)(THIS_ LPDWORD) PURE;
	STDMETHOD(ApplyStateBlock)(THIS_ DWORD) PURE;
	STDMETHOD(CaptureStateBlock)(THIS_ DWORD) PURE;
	STDMETHOD(DeleteStateBlock)(THIS_ DWORD) PURE;
	STDMETHOD(CreateStateBlock)(THIS_ D3DSTATEBLOCKTYPE, LPDWORD) PURE;
	STDMETHOD(Load)(THIS_ LPDIRECTDRAWSURFACE7, LPPOINT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD) PURE;
	STDMETHOD(LightEnable)(THIS_ DWORD, BOOL) PURE;
	STDMETHOD(GetLightEnable)(THIS_ DWORD, BOOL*) PURE;
	STDMETHOD(SetClipPlane)(THIS_ DWORD, D3DVALUE*) PURE;
	STDMETHOD(GetClipPlane)(THIS_ DWORD, D3DVALUE*) PURE;
	STDMETHOD(GetInfo)(THIS_ DWORD, LPVOID, DWORD) PURE;
};

#endif