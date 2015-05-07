BIN_DIR  = ../../../bin13
ZLIB_DIR = ../zlib
OPENJPEG_DIR = ../openjpeg
SPEEX_DIR = ../speex

ifeq ($(64BIT),yes)
	MY_ARCH = x64
else
	MY_ARCH = Win32
endif

ifeq ($(DEBUG),yes)
	MY_DIR_PREFIX = Debug
else
	MY_DIR_PREFIX = Release
endif

OBJ_DIR		= $(BIN_DIR)/obj/$(MY_DIR_PREFIX)_$(MY_ARCH)/ffmpeg/
TARGET_LIB_DIR = $(BIN_DIR)/lib/$(MY_DIR_PREFIX)_$(MY_ARCH)
LIB_LIBAVCODEC = $(OBJ_DIR)libavcodec.a
LIB_LIBAVFILTER = $(OBJ_DIR)libavfilter.a
LIB_LIBAVRESAMPLE = $(OBJ_DIR)libavresample.a
LIB_LIBAVUTIL = $(OBJ_DIR)libavutil.a
LIB_LIBSWRESAMPLE = $(OBJ_DIR)libswresample.a
LIB_LIBSWSCALE = $(OBJ_DIR)libswscale.a
TARGET_LIB	 = $(TARGET_LIB_DIR)/ffmpeg.lib

# Compiler and yasm flags
CFLAGS	= -I. -I.. -Ilibavcodec -Ilibavutil -I$(ZLIB_DIR) -I$(OPENJPEG_DIR) -I$(SPEEX_DIR)\
		-DHAVE_AV_CONFIG_H -D_ISOC99_SOURCE -D_XOPEN_SOURCE=600 \
		-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 \
		-fomit-frame-pointer -std=gnu99 \
		-fno-common -mthreads
YASMFLAGS = -I. -Ilibavutil/x86 -Pconfig.asm

ifeq ($(64BIT),yes)
	FFMPEG_PREFIX = x86_64-w64-mingw32-
	TARGET_OS	 = x86_64-w64-mingw32
	CFLAGS		+= -DWIN64 -D_WIN64 -DARCH_X86_64 -DPIC
	OPTFLAGS	 = -m64 -fno-leading-underscore -msse2
	YASMFLAGS	+= -f win32 -m amd64 -DWIN64=1 -DARCH_X86_32=0 -DARCH_X86_64=1 -DPIC
else
	TARGET_OS	 = i686-w64-mingw32
	CFLAGS		+= -DWIN32 -D_WIN32 -DARCH_X86_32
	OPTFLAGS	 = -m32 -march=i686 -msse -mfpmath=sse,387
	YASMFLAGS	+= -f win32 -m x86 -DWIN32=1 -DARCH_X86_32=1 -DARCH_X86_64=0 -DPREFIX
endif

ifeq ($(DEBUG),yes)
	CFLAGS		+= -DDEBUG -D_DEBUG -g -Og
else
	CFLAGS		+= -DNDEBUG -UDEBUG -U_DEBUG -O2
endif

# Object directories
OBJ_DIRS = $(OBJ_DIR) \
	$(OBJ_DIR)compat \
	$(OBJ_DIR)libavcodec \
	$(OBJ_DIR)libavcodec/x86 \
	$(OBJ_DIR)libavfilter \
	$(OBJ_DIR)libavresample \
	$(OBJ_DIR)libavresample/x86 \
	$(OBJ_DIR)libavutil \
	$(OBJ_DIR)libavutil/x86 \
	$(OBJ_DIR)libswresample \
	$(OBJ_DIR)libswresample/x86 \
	$(OBJ_DIR)libswscale \
	$(OBJ_DIR)libswscale/x86 \
	$(TARGET_LIB_DIR)

