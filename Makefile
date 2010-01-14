# --- Installation options -------------------------------------------------

DESTDIR=
INSTALLDIR=/usr/local
desktopdir=$(INSTALLDIR)/applications
pkgdatadir=$(INSTALLDIR)/share/roadmap
pkgmapsdir=/var/lib/roadmap
bindir=$(INSTALLDIR)/bin
pkgbindir=$(bindir)
menudir=$(DESTDIR)/usr/lib/menu
ICONDIR=$(INSTALLDIR)/share/pixmaps
mandir=$(INSTALLDIR)/share/man
man1dir=$(mandir)/man1

INSTALL      = install
INSTALL_DATA = install -m644


# --- Tool specific options ------------------------------------------------
ifeq ($(DESKTOP),ANDROID)
include android/Makefile.in.android
LIB_INCLUDES = -Izlib -Ifreetype/include -Iandroid
#ZLIB = zlib/libz.a
endif

ifeq ($(DESKTOP),IPHONE)

ifeq ($(IPHONE_TOOLS_DIR),)
IPHONE_TOOLS_DIR=/usr
endif

CC=$(IPHONE_TOOLS_DIR)/bin/arm-apple-darwin9-gcc
AR=$(IPHONE_TOOLS_DIR)/bin/arm-apple-darwin9-ar
RANLIB = $(IPHONE_TOOLS_DIR)/bin/arm-apple-darwin9-ranlib

else

RANLIB = ranlib

endif


WARNFLAGS = -W -Wall -Wno-unused-parameter


ifeq ($(MODE),DEBUG)
# Memory leak detection using mtrace:
# Do not forget to set the trace file using the env. variable MALLOC_TRACE,
# then use the mtrace tool to analyze the output.
ifeq ($(DESKTOP),IPHONE)
   MODECFLAGS=-g $(WARNFLAGS) -ggdb -march=armv6 -mcpu=arm1176jzf-s
else
ifeq ($(DESKTOP),ANDROID)
   MODELDFLAGS=
else
   MODECFLAGS=-g $(WARNFLAGS) -DROADMAP_DEBUG_HEAP -DNOIGNORE
endif   
endif   
   MODELDFLAGS=
else
ifeq ($(MODE),PROFILE)
   MODECFLAGS=-g $(WARNFLAGS) -pg -fprofile-arcs -g
   MODELDFLAGS=-pg
else
   MODECFLAGS=-O2 -ffast-math -fomit-frame-pointer -DNDEBUG=1 $(WARNFLAGS) $(OPTIONS) -DFORCE_INLINE -DINLINE_DEC='extern inline' -fexpensive-optimizations -ffast-math -falign-functions=32 -falign-loops -falign-labels -falign-jumps

   MODELDFLAGS=
endif
endif

ifeq ($(SSD),YES)
# Small screen devices
   SSDCFLAGS=-DSSD 
else
   SSDCFLAGS=
endif

ifeq ($(HI_RES_SCREEN),YES)
   MODECFLAGS+= -DHI_RES_SCREEN 
endif


ifeq ($(CSV_GPS),REALTIME)
   CSVGPSFLAGS=-DCSV_GPS -DCSV_REALTIME
   csv_gps_c=roadmap_gps_csv.c
else ifeq ($(CSV_GPS),YES)
# Small screen devices
   CSVGPSFLAGS=-DCSV_GPS
   csv_gps_c=roadmap_gps_csv.c
else
   CSVGPSFLAGS=
   csv_gps_c=
endif

ifeq ($(J2ME),YES)
# Small screen devices
   J2MECFLAGS=-DJ2MEMAP
else
   J2MECFLAGS=
endif

ifeq ($(DESKTOP),ANDROID)
	RDMODULES=android
else
ifeq ($(DESKTOP),GTK2)
	RDMODULES=gtk2
else
ifeq ($(DESKTOP),GPE)
	RDMODULES=gtk2
else
ifeq ($(DESKTOP),GTK)
	RDMODULES=gtk 
else
ifeq ($(DESKTOP),QT)
	RDMODULES=qt
else
ifeq ($(DESKTOP),QPE)
	RDMODULES=qt
