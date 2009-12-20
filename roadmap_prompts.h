/* roadmap_prompts.h
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi Ben-Shoshan
 *
 *   This file is part of RoadMap.
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
 *
 *
 */


#ifndef ROADMAP_PROMPTS_H_
#define ROADMAP_PROMPTS_H_
void roadmap_prompts_download(const char *lang);
void roadmap_prompts_init(void);
const char *roadmap_prompts_get_name(void);
void roadmap_prompts_set_name(const char *name);
int roadmap_prompts_get_count(void);
const void **roadmap_prompts_get_values(void);
const char **roadmap_prompts_get_labels(void);
const void *roadmap_prompts_get_prompt_value(const char *value);
const void *roadmap_prompts_get_prompt_value_from_name(const char *name);
BOOL roadmap_prompts_exist(const char *name);
const char *roadmap_prompts_get_label (const char *value);
void roadmap_prompts_set_update_time (const char *update_time);
#endif /* ROADMAP_PROMPTS_H_ */
