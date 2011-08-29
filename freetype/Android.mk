###########################################################################################
# Freetype build script for android
# 
# Copyright (C) 2011 Waze Ltd
# Author: Alex Agranovich (AGA)
#

# --- Tool specific options ------------------------------------------------
FT_INCLUDES := $(LOCAL_PATH)/src/base src/pcf $(LOCAL_PATH)/src/psaux $(LOCAL_PATH)/src/pshinter $(LOCAL_PATH)/src/psnames \
		 $(LOCAL_PATH)/src/sfnt $(LOCAL_PATH)/src/truetype $(LOCAL_PATH)/src/type1  \
		 $(LOCAL_PATH)/src/winfonts $(LOCAL_PATH)/include/freetype $(LOCAL_PATH)/include $(LOCAL_PATH)/include/freetype/internal

	     
FTSRCS= src/autofit/autofit.c			\
	src/base/ftbase.c			\
	src/base/ftbbox.c			\
	src/base/ftbdf.c			\
	src/base/ftbitmap.c			\
	src/base/ftglyph.c			\
	src/base/ftinit.c			\
	src/base/ftmm.c				\
	src/base/ftpfr.c			\
	src/base/ftstroke.c			\
	src/base/ftsynth.c			\
	src/base/ftsystem.c			\
	src/base/fttype1.c			\
	src/base/ftwinfnt.c			\
	src/bdf/bdf.c				\
	src/cache/ftcache.c			\
	src/cff/cff.c				\
	src/cid/type1cid.c			\
	src/gzip/ftgzip.c			\
	src/lzw/ftlzw.c				\
	src/pcf/pcf.c				\
	src/pfr/pfr.c				\
	src/psaux/psaux.c			\
	src/pshinter/pshinter.c			\
	src/psnames/psmodule.c			\
	src/raster/raster.c			\
	src/sfnt/sfnt.c				\
	src/smooth/smooth.c			\
	src/truetype/truetype.c			\
	src/type1/type1.c			\
	src/type42/type42.c			\
	src/winfonts/winfnt.c	


include $(MY_LOCAL_PATH)/Android.Common.mk
LOCAL_MODULE    := libft
LOCAL_SRC_FILES := $(FTSRCS)
LOCAL_C_INCLUDES := $(LOCAL_C_INCLUDES) $(FT_INCLUDES)
LOCAL_CFLAGS := $(LOCAL_CFLAGS) -DFT2_BUILD_LIBRARY
include $(BUILD_STATIC_LIBRARY)




	
	
	
