#include <windows.h>
#include <math.h>
#include "maplay.h"
#include "helper.h"
#include "player.h"
#include "libovd.h"

#define INITIAL_THREAD_PRIORITY		THREAD_PRIORITY_BELOW_NORMAL

CPlayer::CPlayer() : 
m_Echo(TRUE), m_Reverb(FALSE)
{
	m_hwndMessage = NULL;
	m_szFile[0] = NULL;

	m_dwThreadID = 0;
	m_hThread = NULL;

	m_nDuration = 0;
	m_nSeek = 0;
	m_nWritten = 0;
	m_fSuppress = FALSE;
	m_Status = MAP_STATUS_STOP;
	m_StreamingStatus = MAP_STREAMING_DISABLED;
	memset(&m_Info, 0, sizeof(MPEG_AUDIO_INFO));

	m_fOpen = OPEN_NONE;
	m_fStop = FALSE;
	
	m_Options.nThreadPriority = THREAD_PRIORITY_ABOVE_NORMAL;
	m_Output.GetOutputParam((LPDWORD)&m_Options.nOutputBufferLen,
							&m_Options.fAlwaysOpenDevice, &m_Options.fFadeIn);
	//m_Options.fAlwaysOpenDevice = TRUE;
	m_Options.nOutputPrebuffer = 50;
	m_Options.fScanMpegCompletely = FALSE;
	m_Options.fFadeIn = FALSE;
	m_Options.fSuppressZeroSamples = FALSE;

	m_pOutHdr = NULL;
	m_cbOutBuf = 0;
	m_cbOutBufLeft = 0;
	m_fPlay = FALSE;
	m_fSeek = FALSE;
	m_fFileBegin = TRUE;

	m_Decoder.GetEqualizer(&m_Equalizer);
	m_fPreamp = FALSE;
	m_nPreamp = 31;
	m_nPreampRate = PREAMP_FIXED_FLAT;

	m_hOvd = NULL;
	m_pOvd_buf = NULL;
	m_fNet = NET_OPEN_NONE;
	m_szOvdTitle[0] = NULL;

	m_hAcm = NULL;
	m_pwfxSrc = NULL;
	m_pwfxDst = NULL;
	m_llDataOffset = 0;
	m_dwDataSize = 0;
	m_dwCurrentSize = 0;

	PlugInLoad();
}

CPlayer::~CPlayer()
{
	Close();
	m_Output.CloseAll();
	PlugInFree();
}

BOOL CPlayer::OpenFile(LPCTSTR pszFile)
{
	CAutoLock lock(&m_csecInterface);
	Close();

	m_fFileBegin = TRUE;

	// PlugIn
	if (PlugInOpenFile(pszFile))
		return TRUE;

	// MPEG Audio
	if (MpgOpenFile(pszFile))
		return TRUE;
	
	// Ogg Vorbis
	if (OvOpenFile(pszFile))
		return TRUE;
#ifndef EMBEDDED_CE
	// Wave
	if (WavOpenFile(pszFile))
		return TRUE;
#endif
	return FALSE;
}

BOOL CPlayer::OpenURL(LPCTSTR pszURL)
{
	CAutoLock lock(&m_csecInterface);
	Close();

	if (!m_Receiver.Open(pszURL))
		return FALSE;

	m_fOpen = OPEN_URL;
	m_StreamingStatus = MAP_STREAMING_DISCONNECTED;
	return TRUE;
}

void CPlayer::Close()
{
	CAutoLock lock(&m_csecInterface);
	Stop();
	
	PlugInClose();
	MpgClose();
	OvClose();
#ifndef EMBEDDED_CE
	WavClose();
#endif
	NetClose();

	memset(&m_Info, 0, sizeof(m_Info));
	m_szFile[0] = NULL;
	m_nDuration = 0;

	m_fOpen = OPEN_NONE;
	m_StreamingStatus = MAP_STREAMING_DISABLED;
}

BOOL CPlayer::Play()
{
	CAutoLock lock(&m_csecInterface);
	if (m_fOpen == OPEN_NONE)
		return FALSE;

	if (m_Status == MAP_STATUS_PAUSE) {
		Pause();
		return TRUE;
	}
	if (m_Status == MAP_STATUS_STOP) {
		//m_hThread = CreateThread(NULL, 0, PlayerThreadProc, this, 0, &m_dwThreadID);
		PlayerThreadProc(this);
		return TRUE;
	}
	return FALSE;
}

