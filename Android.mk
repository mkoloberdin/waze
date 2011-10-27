###########################################################################################
# Main common sources of the waze application
# makes static libraries only (See end of script)
# Copyright (C) 2011 Waze Ltd
# Author: Alex Agranovich (AGA)
#

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
          roadmap_http_comp.c \
          roadmap_io.c \
          roadmap_gps.c \
          roadmap_state.c \
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
          roadmap_base64.c \
          roadmap.c \
          roadmap_tile_manager.c \
          roadmap_tile_status.c \
          roadmap_tile.c \
          roadmap_urlscheme.c \
          roadmap_warning.c \
	  roadmap_tripserver.c \
          roadmap_jpeg.c \
          roadmap_gzm.c \
	  roadmap_debug_info.c \
	  roadmap_zlib.c \
          roadmap_map_download.c \
          roadmap_reminder.c \
          roadmap_scoreboard.c \
          roadmap_speechtt.c \
          roadmap_recorder.c \
          roadmap_recorder_dlg.c \
          roadmap_analytics.c \
	  roadmap_tile_storage_sqlite.c

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
	  roadmap_message_ticker.c \
          roadmap_social.c \
          roadmap_foursquare.c \
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
          roadmap_recommend_ssd.c \
	  roadmap_geo_location_info.c \
          roadmap_start.c \
	  roadmap_geo_config.c \
          roadmap_alternative_routes.c \
	  roadmap_res_download.c \
	  roadmap_prompts.c \
	  roadmap_splash.c \
	  roadmap_speedometer.c \
	  roadmap_social_image.c \
	  roadmap_browser.c \
	  roadmap_groups.c \
	  roadmap_groups_settings.c \
	  roadmap_address_ssd.c \
          roadmap_address_tc.c \
          roadmap_search.c \
          address_search/address_search.c \
          address_search/address_search_dlg.c \
          address_search/local_search.c \
          address_search/local_search_dlg.c \
          address_search/single_search.c \
          address_search/single_search_dlg.c \
          address_search/generic_search_dlg.c \
          address_search/generic_search.c \
          ssd/ssd_progress.c \
	 roadmap_login_ssd.c	


#OPENGL DEPENDENT SOURCES
ifeq ($(RENDERING),OPENGL)
   RMGUISRCS += roadmap_border_ogl.c animation/roadmap_animation.c
   RMLIBSRCS+= $(OPENGL_DIR)/roadmap_canvas.c $(OPENGL_DIR)/roadmap_canvas_font.c 
   RMLIBSRCS+= $(OPENGL_DIR)/roadmap_canvas_atlas.c $(OPENGL_DIR)/roadmap_canvas3d.c $(OPENGL_DIR)/roadmap_glmatrix.c  
else  
   RMGUISRCS += roadmap_border.c
endif


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
             navigate/navigate_res_dlg.c \
             navigate/navigate_route_events.c

ifeq ($(TTS),YES)
  RMLIBSRCS += tts/tts_utils.c tts/tts.c tts/tts_queue.c tts/tts_voices.c tts/tts_db.c tts/tts_cache.c tts/tts_db_files.c tts/tts_db_sqlite.c tts/tts_ui.c
  RMLIBSRCS += tts_was_provider.c tts_apptext.c	
  RMPLUGINSRCS += navigate/navigate_tts.c 
endif

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
       Realtime/RealtimeExternalPoi.c \
       Realtime/RealtimeExternalPoiDlg.c \
       Realtime/RealtimeExternalPoiNotifier.c \
       Realtime/RealtimeTrafficDetection.c \
       Realtime/RealtimePopUp.c \
       
       

WEBSVCSRCS=websvc_trans/cyclic_buffer.c \
	   websvc_trans/efficient_buffer.c \
	   websvc_trans/socket_async_receive.c \
           websvc_trans/string_parser.c \
	   websvc_trans/web_date_format.c \
	   websvc_trans/websvc_address.c \
	   websvc_trans/websvc_trans_queue.c \
           websvc_trans/websvc_trans.c \
           websvc_trans/mkgmtime.c

