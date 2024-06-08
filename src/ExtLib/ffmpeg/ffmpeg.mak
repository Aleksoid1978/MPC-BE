BIN_DIR       = ../../../_bin
ZLIB_DIR      = ../zlib
SPEEX_DIR     = ../speex
SOXR_DIR      = ../soxr
DAV1_DIR      = ../dav1d
FFNVCODEC_DIR = ../nv-codec-headers/include
UAVS3D_DIR    = ../uavs3d/source/decoder
VVDEC_DIR     = ../vvdec/vvdec/include

ifeq ($(64BIT),yes)
	PLATFORM = x64
else
	PLATFORM = Win32
endif

ifeq ($(DEBUG),yes)
	CONFIGURATION = Debug
else
	CONFIGURATION = Release
endif

OBJ_DIR	           = $(BIN_DIR)/obj/$(CONFIGURATION)_$(PLATFORM)/ffmpeg/
TARGET_LIB_DIR     = $(BIN_DIR)/lib/$(CONFIGURATION)_$(PLATFORM)
LIB_LIBAVCODEC     = $(OBJ_DIR)libavcodec.a
LIB_LIBAVCODEC_B   = $(OBJ_DIR)libavcodec_b.a
LIB_LIBAVCODEC_BSF = $(OBJ_DIR)libavcodec_bsf.a
LIB_LIBAVFILTER    = $(OBJ_DIR)libavfilter.a
LIB_LIBAVUTIL      = $(OBJ_DIR)libavutil.a
LIB_LIBSWRESAMPLE  = $(OBJ_DIR)libswresample.a
LIB_LIBSWSCALE     = $(OBJ_DIR)libswscale.a
TARGET_LIB         = $(TARGET_LIB_DIR)/ffmpeg.lib
ARSCRIPT           = $(OBJ_DIR)script.ar

# Compiler and yasm flags
CFLAGS = -I. -I.. -Icompat/atomics/win32 -Icompat/windows \
	   -Ilibavcodec \
	   -I$(ZLIB_DIR) -I$(SPEEX_DIR) -I$(SOXR_DIR) -I$(DAV1_DIR) -I$(FFNVCODEC_DIR) -I$(UAVS3D_DIR) -I$(VVDEC_DIR) \
	   -DHAVE_AV_CONFIG_H -D_ISOC99_SOURCE -D_XOPEN_SOURCE=600 \
	   -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DOPJ_STATIC \
	   -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 \
	   -fomit-frame-pointer -std=c17 \
	   -fno-common -fno-ident -mthreads -Wno-discarded-qualifiers
YASMFLAGS = -I. -Pconfig.asm

ifeq ($(64BIT),yes)
	GCC_PREFIX  = x86_64-w64-mingw32-
	TARGET_OS   = x86_64-w64-mingw32
	CFLAGS     += -DWIN64 -D_WIN64 -DARCH_X86_64 -DPIC
	OPTFLAGS    = -m64 -fno-leading-underscore
	YASMFLAGS  += -f win32 -m amd64 -DWIN64=1 -DARCH_X86_32=0 -DARCH_X86_64=1 -DPIC
else
	TARGET_OS   = i686-w64-mingw32
	CFLAGS     += -DWIN32 -D_WIN32 -DARCH_X86_32
	OPTFLAGS    = -m32 -march=i686 -msse -msse2 -mfpmath=sse -mstackrealign
	YASMFLAGS  += -f win32 -m x86 -DWIN32=1 -DARCH_X86_32=1 -DARCH_X86_64=0 -DPREFIX
endif

ifeq ($(DEBUG),yes)
	CFLAGS     += -DDEBUG -D_DEBUG -g -Og
else
	CFLAGS     += -DNDEBUG -UDEBUG -U_DEBUG -O3 -fno-tree-vectorize -Wno-stringop-overflow
endif

# Object directories
OBJ_DIRS = $(OBJ_DIR) \
	$(OBJ_DIR)compat \
	$(OBJ_DIR)libavcodec \
	$(OBJ_DIR)libavcodec/bsf \
	$(OBJ_DIR)libavcodec/aac \
	$(OBJ_DIR)libavcodec/hevc \
	$(OBJ_DIR)libavcodec/vvc \
	$(OBJ_DIR)libavcodec/x86 \
	$(OBJ_DIR)libavcodec/x86/h26x \
	$(OBJ_DIR)libavcodec/x86/vvc \
	$(OBJ_DIR)libavfilter \
	$(OBJ_DIR)libavfilter/x86 \
	$(OBJ_DIR)libavutil \
	$(OBJ_DIR)libavutil/x86 \
	$(OBJ_DIR)libswresample \
	$(OBJ_DIR)libswresample/x86 \
	$(OBJ_DIR)libswscale \
	$(OBJ_DIR)libswscale/x86 \
	$(TARGET_LIB_DIR)

# Targets
all: make_objdirs $(LIB_LIBAVCODEC) $(LIB_LIBAVCODEC_B) $(LIB_LIBAVCODEC_BSF) $(LIB_LIBAVFILTER) $(LIB_LIBAVUTIL) $(LIB_LIBSWRESAMPLE) $(LIB_LIBSWSCALE) $(TARGET_LIB)

make_objdirs: $(OBJ_DIRS)
$(OBJ_DIRS):
	$(shell test -d $(@) || mkdir -p $(@))

clean:
	@rm -f $(TARGET_LIB)
	@rm -rf $(OBJ_DIR)