# Targets
all: make_objdirs $(LIB_LIBAVCODEC) $(LIB_LIBAVFILTER) $(LIB_LIBSWSCALE) $(LIB_LIBAVUTIL) $(LIB_LIBAVRESAMPLE) $(LIB_LIBSWRESAMPLE)

make_objdirs: $(OBJ_DIRS)
$(OBJ_DIRS):
	$(shell test -d $(@) || mkdir -p $(@))

clean:
	rm -f $(TARGET_LIB)
	rm -rf $(OBJ_DIR)

# Objects
SRCS_LC = \
	config.c \
	\
	libavcodec/8bps.c \
	libavcodec/aac_ac3_parser.c \
	libavcodec/aac_parser.c \
	libavcodec/aacadtsdec.c \
	libavcodec/aacdec.c \
	libavcodec/aacps.c \
	libavcodec/aacpsdsp.c \
	libavcodec/aacsbr.c \
	libavcodec/aactab.c \
	libavcodec/ac3.c \
	libavcodec/ac3_parser.c \
	libavcodec/ac3dec.c \
	libavcodec/ac3dec_data.c \
	libavcodec/ac3dec_fixed.c \
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
	libavcodec/adx.c \
	libavcodec/adx_parser.c \
	libavcodec/adxdec.c \
	libavcodec/alac.c \
	libavcodec/alac_data.c \
	libavcodec/allcodecs.c \
	libavcodec/alsdec.c \
	libavcodec/amrnbdec.c \
	libavcodec/amrwbdec.c \
	libavcodec/apedec.c \
	libavcodec/atrac.c \
	libavcodec/atrac3.c \
	libavcodec/atrac3plus.c \
	libavcodec/atrac3plusdec.c \
	libavcodec/atrac3plusdsp.c \
	libavcodec/audiodsp.c \
	libavcodec/avdct.c \
	libavcodec/avfft.c \
	libavcodec/avpacket.c \
	libavcodec/avpicture.c \
	libavcodec/bgmc.c \
	libavcodec/bink.c \
	libavcodec/binkaudio.c \
	libavcodec/binkdsp.c \
	libavcodec/bitstream.c \
	libavcodec/blockdsp.c \
	libavcodec/bswapdsp.c \
	libavcodec/cabac.c \
	libavcodec/canopus.c \
	libavcodec/celp_filters.c \
	libavcodec/celp_math.c \
	libavcodec/cinepak.c \
	libavcodec/cllc.c \
	libavcodec/codec_desc.c \
	libavcodec/cook.c \
	libavcodec/cscd.c \
	libavcodec/dca.c \
	libavcodec/dca_exss.c \
	libavcodec/dca_parser.c \
	libavcodec/dca_xll.c \
	libavcodec/dcadata.c \
	libavcodec/dcadec.c \
	libavcodec/dcadsp.c \
	libavcodec/dct.c \
	libavcodec/dct32_template.c \
	libavcodec/dct32_float.c \
	libavcodec/dirac.c \
	libavcodec/dirac_arith.c \
	libavcodec/dirac_dwt.c \
	libavcodec/diracdec.c \
	libavcodec/diracdsp.c \
	libavcodec/dnxhd_parser.c \
	libavcodec/dnxhddata.c \
	libavcodec/dnxhddec.c \
	libavcodec/dsddec.c \
	libavcodec/dv.c \
	libavcodec/dv_profile.c \
	libavcodec/dv_tablegen.c \
	libavcodec/dvdata.c \
	libavcodec/dvdec.c \
	libavcodec/eac3_data.c \
	libavcodec/eac3enc.c \
	libavcodec/eac3dec.c \
	libavcodec/error_resilience.c \
	libavcodec/exif.c \
	libavcodec/faandct.c \
	libavcodec/faanidct.c \
	libavcodec/fdctdsp.c \
	libavcodec/fft_template.c \
	libavcodec/fft_fixed.c \
	libavcodec/fft_fixed_32.c \
	libavcodec/fft_float.c \
	libavcodec/fft_init_table.c \
	libavcodec/ffv1.c \
	libavcodec/ffv1dec.c \
	libavcodec/flac.c \
	libavcodec/flacdata.c \
	libavcodec/flacdec.c \
	libavcodec/flacdsp.c \
	libavcodec/flacdsp_template.c \
	libavcodec/flashsv.c \
	libavcodec/flvdec.c \
	libavcodec/fmtconvert.c \
	libavcodec/frame_thread_encoder.c \
	libavcodec/fraps.c \
	libavcodec/g2meet.c \
	libavcodec/golomb.c \
	libavcodec/h263.c \
	libavcodec/h263_parser.c \
	libavcodec/h263dec.c \
	libavcodec/h263dsp.c \
	libavcodec/h264.c \
	libavcodec/h264_cabac.c \
	libavcodec/h264_cavlc.c \
	libavcodec/h264_direct.c \
	libavcodec/h264_loopfilter.c \
	libavcodec/h264_mb.c \
	libavcodec/h264_mp4toannexb_bsf.c \
	libavcodec/h264_parser.c \
	libavcodec/h264_picture.c \
	libavcodec/h264_ps.c \
	libavcodec/h264_refs.c \
	libavcodec/h264_sei.c \
	libavcodec/h264_slice.c \
	libavcodec/h264chroma.c \
	libavcodec/h264dsp.c \
	libavcodec/h264idct.c \
	libavcodec/h264pred.c \
	libavcodec/h264qpel.c \
	libavcodec/hevc.c \
	libavcodec/hevc_cabac.c \
	libavcodec/hevc_filter.c \
	libavcodec/hevc_mvs.c \
	libavcodec/hevc_parser.c \
	libavcodec/hevc_ps.c \
	libavcodec/hevc_refs.c \
	libavcodec/hevc_sei.c \
	libavcodec/hevcdsp.c \
	libavcodec/hevcpred.c \
	libavcodec/hpeldsp.c \
	libavcodec/huffman.c \
	libavcodec/huffyuv.c \
	libavcodec/huffyuvdec.c \
	libavcodec/huffyuvdsp.c \
	libavcodec/idctdsp.c \
	libavcodec/imc.c \
	libavcodec/imdct15.c \
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
	libavcodec/jpegls.c \
	libavcodec/jpeglsdec.c \
	libavcodec/jpegtables.c \
	libavcodec/jrevdct.c \
	libavcodec/kbdwin.c \
	libavcodec/lagarith.c \
	libavcodec/lagarithrac.c \
	libavcodec/latm_parser.c \
	libavcodec/libdcadec.c \
	libavcodec/libopenjpegdec.c \
	libavcodec/libspeexdec.c \
	libavcodec/lossless_audiodsp.c \
	libavcodec/lossless_videodsp.c \
	libavcodec/lsp.c \
	libavcodec/mathtables.c \
	libavcodec/mdct_template.c \
	libavcodec/me_cmp.c \
	libavcodec/mdct_fixed.c \
	libavcodec/mdct_fixed_32.c \
	libavcodec/mdct_float.c \
	libavcodec/metasound.c \
	libavcodec/metasound_data.c \
	libavcodec/mjpeg_parser.c \
	libavcodec/mjpegbdec.c \
	libavcodec/mjpegdec.c \
	libavcodec/mlp.c \
	libavcodec/mlp_parser.c \
	libavcodec/mlpdec.c \
	libavcodec/mlpdsp.c \
	libavcodec/mpc.c \
	libavcodec/mpc7.c \
	libavcodec/mpc8.c \
	libavcodec/mpeg_er.c \
	libavcodec/mpeg12.c \
	libavcodec/mpeg12data.c \
	libavcodec/mpeg12dec.c \
	libavcodec/mpeg4audio.c \
	libavcodec/mpeg4video.c \
	libavcodec/mpeg4video_parser.c \
	libavcodec/mpeg4videodec.c \
	libavcodec/mpegaudio.c \
	libavcodec/mpegaudio_parser.c \
	libavcodec/mpegaudiodata.c \
	libavcodec/mpegaudiodec_fixed.c \
	libavcodec/mpegaudiodsp_data.c \
	libavcodec/mpegaudiodec_float.c \
	libavcodec/mpegaudiodecheader.c \
	libavcodec/mpegaudiodsp.c \
	libavcodec/mpegaudiodsp_float.c \
	libavcodec/mpegaudiodsp_template.c \
	libavcodec/mpegutils.c \
	libavcodec/mpegvideo.c \
	libavcodec/mpegvideo_motion.c \
	libavcodec/mpegvideo_parser.c \
	libavcodec/mpegvideodsp.c \
	libavcodec/mpegvideoencdsp.c \
	libavcodec/msmpeg4.c \
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
	libavcodec/options.c \
	libavcodec/opus.c \
	libavcodec/opus_celt.c \
	libavcodec/opus_parser.c \
	libavcodec/opus_silk.c \
	libavcodec/opusdec.c \
	libavcodec/parser.c \
	libavcodec/pixblockdsp.c \
	libavcodec/png.c \
	libavcodec/pngdec.c \
	libavcodec/pngdsp.c \
	libavcodec/pthread.c \
	libavcodec/pthread_frame.c \
	libavcodec/pthread_slice.c \
	libavcodec/qdm2.c \
	libavcodec/qpeldsp.c \
	libavcodec/qtrle.c \
	libavcodec/proresdata.c \
	libavcodec/proresdec2.c \
	libavcodec/proresdsp.c \
	libavcodec/r210dec.c \
	libavcodec/ra144.c \
	libavcodec/ra144dec.c \
	libavcodec/ra288.c \
	libavcodec/ralf.c \
	libavcodec/rangecoder.c \
	libavcodec/raw.c \
	libavcodec/rawdec.c \
	libavcodec/rdft.c \
	libavcodec/rpza.c \
	libavcodec/rv10.c \
	libavcodec/rv30.c \
	libavcodec/rv30dsp.c \
	libavcodec/rv34.c \
	libavcodec/rv34dsp.c \
	libavcodec/rv40.c \
	libavcodec/rv40dsp.c \
	libavcodec/sbrdsp.c \
	libavcodec/shorten.c \
	libavcodec/simple_idct.c \
	libavcodec/sinewin.c \
	libavcodec/sipr.c \
	libavcodec/sipr16k.c \
	libavcodec/snow_dwt.c \
	libavcodec/snow.c \
	libavcodec/sp5xdec.c \
	libavcodec/startcode.c \
	libavcodec/svq1.c \
	libavcodec/svq13.c \
	libavcodec/svq1dec.c \
	libavcodec/svq3.c \
	libavcodec/synth_filter.c \
	libavcodec/tak.c \
	libavcodec/tak_parser.c \
	libavcodec/takdec.c \
	libavcodec/tiff.c \
	libavcodec/tiff_common.c \
	libavcodec/tiff_data.c \
	libavcodec/tpeldsp.c \
	libavcodec/truespeech.c \
	libavcodec/tscc.c \
	libavcodec/tscc2.c \
	libavcodec/tta.c \
	libavcodec/ttadata.c \
	libavcodec/ttadsp.c \
	libavcodec/twinvq.c \
	libavcodec/utvideo.c \
	libavcodec/utvideodec.c \
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
	libavcodec/videodsp.c \
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
	libavcodec/vp56rac.c \
	libavcodec/vp6.c \
	libavcodec/vp6dsp.c \
	libavcodec/vp8.c \
	libavcodec/vp8_parser.c \
	libavcodec/vp8dsp.c \
	libavcodec/vp9.c \
	libavcodec/vp9_parser.c \
	libavcodec/vp9dsp.c \
	libavcodec/wavpack.c \
	libavcodec/wma.c \
	libavcodec/wma_common.c \
	libavcodec/wma_freqs.c \
	libavcodec/wmadec.c \
	libavcodec/wmalosslessdec.c \
	libavcodec/wmaprodec.c \
	libavcodec/wmavoice.c \
	libavcodec/wmv2.c \
	libavcodec/wmv2dec.c \
	libavcodec/wmv2dsp.c \
	libavcodec/xiph.c \
	libavcodec/xvididct.c \
	\
	libavcodec/x86/ac3dsp_init.c \
	libavcodec/x86/audiodsp_init.c \
	libavcodec/x86/blockdsp_init.c \
	libavcodec/x86/bswapdsp_init.c \
	libavcodec/x86/constants.c \
	libavcodec/x86/dcadsp_init.c \
	libavcodec/x86/dct_init.c \
	libavcodec/x86/dirac_dwt.c \
	libavcodec/x86/diracdsp_mmx.c \
	libavcodec/x86/fdct.c \
	libavcodec/x86/fdctdsp_init.c \
	libavcodec/x86/fft_init.c \
	libavcodec/x86/flacdsp_init.c \
	libavcodec/x86/fmtconvert_init.c \
	libavcodec/x86/h263dsp_init.c \
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
	libavcodec/x86/lossless_audiodsp_init.c \
	libavcodec/x86/lossless_videodsp_init.c \
	libavcodec/x86/me_cmp_init.c \
	libavcodec/x86/mlpdsp_init.c \
	libavcodec/x86/mpegaudiodsp.c \
	libavcodec/x86/mpegvideo.c \
	libavcodec/x86/mpegvideodsp.c \
	libavcodec/x86/mpegvideoencdsp_init.c \
	libavcodec/x86/pixblockdsp_init.c \
	libavcodec/x86/pngdsp_init.c \
	libavcodec/x86/proresdsp_init.c \
	libavcodec/x86/qpeldsp_init.c \
	libavcodec/x86/rv34dsp_init.c \
	libavcodec/x86/rv40dsp_init.c \
	libavcodec/x86/sbrdsp_init.c \
	libavcodec/x86/simple_idct.c \
	libavcodec/x86/snowdsp.c \
	libavcodec/x86/ttadsp_init.c \
	libavcodec/x86/v210-init.c \
	libavcodec/x86/vc1dsp_init.c \
	libavcodec/x86/vc1dsp_mmx.c \
	libavcodec/x86/videodsp_init.c \
	libavcodec/x86/vorbisdsp_init.c \
	libavcodec/x86/vp3dsp_init.c \
	libavcodec/x86/vp6dsp_init.c \
	libavcodec/x86/vp8dsp_init.c \
	libavcodec/x86/vp9dsp_init.c \
	libavcodec/x86/xvididct_init.c

