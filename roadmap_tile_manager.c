/* roadmap_tile_manager.c - Manage loading of map tiles.
 *
 * LICENSE:
 *
 *   Copyright 2009 Israel Disatnik.
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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap_tile_manager.h"
#include "roadmap_tile_storage.h"
#include "roadmap_math.h"
#include "roadmap.h"
#include "roadmap_tile_status.h"
#include "roadmap_httpcopy_async.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_locator.h"
#include "roadmap_data_format.h"
#include "roadmap_label.h"
#include "roadmap_square.h"
#include "roadmap_main.h"
#include "roadmap_config.h"
#include "navigate/navigate_graph.h"
#include "Realtime/Realtime.h"
#include "roadmap_street.h"
#include "roadmap_tile.h"
#include "roadmap_config.h"
#include "roadmap_warning.h"
#include "roadmap_lang.h"
#include "roadmap_map_download.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "navigate/navigate_main.h"

#if defined(__SYMBIAN32__) || defined(J2ME)
#define	TM_MAX_CONCURRENT		1
#elif defined(IPHONE) || defined(ANDROID) 
#define	TM_MAX_CONCURRENT		6
#else
#define	TM_MAX_CONCURRENT		3
#endif

#ifdef _WIN32
#define TM_MAX_QUEUE						128
#else
#define TM_MAX_QUEUE						256
#endif

#define TM_RETRY_CONNECTION_SECONDS	10
#define TM_HTTP_TIMEOUT_SECONDS		5

typedef struct {

	time_t				time_out;
	int					tile_index;
	int					*tile_status;
	char					url[512];
	RoadMapCallback	callback;
	char					*tile_data;
	size_t				tile_size;
	HttpAsyncContext	*http_context;
} ConnectionContext;


static enum {
	stat_First,
	stat_Waiting,
	stat_Active
} Status = stat_First;

static ConnectionContext 			*Connections = NULL;
static int					  		 	NumOpenConnections = 0;

static RoadMapConfigDescriptor 	RoadMapConfigTilesUrl =
                                  ROADMAP_CONFIG_ITEM("Download", "Tiles");

typedef struct {

	int					tile_index;
	int					priority;
	RoadMapCallback	callback;
} TileData;

static TileData						RequestQueue[TM_MAX_QUEUE];
static int								QueueHead = 0;
static int								QueueSize = 0;
static RoadMapCallback				NextLoginCallback = NULL;
static RoadMapTileCallback			TileCallback = NULL;
static int								ActiveLoadingSession = 0;
static int                       TilesRefreshProgressCount = 0;         // Currently updated tiles count
static int                       TilesRefreshTotalCount = -1;           // Total number of tiles to be updated

static RoadMapConfigDescriptor 	LastLoadingSessionCfg =
                        ROADMAP_CONFIG_ITEM("Tiles", "Last Session");
static RoadMapConfigDescriptor 	LoadingSessionLifetimeCfg =
                        ROADMAP_CONFIG_ITEM("Tiles", "Loading session lifetime");



static void load_next_tile (void);
static void queue_tile (int index, int push, RoadMapCallback on_loaded);
static void roadmap_tile_manager_login_cb (void);
static void on_connection_failure (ConnectionContext *conn);
#ifndef INLINE_DEC
#define INLINE_DEC static
#endif //INLINE_DEC
INLINE_DEC void tile_refresh_cb( int tile_id );
static void init_loading_session (void) {

	int last_session;
	int session_period;
	int time_now;
	static int initialized = 0;

	if (initialized) return;
	initialized = 1;

	time_now = (int)time (NULL);

	roadmap_config_declare ("session", &LastLoadingSessionCfg, "0", NULL);
	roadmap_config_declare ("preferences", &LoadingSessionLifetimeCfg, "604800", NULL); // seconds in one week

	last_session = roadmap_config_get_integer (&LastLoadingSessionCfg);
	session_period = roadmap_config_get_integer (&LoadingSessionLifetimeCfg);

	if (time_now > last_session + session_period) {
		ActiveLoadingSession = 1;
		roadmap_config_set_integer (&LastLoadingSessionCfg, time_now);
		roadmap_config_save (0);
	}
}


static void init_connections (void) {

	int i;

	if (Connections != NULL) return;

	Connections = (ConnectionContext *) malloc (TM_MAX_CONCURRENT * sizeof (ConnectionContext));
	for (i = 0; i < TM_MAX_CONCURRENT; i++) {
		Connections[i].tile_status = NULL;
	}
}


static int  http_cb_size (void *context, size_t size) {

   ConnectionContext *conn = (ConnectionContext *)context;

   //printf ("Size for %s is %d\n", conn->url, size);
	conn->tile_data = malloc (size);
	conn->tile_size = 0;

	return size;
}

static void http_cb_progress (void *context, char *data, size_t size) {

   ConnectionContext *conn = (ConnectionContext *)context;

	if (size && (conn->tile_data != NULL)) {
		memcpy (conn->tile_data + conn->tile_size, data, size);
		conn->tile_size += size;
	}
	conn->time_out = time (NULL) + TM_HTTP_TIMEOUT_SECONDS;
}

static void http_cb_error (void *context, int connection_failure, const char *format, ...) {

   va_list ap;
   ConnectionContext *conn = (ConnectionContext *)context;
   char err_string[1024];

   //printf ("Error downloading %s\n", conn->url);

   va_start (ap, format);
   vsnprintf (err_string, 1024, format, ap);
   va_end (ap);

   if (connection_failure) {
		roadmap_log (ROADMAP_ERROR, "Connection error on tile %d: %s", conn->tile_index, err_string);
   } else {
		roadmap_log (ROADMAP_DEBUG, "Download error on tile %d: %s", conn->tile_index, err_string);
   }

	if (conn->tile_data) {
		free (conn->tile_data);
		conn->tile_data = NULL;
	}

	// Update the refresh progress also in case of error on tile
   tile_refresh_cb( conn->tile_index );

	if (connection_failure) {
	   // The callback responsibility to "null" the context in case of failure
	   // The memory is deallocated out of this function
	   // The timeout value is irrelevant in case of known failure
	   conn->http_context  = NULL;
	   conn->time_out = 0;
		on_connection_failure (conn);
		return;
	}

   *conn->tile_status = ((*conn->tile_status) |
   							 (ROADMAP_TILE_STATUS_FLAG_ERROR | ROADMAP_TILE_STATUS_FLAG_UPTODATE)) &
   							~ROADMAP_TILE_STATUS_FLAG_ACTIVE;
   if (conn->callback) {
   	conn->callback ();
   }
   conn->tile_status = NULL;
   NumOpenConnections--;

   load_next_tile ();
}

#ifndef J2ME
#define NOPH_System_currentTimeMillis() time(NULL)
#endif
static void http_cb_done (void *context,char *last_modified) {

   ConnectionContext *conn = (ConnectionContext *)context;
	int *tile_status = conn->tile_status;
   int tile_index = conn->tile_index;
   int unloaded;
	time_t t1;
	time_t t2;
	int rc;

	t1 = NOPH_System_currentTimeMillis();
   unloaded = roadmap_locator_unload_tile (tile_index);
   t2 = NOPH_System_currentTimeMillis();
   //printf("http_cb_done: unload %dms\n", t2 - t1);

   roadmap_tile_store(roadmap_locator_active(), tile_index, conn->tile_data, conn->tile_size);

   t2 = NOPH_System_currentTimeMillis();
   //printf("http_cb_done: save %dms\n", t2 - t1);
   *tile_status = ((*tile_status) |
						 (ROADMAP_TILE_STATUS_FLAG_EXISTS | ROADMAP_TILE_STATUS_FLAG_UPTODATE)) &
						~ROADMAP_TILE_STATUS_FLAG_ACTIVE;
   conn->tile_status = NULL;
   NumOpenConnections--;

  	roadmap_label_clear (tile_index);
  	navigate_graph_clear (tile_index);
   if (!unloaded) {
   	roadmap_square_delete_reference (tile_index);
   }

	rc = roadmap_locator_load_tile_mem (tile_index, conn->tile_data, conn->tile_size);
	free (conn->tile_data);
	conn->tile_data = NULL;

   t2 = NOPH_System_currentTimeMillis();
   //printf("http_cb_done: load %dms\n", t2 - t1);

   // Tiles refresh progress (if active)
   tile_refresh_cb( tile_index );

   load_next_tile ();

   if (rc != ROADMAP_US_OK) {
		return;
	}

  	if (roadmap_tile_get_scale (tile_index) == 0) {
		roadmap_street_update_city_index ();
  	}

   if (conn->callback) {
   	conn->callback ();
   }

	roadmap_log (ROADMAP_DEBUG, "Download of tile %d complete", tile_index);
   if (TileCallback != NULL &&
   	 (*tile_status) & ROADMAP_TILE_STATUS_FLAG_CALLBACK) {
   	roadmap_log (ROADMAP_DEBUG, "Calling callback for tile %d", tile_index);
   	TileCallback (tile_index);
   }

   if (roadmap_square_at_current_scale(tile_index)) {
      RoadMapArea screen_edges;
      RoadMapArea tile_edges;

      roadmap_math_screen_edges (&screen_edges);

      roadmap_tile_edges (tile_index, &tile_edges.west, &tile_edges.east,
                          &tile_edges.south, &tile_edges.north);

      if (roadmap_math_area_contains(&screen_edges, &tile_edges)) {
         roadmap_screen_redraw();
      }
   }
}

static void init_url (void) {

   roadmap_config_declare
      ("preferences",
      &RoadMapConfigTilesUrl, "", NULL);
}

static const char *get_url_prefix (void) {

	return roadmap_config_get (&RoadMapConfigTilesUrl);
}

static void get_url (ConnectionContext *context) {

	int fips = roadmap_locator_active ();
	int tile_id = context->tile_index;

	snprintf (context->url,
				 sizeof (context->url),
				 "%s/%05d_%02x/%05d_%04x/%05d_%06x/%05d_%08x%s",
				 get_url_prefix (),
				 fips, tile_id >> 24,
				 fips, tile_id >> 16,
				 fips, tile_id >> 8,
				 fips, tile_id, ROADMAP_DATA_TYPE);
}


static void next_to_load (int *tile_index, int *priority, RoadMapCallback *callback) {

	if (QueueSize <= 0) {
		*tile_index = -1;
		return;
	}

	*tile_index = RequestQueue[QueueHead].tile_index;
	*callback = RequestQueue[QueueHead].callback;
	*priority = RequestQueue[QueueHead].priority;
	QueueHead = (QueueHead + 1) % TM_MAX_QUEUE;
	QueueSize--;
}


static void load_next_tile (void) {

	static RoadMapHttpAsyncCallbacks callbacks = { http_cb_size, http_cb_progress, http_cb_error, http_cb_done };
	int conn;
	time_t tile_time;
	int tile_index;
	int priority;
	int *tile_status;
	RoadMapCallback tile_callback;

   roadmap_log(ROADMAP_DEBUG, "load_next_tile - status:%d", Status);

	if (NumOpenConnections >= TM_MAX_CONCURRENT || Status != stat_Active) {
		return;
	}

	do {
		next_to_load (&tile_index, &priority, &tile_callback);
		if (tile_index == -1) {
			return;
		}
		tile_status = roadmap_tile_status_get (tile_index);
		assert (tile_status != NULL);
		if (((*tile_status) & ROADMAP_TILE_STATUS_FLAG_UPTODATE ) && tile_callback) {
			tile_callback ();
		}
		*tile_status &= ~ROADMAP_TILE_STATUS_FLAG_QUEUED;
	}
	while ((*tile_status) & (ROADMAP_TILE_STATUS_FLAG_ACTIVE | ROADMAP_TILE_STATUS_FLAG_UPTODATE));

	roadmap_log (ROADMAP_DEBUG, "Loading tile %d -- priority %d",
						tile_index, priority);

	for (conn = 0; conn < TM_MAX_CONCURRENT; conn++) {

		if (Connections[conn].tile_status == NULL) break;
	}

	assert (conn < TM_MAX_CONCURRENT);

	Connections[conn].tile_index = tile_index;
	Connections[conn].tile_status = tile_status;
	Connections[conn].callback = tile_callback;
	Connections[conn].time_out = 0;
	Connections[conn].tile_data = NULL;
	Connections[conn].tile_size = 0;
	get_url (&Connections[conn]);

	//printf ("Requesting %s\n", Connections[conn].url);

	*tile_status |= ROADMAP_TILE_STATUS_FLAG_ACTIVE;

	NumOpenConnections++;
	tile_time = roadmap_square_timestamp (tile_index);

	Connections[conn].http_context =
		roadmap_http_async_copy (&callbacks,
										 &Connections[conn],
										 Connections[conn].url,
										 tile_time);

	// failure is handled by http_cb_error
}

static void requeue_tile (ConnectionContext *conn) {

   *conn->tile_status = (*conn->tile_status) & ~ROADMAP_TILE_STATUS_FLAG_ACTIVE;
	queue_tile (conn->tile_index, (*conn->tile_status) & ROADMAP_TILE_STATUS_MASK_PRIORITY, conn->callback);
   conn->tile_status = NULL;
   NumOpenConnections--;
}

static void roadmap_tile_manager_check_timeouts (void) {

	int i;
	time_t time_now = time (NULL);

	for (i = 0; i < TM_MAX_CONCURRENT; i++) {
		ConnectionContext *conn = Connections + i;
		if (conn->tile_status != NULL && conn->time_out && conn->time_out < time_now) {
			roadmap_log (ROADMAP_ERROR, "Timed out waiting for tile %d", conn->tile_index);
			roadmap_http_async_copy_abort (conn->http_context);
			requeue_tile (conn);
		}
	}
}

static void start_network (void) {
   init_url ();
	Status = stat_Active;
	load_next_tile ();
}

static void roadmap_tile_manager_login_cb (void) {

   roadmap_log(ROADMAP_DEBUG, "roadmap_tile_manager_login_cb called!");
	start_network ();
	roadmap_main_set_periodic (TM_HTTP_TIMEOUT_SECONDS * 1000, roadmap_tile_manager_check_timeouts);
	if (NextLoginCallback) {
		NextLoginCallback ();
		NextLoginCallback = NULL;
	}
}

static void on_connection_failure (ConnectionContext *conn) {

	requeue_tile (conn);
	if (Status != stat_Active) return;
	Status = stat_Waiting;
	roadmap_main_set_periodic (TM_RETRY_CONNECTION_SECONDS * 1000, start_network);
}

static void queue_tile (int index, int priority, RoadMapCallback on_loaded) {

	int slot;

	if (priority) {
		int pos;
		int test_slot;

		QueueHead = (QueueHead + TM_MAX_QUEUE - 1) % TM_MAX_QUEUE;

		// Note{ID}:
		// This is a safety against removing callback items from queue
		// assuming there is only one callback item at a time.
		// if things change (more than one possible callback item)
		// change it to call the callback (it will work but will demand more CPU)
		if (QueueSize == TM_MAX_QUEUE) {
			if (RequestQueue[QueueHead].priority > priority) {
				roadmap_log (ROADMAP_WARNING, "Tile request queue is full with prioritized items");
				return;
			}
			if (RequestQueue[QueueHead].callback != NULL) {

				int before_last = (QueueHead + TM_MAX_QUEUE - 1) % TM_MAX_QUEUE;
				assert (RequestQueue[before_last].callback == NULL);
				*roadmap_tile_status_get (RequestQueue[before_last].tile_index) &= ~ROADMAP_TILE_STATUS_FLAG_QUEUED;
				RequestQueue[before_last] = RequestQueue[QueueHead];
			}
			QueueSize--;
		}

		for (slot = QueueHead, pos = 1; pos <= QueueSize; pos++) {
			test_slot = (QueueHead + pos) % TM_MAX_QUEUE;
			if (RequestQueue[test_slot].priority <= priority) break;
			RequestQueue[slot] = RequestQueue[test_slot];
			slot = test_slot;
		}

		if (QueueSize < TM_MAX_QUEUE) {
			QueueSize++;
		}
	} else {
		if (QueueSize >= TM_MAX_QUEUE) {
			roadmap_log (ROADMAP_INFO, "Tile request queue is full");
			return;
		}
		slot = (QueueHead + QueueSize) % TM_MAX_QUEUE;
		if (QueueSize < TM_MAX_QUEUE) {
			QueueSize++;
		}
	}

	RequestQueue[slot].tile_index = index;
	RequestQueue[slot].callback = on_loaded;
	RequestQueue[slot].priority = priority;
	roadmap_log (ROADMAP_DEBUG, "Queued tile %d at slot %d (position %d) with priority %d Status:%d",
						index, slot, (slot + 1 + TM_MAX_QUEUE - QueueHead) % TM_MAX_QUEUE, priority, Status);
}

void roadmap_tile_request (int index, int priority, int force_update, RoadMapCallback on_loaded) {

	int *tile_status = roadmap_tile_status_get (index);
   
   if (!tile_status)
      return;

	if ((*tile_status) & (ROADMAP_TILE_STATUS_FLAG_ACTIVE | ROADMAP_TILE_STATUS_FLAG_UPTODATE)) {
		return;
	}

	if (((*tile_status) & ROADMAP_TILE_STATUS_FLAG_QUEUED) &&
		 ((*tile_status) & ROADMAP_TILE_STATUS_MASK_PRIORITY) > priority) {
		return;
	}

	if (!force_update) {
		if ((*tile_status) & ROADMAP_TILE_STATUS_FLAG_UNFORCE) return;

		init_loading_session ();
		if ((!ActiveLoadingSession) &&
		 	 roadmap_square_timestamp (index) > 0) {

			*tile_status |= ROADMAP_TILE_STATUS_FLAG_UNFORCE;
			roadmap_log (ROADMAP_DEBUG, "Tile %d is not forced and already has a version - not requesting", index);
			return;
		}
	}

	queue_tile (index, priority, on_loaded);
	*tile_status = ((*tile_status) & ~ROADMAP_TILE_STATUS_MASK_PRIORITY) | ROADMAP_TILE_STATUS_FLAG_QUEUED | priority;

	init_connections ();
	if (Status == stat_First) {

		Status = stat_Waiting;
		NextLoginCallback = Realtime_NotifyOnLogin (roadmap_tile_manager_login_cb);
	}

	if (Status == stat_Active) {
		load_next_tile ();
	}

#ifdef J2ME
	start_network();
#endif
}

RoadMapTileCallback roadmap_tile_register_callback (RoadMapTileCallback cb) {

	RoadMapTileCallback prev = TileCallback;
	TileCallback = cb;
	return prev;
}

void roadmap_tile_reset_session (void) {
	init_loading_session ();
	roadmap_config_set_integer (&LastLoadingSessionCfg, (int)time (NULL));
	roadmap_config_save (0);
	ActiveLoadingSession = 0;
}


/*
 * Warning progress callback
 */