# Objects
SRCS_LC = \
	config.c \
	\
	libavcodec/8bps.c \
	libavcodec/aac_ac3_parser.c \
	libavcodec/aac_parser.c \
	libavcodec/aacps.c \
	libavcodec/aacps_common.c \
	libavcodec/aacps_fixed.c \
	libavcodec/aacps_float.c \
	libavcodec/aacpsdsp_fixed.c \
	libavcodec/aacpsdsp_float.c \
	libavcodec/aacsbr.c \
	libavcodec/aacsbr_fixed.c \
	libavcodec/aactab.c \
	libavcodec/ac3.c \
	libavcodec/ac3_channel_layout_tab.c \
	libavcodec/ac3_parser.c \
	libavcodec/ac3dec_data.c \
	libavcodec/ac3dec_float.c \
	libavcodec/ac3dsp.c \
	libavcodec/ac3enc.c \
	libavcodec/ac3enc_float.c \
	libavcodec/ac3tab.c \
	libavcodec/acelp_filters.c \
	libavcodec/acelp_pitch_delay.c \
	libavcodec/acelp_vectors.c \
	libavcodec/adpcm.c \
	libavcodec/aic.c \
	libavcodec/adpcm_data.c \
	libavcodec/adts_header.c \
	libavcodec/adts_parser.c \
	libavcodec/adx.c \
	libavcodec/adx_parser.c \
	libavcodec/adxdec.c \
	libavcodec/alac.c \
	libavcodec/alac_data.c \
	libavcodec/alacdsp.c \
	libavcodec/allcodecs.c \
	libavcodec/alsdec.c \
	libavcodec/amrnbdec.c \
	libavcodec/amrwbdec.c \
	libavcodec/aom_film_grain.c \
	libavcodec/apedec.c \
	libavcodec/atrac.c \
	libavcodec/atrac3.c \
	libavcodec/atrac3plus.c \
	libavcodec/atrac3plusdec.c \
	libavcodec/atrac3plusdsp.c \
	libavcodec/atrac9dec.c \
	libavcodec/atsc_a53.c \
	libavcodec/audiodsp.c \
	libavcodec/av1_parse.c \
	libavcodec/av1_parser.c \
	libavcodec/av1dec.c \
	libavcodec/avcodec.c \
	libavcodec/avdct.c \
	libavcodec/avfft.c \
	libavcodec/avs3_parser.c \
	libavcodec/bgmc.c \
	libavcodec/bink.c \
	libavcodec/binkaudio.c \
	libavcodec/binkdsp.c \
	libavcodec/bitstream.c \
	libavcodec/bitstream_filters.c \
	libavcodec/blockdsp.c \
	libavcodec/bsf.c \
	libavcodec/bswapdsp.c \
	libavcodec/cabac.c \
	libavcodec/canopus.c \
	libavcodec/cbrt_data.c \
	libavcodec/cbrt_data_fixed.c \
	libavcodec/cbs.c \
	libavcodec/cbs_av1.c \
	libavcodec/cbs_bsf.c \
	libavcodec/cbs_h2645.c \
	libavcodec/cbs_sei.c \
	libavcodec/celp_filters.c \
	libavcodec/celp_math.c \
	libavcodec/cfhd.c \
	libavcodec/cfhddata.c \
	libavcodec/cfhddsp.c \
	libavcodec/cinepak.c \
	libavcodec/cllc.c \
	libavcodec/codec_desc.c \
	libavcodec/codec_par.c \
	libavcodec/cook.c \
	libavcodec/cscd.c \
	libavcodec/d3d11va.c \
	libavcodec/d3d12va_av1.c \
	libavcodec/d3d12va_decode.c \
	libavcodec/d3d12va_h264.c \
	libavcodec/d3d12va_hevc.c \
	libavcodec/d3d12va_mpeg2.c \
	libavcodec/d3d12va_vc1.c \
	libavcodec/d3d12va_vp9.c \
	libavcodec/dca.c \
	libavcodec/dca_core.c \
	libavcodec/dca_exss.c \
	libavcodec/dca_lbr.c \
	libavcodec/dca_parser.c \
	libavcodec/dca_sample_rate_tab.c \
	libavcodec/dca_xll.c \
	libavcodec/dcaadpcm.c \
	libavcodec/dcadata.c \
	libavcodec/dcadct.c \
	libavcodec/dcadec.c \
	libavcodec/dcadsp.c \
	libavcodec/dcahuff.c \
	libavcodec/dct32_fixed.c \
	libavcodec/dct32_float.c \
	libavcodec/decode.c \
	libavcodec/dirac.c \
	libavcodec/dirac_arith.c \
	libavcodec/dirac_dwt.c \
	libavcodec/dirac_vlc.c \
	libavcodec/diracdec.c \
	libavcodec/diracdsp.c \
	libavcodec/diractab.c \
	libavcodec/dnxhd_parser.c \
	libavcodec/dnxhddata.c \
	libavcodec/dnxhddec.c \
	libavcodec/dovi_rpu.c \
	libavcodec/dovi_rpudec.c \
	libavcodec/dsd.c \
	libavcodec/dsddec.c \
	libavcodec/dstdec.c \
	libavcodec/dv.c \
	libavcodec/dv_profile.c \
	libavcodec/dvdata.c \
	libavcodec/dvdec.c \
	libavcodec/dxtory.c \
	libavcodec/dxva2.c \
	libavcodec/dxva2_av1.c \
	libavcodec/dxva2_h264.c \
	libavcodec/dxva2_hevc.c \
	libavcodec/dxva2_mpeg2.c \
	libavcodec/dxva2_vc1.c \
	libavcodec/dxva2_vp9.c \
	libavcodec/dynamic_hdr_vivid.c \
	libavcodec/eac3_data.c \
	libavcodec/eac3dec.c \
	libavcodec/elsdec.c \
	libavcodec/encode.c \
	libavcodec/error_resilience.c \
	libavcodec/exif.c \
	libavcodec/faandct.c \
	libavcodec/faanidct.c \
	libavcodec/fdctdsp.c \
	libavcodec/ffv1.c \
	libavcodec/ffv1dec.c \
	libavcodec/fic.c \
	libavcodec/flac.c \
	libavcodec/flac_parser.c \
	libavcodec/flacdata.c \
	libavcodec/flacdec.c \
	libavcodec/flacdsp.c \
	libavcodec/flashsv.c \
	libavcodec/float2half.c \
	libavcodec/flvdec.c \
	libavcodec/fmtconvert.c \
	libavcodec/frame_thread_encoder.c \
	libavcodec/fraps.c \
	libavcodec/g2meet.c \
	libavcodec/g726.c \
	libavcodec/get_buffer.c \
	libavcodec/golomb.c \
	libavcodec/h263.c \
	libavcodec/h263_parser.c \
	libavcodec/h263data.c \
	libavcodec/h263dec.c \
	libavcodec/h263dsp.c \
	libavcodec/h264_cabac.c \
	libavcodec/h264_cavlc.c \
	libavcodec/h264_direct.c \
	libavcodec/h264_loopfilter.c \
	libavcodec/h264_mb.c \
	libavcodec/h264_parse.c \
	libavcodec/h264_parser.c \
	libavcodec/h264_picture.c \
	libavcodec/h264_ps.c \
	libavcodec/h264_refs.c \
	libavcodec/h264_sei.c \
	libavcodec/h264_slice.c \
	libavcodec/h264dec.c \
	libavcodec/h264chroma.c \
	libavcodec/h264data.c \
	libavcodec/h264dsp.c \
	libavcodec/h264idct.c \
	libavcodec/h264pred.c \
	libavcodec/h264qpel.c \
	libavcodec/h2645_parse.c \
	libavcodec/h2645_sei.c \
	libavcodec/h2645_vui.c \
	libavcodec/h2645data.c \
	libavcodec/h274.c \
	libavcodec/half2float.c \
	libavcodec/hap.c \
	libavcodec/hapdec.c \
	libavcodec/hpeldsp.c \
	libavcodec/hq_hqa.c \
	libavcodec/hq_hqadsp.c \
	libavcodec/hqx.c \
	libavcodec/hqxdsp.c \
	libavcodec/hqxvlc.c \
	libavcodec/huffman.c \
	libavcodec/huffyuv.c \
	libavcodec/huffyuvdec.c \
	libavcodec/huffyuvdsp.c \
	libavcodec/idctdsp.c \
	libavcodec/imc.c \
	libavcodec/imgconvert.c \
	libavcodec/indeo3.c \
	libavcodec/indeo4.c \
	libavcodec/indeo5.c \
	libavcodec/intelh263dec.c \
	libavcodec/intrax8.c \
	libavcodec/intrax8dsp.c \
	libavcodec/ituh263dec.c \
	libavcodec/ivi.c \
	libavcodec/ivi_dsp.c \
	libavcodec/jpeg2000.c \
	libavcodec/jpeg2000_parser.c \
	libavcodec/jpeg2000dec.c \
	libavcodec/jpeg2000dsp.c \
	libavcodec/jpeg2000dwt.c \
	libavcodec/jpeg2000htdec.c \
	libavcodec/jpegls.c \
	libavcodec/jpeglsdec.c \
	libavcodec/jpegquanttables.c \
	libavcodec/jpegtables.c \
	libavcodec/jrevdct.c \
	libavcodec/kbdwin.c \
	libavcodec/lagarith.c \
	libavcodec/lagarithrac.c \
	libavcodec/latm_parser.c \
	libavcodec/libdav1d.c \
	libavcodec/libfdk-aacdec.c \
	libavcodec/libspeexdec.c \
	libavcodec/libuavs3d.c \
	libavcodec/libvvdec.c \
	libavcodec/lossless_audiodsp.c \
	libavcodec/lossless_videodsp.c \
	libavcodec/lsp.c \
	libavcodec/magicyuv.c \
	libavcodec/mathtables.c \
	libavcodec/me_cmp.c \
	libavcodec/metasound.c \
	libavcodec/mjpeg_parser.c \
	libavcodec/mjpegbdec.c \
	libavcodec/mjpegdec.c \
	libavcodec/mjpegdec_common.c \
	libavcodec/mlp.c \
	libavcodec/mlp_parse.c \
	libavcodec/mlp_parser.c \
	libavcodec/mlpdec.c \
	libavcodec/mlpdsp.c \
	libavcodec/mlz.c \
	libavcodec/motion_est.c \
	libavcodec/mpc.c \
	libavcodec/mpc7.c \
	libavcodec/mpc8.c \
	libavcodec/mpeg_er.c \
	libavcodec/mpeg12.c \
	libavcodec/mpeg12data.c \
	libavcodec/mpeg12dec.c \
	libavcodec/mpeg12framerate.c \
	libavcodec/mpeg4audio.c \
	libavcodec/mpeg4audio_sample_rates.c \
	libavcodec/mpeg4video.c \
	libavcodec/mpeg4video_parser.c \
	libavcodec/mpeg4videodec.c \
	libavcodec/mpeg4videodsp.c \
	libavcodec/mpeg4videoenc.c \
	libavcodec/mpegaudio.c \
	libavcodec/mpegaudio_parser.c \
	libavcodec/mpegaudiodata.c \
	libavcodec/mpegaudiodec_common.c \
	libavcodec/mpegaudiodec_common_tablegen.c \
	libavcodec/mpegaudiodec_fixed.c \
	libavcodec/mpegaudiodec_float.c \
	libavcodec/mpegaudiodecheader.c \
	libavcodec/mpegaudiodsp.c \
	libavcodec/mpegaudiodsp_data.c \
	libavcodec/mpegaudiodsp_fixed.c \
	libavcodec/mpegaudiodsp_float.c \
	libavcodec/mpegaudiotabs.c \
	libavcodec/mpegpicture.c \
	libavcodec/mpegutils.c \
	libavcodec/mpegvideo.c \
	libavcodec/mpegvideo_dec.c \
	libavcodec/mpegvideo_motion.c \
	libavcodec/mpegvideo_parser.c \
	libavcodec/mpegvideodata.c \
	libavcodec/mpegvideoencdsp.c \
	libavcodec/mqc.c \
	libavcodec/mqcdec.c \
	libavcodec/msmpeg4.c \
	libavcodec/msmpeg4_vc1_data.c \
	libavcodec/msmpeg4data.c \
	libavcodec/msmpeg4dec.c \
	libavcodec/msrle.c \
	libavcodec/msrledec.c \
	libavcodec/mss1.c \
	libavcodec/mss12.c \
	libavcodec/mss2.c \
	libavcodec/mss2dsp.c \
	libavcodec/mss3.c \
	libavcodec/mss34dsp.c \
	libavcodec/mss4.c \
	libavcodec/msvideo1.c \
	libavcodec/nellymoser.c \
	libavcodec/nellymoserdec.c \
	\
	libavcodec/aac/aacdec.c \
	libavcodec/aac/aacdec_ac.c \
	libavcodec/aac/aacdec_fixed.c \
	libavcodec/aac/aacdec_float.c \
	libavcodec/aac/aacdec_lpd.c \
	libavcodec/aac/aacdec_tab.c \
	libavcodec/aac/aacdec_usac.c \
	\
	libavcodec/hevc/cabac.c \
	libavcodec/hevc/data.c \
	libavcodec/hevc/dsp.c \
	libavcodec/hevc/filter.c \
	libavcodec/hevc/hevcdec.c \
	libavcodec/hevc/mvs.c \
	libavcodec/hevc/parse.c \
	libavcodec/hevc/parser.c \
	libavcodec/hevc/pred.c \
	libavcodec/hevc/ps.c \
	libavcodec/hevc/refs.c \
	libavcodec/hevc/sei.c

