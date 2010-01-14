	/*
 *  GSPlayer - The audio player for WindowsCE
 *  Copyright (C) 2003  Y.Nagamidori
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#if !defined(__MAPLAY_H__)
#define __MAPLAY_H__

#include <windows.h>
#include <tchar.h>
#include "mapplugin.h"

typedef struct
{
	BOOL fEnable;
	int preamp;	  //set value 64 to 0 (default 31) updated 2.18
	int data[10]; //set value 64 to 0 (default 31) updated 2.18
}EQUALIZER;

#define EFFECT_REVERB		0
#define EFFECT_ECHO			1
#define EFFECT_SURROUND		2
#define EFFECT_3DCHORUS		3

typedef struct 
{
	BOOL fEnable;
	int nDelay;
	int nRate;
}EFFECT;

// nDelay
// EFFECT_REVERB: 0 - infinite
// EFFECT_ECHO: 0 - infinite
// EFFECT_SURROUND: ignore
// EFFECT_3DCHORUS: ignore
// nRate
// EFFECT_REVERB: 0 - infinite
// EFFECT_ECHO: 0 - infinite
// EFFECT_SURROUND: 0 - 100
// EFFECT_3DCHORUS: 0 - 100 (updated 2.16)

typedef struct
{
	int			nVersion;
	int			nLayer;
	int			nChannels;
	int			nSamplingRate;
	int			nBitRate;
	int			nDuration;
}MAP_INFORMATION;

typedef struct
{
	int			nThreadPriority;
	int			nOutputBufferLen;		// buffer length (20 - 5000 ms)
	int			nOutputPrebuffer;		// prebuffer (0 - 100 percent)
	BOOL		fScanMpegCompletely;
	BOOL		fFadeIn;
	BOOL		fSuppressZeroSamples;
	BOOL		fAlwaysOpenDevice;
}MAP_OPTIONS;

#define MAX_URL		MAX_PATH
typedef struct
{
	int nBuf;	// 1 buffer = 2048 bytes
	int nPreBuf;
	BOOL fUseProxy;
	TCHAR szProxy[MAX_URL];
	TCHAR szUserAgent[MAX_URL];
}MAP_STREAMING_OPTIONS;

#define MAX_TAG_STR		255
typedef struct tID3Tag
{
	TCHAR szTrack[MAX_TAG_STR];
	TCHAR szArtist[MAX_TAG_STR];
	TCHAR szAlbum[MAX_TAG_STR];
	TCHAR szComment[MAX_TAG_STR];
	TCHAR szGenre[MAX_TAG_STR];
	int nYear;
	int nTrackNum;
}ID3TAGV1;

#ifndef MAXLONGLONG
#define MAXLONGLONG                      (0x7fffffffffffffff)
#endif

#define MAX_WAVEOUTVOLUME	0xFFFFFFFF

#define MAP_MSG_BASE		(WM_USER + 10000)
#define MAP_MSG_STATUS		(MAP_MSG_BASE + 1) // wParam == MAP_STATUS, lParam = error (stop only)
#define MAP_MSG_PEEK		(MAP_MSG_BASE + 2) // wParam == l lParam == r
#define MAP_MSG_STREAM				(MAP_MSG_BASE + 10000)
#define MAP_MSG_STREAM_TITLE		(MAP_MSG_STREAM + 1) // wParam = Stream Title (TCHAR[MAX_URL])

enum MAP_STATUS{MAP_STATUS_STOP, MAP_STATUS_PLAY, MAP_STATUS_PAUSE, MAP_STATUS_WAIT};
enum MAP_STREAMING_STATUS{
	MAP_STREAMING_DISABLED, MAP_STREAMING_DISCONNECTED, MAP_STREAMING_CONNECTING, 
	MAP_STREAMING_BUFFERING, MAP_STREAMING_CONNECTED
};

#ifdef MAPLAY_EXPORTS
#define MAPLIBEXPORT(ret) __declspec(dllexport) ret WINAPI
#else
#define MAPLIBEXPORT(ret) ret WINAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

MAPLIBEXPORT(HANDLE) MAP_Initialize();
MAPLIBEXPORT(void) MAP_Uninitialize(HANDLE hLib);
MAPLIBEXPORT(BOOL) MAP_Open(HANDLE hLib, LPCTSTR pszFile);
MAPLIBEXPORT(void) MAP_Close(HANDLE hLib);
MAPLIBEXPORT(BOOL) MAP_Play(HANDLE hLib);
MAPLIBEXPORT(void) MAP_Stop(HANDLE hLib);
MAPLIBEXPORT(void) MAP_Pause(HANDLE hLib);
MAPLIBEXPORT(BOOL) MAP_Ff(HANDLE hLib, long lSkip);
MAPLIBEXPORT(BOOL) MAP_Rew(HANDLE hLib, long lSkip);
MAPLIBEXPORT(BOOL) MAP_Seek(HANDLE hLib, long lTime);
MAPLIBEXPORT(long) MAP_GetDuration(HANDLE hLib);
MAPLIBEXPORT(BOOL) MAP_IsValidStream(HANDLE hLib, LPCTSTR pszFile);
MAPLIBEXPORT(void) MAP_SetMessageWindow(HANDLE hLib, HWND hwndMessage);
MAPLIBEXPORT(long) MAP_GetCurrent(HANDLE hLib); /* ms */
MAPLIBEXPORT(void) MAP_SetEqualizer(HANDLE hLib, EQUALIZER* value);
MAPLIBEXPORT(void) MAP_GetEqualizer(HANDLE hLib, EQUALIZER* value);
MAPLIBEXPORT(void) MAP_SetEffect(HANDLE hLib, int nEffect, EFFECT* value);
MAPLIBEXPORT(void) MAP_GetEffect(HANDLE hLib, int nEffect, EFFECT* value);
MAPLIBEXPORT(void) MAP_SetBassBoostLevel(HANDLE hLib, int nLevel);
MAPLIBEXPORT(int) MAP_GetBassBoostLevel(HANDLE hLib);
MAPLIBEXPORT(void) MAP_GetFileInfo(HANDLE hLib, MAP_INFORMATION* pInfo);
MAPLIBEXPORT(BOOL) MAP_GetId3Tag(HANDLE hLib, ID3TAGV1* pTag);
MAPLIBEXPORT(BOOL) MAP_SetId3Tag(HANDLE hLib, ID3TAGV1* pTag);
MAPLIBEXPORT(BOOL) MAP_GetId3TagFile(HANDLE hLib, LPCTSTR pszFile, ID3TAGV1* pTag);
MAPLIBEXPORT(BOOL) MAP_SetId3TagFile(HANDLE hLib, LPCTSTR pszFile, ID3TAGV1* pTag);
MAPLIBEXPORT(void) MAP_GetGenreString(HANDLE hLib, int nGenre, LPTSTR pszGenre);
MAPLIBEXPORT(MAP_STATUS) MAP_GetStatus(HANDLE hLib);
MAPLIBEXPORT(BOOL) MAP_SetOptions(HANDLE hLib, MAP_OPTIONS* pOptions);
MAPLIBEXPORT(void) MAP_GetOptions(HANDLE hLib, MAP_OPTIONS* pOptions);
MAPLIBEXPORT(BOOL) MAP_GetScanPeek(HANDLE hLib);
MAPLIBEXPORT(void) MAP_SetScanPeek(HANDLE hLib, BOOL fScan);
MAPLIBEXPORT(void) MAP_AudioDeviceClose(HANDLE hLib);

