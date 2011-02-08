/* roadmap_social_image.c - Manages social network images
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

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_config.h"
#include "roadmap_screen.h"
#include "roadmap_res.h"
#include "roadmap_social_image.h"
#include "roadmap_httpcopy_async.h"
#include "Realtime/Realtime.h"
#include "roadmap_jpeg.h"
#include "ssd/ssd_bitmap.h"

#define SOCIAL_IMAGE_CACHE_SIZE 50

typedef struct
{
   char *name;
   char* data;
   size_t data_size;
   SocialImageDownloadCallback download_cb;
   void* context_cb;
} ImageDownloadContext;

typedef struct image_slot {
   char *name;
   RoadMapImage image;
}image_slot;

static int  download_size_callback( void *context_cb, size_t size );
static void download_progress_callback( void *context_cb, char *data, size_t size );
static void download_error_callback( void *context_cb, int connection_failure, const char *format, ... );
static void download_done_callback( void *context_cb,char *last_modified );

static RoadMapHttpAsyncCallbacks gHttpAsyncCallbacks =
{
   download_size_callback,
   download_progress_callback,
   download_error_callback,
   download_done_callback
};

static RoadMapConfigDescriptor CfgSocialImageUrlPrefix =
                           ROADMAP_CONFIG_ITEM( SOCIAL_IMAGE_CONFIG_TAB, SOCIAL_IMAGE_CONFIG_URL_PREFIX );

image_slot  gImageDownloadCache[SOCIAL_IMAGE_CACHE_SIZE];
static int  gImageDownloadCacheCurIndex = 0;



///////////////////////////////////////////////////////////////////////////////////////////
static void social_image_download_update_bitmap_cb ( void* context, int status, RoadMapImage image ){
   SsdWidget facebook_image;
   if (status == 0){
      facebook_image = (SsdWidget)context;
      ssd_bitmap_image_update(facebook_image,image);
      roadmap_screen_refresh();
   }
}
///////////////////////////////////////////////////////////////////////////////////////////////////
static void cache_add( const char* name, RoadMapImage image )
{

   if ( gImageDownloadCache[gImageDownloadCacheCurIndex].name != NULL )
   {
      free( gImageDownloadCache[gImageDownloadCacheCurIndex].name );
   }

   gImageDownloadCache[gImageDownloadCacheCurIndex].name = strdup( name );
   gImageDownloadCache[gImageDownloadCacheCurIndex].image = image;
   gImageDownloadCacheCurIndex = ( gImageDownloadCacheCurIndex + 1 ) % SOCIAL_IMAGE_CACHE_SIZE;

}

///////////////////////////////////////////////////////////////////////////////////////////////////
static void cache_clear( void )
{
   int i;
   for ( i = 0; i < SOCIAL_IMAGE_CACHE_SIZE; ++i )
   {
      if ( gImageDownloadCache[i].name != NULL )
         free( gImageDownloadCache[i].name );
      gImageDownloadCache[i].name = NULL;
   }

}

///////////////////////////////////////////////////////////////////////////////////////////////////
static RoadMapImage cache_find(const char* name){
   int i;

   for ( i = 0; i < SOCIAL_IMAGE_CACHE_SIZE; ++i )
   {
       if ( (gImageDownloadCache[i].name != NULL) && (!strcmp(gImageDownloadCache[i].name, name)) )
          return gImageDownloadCache[i].image;
   }

   return NULL;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_social_image_initialize( void ){
   roadmap_config_declare( "preferences", &CfgSocialImageUrlPrefix, "", NULL );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_social_image_terminate( void ){
   cache_clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
static int  download_size_callback( void *context_cb, size_t size )
{
   ImageDownloadContext* context = (ImageDownloadContext*) context_cb;

   // Allocate data storage
   if ( size > 0 )
   {
      context->data = malloc( size );
      context->data_size = 0;
   }

   return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
static void download_progress_callback( void *context_cb, char *data, size_t size )
{
   ImageDownloadContext* context = (ImageDownloadContext*) context_cb;

   // Store data
   if ( context->data )
   {
      memcpy( context->data + context->data_size, data, size );
      context->data_size += size;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
static void download_error_callback( void *context_cb, int connection_failure, const char *format, ... )
{
   ImageDownloadContext* context = (ImageDownloadContext*) context_cb;
   va_list ap;
   char err_string[1024];

   // Load the arguments
   va_start( ap, format );
   vsnprintf( err_string, 1024, format, ap );
   va_end( ap );

   // Log it...
   roadmap_log( ROADMAP_WARNING, err_string );

   // Free allocated space
   if ( context->data )
   {
      free( context->data );
      context->data = NULL;
   }
   free( context->name );

   if (context->download_cb)
      context->download_cb( context->context_cb, -1, NULL );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
static void download_done_callback( void *context_cb, char *last_modified )
{
   ImageDownloadContext* context = (ImageDownloadContext*) context_cb;
   RoadMapImage image;

   image = roadmap_jpeg_from_buff( (unsigned char *)context->data, context->data_size);

   if (context->download_cb)
      context->download_cb( context->context_cb, 0, image );

   // Add file to cache
   cache_add( context->name, image );

   // Deallocate the download context
   free( context->data );
   free( context->name );
   free( context );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
static char* get_download_url(int id_type, int id, int id2, int size)
{
   char* url;
   const char* url_prefix;
   char id_type_str[20];
   char type[20];

    url_prefix = roadmap_config_get ( &CfgSocialImageUrlPrefix );

    url = malloc( strlen( url_prefix ) + 200 );

    roadmap_check_allocated( url );


    if (id_type == SOCIAL_IMAGE_ID_TYPE_USER)
        strcpy(id_type_str,"session");
    else if (id_type == SOCIAL_IMAGE_ID_TYPE_ALERT)
       strcpy(id_type_str,"alert");

    switch (size){
       case SOCIAL_IMAGE_SIZE_SQUARE:
          if (roadmap_screen_get_screen_scale() == 200)
             strcpy (type, "square100");
          else if (roadmap_screen_is_hd_screen())
             strcpy(type, "square75");
          else
             strcpy(type, "square");
          break;
       case SOCIAL_IMAGE_SIZE_SMALL:
          strcpy(type, "small");
          break;
       case SOCIAL_IMAGE_SIZE_LARGE:
          strcpy(type, "large");
          break;
       default:
          snprintf(type,sizeof(type),"square%d",size);
    }

    if (id2 == -1)
       snprintf(url, strlen( url_prefix ) + 200, "%s/%s/%d/picture?type=%s&sessionid=%d&cookie=%s",
                url_prefix,id_type_str,id,type,
                Realtime_GetServerId(),
                Realtime_GetServerCookie());
    else
       snprintf(url, strlen( url_prefix ) + 200, "%s/%s/%d_%d/picture?type=%s&sessionid=%d&cookie=%s",
                url_prefix,id_type_str,id,id2,type,
                Realtime_GetServerId(),
                Realtime_GetServerCookie());

   return url;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_social_image_download( int source, int id_type, int id, int id2, int size, void* context_cb, SocialImageDownloadCallback download_cb ){
   ImageDownloadContext* context;
   char name[50];
   RoadMapImage cached_image;
   char *url;

   snprintf(name, sizeof(name), "image_%d_%d_%d_%d_%d", source, id_type, id, id2, size);

   cached_image = cache_find(name);
   if (cached_image){
      download_cb(context_cb, 0, cached_image );
      return TRUE;
   }

   url = get_download_url( id_type, id, id2, size );
   context = malloc( sizeof( ImageDownloadContext ) );
   context->context_cb = context_cb;
   context->download_cb = download_cb;
   context->data = NULL;
   context->name = strdup(name);
   roadmap_http_async_copy( &gHttpAsyncCallbacks, context, url ,0 );
   free( url );

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_social_image_download_update_bitmap( int source, int id_type, int id, int id2, int size, SsdWidget bitmap ){
   void * context = (void *)bitmap;
   return roadmap_social_image_download( source, id_type, id, id2, size, context, social_image_download_update_bitmap_cb );
}
