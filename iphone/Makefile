# IPHONE makefile

TOP = ..

ifeq ($(IPHONE_TOOLS_DIR),)
IPHONE_TOOLS_DIR=/usr
endif

CC=$(IPHONE_TOOLS_DIR)/bin/arm-apple-darwin9-gcc
AR=$(IPHONE_TOOLS_DIR)/bin/arm-apple-darwin9-ar
RANLIB = $(IPHONE_TOOLS_DIR)/bin/arm-apple-darwin9-ranlib

CFLAGS=-O2 -Wall -I../ -I/var/include -I$(IPHONE_TOOLS_DIR)/include
LD=$(CC)

WARNFLAGS = -W -Wall -Wno-unused-parameter

ARFLAGS = "r"

LIBS = ../libroadmap.a ../unix/libosroadmap.a ../libssd_widgets.a -lm


# --- IPHONE-specific options ---------------------------------------

ROADMAP_BUNDLE=/Applications/waze.app
ROADGPS_BUNDLE=RoadGps.app

LDFLAGS=-multiply_defined suppress -lobjc -L"/usr/lib" -F"/System/Library/Frameworks" -F"/System/Library/PrivateFrameworks" -bind_at_load -framework CoreFoundation -framework Foundation -framework UIKit -framework CoreGraphics -framework GraphicsServices -framework AudioToolbox -framework CoreLocation -framework CFNetwork ../libguiroadmap.a ../libroadmap.a ../unix/libosroadmap.a ../zlib/libz.a
CFLAGS +=-DIPHONE -DBOOL_DEFINED

# --- IPHONE sources & targets -----------------------------------


RMLIBSRCS=roadmap_dialog.m \
          roadmap_fileselection.m \
		  roadmap_canvas.m \
        roadmap_sound.m \
        roadmap_device.m \
        roadmap_location.m \
        roadmap_native_keyboard.m

RMLIBOBJS = $(RMLIBSRCS:.m=.o)

HEADERS = \
	roadmap_iphonecanvas.h \
	roadmap_iphonemain.h \
	colors.h
	

TARGETS = iphoneroadmap

# --- Conventional targets ----------------------------------------

.PHONY: all strip clean install uninstall

all: $(TARGETS) install

strip:
	$(STRIP) $(TARGETS)

clean:
	rm -f *.o *.a *.da $(TARGETS) 
	rm -rf $(ROADMAP_BUNDLE)/waze_ $(ROADGPS_BUNDLE)

cleanone: clean

install:
#	mkdir -p $(ROADMAP_BUNDLE)/resources/default
	mkdir -p $(ROADMAP_BUNDLE)
	/usr/bin/ldid -S iphoneroadmap
	cp iphoneroadmap $(ROADMAP_BUNDLE)/waze_
	chmod 755 $(ROADMAP_BUNDLE)/waze_
#	cp ../session $(ROADMAP_BUNDLE)/resources
#	cp ../preferences $(ROADMAP_BUNDLE)/resources
#	cp ../sprites $(ROADMAP_BUNDLE)/resources
#	cp ../usstates.txt $(ROADMAP_BUNDLE)/resources
#	cp ../roadmap.screenobjects $(ROADMAP_BUNDLE)/resources
#	cp ../default/All $(ROADMAP_BUNDLE)/resources/default
#	cp Info.plist.roadmap $(ROADMAP_BUNDLE)/Info.plist
#	mkdir -p $(ROADGPS_BUNDLE)/resources/default
#	cp iphoneroadgps $(ROADGPS_BUNDLE)/RoadGps
#	cp ../session $(ROADGPS_BUNDLE)/resources
#	cp ../preferences $(ROADGPS_BUNDLE)/resources
#	cp ../sprites $(ROADGPS_BUNDLE)/resources
#	cp ../usstates.txt $(ROADGPS_BUNDLE)/resources
#	cp ../roadmap.screenobjects $(ROADGPS_BUNDLE)/resources
#	cp ../default/All $(ROADGPS_BUNDLE)/resources/default
#	cp Info.plist.roadgps $(ROADGPS_BUNDLE)/Info.plist

uninstall:
	cd $(bindir) && rm -f $(TARGETS) roadmap roadgps

sourcelist:
	@echo Makefile roadmap_main.m \
	    roadmap_canvas.m \
	    $(RMLIBSRCS) $(HEADERS)

# --- The real targets --------------------------------------------

libiphoneroadmap.a: $(RMLIBOBJS)
	$(AR) $(ARFLAGS) libiphoneroadmap.a $(RMLIBOBJS)
	$(RANLIB) libiphoneroadmap.a

%.o: %.m
	$(CC) $(CFLAGS) -c $< -o $@

iphoneroadmap: roadmap_main.o libiphoneroadmap.a \
		$(TOP)/libguiroadmap.a $(filter %.a, $(LIBS))
	$(CC) $(LDFLAGS) -o iphoneroadmap roadmap_main.o \
		$(TOP)/libguiroadmap.a libiphoneroadmap.a $(LIBS)

iphoneroadgps: roadmap_main.o libiphoneroadmap.a \
		$(TOP)/libguiroadgps.a  $(filter %.a, $(LIBS))
	$(CC) $(LDFLAGS) -o iphoneroadgps roadmap_main.o \
		$(TOP)/libguiroadgps.a libiphoneroadmap.a $(LIBS)
