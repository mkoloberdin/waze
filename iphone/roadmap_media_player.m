/* roadmap_media_player.m - iPod integral media player
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R
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

#import <MediaPlayer/MediaPlayer.h>

#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_lang.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_lang.h"
#include "roadmap_screen.h"
#include "roadmap_device_events.h"
#include "roadmap_media_player.h"
#include "roadmap_iphonemedia_player.h"


// it is better to fade the music while voice is played. Pause does not sound well => disabled for now
#define ALLOW_PAUSE                 0

#define PLAYER_VOLUME_BAR_HEIGHT    40
#define PLAYER_CONTROLS_HEIGHT      60
#define PLAYER_MEDIA_DETAILS_HEIGHT 60
#define PLAYER_LABEL_HEIGHT         20
#define PLAYER_TRACK_SEEK_HEIGHT    40
#define PLAYER_TIME_LABEL_WIDTH     50

#define PLAYER_STATE_REFRESH_RESOLUTION 3

enum player_button_touch_stats {
   PLAYER_BUTTON_RELEASED = 1,
   PLAYER_BUTTON_DOWN,
   PLAYER_BUTTON_HOLD
};

enum player_button_tags {
   PLAYER_TAG_REWIND = 1,
   PLAYER_TAG_FF
};

static MPMusicPlayerController *RoadMapMusicPlayer = NULL;
static int gs_isActive = -1;


int roadmap_media_player_state (void) {
   return gs_isActive;
}

void roadmap_media_player_pause (BOOL should_pause) {
   static BOOL paused = FALSE;
   
   if (ALLOW_PAUSE) {
      if (should_pause && RoadMapMusicPlayer.playbackState == MPMusicPlaybackStatePlaying) {
         [RoadMapMusicPlayer pause];
         paused = TRUE;
      } else if (!should_pause && paused) {
         [RoadMapMusicPlayer play];
         paused = FALSE;
      }
   }
}

void roadmap_media_player_toggle (void) {
   if (RoadMapMusicPlayer.playbackState == MPMusicPlaybackStatePlaying) {
      [RoadMapMusicPlayer pause];
      gs_isActive = 0;
   } else {
      [RoadMapMusicPlayer play];
      gs_isActive = 1;
   }
   
   roadmap_screen_redraw();
}


void roadmap_media_player_shutdown(){
   roadmap_media_player_pause (FALSE);
}

void roadmap_media_player_init() {
#if TARGET_IPHONE_SIMULATOR
	return;
#endif //TARGET_IPHONE_SIMULATOR
   
   if (!RoadMapMusicPlayer)
      RoadMapMusicPlayer = [MPMusicPlayerController iPodMusicPlayer];
   
   if (RoadMapMusicPlayer.playbackState == MPMusicPlaybackStatePlaying)
      gs_isActive = 1;
}

void roadmap_media_player() {
   if (!RoadMapMusicPlayer) {
      roadmap_log (ROADMAP_ERROR, "Error - music player not initialized");
      return;
   }
   
   RoadMapMediaPlayerView *MediaPlayerView = [[RoadMapMediaPlayerView alloc] init];
   [MediaPlayerView show];
}




//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

@implementation RoadMapMediaPlayerView
@synthesize playerControlsToolbar;
@synthesize playerArtworkView;
@synthesize playerVolumeControlView;
@synthesize playerTrackSeekView;
@synthesize noArtworkImage;
@synthesize nowPlayingArtistLabel;
@synthesize nowPlayingTrackLabel;
@synthesize nowPlayingAlbumLabel;
@synthesize nowPlayingElapsedLabel;
@synthesize nowPlayingRemainingLabel;
@synthesize playerElapsedTimer;
@synthesize playerShuffleBtn;

- (void) resizeViews
{
   CGRect bounds = self.view.bounds;
   CGRect rect;
   
   rect = CGRectMake(10, bounds.size.height - PLAYER_VOLUME_BAR_HEIGHT, bounds.size.width - 20, PLAYER_VOLUME_BAR_HEIGHT);
   playerVolumeControlView.frame = rect;
   
   rect = CGRectMake(0, bounds.size.height - PLAYER_VOLUME_BAR_HEIGHT - PLAYER_CONTROLS_HEIGHT, bounds.size.width, PLAYER_CONTROLS_HEIGHT);
   playerControlsToolbar.frame = rect;
   
   playerArtworkView.center = self.view.center;
   rect = playerArtworkView.frame;
   rect.origin.y += 10;
   playerArtworkView.frame = rect;
   
   rect = CGRectMake(10, 0, bounds.size.width - 20, PLAYER_LABEL_HEIGHT);
   nowPlayingArtistLabel.frame = rect;
   //rect.origin.y += PLAYER_LABEL_HEIGHT;
   //nowPlayingTrackLabel.frame = rect;
   [nowPlayingTrackLabel sizeToFit];
   rect.origin.y += PLAYER_LABEL_HEIGHT;
   nowPlayingAlbumLabel.frame = rect;
   
   rect.origin.y += PLAYER_LABEL_HEIGHT;
   rect.size.height = PLAYER_TRACK_SEEK_HEIGHT;
   rect.size.width = PLAYER_TIME_LABEL_WIDTH;
   rect.origin.x = 0;
   nowPlayingElapsedLabel.frame = rect;
   rect.origin.x = bounds.size.width - PLAYER_TIME_LABEL_WIDTH;
   nowPlayingRemainingLabel.frame = rect;
   rect.origin.x = PLAYER_TIME_LABEL_WIDTH;
   rect.size.width = bounds.size.width - 2*PLAYER_TIME_LABEL_WIDTH;
   if (playerTrackSeekView.enabled) {
      playerTrackSeekView.frame = rect;
   } else {
      playerTrackSeekView.enabled = YES;
      playerTrackSeekView.frame = rect;
      playerTrackSeekView.enabled = NO;
   }
   
   rect = playerShuffleBtn.frame;
   rect.origin.x = bounds.size.width - 10 - rect.size.width;
   //rect.origin.y = 2* PLAYER_LABEL_HEIGHT;
   rect.origin.y = PLAYER_LABEL_HEIGHT;
   playerShuffleBtn.frame = rect;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
   [self resizeViews];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}

- (void)viewDidAppear:(BOOL)animated {
   [self resizeViews]; //workaround
}

- (void)viewWillAppear:(BOOL)animated {   
   UIButton *button;
   UIBarButtonItem *barButton;
   UIImage *image;
   
   UINavigationBar *navBar = self.navigationController.navigationBar;
   navBar.barStyle = UIBarStyleBlackOpaque;
   
   //Nav bar controls
   
   UINavigationItem *navItem = [self navigationItem];
   image = roadmap_iphoneimage_load("player_back");
   if (image) {
      button = [UIButton buttonWithType:UIButtonTypeCustom];
      [button addTarget:self action:@selector(onBack) forControlEvents:UIControlEventTouchUpInside];
      [button setBackgroundImage:image forState:UIControlStateNormal];
      [button sizeToFit];
      barButton = [[UIBarButtonItem alloc] initWithCustomView:button];
      [image release];
   } else
      barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Settings menu")]
                                                style:UIBarButtonItemStylePlain target:self action:@selector(onBack)];
   [navItem setLeftBarButtonItem: barButton];
	[barButton release];
   
   
   image = roadmap_iphoneimage_load("player_list");
   if (image) {
      button = [UIButton buttonWithType:UIButtonTypeCustom];
      [button addTarget:self action:@selector(onPicker) forControlEvents:UIControlEventTouchUpInside];
      [button setBackgroundImage:image forState:UIControlStateNormal];
      [button sizeToFit];
      barButton = [[UIBarButtonItem alloc] initWithCustomView:button];
      [image release];
   } else
      barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Select music")]
                                                   style:UIBarButtonItemStylePlain target:self action:@selector(onPicker)];
   [navItem setRightBarButtonItem: barButton];
	[barButton release];
   
   [super viewWillAppear:animated];
   [self resizeViews];
}

- (void) onBack
{
   [self removeElapsedTimer];
   skipSliderRefresh = 0;
   
   [RoadMapMusicPlayer endGeneratingPlaybackNotifications];
   
	[self unregisterObservers];
   
   UINavigationBar *navBar = self.navigationController.navigationBar;
   navBar.barStyle = UIBarStyleDefault;
   
   roadmap_main_show_root(YES);
}

- (void) onPicker
{
   MPMediaPickerController *mediaPicker = [[MPMediaPickerController alloc] init];
   
   mediaPicker.delegate = self;
   mediaPicker.prompt = [NSString stringWithUTF8String:roadmap_lang_get("Listen to music while using waze")];
   mediaPicker.allowsPickingMultipleItems = YES;
   
   roadmap_main_present_modal(mediaPicker);
   
   [mediaPicker release];
}

- (void) close
{
   roadmap_main_dismiss_modal();
}

- (void) onElapsedTimer
{
   NSTimeInterval currentPlaybackTime, leftPlaybackTime, totalPlaybackTime;
   MPMediaItem *nowPlayingItem = [RoadMapMusicPlayer nowPlayingItem];
   int minutes,seconds;
   
   if (nowPlayingItem) {
      currentPlaybackTime = RoadMapMusicPlayer.currentPlaybackTime;
      totalPlaybackTime = ((NSNumber *)[nowPlayingItem valueForProperty: MPMediaItemPropertyPlaybackDuration]).longValue;
      leftPlaybackTime = totalPlaybackTime - currentPlaybackTime;
      minutes = floor(currentPlaybackTime/60);
      seconds = currentPlaybackTime - minutes*60;
      nowPlayingElapsedLabel.text = [NSString stringWithFormat:@"%02d:%02d", minutes, seconds];
      minutes = floor(leftPlaybackTime/60);
      seconds = leftPlaybackTime - minutes*60;
      nowPlayingRemainingLabel.text = [NSString stringWithFormat:@"-%02d:%02d", minutes, seconds];
      
      if (!skipSliderRefresh)
         [playerTrackSeekView setValue:currentPlaybackTime / totalPlaybackTime animated:YES];
      else
         skipSliderRefresh--;
   }
   
}

- (void) setElapsedTimer
{
   if (playerElapsedTimer) return;
   
   playerElapsedTimer = [NSTimer scheduledTimerWithTimeInterval: 0.5
                                                         target: self
                                                       selector: @selector (onElapsedTimer)
                                                       userInfo: nil
                                                        repeats: YES];
}

- (void) removeElapsedTimer
{
   if (playerElapsedTimer) {
      [playerElapsedTimer invalidate];
      playerElapsedTimer = NULL;
   }
}

- (void) sliderAction
{
   NSTimeInterval totalPlaybackTime;
   MPMediaItem *nowPlayingItem = [RoadMapMusicPlayer nowPlayingItem];
   
   if (nowPlayingItem) {
      totalPlaybackTime = ((NSNumber *)[nowPlayingItem valueForProperty: MPMediaItemPropertyPlaybackDuration]).longValue;
      RoadMapMusicPlayer.currentPlaybackTime = playerTrackSeekView.value * totalPlaybackTime;
      skipSliderRefresh = 2;
   }
}

- (void) NowPlayingItemDidChangeNotification: (id) notification
{
   //update artwork
   UIImage *image = noArtworkImage;
   MPMediaItem *nowPlayingItem = [RoadMapMusicPlayer nowPlayingItem];
   
   if (nowPlayingItem) {
      MPMediaItemArtwork *artwork = [nowPlayingItem valueForProperty: MPMediaItemPropertyArtwork];
      if (artwork)
         image = [artwork imageWithSize:CGSizeMake(200,200)];
      playerTrackSeekView.enabled = YES;
      
      //update labels
      nowPlayingArtistLabel.text = [nowPlayingItem valueForProperty: MPMediaItemPropertyArtist];
      nowPlayingTrackLabel.text = [nowPlayingItem valueForProperty: MPMediaItemPropertyTitle];
      //self.title = [nowPlayingItem valueForProperty: MPMediaItemPropertyTitle];
      nowPlayingAlbumLabel.text = [nowPlayingItem valueForProperty: MPMediaItemPropertyAlbumTitle];
      
      [self setElapsedTimer];
   } else {
      [self removeElapsedTimer];
      
      nowPlayingElapsedLabel.text = @"00:00";
      nowPlayingRemainingLabel.text = @"-00:00";
      [playerTrackSeekView setValue:0.0f animated:YES];;
      playerTrackSeekView.enabled = NO;
      
      //update labels
      nowPlayingArtistLabel.text = @"";
      nowPlayingTrackLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("(no playlist selected)")];
      //self.title = @"";
      nowPlayingAlbumLabel.text = @"";
   }
   
   playerArtworkView.image = image;
   playerControlsToolbar.hidden = YES;
   [playerArtworkView sizeToFit];
   playerArtworkView.center = playerArtworkView.superview.center;
   playerControlsToolbar.hidden = NO;
}

- (void) PlaybackStateDidChangeNotification: (id) notification
{
   UIBarButtonItem   *barItem;
   UIImage           *image;
   
   if (RoadMapMusicPlayer.playbackState == MPMusicPlaybackStatePlaying) {
      image = roadmap_iphoneimage_load("player_pause");
      gs_isActive = 1;
   } else {
      image = roadmap_iphoneimage_load("player_play");
      gs_isActive = 0;
      
      if (RoadMapMusicPlayer.playbackState == MPMusicPlaybackStateStopped)
         [RoadMapMusicPlayer stop];
   }
   
   if (!image)
      return;
   
   NSMutableArray *buttonArray = [NSMutableArray arrayWithArray: playerControlsToolbar.items];
   barItem = [buttonArray objectAtIndex:3];
   [(UIButton *)barItem.customView setBackgroundImage:image forState:UIControlStateNormal];
   [image release];
   [(UIButton *)barItem.customView sizeToFit];
   [buttonArray replaceObjectAtIndex:3 withObject:barItem];
   [playerControlsToolbar setItems:buttonArray];
}

- (void) onShuffleToggle
{
   UIImage *image;
   
   if (RoadMapMusicPlayer.shuffleMode == MPMusicShuffleModeOff) {
      RoadMapMusicPlayer.shuffleMode = MPMusicShuffleModeSongs;
      image = roadmap_iphoneimage_load("player_shuffle_on");
   } else {
      RoadMapMusicPlayer.shuffleMode = MPMusicShuffleModeOff;
      image = roadmap_iphoneimage_load("player_shuffle_off");
   }
   
   if (image) {
      [playerShuffleBtn setBackgroundImage:image forState:UIControlStateNormal];
      [image release];
   }
}

- (void) onPlayPause
{
   if (RoadMapMusicPlayer.playbackState == MPMusicPlaybackStatePlaying)
      [RoadMapMusicPlayer pause];
   else if (RoadMapMusicPlayer.playbackState == MPMusicPlaybackStatePaused ||
            RoadMapMusicPlayer.playbackState == MPMusicPlaybackStateInterrupted ||
            RoadMapMusicPlayer.playbackState == MPMusicPlaybackStateStopped)
      [RoadMapMusicPlayer play];
}

- (void) onRewind
{
   if (RoadMapMusicPlayer.playbackState == MPMusicPlaybackStatePlaying ||
       RoadMapMusicPlayer.playbackState == MPMusicPlaybackStatePaused ||
       RoadMapMusicPlayer.playbackState == MPMusicPlaybackStateInterrupted) {
      if (RoadMapMusicPlayer.currentPlaybackTime > 1)
         [RoadMapMusicPlayer skipToBeginning];
      else
         [RoadMapMusicPlayer skipToPreviousItem];
   }
}

- (void) onFF
{
   if (RoadMapMusicPlayer.playbackState == MPMusicPlaybackStatePlaying ||
       RoadMapMusicPlayer.playbackState == MPMusicPlaybackStatePaused ||
       RoadMapMusicPlayer.playbackState == MPMusicPlaybackStateInterrupted)
      [RoadMapMusicPlayer skipToNextItem];
}

- (void) onSeekBackward
{
   if (RoadMapMusicPlayer.playbackState == MPMusicPlaybackStatePlaying ||
       RoadMapMusicPlayer.playbackState == MPMusicPlaybackStatePaused ||
       RoadMapMusicPlayer.playbackState == MPMusicPlaybackStateInterrupted) {
      [RoadMapMusicPlayer beginSeekingBackward];
   }
}

- (void) onSeekForward
{
   if (RoadMapMusicPlayer.playbackState == MPMusicPlaybackStatePlaying ||
       RoadMapMusicPlayer.playbackState == MPMusicPlaybackStatePaused ||
       RoadMapMusicPlayer.playbackState == MPMusicPlaybackStateInterrupted) {
      [RoadMapMusicPlayer beginSeekingForward];
   }
}

- (void) onLongTouchTimer: (NSTimer*)theTimer
{
   UIButton *button = (UIButton *)[theTimer userInfo];
   
   if (button.tag == PLAYER_TAG_REWIND &&
       button_rewind_touch_state == PLAYER_BUTTON_DOWN) {
      button_rewind_touch_state = PLAYER_BUTTON_HOLD;
      [self onSeekBackward];
   } else if (button.tag == PLAYER_TAG_FF &&
              button_ff_touch_state == PLAYER_BUTTON_DOWN) {
      button_ff_touch_state = PLAYER_BUTTON_HOLD;
      [self onSeekForward];
   }
}

- (void) onDown: (id) object
{
   NSTimer *timer;
   UIButton *button = (UIButton *)object;
   
   if (button.tag == PLAYER_TAG_REWIND)
      button_rewind_touch_state = PLAYER_BUTTON_DOWN;
   else if (button.tag == PLAYER_TAG_FF)
      button_ff_touch_state = PLAYER_BUTTON_DOWN;
   else return;
   
   timer = [NSTimer scheduledTimerWithTimeInterval: 0.5
                                            target: self
                                          selector: @selector (onLongTouchTimer:)
                                          userInfo: button
                                           repeats: NO];
}

- (void) onUp: (id) object
{
   UIButton *button = (UIButton *)object;
   
   if (button.tag == PLAYER_TAG_REWIND &&
       button_rewind_touch_state == PLAYER_BUTTON_DOWN) {
      button_rewind_touch_state = PLAYER_BUTTON_RELEASED;
      [self onRewind];
   } else if (button.tag == PLAYER_TAG_FF &&
              button_ff_touch_state == PLAYER_BUTTON_DOWN) {
      button_ff_touch_state = PLAYER_BUTTON_RELEASED;
      [self onFF];
   } else if (button.tag == PLAYER_TAG_REWIND &&
             button_rewind_touch_state == PLAYER_BUTTON_HOLD) {
      button_rewind_touch_state = PLAYER_BUTTON_RELEASED;
      [RoadMapMusicPlayer endSeeking];
   } else if (button.tag == PLAYER_TAG_FF &&
              button_ff_touch_state == PLAYER_BUTTON_HOLD) {
      button_ff_touch_state = PLAYER_BUTTON_RELEASED;
      [RoadMapMusicPlayer endSeeking];
   }
}

- (void) onOut: (id) object
{
   UIButton *button = (UIButton *)object;
   
   if (button.tag == PLAYER_TAG_REWIND &&
       button_rewind_touch_state == PLAYER_BUTTON_DOWN) {
      button_rewind_touch_state = PLAYER_BUTTON_RELEASED;
   } else if (button.tag == PLAYER_TAG_FF &&
       button_ff_touch_state == PLAYER_BUTTON_DOWN) {
      button_ff_touch_state = PLAYER_BUTTON_RELEASED;
   } else if (button.tag == PLAYER_TAG_REWIND &&
              button_rewind_touch_state == PLAYER_BUTTON_HOLD) {
      button_rewind_touch_state = PLAYER_BUTTON_RELEASED;
      [RoadMapMusicPlayer endSeeking];
   } else if (button.tag == PLAYER_TAG_FF &&
              button_ff_touch_state == PLAYER_BUTTON_HOLD) {
      button_ff_touch_state = PLAYER_BUTTON_RELEASED;
      [RoadMapMusicPlayer endSeeking];
   }
}


- (void) registerObservers
{
   NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
   
   [notificationCenter addObserver: self
                          selector: @selector (NowPlayingItemDidChangeNotification:)
                              name: MPMusicPlayerControllerNowPlayingItemDidChangeNotification
                            object: RoadMapMusicPlayer];
   
   [notificationCenter addObserver: self
                          selector: @selector (PlaybackStateDidChangeNotification:)
                              name: MPMusicPlayerControllerPlaybackStateDidChangeNotification
                            object: RoadMapMusicPlayer];
   
}

- (void) unregisterObservers
{
   NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
   
   [notificationCenter removeObserver:self];
}

- (void) createView
{
   CGRect rect;
   UIImage *image;
   UIButton *button;
   UIBarButtonItem *barItem;
   
   noArtworkImage = roadmap_iphoneimage_load("player_no_artwork");
   
   NSMutableArray *buttonArray = [NSMutableArray arrayWithCapacity:0];
	UIBarButtonItem *flexSpacer = [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemFlexibleSpace
                                                                               target:nil action:nil]; 
   
   rect = [[UIScreen mainScreen] applicationFrame];
	rect.origin.x = 0;
	rect.origin.y = 0;
	rect.size.height = roadmap_main_get_mainbox_height();
	UIView *containerView = [[UIView alloc] initWithFrame:rect];
	self.view = containerView;
	[containerView release]; // decrement retain count
   
   containerView.backgroundColor = [UIColor blackColor];
   
    
   //Add artwork
   MPMediaItem *nowPlayingItem = [RoadMapMusicPlayer nowPlayingItem];

   image = noArtworkImage;
   if (nowPlayingItem) {
      MPMediaItemArtwork *artwork = [nowPlayingItem valueForProperty: MPMediaItemPropertyArtwork];
      if (artwork)
         image = [artwork imageWithSize:CGSizeMake(200,200)];
   }
   
   playerArtworkView = [[UIImageView alloc] initWithImage:image];
   playerArtworkView.center = containerView.center;
   [containerView addSubview:playerArtworkView];
   [playerArtworkView release];
   
   //Add now playing labels
   nowPlayingArtistLabel = [[UILabel alloc] initWithFrame:CGRectZero];
   nowPlayingTrackLabel = [[UILabel alloc] initWithFrame:CGRectZero];
   nowPlayingAlbumLabel = [[UILabel alloc] initWithFrame:CGRectZero];
   nowPlayingArtistLabel.text = [nowPlayingItem valueForProperty: MPMediaItemPropertyArtist];
   nowPlayingTrackLabel.text = [nowPlayingItem valueForProperty: MPMediaItemPropertyTitle];
   nowPlayingAlbumLabel.text = [nowPlayingItem valueForProperty: MPMediaItemPropertyAlbumTitle];
   nowPlayingArtistLabel.textColor = [UIColor lightGrayColor];
   nowPlayingTrackLabel.textColor = [UIColor whiteColor];
   nowPlayingAlbumLabel.textColor = [UIColor lightGrayColor];
   nowPlayingArtistLabel.backgroundColor = [UIColor clearColor];
   nowPlayingTrackLabel.backgroundColor = [UIColor clearColor];
   nowPlayingAlbumLabel.backgroundColor = [UIColor clearColor];
   nowPlayingArtistLabel.textAlignment = UITextAlignmentCenter;
   nowPlayingTrackLabel.textAlignment = UITextAlignmentCenter;
   nowPlayingAlbumLabel.textAlignment = UITextAlignmentCenter;
   [containerView addSubview:nowPlayingArtistLabel];
   //[containerView addSubview:nowPlayingTrackLabel];
   [self.navigationItem setTitleView:nowPlayingTrackLabel];
   nowPlayingTrackLabel.adjustsFontSizeToFitWidth = YES;
   nowPlayingTrackLabel.minimumFontSize = 12;
   [containerView addSubview:nowPlayingAlbumLabel];
   [nowPlayingArtistLabel release];
   [nowPlayingTrackLabel release];
   [nowPlayingAlbumLabel release];
   
   //Add track seek
   playerTrackSeekView = [[UISlider alloc] initWithFrame:CGRectZero];
   [playerTrackSeekView setValue:0.0f animated:YES];;
   playerTrackSeekView.enabled = YES;
   [playerTrackSeekView addTarget:self action:@selector(sliderAction) forControlEvents:UIControlEventValueChanged];
   [containerView addSubview:playerTrackSeekView];
   nowPlayingElapsedLabel = [[UILabel alloc] initWithFrame:CGRectZero];
   nowPlayingRemainingLabel = [[UILabel alloc] initWithFrame:CGRectZero];
   nowPlayingElapsedLabel.textColor = [UIColor whiteColor];
   nowPlayingRemainingLabel.textColor = [UIColor whiteColor];
   nowPlayingElapsedLabel.font = [UIFont systemFontOfSize:12];
   nowPlayingRemainingLabel.font = [UIFont systemFontOfSize:12];
   nowPlayingElapsedLabel.backgroundColor = [UIColor clearColor];
   nowPlayingRemainingLabel.backgroundColor = [UIColor clearColor];
   nowPlayingElapsedLabel.textAlignment = UITextAlignmentCenter;
   nowPlayingRemainingLabel.textAlignment = UITextAlignmentCenter;
   [containerView addSubview:nowPlayingElapsedLabel];
   [containerView addSubview:nowPlayingRemainingLabel];
   
   
   //Add volume control
   playerVolumeControlView = [[MPVolumeView alloc] initWithFrame:CGRectZero];
   [containerView addSubview:playerVolumeControlView];
   [playerVolumeControlView release];
   
   //Add playback controls
   playerControlsToolbar = [[UIToolbar alloc] initWithFrame:CGRectZero];
   playerControlsToolbar.barStyle = UIBarStyleBlack;
   playerControlsToolbar.translucent = YES;
   [buttonArray addObject:flexSpacer];
   
   image = roadmap_iphoneimage_load("player_rw");
   if (image) {
      button = [UIButton buttonWithType:UIButtonTypeCustom];
      button.tag = PLAYER_TAG_REWIND;
      [button setBackgroundImage:image forState:UIControlStateNormal];
      [button sizeToFit];
      button.showsTouchWhenHighlighted = YES;
      //[button addTarget:self action:@selector(onRewind) forControlEvents:UIControlEventTouchUpInside];
      [button addTarget:self action:@selector(onDown:) forControlEvents:UIControlEventTouchDown];
      [button addTarget:self action:@selector(onUp:) forControlEvents:UIControlEventTouchUpInside];
      [button addTarget:self action:@selector(onOut:) forControlEvents:UIControlEventTouchDragOutside];
      barItem = [[UIBarButtonItem alloc] initWithCustomView:button];
      [buttonArray addObject:barItem];
      [barItem release];
      [buttonArray addObject:flexSpacer];
   }
   
   image = roadmap_iphoneimage_load("player_play");
   if (image) {
      button = [UIButton buttonWithType:UIButtonTypeCustom];
      [button setBackgroundImage:image forState:UIControlStateNormal];
      [button sizeToFit];
      button.showsTouchWhenHighlighted = YES;
      [button addTarget:self action:@selector(onPlayPause) forControlEvents:UIControlEventTouchUpInside];
      barItem = [[UIBarButtonItem alloc] initWithCustomView:button];
      [buttonArray addObject:barItem];
      [barItem release];
      [buttonArray addObject:flexSpacer];
   }
   
   image = roadmap_iphoneimage_load("player_ff");
   if (image) {
      button = [UIButton buttonWithType:UIButtonTypeCustom];
      button.tag = PLAYER_TAG_FF;
      [button setBackgroundImage:image forState:UIControlStateNormal];
      [button sizeToFit];
      button.showsTouchWhenHighlighted = YES;
      //[button addTarget:self action:@selector(onFF) forControlEvents:UIControlEventTouchUpInside];
      [button addTarget:self action:@selector(onDown:) forControlEvents:UIControlEventTouchDown];
      [button addTarget:self action:@selector(onUp:) forControlEvents:UIControlEventTouchUpInside];
      [button addTarget:self action:@selector(onOut:) forControlEvents:UIControlEventTouchDragOutside];
      barItem = [[UIBarButtonItem alloc] initWithCustomView:button];
      [buttonArray addObject:barItem];
      [barItem release];
      [buttonArray addObject:flexSpacer];
   }
   
   [playerControlsToolbar setItems: buttonArray];
   [flexSpacer release];
   
   [containerView addSubview:playerControlsToolbar];
   
   //suffle button
   if (RoadMapMusicPlayer.shuffleMode == MPMusicShuffleModeOff)
      image = roadmap_iphoneimage_load("player_shuffle_off");
   else
      image = roadmap_iphoneimage_load("player_shuffle_on");
   
   if (image) {
      playerShuffleBtn = [UIButton buttonWithType:UIButtonTypeCustom];
      [playerShuffleBtn setBackgroundImage:image forState:UIControlStateNormal];
      [image release];
      [playerShuffleBtn sizeToFit];
      playerShuffleBtn.showsTouchWhenHighlighted = YES;
      [playerShuffleBtn addTarget:self action:@selector(onShuffleToggle) forControlEvents:UIControlEventTouchUpInside];
      [containerView addSubview:playerShuffleBtn];
   }
   
   [self PlaybackStateDidChangeNotification:NULL];
   [self NowPlayingItemDidChangeNotification:NULL];
}

- (void) show
{
   playerElapsedTimer = NULL;
   [self createView];
   [self registerObservers];
   [RoadMapMusicPlayer beginGeneratingPlaybackNotifications];

   button_ff_touch_state = PLAYER_BUTTON_RELEASED;
   button_rewind_touch_state = PLAYER_BUTTON_RELEASED;
   
   roadmap_main_push_view(self);
}


- (void)dealloc
{
   if (noArtworkImage)
      [noArtworkImage release];
   
   
	[super dealloc];
}



/////////////////////////////
// Media Picker delegate

- (void)mediaPicker: (MPMediaPickerController *)mediaPicker didPickMediaItems:(MPMediaItemCollection *)mediaItemCollection
{
   if (mediaItemCollection) {
      [RoadMapMusicPlayer setQueueWithItemCollection:mediaItemCollection];
      [RoadMapMusicPlayer play];
   }
   
   [self close];
}

- (void)mediaPickerDidCancel:(MPMediaPickerController *)mediaPicker
{
   [self close];
}


@end

