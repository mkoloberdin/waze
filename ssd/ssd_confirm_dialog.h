/* ssd_confirm_dialog.h - confirmation dialogt
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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

#ifndef __SSD_CONFIRM_DIALOG_H_
#define __SSD_CONFIRM_DIALOG_H_



typedef void(*ConfirmDialogCallback)(int exit_code, void *context);

void ssd_confirm_dialog (const char *title, const char *text, BOOL default_yes,  ConfirmDialogCallback callback, void *context);
void ssd_confirm_dialog_timeout (const char *title, const char *text, BOOL default_yes,  ConfirmDialogCallback callback, void *context,int seconds);
void ssd_confirm_dialog_custom_timeout (const char *title, const char *text, BOOL default_yes, ConfirmDialogCallback callback, void *context,const char *textYes, const char *textNo, int seconds);

/**
 * Creates a confirm dialog, same as regluar ssd_confirm_dialog, but with customized button text.
 */
void ssd_confirm_dialog_custom (const char *title, const char *text, BOOL default_yes, ConfirmDialogCallback callback, void *context,const char *textYes, const char *textNo);
void ssd_confirm_dialog_close( void );
#endif // __SSD_CONFIRM_DIALOG_H_

