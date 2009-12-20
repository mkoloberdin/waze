/* roadmap_download.h - Download mechanism for the roadmap maps.
 *
 * LICENSE:
 *
 *   Copyright 2003 Pascal Martin
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

#ifndef INCLUDE__ROADMAP_DOWNLOAD__H
#define INCLUDE__ROADMAP_DOWNLOAD__H

/* These two callback functions are roadmap_download functions to be called
 * by the protocol module. The first one should be called before saving any
 * data (it is used to check that there is enough space--or make some). The
 * second callback should be called from time to time during the download,
 * to show the progress of the download.
 * These callbacks are called through a pointer, because the protocol
 * modules are designed to be plugins, dynamically linked.
 */
typedef int  (*RoadMapDownloadCallbackSize)     (int size);
typedef void (*RoadMapDownloadCallbackProgress) (int loaded);
typedef void (*RoadMapDownloadCallbackError)    (const char *format, ...);

typedef struct {
   RoadMapDownloadCallbackSize     size;
   RoadMapDownloadCallbackProgress progress;
   RoadMapDownloadCallbackError    error;
} RoadMapDownloadCallbacks;

/* This generic function is a protocol handler function, called once
 * on each file download.
 */
typedef int  (*RoadMapDownloadProtocol)
                  (RoadMapDownloadCallbacks *callbacks,
                   const char *from,
                   const char *to);

/* This callback function is used by the protocol modules to subscribe
 * their protocol handler to the download module.
 */
typedef void (*RoadMapDownloadSubscribe) (const char *prefix,
                                          RoadMapDownloadProtocol handler);


/* This generic function is to be provided by the beneficiary of
 * the download (i.e. a RoadMap module).
 */
typedef void (*RoadMapDownloadEvent) (void);


int roadmap_download_get_county (int fips, int download_usdir,
                                 RoadMapDownloadCallbacks *callbacks);

void roadmap_download_show_space (void);
void roadmap_download_delete (void);

void roadmap_download_subscribe_protocol  (const char *prefix,
                                           RoadMapDownloadProtocol handler);

void roadmap_download_subscribe_when_done (RoadMapDownloadEvent handler);

int  roadmap_download_enabled (void);

void roadmap_download_initialize (void);


/* The following functions make it possible to selectively disable or enable
 * the download of a specific county.
 */
void roadmap_download_block       (int fips);
void roadmap_download_unblock     (int fips);
void roadmap_download_unblock_all (void);
int  roadmap_download_blocked     (int fips);

#endif // INCLUDE__ROADMAP_DOWNLOAD__H