SRCS_LF = \
	libavfilter/af_atempo.c \
	libavfilter/af_biquads.c \
	libavfilter/allfilters.c \
	libavfilter/audio.c \
	libavfilter/avcodec.c \
	libavfilter/avfilter.c \
	libavfilter/avfiltergraph.c \
	libavfilter/buffer.c \
	libavfilter/buffersink.c \
	libavfilter/buffersrc.c \
	libavfilter/formats.c \
	libavfilter/graphparser.c \
	libavfilter/pthread.c \
	libavfilter/video.c

SRCS_LBR = \
	libavresample/audio_convert.c \
	libavresample/audio_data.c \
	libavresample/audio_mix.c \
	libavresample/audio_mix_matrix.c \
	libavresample/dither.c \
	libavresample/options.c \
	libavresample/resample.c \
	libavresample/utils.c \
	\
	libavresample/x86/audio_convert_init.c \
	libavresample/x86/audio_mix_init.c \
	libavresample/x86/dither_init.c

SRCS_LU = \
	libavutil/atomic.c \
	libavutil/audio_fifo.c \
	libavutil/avstring.c \
	libavutil/bprint.c \
	libavutil/buffer.c \
	libavutil/channel_layout.c \
	libavutil/cpu.c \
	libavutil/crc.c \
	libavutil/dict.c \
	libavutil/display.c \
	libavutil/downmix_info.c \
	libavutil/error.c \
	libavutil/eval.c \
	libavutil/fifo.c \
	libavutil/file_open.c \
	libavutil/fixed_dsp.c \
	libavutil/float_dsp.c \
	libavutil/frame.c \
	libavutil/imgutils.c \
	libavutil/intmath.c \
	libavutil/lfg.c \
	libavutil/lls.c \
	libavutil/log.c \
	libavutil/log2_tab.c \
	libavutil/lzo.c \
	libavutil/mathematics.c \
	libavutil/md5.c \
	libavutil/mem.c \
	libavutil/opt.c \
	libavutil/parseutils.c \
	libavutil/pixdesc.c \
	libavutil/random_seed.c \
	libavutil/rational.c \
	libavutil/samplefmt.c \
	libavutil/sha.c \
	libavutil/stereo3d.c \
	libavutil/threadmessage.c \
	libavutil/timecode.c \
	libavutil/utils.c \
	\
	libavutil/x86/cpu.c \
	libavutil/x86/float_dsp_init.c \
	libavutil/x86/lls_init.c

