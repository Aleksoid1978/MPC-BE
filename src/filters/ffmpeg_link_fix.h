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
#endif

#endif // FFMPEG_LINK_FIX_H
