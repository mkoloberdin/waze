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
#include "../roadmap_config.h"
#include "../roadmap_main.h"
#include "roadmap_messagebox.h"
#include "Realtime.h"
#include "RealtimeDefs.h"

static RoadMapConfigDescriptor RoadMapConfigLastIdDisplayed =
      ROADMAP_CONFIG_ITEM("System Messages", "Last message ID displayed");

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void RTSystemMessage_Init( LPRTSystemMessage this)
{ memset( this, 0, sizeof(RTSystemMessage));}

void RTSystemMessage_Free( LPRTSystemMessage this)
{
   FREE_MEM(this->Title)
   FREE_MEM(this->Text)
   RTSystemMessage_Init( this);
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
static   RTSystemMessage   RTSystemMessageQueue[RTSYSTEMMESSAGE_QUEUE_MAXSIZE];
static   int               FirstItem = -1;
static   int               ItemsCount=  0;

static int IncrementIndex( int i)
{
   i++;
   
   if( i < RTSYSTEMMESSAGE_QUEUE_MAXSIZE)
      return i;
      
   return 0;
}

static int AllocCell()
{
   int iNextFreeCell;

   if( RTSYSTEMMESSAGE_QUEUE_MAXSIZE == ItemsCount)
      return -1;
   
   if( -1 == FirstItem)
   {
      FirstItem = 0;
      ItemsCount= 1;
      return 0;
   }
   
   iNextFreeCell = (FirstItem + ItemsCount);
   ItemsCount++;

   if( iNextFreeCell < RTSYSTEMMESSAGE_QUEUE_MAXSIZE)
      return iNextFreeCell;
      
   return (iNextFreeCell - RTSYSTEMMESSAGE_QUEUE_MAXSIZE);
}

void RTSystemMessagesDisplay_CB( int exit_code ){
   RTSystemMessagesDisplay();
}

void RTSystemMessagesDisplay_Timer( void ){
   RTSystemMessagesDisplay();
}

static LPRTSystemMessage AllocSystemMessage()
{
   LPRTSystemMessage pCell = NULL;
   int               iCell = AllocCell();
   if( -1 == iCell)
      return NULL;
   
   pCell = RTSystemMessageQueue + iCell;
   RTSystemMessage_Init( pCell);
   
   return pCell;
}

static BOOL PopOldest( LPRTSystemMessage pSM)
{
   LPRTSystemMessage pCell;
   
   if( !ItemsCount || (-1 == FirstItem))
   {
      if( pSM)
         RTSystemMessage_Init( pSM);
      return FALSE;
   }
   
   pCell = RTSystemMessageQueue + FirstItem;

   if( pSM)
   {
      // Copy item data:
      (*pSM) = (*pCell);
      // Detach item from queue:
      RTSystemMessage_Init( pCell);
   }
   else
      // Item is NOT being copied; Release item resources:
      RTSystemMessage_Free( pCell);
   
   if( 1 == ItemsCount)
   {
      ItemsCount =  0;
      FirstItem  = -1;
   }
   else
   {
      ItemsCount--;
      FirstItem = IncrementIndex( FirstItem);
   }
   
   return TRUE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void  RTSystemMessageQueue_Push( LPRTSystemMessage this)
{
   LPRTSystemMessage p;
   
   if( RTSystemMessageQueue_IsFull())
      PopOldest( NULL);

   if (!RTSystemMessageQueue_Size())
      roadmap_main_set_periodic( 60000, RTSystemMessagesDisplay_Timer );

   p = AllocSystemMessage();
   
   (*p) = (*this);     
}

BOOL  RTSystemMessageQueue_Pop ( LPRTSystemMessage this)
{ 
   BOOL success = PopOldest( this);
   if (!RTSystemMessageQueue_Size())
      roadmap_main_remove_periodic(RTSystemMessagesDisplay_Timer );
   return success; 
}

int   RTSystemMessageQueue_Size()
{ return ItemsCount;}

BOOL  RTSystemMessageQueue_IsEmpty()
{ return 0 == ItemsCount;}

BOOL  RTSystemMessageQueue_IsFull()
{ return RTSYSTEMMESSAGE_QUEUE_MAXSIZE == ItemsCount;}

void  RTSystemMessageQueue_Empty()
{ while( PopOldest( NULL));}

int RTSystemMessagesGetLastMessageDisplayed(void){
   return roadmap_config_get_integer(&RoadMapConfigLastIdDisplayed);
}

void RTSystemMessagesSetLastMessageDisplayed(int iLastMessageDisplayed){
   roadmap_config_set_integer(&RoadMapConfigLastIdDisplayed, iLastMessageDisplayed);
   roadmap_config_save(TRUE);
}



void RTSystemMessagesDisplay(void){
   if( RTSystemMessageQueue_Size()){
      RTSystemMessage systemMessage;
      RTSystemMessage_Init(&systemMessage);
      RTSystemMessageQueue_Pop(&systemMessage);
      RTSystemMessagesSetLastMessageDisplayed(systemMessage.iId);
      roadmap_messagebox_cb(systemMessage.Title, systemMessage.Text, RTSystemMessagesDisplay_CB);
   }
}

void RTSystemMessagesInit(void){
   
   
   roadmap_config_declare ("user",
                           &RoadMapConfigLastIdDisplayed,
                           "0", NULL);
   
}

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
