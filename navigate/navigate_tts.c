/* navigate_tts.c - Navigation module client of the Text To Speech (TTS) engine
 *
 * LICENSE:
 *   Copyright 2011, Waze Ltd      Alex Agranovich (AGA)
 *
 *
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
 * SYNOPSYS:
 *
 *   See navigate_tts.h
 *       tts.h
 *
 */

#include <string.h>
#include "tts/tts.h"
#include "navigate_tts.h"
#include "roadmap_lang.h"
#include "roadmap_math.h"
#include "roadmap_warning.h"
#include "Realtime/Realtime.h"

//======================== Local defines ========================
#define NAV_TTS_CFG_CATEGORY                    ("TTS Navigate")
#define NAV_TTS_CFG_PREFIX_FILTER_LIST          ("Prefix Filter List")
#define NAV_TTS_CFG_LIST_DELIM                  ("|")
#define NAV_TTS_LOCAL_PARSING_ENABLED           (1)
#define NAV_TTS_CTX_INITIALIZER                 { FALSE, NULL, NULL, NULL, NULL, NULL };
#define NAV_TTS_TEXT_MAXLEN                     2048
#define NAV_TTS_FILTER_PREFIX_MAXNUM            16

#define NAV_TTS_LOGSTR( str )                   ( "Navigate TTS. " str )
#define NAV_TTS_SYLLABLE_PRONOUNCE_TIME         150            // Estimation in ms of time needed to pronounce one syllable
#if !defined(INLINE_DEC)
#define INLINE_DEC
#endif

//======================== Local types ========================

typedef struct
{
   BOOL                   tts_available;
   TtsPlayList            sound_list;
   TtsRequestCompletedCb  request_cb;
   void*                  request_cb_ctx;
   void*                  voice_prepare_ctx;
   char*                  voice_id;
} NavTtsContext;

typedef struct
{
   int                    prepared_count;
   int                    total_count;
   char                   voice_id[TTS_VOICE_MAXLEN];
 } NavTtsVoicePrepareCtx;
   //======================== Globals ========================

static const char* sgNavTtsCommon[] = {
                                       NAV_TTS_TEXT_ARRIVING_AT,
                                       NAV_TTS_TEXT_WITHIN,
                                       NAV_TTS_TEXT_METERS,
                                       NAV_TTS_TEXT_MILES,
                                       NAV_TTS_TEXT_TAKE_THE,
                                       NAV_TTS_TEXT_EXIT,
                                       NAV_TTS_TEXT_AND_THEN,
                                       NAV_TTS_TEXT_AT,
                                       NAV_TTS_TEXT_IN_200_METERS,
                                       NAV_TTS_TEXT_IN_400_METERS,
                                       NAV_TTS_TEXT_IN_500_METERS,
                                       NAV_TTS_TEXT_IN_800_METERS,
                                       NAV_TTS_TEXT_IN_1_KILOMETER,
                                       NAV_TTS_TEXT_IN_1p5_KILOMETERS,
                                       NAV_TTS_TEXT_IN_0p1_MILE,
                                       NAV_TTS_TEXT_IN_QUARTER_OF_MILE,
                                       NAV_TTS_TEXT_IN_0p3_MILE,
                                       NAV_TTS_TEXT_IN_HALF_MILE,
                                       NAV_TTS_TEXT_IN_0p6_MILE,
                                       NAV_TTS_TEXT_IN_1_MILE,
                                   };

static const char* sgNavTtsExits[] = {
                                       NAV_TTS_TEXT_FIRST_EXIT,
                                       NAV_TTS_TEXT_SECOND_EXIT,
                                       NAV_TTS_TEXT_THIRD_EXIT,
                                       NAV_TTS_TEXT_FOURTH_EXIT,
                                       NAV_TTS_TEXT_FIFTH_EXIT,
                                       NAV_TTS_TEXT_SIXTH_EXIT,
                                       NAV_TTS_TEXT_SEVENTH_EXIT
                                     };


static NavTtsContext sgCtx = NAV_TTS_CTX_INITIALIZER;      // Navigation TTS related context

static RoadMapCallback sgOnLoginNextCallback = NULL;

static RoadMapConfigDescriptor sgNavTtsCfgPrefixList =
      ROADMAP_CONFIG_ITEM( NAV_TTS_CFG_CATEGORY, NAV_TTS_CFG_PREFIX_FILTER_LIST );

static const char* sgFilterPrefixList[NAV_TTS_FILTER_PREFIX_MAXNUM] = {NULL};
static int sgFilterPrefixCount = 0;


//======================== Local Declarations ========================

