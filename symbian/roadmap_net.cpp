/* roadmap_net.cpp - Network interface for the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps Ltd
 *   Copyright 2008 Ehud Shabtai
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
 *   See roadmap_net.h
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>

extern "C"{
#include "roadmap.h"
#include "md5.h"
#include "roadmap_net.h"
#include "roadmap_net_mon.h"
#include "roadmap_start.h"
#include "../roadmap_config.h"
}

#include <es_sock.h>
#include <in_sock.h>
#include "Roadmap_NativeSocket.h"
#include "Roadmap_NativeHTTP.h"
#include "GSConvert.h"


static RoadMapConfigDescriptor RoadMapConfigConnectionIAPId =
                        ROADMAP_CONFIG_ITEM("Connection", "IAP");
static RoadMapConfigDescriptor RoadMapConfigConnectionAuto =
                        ROADMAP_CONFIG_ITEM("Connection", "AutoConnect");


static RoadMapSocket roadmap_net_connect_async_internal (const char *protocol,
                                         const char *name,
                                         time_t update_time,
                                         int default_port,
                                         RoadMapNetConnectCallback callback,
                                         void *context, 
                                         int& retVal)
{
  //  Create a CRoadMapNativeNet object
  CRoadMapNativeNet* s = NULL;
  ENativeSockType protocolType = ESockTypeInvalid;
  int err = 0; 
   
  if (strncmp (protocol, "http", 4) == 0)
  {
    protocolType = ESockTypeHttp;
  }
  else if (strcmp (protocol, "udp") == 0)
  {
    protocolType = ESockTypeUdp;
  } 
  else if (strcmp (protocol, "tcp") == 0) 
  {
    protocolType = ESockTypeTcp;
  } 
  else 
  {
    roadmap_log (ROADMAP_ERROR, "unknown protocol %s", protocol);
    retVal = KErrGeneral;
  }
  
  if ( protocolType == ESockTypeHttp )
  {
    const char *http_type = "POST";
    if (!strcmp(protocol, "http_get")) http_type = "GET";
    TRAPD(err1, s = CRoadMapNativeHTTP::NewL( http_type, name, default_port, update_time, 
                                              callback, context));
    if ( s == NULL || err1 != KErrNone) { err = err1; }
  }
  else
  {
    TRAPD(err1, s = CRoadMapNativeSocket::NewL( name, default_port, 
                                                callback, context));
    if ( s == NULL || err1 != KErrNone) { err = err1; }
  }

  retVal = err;
  return s;
}


RoadMapSocket roadmap_net_connect (const char *protocol,
                                   const char *name, 
                                   time_t update_time,
                                   int default_port, roadmap_result* err)
{
  int retVal = KErrNone;
  RoadMapSocket s;
  
  if (err != NULL)
 	(*err) = succeeded;
 	   
  s = roadmap_net_connect_async_internal(protocol, name, update_time, default_port, NULL, NULL, retVal);
  
  if ((s == NULL) && (err != NULL))
   	(*err) = err_net_failed;                                                   
 
  return s;
  
}

int roadmap_net_connect_async (const char *protocol,
                               const char *name,
                               time_t update_time,
                               int default_port,
                               RoadMapNetConnectCallback callback,
                               void *context)
{
  int retVal = KErrNone;
  RoadMapSocket s = roadmap_net_connect_async_internal(protocol, name, update_time,
                                                      default_port, callback, context,
                                                      retVal);
	if ( s == NULL) 
	{
	  if ( retVal == KErrNotReady )
	  {
	    roadmap_net_mon_offline();
	  }
	  else
	  {
			roadmap_net_mon_error("Error in connect.");
	  }
	  return -1;
	}
	
	/*
	if ( strcmp(protocol, "http") == 0 )
	{
	  (*callback)(s, context);
	}
	*/
	return 0;
}

int roadmap_net_send_socket (RoadMapSocket s, const void *data, int length, int wait) {

   int total = length;
   CRoadMapNativeNet* net = (CRoadMapNativeNet*)s;
   
   roadmap_net_mon_send(length);
   
   while (length > 0) {
      int res = net->Write(data, length);
      if (res < 0) {
         roadmap_net_mon_error("Error in send.");
         roadmap_log (ROADMAP_ERROR, "Error sending data");
         return -1;
      }

      length -= res;
      data = (char *)data + res;
   }

   return total;
}