void CPlayer::Pause()
{
	CAutoLock lock(&m_csecInterface);
	if (m_fOpen == OPEN_NONE)
		return;

	if (m_fOpen == OPEN_URL)
		return;

	if (m_Status == MAP_STATUS_PLAY) {
		UpdateStatus(MAP_STATUS_PAUSE);
		m_Output.Pause(TRUE);
		m_fPlay = FALSE;
	}
	else if (m_Status == MAP_STATUS_PAUSE) {
		if (m_Output.GetBufferingCount() == m_Output.GetBufferCount())
			m_Output.Pause(FALSE);
		else
			m_fPlay = TRUE;
		UpdateStatus(MAP_STATUS_PLAY);
	}
}

void CPlayer::Stop()
{
	CAutoLock lock(&m_csecInterface);
	
	if (m_hThread) {
		m_fStop = TRUE;
		m_Receiver.Stop();
		m_Output.Reset();
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	m_nSeek = 0;
	m_nWritten = 0;
	m_fSuppress = FALSE;
	m_fPlay = FALSE;
	m_fStop = FALSE;
	m_fSeek = FALSE;
	Seek(0);
}


BOOL CPlayer::Ff(long lSkip)
{
	CAutoLock lock(&m_csecInterface);
	if (m_fOpen == OPEN_NONE)
		return FALSE;

	LONG lTime = GetCurrent();
	lTime += lSkip;
	return Seek(lTime);
}

BOOL CPlayer::Rew(long lSkip)
{
	CAutoLock lock(&m_csecInterface);
	if (m_fOpen == OPEN_NONE)
		return FALSE;

	LONG lTime = GetCurrent();
	lTime -= lSkip;
	return Seek(lTime);
}

BOOL CPlayer::Seek(long lTime)
{
	CAutoLock lock(&m_csecInterface);
	CAutoLock lock2(&m_csecThread);
	if (m_fOpen == OPEN_NONE)
		return FALSE;
	
	// 時間のチェック
	if (lTime < 0)
		lTime = 0;
	if (lTime >= int((double)m_nDuration / m_Info.nSamplingRate * 1000))
		return FALSE;

	BOOL fRet = FALSE;
	if (m_fOpen == OPEN_PLUGIN)
		fRet = PlugInSeekFile(lTime);
	else if (m_fOpen == OPEN_MPG_FILE)
		fRet = MpgSeekFile(lTime);
	else if (m_fOpen == OPEN_OV_FILE)
		fRet = OvSeekFile(lTime);
#ifndef EMBEDDED_CE
	else if (m_fOpen == OPEN_WAV_FILE)
		fRet = WavSeekFile(lTime);
#endif
	m_fFileBegin = m_nSeek == 0;
	if (fRet) {
		m_nWritten = 0;
		m_fSuppress = FALSE;

		SetThreadPriority(m_hThread, INITIAL_THREAD_PRIORITY);
	}

	return fRet;
}

void CPlayer::SetMessageWindow(HWND hwndMessage)
{
	m_hwndMessage = hwndMessage;
	m_Receiver.SetMessageWindow(hwndMessage);
}

long CPlayer::GetCurrent()
{
	CAutoLock lock(&m_csecInterface);
	if (m_fOpen == OPEN_NONE)
		return 0;

	if (GetStatus() == MAP_STATUS_STOP)
		return (int)(((double)m_nSeek) * 1000 / m_Info.nSamplingRate);
	else
		return (int)(((double)m_Output.GetCurrent() + m_nSeek) * 1000 / m_Info.nSamplingRate);
}

long CPlayer::GetDuration()
{
	if (m_fOpen == OPEN_NONE)
		return 0;

	return (int)((double)m_nDuration / m_Info.nSamplingRate * 1000);
}

MAP_STATUS CPlayer::GetStatus()
{
	return m_Status;
}

void CPlayer::SetEqualizer(EQUALIZER* value)
{
	m_Equalizer = *value;
	m_Decoder.SetEqualizer(&m_Equalizer);
	
	m_fPreamp = value->fEnable;
	m_nPreamp = value->preamp;
	double dLevel = -((double)value->preamp - 31) * 20 / 31;
	m_nPreampRate = (int)(pow(10, dLevel / 20) * PREAMP_FIXED_FLAT);

	PlugInSetEqualizer();
}

void CPlayer::GetEqualizer(EQUALIZER* value)
{
	*value = m_Equalizer;
}

void CPlayer::SetEffect(int nEffect, EFFECT* value)
{
	switch (nEffect) {
	case EFFECT_REVERB:
		m_Reverb.SetParameter(value);
		break;
	case EFFECT_ECHO:
		m_Echo.SetParameter(value);
		break;
	case EFFECT_SURROUND:
		m_Surround.SetParameter(value);
		break;
	case EFFECT_3DCHORUS:
		m_3DChorus.SetParameter(value);
		break;
	}
}

void CPlayer::GetEffect(int nEffect, EFFECT* value)
{
	switch (nEffect) {
	case EFFECT_REVERB:
		m_Reverb.GetParameter(value);
		break;
	case EFFECT_ECHO:
		m_Echo.GetParameter(value);
		break;
	case EFFECT_SURROUND:
		m_Surround.GetParameter(value);
		break;
	case EFFECT_3DCHORUS:
		m_3DChorus.GetParameter(value);
		break;
	}
}

void CPlayer::SetBassBoostLevel(int nLevel)
{
	m_BassBoost.SetLevel(nLevel);
}

int CPlayer::GetBassBoostLevel()
{
	return m_BassBoost.GetLevel();
}

void CPlayer::GetFileInformation(MAP_INFORMATION* pInfo)
{
	if (m_fOpen == OPEN_NONE)
		return;
	pInfo->nBitRate = m_Info.nBitRate;
	pInfo->nChannels = m_Info.nChannels;
	pInfo->nLayer = m_Info.nLayer;
	pInfo->nSamplingRate = m_Info.nSamplingRate;
	pInfo->nVersion = m_Info.nVersion;
	pInfo->nDuration = (int)((double)m_nDuration / m_Info.nSamplingRate * 1000);
}

BOOL CPlayer::GetId3Tag(ID3TAGV1* pTag)
{
	BOOL bRet;
	if (m_fOpen == OPEN_MPG_FILE)
		bRet = MpgGetId3Tag(pTag);
	else if (m_fOpen == OPEN_OV_FILE)
		bRet = OvGetId3Tag(pTag);
	else if (m_fOpen == OPEN_PLUGIN)
		bRet = PlugInGetId3Tag(pTag);

	if (!bRet) 
		bRet = ::GetId3Tag(m_szFile, pTag);

	return bRet;
}

BOOL CPlayer::SetId3Tag(ID3TAGV1* pTag)
{
	return FALSE; // not support
}

BOOL CPlayer::GetStreamInfo(LPTSTR pszName, LPTSTR pszGenre, LPTSTR pszURL)
{
	return m_Receiver.GetStreamInfo(pszName, pszGenre, pszURL);
}

BOOL CPlayer::GetStreamTitle(LPTSTR pszTitle)
{
	if (_tcslen(m_szOvdTitle)) {
		_tcscpy(pszTitle, m_szOvdTitle);
		return TRUE;
	}

	return m_Receiver.GetStreamTitle(pszTitle);
}

void CPlayer::GetOptions(MAP_OPTIONS* pOptions)
{
	*pOptions = m_Options;
}

BOOL CPlayer::SetOptions(MAP_OPTIONS* pOptions)
{
	if (m_Status != MAP_STATUS_STOP)
		return FALSE;

	if (m_Options.nOutputPrebuffer < 0 || m_Options.nOutputPrebuffer > 100)
		return FALSE;

	if (pOptions->nOutputBufferLen == m_Options.nOutputBufferLen &&
		pOptions->fAlwaysOpenDevice == m_Options.fAlwaysOpenDevice &&
		pOptions->fFadeIn == m_Options.fFadeIn) {
		m_Options = *pOptions;
		return TRUE;
	}

	m_Output.CloseAll();
	if (!m_Output.SetOutputParam(pOptions->nOutputBufferLen, 
							pOptions->fAlwaysOpenDevice, pOptions->fFadeIn))
		return FALSE;

	m_Options = *pOptions;
	return TRUE;
}

void CPlayer::GetStreamingOptions(MAP_STREAMING_OPTIONS* pOptions)
{
	pOptions->nBuf = m_Receiver.GetBufferCount();
	pOptions->nPreBuf = m_Receiver.GetPrebufferingCount();
	pOptions->fUseProxy = m_Receiver.GetProxy(pOptions->szProxy);
	m_Receiver.GetUserAgent(pOptions->szUserAgent);
}

BOOL CPlayer::SetStreamingOptions(MAP_STREAMING_OPTIONS* pOptions)
{
	if (!m_Receiver.SetBufferCount(pOptions->nBuf))
		return FALSE;

	int nPreBuf = pOptions->nPreBuf;
	if (nPreBuf > pOptions->nBuf)
		nPreBuf = pOptions->nBuf;
	m_Receiver.SetPrebufferingCount(nPreBuf);
	m_Receiver.SetProxy(pOptions->fUseProxy, pOptions->szProxy);
	m_Receiver.SetUserAgent(pOptions->szUserAgent);

	return TRUE;
}

MAP_STREAMING_STATUS CPlayer::GetStreamingStatus()
{
	return m_StreamingStatus;
}

int CPlayer::GetStreamingBufferingCount()
{
	if (m_StreamingStatus < MAP_STREAMING_CONNECTING)
		return 0;

	return m_Receiver.GetBufferingCount();
}

// protected members

void CPlayer::PostCallbackMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PostMessage(m_hwndMessage, uMsg, wParam, lParam);
}

