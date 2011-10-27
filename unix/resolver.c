/*
 * resolver.c
 * Implements pthread based async resolver
 *
 * LICENSE:
 *   Copyright 2011, Waze Ltd      Alex Agranovich (AGA)
 *
 *   TODO:: Rename to dnsresolver ?
 *   TODO:  Rename domain entry to resolver entry ?
 *
 */
#include "roadmap.h"
#include "roadmap_hash.h"
#include "roadmap_main.h"
#include "resolver.h"
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>

//======================== Local defines =========================

#define RSLV_TABLE_SIZE             64          // Maximum size 255 (see messages related code )
#define RSLV_DOMAIN_NAME_MAX_LEN    256
#define RSLV_IP_ADDR_MAX_LEN        256
#define RSLV_REQUEST_MAX_NUM        16
#define RSLV_WATCHDOG_TIMEOUT       5000
#define RSLV_HANDLER_TIMEOUT        30000
#define RSLV_RETRY_COUNT            5
#define RSLV_RETRY_PERIOD           30000             // 30 sec

#define RSLV_MSG_RETRY_INDEX_SHIFT  8
#define RSLV_MSG_ENTRY_ID_MASK      0x00FF
#define RSLV_MSG_RETRY_INDEX_MASK   0xFF00

#define RSLV_MSG_RETRY_INDEX( msg ) ( ( msg & RSLV_MSG_RETRY_INDEX_MASK ) >> RSLV_MSG_RETRY_INDEX_SHIFT )
#define RSLV_MSG_BUILD( entry_id, retry_idx ) ( ( RSLV_MSG_RETRY_INDEX_MASK & (retry_idx<<RSLV_MSG_RETRY_INDEX_SHIFT) ) \
                                                | ( RSLV_MSG_ENTRY_ID_MASK & entry_id ) )
//======================== Local types ========================

typedef struct
{
   ResolverRequestCb cb;
   const void* context;
} ResolverRequest_t;
typedef ResolverRequest_t *ResolverRequest;

typedef struct
{
//   int busy __attribute__ ((aligned(32)));   // Ensure this variable is aligned. Necessary to ensure the read/write is atomic-wise (one cycle)
   int busy;
   char domain[RSLV_DOMAIN_NAME_MAX_LEN];    // Domain to resolve
   in_addr_t ip_addr;                        // Resolved address. Note: This is updated in resolver thread

   pthread_t         thread_id;
   time_t            start_time;
   int               retry_index  __attribute__ ((aligned(32)));    // Ensure this variable is atomic-wise (one cycle) read/write
                                                                    // Current retry number starting from 0
   int               request_count;
   ResolverRequest_t requests[RSLV_REQUEST_MAX_NUM];
} ResolverEntry_t;

typedef ResolverEntry_t* ResolverEntry;
#define RSLV_ENTRY_INITIALIZER         { 0, {0}, INADDR_NONE, 0, 0, 0, 0, {{NULL, NULL}} }

//======================== Globals ========================

static ResolverEntry_t  sgResolverTable[RSLV_TABLE_SIZE];
static RoadMapHash*    sgResolverHash = NULL;

//======================== Local Declarations ========================
static ResolverEntry _find_entry( const char* domain );
static void *_resolver( void* params );
static int _find_empty( void );
static void _start_resolver( const char* domain, ResolverRequestCb callback, const void* context );
static void _watchdog( void );
static void _reset_entry( ResolverEntry entry );
static void _reset_table( void );
static BOOL _restart_resolver( int entry_id );

/*
 ******************************************************************************
 */
void resolver_init( void )
{
   sgResolverHash = roadmap_hash_new( "RESOLVER TABLE", RSLV_TABLE_SIZE );
   roadmap_main_set_periodic( RSLV_WATCHDOG_TIMEOUT, _watchdog );
   _reset_table();
}

/*
 ******************************************************************************
 */
void resolver_shutdown( void )
{
   roadmap_hash_free( sgResolverHash );
}
/*
 ******************************************************************************
 */