SRCS_LC_B = \
	libavcodec/nvdec.c \
	libavcodec/nvdec_av1.c \
	libavcodec/nvdec_h264.c \
	libavcodec/nvdec_hevc.c \
	libavcodec/nvdec_mpeg12.c \
	libavcodec/nvdec_vc1.c \
	libavcodec/nvdec_vp9.c \
	libavcodec/options.c \
	libavcodec/opus_celt.c \
	libavcodec/opus_parse.c \
	libavcodec/opus_parser.c \
	libavcodec/opus_pvq.c \
	libavcodec/opus_rc.c \
	libavcodec/opus_silk.c \
	libavcodec/opusdec.c \
	libavcodec/opusdec_celt.c \
	libavcodec/opusdsp.c \
	libavcodec/opustab.c \
	libavcodec/packet.c \
	libavcodec/parser.c \
	libavcodec/parsers.c \
	libavcodec/pcm.c \
	libavcodec/pixblockdsp.c \
	libavcodec/png.c \
	libavcodec/pngdec.c \
	libavcodec/pngdsp.c \
	libavcodec/profiles.c \
	libavcodec/pthread.c \
	libavcodec/pthread_frame.c \
	libavcodec/pthread_slice.c \
	libavcodec/qdm2.c \
	libavcodec/qdmc.c \
	libavcodec/qpeldsp.c \
	libavcodec/qtrle.c \
	libavcodec/proresdata.c \
	libavcodec/proresdec.c \
	libavcodec/proresdsp.c \
	libavcodec/r210dec.c \
	libavcodec/ra144.c \
	libavcodec/ra144dec.c \
	libavcodec/ra288.c \
	libavcodec/ralf.c \
	libavcodec/rangecoder.c \
	libavcodec/raw.c \
	libavcodec/rawdec.c \
	libavcodec/refstruct.c \
	libavcodec/rl.c \
	libavcodec/rpza.c \
	libavcodec/rv10.c \
	libavcodec/rv30.c \
	libavcodec/rv30dsp.c \
	libavcodec/rv34.c \
	libavcodec/rv34dsp.c \
	libavcodec/rv40.c \
	libavcodec/rv40dsp.c \
	libavcodec/s302m.c \
	libavcodec/sbrdsp.c \
	libavcodec/sbrdsp_fixed.c \
	libavcodec/shorten.c \
	libavcodec/simple_idct.c \
	libavcodec/sinewin.c \
	libavcodec/sipr.c \
	libavcodec/sipr16k.c \
	libavcodec/snappy.c \
	libavcodec/snow_dwt.c \
	libavcodec/snow.c \
	libavcodec/sp5xdec.c \
	libavcodec/speedhq.c \
	libavcodec/speedhqdec.c \
	libavcodec/startcode.c \
	libavcodec/svq1.c \
	libavcodec/svq1dec.c \
	libavcodec/svq3.c \
	libavcodec/synth_filter.c \
	libavcodec/tak.c \
	libavcodec/tak_parser.c \
	libavcodec/takdec.c \
	libavcodec/takdsp.c \
	libavcodec/texturedsp.c \
	libavcodec/threadprogress.c \
	libavcodec/tiff_common.c \
	libavcodec/to_upper4.c \
	libavcodec/tpeldsp.c \
	libavcodec/truespeech.c \
	libavcodec/tscc.c \
	libavcodec/tscc2.c \
	libavcodec/tta.c \
	libavcodec/ttadata.c \
	libavcodec/ttadsp.c \
	libavcodec/twinvq.c \
	libavcodec/utvideodec.c \
	libavcodec/utvideodsp.c \
	libavcodec/utils.c \
	libavcodec/v210dec.c \
	libavcodec/v410dec.c \
	libavcodec/vc1.c \
	libavcodec/vc1_block.c \
	libavcodec/vc1_loopfilter.c \
	libavcodec/vc1_mc.c \
	libavcodec/vc1_parser.c \
	libavcodec/vc1_pred.c \
	libavcodec/vc1data.c \
	libavcodec/vc1dec.c \
	libavcodec/vc1dsp.c \
	libavcodec/version.c \
	libavcodec/videodsp.c \
	libavcodec/vlc.c \
	libavcodec/vmnc.c \
	libavcodec/vorbis.c \
	libavcodec/vorbis_data.c \
	libavcodec/vorbis_parser.c \
	libavcodec/vorbisdec.c \
	libavcodec/vorbisdsp.c \
	libavcodec/vp3.c \
	libavcodec/vp3_parser.c \
	libavcodec/vp3dsp.c \
	libavcodec/vp5.c \
	libavcodec/vp56.c \
	libavcodec/vp56data.c \
	libavcodec/vp56dsp.c \
	libavcodec/vp6.c \
	libavcodec/vp6dsp.c \
	libavcodec/vp8.c \
	libavcodec/vp8_parser.c \
	libavcodec/vp8data.c \
	libavcodec/vp8dsp.c \
	libavcodec/vp9.c \
	libavcodec/vp9_parser.c \
	libavcodec/vp9block.c \
	libavcodec/vp9data.c \
	libavcodec/vp9dsp.c \
	libavcodec/vp9dsp_8bpp.c \
	libavcodec/vp9dsp_10bpp.c \
	libavcodec/vp9dsp_12bpp.c \
	libavcodec/vp9lpf.c \
	libavcodec/vp9mvs.c \
	libavcodec/vp9prob.c \
	libavcodec/vp9recon.c \
	libavcodec/vpx_rac.c\
	libavcodec/vvc_parser.c \
	libavcodec/wavpack.c \
	libavcodec/wavpackdata.c \
	libavcodec/wma.c \
	libavcodec/wma_common.c \
	libavcodec/wma_freqs.c \
	libavcodec/wmadec.c \
	libavcodec/wmalosslessdec.c \
	libavcodec/wmaprodec.c \
	libavcodec/wmavoice.c \
	libavcodec/wmv2.c \
	libavcodec/wmv2data.c \
	libavcodec/wmv2dec.c \
	libavcodec/wmv2dsp.c \
	libavcodec/xiph.c \
	libavcodec/xvididct.c \
	libavcodec/zlib_wrapper.c \
	\
	libavcodec/vvc/dec.c \
	libavcodec/vvc/dsp.c \
	libavcodec/vvc/cabac.c \
	libavcodec/vvc/ctu.c \
	libavcodec/vvc/data.c \
	libavcodec/vvc/filter.c \
	libavcodec/vvc/inter.c \
	libavcodec/vvc/intra.c \
	libavcodec/vvc/intra_utils.c \
	libavcodec/vvc/itx_1d.c \
	libavcodec/vvc/mvs.c \
	libavcodec/vvc/ps.c \
	libavcodec/vvc/refs.c \
	libavcodec/vvc/thread.c \
	\
	libavcodec/x86/aacpsdsp_init.c \
	libavcodec/x86/ac3dsp_init.c \
	libavcodec/x86/alacdsp_init.c \
	libavcodec/x86/audiodsp_init.c \
	libavcodec/x86/blockdsp_init.c \
	libavcodec/x86/bswapdsp_init.c \
	libavcodec/x86/cfhddsp_init.c \
	libavcodec/x86/constants.c \
	libavcodec/x86/dcadsp_init.c \
	libavcodec/x86/dirac_dwt_init.c \
	libavcodec/x86/diracdsp_init.c \
	libavcodec/x86/fdct.c \
	libavcodec/x86/fdctdsp_init.c \
	libavcodec/x86/flacdsp_init.c \
	libavcodec/x86/fmtconvert_init.c \
	libavcodec/x86/h263dsp_init.c \
	libavcodec/x86/h264_cabac.c \
	libavcodec/x86/h264_intrapred_init.c \
	libavcodec/x86/h264chroma_init.c \
	libavcodec/x86/h264dsp_init.c \
	libavcodec/x86/hevc_idct_intrinsic.c \
	libavcodec/x86/hevc_intra_intrinsic.c \
	libavcodec/x86/hevcdsp_init.c \
	libavcodec/x86/h264_qpel.c \
	libavcodec/x86/hpeldsp_init.c \
	libavcodec/x86/huffyuvdsp_init.c \
	libavcodec/x86/idctdsp_init.c \
	libavcodec/x86/jpeg2000dsp_init.c \
	libavcodec/x86/lossless_audiodsp_init.c \
	libavcodec/x86/lossless_videodsp_init.c \
	libavcodec/x86/me_cmp_init.c \
	libavcodec/x86/mlpdsp_init.c \
	libavcodec/x86/mpeg4videodsp.c \
	libavcodec/x86/mpegaudiodsp.c \
	libavcodec/x86/mpegvideo.c \
	libavcodec/x86/mpegvideoencdsp_init.c \
	libavcodec/x86/opusdsp_init.c \
	libavcodec/x86/pixblockdsp_init.c \
	libavcodec/x86/pngdsp_init.c \
	libavcodec/x86/proresdsp_init.c \
	libavcodec/x86/qpeldsp_init.c \
	libavcodec/x86/rv34dsp_init.c \
	libavcodec/x86/rv40dsp_init.c \
	libavcodec/x86/sbrdsp_init.c \
	libavcodec/x86/snowdsp.c \
	libavcodec/x86/synth_filter_init.c \
	libavcodec/x86/takdsp_init.c \
	libavcodec/x86/ttadsp_init.c \
	libavcodec/x86/utvideodsp_init.c \
	libavcodec/x86/v210-init.c \
	libavcodec/x86/vc1dsp_init.c \
	libavcodec/x86/vc1dsp_mmx.c \
	libavcodec/x86/videodsp_init.c \
	libavcodec/x86/vorbisdsp_init.c \
	libavcodec/x86/vp3dsp_init.c \
	libavcodec/x86/vp6dsp_init.c \
	libavcodec/x86/vp8dsp_init.c \
	libavcodec/x86/vp9dsp_init.c \
	libavcodec/x86/vp9dsp_init_10bpp.c \
	libavcodec/x86/vp9dsp_init_12bpp.c \
	libavcodec/x86/vp9dsp_init_16bpp.c \
	libavcodec/x86/xvididct_init.c \
	\
	libavcodec/x86/h26x/h2656dsp.c \
	\
	libavcodec/x86/vvc/vvcdsp_init.c

