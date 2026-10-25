#ifndef FFMPEG_LINK_FIX_H
#define FFMPEG_LINK_FIX_H

// hack for ffmpeg linking error
void *__imp__aligned_malloc  = _aligned_malloc;
void *__imp__aligned_realloc = _aligned_realloc;
void *__imp__aligned_free    = _aligned_free;

#ifdef REGISTER_FILTER
#include <io.h>
void *__imp__sopen  = _sopen;
void *__imp__wsopen = _wsopen;
#ifdef _M_IX86
extern "C" float _acosf(float x) {
	// hack to avoid error "unresolved external symbol" when linking
	return std::acos(x);
}
#endif // _M_IX86
#endif // REGISTER_FILTER

#endif // FFMPEG_LINK_FIX_H
