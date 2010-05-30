/* iphone_urlscheme.h - iPhone URL scheme handling
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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

#ifndef __ROADMAP_URLSCHEME__H
#define __ROADMAP_URLSCHEME__H

#define URL_MAX_LENGTH        512

void roadmap_urlscheme (void);
BOOL roadmap_urlscheme_pending (void);
BOOL roadmap_urlscheme_init ( const char* url_decoded_escapes );
void roadmap_urlscheme_reset (void);
BOOL roadmap_urlscheme_valid ( const char* raw_url );
void roadmap_urlscheme_remove_prefix( char* dst_url, const char *src_url );

#endif // __ROADMAP_URLSCHEME__H