SRCS_LC_BSF = \
	libavcodec/bsf/aac_adtstoasc.c \
	libavcodec/bsf/av1_frame_split.c \
	libavcodec/bsf/chomp.c \
	libavcodec/bsf/dca_core.c \
	libavcodec/bsf/dump_extradata.c \
	libavcodec/bsf/extract_extradata.c \
	libavcodec/bsf/h264_mp4toannexb.c \
	libavcodec/bsf/h266_metadata.c \
	libavcodec/bsf/hevc_mp4toannexb.c \
	libavcodec/bsf/imx_dump_header.c \
	libavcodec/bsf/mjpeg2jpeg.c \
	libavcodec/bsf/mjpega_dump_header.c \
	libavcodec/bsf/movsub.c \
	libavcodec/bsf/mpeg4_unpack_bframes.c \
	libavcodec/bsf/noise.c \
	libavcodec/bsf/null.c \
	libavcodec/bsf/remove_extradata.c \
	libavcodec/bsf/vp9_raw_reorder.c \
	libavcodec/bsf/vp9_superframe.c \
	libavcodec/bsf/vp9_superframe_split.c \
	libavcodec/bsf/vvc_mp4toannexb.c

SRCS_LF = \
	libavfilter/af_aresample.c \
	libavfilter/af_atempo.c \
	libavfilter/af_biquads.c \
	libavfilter/af_compand.c \
	libavfilter/allfilters.c \
	libavfilter/audio.c \
	libavfilter/avfilter.c \
	libavfilter/avfiltergraph.c \
	libavfilter/buffersink.c \
	libavfilter/buffersrc.c \
	libavfilter/formats.c \
	libavfilter/framepool.c \
	libavfilter/framequeue.c \
	libavfilter/graphparser.c \
	libavfilter/pthread.c \
	libavfilter/version.c \
	libavfilter/video.c