else
ifeq ($(DESKTOP),IPHONE)
	RDMODULES=iphone
	IPHONEFLAGS=-DIPHONE -DTOUCH_SCREEN -I/var/include -I$(IPHONE_TOOLS_DIR)/include
else
	RDMODULES=gtk gtk2 qt
   IPHONEFLAGS=
endif
endif
endif
endif
endif
endif
endif

ifeq ($(TOUCH), YES)
	TOUCHFLAGS=-DTOUCH_SCREEN
else
	TOUCHFLAGS=
endif

HOST=`uname -s`
ifeq ($(HOST),Darwin)
	ARFLAGS="r"
else
	ARFLAGS="r"
endif

CFLAGS=$(MODECFLAGS) $(CSVGPSFLAGS) $(SSDCFLAGS) $(J2MECFLAGS) $(IPHONEFLAGS) $(TOUCHFLAGS) $(CFLAGS_ANDROID) $(LIB_INCLUDES) -I$(PROJ_NAME)/src/ -I/usr/local/include -I$(PWD)

LDFLAGS=$(MODELDFLAGS)

RDMLIBS=libroadmap.a unix/libosroadmap.a libroadmap.a
LIBS=$(RDMLIBS) -lpopt -lm

# --- RoadMap sources & targets --------------------------------------------

RMLIBSRCS=roadmap_log.c \
          roadmap_message.c \
          roadmap_string.c \
          roadmap_voice.c \
          roadmap_list.c \
          roadmap_config.c \
          roadmap_option.c \
          roadmap_metadata.c \
          roadmap_county.c \
          roadmap_locator.c \
          roadmap_math.c \
          roadmap_hash.c \
          roadmap_dbread.c \
          roadmap_dictionary.c \
          roadmap_square.c \
          roadmap_point.c \
          roadmap_line.c \
          roadmap_line_route.c \
          roadmap_line_speed.c \
          roadmap_shape.c \
          roadmap_alert.c \
          roadmap_turns.c \
          roadmap_polygon.c \
          roadmap_street.c \
          roadmap_plugin.c \
          roadmap_geocode.c \
          roadmap_history.c \
          roadmap_input.c \
          roadmap_nmea.c \
          roadmap_gpsd2.c \
          $(csv_gps_c) \
          roadmap_io.c \
          roadmap_gps.c \
          roadmap_state.c \
          roadmap_driver.c \
          roadmap_adjust.c \
          roadmap_lang.c \
          roadmap_city.c \
          roadmap_range.c \
          roadmap_net_mon.c \
          roadmap_cyclic_array.c \
          roadmap_device_events.c \
          roadmap_sunrise.c \
          roadmap_power.c \
          roadmap_camera_image.c \
          roadmap_httpcopy_async.c \
          roadmap.c \
          roadmap_tile_manager.c \
          roadmap_tile_storage.c \
          roadmap_tile_status.c \
          roadmap_tile.c \
          roadmap_warning.c \
	  roadmap_tripserver.c \
          roadmap_jpeg.c \
          roadmap_gzm.c \
	  roadmap_debug_info.c \
	  roadmap_zlib.c \
          roadmap_map_download.c \
          roadmap_reminder.c


RMLIBOBJS=$(RMLIBSRCS:.c=.o)


RMGUISRCS=roadmap_sprite.c \
          roadmap_object.c \
          roadmap_screen_obj.c \
	  roadmap_bar.c \
          roadmap_general_settings.c \
          roadmap_map_settings.c\
          roadmap_download_settings.c\
	  roadmap_mood.c \
	  roadmap_car.c \
          roadmap_trip.c \
          roadmap_skin.c \
          roadmap_layer.c \
          roadmap_fuzzy.c \
          roadmap_navigate.c \
          roadmap_pointer.c \
          roadmap_screen.c \
          roadmap_view.c \
          roadmap_softkeys.c \
          roadmap_utf8.c \
          roadmap_display.c \
	  roadmap_ticker.c \
          roadmap_twitter.c \
          roadmap_foursquare.c \
    	  roadmap_border.c \
          roadmap_factory.c \
          roadmap_preferences.c \
          roadmap_crossing.c \
          roadmap_coord.c \
          roadmap_download.c \
          roadmap_keyboard.c \
          roadmap_phone_keyboard.c \
          roadmap_keyboard_text.c \
          roadmap_help.c \
          roadmap_label.c \
          roadmap_res.c \
          roadmap_login.c \
          auto_hide_dlg.c \
          roadmap_welcome_wizard.c \
	  roadmap_geo_location_info.c \
          roadmap_start.c \
	  roadmap_geo_config.c \
          roadmap_alternative_routes.c \
	  roadmap_res_download.c \
	  roadmap_prompts.c

