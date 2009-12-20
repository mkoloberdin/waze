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
#include "RealtimeDefs.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
const char* ERTVisabilityGroup_to_string( ERTVisabilityGroup e)
{
   switch(e)
   {  
      case VisGrp_NickName:    		  return RT_CFG_PRM_VISGRP_Nickname;
      case VisGrp_Anonymous: 		  return RT_CFG_PRM_VISGRP_Anonymous;
      case VisGrp_Invisible:     	  return RT_CFG_PRM_VISGRP_Invisible;
      default:             		      return RT_CFG_PRM_VISGRP_Anonymous;
      
   }
} 

ERTVisabilityGroup ERTVisabilityGroup_from_string( const char* szE)
{
   if( !strcmp( szE, RT_CFG_PRM_VISGRP_Nickname))return VisGrp_NickName;
   if( !strcmp( szE, RT_CFG_PRM_VISGRP_Anonymous))return VisGrp_Anonymous;
   if( !strcmp( szE, RT_CFG_PRM_VISGRP_Invisible)) return VisGrp_Invisible;
   
   return VisGrp_Anonymous;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char* ERTVisabilityReport_to_string( ERTVisabilityReport e)
{
   switch(e)
   {  
      case VisRep_NickName:    		  return RT_CFG_PRM_VISREP_Nickname;
      case VisRep_Anonymous: 		  return RT_CFG_PRM_VISREP_Anonymous;
      default:             		      return RT_CFG_PRM_VISREP_Anonymous;
      
   }
} 

ERTVisabilityReport ERTVisabilityReport_from_string( const char* szE)
{
   if( !strcmp( szE, RT_CFG_PRM_VISREP_Nickname))return VisRep_NickName;
   if( !strcmp( szE, RT_CFG_PRM_VISREP_Anonymous))return VisRep_Anonymous;

  
   return VisRep_Anonymous;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void  VersionUpgradeInfo_Init( LPVersionUpgradeInfo this)
{ memset( this, 0, sizeof(VersionUpgradeInfo)); this->eSeverity = VUS_NA;}

void StatusStatistics_Init( LPStatusStatistics this)
{ memset( this, 0, sizeof(StatusStatistics));}

void StatusStatistics_Reset( LPStatusStatistics this)
{
   this->iNetworkErrors          = 0;
   this->iNetworkErrorsSuccessive= 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
