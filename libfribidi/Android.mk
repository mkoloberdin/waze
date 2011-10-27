###########################################################################################
# Fribidi static library
# 
# Copyright (C) 2011 Waze Ltd
# Author: Alex Agranovich (AGA)
#

BIDI_SRCS=fribidi.c \
	  fribidi_char_type.c \
	  fribidi_mem.c \
	  fribidi_mirroring.c \
	  fribidi_types.c

include $(MY_LOCAL_PATH)/Android.Common.mk
LOCAL_MODULE    := libfribidi
LOCAL_SRC_FILES := $(BIDI_SRCS)
LOCAL_CFLAGS := $(LOCAL_CFLAGS) -DFRIBIDI_EXPORTS -DHAS_FRIBIDI_TAB_CHAR_TYPE_9_I \
				-DHAS_FRIBIDI_TAB_CHAR_TYPE_2_I -DMEM_OPTIMIZED -DFRIBIDI_NO_CHARSETS

include $(BUILD_STATIC_LIBRARY)
