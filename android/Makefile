include Makefile.in.android

# --- Installation options -------------------------------------------------



DESTDIR=
INSTALLDIR=/data
desktopdir=$(INSTALLDIR)/applications
pkgdatadir=$(INSTALLDIR)/share/roadmap
pkgmapsdir=/var/lib/roadmap
bindir=$(INSTALLDIR)/bin
pkgbindir=$(bindir)
menudir=$(DESTDIR)/usr/lib/menu
ICONDIR=$(INSTALLDIR)/share/pixmaps
mandir=$(INSTALLDIR)/share/man
man1dir=$(mandir)/man1



# --- Tool specific options ------------------------------------------------
OPENGL_DIR=../ogl

STDCFLAGS= $(CFLAGS)
LIB_INCLUDES+= -I. -I.. -I../agg/include -I../freetype/include/freetype -I../freetype/include -I../libfribidi -I../libpng -I../zlib
LIB_INCLUDES+= -I/usr/lib/jvm/java-6-sun/include -I/usr/lib/jvm/java-6-sun/include/linux
LIB_DIRS+= -L.. -L../unix -L../freetype -L../agg -L../libpng -L../libfribidi -L../libpng -L../zlib -L$(OPENGL_DIR)/glu



ifeq ($(MODE),DEBUG)
   MODECFLAGS=-g -DROADMAP_DEBUG_HEAP
else
ifeq ($(MODE),PROFILE)
   MODECFLAGS=-g-pg -fprofile-arcs   
else
   
endif
endif

ifeq ($(RENDERING),OPENGL)
  MODECFLAGS+= "-DOPENGL -DANDROID_OGL" 
endif


ifeq ($(SSD),YES)
CFLAGS += -DSSD
SSDLIB=../libssd_widgets.a
endif

RDMLIBS=../libroadmap.a libandroidroadmap.a ../unix/libosroadmap.a 


ARFLAGS="rf"

CFLAGS = $(CFLAGS_ANDROID) $(MODECFLAGS) $(LIB_INCLUDES)
LIBS += $(RDMLIBS) -lft -lpng

ifneq ($(RENDERING),OPENGL)
LIBS += -lagg
else
LIBS += -lglu
endif

ifeq ($(BIDI),YES)
LIBS += -lfribidi
CFLAGS += -DUSE_FRIBIDI
endif


ifeq ($(FREEMAP_IL),YES)
CFLAGS += -DFREEMAP_IL
RUNTIME=libandroidroadmap.so  
endif


RUNTIME=libandroidroadmap.so

CXXFLAGS = $(CFLAGS) 
# --- RoadMap sources & targets -----------------------------------

RMLIBSRCS=roadmap_main.c \
	      roadmap_libpng.c \
		  roadmap_path.c \
		  roadmap_dialog.c \
          roadmap_fileselection.c \
          roadmap_wchar.c \
          roadmap_androidgps.c \
          roadmap_androidbrowser.c \
          roadmap_sound.c \
          roadmap_spawn.c \
          roadmap_device.c \
          roadmap_camera.c \
          roadmap_thread.c \
          roadmap_editbox.c \
          roadmap_native_keyboard.c \
          roadmap_androidmenu.c \
          roadmap_androidmsgbox.c \
          roadmap_androidrecommend.c \
          roadmap_androidspeechtt.c \
          JNI/FreeMapNativeTimerTask_JNI.c \
          JNI/FreeMapNativeTimerManager_JNI.c \
          JNI/FreeMapNativeManager_JNI.c \
          JNI/FreeMapNativeCanvas_JNI.c \
          JNI/FreeMapNativeLocListener_JNI.c \
          JNI/FreeMapNativeSoundManager_JNI.c \
          JNI/WazeLog_JNI.c \
          JNI/WazeMenuManager_JNI.c \
          JNI/WazeMsgBox_JNI.c \
          JNI/WazeSoundRecorder_JNI.c \
          JNI/WazeSpeechttManager_JNI.c \
          JNI/FreeMapJNI.c	                             


ifeq ($(RENDERING),OPENGL)
RMLIBSRCS+= roadmap_canvas_ogl.c 
#RMLIBSRCS+= $(OPENGL_DIR)/roadmap_canvas.c $(OPENGL_DIR)/roadmap_canvas_font.c $(OPENGL_DIR)/roadmap_canvas_atlas.c
else
RMLIBSRCS+= roadmap_canvas_agg.c 
endif


ifneq ($(SSD),YES)
RMLIBSRCS += roadmap_messagebox.c
endif

RMLIBOBJS=$(RMLIBSRCS:.c=.o)

#RMLIBOBJS=$(RMLIBSRCS:.c=.o) roadmap_canvas_agg.o \
#								roadmap_libpng.o roadmap_main.o
							  

# --- Conventional targets ----------------------------------------

.PHONY: all others build runtime clean cleanone install uninstall ../libssd_widgets.a

all: runtime libstrip libpackage
	
others:
	$(MAKE) -C ../freetype -f Makefile.android
#	$(MAKE) -C ../agg -f Makefile.android
	$(MAKE) -C ../libpng -f Makefile.android
#	$(MAKE) -C ../zlib   -f Makefile libz.a
	$(MAKE) -C ../libfribidi   -f Makefile.android
	$(MAKE) -C .. runtime
	$(MAKE) -C ../unix runtime

build:
	$(MAKE) -C .. CFLAGS="$(STDCFLAGS)" build

runtime: others javaobjs $(RUNTIME)

libstrip: 
		$(STRIP) libandroidroadmap.so -s -o libandroidroadmap.so.nosymbols
		
libpackage:
		cp libandroidroadmap.so.nosymbols java/assets/libandroidroadmap.so
		
clean: cleanone

cleanone:
	rm -f *.o *.a *.da $(RUNTIME)
	rm -f libandroidroadmap.so.nosymbols
	rm -f JNI/*.o

install: all
	mkdir -p $(DESTDIR)$(bindir)
	cd $(DESTDIR)$(bindir) && rm -f $(RUNTIME)
	cp $(RUNTIME) $(DESTDIR)$(bindir)

uninstall:
	cd $(DESTDIR)$(bindir) && rm -f $(RUNTIME) roadmap roadgps
	$(MAKE) -C .. uninstall
cleanall:
	$(MAKE) -C .. cleanone
	$(MAKE) -C ../unix cleanone
	$(MAKE) -C ../freetype -f Makefile.android clean
	$(MAKE) -C ../agg -f Makefile.android clean
	$(MAKE) -C ../libpng -f Makefile.android clean
	$(MAKE) -C ../zlib   -f Makefile clean
	$(MAKE) -C ../libfribidi   -f Makefile.android clean

# --- The real targets --------------------------------------------
javaobjs:
	$(MAKE) -C java -f Makefile.jni


libandroidroadmap.a: $(RMLIBOBJS) javaobjs
	$(AR) $(ARFLAGS) libandroidroadmap.a $(RMLIBOBJS)
	$(RANLIB) libandroidroadmap.a
	
	
libandroidroadmap.so: $(SSDLIB) ../libguiroadmap.a $(RDMLIBS)
	$(LD) $(LDSHAREDFLAGS) \
	-o libandroidroadmap.so --whole-archive $(LIB_DIRS) \
	$(SSDLIB) ../libguiroadmap.a $(LIBS)
 	
../libssd_widgets.a:
	$(MAKE) -C .. libssd_widgets.a
