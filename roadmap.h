/* roadmap.h - general definitions use by the RoadMap program.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2008 Ehud Shabtai
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

#ifndef INCLUDE__ROADMAP__H
#define INCLUDE__ROADMAP__H


#define WAZE_ALPHA

#include <assert.h>
#include <stdio.h>
#ifdef __SYMBIAN32__
typedef   unsigned int      uint32_t;
typedef unsigned short      uint16_t;
#elif defined _WIN32

   #define INVALID_THREAD_ID        (0xFFFFFFFF)

   #include "Win32\stdint.h"

   #if(!(defined WINCE) && !(defined _WIN32_WCE))
      #define WIN32PC
      ///[BOOKMARK]:[NOTE] - Here you can define Touch-Screen build for Windows PC (not device)
      #define TOUCH_SCREEN
   #endif   // PC only

   #if(defined _DEBUG || defined DEBUG)
      #define  WIN32_DEBUG
   #endif   // DEBUG

#else

   #include <stdint.h>

#endif

#define  CLIENT_VERSION_MAJOR       (2)
#define  CLIENT_VERSION_MINOR       (0)
#define  CLIENT_VERSION_SUB         (3)
#define  CLIENT_VERSION_CFG         (0)	/* Build number for internal use only */

#include "roadmap_types.h"
#if defined (_WIN32) && !defined (__SYMBIAN32__)
#include "win32/roadmap_win32.h"
#elif defined (__SYMBIAN32__)
#include "symbian/roadmap_symbian_porting.h"
  #ifdef __SERIES60_31__
  #define __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__
  #warning "SERIES60_31 : __REMOVE_PLATSEC_DIAGNOSTIC_STRINGS__ defined!"
  #endif
#include <e32def.h>
#endif

#if defined (__SYMBIAN32__) || (!defined (_WIN32) && !defined (IPHONE))
#if((!defined BOOL_DEFINED))
   #define   BOOL_DEFINED
   typedef signed char   BOOL;
#endif   //   BOOL_DEFINED
#elif defined (IPHONE)
#include <objc/objc.h>
#endif

#define OBJ2STR_SCAN( val ) ( #val )
#define OBJ2STR( val ) ( OBJ2STR_SCAN( val ) )
#define GET_2_DIGIT_STRING( num_in, str_out ) \
{ \
str_out[0] = '0'; \
sprintf( &str_out[(num_in < 10)], "%d", num_in ); \
}
#define SAFE_STR( str ) ( ( str ) ? ( str ) : "NULL" )



#if defined(ANDROID) && defined(OPENGL)
#define ANDROID_OGL
#endif

extern int USING_PHONE_KEYPAD;

#ifndef   TRUE
   #define   TRUE   (1)
#endif   //   TRUE
#ifndef   FALSE
   #define   FALSE   (0)
#endif   //   FALSE

#ifndef   FREE
   #define   FREE(_p_)            if((_p_)){ free((_p_)); (_p_)=NULL;}
#endif   //   FREE

#ifdef J2ME

#ifdef assert
#undef assert
#endif

#include <java/lang.h>
#include <javax/microedition/lcdui.h>

static NOPH_Display_t assert_display;

static inline void do_assert(char *text) {

  printf ("do_assert:%s********************************************************************************************\n", text);
  if (!assert_display) assert_display = NOPH_Display_getDisplay(NOPH_MIDlet_get());
  NOPH_Alert_t msg = NOPH_Alert_new("ASSERT!", text, 0, NOPH_AlertType_get(NOPH_AlertType_INFO));
  NOPH_Alert_setTimeout(msg, NOPH_Alert_FOREVER);
  NOPH_Display_setCurrent(assert_display, msg);
  NOPH_Thread_sleep( 10000 );
}

# define assert(x) do { \
 if ( ! (x) ) \
 {\
     char msg[256]; \
     snprintf (msg, sizeof(msg), \
        "ASSERTION FAILED at %d in %s:%s\n", __LINE__, __FILE__, __FUNCTION__); \
     do_assert(msg); \
     exit(1); \
 } \
 } while(0)

#endif

#define strncpy_safe(dest, src, size) { strncpy ((dest), (src), (size)); (dest)[(size)-1] = '\0'; }

#define ROADMAP_MESSAGE_DEBUG      1
#define ROADMAP_MESSAGE_INFO       2
#define ROADMAP_MESSAGE_WARNING    3
#define ROADMAP_MESSAGE_ERROR      4
#define ROADMAP_MESSAGE_FATAL      5

#define ROADMAP_DEBUG   ROADMAP_MESSAGE_DEBUG,__FILE__,__LINE__
#define ROADMAP_INFO    ROADMAP_MESSAGE_INFO,__FILE__,__LINE__
#define ROADMAP_WARNING ROADMAP_MESSAGE_WARNING,__FILE__,__LINE__
#define ROADMAP_ERROR   ROADMAP_MESSAGE_ERROR,__FILE__,__LINE__
#define ROADMAP_FATAL   ROADMAP_MESSAGE_FATAL,__FILE__,__LINE__