SRCS_LU = \
	libavutil/ambient_viewing_environment.c \
	libavutil/audio_fifo.c \
	libavutil/avsscanf.c \
	libavutil/avstring.c \
	libavutil/bprint.c \
	libavutil/buffer.c \
	libavutil/channel_layout.c \
	libavutil/cpu.c \
	libavutil/crc.c \
	libavutil/csp.c \
	libavutil/dict.c \
	libavutil/display.c \
	libavutil/dovi_meta.c \
	libavutil/downmix_info.c \
	libavutil/error.c \
	libavutil/eval.c \
	libavutil/executor.c \
	libavutil/fifo.c \
	libavutil/file_open.c \
	libavutil/film_grain_params.c \
	libavutil/fixed_dsp.c \
	libavutil/float_dsp.c \
	libavutil/float2half.c \
	libavutil/frame.c \
	libavutil/half2float.c \
	libavutil/hdr_dynamic_metadata.c \
	libavutil/hdr_dynamic_vivid_metadata.c \
	libavutil/hwcontext.c \
	libavutil/hwcontext_cuda.c \
	libavutil/hwcontext_d3d11va.c \
	libavutil/hwcontext_d3d12va.c \
	libavutil/hwcontext_dxva2.c \
	libavutil/imgutils.c \
	libavutil/integer.c \
	libavutil/intmath.c \
	libavutil/lfg.c \
	libavutil/lls.c \
	libavutil/log.c \
	libavutil/log2_tab.c \
	libavutil/lzo.c \
	libavutil/mastering_display_metadata.c \
	libavutil/mathematics.c \
	libavutil/md5.c \
	libavutil/mem.c \
	libavutil/opt.c \
	libavutil/parseutils.c \
	libavutil/pixdesc.c \
	libavutil/random_seed.c \
	libavutil/rational.c \
	libavutil/reverse.c \
	libavutil/samplefmt.c \
	libavutil/sha.c \
	libavutil/slicethread.c \
	libavutil/stereo3d.c \
	libavutil/threadmessage.c \
	libavutil/time.c \
	libavutil/timecode.c \
	libavutil/timestamp.c \
	libavutil/tx.c \
	libavutil/tx_double.c \
	libavutil/tx_float.c \
	libavutil/tx_int32.c \
	libavutil/utils.c \
	libavutil/version.c \
	libavutil/video_enc_params.c \
	\
	libavutil/x86/cpu.c \
	libavutil/x86/fixed_dsp_init.c \
	libavutil/x86/float_dsp_init.c \
	libavutil/x86/imgutils_init.c \
	libavutil/x86/lls_init.c \
	libavutil/x86/tx_float_init.c

