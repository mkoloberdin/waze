/* RealtimeExternalPoiDlg.h - Manage External POIs Dialog
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi B.S
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


#ifndef REALTIMEEXTERNALPOIDLG_H_
#define REALTIMEEXTERNALPOIDLG_H_

#define EXTERNAL_POI_DLG_NAME "RealtimeExternalPoiDlg"

void RealtimeExternalPoiDlg(RTExternalPoi *externalPoi);
void RealtimeExternalPoiDlg_Timed(RTExternalPoi *externalPoi, int seconds);
char *RealtimeExternalPoiDlg_GetPromotionUrl(RTExternalPoi *externalPoi);
#endif /* REALTIMEEXTERNALPOIDLG_H_ */
