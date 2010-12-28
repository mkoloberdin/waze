/* roadmap_social_image.h - Manages social network images
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi B.S
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

#ifndef ROADMAP_SOCIAL_IMAGE_H_
#define ROADMAP_SOCIAL_IMAGE_H_

#include "roadmap_canvas.h"
#include "ssd/ssd_widget.h"

#define  SOCIAL_IMAGE_CONFIG_PREF_TYPE       ("preferences")
#define  SOCIAL_IMAGE_CONFIG_TAB             ("Social Image")
#define  SOCIAL_IMAGE_CONFIG_URL_PREFIX      ("Url prefix")

typedef void (*SocialImageDownloadCallback) ( void* context, int status, RoadMapImage image );

#define SOCIAL_IMAGE_ID_TYPE_USER  1
#define SOCIAL_IMAGE_ID_TYPE_ALERT 2

#define SOCIAL_IMAGE_SIZE_SQUARE -1
#define SOCIAL_IMAGE_SIZE_SMALL  -2
#define SOCIAL_IMAGE_SIZE_LARGE  -3

#define SOCIAL_IMAGE_SOURCE_FACEBOOK 1

void roadmap_social_image_initialize( void );
void roadmap_social_image_terminate( void );
BOOL roadmap_social_image_download( int source, int id_type, int id, int id2, int size, void* context_cb, SocialImageDownloadCallback download_cb );
BOOL roadmap_social_image_download_update_bitmap( int source, int id_type, int id, int id2, int size, SsdWidget bitmap );
#endif /* ROADMAP_SOCIAL_IMAGE_H_ */
