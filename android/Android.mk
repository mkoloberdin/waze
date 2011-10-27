###########################################################################################
# Android platform specific library.
# Static and shared target objects
#
# Copyright (C) 2011 Waze Ltd
# Author: Alex Agranovich (AGA)
#

LIBSRCS=roadmap_main.c \
	roadmap_libpng.c \
        roadmap_path.c \
	roadmap_dialog.c \
	roadmap_fileselection.c \
	roadmap_androidgps.c \
	roadmap_androidbrowser.c \
	roadmap_sound.c \
	roadmap_spawn.c \
	roadmap_device.c \
	roadmap_camera.c \
	roadmap_thread.c \
	roadmap_wchar.c \
	roadmap_editbox.c \
	roadmap_native_keyboard.c \
	roadmap_androidmenu.c \
	roadmap_androidmsgbox.c \
	roadmap_androidrecommend.c \
	roadmap_androidspeechtt.c \
	tts_android_provider.c \
	JNI/FreeMapNativeTimerTask_JNI.c \
	JNI/FreeMapNativeTimerManager_JNI.c \
	JNI/FreeMapNativeManager_JNI.c \
	JNI/FreeMapNativeCanvas_JNI.c \
	JNI/FreeMapNativeLocListener_JNI.c \
	JNI/FreeMapNativeSoundManager_JNI.c \
	JNI/WazeLog_JNI.c \
	JNI/WazeMenuManager_JNI.c \
	JNI/WazeMsgBox_JNI.c \
	JNI/WazeTtsManager_JNI.c \
	JNI/WazeSoundRecorder_JNI.c \
	JNI/WazeSpeechttManager_JNI.c \
	JNI/WazeResManager_JNI.c \
	JNI/FreeMapJNI.c

ifeq ($(RENDERING),OPENGL)
  LIBSRCS+= roadmap_canvas_ogl.c 
else
  LIBSRCS+= roadmap_canvas_agg.c 
endif

include $(MY_LOCAL_PATH)/Android.Common.mk
LOCAL_SRC_FILES := $(LIBSRCS)
LOCAL_C_INCLUDES := $(LOCAL_C_INCLUDES) $(LOCAL_PATH)/JNI
LOCAL_MODULE    := waze
LOCAL_STATIC_LIBRARIES := libroadmap libguiroadmap libssd librt libunix libfribidi libft libpng libglu libguiroadmap libroadmap
LOCAL_LDLIBS    := -lGLESv1_CM -lEGL -lz -lsqlite -L$(LOCAL_PATH)/external -lssl -lcrypto
include $(BUILD_SHARED_LIBRARY)



