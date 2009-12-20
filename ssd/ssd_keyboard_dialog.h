
/* ssd_keyboard_dialog.h
 *
 * LICENSE:
 *
 *   Copyright 2009 PazO
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
 */

#ifndef __SSD_KEYBOARD_DIALOG_H__
#define __SSD_KEYBOARD_DIALOG_H__

#include "ssd_dialog.h"


#define SSD_KB_DLG_SHOW_NEXT_BTN		0x00000001		/* Show "next" button under the edit box */
#define SSD_KB_DLG_LAST_KB_STATE		0x00000002		/* Show last keyboard state (input grid and input type) */
#define SSD_KB_DLG_INPUT_ENGLISH		0x00000004		/* Show keyboard of English input type   (if last keyboard flag not set */
#define SSD_KB_DLG_TYPING_LOCK_ENABLE	0x00000008		/* Enables typing lock while driving */

// If callback returns TRUE - keyboard will be closed
//    Otherwize - keyboard will remain open
// Exception:  If exit_code is 'dec_cancel' keyboard is closed
typedef BOOL(*CB_OnKeyboardDone)(int         exit_code,
                                 const char* value,
                                 void*       context);

void ssd_show_keyboard_dialog_ext( const char*       title,
								   const char*       value,
								   const char*       label,
								   const char*       note,
								   CB_OnKeyboardDone cbOnDone,
								   void*             context,
								   int kb_dlg_flags );

void ssd_show_keyboard_dialog(const char*       title,
                              const char*       value,
                              CB_OnKeyboardDone cbOnDone,
                              void*             context);


#endif // __SSD_KEYBOARD_DIALOG_H__