SRCS_LR = \
	libswresample/audioconvert.c \
	libswresample/dither.c\
	libswresample/options.c \
	libswresample/rematrix.c \
	libswresample/resample.c \
	libswresample/resample_dsp.c \
	libswresample/swresample.c \
	libswresample/swresample_frame.c \
	\
	libswresample/x86/audio_convert_init.c \
	libswresample/x86/rematrix_init.c \
	libswresample/x86/resample_init.c

SRCS_LS = \
	libswscale/hscale_fast_bilinear.c \
	libswscale/input.c \
	libswscale/options.c \
	libswscale/output.c \
	libswscale/rgb2rgb.c \
	libswscale/swscale.c \
	libswscale/swscale_unscaled.c \
	libswscale/utils.c \
	libswscale/yuv2rgb.c \
	\
	libswscale/x86/hscale_fast_bilinear_simd.c \
	libswscale/x86/rgb2rgb.c \
	libswscale/x86/swscale.c \
	libswscale/x86/yuv2rgb.c

# Yasm objects
SRCS_YASM_LC = \
	libavcodec/x86/ac3dsp.asm \
	libavcodec/x86/audiodsp.asm \
	libavcodec/x86/blockdsp.asm \
	libavcodec/x86/bswapdsp.asm \
	libavcodec/x86/dcadsp.asm \
	libavcodec/x86/dct32.asm \
	libavcodec/x86/deinterlace.asm \
	libavcodec/x86/diracdsp_yasm.asm \
	libavcodec/x86/dwt_yasm.asm \
	libavcodec/x86/fft.asm \
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
	libavcodec/x86/hevc_deblock.asm \
	libavcodec/x86/hevc_idct.asm \
	libavcodec/x86/hevc_mc.asm \
	libavcodec/x86/hevc_res_add.asm \
	libavcodec/x86/hevc_sao.asm \
	libavcodec/x86/hpeldsp.asm \
	libavcodec/x86/huffyuvdsp.asm \
	libavcodec/x86/idctdsp.asm \
	libavcodec/x86/imdct36.asm \
	libavcodec/x86/lossless_audiodsp.asm \
	libavcodec/x86/lossless_videodsp.asm \
	libavcodec/x86/me_cmp.asm \
	libavcodec/x86/mlpdsp.asm \
	libavcodec/x86/mpegvideoencdsp.asm \
	libavcodec/x86/pixblockdsp.asm \
	libavcodec/x86/pngdsp.asm \
	libavcodec/x86/proresdsp.asm \
	libavcodec/x86/qpel.asm \
	libavcodec/x86/qpeldsp.asm \
	libavcodec/x86/rv34dsp.asm \
	libavcodec/x86/rv40dsp.asm \
	libavcodec/x86/sbrdsp.asm \
	libavcodec/x86/ttadsp.asm \
	libavcodec/x86/v210.asm \
	libavcodec/x86/vc1dsp.asm \
	libavcodec/x86/videodsp.asm \
	libavcodec/x86/vorbisdsp.asm \
	libavcodec/x86/vp3dsp.asm \
	libavcodec/x86/vp6dsp.asm \
	libavcodec/x86/vp8dsp.asm \
	libavcodec/x86/vp8dsp_loopfilter.asm \
	libavcodec/x86/vp9intrapred.asm \
	libavcodec/x86/vp9itxfm.asm \
	libavcodec/x86/vp9lpf.asm \
	libavcodec/x86/vp9mc.asm \
	libavcodec/x86/xvididct.asm