void resolver_handler( int msg )
{
   int i;
   int entry_id = msg & RSLV_MSG_ENTRY_ID_MASK;
   int retry_idx = RSLV_MSG_RETRY_INDEX( msg );
   ResolverEntry entry = &sgResolverTable[entry_id];
   ResolverRequestCb cb;
   const void *ctx;
   struct in_addr addr = {entry->ip_addr};
   time_t timeout = time( NULL ) - entry->start_time;

   roadmap_log( ROADMAP_INFO, "Resolver handler is called for entry %d. Domain '%s' is resolved to %s (%s) within: %d msec.\n" \
         "Current retry index: %d, reported retry index: %d",
         entry_id, entry->domain, inet_ntoa( addr ),
         entry->ip_addr == INADDR_NONE ? "Failure" : "Success",
         timeout, entry->retry_index, retry_idx );

   if ( retry_idx != entry->retry_index )
   {
      roadmap_log( ROADMAP_WARNING, "Mismatch in retry indexes (%d, %d). This handling message is obsolete - dismissing",
            entry->retry_index, retry_idx );
      return;
   }

   // If any retries remain - give a chance
   if ( entry->ip_addr == INADDR_NONE ) //Failure
   {
      if ( entry->retry_index < RSLV_RETRY_COUNT )
      {
         // AGA TODO:: Apply retry after timeout ?
         // Now the intention is to apply retry after timeout
         // Check can be added for the time passed from the last handler thread start
         if ( _restart_resolver( entry_id ) )
            return;
      }
      else
      {
         roadmap_log( ROADMAP_ERROR, "Failure in resolving domain: %s. No more retries - giving up",
                        entry->domain );
      }
   }

   for ( i = 0; i < entry->request_count; ++i )
   {
      cb = entry->requests[i].cb;
      ctx = entry->requests[i].context;

      if ( cb )
         cb( ctx, entry->ip_addr );
   }

   if ( entry->ip_addr == INADDR_NONE ) // Failure
   {
      _reset_entry( entry );
   }
   else  // Add to hash
   {
      entry->busy = 0;  // Not busy any more
      entry->request_count = 0;
      roadmap_hash_add( sgResolverHash, roadmap_hash_string( entry->domain ), entry_id );
   }

}

/*
 ******************************************************************************
 */
in_addr_t resolver_find( const char* domain )
{
   in_addr_t ip_addr = INADDR_NONE;
   ResolverEntry entry = NULL;
   entry = _find_entry( domain );

   if ( entry && !entry->busy )
   {
      ip_addr = entry->ip_addr;
   }

   return ip_addr;
}
/*
 ******************************************************************************
 */
in_addr_t resolver_request( const char* domain, ResolverRequestCb callback, const void* context )
{
   in_addr_t ip_addr = INADDR_NONE;
   ResolverEntry entry = NULL;

   entry = _find_entry( domain );

   if ( entry )
   {
      if ( entry->busy > 0 ) // In process
      {
         if ( entry->request_count < RSLV_REQUEST_MAX_NUM )
         {
            ResolverRequest request = &entry->requests[entry->request_count];
            request->cb = callback;
            request->context = context;
            entry->request_count++;
         }
         else // TODO:: Add error notification
         {
            roadmap_log( ROADMAP_ERROR, "Too many requests for the domain %s resolving", domain );
         }
      }
      else
      {
         ip_addr = entry->ip_addr;
      }
   }
   else
   {
      _start_resolver( domain, callback, context );
   }

   return ip_addr;
}
/*
 ******************************************************************************
 */
static ResolverEntry _find_entry( const char* domain )
{
   ResolverEntry entry = NULL;
   int i, hash;

   if ( domain == NULL )
      return NULL;

   hash = roadmap_hash_string ( domain );

   i = roadmap_hash_get_first ( sgResolverHash, hash );
   while ( i >= 0 )
   {
      if ( !strcmp( domain, sgResolverTable[i].domain ) )
      {
         entry = &sgResolverTable[i];
         break;
      }
      i = roadmap_hash_get_next ( sgResolverHash, i );
   }

   return entry;
}
/*
 ******************************************************************************
 * Android workaround due to absence of pthread_cancel
 */

static void _resolver_exit_handler(int sig)
{
   pthread_exit(0);
}
/*
 ******************************************************************************
 */
static void *_resolver( void* params )
{
   int entry_id = (int) params;
   ResolverEntry entry = &sgResolverTable[entry_id];
   struct hostent *host = NULL;
   in_addr_t ip_addr = INADDR_NONE;
   struct sigaction actions;
   int retry_idx;
   int msg;

   // This is very important to save the retry_index here. It's used to match thread and resolving request
   // For example If the thread will stuck on gethostbyname
   // But afterwards will send the message just before being killed due to timeout, the retry_idx in the sgResolverTable[entry_id]
   // will be already different.
   retry_idx = entry->retry_index;

   // AGA DEBUG
//   roadmap_log( ROADMAP_WARNING, "Started resolver for: %s. Retry: %d. Thread id: %ld",
//         entry->domain, entry->retry_index, pthread_self() );
/*
 * AGA Comment. No pthread_cancel in android - using _kill ( SIGTERM )
 * instead
 */

//   pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL );
//   pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );

   memset( &actions, 0, sizeof(actions) );
   sigemptyset( &actions.sa_mask );
   actions.sa_flags = 0;
   actions.sa_handler = _resolver_exit_handler;
   sigaction( SIGTERM, &actions, NULL );

   if ( isdigit( entry->domain[0] ) )
   {
      ip_addr = inet_addr( entry->domain );
   }
   else
   {
      struct in_addr addr;
      host = gethostbyname( entry->domain );

      if ( host )
      {
         memcpy ( &addr , host->h_addr, host->h_length );
         ip_addr = addr.s_addr;
      }
   }

   // Cancellation point !!!!
   if ( entry->retry_index != retry_idx )
   {
      return NULL;
   }

   entry->ip_addr = ip_addr;

   // Notify the main thread on completion
   msg = RSLV_MSG_BUILD( entry_id, retry_idx );
   roadmap_main_post_resolver_result( msg );

   return NULL;
}