static void _voice_prepare_cb( const void* user_context, int res_status, const char* text );
static void prepare_preload_list( const char* preload_list[], int list_size, int apply_lang );
static void playlist_add_atstreet( const NavigateSegment *prev_segment, const NavigateSegment *segment );
static void initialize_on_login( void );
static void _on_voice_changed( const char* voice_id, BOOL force_recommit );
static const char* _parse_nav_text( const char* text );
static BOOL _voice_prepare_warning_fn ( char* dest_string );
INLINE_DEC void _add_playlist( const char* text, BOOL failure_set_unavailable );
static const char* _get_street_name( const NavigateSegment *prev_segment, const NavigateSegment *segment );
static const char* _get_full_instruction( const NavTtsState* state );
static NavSmallValue _get_state_instruction( const NavTtsState* state );
static int _prepare_nav_voice( const char* voice_id );
static const char* _get_destination_name( const char* street, const char* street_num );
INLINE_DEC _format_street_text( const char* street_name, BOOL add_at, char buf_out[], int buf_size );

void navigate_tts_initialize( void )
{
// TODO: Reduce the default. Assign ""
   roadmap_config_declare( "preferences", &sgNavTtsCfgPrefixList, "to ", NULL );
   sgOnLoginNextCallback = Realtime_NotifyOnLogin ( initialize_on_login );
}

static void initialize_on_login( void )
{
   // More naturally to put this in navigate main.
   // Put here to minimize code changing
   navigate_main_override_nav_settings();

   sgFilterPrefixCount = roadmap_config_get_list( &sgNavTtsCfgPrefixList, NAV_TTS_CFG_LIST_DELIM,
         sgFilterPrefixList, NAV_TTS_FILTER_PREFIX_MAXNUM );

//   navigate_tts_preload( 0 );
   if ( tts_feature_enabled() )
   {
      _prepare_nav_voice( NULL );
      tts_register_on_voice_changed( ( TtsOnVoiceChangedCb ) _on_voice_changed );
   }

   if ( sgOnLoginNextCallback )
      sgOnLoginNextCallback();
}
/*
 ******************************************************************************
 */
int navigate_tts_preload( int apply_lang )
{
   int i;
   const char* text;
   int nav_common_size = sizeof( sgNavTtsCommon )/sizeof( *sgNavTtsCommon );
   int nav_exits_size = sizeof( sgNavTtsExits )/sizeof( *sgNavTtsExits );
   int nav_instr_size = 0;
   /*
    * Common list
    */
   prepare_preload_list( sgNavTtsCommon, nav_common_size, apply_lang );
   /*
    * Exits list
    */
   prepare_preload_list( sgNavTtsExits, nav_exits_size, apply_lang );

   /*
    * Instructions list
    */
   for ( i = 0; i < NAVIGATE_INSTRUCTIONS_COUNT; ++i )
   {
      text = navigate_tts_instruction_text( i );
      if ( text )
      {
         if ( apply_lang )
            text = roadmap_lang_get( text );
         tts_request_ex( text, TTS_TEXT_TYPE_DEFAULT, sgCtx.request_cb, sgCtx.request_cb_ctx, TTS_FLAG_RETRY|TTS_FLAG_RETRY_CALLBACK );
         nav_instr_size++;
      }
   }

   // Commit the tts request
   tts_commit();
   return ( nav_common_size + nav_exits_size + nav_instr_size );
}


/*
 ******************************************************************************
 */
BOOL navigate_tts_prepare_street( const NavigateSegment* segment )
{
   BOOL result = FALSE;
   const char* street_name_next;
   const char* street_name;

   street_name_next = _get_street_name( segment, NULL );
   street_name = _get_street_name( NULL, segment );

   if ( tts_enabled() && ( street_name || street_name_next ) )
   {
      char street_text[NAV_TTS_TEXT_MAXLEN];

      if ( street_name )
      {
         _format_street_text( street_name, TRUE, street_text, sizeof( street_text ) );

         tts_request_ex( street_text, TTS_TEXT_TYPE_STREET,
                              sgCtx.request_cb, sgCtx.request_cb_ctx, TTS_FLAG_RETRY|TTS_FLAG_RETRY_CALLBACK );
      }
      if ( street_name_next )
      {
         _format_street_text( street_name_next, TRUE, street_text, sizeof( street_text ) );

         tts_request_ex( street_text, TTS_TEXT_TYPE_STREET,
                              sgCtx.request_cb, sgCtx.request_cb_ctx, TTS_FLAG_RETRY|TTS_FLAG_RETRY_CALLBACK );
      }

      result = TRUE;
   }

   return result;
}

BOOL navigate_tts_prepare_arrive( const char* street, const char* street_num )
{
   BOOL result = FALSE;

   if ( tts_enabled() && street && street[0] )
   {
      const char *dest = _get_destination_name( street, street_num );

      if ( dest && dest[0] )
      {
         tts_request_ex( _parse_nav_text( dest ), TTS_TEXT_TYPE_STREET,
                                sgCtx.request_cb, sgCtx.request_cb_ctx, TTS_FLAG_RETRY|TTS_FLAG_RETRY_CALLBACK );
      }

      result = TRUE;
   }

   return result;
}