SRCS_YASM_LBR = \
	libavresample/x86/audio_convert.asm \
	libavresample/x86/audio_mix.asm \
	libavresample/x86/dither.asm \
	libavresample/x86/util.asm

SRCS_YASM_LU = \
	libavutil/x86/cpuid.asm \
	libavutil/x86/emms.asm \
	libavutil/x86/float_dsp.asm \
	libavutil/x86/lls.asm

SRCS_YASM_LR = \
	libswresample/x86/audio_convert.asm \
	libswresample/x86/rematrix.asm \
	libswresample/x86/resample.asm

SRCS_YASM_LS = \
	libswscale/x86/input.asm \
	libswscale/x86/output.asm \
	libswscale/x86/scale.asm

OBJS_LC = \
	$(SRCS_LC:%.c=$(OBJ_DIR)%.o) \
	$(SRCS_YASM_LC:%.asm=$(OBJ_DIR)%.o)

OBJS_LF = \
	$(SRCS_LF:%.c=$(OBJ_DIR)%.o)

OBJS_LBR = \
	$(SRCS_LBR:%.c=$(OBJ_DIR)%.o) \
	$(SRCS_YASM_LBR:%.asm=$(OBJ_DIR)%.o)

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
	@$(FFMPEG_PREFIX)gcc -c $(CFLAGS) $(OPTFLAGS) -MMD -Wno-deprecated-declarations -Wno-pointer-to-int-cast -o $@ $<

