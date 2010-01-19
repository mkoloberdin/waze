/* RoadmapNativeSound.cpp - native sound implementation for Roadmap
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

#include <stdlib.h>
#include <string.h>

extern "C" {
#include "roadmap_res.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_symbian_porting.h"
#include "roadmap.h"
#include "roadmap_lang.h"
#include "roadmap_prompts.h"
}

#include "Roadmap_NativeSound.h"
#include "AudioPlayer.h"
#include "GSConvert.h"
#include <f32file.h>

CRoadMapNativeSound* CRoadMapNativeSound::m_pInstance = NULL;
CRoadMapNativeSound* CRoadMapNativeSound::GetInstance () 
{// Assuming threaded application
  if ( m_pInstance == NULL )
  {
    m_pInstance = new CRoadMapNativeSound;
  }
  return m_pInstance;
}

void CRoadMapNativeSound::FreeInstance () 
{// Assuming threaded application
  if ( m_pInstance != NULL )
  {
    delete m_pInstance;
    m_pInstance = NULL;
  }
}
  
CRoadMapNativeSound::CRoadMapNativeSound() 
{
  iState = ENotReady;
  m_NextPlaying = 0;
  m_NumReady = 0;
  m_ListSize = 0;
}

CRoadMapNativeSound::~CRoadMapNativeSound()
{
  Shutdown();
}

void CRoadMapNativeSound::ConstructL()
{
  /* single audio player init
  if ( iAudioPlayer == NULL )
  {
    iAudioPlayer = CMdaAudioPlayerUtility::NewL(*this);
    if ( iAudioPlayer != NULL )
    {
      iAudioPlayer->SetVolume(iAudioPlayer->MaxVolume());
      iState = ENotReady;
    }
  }
  */
}

/*
void CRoadMapNativeSound::MapcInitComplete(TInt aError,const TTimeIntervalMicroSeconds &aDuration)
{
  if ( aError != KErrNone )
  {//TODO do something here...
    //roadmap_log(ROADMAP_ERROR,"Sound file error %d", aError);

    //  We do not know which player failed to init,
    //  so we cannot remove it properly. 
    
    //TODO Perhaps use the NumReady to get the failed index list? 
  }

  ++m_NumReady;  
  if ( m_ListSize == m_NumReady )
  {// Everyone's on board, we can start playing
    m_NextPlaying = 0;
    PlayNextAvailable();
  }
}

void CRoadMapNativeSound::MapcPlayComplete(TInt aError)
{
  //  Stop the current one playing
  ((CAudioPlayer*)m_AudioPlayers[m_NextPlaying-1])->Stop();
  //  Go on to the next one
  PlayNextAvailable();
}
*/

void CRoadMapNativeSound::MoscoStateChangeEvent(CBase *aObject, TInt aPreviousState, TInt aCurrentState, TInt aErrorCode)
{
  if ((aPreviousState == CMdaAudioClipUtility::ENotReady) && 
      (aCurrentState == CMdaAudioClipUtility::EOpen)) 
  { //Audio sample is open now
    iState = EReadyToPlay;
    ++m_NumReady;
    if ( m_ListSize == m_NumReady )
    {// Everyone's on board, we can start playing
      m_NextPlaying = 0;
      PlayNextAvailable();
    }
  }

  if ((aPreviousState == CMdaAudioClipUtility::EOpen) && 
      (aCurrentState == CMdaAudioClipUtility::EPlaying)) 
  { //Beginning to play sample
    iState = EPlaying;
  }

  if ((aPreviousState == CMdaAudioClipUtility::EPlaying) && 
      (aCurrentState == CMdaAudioClipUtility::EOpen)) 
  { //Not playing sample
    iState = EReadyToPlay;
    //  Stop the current one playing 
    //  TODO do we need this??
    ((CAudioPlayer*)m_AudioPlayers[m_NextPlaying-1])->Stop();
    //  Go on to the next one
    PlayNextAvailable();
  }
}

void CRoadMapNativeSound::PlayNextAvailable()
{
  while ( m_NextPlaying <= m_AudioPlayers.Count()-1 )
  {
    CAudioPlayer* pPlayer = (CAudioPlayer*)m_AudioPlayers[m_NextPlaying++];
    if ( pPlayer != NULL )
    {
      SetSystemVolume( *pPlayer );
      TInt err = pPlayer->Play();
      if ( err != KErrNone )
      {// ain't no good player, mon
        pPlayer->Stop();
      }
      break;
    }
  }
}

