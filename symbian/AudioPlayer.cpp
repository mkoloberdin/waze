/* AudioPlayer.cpp
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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


#include "AudioPlayer.h"
#include <eikenv.h>

CAudioPlayer::CAudioPlayer() 
{
  iMdaAudioPlayerUtility = NULL;
}

CAudioPlayer* CAudioPlayer::NewL( const TDes& aFileName, MMdaObjectStateChangeObserver* apObserver)
{
    CAudioPlayer* self = NewLC( aFileName, apObserver);
    CleanupStack::Pop( self );  
    return self;
}

CAudioPlayer* CAudioPlayer::NewLC( const TDes& aFileName, MMdaObjectStateChangeObserver* apObserver)
{
    CAudioPlayer* self = new ( ELeave ) CAudioPlayer();
    CleanupStack::PushL( self );
    self->ConstructL( aFileName , apObserver );
    return self;
}

void CAudioPlayer::ConstructL( const TDes& aFileName, MMdaObjectStateChangeObserver* apObserver )
{
  iMdaAudioPlayerUtility = CMdaAudioRecorderUtility::NewL( *apObserver );
  iMdaAudioPlayerUtility->OpenFileL(aFileName);
}

CAudioPlayer* CAudioPlayer::NewL( const TDes8& aDesc, MMdaObjectStateChangeObserver* apObserver)
{
  CAudioPlayer* self = NewLC( aDesc, apObserver);
  CleanupStack::Pop( self );  
  return self;
}

CAudioPlayer* CAudioPlayer::NewLC( const TDes8& aDesc, MMdaObjectStateChangeObserver* apObserver)
{
  CAudioPlayer* self = new ( ELeave ) CAudioPlayer();
  CleanupStack::PushL( self );
  self->ConstructL( aDesc , apObserver );
  return self;
}

void CAudioPlayer::ConstructL( const TDes8& aDesc, MMdaObjectStateChangeObserver* apObserver )
{
  iMdaAudioPlayerUtility = CMdaAudioRecorderUtility::NewL( *apObserver );
  iMdaAudioPlayerUtility->OpenDesL(aDesc);
}

CAudioPlayer::~CAudioPlayer()
{
  Stop();
  if ( iMdaAudioPlayerUtility != NULL )
  {
    iMdaAudioPlayerUtility->Close();
    delete iMdaAudioPlayerUtility;    
  }
}

TInt CAudioPlayer::Play()
{
  if ( iMdaAudioPlayerUtility == NULL )
  {
    return KErrNotFound;
  }
  iMdaAudioPlayerUtility->PlayL();
  return KErrNone;
}

void CAudioPlayer::Stop()
{
  if ( iMdaAudioPlayerUtility != NULL )
  {
    iMdaAudioPlayerUtility->Stop();
  }
}

void CAudioPlayer::SetVolume( TInt aVolume )
{
  if ( iMdaAudioPlayerUtility != NULL )
  {
    	iMdaAudioPlayerUtility->SetVolume( aVolume );
  }
  
}

const CMdaAudioRecorderUtility& CAudioPlayer::GetAudioUtility() const 
{ 
	return *iMdaAudioPlayerUtility;
};