ifneq ($(SSD),YES)
	RMGUISRCS += roadmap_address.c
else
	RMGUISRCS += roadmap_address_ssd.c roadmap_address_tc.c roadmap_search.c address_search/address_search.c address_search/address_search_dlg.c address_search/local_search.c address_search/local_search_dlg.c address_search/generic_search_dlg.c address_search/generic_search.c ssd/ssd_progress.c \
	roadmap_login_ssd.c	
endif

RMGUIOBJS=$(RMGUISRCS:.c=.o)

RMPLUGINSRCS=roadmap_copy.c \
             roadmap_httpcopy.c \
             roadmap_alerter.c \
             editor/editor_main.c \
             editor/editor_plugin.c \
             editor/editor_screen.c \
	     		 editor/editor_points.c \
	     		 editor/editor_cleanup.c \
             editor/static/add_alert.c \
             editor/static/update_range.c \
             editor/static/edit_marker.c \
             editor/static/notes.c \
             editor/static/editor_dialog.c \
             editor/db/editor_marker.c \
             editor/db/editor_dictionary.c \
             editor/db/editor_shape.c \
             editor/db/editor_point.c \
             editor/db/editor_trkseg.c \
             editor/db/editor_street.c \
             editor/db/editor_line.c \
             editor/db/editor_override.c \
             editor/db/editor_db.c \
             editor/track/editor_gps_data.c \
             editor/track/editor_track_filter.c \
             editor/track/editor_track_known.c \
             editor/track/editor_track_unknown.c \
             editor/track/editor_track_util.c \
             editor/track/editor_track_main.c \
             editor/track/editor_track_compress.c \
             editor/track/editor_track_report.c \
             editor/export/editor_sync.c \
             editor/export/editor_download.c \
             editor/export/editor_report.c \
             editor/export/editor_upload.c \
	     editor/editor_bar.c \
             navigate/navigate_main.c \
             navigate/navigate_instr.c \
             navigate/navigate_bar.c \
             navigate/navigate_zoom.c \
             navigate/navigate_plugin.c \
             navigate/navigate_traffic.c \
             navigate/navigate_graph.c \
             navigate/navigate_cost.c \
             navigate/navigate_route_astar.c \
             navigate/fib-1.1/fib.c \
             navigate/navigate_route_trans.c \

RMPLUGINOBJS=$(RMPLUGINSRCS:.c=.o)

RTSRCS=Realtime/Realtime.c \
       Realtime/RealtimeNet.c \
       Realtime/RealtimeAlerts.c \
       Realtime/RealtimeAlertsList.c \
       Realtime/RealtimeTrafficInfo.c \
       Realtime/RealtimeTrafficInfoPlugin.c \
       Realtime/RealtimeAlertCommentsList.c \
       Realtime/RealtimeNetDefs.c \
       Realtime/RealtimeNetRec.c \
       Realtime/RealtimeUsers.c \
       Realtime/RealtimeMath.c \
       Realtime/RealtimeDefs.c \
       Realtime/RealtimeSystemMessage.c \
       Realtime/RealtimePrivacy.c \
       Realtime/RealtimeOffline.c \
       Realtime/RealtimeBonus.c \
       Realtime/RealtimeAltRoutes.c \
       



RTOBJS=$(RTSRCS:.c=.o)

WEBSVCSRCS=websvc_trans/cyclic_buffer.c \
			  websvc_trans/efficient_buffer.c \
			  websvc_trans/socket_async_receive.c \
			  websvc_trans/string_parser.c \
		       	  websvc_trans/web_date_format.c \
			  websvc_trans/websvc_address.c \
			  websvc_trans/websvc_trans_queue.c \
			  websvc_trans/websvc_trans.c \
			  websvc_trans/mkgmtime.c

			  