int CRoadMapNativeSound::PlayFile(const char *file_name)
{
  TFileName fileName;
  GSConvert::CharPtrToTDes16(file_name, fileName);
  CAudioPlayer* pPlayer = NULL;
  TRAPD(err, pPlayer = CAudioPlayer::NewL(fileName, this)); 
  if ( err == KErrNone && pPlayer != NULL )
  {
    m_AudioPlayers.Append(pPlayer);
  }
  return 0;
}

int CRoadMapNativeSound::PlaySound(void *mem_sound)
{
  if ( mem_sound == NULL )  { return -1;  }
  
  TPtr8 memPtr((TUint8*)(roadmap_file_base((RoadMapFileContext)mem_sound)), 
                roadmap_file_size((RoadMapFileContext)mem_sound));
  CAudioPlayer* pPlayer = NULL;
  TRAPD(err, pPlayer = CAudioPlayer::NewL(memPtr, this)); 
  if ( err == KErrNone && pPlayer != NULL )
  {
    m_AudioPlayers.Append(pPlayer);
  }
  return 0;
}

int CRoadMapNativeSound::PlayList(const RoadMapSoundList list)
{
  m_AudioPlayers.ResetAndDestroy(); //  stop playing whatever is playing now
  m_NextPlaying = 0;
  m_NumReady = 0;
  
  int listSize = roadmap_sound_list_count(list);
  m_ListSize = listSize;  //  know how much we need to play
  for (int i = 0; i < roadmap_sound_list_count(list); i++) 
  {//TODO load resource 
     const char *name = roadmap_sound_list_get (list, i);
     RoadMapSound sound =
                    (RoadMapSound) roadmap_res_get (RES_SOUND, RES_NOCREATE, name);
     if (sound) {
       PlaySound (sound);
     } else {
       char sound_filename[MAX_SOUND_NAME];
       char *path = roadmap_path_join (roadmap_path_user (), "sound");
       snprintf(sound_filename, sizeof(sound_filename), "%s\\%s\\%s.mp3", path, roadmap_prompts_get_name(), name);
       PlayFile (sound_filename);
     }
  }
  
  if ( (list->flags & SOUND_LIST_NO_FREE) == false )
  {
    free (list);
  }
  return 0;
}

void CRoadMapNativeSound::Initialize()
{
  ConstructL();  
}

void CRoadMapNativeSound::Shutdown()
{
  iState = ENotReady;
  m_AudioPlayers.ResetAndDestroy();
//  StopPlayingAll();
}

void CRoadMapNativeSound::StopPlayingAll()
{
  for ( int i = 0 ; i < m_AudioPlayers.Count() ; i++ )
  {
    CAudioPlayer* pAudioPlayer = (CAudioPlayer*)(m_AudioPlayers[i]);
    if ( pAudioPlayer != NULL )
    {
      pAudioPlayer->Stop();
    }
  }
}

int CRoadMapNativeSound::Record(const char */*file_name*/, int /*seconds*/)
{// TODO implement
  return 0;
}

void CRoadMapNativeSound::SetVolume( TInt aUserVolume, TInt aUserVolumeMin, TInt aUserVolumeMax )
{
	m_UserVolume.volume = aUserVolume;
	m_UserVolume.minVolume = aUserVolumeMin;
	m_UserVolume.maxVolume = aUserVolumeMax;
}

void CRoadMapNativeSound::SetSystemVolume( CAudioPlayer& aPlayer )
{
	TInt sysVolume;
	CMdaAudioRecorderUtility& audioUtility = const_cast<CMdaAudioRecorderUtility&>( aPlayer.GetAudioUtility() );
	TInt sysMaxVolume = audioUtility.MaxVolume();
	
	// Linear map of the user volume level to the system volume level
	// Pay attention fixed point tangent 
	sysVolume = ( ( m_UserVolume.volume - m_UserVolume.minVolume ) * sysMaxVolume ) / 
												( m_UserVolume.maxVolume - m_UserVolume.minVolume ); 
	// Set the audio device volume
	aPlayer.SetVolume( sysVolume );
	
	// Log the operation
	roadmap_log( ROADMAP_DEBUG, "Audio volume settings. User :%d. System : %d. System Max : %d", 
			m_UserVolume.volume, sysVolume, sysMaxVolume );
}