/*
 ******************************************************************************
 */
const char* navigate_tts_instruction_text( NavSmallValue instruction )
{
   const char* text;

   switch ( instruction )
   {
         case TURN_LEFT:
            text = NAV_TTS_TEXT_TURN_LEFT;
            break;
         case ROUNDABOUT_LEFT:
            text = NAV_TTS_TEXT_ROUNDABOUT_LEFT;
            break;
         case KEEP_LEFT:
            text = NAV_TTS_TEXT_KEEP_LEFT;
            break;
         case TURN_RIGHT:
            text = NAV_TTS_TEXT_TURN_RIGHT;
            break;
         case ROUNDABOUT_RIGHT:
            text = NAV_TTS_TEXT_ROUNDABOUT_RIGHT;
            break;
         case KEEP_RIGHT:
            text = NAV_TTS_TEXT_KEEP_RIGHT;
            break;
         case EXIT_RIGHT:
            text = NAV_TTS_TEXT_EXIT_RIGHT;
            break;
         case EXIT_LEFT:
            text = NAV_TTS_TEXT_EXIT_LEFT;
            break;
         case APPROACHING_DESTINATION:
            text = NAV_TTS_TEXT_APPROACHING_DESTINATION;
            break;
         case CONTINUE:
            text = NAV_TTS_TEXT_CONTINUE;
            break;
         case ROUNDABOUT_STRAIGHT:
            text = NAV_TTS_TEXT_ROUNDABOUT_STRAIGHT;
            break;
         case ROUNDABOUT_ENTER:
            text = NAV_TTS_TEXT_ROUNDABOUT_ENTER;
            break;
         case PREPARE_TURN_LEFT:
            text = NAV_TTS_TEXT_PREPARE_TURN_LEFT;
            break;
         case PREPARE_TURN_RIGHT:
            text = NAV_TTS_TEXT_PREPARE_TURN_RIGHT;
            break;
         case PREPARE_EXIT_LEFT:
            text = NAV_TTS_TEXT_PREPARE_EXIT_LEFT;
            break;
         case PREPARE_EXIT_RIGHT:
            text = NAV_TTS_TEXT_PREPARE_EXIT_RIGHT;
            break;

         default:
            text = NULL;
            break;
      }
   return text;
}

/*
 ******************************************************************************
 */
BOOL navigate_tts_available( void )
{
   return sgCtx.tts_available;
}

/*
 ******************************************************************************
 */
void navigate_tts_prepare_context( void )
{
   roadmap_log( ROADMAP_INFO, NAV_TTS_LOGSTR( "Preparing navigation TTS context" ) );

   sgCtx.tts_available = tts_enabled();
   sgCtx.voice_id = strdup( tts_voice_id() );
   sgCtx.request_cb = NULL;
   sgCtx.request_cb_ctx = NULL;
   sgCtx.sound_list = NULL;
}

/*
 ******************************************************************************
 */
void navigate_tts_finish_route( void )
{
   roadmap_log( ROADMAP_INFO, NAV_TTS_LOGSTR( "Finishing route" ) );

   if ( sgCtx.voice_id )
      free( sgCtx.voice_id );
   sgCtx = (NavTtsContext) NAV_TTS_CTX_INITIALIZER;
}

/*
 ******************************************************************************
*/
BOOL navigate_tts_playlist_add_within( const char* distance_str )
{
   const char* units_str = NAV_TTS_TEXT_MILES;
   const char* tts_string = distance_str;

   if ( !sgCtx.tts_available )
      return FALSE;

   if ( !strcmp( roadmap_math_distance_unit(), "m" ) )
   {

      if ( !strcmp( distance_str, "200" ) || !strcmp( distance_str, "200meters" ))
         tts_string = NAV_TTS_TEXT_IN_200_METERS;
      if ( !strcmp( distance_str, "400" ) )
         tts_string = NAV_TTS_TEXT_IN_400_METERS;
      if ( !strcmp( distance_str, "500" ) )
         tts_string = NAV_TTS_TEXT_IN_500_METERS;
      if ( !strcmp( distance_str, "800" ) )
         tts_string = NAV_TTS_TEXT_IN_800_METERS;
      if ( !strcmp( distance_str, "1000" ) )
         tts_string = NAV_TTS_TEXT_IN_1_KILOMETER;
      if ( !strcmp( distance_str, "1500" ) )
         tts_string = NAV_TTS_TEXT_IN_1p5_KILOMETERS;

      units_str = NAV_TTS_TEXT_METERS;
   }
   if ( !strcmp(roadmap_math_distance_unit(), "ft" ) )
   {
      if ( !strcmp( distance_str, "200" ) )
         tts_string = NAV_TTS_TEXT_IN_0p1_MILE;
      if ( !strcmp( distance_str, "400" ) )
         tts_string = NAV_TTS_TEXT_IN_QUARTER_OF_MILE;
      if ( !strcmp( distance_str, "500" ) )
         tts_string = NAV_TTS_TEXT_IN_0p3_MILE;
      if ( !strcmp( distance_str, "800" ) )
         tts_string = NAV_TTS_TEXT_IN_HALF_MILE;
      if ( !strcmp( distance_str, "1000" ) )
         tts_string = NAV_TTS_TEXT_IN_0p6_MILE;
      if ( !strcmp( distance_str, "1500" ) )
         tts_string = NAV_TTS_TEXT_IN_1_MILE;

      units_str = NAV_TTS_TEXT_MILES;
   }

   _add_playlist( tts_string, TRUE );

   return sgCtx.tts_available;
}

