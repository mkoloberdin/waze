/* AudioPlayer.h
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

#ifndef __AUDIO_PLAYER_H__
#define __AUDIO_PLAYER_H__

#include <MdaAudioSampleEditor.h>

class CAudioPlayer : public CBase
{
public:

  static CAudioPlayer* NewL( const TDes& aFileName, MMdaObjectStateChangeObserver* apObserver);
  static CAudioPlayer* NewLC( const TDes& aFileName, MMdaObjectStateChangeObserver* apObserver);
  static CAudioPlayer* NewL( const TDes8& aDesc, MMdaObjectStateChangeObserver* apObserver);
  static CAudioPlayer* NewLC( const TDes8& aDesc, MMdaObjectStateChangeObserver* apObserver);

  virtual ~CAudioPlayer();

  TInt Play();
  void Stop();
  void SetVolume( TInt aVolume );
  const CMdaAudioRecorderUtility& GetAudioUtility( void ) const; 
private:
  CAudioPlayer();
  void ConstructL( const TDes& aFileName, MMdaObjectStateChangeObserver* apObserver );
  void ConstructL( const TDes8& aDesc, MMdaObjectStateChangeObserver* apObserver );

  // The audio player utility object. owned by CAudioPlayer object. 
  CMdaAudioRecorderUtility* iMdaAudioPlayerUtility;
};

#endif // __AUDIO_PLAYER_H__
