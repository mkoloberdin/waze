/* roadmap_ssl.h - SSL implementation
 *
 * LICENSE:
 *
 *   Copyright 2011, Avi R.
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
 
#ifndef INCLUDE__ROADMAP_SSL__H
#define INCLUDE__ROADMAP_SSL__H

struct roadmap_ssl_io_t;
typedef struct roadmap_ssl_io_t *RoadMapSslIO;

#if 0

//void roadmap_ssl_set_secured_input (RoadMapIO *io, RoadMapInput callback, void *ssl_context);
#endif

int roadmap_ssl_read (void *context, void *buffer, int buffer_size);
int roadmap_ssl_send (RoadMapSslIO io, const void *data, int length);
int roadmap_ssl_open (RoadMapSocket s, void *data, RoadMapNetSslConnectCallback callback);
void roadmap_ssl_close (void *context);

#endif /* INCLUDE__ROADMAP_SSL__H */
