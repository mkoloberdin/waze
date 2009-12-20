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


#ifndef  __WEBSVC_ADDRESS_DEFS_H__
#define  __WEBSVC_ADDRESS_DEFS_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include "../roadmap.h"

#define  WSA_PREFIX                       ("http://")
#define  WSA_PREFIX_SIZE                  (sizeof(WSA_PREFIX)-1)

#define  WSA_SERVER_DEFAULT_PORT          (80)
#define  WSA_SERVER_URL_MAXSIZE           (64)
#define  WSA_SERVER_PORT_DELIMITER        (':')
#define  WSA_SERVER_PORT_MAXSIZE          (10)
#define  WSA_SERVER_PORT_INVALIDVALUE     (0)
#define  WSA_SERVER_URL_DELIMITER         ('/')
#define  WSA_SERVICE_NAME_MAXSIZE         (512)

#define  WSA_STRING_MINSIZE               (  WSA_PREFIX_SIZE               +   \
                                             5   /*   Address name      */ +   \
                                             1   /*   Address delimiter */ +   \
                                             1   /*   Service name      */)
                                  
#define  WSA_STRING_MAXSIZE               (  WSA_PREFIX_SIZE               +   \
                                             WSA_SERVER_URL_MAXSIZE        +   \
                                             1   /*   Port delimiter    */ +   \
                                             WSA_SERVER_PORT_MAXSIZE       +   \
                                             1   /*   Address delimiter */ +   \
                                             WSA_SERVICE_NAME_MAXSIZE)
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif   //   __WEBSVC_ADDRESS_DEFS_H__

