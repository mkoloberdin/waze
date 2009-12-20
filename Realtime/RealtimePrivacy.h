/* RealtimePrivacy.h - Manages users privacy settings
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2008 Ehud Shabtai
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
 
#ifndef REALTIMEPRIVACY_H_
#define REALTIMEPRIVACY_H_

//////////////////////////////////////////////
// Context menu:
typedef enum privacy_context_menu_items
{
   privacy_cm_save,
   privacy_cm_exit,
   
   privacy_cm__count,
   privacy_cm__invalid

}  privacy_context_menu_items;

#define PRIVACY_TITLE "Privacy settings"
#define PRIVACY_DIALOG "privacy_prefs"

void RealtimePrivacyInit(void);
void RealtimePrivacySettings(void);
int  RealtimePrivacyState(void);

#endif /*REALTIMEPRIVACY_H_*/
