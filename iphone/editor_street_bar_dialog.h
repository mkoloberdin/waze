/* editor_street_bar_dialog.h - iPhone dialog for editor street bar
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
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

#ifndef EDITOR_STREET_BAR_DIALOG_H_
#define EDITOR_STREET_BAR_DIALOG_H_

#include "roadmap_gps.h"
#include "roadmap_plugin.h"

@interface EditorStreetBarView : UIView <UITextFieldDelegate, UIPickerViewDelegate, UIPickerViewDataSource> {
   BOOL     isCreated;
   int      secondaryState;
   PluginLine       selectedLine;
   BOOL     isTracking;
   BOOL     isEditing;
   BOOL     isChangesMade;
   BOOL isRangeUpdated;
   BOOL     isPreviouseNewName;
   BOOL     isLock;
   UIPickerView *typePicker;
   int catCount;
   char **categories;
   int cfcc;
   int dir;
   int saved_dir;
   RoadMapGpsPosition CurrentGpsPoint;
   RoadMapPosition    CurrentFixedPosition;
   int CurrentDirection;
   char  street_t2s[512];
   char street_type[512];
}

- (void) showStreet: (SelectedLine *)line;
- (void) showTracking:(PluginLine *)line andPos:(const RoadMapGpsPosition *) gps_position andLock:(BOOL) lock;
- (void) stopTracking;
- (void) onSaveConfirmed;
- (void) onClose;
- (BOOL) isActive;

@end

#endif //EDITOR_STREET_BAR_DIALOG_H_