SRCS_LR = \
	libswresample/audioconvert.c \
	libswresample/dither.c\
	libswresample/options.c \
	libswresample/rematrix.c \
	libswresample/resample.c \
	libswresample/resample_dsp.c \
	libswresample/soxr_resample.c \
	libswresample/swresample.c \
	libswresample/swresample_frame.c \
	libswresample/version.c \
	\
	libswresample/x86/audio_convert_init.c \
	libswresample/x86/rematrix_init.c \
	libswresample/x86/resample_init.c

SRCS_LS = \
	libswscale/alphablend.c \
	libswscale/gamma.c \
	libswscale/half2float.c \
	libswscale/hscale.c \
	libswscale/hscale_fast_bilinear.c \
	libswscale/input.c \
	libswscale/options.c \
	libswscale/output.c \
	libswscale/rgb2rgb.c \
	libswscale/slice.c \
	libswscale/swscale.c \
	libswscale/swscale_unscaled.c \
	libswscale/utils.c \
	libswscale/version.c \
	libswscale/vscale.c \
	libswscale/yuv2rgb.c \
	\
	libswscale/x86/hscale_fast_bilinear_simd.c \
	libswscale/x86/rgb2rgb.c \
	libswscale/x86/swscale.c \
	libswscale/x86/yuv2rgb.c

