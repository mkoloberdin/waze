/* roadmap_androidmain.h - the interface to the android main module
 *
 * LICENSE:
 *
 *   Copyright 2008 Alex Agranovich
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * DESCRIPTION:
 *
 *
 *
 *
 */

#ifndef INCLUDE__ROADMAP_ANDROID_MAIN__H
#define INCLUDE__ROADMAP_ANDROID_MAIN__H

#define ANDROID_OS_VER_DONUT	 4
#define ANDROID_OS_VER_ECLAIR  5
#define ANDROID_OS_VER_FROYO   8

// Main loop messages dispatcher
void roadmap_main_message_dispatcher( int aMsg );

void roadmap_main_start_init();

void roadmap_canvas_agg_close(void);

void roadmap_main_shutdown();

int roadmap_main_key_pressed( int aKeyCode, int aIsSpecial, const char* aUtf8Bytes );

void roadmap_main_show_gps_disabled_warning();

BOOL roadmap_horizontal_screen_orientation();

BOOL roadmap_main_mtouch_supported( void );

void roadmap_main_set_build_sdk_version( int aVersion );

int roadmap_main_get_build_sdk_version( void );

void roadmap_main_set_device_name( const char* device_name );

void roadmap_main_set_device_model( const char* model );

void roadmap_main_set_device_manufacturer( const char* manufacturer );

const char* roadmap_main_get_device_name( void );

void roadmap_main_show_contacts( void );

int LogResult( int aVal, const char *aStrPrefix, int level, char *source, int line );

#endif // INCLUDE__ROADMAP_ANDROID_MAIN__H