$(OBJ_DIR)%.o: %.asm
	@echo $<
	@yasm $(YASMFLAGS) -I$(<D)/ -o $@ $<

$(LIB_LIBAVCODEC): $(OBJS_LC)
	@echo $@
	@$(FFMPEG_PREFIX)ar rc $@ $(OBJS_LC)

$(LIB_LIBAVFILTER): $(OBJS_LF)
	@echo $@
	@$(FFMPEG_PREFIX)ar rc $@ $(OBJS_LF)

$(LIB_LIBAVRESAMPLE): $(OBJS_LBR)
	@echo $@
	@$(FFMPEG_PREFIX)ar rc $@ $(OBJS_LBR)

$(LIB_LIBAVUTIL): $(OBJS_LU)
	@echo $@
	@$(FFMPEG_PREFIX)ar rc $@ $(OBJS_LU)

$(LIB_LIBSWSCALE): $(OBJS_LS)
	@echo $@
	@$(FFMPEG_PREFIX)ar rc $@ $(OBJS_LS)

$(LIB_LIBSWRESAMPLE): $(OBJS_LR)
	@echo $@
	@$(FFMPEG_PREFIX)ar rc $@ $(OBJS_LR)

-include $(SRCS_LC:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LF:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LBR:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LU:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LR:%.c=$(OBJ_DIR)%.d)
-include $(SRCS_LS:%.c=$(OBJ_DIR)%.d)

.PHONY: clean make_objdirs $(OBJ_DIRS)