WEBSVCOBJS=$(WEBSVCSRCS:.c=.o)

SSD_WIDGETS_SRCS=ssd/ssd_dialog.c \
                 ssd/ssd_widget.c \
                 ssd/ssd_container.c \
                 ssd/ssd_text.c \
                 ssd/ssd_entry.c \
                 ssd/ssd_button.c \
                 ssd/ssd_list.c \
                 ssd/ssd_generic_list_dialog.c \
				 ssd/ssd_keyboard.c \
				 ssd/ssd_keyboard_layout.c \
				 ssd/ssd_keyboard_dialog.c \
                 ssd/ssd_menu.c \
                 ssd/ssd_messagebox.c \
		 		 ssd/ssd_confirm_dialog.c \
                 ssd/ssd_choice.c \
                 ssd/ssd_checkbox.c \
		 	     ssd/ssd_combobox.c \
                 ssd/ssd_combobox_dialog.c \
		 		 ssd/ssd_contextmenu.c \
		         ssd/ssd_tabcontrol.c \
				 ssd/ssd_bitmap.c \
				 ssd/ssd_popup.c \
				 ssd/ssd_separator.c \
                 ssd/ssd_icon.c \
                 ssd/ssd_widget_tab_order.c \
                 ssd/ssd_progress_msg_dialog.c \
                 roadmap_strings.c 
                 

SSD_WIDGETS_OBJS=$(SSD_WIDGETS_SRCS:.c=.o)

RUNTIME=$(RDMLIBS) libguiroadmap.a


# --- Conventional targets ----------------------------------------

all: everything

runtime: $(RUNTIME) 

strip:
	strip -s $(RUNTIME)

clean: cleanone
	for module in $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			$(MAKE) -C $$module cleanone || exit 1; \
		fi ; \
	done
	if [ -d unix ] ; then $(MAKE) -C unix cleanone ; fi
	if [ -d iphone ] ; then $(MAKE) -C iphone clean ; fi
	if [ -d zlib ] ; then $(MAKE) -C zlib clean ; fi
	find address_search -name \*.o -exec rm {} \;
	find editor -name \*.o -exec rm {} \;
	find navigate -name \*.o -exec rm {} \;
	find agg -name \*.o -exec rm {} \;
	find ssd -name \*.o -exec rm {} \;
	find Realtime -name \*.o -exec rm {} \;
	find websvc_trans -name \*.o -exec rm {} \;
	find address_search -name \*.o -exec rm {} \;

cleanone:
	rm -f *.o *.a *.da 
	# Clean up CVS backup files as well.
	$(RM) .#*

everything: modules runtime

modules:
	for module in $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			$(MAKE) -C $$module STDCFLAGS="$(CFLAGS)" all || exit 1; \
		fi ; \
	done

cleanall:
	for module in $(RDMODULES) ; \
	do \
		if [ -d $$module ] ; then \
			$(MAKE) -C $$module clean ; \
		fi ; \
	done
	find editor/ -name \*.o -exec rm {} \;

rebuild: cleanall everything

# --- The real targets --------------------------------------------

libroadmap.a: $(RMLIBOBJS)
	$(AR) $(ARFLAGS) libroadmap.a $(RMLIBOBJS)
	$(RANLIB) libroadmap.a

libguiroadmap.a: $(RMGUIOBJS) $(RMPLUGINOBJS) $(RTOBJS) $(WEBSVCOBJS)
	$(AR) $(ARFLAGS) libguiroadmap.a $(RMGUIOBJS) $(RMPLUGINOBJS) $(RTOBJS) $(WEBSVCOBJS)
	$(RANLIB) libguiroadmap.a

unix/libosroadmap.a:
	if [ -d unix ] ; then $(MAKE) -C unix "CFLAGS=$(CFLAGS) -I.." ; fi

libssd_widgets.a: $(SSD_WIDGETS_OBJS)
	$(AR) $(ARFLAGS) libssd_widgets.a $(SSD_WIDGETS_OBJS)
	$(RANLIB) libssd_widgets.a

zlib/libz.a: 
	$(MAKE) -C zlib