void CPlayer::UpdateStatus(MAP_STATUS status, BOOL fError)
{
	if (m_Status != status) {
		m_Status = status;
		PostCallbackMessage(MAP_MSG_STATUS, status, fError);
	}
}

void CPlayer::UpdatePeek()
{
	PostCallbackMessage(MAP_MSG_PEEK, m_Output.m_nLPeek, m_Output.m_nRPeek);
}

BOOL CPlayer::PreparePlayback()
{
	if (m_fStop)
		return FALSE;

	CAutoLock lock(&m_csecInterface);

	int nBitsPerSample;
	switch (m_fOpen) {
#ifndef EMBEDDED_CE
	case OPEN_WAV_FILE:
		nBitsPerSample = m_pwfxDst->wBitsPerSample; break;
#endif
	case OPEN_PLUGIN:
		nBitsPerSample = m_nPlugInBps; break;
	default:
		nBitsPerSample = 16;
	}

	if (!m_Output.Open(m_Info.nChannels, m_Info.nSamplingRate, nBitsPerSample))
		return FALSE;

	m_cbOutBuf = m_Output.GetBufferSize();
	m_Output.Pause(TRUE);

	m_Decoder.Init();
	m_BassBoost.Open(m_Info.nChannels, m_Info.nSamplingRate);
	m_Echo.Open(m_Info.nChannels, m_Info.nSamplingRate);
	m_Reverb.Open(m_Info.nChannels, m_Info.nSamplingRate);

	if (m_Info.nChannels == 2)
		m_3DChorus.Open(m_Info.nSamplingRate);

	m_fPlay = TRUE;
	m_fStop = FALSE;
	SetThreadPriority(m_hThread, INITIAL_THREAD_PRIORITY);

	return TRUE;
}