int roadmap_net_send_http (RoadMapSocket s, const void *ptr, int in_size, int wait) 
{
  int data_size = in_size;
  CRoadMapNativeNet *n = (CRoadMapNativeNet *)s;
  CRoadMapNativeHTTP* net = dynamic_cast<CRoadMapNativeHTTP*>(n);
    
  if ( net->ReadyToSendData() == false )
  {
    char *start = NULL;
    char *cur = NULL;
    char *value;
    char *eol;
    char *backslash_r = NULL;

    start = strdup((const char *)ptr);
    cur = start;
    while ((eol = strstr(cur, "\r\n"))) 
    {
      *eol = '\0';
      eol += 2;
      if (*cur == 0) 
      {
        net->SetRequestProperty("User-Agent", roadmap_start_version());
        net->SetReadyToSendData(true);
        cur = eol;
        break;
      }

      value = strchr(cur, ':');
      *value = '\0';
      value++;
      while (*value == ' ') value++;
      backslash_r = strchr(value, '\r');
      if (backslash_r != NULL)
      {
        *backslash_r = '\0';
      }

      //printf ("HTTP send - %s:%s\n", cur, value);
      net->SetRequestProperty(cur, value);
      cur = eol;
    }

    ptr = (char *)ptr + (cur - start);
    data_size -= (cur - start);
    free(start);
  }

  roadmap_net_mon_send(data_size);
  // Send the body data
  if (data_size) net->Write(ptr, data_size);

  return in_size;
}

int roadmap_net_send (RoadMapSocket s, const void *ptr, int in_size, int wait) 
{
  if (((CRoadMapNativeNet*)s)->IsHttp() == true) 
  {
    return roadmap_net_send_http(s, ptr, in_size, wait);
  }
  return roadmap_net_send_socket(s, ptr, in_size, wait);
}
  
int roadmap_net_receive (RoadMapSocket s, void *data, int size) {

  CRoadMapNativeNet* net = (CRoadMapNativeNet*)s;
  int received = net->Read(data, size);

   if (received == 0) {
      roadmap_net_mon_error("Error in recv.");
      return -1; /* On UNIX, this is sign of an error. */
   }

   roadmap_net_mon_recv(received);
   return received;
}


RoadMapSocket roadmap_net_listen(int port) {

   return ROADMAP_INVALID_SOCKET; // TODO implement
}


RoadMapSocket roadmap_net_accept(RoadMapSocket server_socket) {

   return ROADMAP_INVALID_SOCKET; // TODO implement
}


void roadmap_net_close (RoadMapSocket s) 
{
  CRoadMapNativeNet* net = (CRoadMapNativeNet*)s;
  net->Close();
  //delete(net);
  roadmap_net_mon_disconnect();
}

void roadmap_net_shutdown ()
{
  CRoadMapNativeNet::Shutdown();
  roadmap_net_mon_destroy();
  // Save the chosen value
  roadmap_config_set_integer( &RoadMapConfigConnectionIAPId, 
		  							CRoadMapNativeNet::GetChosenAP() );
}

int roadmap_net_unique_id (unsigned char *buffer, unsigned int size) {
   struct MD5Context context;
   unsigned char digest[16];
   time_t tm;

   time(&tm);
      
//   MD5Init (&context);
//   MD5Update (&context, (unsigned char *)&tm, sizeof(tm));
//   MD5Final (digest, &context);

//   if (size > sizeof(digest)) size = sizeof(digest);
   memcpy(buffer, digest, size);

   return size;
}

void roadmap_net_initialize()
{
	TUint32 configAP = 0;
	TBool isAutoConnect;  
	// Declaration and loading of the autoconnection parameter
	roadmap_config_declare( "user", &RoadMapConfigConnectionAuto, "no", NULL );
	isAutoConnect = roadmap_config_match( &RoadMapConfigConnectionAuto, "yes" );
	
	// Declaration of the last network id parameter
	roadmap_config_declare( "user", &RoadMapConfigConnectionIAPId, "0", NULL );
	
	// Only if autoconnect flag is positive, the last AP id is relevant
	if ( isAutoConnect )
	{
		configAP = roadmap_config_get_integer( &RoadMapConfigConnectionIAPId );
	}
	
	// Setting up
	CRoadMapNativeNet::SetChosenAP( configAP );	
}
