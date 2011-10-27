###########################################################################################
# Application common make parameters
# 
# Copyright (C) 2011 Waze Ltd
# Author: Alex Agranovich (AGA)
#

#=============== Internal variables ===================#

RENDERING := OPENGL
TTS:=YES
ANDROID_WIDGET := NO


MY_LOCAL_PATH := $(call my-dir)
MY_ROOT := $(MY_LOCAL_PATH)/../../../

MY_DEFS := -DSSD -DTOUCH_SCREEN -DUSE_FRIBIDI -DFREEMAP_IL \
           -DFORCE_INLINE -DINLINE_DEC='static inline'


MY_INCLUDES := -I$(MY_ROOT) \
	       -I$(MY_ROOT)/freetype/include \
	       -I$(MY_ROOT)/freetype/include/freetype \
	       -I$(MY_ROOT)/android \
               -I$(MY_ROOT)/android/external \
	       -I$(MY_ROOT)/ssd	 \
               -I$(MY_ROOT)/libpng

ifeq ($(RENDERING),OPENGL)
   OPENGL_DIR = ogl
   MY_DEFS := $(MY_DEFS) -DOPENGL -DVIEW_MODE_3D_OGL  
   #RDMLIBS += libglu.a
endif


#=============== APP variables assignment ===================#

ifeq ($(NDK_DEBUG),0)
APP_ABI := armeabi armeabi-v7a
MY_OPTS :=  -fomit-frame-pointer \
		    -fstrict-aliasing \
		    -ffast-math		\
		    -funswitch-loops  \
        	-fexpensive-optimizations \
        	-falign-functions=32 \
        	-falign-loops \
        	-falign-labels \
        	-falign-jumps

else
CSV_GPS :=YES
APP_ABI := armeabi
MY_OPTS :=  
endif

ifeq ($(CSV_GPS),YES)
   MY_DEFS := $(MY_DEFS) -DCSV_GPS
   csv_gps_c=roadmap_gps_csv.c
endif

ifeq ($(ANDROID_WIDGET),YES)
   MY_DEFS := $(MY_DEFS) -DANDROID_WIDGET
endif

APP_PLATFORM := android-9

APP_CFLAGS := $(MY_DEFS) $(MY_INCLUDES) $(MY_OPTS)