BOOL CPlayer::UnpreparePlayback(BOOL fEos, BOOL fError)
{
	if (fEos) {
		if (m_pOutHdr) {
			OutputBuffer(m_pOutHdr, m_cbOutBuf - m_cbOutBufLeft);
			m_pOutHdr = NULL;
			m_cbOutBufLeft = 0;
		}
		while (!m_Output.IsFlushed()) {
			if (m_fPlay && GetStatus() == MAP_STATUS_PLAY) {
				m_Output.Pause(FALSE);
				m_fPlay = FALSE;
			}
			Sleep(1);
			if (m_fSeek)
				return FALSE;
			if (m_fStop)
				break;
			UpdatePeek();
		}
	}

	m_Output.Close();
	m_Echo.Close();
	m_Reverb.Close();
	m_BassBoost.Close();
	m_3DChorus.Close();
	m_Decoder.Destroy();

	if (!m_Options.fAlwaysOpenDevice || fError)
		m_Output.CloseAll();

	if (m_fOpen == OPEN_URL) {
		m_Receiver.Disconnect();
		m_StreamingStatus = MAP_STREAMING_DISCONNECTED;
	}

	m_pOutHdr = NULL;
	m_cbOutBufLeft = 0;

	m_nSeek = 0;
	m_fPlay = FALSE;
	m_fStop = FALSE;
	m_fSeek = FALSE;

	switch (m_fOpen) {
	case OPEN_PLUGIN:
		PlugInStop(); break;
	case OPEN_MPG_FILE:
		MpgStop(); break;
	case OPEN_OV_FILE:
		OvStop(); break;
#ifndef EMBEDDED_CE
	case OPEN_WAV_FILE:
		WavStop(); break;
#endif
	case OPEN_URL:
		NetStop(); break;
	}

	PostMessage(m_hwndMessage, MAP_MSG_PEEK, 0, 0);
	UpdateStatus(MAP_STATUS_STOP, fError);
	return TRUE;
}

