/*
 * LICENSE:
 *
 *   Copyright 2008 PazO
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
 */

#ifndef   __HTTPTRANSACTIONDEFS_H__
#define   __HTTPTRANSACTIONDEFS_H__

#include <time.h>
#include "../roadmap.h"
#include "../roadmap_net.h"
#include "../websvc_trans/efficient_buffer.h"
#include "../websvc_trans/string_parser.h"
#include "../websvc_trans/websvc_address.h"

#define  WST_HTTP_STATUS_STRING_MAXSIZE      (63)
#define  WST_RESPONSE_TAG_MAXSIZE            (31)
#if defined(__SYMBIAN32__)
#define  WST_RESPONSE_BUFFER_SIZE            (2048)   // 2K
#else
#define  WST_RESPONSE_BUFFER_SIZE            (32768)   // 32K
#endif
#define  WST_SESSION_TIMEOUT                 (75)  /* seconds */
#define  HTTP_HEADER_MAX_SIZE                (400)
#define  WST_WEBSERVICE_METHOD_MAX_SIZE      (0xFF)
#define  WST_MIN_PARSERS_COUNT               ( 1)
#define  WST_MAX_PARSERS_COUNT               (30)

typedef void*  wst_handle;

typedef enum tag_transaction_result
{
   trans_succeeded,
   trans_failed,
   trans_in_progress

}  transaction_result;

typedef enum tag_transaction_state
{
   trans_idle,
   trans_active,
   trans_stopping,

}  transaction_state;

//   Response Parser State
typedef enum tag_http_parsing_state
{
   http_not_parsed,

   http_status_verified,
   http_parse_completed

}  http_parsing_state;

//   Response Parser
//      Structure to hold details of the custom status ("200 OK")
typedef struct tag_http_response_status
{
   int   rc;
   char  text[WST_HTTP_STATUS_STRING_MAXSIZE+1];

}  http_response_status;

void                 http_response_status_init( http_response_status*   this);
transaction_result   http_response_status_load( http_response_status*   this,
                                                const char*             szResponse,
                                                BOOL                    verify_tag,
                                                int*                    pBytesRead);

#include "../websvc_trans/cyclic_buffer.h"

// Callback:   need_more_data
//
// Abstract:   Notifies caller on transaction termination
//
// Input:
//    context  -  [in]  Caller context
//    res      -  [in]  Transaction result
//
typedef void (*CB_OnWSTCompleted)( void* context, roadmap_result res);

//
// Callback:   CB_OnWSTResponse
//
// Abstract:   Client data parser
//
// Input:
//    data              -  [in]  Null terminated string to parse
//    context           -  [in]  Caller context
//    more_data_needed  -  [out] Caller need more data
//    rc                -  [out] In case of failure - additional information
//                               regarding the failure
//
// Return:
//    On success     -  Pointer to location following the parsed data.
//                      Example: If 'data' was "abc\ncbs" and method parsed
//                               "abc\n", then method should return pointer
//                               to "cbs".
//    On failure     -  NULL
//                      In this case parsing is stopped and transaction fails.
typedef const char* (*CB_OnWSTResponse)(  /* IN  */   const char*       data,
                                          /* IN  */   void*             context,
                                          /* OUT */   BOOL*             more_data_needed,
                                          /* OUT */   roadmap_result*   rc);


typedef struct tag_wst_parser
{
   const char*       tag;
   CB_OnWSTResponse  parser;

}  wst_parser, *wst_parser_ptr;

#include "../websvc_trans/websvc_trans_queue.h"

typedef struct tag_wst_context
{
/* General:       */
/* 1*/   const char*          service;
/* 2*/   const char*          content_type;
/* 3*/   int                  port;
/* 4*/   RoadMapSocket        Socket;
/* 5*/   transaction_state    state;
/* 6*/   BOOL                 async_receive_started;
/* 7*/   time_t               starting_time;
/* 8*/   wst_queue            queue;
/* 9*/   void*                context;

/* Send:          */
/*10*/   ebuffer              packet;

/* Receive:       */
/*11*/   cyclic_buffer        CB;
/*12*/   http_parsing_state   http_parser_state;
/*13*/   wst_parser_ptr       parsers;
/*14*/   int                  parsers_count;

/* Completion:    */
/*15*/   CB_OnWSTCompleted    cbOnWSTCompleted;
/*16*/   transaction_result   result;
/*17*/   roadmap_result       rc;
/*18*/   BOOL                 delete_on_idle;   // Object should be deleted after transaction

}     wst_context, *wst_context_ptr;
void  wst_context_init  (  wst_context_ptr      this);
void  wst_context_free  (  wst_context_ptr      this);
void  wst_context_reset (  wst_context_ptr      this);
void  wst_context_load  (  wst_context_ptr      this,
                           const wst_parser_ptr parsers,
                           int                  parsers_count,
                           CB_OnWSTCompleted   cbOnCompleted,
                           void*                context);

// Same function, but buffer is ready formatted:
BOOL RTNet_HttpAsyncTransaction_FormattedBuffer(
               wst_handle           session,       // Session object
               const char*          action,        // (/<service_name>/)<ACTION>
               const wst_parser_ptr parsers,       // Array of 1..n data parsers
               int                  parsers_count, // Parsers count
               CB_OnWSTCompleted   cbOnCompleted, // Callback for transaction completion
               void*                context,       // Caller context
               char*                Data);         // Custom data for the HTTP request


#define  DECLARE_WEBSVC_PARSER(_name_)                                  \
   const char* _name_(  /* IN  */   const char*       data,             \
                        /* IN  */   void*             context,          \
                        /* OUT */   BOOL*             more_data_needed, \
                        /* OUT */   roadmap_result*   rc);

#endif   //   __HTTPTRANSACTIONDEFS_H__