/*
 ******************************************************************************
*/
BOOL navigate_tts_playlist_add_arrive( const NavTtsState* state )
{
   char instr_text[NAV_TTS_TEXT_MAXLEN] = {0};
   const char* text;

   if ( !sgCtx.tts_available )
      return FALSE;

   text = _get_destination_name( state->dest_street, state->dest_street_number );

   if ( text && text[0] && tts_text_available( text, sgCtx.voice_id ) )
   {
      _add_playlist( NAV_TTS_TEXT_ARRIVING_AT, FALSE );
      _add_playlist( text, FALSE );
   }
   else
   {
      text = navigate_tts_instruction_text( APPROACHING_DESTINATION );
      _add_playlist( text, FALSE ); // Already at the end - don't make it unavailable
   }



   return sgCtx.tts_available;
}

/*
 ******************************************************************************
*/
BOOL navigate_tts_playlist_add_instr( const NavTtsState *state )
{
   static int exits_count = sizeof( sgNavTtsExits ) / sizeof( *sgNavTtsExits );
   const char* text = NULL;
   const NavigateSegment *segment = state->segment;
   const NavigateSegment *next_segment = state->next_segment;
   NavSmallValue instruction;

   if ( !sgCtx.tts_available )
      return FALSE;

   /*
    * Add current instruction
    */
   instruction = _get_state_instruction( state );

   text = navigate_tts_instruction_text( instruction );

   if ( !text )
      return FALSE;

   _add_playlist( text, TRUE );

   if ( segment->instruction == ROUNDABOUT_ENTER )
   {
      int exit_no = segment->exit_no;
      if ( exit_no > 0 && exit_no <= exits_count )
      {
         text = sgNavTtsExits[exit_no-1];
         _add_playlist( text, TRUE );
      }
   }

   /*
    * Find street to add phrase "at ..."
    */
   if ( state->announce_state != NAV_ANNOUNCE_STATE_FIRST )
      playlist_add_atstreet( segment, next_segment );

   return sgCtx.tts_available;
}


/*
 ******************************************************************************
*/
int navigate_tts_announce_dist_factor( const NavTtsState* state, int speed_kph )
{
   const char *full_instr;
   int spaces_count = 0;
   int instr_len, i, syllable_count;
   int distance_factor;

   if ( !sgCtx.tts_available )
      return 0;

   full_instr = _get_full_instruction( state );

   if ( !full_instr )
      return 0;

   instr_len = strlen( full_instr );
   /* Count spaces */
   for ( i = 0; i < instr_len; ++i )
   {
      if ( full_instr[i] == ' ' )
         spaces_count++;
   }

   syllable_count = ( instr_len - spaces_count )/2 + spaces_count;

   /* Calculate the factor in meters */
   distance_factor = speed_kph * 1000 * syllable_count * NAV_TTS_SYLLABLE_PRONOUNCE_TIME / ( 3600 * 1000 /* ms */ );

   return distance_factor;
}


/*
 ******************************************************************************
*/
BOOL navigate_tts_playlist_add_andthen( NavSmallValue instruction, NavSmallValue exit_no, const NavTtsState* state )
{
   static int exits_count = sizeof( sgNavTtsExits ) / sizeof( *sgNavTtsExits );
   const char* text = NULL;
   const NavigateSegment *segment = state->segment;
   const NavigateSegment *next_segment = state->next_segment;

   if ( !sgCtx.tts_available )
      return FALSE;

    text = navigate_tts_instruction_text( instruction );

    if ( !text )
       return FALSE;

   _add_playlist( NAV_TTS_TEXT_AND_THEN, TRUE );
   _add_playlist( text, TRUE );

   if ( instruction == ROUNDABOUT_ENTER )
   {
      if ( exit_no > 0 && exit_no <= exits_count )
      {
         text = sgNavTtsExits[exit_no-1];
         _add_playlist( text, TRUE );
      }
   }
   /*
    * Find street to add phrase "at ..."
    */
   if ( state->announce_state != NAV_ANNOUNCE_STATE_FIRST )
      playlist_add_atstreet( segment, next_segment );

   return sgCtx.tts_available;
}

