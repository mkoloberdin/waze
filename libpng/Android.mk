###########################################################################################
# PNG library makefile Android
# 
# Copyright (C) 2011 Waze Ltd
# Author: Alex Agranovich (AGA)
#
############################################################################
# ANDROID Related makefile for AGG library
#
############################################################################

PNG_SRCS= png.c \
  	pngerror.c \
	pnggccrd.c \
	pngget.c \
	pngmem.c \
	pngpread.c \
	pngread.c \
	pngrio.c \
	pngrtran.c \
	pngrutil.c \
	pngset.c \
	pngtrans.c \
	pngvcrd.c \
	pngwio.c \
	pngwrite.c \
	pngwtran.c \
	pngwutil.c 
	
include $(MY_LOCAL_PATH)/Android.Common.mk	 
LOCAL_MODULE    := libpng
LOCAL_SRC_FILES := $(PNG_SRCS)
include $(BUILD_STATIC_LIBRARY)
