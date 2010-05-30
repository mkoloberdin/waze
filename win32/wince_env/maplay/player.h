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


#if !defined(__PLAYER_H_INCLUDED)
#define __PLAYER_H_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "helper.h"
#include "mahelper.h"
#include "reader.h"
#include "decoder.h"
#include "effect.h"
#include "output.h"
#include "Receiver.h"
#include "libovd.h"
#include "mapplugin.h"
#include "MultiBuff.h"

#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

#define MPG_SCAN_BUFF_LEN	4
#define MPG_FILE_BUFF_LEN	2048
#define NET_READ_BUFF_LEN	4096
#define RET_EOF			0
#define RET_STOP		1
#define RET_ERROR		2

class CPlayer
{
public:
	CPlayer();
	~CPlayer();

	BOOL OpenFile(LPCTSTR pszFile);
	BOOL OpenURL(LPCTSTR pszURL);
	void Close();
	void SetMessageWindow(HWND hwndMessage);

	BOOL Play();
	void Pause();
	void Stop();
	BOOL Ff(long lSkip);
	BOOL Rew(long lSkip);
	BOOL Seek(long lTime);

	long GetCurrent();
	long GetDuration();
	MAP_STATUS GetStatus();

	void SetEqualizer(EQUALIZER* value);
	void GetEqualizer(EQUALIZER* value);
	void SetEffect(int nEffect, EFFECT* value);
	void GetEffect(int nEffect, EFFECT* value);
	void SetBassBoostLevel(int nLevel);
	int GetBassBoostLevel();
	
	void GetFileInformation(MAP_INFORMATION* pInfo);
	BOOL GetId3Tag(ID3TAGV1* pTag);
	BOOL SetId3Tag(ID3TAGV1* pTag);
	void GetOptions(MAP_OPTIONS* pOptions);
	BOOL SetOptions(MAP_OPTIONS* pOptions);
	BOOL GetScanPeek() {return m_Output.GetScanPeek();}
	void SetScanPeek(BOOL fScan) {m_Output.SetScanPeek(fScan);}
	void AudioDeviceClose();
	void GetBufferInfo(DWORD* pcbTotalAudio, DWORD* pcbBufferedAudio,
							DWORD* pcbTotalStream, DWORD* pcbBufferedStream);

	BOOL GetStreamInfo(LPTSTR pszName, LPTSTR pszGenre, LPTSTR pszURL);	
	BOOL GetStreamTitle(LPTSTR pszTitle);
	void GetStreamingOptions(MAP_STREAMING_OPTIONS* pOptions);
	BOOL SetStreamingOptions(MAP_STREAMING_OPTIONS* pOptions);
	MAP_STREAMING_STATUS GetStreamingStatus();
	int GetStreamingBufferingCount();

	DWORD GetVolume(BOOL bSysVolume);
	void SetVolume(DWORD dwVolume, BOOL bSysVolume);
	
protected:
	CReader			m_Reader;
	CReceiver		m_Receiver;
	CDecoder		m_Decoder;
	CDelayEffect	m_Reverb;
	CDelayEffect	m_Echo;
	CSurroundEffect m_Surround;
	CBassBoost		m_BassBoost;
	C3DChorus		m_3DChorus;
	COutput			m_Output;

	HWND		m_hwndMessage;
	TCHAR		m_szFile[MAX_PATH];

	DWORD		m_dwThreadID;
	HANDLE		m_hThread;

	DWORD		m_nDuration;	// Duration (samples)
	DWORD		m_nSeek;		// Last seek position (samples)
	DWORD		m_nWritten;		// Written samples from Last seeking
	BOOL m_fSuppress;

	MPEG_AUDIO_INFO			m_Info;
	MAP_STATUS				m_Status;
	MAP_STREAMING_STATUS	m_StreamingStatus;
	EQUALIZER				m_Equalizer;

	WAVEHDR*	m_pOutHdr;
	DWORD		m_cbOutBuf;
	DWORD		m_cbOutBufLeft;

	BOOL		m_fPreamp;
	int			m_nPreamp;
	int			m_nPreampRate;
	
	BOOL		m_fSeek;
	BOOL		m_fPlay;
	BOOL		m_fFileBegin;
	BOOL		m_fStop;

	enum {OPEN_NONE = 0, OPEN_MPG_FILE, OPEN_OV_FILE, OPEN_WAV_FILE, OPEN_PLUGIN, OPEN_URL} m_fOpen;

protected:
	void PostCallbackMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
	void UpdatePeek();
	void UpdateStatus(MAP_STATUS status, BOOL fError = FALSE);
	static DWORD WINAPI PlayerThreadProc(LPVOID pParam);
	void FilePlayerThread();