# Yasm objects
SRCS_YASM_LC = \
	libavcodec/x86/aacpsdsp.asm \
	libavcodec/x86/ac3dsp.asm \
	libavcodec/x86/ac3dsp_downmix.asm \
	libavcodec/x86/alacdsp.asm \
	libavcodec/x86/audiodsp.asm \
	libavcodec/x86/blockdsp.asm \
	libavcodec/x86/bswapdsp.asm \
	libavcodec/x86/cfhddsp.asm \
	libavcodec/x86/dcadsp.asm \
	libavcodec/x86/dct32.asm \
	libavcodec/x86/dirac_dwt.asm \
	libavcodec/x86/diracdsp.asm \
	libavcodec/x86/flacdsp.asm \
	libavcodec/x86/fmtconvert.asm \
	libavcodec/x86/fpel.asm \
	libavcodec/x86/h263_loopfilter.asm \
	libavcodec/x86/h264_chromamc.asm \
	libavcodec/x86/h264_chromamc_10bit.asm \
	libavcodec/x86/h264_deblock.asm \
	libavcodec/x86/h264_deblock_10bit.asm \
	libavcodec/x86/h264_idct.asm \
	libavcodec/x86/h264_idct_10bit.asm \
	libavcodec/x86/h264_intrapred.asm \
	libavcodec/x86/h264_intrapred_10bit.asm \
	libavcodec/x86/h264_qpel_10bit.asm \
	libavcodec/x86/h264_qpel_8bit.asm \
	libavcodec/x86/h264_weight.asm \
	libavcodec/x86/h264_weight_10bit.asm \
	libavcodec/x86/hevc_add_res.asm \
	libavcodec/x86/hevc_deblock.asm \
	libavcodec/x86/hevc_idct.asm \
	libavcodec/x86/hevc_mc.asm \
	libavcodec/x86/hevc_sao.asm \
	libavcodec/x86/hevc_sao_10bit.asm \
	libavcodec/x86/hpeldsp.asm \
	libavcodec/x86/huffyuvdsp.asm \
	libavcodec/x86/idctdsp.asm \
	libavcodec/x86/imdct36.asm \
	libavcodec/x86/jpeg2000dsp.asm \
	libavcodec/x86/lossless_audiodsp.asm \
	libavcodec/x86/lossless_videodsp.asm \
	libavcodec/x86/me_cmp.asm \
	libavcodec/x86/mlpdsp.asm \
	libavcodec/x86/mpegvideoencdsp.asm \
	libavcodec/x86/opusdsp.asm \
	libavcodec/x86/pixblockdsp.asm \
	libavcodec/x86/pngdsp.asm \
	libavcodec/x86/proresdsp.asm \
	libavcodec/x86/qpel.asm \
	libavcodec/x86/qpeldsp.asm \
	libavcodec/x86/rv34dsp.asm \
	libavcodec/x86/rv40dsp.asm \
	libavcodec/x86/sbrdsp.asm \
	libavcodec/x86/simple_idct.asm \
	libavcodec/x86/simple_idct10.asm \
	libavcodec/x86/synth_filter.asm \
	libavcodec/x86/takdsp.asm \
	libavcodec/x86/ttadsp.asm \
	libavcodec/x86/utvideodsp.asm \
	libavcodec/x86/v210.asm \
	libavcodec/x86/vc1dsp_loopfilter.asm \
	libavcodec/x86/vc1dsp_mc.asm \
	libavcodec/x86/videodsp.asm \
	libavcodec/x86/vorbisdsp.asm \
	libavcodec/x86/vp3dsp.asm \
	libavcodec/x86/vp6dsp.asm \
	libavcodec/x86/vp8dsp.asm \
	libavcodec/x86/vp8dsp_loopfilter.asm \
	libavcodec/x86/vp9intrapred.asm \
	libavcodec/x86/vp9intrapred_16bpp.asm \
	libavcodec/x86/vp9itxfm.asm \
	libavcodec/x86/vp9itxfm_16bpp.asm \
	libavcodec/x86/vp9lpf.asm \
	libavcodec/x86/vp9lpf_16bpp.asm \
	libavcodec/x86/vp9mc.asm \
	libavcodec/x86/vp9mc_16bpp.asm \
	libavcodec/x86/xvididct.asm \
	\
	libavcodec/x86/h26x/h2656_inter.asm \
	\
	libavcodec/x86/vvc/vvc_alf.asm \
	libavcodec/x86/vvc/vvc_mc.asm \
	libavcodec/x86/vvc/vvc_sad.asm

