#ifndef AVCODEC_X86_HEVCPRED_H
#define AVCODEC_X86_HEVCPRED_H

void pred_planar_0_8_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride);
void pred_planar_1_8_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride);
void pred_planar_2_8_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride);
void pred_planar_3_8_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride);

void pred_angular_0_8_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride, int c_idx, int mode);
void pred_angular_1_8_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride, int c_idx, int mode);
void pred_angular_2_8_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride, int c_idx, int mode);
void pred_angular_3_8_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride, int c_idx, int mode);

void pred_planar_0_10_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride);
void pred_planar_1_10_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride);
void pred_planar_2_10_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride);
void pred_planar_3_10_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride);

void pred_angular_0_10_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride, int c_idx, int mode);
void pred_angular_1_10_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride, int c_idx, int mode);
void pred_angular_2_10_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride, int c_idx, int mode);
void pred_angular_3_10_sse(uint8_t *_src, const uint8_t *_top, const uint8_t *_left, ptrdiff_t stride, int c_idx, int mode);

#endif // AVCODEC_X86_HEVCPRED_H