
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

#ifndef  __LOCAL_SEARCH_DLG_H__
#define  __LOCAL_SEARCH_DLG_H__

#include "../ssd/ssd_dialog.h"

void local_search_dlg_show( PFN_ON_DIALOG_CLOSED cbOnClosed,
                              void*                context);

BOOL local_search_auto_search( const char* address);

#endif   // __LOCAL_SEARCH_DLG_H__
