/* roadmap_androidmsgbox.h - the interface to the android msgbox module
 *
 * LICENSE:
 *
 *   Copyright 2010 Waze Ltd, Alex Agranovich (AGA)
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
 *
 * DESCRIPTION:
 *
 *
 *
 *
 */

#ifndef INCLUDE__ROADMAP_ANDROID_MSGBOX__H
#define INCLUDE__ROADMAP_ANDROID_MSGBOX__H

typedef void (*MsgBoxOnCloseCallback)( int res_code, void* context );

/*
 * JNI Callback context
 */
typedef struct
{
   MsgBoxOnCloseCallback callback;
   void *context;
} AndrMsgBoxCbContext;


/***********************************************************/
/*  Name        : void roadmap_androidmsgbox_show()
 *  Purpose     : Shows android native message box
 *  Params      :  title - title at the top of the box
 *              :  message - text at the body of the box
 *              :  label_ok - label of the ok button
 *              :  label_cancel - label of the cancel button
 *              :  context - context for the buttons' callback
 *              :  callback - buttons callback
 *
 */
void roadmap_androidmsgbox_show( const char* title, const char* message, const char* label_ok,
      const char* label_cancel, void* context, MsgBoxOnCloseCallback callback );

#endif // INCLUDE__ROADMAP_ANDROID_MSGBOX__H