static BOOL tile_load_progress_warn( char* warn_string )
{
   BOOL res = FALSE;
   if ( TilesRefreshTotalCount > 0 )
   {
      int percentage = ( TilesRefreshProgressCount * 100 ) / TilesRefreshTotalCount;
      snprintf ( warn_string, ROADMAP_WARNING_MAX_LEN, "%s: %d%%%%", roadmap_lang_get ( "Refreshing map tiles" ),
               percentage );
      res = TRUE;
   }
   else
   {
      roadmap_warning_unregister( tile_load_progress_warn );
      res = FALSE;
   }

   return res;
}

/*
 * Callback executed upon subsequent tile download
 */
INLINE_DEC void tile_refresh_cb( int tile_id )
{
   if ( TilesRefreshTotalCount < 0 )
      return;

   if ( ++TilesRefreshProgressCount >= TilesRefreshTotalCount )
   {
      TilesRefreshTotalCount = -1;
      TilesRefreshProgressCount = 0;
      roadmap_screen_refresh();
   }

   roadmap_log( ROADMAP_DEBUG, "Updated %d tiles of %d", TilesRefreshProgressCount, TilesRefreshTotalCount );
}


/*
 * Tiles refresh procedure - executed by timer
 */
static void refresh_all_tiles( void )
{
   const char * wzm_file;
   int fips = roadmap_locator_active();

   roadmap_main_remove_periodic( refresh_all_tiles );
   /*
    * Removing the old tiles from disk
    */
   navigate_main_stop_navigation();
   roadmap_locator_close (fips);
   wzm_file = roadmap_map_download_build_file_name( fips );
   roadmap_file_remove( wzm_file, NULL );

   roadmap_tile_remove_all( fips );
   ssd_progress_msg_dialog_hide();
   roadmap_screen_refresh();
   
   if ( roadmap_locator_activate( fips ) != ROADMAP_US_OK )
      roadmap_log( ROADMAP_ERROR, "Problem activating locator" );

   /*
    * Requesting the tiles
    */
   TilesRefreshProgressCount = 0;
   TilesRefreshTotalCount = -1;
   roadmap_warning_register( tile_load_progress_warn, "refreshmap" );

   TilesRefreshTotalCount = roadmap_square_refresh( fips, TM_MAX_QUEUE, NULL );
   roadmap_log( ROADMAP_WARNING, "Going to update %d tiles", TilesRefreshTotalCount );
}
/*
 * Tiles refresh interface
 */
void roadmap_tile_refresh_all( void )
{ 
   if ( TilesRefreshTotalCount >= 0 )
   {
      roadmap_log( ROADMAP_WARNING, "Previous 'refresh tiles' request still in progress." );
      return;
   }

   roadmap_log( ROADMAP_DEBUG, "Refreshing all the tiles" );
   ssd_progress_msg_dialog_show( roadmap_lang_get( "Removing old tiles..." ) );
   if ( !roadmap_screen_refresh() )
      roadmap_screen_redraw();

   roadmap_main_set_periodic( 100, refresh_all_tiles );

}