SSD_WIDGETS_SRCS=ssd/ssd_dialog.c \
                 ssd/ssd_widget.c \
                 ssd/ssd_container.c \
                 ssd/ssd_text.c \
                 ssd/ssd_entry.c \
                 ssd/ssd_entry_label.c \
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
                 ssd/ssd_segmented_control.c \
                 roadmap_strings.c                  

UNIX_SRCS=unix/roadmap_file.c \
	      unix/roadmap_library.c \
	      unix/roadmap_net.c \
	      md5.c \
	      unix/roadmap_serial.c \
	      unix/roadmap_device_events.c \
	      unix/roadmap_input_type.c \
	      unix/roadmap_time.c \
	      unix/roadmap_ssl.c
#	      unix/resolver.c

GLU_SRCS= $(OPENGL_DIR)/glu/libtess/dict.c \
	$(OPENGL_DIR)/glu/libtess/geom.c \
	$(OPENGL_DIR)/glu/libtess/memalloc.c \
	$(OPENGL_DIR)/glu/libtess/mesh.c \
	$(OPENGL_DIR)/glu/libtess/normal.c \
	$(OPENGL_DIR)/glu/libtess/priorityq.c \
	$(OPENGL_DIR)/glu/libtess/render.c \
	$(OPENGL_DIR)/glu/libtess/sweep.c \
	$(OPENGL_DIR)/glu/libtess/tess.c \
	$(OPENGL_DIR)/glu/libtess/tessmono.c \
	$(OPENGL_DIR)/glu/gluUnProject.c \
	$(OPENGL_DIR)/glu/gluProject.c \
	$(OPENGL_DIR)/glu/gluLookAt.c \
	$(OPENGL_DIR)/glu/gluPerspective.c 

###########################################
# Target Build: libroadmap.a
include $(CLEAR_VARS)
include $(MY_LOCAL_PATH)/Android.Common.mk
LOCAL_MODULE    := libroadmap
LOCAL_SRC_FILES := $(RMLIBSRCS) $(RMPLUGINSRCS) $(WEBSVCSRCS)
include $(BUILD_STATIC_LIBRARY)

###########################################
# Target Build: librt.a
include $(CLEAR_VARS)
include $(MY_LOCAL_PATH)/Android.Common.mk
LOCAL_MODULE    := librt
LOCAL_SRC_FILES := $(RTSRCS)
include $(BUILD_STATIC_LIBRARY)

###########################################
# Target Build: libssd.a
include $(CLEAR_VARS)
include $(MY_LOCAL_PATH)/Android.Common.mk
LOCAL_MODULE    := libssd
LOCAL_SRC_FILES := $(SSD_WIDGETS_SRCS)
include $(BUILD_STATIC_LIBRARY)


###########################################
# Target Build: libguiroadmap.a
include $(CLEAR_VARS)
include $(MY_LOCAL_PATH)/Android.Common.mk
LOCAL_MODULE     := libguiroadmap
LOCAL_SRC_FILES  := $(RMGUISRCS)
include $(BUILD_STATIC_LIBRARY)

###########################################
# Target Build: libunix.a
include $(CLEAR_VARS)
include $(MY_LOCAL_PATH)/Android.Common.mk
LOCAL_MODULE     := libunix
LOCAL_SRC_FILES  := $(UNIX_SRCS)
include $(BUILD_STATIC_LIBRARY)

###########################################
# Target Build: libglu.a
include $(CLEAR_VARS)
include $(MY_LOCAL_PATH)/Android.Common.mk
GLU_DIR=$(LOCAL_PATH)/$(OPENGL_DIR)/glu
LOCAL_MODULE     := libglu
LOCAL_SRC_FILES  := $(GLU_SRCS)
LOCAL_C_INCLUDES := $(LOCAL_C_INCLUDES) $(GLU_DIR) $(GLU_DIR)/include $(GLU_DIR)/libtess $(LOCAL_PATH)/$(OPENGL_DIR)
include $(BUILD_STATIC_LIBRARY)