MAPLIBEXPORT(void) MAP_GetBufferInfo(HANDLE hLib, DWORD* pcbTotalAudio, DWORD* pcbBufferedAudio,
									 DWORD* pcbTotalStream, DWORD* pcbBufferedStream);

// for streaming
MAPLIBEXPORT(BOOL) MAP_OpenURL(HANDLE hLib, LPCTSTR pszURL);
MAPLIBEXPORT(BOOL) MAP_GetStreamInfo(HANDLE hLib, LPTSTR pszName, LPTSTR pszGenre, LPTSTR pszURL);
MAPLIBEXPORT(BOOL) MAP_GetStreamTitle(HANDLE hLib, LPTSTR pszTitle);
MAPLIBEXPORT(BOOL) MAP_SetStreamingOptions(HANDLE hLib, MAP_STREAMING_OPTIONS* pOptions);
MAPLIBEXPORT(void) MAP_GetStreamingOptions(HANDLE hLib, MAP_STREAMING_OPTIONS* pOptions);
MAPLIBEXPORT(MAP_STREAMING_STATUS) MAP_GetStreamingStatus(HANDLE hLib);
MAPLIBEXPORT(int) MAP_GetStreamingBufferingCount(HANDLE hLib);

// plug-in
MAPLIBEXPORT(int) MAP_GetDecoderPlugInCount(HANDLE hLib);
MAPLIBEXPORT(MAP_DEC_PLUGIN*) MAP_GetDecoderPlugIn(HANDLE hLib, int nIndex);

// volume
MAPLIBEXPORT(DWORD) MAP_GetVolume(HANDLE hLib, BOOL bSysVolume);
MAPLIBEXPORT(void) MAP_SetVolume(HANDLE hLib, DWORD dwVolume, BOOL bSysVolume);

#ifdef __cplusplus
};
#endif

#endif __MAPLAY_H__
