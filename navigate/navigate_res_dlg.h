/* navigate_res_dlg.h
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi Ben-Shoshan
 *
 *   This file is part of RoadMap.
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
 *
 *
 */


#ifndef NAVIGATE_RES_DLG_H_
#define NAVIGATE_RES_DLG_H_

#include "ssd/ssd_widget.h"
SsdWidget navigate_res_ETA_widget(int iRouteDistance, int iRouteLenght, const char *via, BOOL showDistance, BOOL showAltBtn, SsdCallback callback);
void navigate_res_update_ETA_widget(SsdWidget dlg, SsdWidget container, int iRouteDistance, int iRouteLenght, const char *via, BOOL showDistance);
void navigate_res_hide_ETA_widget(SsdWidget container);
void navigate_res_show_ETA_widget(SsdWidget container);
void navigate_res_dlg (int NavigateFlags, const char *pTitleText, int iRouteDistance, int iRouteLenght, const char *via, int iTimeOut, BOOL show_diclaimer) ;
void navigate_res_dlg_switch_eta_display(void);
#endif /* NAVIGATE_RES_DLG_H_ */
