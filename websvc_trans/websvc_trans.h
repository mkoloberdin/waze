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


#ifndef   __HTTPTRANSACTION_H__
#define   __HTTPTRANSACTION_H__

#include "../websvc_trans/websvc_trans_defs.h"

#define  INVALID_WEBSVC_HANDLE      (NULL)

wst_handle  wst_init( const char* service_name, const char* content_type);
void        wst_term( wst_handle h);

BOOL        wst_start_trans(   
               wst_handle           session,       // Session object
               const char*          action,        // (/<service_name>/)<ACTION>
               const wst_parser_ptr parsers,       // Array of 1..n data parsers
               int                  parsers_count, // Parsers count
               CB_OnWSTCompleted    cbOnCompleted, // Callback for transaction completion
               void*                context,       // Caller context
               const char*          szFormat,      // Custom data for the HTTP request
               ...);                               // Parameters

transaction_state wst_get_trans_state(   
               wst_handle           session);

void        wst_stop_trans(   
               wst_handle           session);

void        wst_watchdog(           wst_handle  session);
BOOL        wst_queue_is_empty(     wst_handle  session);
void        wst_queue_clear(        wst_handle  session);
BOOL        wst_process_queue_item( wst_handle  session,
                                    BOOL*       transaction_started);

#endif   //   __HTTPTRANSACTION_H__
