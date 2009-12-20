/* ssd_progres_msg_dialog.h - The interface for the progress message dialog
 *
 * LICENSE:
 *
 *   Copyright 2009, Waze Ltd
 *                   Alex Agranovich
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
 */

#ifndef INCLUDE__SSD_PROGRESS_MSG_DIALOG__H
#define INCLUDE__SSD_PROGRESS_MSG_DIALOG__H

#ifdef __cplusplus
extern "C" {
#endif
#include "../roadmap.h"
#include "../ssd/ssd_widget.h"

void ssd_progress_msg_dialog_show( const char* dlg_text );

void ssd_progress_msg_dialog_set_text( const char* dlg_text );

void ssd_progress_msg_dialog_hide( void );

void ssd_progress_msg_dialog_show_timed( const char* dlg_text , int seconds);

#ifdef __cplusplus
}
#endif
#endif /* INCLUDE__SSD_PROGRESS_MSG_DIALOG__H */

