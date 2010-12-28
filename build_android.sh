#!/bin/sh
# Note: "adb" and "arm-eabi-strip" has to be defined in the path environment 
# Params: $1 - the general make parameter (all/clean/... )
#         $2 - if upload the shared object to the emulator: optional (for debug purposes) 


# ==========================================================================
# Android source codebase directory. If not supplied build uses the default
# one. ( Refer to android/Makefile.in.android )
#
export DROID_SRC=


# ==========================================================================
# Android libraries directory for linkage. If not supplied build uses the default
# one. ( Refer to android/Makefile.in.android )
#
export DROID_LIB=


# ==========================================================================
# Android toolchain directory, providing the cross tools for 
# building/debugging/archiving. If not supplied build uses the default one. 
# ( Refer to android/Makefile.in.android )
# Android toolchain version: must match 
#
export DROID_TOOLCHAIN_VER=
export DROID_TOOLCHAIN=

# ==========================================================================
# Make the library
make DESKTOP=ANDROID MODE=DEBUG SSD=YES FREEMAP_IL=YES BIDI=YES SHAPEFILES=NO J2ME=NO RENDERING=OPENGL TILESTORAGE=SQLITE -S $1



if [ "$2" = "UPLOAD" ]; then
	echo "Pushing the library libandroidroadmap.so to the emulator"
	#adb -e push ./android/java/assets/libandroidroadmap.so /data/data/com.waze
	#adb -d push ./android/java/assets/libandroidroadmap.so /data/data/com.waze
	#adb -d push ./android/java/assets/libandroidroadmap.so /data/data/com.waze
	#adb -d push ./android/libandroidroadmap.so.nosymbols /sdcard/libandroidroadmap.so
	adb -d push ./android/libandroidroadmap.so.nosymbols /data/data/com.waze/libandroidroadmap.so
fi

echo "The Waze Android library build process is finished !!"
