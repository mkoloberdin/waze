/* roadmap_res_download.h
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


#ifndef ROADMAP_RES_DOWNLOAD_H_
#define ROADMAP_RES_DOWNLOAD_H_

typedef void (*RoadMapResDownloadCallback) (const char* res_name, int success, void *context, char *last_modified);

#define RES_DOWNLOAD_IMAGE    0
#define RES_DOWNLOAD_SOUND    1
#define RES_DOWNLOAD_CONFIFG  2
#define RES_DOWNLOAD_LANG     3


void roadmap_res_download_init (void);
void roadmap_res_download_term (void);
void roadmap_res_download (int type, const char*name, const char *lang, BOOL override,time_t update_time,RoadMapResDownloadCallback on_loaded, void *context);

#endif /* ROADMAP_RES_DOWNLOAD_H_ */