/*
 ******************************************************************************
*/
BOOL navigate_tts_playlist_add( const char* text )
{
   BOOL result = TRUE;

   if ( !sgCtx.tts_available )
      return FALSE;

   if ( !sgCtx.sound_list )
      sgCtx.sound_list = tts_playlist_create( sgCtx.voice_id );

   result = tts_playlist_add( sgCtx.sound_list, text );

   roadmap_log( ROADMAP_DEBUG, NAV_TTS_LOGSTR( "Adding text: %s to the TTS playlist. Voice: %s. Result: %d" ), text, sgCtx.voice_id, result );

   return result;
}

/*
 ******************************************************************************
*/
void navigate_tts_set_unavailable( void )
{
   /*
    * Free the playlist
    */
   if ( sgCtx.sound_list )
   {
      tts_playlist_free( sgCtx.sound_list );
      sgCtx.sound_list = NULL;
   }

   // Mark as unavailable
   sgCtx.tts_available = FALSE;
}

/*
 ******************************************************************************
*/
BOOL navigate_tts_playlist_play( void )
{
   BOOL result = FALSE;

   if ( !sgCtx.tts_available )
      return FALSE;

   if ( !sgCtx.sound_list )
   {
      roadmap_log( ROADMAP_WARNING, NAV_TTS_LOGSTR( "Unable to play. TTS playlist is unavailable!" ) );
      return FALSE;
   }

   result = tts_playlist_play( sgCtx.sound_list );
   sgCtx.sound_list = NULL;

   return result;
}


/*
 ******************************************************************************
 */
void navigate_tts_commit( void )
{

   if ( tts_enabled() )
   {
      tts_commit();
   }
}

/*
 ******************************************************************************
 * TODO:: Redesign and rewrite this !
 */
static void _voice_prepare_cb( const void* user_context, int res_status, const char* text )
{
   NavTtsVoicePrepareCtx* ctx = (NavTtsVoicePrepareCtx*) sgCtx.voice_prepare_ctx;

   if ( !ctx )
      return;

   if ( res_status & TTS_RES_STATUS_SUCCESS )
   {
      ctx->prepared_count++;
   }
   else
   {
      if ( res_status & TTS_RES_STATUS_RETRY_ON )
         roadmap_log( ROADMAP_WARNING, NAV_TTS_LOGSTR( "Error in tts request for text: %s. Waiting for retry result..." ), text );

      if ( res_status & TTS_RES_STATUS_RETRY_EXHAUSTED )
      {
         roadmap_log( ROADMAP_ERROR, NAV_TTS_LOGSTR( "Error in tts request for text: %s. Retries exhausted. Text will not be available" ), text );
         ctx->prepared_count++; // Just force warning to disappear
      }
   }


   if ( ctx->total_count && ( ctx->prepared_count == ctx->total_count ) )
   {
      roadmap_log( ROADMAP_WARNING, NAV_TTS_LOGSTR( "All the %d voices are received - changing navigation voice" ), ctx->total_count );

      if ( sgCtx.voice_id )
         free( sgCtx.voice_id );
      sgCtx.voice_id = strdup( ctx->voice_id );
      sgCtx.voice_prepare_ctx = NULL;
      sgCtx.tts_available = tts_enabled();
      roadmap_warning_unregister( _voice_prepare_warning_fn );
      free( ctx );
   }
}


/*
 ******************************************************************************
 * Prepares the text in the list
 * Auxiliary
 */
static void prepare_preload_list( const char* preload_list[], int list_size, int apply_lang )
{
   const char* text;
   int i;

   for ( i = 0; i < list_size; ++i )
   {
      if ( apply_lang )
         text = roadmap_lang_get( preload_list[i] );
      else
         text = preload_list[i];
      tts_request_ex( text, TTS_TEXT_TYPE_DEFAULT, sgCtx.request_cb, sgCtx.request_cb_ctx, TTS_FLAG_RETRY|TTS_FLAG_RETRY_CALLBACK );
   }
}


/*
 ******************************************************************************
 * Prepares the text of the street of the segment
 * Auxiliary
 */