	BOOL PreparePlayback();
	BOOL UnpreparePlayback(BOOL fEos = FALSE, BOOL fError = FALSE);
	void OutputBuffer(WAVEHDR* pHdr, DWORD cbRecorded);
	BOOL WaitForPrebuffering(int nBuffering = 0);
	void Preamp(LPBYTE pbBuf, DWORD cbBuf); // for Ogg/Wav

private:
	MAP_OPTIONS m_Options;
	CCritSec	m_csecThread;
	CCritSec	m_csecInterface;

// for MPEG Audio
protected:
	DWORD MpgPlayerThread();
	int MpgRender(LPBYTE pbInBuf, DWORD cbInBuf, LPDWORD pcbProceed);

	BOOL MpgOpenFile(LPCTSTR pszFile);
	BOOL MpgScanFile();
	BOOL MpgSeekFile(long lTime);
	void MpgStop();
	void MpgClose();
	BOOL MpgGetId3Tag(ID3TAGV1* pTag);
	BOOL MpgScanFileNormally();
	BOOL MpgScanFileCompletely();

// for Ogg Vorbis
protected:
	HANDLE	m_hOvd;
	DWORD OvPlayerThread();

	BOOL OvOpenFile(LPCTSTR pszFile);
	BOOL OvScanFile();
	BOOL OvSeekFile(long lTime);
	void OvStop();
	void OvClose();
	BOOL OvGetId3Tag(ID3TAGV1* pTag);

public:
	BOOL OvIsValidFile(LPCTSTR pszFile);
	BOOL OvGetId3TagFile(LPCTSTR pszFile, ID3TAGV1* pTag);

// for RIFF Wave (ACM & PCM)
protected:
	HACMSTREAM		m_hAcm;
	WAVEFORMATEX*	m_pwfxSrc;
	WAVEFORMATEX*	m_pwfxDst;
	LONGLONG		m_llDataOffset;
	DWORD			m_dwDataSize;
	DWORD			m_dwCurrentSize;

	DWORD WavPlayerThread();
	int WavRender(LPBYTE pbInBuf, DWORD cbInBuf, LPDWORD pcbProceed);

	BOOL WavOpenFile(LPCTSTR pszFile);
	BOOL WavScanFile();
	BOOL WavSeekFile(long lTime);
	void WavStop();
	void WavClose();

public:
	BOOL WavIsValidFile(LPCTSTR pszFile);

// for streaming
protected:
	enum {NET_OPEN_NONE, NET_OPEN_MPEG, NET_OPEN_OV, NET_OPEN_PLUGIN}m_fNet;
	ovd_stream_buf* m_pOvd_buf;
	TCHAR m_szOvdTitle[MAX_URL];

	void NetStreamingThread();
	void NetStop();
	void NetClose();
	BOOL NetReconnect();

	BOOL NetParseMpegStream(LPBYTE pbBuf, DWORD cbBuf);
	BOOL NetParseOvStream(LPBYTE pbBuf, DWORD cbBuf);
	void NetMpegStreaming(LPBYTE pbBuf, DWORD cbBuf);
	void NetOvStreaming(LPBYTE pbBuf, DWORD cbBuf);
	void NetCheckStreamId3Tag(LPBYTE pbBuf, DWORD cbBuf);

// for plug-in
public:
	BOOL PlugInIsValidFile(LPCTSTR pszFile);
	BOOL PlugInGetId3TagFile(LPCTSTR pszFile, ID3TAGV1* pTag);
	int PlugInGetCount();
	MAP_DEC_PLUGIN* PlugInGetInterface(int nPlugIn);

protected:
	int					m_nFilePlugIn;
	int					m_nStreamingPlugIn;
	int					m_nPlugInBps;
	CMultiBuff			m_PlugInInfo;

	void PlugInLoad();
	void PlugInFree();
	void PlugInSetEqualizer();

	BOOL PlugInOpenFile(LPCTSTR pszFile);
	BOOL PlugInSeekFile(long lTime);
	DWORD PlugInPlayerThread();
	void PlugInStop();
	void PlugInClose();
	BOOL PlugInGetId3Tag(ID3TAGV1* pTag);

	BOOL PlugInParseStream(LPBYTE pbBuf, DWORD cbBuf);
	void PlugInStreaming(LPBYTE pbBuf, DWORD cbBuf);
	void PlugInStopStreaming();
};

#endif //!__PLAYER_H_INCLUDED