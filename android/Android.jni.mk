###########################################################################################
# JNI Headers build makefiles
# Automatically builds the JNI headers from the .class files
#
# Copyright (C) 2011 Waze Ltd
# Author: Alex Agranovich (AGA)
#

# Target path
JNI_TRGT_DIR=$(LOCAL_PATH)/JNI/

# Class path
JAVACLASSPATH = $(LOCAL_PATH)/java/bin/

# Package prefix
PKG_PREFIX = com.waze

#Path
OBJS_PATH = $(JAVACLASSPATH)/com/waze/

# Java objects
JAVAOBJS=FreeMapNativeManager \
		FreeMapNativeCanvas \
		FreeMapNativeTimerManager \
		FreeMapNativeTimerTask \
		FreeMapNativeLocListener \
		FreeMapNativeSoundManager \
		WazeLog \
		WazeMenuManager \
		WazeEditBox \
		WazeMsgBox \
		WazeSpeechttManagerBase \
		WazeSpeechttManager \
		WazeResManager \
		WazeSoundRecorder \
		WazeTtsManager
		
JAVAOBJS_WIDGET= widget.WazeAppWidgetManager

#Suffix of the JNI output files
JNI_SUFFIX=_JNI.h

JNI_HEADERS=$(JAVAOBJS:%=$(JNI_TRGT_DIR)/%_JNI.h)

JNI_WIDGET_HEADERS=$(JAVAOBJS_WIDGET:widget.%=$(JNI_TRGT_DIR)/%_JNI.h)

#jni_headers: $(JNI_HEADERS) $(JNI_WIDGET_HEADERS)
jni_headers: $(JNI_HEADERS)

$(JNI_TRGT_DIR)/%_JNI.h: $(JAVACLASSPATH)/com/waze/%.class
		@echo Making JNI header: $*_JNI.h ; 
		@javah -jni -force -classpath $(JAVACLASSPATH) -o $@ $(PKG_PREFIX).$*

$(JNI_TRGT_DIR)/%_JNI.h: $(JAVACLASSPATH)/com/waze/widget/%.class
		@echo Making JNI header: $*_JNI.h ; 
		@javah -jni -force -classpath $(JAVACLASSPATH) -o $@ $(PKG_PREFIX).widget.$*


# Clean actually unnecessary. These header files are actually has no impact
clean: ;
	@rm -f $(JNI_TRGT_DIR)/*_JNI.h