void CPlayer::OutputBuffer(WAVEHDR* pHdr, DWORD cbRecorded)
{
	int nBitsPerSample;
	switch (m_fOpen) {
#ifndef EMBEDDED_CE
	case OPEN_WAV_FILE:
		nBitsPerSample = m_pwfxDst->wBitsPerSample; break;
#endif
	case OPEN_PLUGIN:
		nBitsPerSample = m_nPlugInBps; break;
	default:
		nBitsPerSample = 16;
	}

	// 先頭の無音サンプルの除去
	if (m_fOpen != OPEN_URL) {
		if (m_fFileBegin) {
			m_Output.SetFadeOff();
			if (m_Options.fSuppressZeroSamples) {
				DWORD cb = m_Output.ScanZeroSamples(TRUE, (BYTE*)pHdr->lpData, cbRecorded);
				if (cb == cbRecorded) {
					pHdr->dwBytesRecorded = 0;
					m_Output.OutputBuffer(pHdr);
					m_nSeek += cb / (m_Info.nChannels * (nBitsPerSample / 8));
#ifdef _WIN32_WCE
					Sleep(1);
#endif
					return;
				}
				else {
					cbRecorded -= cb;
					memmove(pHdr->lpData, pHdr->lpData + cb, cbRecorded);
					m_nSeek += cb / (m_Info.nChannels * (nBitsPerSample / 8));
				}
			}
			m_fFileBegin = FALSE;
		}
		else {
			// ファイル末尾の無音の除去
			if (m_Options.fSuppressZeroSamples) {
				if (m_fSuppress) {
					pHdr->dwBytesRecorded = 0;
					m_Output.OutputBuffer(pHdr);
					return;
				}

				if (m_nWritten + m_nSeek >= m_nDuration - (m_Info.nSamplingRate * 10)) { // 末端10秒に到達したか？
					DWORD cb = m_Output.ScanZeroSamples(FALSE, (BYTE*)pHdr->lpData, cbRecorded);
					if ((int)cb > m_Info.nChannels * m_Info.nSamplingRate * (nBitsPerSample / 8) / 100) { // 10ミリ秒以上無音があるか？
						cbRecorded -= cb;
						m_fSuppress = TRUE;
					}
				}
			}
		}
	}
	
	pHdr->dwBufferLength = pHdr->dwBytesRecorded = cbRecorded;

	// エフェクトの処理
	short* p = (short*)pHdr->lpData;
	if (m_Info.nChannels == 2) {
		m_3DChorus.Process(nBitsPerSample, (LPBYTE)p, cbRecorded);
		m_Surround.Process(nBitsPerSample, p, cbRecorded);
	}

	m_BassBoost.Process(nBitsPerSample, p, cbRecorded);
	m_Echo.Process(nBitsPerSample, p, cbRecorded);
	m_Reverb.Process(nBitsPerSample, p, cbRecorded);

	// ピークメータを更新
	UpdatePeek();

	// 出力
	m_Output.OutputBuffer(pHdr);
	m_nWritten += pHdr->dwBytesRecorded / (nBitsPerSample / 8 * m_Info.nChannels);

	// 再生開始通知
	if (m_fPlay && GetStatus() == MAP_STATUS_PLAY) {
		if ((int)m_Output.GetBufferingSamples() >= 
			((m_Info.nSamplingRate * m_Options.nOutputBufferLen) / 1000) * m_Options.nOutputPrebuffer / 100) {
			m_Output.Pause(FALSE);
			m_fPlay = FALSE;

			::SetThreadPriority(m_hThread, m_Options.nThreadPriority);
		}
		else if (m_Output.GetBufferingCount() == m_Output.GetBufferCount()) {
			m_Output.Pause(FALSE);
			m_fPlay = FALSE;

			::SetThreadPriority(m_hThread, m_Options.nThreadPriority);
		}
	}
}