#if(defined _DEBUG || defined DEBUG)
   ///[BOOKMARK]:[NOTE] -  Enable logs in debug build:
   ///#define  SKIP_DEBUG_LOGS
#endif   // DEBUG

#define DEBUG_LEVEL_SET_PATTERN  "2##2"

#define  DEFAULT_LOG_LEVEL       ROADMAP_MESSAGE_WARNING

#ifdef   WIN32_DEBUG
   ///[BOOKMARK]:[NOTE] - In case of fatal error - halt process and wait for debugger to attach
   #define  FREEZE_ON_FATAL_ERROR
#endif   // WIN32_DEBUG

typedef void (*roadmap_log_msgbox_handler) (const char *title, const char *msg);

#ifndef J2ME
void roadmap_log_push        (const char *description);
void roadmap_log_pop         (void);
#else
#define roadmap_log_push(x)
#define roadmap_log_pop()
#endif
void roadmap_log_reset_stack (void);

void roadmap_log (int level, const char *source, int line, const char *format, ...);
BOOL roadmap_log_raw_data ( const char* data );
BOOL roadmap_log_raw_data_fmt( const char *format, ... );

void roadmap_log_save_all  (void);
void roadmap_log_save_none (void);
void roadmap_log_purge     (void);

int  roadmap_log_enabled (int level, char *source, int line);

void roadmap_log_register_msgbox (roadmap_log_msgbox_handler handler);

const char *roadmap_log_path     (void);
const char *roadmap_log_filename (void);


#define roadmap_check_allocated(p) \
            roadmap_check_allocated_with_source_line(__FILE__,__LINE__,p)

#define ROADMAP_SHOW_AREA        1
#define ROADMAP_SHOW_SQUARE      2



void roadmap_option_initialize (void);

int  roadmap_option_is_synchronous (void);

char *roadmap_debug (void);

int   roadmap_verbosity  (void); /* return a minimum message level. */
int   roadmap_is_visible (int category);
void roadmap_option_set_verbosity( int verbosity_level );

char *roadmap_gps_source (void);

int roadmap_option_cache  (void);
int roadmap_option_width  (const char *name);
int roadmap_option_height (const char *name);

float roadmap_fast_forward_factor (void);

typedef void (*RoadMapUsage) (const char *section);

void roadmap_option (int argc, char **argv, RoadMapUsage usage);


/* This function is hidden by a macro: */
void roadmap_check_allocated_with_source_line
                (const char *source, int line, const void *allocated);

typedef void (* RoadMapCallback) (void);

typedef enum tag_roadmap_result
{
   // Success:
   succeeded   = 0,
   no_error    = 0,

   // General errors
   general_errors,
      err_failed,
      err_no_memory,
      err_invalid_argument,
      err_aborted,
      err_access_denied,
      err_timed_out,
      err_internal_error,
   general_errors_end,

   // Network errors
   network_errors,
      err_net_failed,
      err_net_unknown_protocol,
      err_net_remote_error,
      err_net_request_pending,
      err_net_no_path_to_destination,
   network_errors_end,

   // Data parser errors
   parser_errors,
      err_parser_unexpected_data,
      err_parser_failed,
      err_parser_missing_tag_handler,
   parser_errors_end,

   // Realtime errors
   realtime_errors,
      err_rt_no_data_to_send,
      err_rt_login_failed,
      err_rt_wrong_name_or_password,
      err_rt_wrong_network_settings,
      err_rt_unknown_login_id,
   realtime_errors_end,

   // Update account errors
   update_account_errors,
      err_upd_account_invalid_user_name,
      err_upd_account_name_already_exists,
      err_upd_account_invalid_password,
      err_upd_account_invalid_email,
      err_upd_account_email_exists,
      err_upd_account_cannot_complete_request,
   update_account_errors_end,

   // Address Search - String Resolution
   address_search,
      err_as_could_not_find_matches,
      err_as_wrong_input_string_size,
      err_as_already_in_transaction,
   address_search_end,

   // This must be last:
   all_errors_end,
   invalid_error

}  roadmap_result;

const char* roadmap_result_string( int res);
FILE * roadmap_log_get_log_file();
#define  is_module_error(_module_,_error_)    \
   ((_module_##_errors<_error_)&&(_error_<_module_##_errors_end))

#define  is_general_error(_error_)     is_module_error(general,_error_)
#define  is_network_error(_error_)     is_module_error(network,_error_)
#define  is_parser_error(_error_)      is_module_error(parser,_error_)
#define  is_realtime_error(_error_)    is_module_error(realtime,_error_)

#endif // INCLUDE__ROADMAP__H

