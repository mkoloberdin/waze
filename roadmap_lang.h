/* roadmap_lang.h - I18n
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 */

#ifndef __ROADMAP_LANG__H
#define __ROADMAP_LANG__H

#include "roadmap.h"

void roadmap_lang_initialize (void);
const char* roadmap_lang_get (const char *name);
int roadmap_lang_rtl (void);
const char *roadmap_lang_get_system_lang();
void roadmap_lang_set_system_lang(const char *lang, BOOL download);
const void *roadmap_lang_get_lang_value(const char *value);
void roadmap_lang_reload(void);
void roadmap_lang_set_update_time(const char *update_time);
const char *roadmap_lang_get_update_time(void);
void roadmap_lang_set_default_lang(const char *lang);
const char *roadmap_lang_get_default_lang();
void roadmap_lang_download_lang_file(const char *lang, RoadMapCallback callback);
int roadmap_lang_get_available_langs_count(void);
const void **roadmap_lang_get_available_langs_values(void);
const char **roadmap_lang_get_available_langs_labels(void);
const void *roadmap_lang_get_label(const char *value);
void roadmap_lang_download_conf_file(RoadMapCallback callback);
void download_lang_files(void);
const char *roadmap_lang_get_user_lang();
void roadmap_lang_set_update_time(const char *update_time);
void roadmap_lang_reload(void);
#endif // __ROADMAP_LANG__H
