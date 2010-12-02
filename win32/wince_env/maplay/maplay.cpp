#include "maplay.h"
#include "player.h"

#if defined(MAPLAY_EXPORTS) && !defined(_WIN32_WCE)
BOOL __declspec(dllexport) APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}
#endif

HANDLE WINAPI MAP_Initialize()
{
	CPlayer* pPlayer = new CPlayer();
	return pPlayer;
}

void WINAPI MAP_Uninitialize(HANDLE hLib)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	delete pPlayer;
}

BOOL WINAPI MAP_Open(HANDLE hLib, LPCTSTR pszFile)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->OpenFile(pszFile);
}

void WINAPI MAP_Close(HANDLE hLib)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->Close();
}

BOOL WINAPI MAP_Play(HANDLE hLib)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->Play();
}

void WINAPI MAP_Stop(HANDLE hLib)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->Stop();
}

void WINAPI MAP_Pause(HANDLE hLib)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->Pause();
}

BOOL WINAPI MAP_Ff(HANDLE hLib, long lSkip)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->Ff(lSkip);
}

BOOL WINAPI MAP_Rew(HANDLE hLib, long lSkip)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->Rew(lSkip);
}

BOOL WINAPI MAP_Seek(HANDLE hLib, long lTime)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->Seek(lTime);
}

long WINAPI MAP_GetDuration(HANDLE hLib)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->GetDuration();
}

BOOL WINAPI MAP_IsValidStream(HANDLE hLib, LPCTSTR pszFile)
{
	CPlayer* pPlayer = (CPlayer*) hLib;

	if (IsValidFile(pszFile))
		return TRUE;

	if (pPlayer->OvIsValidFile(pszFile))
		return TRUE;
#ifndef EMBEDDED_CE
	if (pPlayer->WavIsValidFile(pszFile))
		return TRUE;
#endif
	if (pPlayer->PlugInIsValidFile(pszFile))
		return TRUE;

	return FALSE;
}

void WINAPI MAP_SetMessageWindow(HANDLE hLib, HWND hwndMessage)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->SetMessageWindow(hwndMessage);
}

long WINAPI MAP_GetCurrent(HANDLE hLib)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->GetCurrent();
}

void WINAPI MAP_SetEqualizer(HANDLE hLib, EQUALIZER* value)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->SetEqualizer(value);
}
void WINAPI MAP_GetEqualizer(HANDLE hLib, EQUALIZER* value)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->GetEqualizer(value);
}

void WINAPI MAP_SetEffect(HANDLE hLib, int nEffect, EFFECT* value)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->SetEffect(nEffect, value);
}

void WINAPI MAP_GetEffect(HANDLE hLib, int nEffect, EFFECT* value)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->GetEffect(nEffect, value);
}

void WINAPI MAP_SetBassBoostLevel(HANDLE hLib, int nLevel)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->SetBassBoostLevel(nLevel);
}

int WINAPI MAP_GetBassBoostLevel(HANDLE hLib)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->GetBassBoostLevel();
}

void WINAPI MAP_GetFileInfo(HANDLE hLib, MAP_INFORMATION* pInfo)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->GetFileInformation(pInfo);
}

BOOL WINAPI MAP_GetId3Tag(HANDLE hLib, ID3TAGV1* pTag)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->GetId3Tag(pTag);
}

BOOL WINAPI MAP_SetId3Tag(HANDLE hLib, ID3TAGV1* pTag)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->SetId3Tag(pTag);
}

void WINAPI MAP_GetGenreString(HANDLE hLib, int nGenre, LPTSTR pszGenre)
{
	if (nGenre >= 0 && nGenre <= 147)
		_tcscpy(pszGenre, genre_strings[nGenre]);
}

BOOL WINAPI MAP_GetId3TagFile(HANDLE hLib, LPCTSTR pszFile, ID3TAGV1* pTag)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	if (pPlayer->PlugInGetId3TagFile(pszFile, pTag))
		return TRUE;

	if (pPlayer->OvGetId3TagFile(pszFile, pTag))
		return TRUE;

	if (!IsValidFile(pszFile))
		return FALSE;

	return GetId3Tag(pszFile, pTag);
}

BOOL WINAPI MAP_SetId3TagFile(LPCTSTR pszFile, ID3TAGV1* pTag)
{
	return SetId3Tag(pszFile, pTag);
}

MAP_STATUS WINAPI MAP_GetStatus(HANDLE hLib)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->GetStatus();
}

BOOL WINAPI MAP_SetOptions(HANDLE hLib, MAP_OPTIONS* pOptions)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->SetOptions(pOptions);
}

void WINAPI MAP_GetOptions(HANDLE hLib, MAP_OPTIONS* pOptions)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->GetOptions(pOptions);
}

BOOL WINAPI MAP_GetScanPeek(HANDLE hLib)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->GetScanPeek();
}

void WINAPI MAP_SetScanPeek(HANDLE hLib, BOOL fScan)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->SetScanPeek(fScan);
}

void WINAPI MAP_AudioDeviceClose(HANDLE hLib)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->AudioDeviceClose();
}

void WINAPI MAP_GetBufferInfo(HANDLE hLib, DWORD* pcbTotalAudio, DWORD* pcbBufferedAudio,
									 DWORD* pcbTotalStream, DWORD* pcbBufferedStream)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->GetBufferInfo(pcbTotalAudio, pcbBufferedAudio, pcbTotalStream, pcbBufferedStream);
}

// for streaming
BOOL WINAPI MAP_OpenURL(HANDLE hLib, LPCTSTR pszURL)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->OpenURL(pszURL);
}

BOOL WINAPI MAP_GetStreamInfo(HANDLE hLib, LPTSTR pszName, LPTSTR pszGenre, LPTSTR pszURL)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->GetStreamInfo(pszName, pszGenre, pszURL);
}

BOOL WINAPI MAP_GetStreamTitle(HANDLE hLib, LPTSTR pszTitle)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->GetStreamTitle(pszTitle);
}

BOOL WINAPI MAP_SetStreamingOptions(HANDLE hLib, MAP_STREAMING_OPTIONS* pOptions)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->SetStreamingOptions(pOptions);
}

void WINAPI MAP_GetStreamingOptions(HANDLE hLib, MAP_STREAMING_OPTIONS* pOptions)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->GetStreamingOptions(pOptions);
}

MAP_STREAMING_STATUS WINAPI MAP_GetStreamingStatus(HANDLE hLib)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->GetStreamingStatus();
}

int WINAPI MAP_GetStreamingBufferingCount(HANDLE hLib)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->GetStreamingBufferingCount();
}

int WINAPI MAP_GetDecoderPlugInCount(HANDLE hLib)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->PlugInGetCount();
}

MAP_DEC_PLUGIN* WINAPI MAP_GetDecoderPlugIn(HANDLE hLib, int nIndex)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->PlugInGetInterface(nIndex);
}

DWORD WINAPI MAP_GetVolume(HANDLE hLib, BOOL bSysVolume)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	return pPlayer->GetVolume(bSysVolume);
}

void WINAPI MAP_SetVolume(HANDLE hLib, DWORD dwVolume, BOOL bSysVolume)
{
	CPlayer* pPlayer = (CPlayer*) hLib;
	pPlayer->SetVolume(dwVolume, bSysVolume);
}