SRCS_YASM_LF = 

SRCS_YASM_LU = \
	libavutil/x86/cpuid.asm \
	libavutil/x86/emms.asm \
	libavutil/x86/fixed_dsp.asm \
	libavutil/x86/float_dsp.asm \
	libavutil/x86/imgutils.asm \
	libavutil/x86/lls.asm \
	libavutil/x86/tx_float.asm

SRCS_YASM_LR = \
	libswresample/x86/audio_convert.asm \
	libswresample/x86/rematrix.asm \
	libswresample/x86/resample.asm

SRCS_YASM_LS = \
	libswscale/x86/input.asm \
	libswscale/x86/output.asm \
	libswscale/x86/rgb_2_rgb.asm \
	libswscale/x86/scale.asm \
	libswscale/x86/scale_avx2.asm \
	libswscale/x86/yuv_2_rgb.asm \
	libswscale/x86/yuv2yuvX.asm

OBJS_LC = \
	$(SRCS_LC:%.c=$(OBJ_DIR)%.o) \
	$(SRCS_YASM_LC:%.asm=$(OBJ_DIR)%.o)

OBJS_LC_B = \
	$(SRCS_LC_B:%.c=$(OBJ_DIR)%.o)
	
OBJS_LC_BSF = \
	$(SRCS_LC_BSF:%.c=$(OBJ_DIR)%.o)

OBJS_LF = \
	$(SRCS_LF:%.c=$(OBJ_DIR)%.o) \
	$(SRCS_YASM_LF:%.asm=$(OBJ_DIR)%.o)

OBJS_LU = \
	$(SRCS_LU:%.c=$(OBJ_DIR)%.o) \
	$(SRCS_YASM_LU:%.asm=$(OBJ_DIR)%.o)

OBJS_LR = \
	$(SRCS_LR:%.c=$(OBJ_DIR)%.o) \
	$(SRCS_YASM_LR:%.asm=$(OBJ_DIR)%.o)

OBJS_LS = \
	$(SRCS_LS:%.c=$(OBJ_DIR)%.o) \
	$(SRCS_YASM_LS:%.asm=$(OBJ_DIR)%.o)

# Commands
$(OBJ_DIR)%.o: %.c
	@echo $<
	@$(GCC_PREFIX)gcc -c $(CFLAGS) $(OPTFLAGS) -MMD -Wno-deprecated-declarations -Wno-pointer-to-int-cast -o $@ $<

$(OBJ_DIR)%.o: %.asm
	@echo $<
	@yasm $(YASMFLAGS) -I$(<D)/ -o $@ $<

$(LIB_LIBAVCODEC): $(OBJS_LC)
	@echo $@
	@$(GCC_PREFIX)ar rc $@ $(OBJS_LC)

$(LIB_LIBAVCODEC_B): $(OBJS_LC_B)
	@echo $@
	@$(GCC_PREFIX)ar rc $@ $(OBJS_LC_B)
	
$(LIB_LIBAVCODEC_BSF): $(OBJS_LC_BSF)
	@echo $@
	@$(GCC_PREFIX)ar rc $@ $(OBJS_LC_BSF)

$(LIB_LIBAVFILTER): $(OBJS_LF)
	@echo $@
	@$(GCC_PREFIX)ar rc $@ $(OBJS_LF)

$(LIB_LIBAVUTIL): $(OBJS_LU)
	@echo $@
	@$(GCC_PREFIX)ar rc $@ $(OBJS_LU)

$(LIB_LIBSWSCALE): $(OBJS_LS)
	@echo $@
	@$(GCC_PREFIX)ar rc $@ $(OBJS_LS)

$(LIB_LIBSWRESAMPLE): $(OBJS_LR)
	@echo $@
	@$(GCC_PREFIX)ar rc $@ $(OBJS_LR)

$(TARGET_LIB): $(LIB_LIBAVCODEC) $(LIB_LIBAVCODEC_B) $(LIB_LIBAVCODEC_BSF) $(LIB_LIBAVFILTER) $(LIB_LIBAVUTIL) $(LIB_LIBSWRESAMPLE) $(LIB_LIBSWSCALE)
	@rm -f $(ARSCRIPT)
	@echo "CREATE $@"                    >> $(ARSCRIPT)
	@echo "ADDLIB $(LIB_LIBAVCODEC)"     >> $(ARSCRIPT)
	@echo "ADDLIB $(LIB_LIBAVCODEC_B)"   >> $(ARSCRIPT)
	@echo "ADDLIB $(LIB_LIBAVCODEC_BSF)" >> $(ARSCRIPT)	
	@echo "ADDLIB $(LIB_LIBAVFILTER)"    >> $(ARSCRIPT)
	@echo "ADDLIB $(LIB_LIBSWSCALE)"     >> $(ARSCRIPT)
	@echo "ADDLIB $(LIB_LIBAVUTIL)"      >> $(ARSCRIPT)
	@echo "ADDLIB $(LIB_LIBSWRESAMPLE)"  >> $(ARSCRIPT)
	@echo "SAVE"                         >> $(ARSCRIPT)
	@echo "END"                          >> $(ARSCRIPT)
	@$(GCC_PREFIX)ar -M < $(ARSCRIPT)

-include $(SRCS_LC:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LC_B:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LC_BSF:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LF:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LU:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LR:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LS:%.c=$(OBJ_DIR)%.d)

.PHONY: clean make_objdirs $(OBJ_DIRS)