/*
 ******************************************************************************
 */
static void _start_resolver( const char* domain, ResolverRequestCb callback, const void* context )
{
   pthread_t thread_id;
   int res;
   int entry_id = _find_empty();
   ResolverEntry entry = &sgResolverTable[entry_id];
   if ( entry_id >= 0 )
   {

      // Post the request
      entry->busy = 1;
      entry->start_time = time( NULL );
      entry->request_count = 1;
      entry->requests[0].cb = callback;
      entry->requests[0].context = context;
      strncpy_safe( entry->domain, domain, sizeof( entry->domain ) );
      res = pthread_create( &thread_id, NULL, _resolver, (void*) entry_id );
      entry->thread_id = thread_id;

      roadmap_log( ROADMAP_INFO, "Started resolver thread %ld with result %d. Domain '%s'. Callback: 0x%x",
            entry->thread_id, res, domain, callback );

      if ( res < 0 )
      {
         _reset_entry( entry );
         roadmap_log( ROADMAP_ERROR, "Error starting thread. Error: %d ( %s )", errno, strerror( errno ) );
      }
   }
   else
   {
      roadmap_log( ROADMAP_ERROR, "Cannot find empty entry for the new domain: %s", domain );
   }
}

/*
 ******************************************************************************
 */
static BOOL _restart_resolver( int entry_id )
{
   pthread_t thread_id;
   int res;
   ResolverEntry entry = &sgResolverTable[entry_id];
   // Post the request
   entry->busy = 1;
   entry->start_time = time( NULL );
   entry->retry_index++;
   entry->ip_addr = INADDR_NONE;
   res = pthread_create( &thread_id, NULL, _resolver, (void*) entry_id );
   entry->thread_id = thread_id;

   roadmap_log( ROADMAP_INFO, "Retry: %d. Restarted resolver thread %d with result %d. Domain '%s'.",
         entry->retry_index, entry->thread_id, res, entry->domain );

   if ( res != 0 )
   {
      _reset_entry( entry );
      roadmap_log( ROADMAP_ERROR, "Error restarting thread. Error: %d ( %s )", errno, strerror( errno ) );
   }
   return ( res == 0 );
}


/*
 ******************************************************************************
 */
static int _find_empty( void )
{
   int i = -1;
   ResolverEntry entry = NULL;

   for ( i = 0; i < RSLV_TABLE_SIZE; ++i )
   {
      entry = &sgResolverTable[i];
      if ( !entry->busy && !entry->domain[0] )
         break;
   }
   return i;
}
/*
 ******************************************************************************
 */
static void _reset_entry( ResolverEntry entry )
{
   if ( entry )
   {
      entry->busy = 0;
      entry->domain[0] = 0;
      entry->ip_addr = INADDR_NONE;
      entry->request_count = 0;
      entry->retry_index = 0;
   }
}
/*
 ******************************************************************************
 */
static void _watchdog( void )
{
   int i;
   ResolverEntry entry;
   time_t timeout;
   int res;
   int msg;

   for ( i = 0; i < RSLV_TABLE_SIZE; ++i )
   {
      entry = &sgResolverTable[i];
      if ( entry->busy == 0 ) // Not busy go to next
         continue;

      timeout = 1000*( time( NULL ) - entry->start_time );

      if ( timeout < RSLV_HANDLER_TIMEOUT ) // Still has time to work
         continue;

      roadmap_log( ROADMAP_WARNING, "Timeout passed in resolving domain '%s'. Cancelling thread: %d",
            entry->domain, entry->thread_id );

//      pthread_cancel( entry->thread_id );
      if ( ( res = pthread_kill( entry->thread_id, SIGTERM ) ) != 0 )
      {
         roadmap_log( ROADMAP_ERROR, "Error cancelling resolver thread %d, error = %d (%s)",
               entry->thread_id, res, strerror( res ) );
      }
      pthread_join( entry->thread_id, NULL );  // Wait for the thread to completely terminate

      msg = RSLV_MSG_BUILD( i, entry->retry_index );
      resolver_handler( msg );
   }
}
/*
 ******************************************************************************
 */
static void _reset_table( void )
{
   int i;

   for ( i = 0; i < RSLV_TABLE_SIZE; ++i )
   {
      _reset_entry( &sgResolverTable[i] );
   }
}
