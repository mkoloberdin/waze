
ifeq ($(DESKTOP),ANDROID)
include ../../android/Makefile.in.android
else
CC=cc
RANLIB=ranlib
endif

# --- Tool specific options ------------------------------------------------
INCLUDES+= -Iinclude -Ilibtess -I. -I../
LIBDIRS+= 

ARFLAGS="r"

WARNFLAGS = -W -Wall -Wno-unused-parameter -Wno-extra

ifeq ($(MODE),DEBUG)
# Memory leak detection using mtrace:
# Do not forget to set the trace file using the env. variable MALLOC_TRACE,
# then use the mtrace tool to analyze the output.
   MODECFLAGS=-g $(WARNFLAGS) -DROADMAP_DEBUG_HEAP -DNOIGNORE    
   MODELDFLAGS=
else
ifeq ($(MODE),PROFILE)
   MODECFLAGS=-g $(WARNFLAGS) -pg -fprofile-arcs -g
   MODELDFLAGS=-pg
else
   MODECFLAGS=-O2 -ffast-math -fomit-frame-pointer -DNDEBUG=1 $(WARNFLAGS) $(OPTIONS)
   MODELDFLAGS=
endif
endif
	
CFLAGS=$(MODECFLAGS) $(CFLAGS_ANDROID) $(INCLUDES) -I$(PWD) $(LIBDIRS)
CXXFLAGS=$(CFLAGS) 
LDFLAGS=$(MODELDFLAGS)

LIBS=libglu.a

# --- Sources & targets --------------------------------------------
#TODO ....
SRCS= libtess/dict.c \
		libtess/geom.c \
		libtess/memalloc.c \
		libtess/mesh.c \
		libtess/normal.c \
		libtess/priorityq.c \
		libtess/render.c \
		libtess/sweep.c \
		libtess/tess.c \
		libtess/tessmono.c \
		gluUnProject.c \
		gluProject.c \
		gluLookAt.c \
		gluPerspective.c 
	
	 
OBJS=$(SRCS:.c=.o)

RUNTIME=$(LIBS)

# --- Conventional targets ----------------------------------------

all: everything

runtime: $(RUNTIME) 

strip:
	strip -s $(RUNTIME)

clean: cleanall
		
cleanone: cleanall
	
everything: runtime

cleanall:
	find . -name \*.o -exec rm {} \;
rebuild: cleanall everything

# --- The real targets --------------------------------------------

libglu.a: $(OBJS)
	  $(AR) $(ARFLAGS) libglu.a $(OBJS)
	  $(RANLIB) libglu.a