void CPlayer::AudioDeviceClose()
{
	if (m_Status != MAP_STATUS_STOP)
		Stop();

	m_Output.CloseAll();
}

void CPlayer::GetBufferInfo(DWORD* pcbTotalAudio, DWORD* pcbBufferedAudio,
							DWORD* pcbTotalStream, DWORD* pcbBufferedStream)
{
	m_Output.GetBufferInfo(pcbTotalAudio, pcbBufferedAudio);
	m_Receiver.GetBufferInfo(pcbTotalStream, pcbBufferedStream);
}

BOOL CPlayer::WaitForPrebuffering(int nBuffering)
{
	if (!nBuffering)
		nBuffering = m_Receiver.GetPrebufferingCount();

	while (TRUE) {
		if (m_fStop)
			return FALSE;
		if (!m_Receiver.IsConnected())
			return m_Receiver.IsEos() ? TRUE : FALSE;

		if (m_Receiver.GetBufferingCount() >= nBuffering)
			return TRUE;

		Sleep(1);
	}
}

void CPlayer::Preamp(LPBYTE pbBuf, DWORD cbBuf)
{
	if (m_fPreamp)
		m_Output.Preamp(pbBuf, cbBuf, m_nPreampRate);
}

DWORD WINAPI CPlayer::PlayerThreadProc(LPVOID pParam)
{
	CPlayer* pPlayer = (CPlayer*)pParam;

	if (pPlayer->m_fOpen == OPEN_URL)
		pPlayer->NetStreamingThread();
	else
		pPlayer->FilePlayerThread();

	return 0;
}

void CPlayer::FilePlayerThread()
{
	int nRet = RET_ERROR;

	UpdateStatus(MAP_STATUS_WAIT);
	if (!PreparePlayback()) {
		UnpreparePlayback(FALSE, TRUE);
		return;
	}
	UpdateStatus(MAP_STATUS_PLAY);

retry:
	switch (m_fOpen) {
	case OPEN_PLUGIN:
		nRet = PlugInPlayerThread(); break;
	case OPEN_MPG_FILE:
		nRet = MpgPlayerThread(); break;
	case OPEN_OV_FILE:
		nRet = OvPlayerThread(); break;
#ifndef EMBEDDED_CE
	case OPEN_WAV_FILE:
		nRet = WavPlayerThread(); break;
#endif
	}

	switch (nRet) {
	case RET_EOF:
		if (!UnpreparePlayback(TRUE))
			goto retry;
		break;
	case RET_STOP:
		UnpreparePlayback();
		break;
	case RET_ERROR:
		UnpreparePlayback(FALSE, TRUE);
		break;
	}
}

DWORD CPlayer::GetVolume(BOOL bSysVolume)
{
	DWORD dwVolume = 0;
	if (bSysVolume) {
		waveOutGetVolume(NULL, &dwVolume);
		return dwVolume;
	}
	else {
		return m_Output.GetVolume();
	}
}

void CPlayer::SetVolume(DWORD dwVolume, BOOL bSysVolume)
{
	if (bSysVolume) {
		waveOutSetVolume(NULL, dwVolume);
	}
	else {
		m_Output.SetVolume(dwVolume);
	}
}
