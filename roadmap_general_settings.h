/*  roadmap_general_settings.h
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ROADMAP_GENERAL_SETTINGS_H__
#define __ROADMAP_GENERAL_SETTINGS_H__
#include "ssd/ssd_widget.h"

void roadmap_general_settings_show(void);
void quick_settins_exit( int exit_code, void* context);
SsdWidget create_quick_setting_menu();

#endif // __ROADMAP_GENERAL_SETTINGS_H__
