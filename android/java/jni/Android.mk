###########################################################################################
# Entry point NDK make file
# Copyright (C) 2011 Waze Ltd
# Author: Alex Agranovich (AGA)
#

#===================================
# Freetype build
#
include $(CLEAR_VARS)
LOCAL_PATH := $(MY_ROOT)/freetype
include $(LOCAL_PATH)/Android.mk

#===================================
# Fribidi build
#
include $(CLEAR_VARS)
LOCAL_PATH := $(MY_ROOT)/libfribidi
include $(LOCAL_PATH)/Android.mk

#===================================
# PNG build
#
include $(CLEAR_VARS)
LOCAL_PATH := $(MY_ROOT)/libpng
include $(LOCAL_PATH)/Android.mk

#===================================
# Main static libraries build
#
include $(CLEAR_VARS)
LOCAL_PATH := $(MY_ROOT)
include $(LOCAL_PATH)/Android.mk

include $(CLEAR_VARS)
LOCAL_PATH := $(MY_ROOT)/android
include $(LOCAL_PATH)/Android.jni.mk

#===================================
# Android platform specific code and
# Target shared object
include $(CLEAR_VARS)
LOCAL_PATH := $(MY_ROOT)/android
include $(LOCAL_PATH)/Android.mk

