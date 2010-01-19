/* RoadmapNativeSound.h - native sound implementation for Roadmap
 *
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps Ltd.
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
 * SYNOPSYS:
 *
 *   See roadmap_sound.h
 */

#ifndef _ROADMAP_NATIVE_SOUND__H_
#define _ROADMAP_NATIVE_SOUND__H_

extern "C" {
#include "roadmap_sound.h"
}

//  This class uses CMdaAudioPlayerUtility to play files
#include <e32def.h>
#include <MdaAudioSamplePlayer.h>
#include <AudioPlayer.h>

class CRoadMapNativeSound : public CBase,
                            //public MMdaAudioPlayerCallback,
                            public MMdaObjectStateChangeObserver
{
public:
  //  getInstance for bridging
  static CRoadMapNativeSound* GetInstance();
  static void FreeInstance();
  
  //  audio system callbacks 
  /*
  void MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds &aDuration);
  void MapcPlayComplete(TInt aError);
  */
  void MoscoStateChangeEvent(CBase *aObject, TInt aPreviousState, TInt aCurrentState, TInt aErrorCode);
  
  //  sound methods
  int PlayFile(const char *file_name);
  int PlaySound(void *mem_sound); //  play a sound that was preloaded to memory
  int PlayList(const RoadMapSoundList list);
  void Initialize();
  void Shutdown();
  int Record(const char *file_name, int seconds);
  void SetVolume( TInt aUserVolume, TInt aUserVolumeMin, TInt aUserVolumeMax );
  void SetSystemVolume( CAudioPlayer& aPlayer );
  
  typedef struct 
  {
  	TInt volume;
  	TInt minVolume;
  	TInt maxVolume;
  } UserVolumeData;
  
private:
  //  ctor/dtor
  CRoadMapNativeSound();
  virtual ~CRoadMapNativeSound();
  void CRoadMapNativeSound::ConstructL();

  //  methods
  //  Stop playing and clean up all of the players
  void StopPlayingAll();
  //  Search for the next available player and call it to Play() its file
  void PlayNextAvailable();
  
  //  members
  
  enum TState
  {
      ENotReady,
      EReadyToPlay,
      EPlaying,
  };

  // The current state of the audio player    
  TState iState;
  RPointerArray<CAudioPlayer> m_AudioPlayers;
  UserVolumeData m_UserVolume;
  
  int m_NextPlaying;  //  next item to be played; initialized to 0
  int m_NumReady;     //  number of audio items ready to play; initialized to -1   
  int m_ListSize;     //  how many do we need to be ready to play (AT LEAST)
  static CRoadMapNativeSound* m_pInstance;  
};

#endif  //  #ifndef _ROADMAP_NATIVE_SOUND__H_
