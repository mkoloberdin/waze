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


#ifndef   __FREEMAP_REALTIMESYSTEMMESSAGE_H__
#define   __FREEMAP_REALTIMESYSTEMMESSAGE_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  RTSYSTEMMESSAGE_QUEUE_MAXSIZE          (20)

#define  URL_SYSTEM_MESSAGE_POPUP       10
#define  URL_SYSTEM_MESSAGE_FULL_SCREEN 11

typedef struct tagRTSystemMessage
{
   int   iId;
   int   iType;
   int   iShow;
   char* title;   // Max size: RTNET_SYSTEMMESSAGE_TITLE_MAXSIZE
   char  titleTextColor[16];
   int   titleTextSize;
   char* msg;    // Max size: RTNET_SYSTEMMESSAGE_TEXT_MAXSIZE
   char  msgTextColor[16];
   int   msgTextSize;
   char* icon;

}  RTSystemMessage, *LPRTSystemMessage;

void RTSystemMessage_Init( LPRTSystemMessage this);
void RTSystemMessage_Free( LPRTSystemMessage this);

void RTSystemMessagesInit(void);
void RTSystemMessagesDisplay(void);
int RTSystemMessagesGetLastMessageDisplayed(void);

// System-Message queue
void  RTSystemMessageQueue_Push( LPRTSystemMessage this);
BOOL  RTSystemMessageQueue_Pop ( LPRTSystemMessage this);
int   RTSystemMessageQueue_Size();
BOOL  RTSystemMessageQueue_IsEmpty();
BOOL  RTSystemMessageQueue_IsFull();
void  RTSystemMessageQueue_Empty();
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif   //   __FREEMAP_REALTIMESYSTEMMESSAGE_H__