static void playlist_add_atstreet( const NavigateSegment *prev_segment, const NavigateSegment *segment )
{
     const char* street_name;

     if ( !sgCtx.tts_available )
        return;

     street_name = _get_street_name( prev_segment, segment );

     if ( street_name )
     {
        char street_text[NAV_TTS_TEXT_MAXLEN];
        _format_street_text( street_name, TRUE, street_text, sizeof( street_text ) );

        if ( tts_text_available( street_text, sgCtx.voice_id )  )
        {
//           _add_playlist( NAV_TTS_TEXT_AT, TRUE );
           _add_playlist( street_text, FALSE );
        }
        else
        {
           roadmap_log( ROADMAP_WARNING, NAV_TTS_LOGSTR( "Unable to add street name '%s'. Not cached." ), street_name );
        }
     }
}
/*
 ******************************************************************************
 * Resubmits the texts with the new voice
 * TODO:: Redesign and rewrite this !!
 * Auxiliary
 */
static void _on_voice_changed( const char* voice_id, BOOL force_recommit )
{
   // TODO : Check apply_lang attribute
   if ( !voice_id )
      return;

   if ( sgCtx.voice_id )
   {
      if ( !strcmp( voice_id, sgCtx.voice_id ) )
      {
         if ( force_recommit )
         {
            navigate_tts_preload( 0 );
         }
         else
         {
            roadmap_log( ROADMAP_DEBUG, NAV_TTS_LOGSTR( "Voice is the same. Stop voice change monitoring..." ) );
            if ( sgCtx.voice_prepare_ctx )
            {
               free( sgCtx.voice_prepare_ctx );
               sgCtx.voice_prepare_ctx = NULL;
            }
            roadmap_warning_unregister( _voice_prepare_warning_fn );
            return;
         }
      }
   }
   else
   { // Not in navigation now
      sgCtx.voice_id = strdup( voice_id );
   }

   _prepare_nav_voice( voice_id );

   roadmap_log( ROADMAP_WARNING, NAV_TTS_LOGSTR( "Voice change request from %s to %s." ),
          sgCtx.voice_id, voice_id );

}

/*
 ******************************************************************************
 * Starts the process of preparing the navigation voice
 * Returns number of prepared texts or -1 if fails
 * Auxiliary
 */
static int _prepare_nav_voice( const char* voice_id )
{
   int preload_count, route_count;
   if ( sgCtx.voice_prepare_ctx )
   {
      roadmap_log( ROADMAP_WARNING, NAV_TTS_LOGSTR( "Previous voice prepare request is in process" ) );
      return -1;
   }
   // TODO:: Start progress
   roadmap_warning_register( _voice_prepare_warning_fn, "navigate_tts" );

   NavTtsVoicePrepareCtx* ctx = calloc( sizeof( NavTtsVoicePrepareCtx ), 1 );

   if ( !voice_id )
      voice_id = tts_voice_id();

   strncpy_safe( ctx->voice_id, voice_id, TTS_VOICE_MAXLEN );

   sgCtx.request_cb = _voice_prepare_cb;
   sgCtx.request_cb_ctx = ctx;
   sgCtx.voice_prepare_ctx = ctx;

   preload_count = navigate_tts_preload( 0 );
   route_count = navigate_main_tts_prepare_route();

   ctx->total_count = preload_count + route_count;

   /*
    * Rollback to the default callback.
    * TODO:: Check this. Not the best implementation :(
    */
   sgCtx.request_cb = NULL;
   sgCtx.request_cb_ctx = NULL;

   roadmap_log( ROADMAP_DEBUG, NAV_TTS_LOGSTR( "Posting the request for preparing %d texts" ),
         ctx->total_count );

   // If all the items in cache the total count in callback will be 0
   if ( ctx->prepared_count == ctx->total_count )
   {
      if ( sgCtx.voice_id )
         free( sgCtx.voice_id );
      sgCtx.voice_id = strdup( ctx->voice_id );
      sgCtx.voice_prepare_ctx = NULL;
      sgCtx.tts_available = tts_enabled();
      roadmap_warning_unregister( _voice_prepare_warning_fn );
      free( ctx );
   }


   navigate_tts_commit();

   return ctx->total_count;
}

/*
 ******************************************************************************
 * Parses the text. Returns the statically allocated char buffer.
 * TEMPORARY SOLUTION. Should be done on server side!!!
 * Auxiliary
 */
