/* roadmap_iphonemedia_player.h - iPhone media player delegate
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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

#ifndef __ROADMAP_IPHONEMEDIA_PLAYER__H
#define __ROADMAP_IPHONEMEDIA_PLAYER__H

#include "roadmap_camera_defs.h"


@interface RoadMapMediaPlayerView : UIViewController <MPMediaPickerControllerDelegate> {
   UIToolbar      *playerControlsToolbar;
   UIImageView    *playerArtworkView;
   UIImage        *noArtworkImage;
   MPVolumeView   *playerVolumeControlView;
   UISlider       *playerTrackSeekView;
   UILabel        *nowPlayingArtistLabel;
   UILabel        *nowPlayingTrackLabel;
   UILabel        *nowPlayingAlbumLabel;
   UILabel        *nowPlayingElapsedLabel;
   UILabel        *nowPlayingRemainingLabel;
   NSTimer        *playerElapsedTimer;
   UIButton       *playerShuffleBtn;
   int            skipSliderRefresh;
   int            button_ff_touch_state;
   int            button_rewind_touch_state;
}

@property (nonatomic, retain) UIToolbar      *playerControlsToolbar;
@property (nonatomic, retain) UIImageView    *playerArtworkView;
@property (nonatomic, retain) UIImage        *noArtworkImage;
@property (nonatomic, retain) MPVolumeView   *playerVolumeControlView;
@property (nonatomic, retain) UISlider       *playerTrackSeekView;
@property (nonatomic, retain) UILabel        *nowPlayingArtistLabel;
@property (nonatomic, retain) UILabel        *nowPlayingTrackLabel;
@property (nonatomic, retain) UILabel        *nowPlayingAlbumLabel;
@property (nonatomic, retain) UILabel        *nowPlayingElapsedLabel;
@property (nonatomic, retain) UILabel        *nowPlayingRemainingLabel;
@property (nonatomic, retain) UIButton       *playerShuffleBtn;
@property (nonatomic, retain) NSTimer        *playerElapsedTimer;


- (void) show;
- (void) removeElapsedTimer;
- (void) unregisterObservers;


@end


#endif // __ROADMAP_IPHONEMEDIA_PLAYER__H