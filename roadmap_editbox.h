/* roadmap_editbox.h - Android edit box defines and declarations
 *
 * LICENSE:
 *
 *   Copyright 2010 Alex Agranovich (AGA), Waze Ltd
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
 */

#ifndef INCLUDE__ROADMAP_EDITBOX__H
#define INCLUDE__ROADMAP_EDITBOX__H
#ifdef __cplusplus
extern "C" {
#endif

#include "roadmap.h"

typedef BOOL (*SsdKeyboardCallback) ( int exit_code, const char *value, void *context );

typedef struct
{
   SsdKeyboardCallback callback;
   void* cb_context;
   const char* text;
} EditBoxContextType;

#define EDITBOX_TOP_MARGIN         (60)              // In pixels of HVGA                TODO: Should be dynamic

#define EDITBOX_TYPE_MASK     0x000000FFL
#define EDITBOX_ACTION_MASK   0x0000FF00L
/*
 * Edit box type flags
 */
typedef enum
{
      // Edit box types.  Masked by 0x0000000F
      EEditBoxStandard      = 0x00000001,      // Standard edit box
      EEditBoxPassword      = 0x00000002,      // Secured edit box for password
      EEditBoxAlphaNumeric  = 0x00000004,
      EEditBoxNumeric       = 0x00000008,
      EEditBoxAlphabetic    = 0x00000010,
      // Edit box action types. Masked by 0x000000F0
      EEditBoxActionDefault  = 0x00000100,
      EEditBoxActionDone     = 0x00000200,
      EEditBoxActionSearch   = 0x00000400,
      EEditBoxActionNext     = 0x00000800,
      // If should stay on screen after action (ENTER)
      EEditBoxStayOnAction  = 0x00010000,
      // If empty input is forbidden
      EEditBoxEmptyForbidden = 0x00020000,
      // If editbox should be shown without drawing new dialog
      EEditBoxEmbedded = 0x00040000
} TEditBoxType;

void ShowEditbox(const char* aTitleUtf8, const char* aTextUtf8, SsdKeyboardCallback callback, void *context, TEditBoxType aBoxType );

void roadmap_editbox_dlg_hide( void );

#ifdef __cplusplus
}
#endif
#endif // INCLUDE__ROADMAP_EDITBOX__H