static const char* _parse_nav_text( const char* text )
{
   const char* ret_value = NULL;
   static char s_parsed_text[NAV_TTS_TEXT_MAXLEN];

   if ( NAV_TTS_LOCAL_PARSING_ENABLED )
   {
      int i;
      const char* prefix;

      if ( !text )
         return NULL;

      // Parse for the prefixes
      for ( i = 0; i < sgFilterPrefixCount; ++i )
      {
         prefix = sgFilterPrefixList[i];
         if ( prefix && !strncasecmp( text, prefix, strlen( prefix ) ) )
         {
            strncpy_safe( s_parsed_text, &text[strlen( prefix )], NAV_TTS_TEXT_MAXLEN );
            break;
         }
      }

      if ( i == sgFilterPrefixCount )  // No match
         strncpy_safe( s_parsed_text, text, NAV_TTS_TEXT_MAXLEN );

/*
      if ( strlen( text ) > NAV_TTS_TEXT_MAXLEN )
      {
         roadmap_log( ROADMAP_WARNING, NAV_TTS_LOGSTR( "Cannot parse text longer than %d. Requested text length: %d" ),
               NAV_TTS_TEXT_MAXLEN, strlen( text ) );
         return s_parsed_text;
      }

      int text_len = strlen( text );
      int remaining = NAV_TTS_TEXT_MAXLEN - text_len - 1; // Zero termination

      if ( text[text_len-1] == 'S' || text[text_len-1] == 'N' ||
            text[text_len-1] == 'E' || text[text_len-1] == 'W' )
      {
           if ( text[text_len-1] == 'S' && (unsigned) remaining >= ( strlen( NAV_TTS_TEXT_SOUTH ) - 1 ) )
           {
              strcpy( &s_parsed_text[text_len-1], NAV_TTS_TEXT_SOUTH );
           }
           if ( text[text_len-1] == 'E' && (unsigned) remaining >= ( strlen( NAV_TTS_TEXT_EAST ) - 1 ) )
           {
              strcpy( &s_parsed_text[text_len-1], NAV_TTS_TEXT_EAST );
           }
           if ( text[text_len-1] == 'N' && (unsigned) remaining >= ( strlen( NAV_TTS_TEXT_NORTH ) - 1 ) )
           {
              strcpy( &s_parsed_text[text_len-1], NAV_TTS_TEXT_NORTH );
           }
           if ( text[text_len-1] == 'W' && (unsigned) remaining >= ( strlen( NAV_TTS_TEXT_WEST ) - 1 ) )
           {
              strcpy( &s_parsed_text[text_len-1], NAV_TTS_TEXT_WEST );
           }
      }
      if ( text_len >= 4 && !strcasecmp( &text[text_len-4], " Ave" ) )
      {
         if ( (unsigned) remaining >= ( strlen( NAV_TTS_TEXT_AVENUE ) - 3 ) )
            strcpy( &s_parsed_text[text_len-3], NAV_TTS_TEXT_AVENUE );
      }
      if ( text_len >= 3 && !strcasecmp( &text[text_len-3], " Rd" ) )
      {
         if ( (unsigned) remaining >= ( strlen( NAV_TTS_TEXT_ROAD ) - 2 ) )
            strcpy( &s_parsed_text[text_len-2], NAV_TTS_TEXT_ROAD );
      }
      if ( text_len >= 3 && !strcasecmp( &text[text_len-3], " Ln" ) )
      {
         if ( (unsigned) remaining >= ( strlen( NAV_TTS_TEXT_LANE ) - 2 ) )
            strcpy( &s_parsed_text[text_len-2], NAV_TTS_TEXT_LANE );
      }
      if ( text_len >= 3 && !strcasecmp( &text[text_len-3], " st" ) )
      {
         if ( (unsigned) remaining >= ( strlen( NAV_TTS_TEXT_STREET ) - 2 ) )
            strcpy( &s_parsed_text[text_len-2], NAV_TTS_TEXT_STREET );
      }
      */
      ret_value = s_parsed_text;
   }
   else
   {
      ret_value = text;
   }

   return ret_value;
}

/*
 ******************************************************************************
 * Warning formatter
 * Auxiliary
 */
static BOOL _voice_prepare_warning_fn ( char* dest_string )
{
   NavTtsVoicePrepareCtx *ctx = (NavTtsVoicePrepareCtx*) sgCtx.voice_prepare_ctx;
   if ( ctx == NULL ){
      snprintf ( dest_string, ROADMAP_WARNING_MAX_LEN, " " );
      return FALSE;
   }

   snprintf( dest_string, ROADMAP_WARNING_MAX_LEN, "%s: %d%%%%",
         roadmap_lang_get ( "Preparing navigation voice" ), ctx->prepared_count * 100 / ( ctx->total_count + 1 ) );
   return TRUE;
}

/*
 ******************************************************************************
 * Adds to playlist. Sets the engine unavailable in case of failure if requested
 * Auxiliary
 */
INLINE_DEC void _add_playlist( const char* text, BOOL failure_set_unavailable )
{
   if ( !tts_text_available( text, sgCtx.voice_id ) && failure_set_unavailable )
   {
      roadmap_log( ROADMAP_WARNING, NAV_TTS_LOGSTR( "Set the navigate TTS engine as unavailable for text: %s" ), text );
      navigate_tts_set_unavailable();
   }
   else
   {
      navigate_tts_playlist_add( text );
   }
}

/*
 ******************************************************************************
 * Extracts street name from the segment.
 * Auxiliary
 */
