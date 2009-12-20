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


#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include "websvc_address.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SN_IsValid( const char* szWebServiceAddress)
{ return WSA_ExtractParams( szWebServiceAddress, NULL, NULL, NULL);}

/*   

Method:   
   BOOL WSA_ExtractParams(...)

Abstract:
   Break full service name,such as 'http://MyAddress.com:30/MyServiceName', into:
      Address:    'MyAddress.com:30'
      Port:       30
      Service:    '/MyServiceName'

   Another case:   'http://MyAddress.com/MyServiceName'
      Address:    'MyAddress.com'
      Port:       WSA_SERVER_PORT_INVALIDVALUE
      Service:    '/MyServiceName'

Return result:
   TRUE:    Method succeeded;
            Service full name 'szWebServiceAddress' was found valid; 
            If output parameters were supplied, they contain the parameters value.

   FALSE:   Method failed; 
            Service name 'szWebServiceAddress' contains invalid value.
   
Assumption:
   If the OPTIONAL buffers 'pServerURL' and 'pServiceName' were supplied, their sizes 
   must be at least 'WSA_SERVER_URL_MAXSIZE+1' and 'WSA_SERVICE_NAME_MAXSIZE+1', 
   correspondingly.                                                   */

BOOL   WSA_ExtractParams(   
         const char* szWebServiceAddress, //   IN     -  Web service full address (http://...)
         char*       pServerURL,          //   OUT,OPT-  Server URL[:Port]
         int*        pServerPort,         //   OUT,OPT-  Server Port
         char*       pServiceName)        //   OUT,OPT-  Web service name
{
   size_t   i;
   char*    pTmp     = NULL;
   char*    pSrvURL  = NULL;
   char*    pSrvPrt  = NULL;
   char*    pWebSvc  = NULL;
   int      iPort    = WSA_SERVER_DEFAULT_PORT;
   char     Copy[WSA_STRING_MAXSIZE+1];
   
   //   Verify sizes are ok:
   if( !szWebServiceAddress                              || 
      !(*szWebServiceAddress)                            || 
      (strlen(szWebServiceAddress)< WSA_STRING_MINSIZE)  ||
      (WSA_STRING_MAXSIZE < strlen(szWebServiceAddress)))
      return FALSE;
   
   //   Copy of service-name:
   for( i=0; i<strlen(szWebServiceAddress); i++)
      Copy[i] = szWebServiceAddress[i];
   Copy[i] = '\0';
   
   //   Search for 'http://':
   if( 0 != strncasecmp( (const char*)Copy, WSA_PREFIX, WSA_PREFIX_SIZE))
      return FALSE;

   //   Server URL:
   pSrvURL = (Copy + WSA_PREFIX_SIZE);
   
   //   Do we have a port number?
   pTmp = strchr( pSrvURL, WSA_SERVER_PORT_DELIMITER);
   if( pTmp)
   {
      //   Service-port found
      pSrvPrt = pTmp + 1;
      
      //   Reset 'tmp'
      pTmp = pSrvPrt;
   }
   else
      pTmp = pSrvURL;

   //   Search for server URL termination dellimiter:
   pTmp = strchr( pTmp, WSA_SERVER_URL_DELIMITER);
   if( !pTmp)
      return FALSE;   //   Address termination delimiter was not found
   
   //   Next value is service-name
   pWebSvc = pTmp + 1;
   (*pTmp) = '\0';      //   Terminate last value (port-number, or server URL)

   //   Verify server URL is valid:
   if( !(*pSrvURL) || (WSA_SERVER_URL_MAXSIZE < strlen(pSrvURL)))
      return FALSE;

   //   If we had port-number, verify it is a valid integer value:
   if( pSrvPrt)
   {
      //   Terminate port value string:
      iPort = atoi(pSrvPrt);
      if( (0 == iPort) || (WSA_SERVER_PORT_INVALIDVALUE == iPort))
         return FALSE;   //   Port-number was zero, or string did not hold an integer value
   }

   //   Do we have a name?
   if( !(*pWebSvc) || (WSA_SERVICE_NAME_MAXSIZE < strlen(pWebSvc)))
      return FALSE;

   //   All is cosher, copy output:
   if( pServerURL)
      strncpy( pServerURL, pSrvURL, WSA_SERVER_URL_MAXSIZE);
   if( pServerPort)
      (*pServerPort) = iPort;
   if( pServiceName)
      snprintf( pServiceName, WSA_SERVICE_NAME_MAXSIZE, "/%s", pWebSvc);

   return TRUE;
}

void  WSA_RemovePortNumberFromURL(
            char*       szWebServiceAddress) //   IN,OUT    -   Web service full address (http://...)
{
   char* szInput           = szWebServiceAddress;
   char* szPortOffset_begin= NULL;
   char* szPortOffset_end  = NULL;
   int   iEndSize          = 0;

   if( !szWebServiceAddress || !(*szWebServiceAddress))
      return;

   do
   {
      szPortOffset_begin = strchr( szInput, ':');
      if( !szPortOffset_begin)
         return;

      if( isdigit( szPortOffset_begin[1]))
         break;

      szInput = (szPortOffset_begin + 1);

   }  while( *szInput);
   
   if( !(*szInput))
      return;

   szPortOffset_end  = strchr( szPortOffset_begin, '/');
   if( !szPortOffset_end)
      szPortOffset_end  = strchr( szPortOffset_begin, '\\');
   
   if( !szPortOffset_end)
   {
      (*szPortOffset_begin) = '\0';
      return;
   }

   iEndSize = strlen(szPortOffset_end);
   memmove( szPortOffset_begin, szPortOffset_end, iEndSize);
   szPortOffset_begin[iEndSize] = '\0';
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
