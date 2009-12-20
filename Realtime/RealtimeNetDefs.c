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

#include <stdio.h>
#include <string.h>
#include "RealtimeNetDefs.h"
#include "RealtimeTrafficInfo.h"

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
// Init: memset + basic settings
void RTConnectionInfo_Init(LPRTConnectionInfo   this, 
                           PFN_ONUSER           pfnOnAddUser, 
                           PFN_ONUSER           pfnOnMoveUser, 
                           PFN_ONUSER           pfnOnRemoveUser)
{
   memset( this, 0, sizeof(RTConnectionInfo));
   this->bLoggedIn      = FALSE;
   this->iServerID      = RT_INVALID_LOGINID_VALUE;
   this->bStatusVerified= FALSE;
   this->LastError      = succeeded;
   this->iMyTotalPoints = -1;
   this->iMyRanking 	= -1;
   this->iMyPreviousRanking	= -1;
   this->iMyRating	= -1;
   RTUsers_Init( &(this->Users), pfnOnAddUser, pfnOnMoveUser, pfnOnRemoveUser);
}

// Reset all data:
void RTConnectionInfo_FullReset( LPRTConnectionInfo this)
{
      RTConnectionInfo_ResetLogin( this);
/*11*/this->LastError   = succeeded;
}

// Reset data related to a login session:
void RTConnectionInfo_ResetLogin( LPRTConnectionInfo this)
{
/* 1*/memset( this->UserNm,      0, sizeof(this->UserNm));
/* 2*/memset( this->UserPW,      0, sizeof(this->UserPW));
/* 3*/memset( this->UserNk,      0, sizeof(this->UserNk));
/* 4*/memset( this->ServerCookie,0, sizeof(this->ServerCookie));
/* 5*/this->bLoggedIn = FALSE;
/* 6*/this->iServerID = RT_INVALID_LOGINID_VALUE;
/* 7*/memset( &(this->LastMapPosSent), 0, sizeof(RoadMapArea));
/* 8*/RTUsers_Reset( &(this->Users));
/* 9*/RTTrafficInfo_Reset();
 
      RTConnectionInfo_ResetTransaction( this);
}

// Reset at the end of a transaction:
void RTConnectionInfo_ResetTransaction( LPRTConnectionInfo this)
{
/* 9*/this->eTransactionStatus= TS_Idle;
      RTConnectionInfo_ResetParser( this);
}

// Reset between packet parsings:

void RTConnectionInfo_ResetParser( LPRTConnectionInfo this)
{ 
/*10*/this->bStatusVerified = FALSE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