static const char* _get_street_name( const NavigateSegment *prev_segment, const NavigateSegment *segment )
{
   const char* street_name = NULL;
   PluginStreetProperties properties;
   PluginLine segment_line;

   if ( prev_segment && ( prev_segment->dest_name != NULL ) )
   {
      street_name = prev_segment->dest_name;
   }
   else if ( segment )
   {
      navigate_main_get_plugin_line ( &segment_line, segment );
      roadmap_plugin_get_street_properties ( &segment_line, &properties, 0 );
      if ( properties.street && properties.street[0] )
      {
         street_name = properties.street;
      }
   }
   return street_name;
}
/*
 ******************************************************************************
 * Extracts street name from the segment.
 * Auxiliary
 */
static const char* _get_destination_name( const char* street, const char* street_num )
{
   static char tts_text[NAV_TTS_TEXT_MAXLEN];

   tts_text[0] = 0;

   if ( street && street[0] )
   {
      if ( street_num && street_num[0] )
      {
         strncat( tts_text, street_num, NAV_TTS_TEXT_MAXLEN - 2 );
         strcat( tts_text, " " );
      }

      strncat( tts_text, street, NAV_TTS_TEXT_MAXLEN - strlen( tts_text ) - 1 );
      tts_text[NAV_TTS_TEXT_MAXLEN-1] = 0;
   }

   return tts_text;
}
/*
 ******************************************************************************
 * Constructs the full navigation instruction text string containing the
 * instruction and street name
 * Auxiliary
 */
static const char* _get_full_instruction ( const NavTtsState* state )
{
   static int exits_count = sizeof( sgNavTtsExits ) / sizeof( *sgNavTtsExits );
   static char full_instr[NAV_TTS_TEXT_MAXLEN];
   int len;
   const char* text;
   const char* space = " ";
   const NavigateSegment* segment = state->segment;
   const NavigateSegment* next_segment = state->next_segment;
   NavSmallValue instruction = _get_state_instruction( state );
   const char* instruction_text = navigate_tts_instruction_text( instruction );

   if ( !instruction_text )
      return NULL;

   /*
    * Add current instruction
    */
   strncpy_safe( full_instr, instruction_text, NAV_TTS_TEXT_MAXLEN );

   if ( instruction == ROUNDABOUT_ENTER )
   {
      int exit_no = segment->exit_no;
      if ( exit_no > 0 && exit_no <= exits_count )
      {
         /* Space */
         len = strlen( full_instr );
         strncpy_safe( full_instr + len, space, NAV_TTS_TEXT_MAXLEN - len );

         /* Exit text */
         text = sgNavTtsExits[exit_no-1];
         len = strlen( full_instr );
         strncpy_safe( full_instr + len, text, NAV_TTS_TEXT_MAXLEN - len );

      }
   }

   text = _get_street_name( segment, next_segment );
   if ( text )
   {
      /* Space */
      len = strlen( full_instr );
      strncpy_safe( full_instr + len, space, NAV_TTS_TEXT_MAXLEN - len );

      /* At */
      len = strlen( full_instr );
      strncpy_safe( full_instr + len, NAV_TTS_TEXT_AT, NAV_TTS_TEXT_MAXLEN - len );

      /* Space */
      len = strlen( full_instr );
      strncpy_safe( full_instr + len, space, NAV_TTS_TEXT_MAXLEN - len );

      // Street
      len = strlen( full_instr );
      strncpy_safe( full_instr + len, _parse_nav_text( text ), NAV_TTS_TEXT_MAXLEN - len );
   }

   return full_instr;
}

/*
 ******************************************************************************
 * Returns the instruction code applying the navigation state logic
 * instruction and street name
 * Auxiliary
 */
static NavSmallValue _get_state_instruction( const NavTtsState* state )
{
   const NavigateSegment *segment = state->segment;
   NavSmallValue instruction = segment->instruction;

   if ( state->announce_state == NAV_ANNOUNCE_STATE_FIRST )
   {
      switch ( instruction )
      {
         case TURN_LEFT:
            instruction = PREPARE_TURN_LEFT;
            break;
         case TURN_RIGHT:
            instruction = PREPARE_TURN_RIGHT;
            break;
         case EXIT_LEFT:
            instruction = PREPARE_EXIT_LEFT;
            break;
         case EXIT_RIGHT:
            instruction = PREPARE_EXIT_RIGHT;
            break;
      }
   }

   return instruction;
}

/*
 ******************************************************************************
 * buf_out will contain street related text
 * Auxiliary
 */
INLINE_DEC _format_street_text( const char* street_name, BOOL add_at, char buf_out[], int buf_size )
{
   if ( add_at )
      snprintf( buf_out, buf_size, "%s %s", NAV_TTS_TEXT_AT, _parse_nav_text( street_name ) );
   else
      snprintf( buf_out, buf_size, "%s", _parse_nav_text( street_name ) );
}
