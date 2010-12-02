#include <windows.h>
#include "maplay.h"
#include "helper.h"
#include "player.h"

#define WAV_FILE_BUFF_LEN	4096
#ifndef EMBEDDED_CE
BOOL CPlayer::WavOpenFile(LPCTSTR pszFile)
{
	if (!m_Reader.Open(pszFile))
		return FALSE;

	_tcscpy(m_szFile, pszFile);
	if (!WavScanFile()) {
		WavClose();
		return FALSE;
	}

	m_fOpen = OPEN_WAV_FILE;
	return TRUE;
}

BOOL CPlayer::WavScanFile()
{
	DWORD dwSamples = 0;
	DWORD dwBuf = 0, dwRead = 0;
	m_Reader.SetPointer(0, FILE_BEGIN);
	WAVEFORMATEX *pwfxSrc, wfxDst;
	pwfxSrc = NULL;
	
	// "RIFF"
	if (!m_Reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead) ||
		dwBuf != MAKEFOURCC('R', 'I', 'F', 'F'))
		return FALSE;

	// "WAVE"
	m_Reader.SetPointer(4, FILE_CURRENT);	
	if (!m_Reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead) ||
		dwBuf != MAKEFOURCC('W', 'A', 'V', 'E'))
		goto fail;

	// "fmt "
	while (TRUE) {
		if (!m_Reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead))
			goto fail;

		if (dwBuf == MAKEFOURCC('f', 'm', 't', ' '))
			break;

		if (!m_Reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead))
			goto fail;

		if (m_Reader.SetPointer(dwBuf, FILE_CURRENT) == MAXLONGLONG)
			goto fail;
	}

	if (!m_Reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead) || dwRead != sizeof(DWORD))
		goto fail;

	pwfxSrc = (WAVEFORMATEX*)new BYTE[dwBuf];
	if (!m_Reader.Read((LPBYTE)pwfxSrc, dwBuf, &dwRead) || dwRead != dwBuf)
		goto fail;

	if (pwfxSrc->nBlockAlign > WAV_FILE_BUFF_LEN)
		goto fail;

#if 1
	memset(&wfxDst, 0, sizeof(wfxDst));
	wfxDst.wFormatTag = WAVE_FORMAT_PCM;
	if (acmFormatSuggest(NULL, pwfxSrc, &wfxDst,
		sizeof(WAVEFORMATEX), ACM_FORMATSUGGESTF_WFORMATTAG) != 0)
		goto fail;
#endif

	// "fact" or "data"
	m_Reader.SetPointer(dwBuf - dwRead, FILE_CURRENT);

	if (!m_Reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead))
		goto fail;

	if (dwBuf == MAKEFOURCC('f', 'a', 'c', 't')) {
		if (!m_Reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead) ||
			dwBuf != 4)
			goto fail;
		
		if (!m_Reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead) ||
			dwRead != sizeof(DWORD))
			goto fail;

		dwSamples = dwBuf;

		if (!m_Reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead) ||
			dwRead != sizeof(DWORD))
			goto fail;
	}

	// "data"
	//if (dwBuf != MAKEFOURCC('d', 'a', 't', 'a'))
	//	goto fail;
retry:
	while (dwBuf != MAKEFOURCC('d', 'a', 't', 'a')) {
		BYTE b;
		if (!m_Reader.Read(&b, sizeof(b), &dwRead) || !dwRead)
			goto fail;
		LPBYTE pb = (LPBYTE)&dwBuf;
		pb[0] = pb[1];
		pb[1] = pb[2];
		pb[2] = pb[3];
		pb[3] = b;
	}

	if (!m_Reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead) || !dwRead)
		goto fail;

	if (dwBuf == 0)
		goto retry;

	// データの準備
	m_dwDataSize = dwBuf;
	m_llDataOffset = m_Reader.SetPointer(0, FILE_CURRENT);
	if (m_dwDataSize > m_Reader.GetSize() - m_llDataOffset) {
		m_dwDataSize = (DWORD)(m_Reader.GetSize() - m_llDataOffset);
	}
	if (pwfxSrc->wFormatTag == WAVE_FORMAT_PCM) {
		dwSamples = m_dwDataSize / (pwfxSrc->nChannels * pwfxSrc->wBitsPerSample / 8);
		wfxDst = *pwfxSrc;
	}
	else if (!dwSamples) {
		dwSamples = m_dwDataSize / pwfxSrc->nAvgBytesPerSec * pwfxSrc->nSamplesPerSec;
	}

	memset(&m_Info, 0, sizeof(m_Info));
	m_Info.nChannels = wfxDst.nChannels;
	m_Info.nSamplingRate = wfxDst.nSamplesPerSec;
	m_Info.nBitRate = pwfxSrc->nAvgBytesPerSec * 8 / 1000;
	m_nDuration = dwSamples;

#if 1
	// ACMを開く
	if (acmStreamOpen(&m_hAcm, NULL, pwfxSrc, &wfxDst, 
						NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME))
		goto fail;
#endif

	m_pwfxSrc = pwfxSrc;
	m_pwfxDst = new WAVEFORMATEX;
	*m_pwfxDst = wfxDst;
	m_dwCurrentSize = 0;

	return TRUE;
fail:
	m_Reader.Close();
	if (pwfxSrc) delete pwfxSrc;
	return FALSE;
}

void CPlayer::WavClose()
{
	if (m_pwfxSrc) {
		delete m_pwfxSrc;
		m_pwfxSrc = NULL;
	}
	if (m_pwfxDst) {
		delete m_pwfxDst;
		m_pwfxDst = NULL;
	}
	if (m_hAcm) {
		acmStreamClose(m_hAcm, 0);
		m_hAcm = NULL;
	}
	m_dwDataSize = 0;
	m_llDataOffset = 0;
	m_dwCurrentSize = 0;
}

BOOL CPlayer::WavIsValidFile(LPCTSTR pszFile)
{
	CReader reader;
	DWORD dwBuf = 0, dwRead = 0;
	WAVEFORMATEX *pwfxSrc, wfxDst;
	pwfxSrc = NULL;

	if (!reader.Open(pszFile))
		return FALSE;

	// "RIFF"
	if (!reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead) ||
		dwBuf != MAKEFOURCC('R', 'I', 'F', 'F'))
		goto error;

	// "WAVE"
	reader.SetPointer(4, FILE_CURRENT);	
	if (!reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead) ||
		dwBuf != MAKEFOURCC('W', 'A', 'V', 'E'))
		goto error;

	// "fmt "
	while (TRUE) {
		if (!reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead))
			goto error;

		if (dwBuf = MAKEFOURCC('f', 'm', 't', ' '))
			break;

		if (!reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead))
			goto error;

		if (reader.SetPointer(dwBuf, FILE_CURRENT) == MAXLONGLONG)
			goto error;
	}

	if (!reader.Read((LPBYTE)&dwBuf, sizeof(dwBuf), &dwRead))
		goto error;

	pwfxSrc = (WAVEFORMATEX*)new BYTE[dwBuf];
	if (!reader.Read((LPBYTE)pwfxSrc, dwBuf, &dwRead))
		goto error;

#if 1
	memset(&wfxDst, 0, sizeof(wfxDst));
	wfxDst.wFormatTag = WAVE_FORMAT_PCM;
	if (acmFormatSuggest(NULL, pwfxSrc, &wfxDst,
		sizeof(WAVEFORMATEX), ACM_FORMATSUGGESTF_WFORMATTAG) != 0)
		goto error;
#endif

	delete pwfxSrc;
	reader.Close();
	return TRUE;

error:
	if (pwfxSrc) delete pwfxSrc;
	reader.Close();
	return FALSE;
}

BOOL CPlayer::WavSeekFile(long lTime)
{
	LONGLONG llPointer;
	int nNew = (int)((double)m_pwfxSrc->nSamplesPerSec * lTime / 1000);
	if (m_pwfxSrc->wFormatTag == WAVE_FORMAT_PCM) {
		llPointer = (LONGLONG)((double)m_pwfxSrc->nChannels *
						m_pwfxSrc->wBitsPerSample / 8 * nNew);
		llPointer += m_llDataOffset;
	}
	else {
		llPointer = (LONGLONG)((double)m_pwfxSrc->nAvgBytesPerSec * lTime / 1000);
		llPointer -= llPointer % m_pwfxSrc->nBlockAlign;
		llPointer += m_llDataOffset;
	}
	if (m_Reader.SetPointer(llPointer, FILE_BEGIN) == MAXLONGLONG)
		return FALSE;

	// シーク後の後処理
	m_fSeek = TRUE;
	m_nSeek = nNew;
	m_Output.Reset(); // サウンドバッファはクリアする

	m_dwCurrentSize = (DWORD)(llPointer - m_llDataOffset);
	return TRUE;
}

void CPlayer::WavStop()
{
	WavSeekFile(0);
}

DWORD CPlayer::WavPlayerThread()
{
	BOOL fFlush = FALSE;
	if (m_pwfxSrc->wFormatTag == WAVE_FORMAT_PCM) {
		// PCM
		DWORD cbInBuf, cbRead;
		while (TRUE) {
			// 停止フラグのチェック
			if (m_fStop)
				return RET_STOP;

			if (!m_pOutHdr)
				m_pOutHdr = m_Output.GetBuffer();

			{
				// Critical Sectionのセット
				CAutoLock lock(&m_csecThread);
				if (m_fSeek) {
					if (m_Status == MAP_STATUS_PLAY)
						m_fPlay = TRUE;

					m_Reverb.Reset();
					m_Echo.Reset();
					m_BassBoost.Reset();
					m_3DChorus.Reset();
					m_Output.Reset();
					m_fSeek = FALSE;
					m_pOutHdr = NULL;
					continue;
				}

				// 読み込み
				cbRead = min(m_dwDataSize - m_dwCurrentSize, m_cbOutBuf);
				if (!m_Reader.Read((LPBYTE)m_pOutHdr->lpData, cbRead, &cbInBuf) || !cbInBuf) {
					if (GetLastError() != ERROR_SUCCESS)
						return RET_ERROR;
					fFlush = TRUE;
				}

				Preamp((LPBYTE)m_pOutHdr->lpData, cbInBuf);

				m_dwCurrentSize += cbInBuf;
			}
			if (fFlush)
				return RET_EOF;

			OutputBuffer(m_pOutHdr, cbInBuf);
			m_cbOutBufLeft = 0;
			m_pOutHdr = NULL;

			if (m_dwCurrentSize >= m_dwDataSize)
				return RET_EOF;
			if (m_fSuppress)
				return RET_EOF;
		}
	}
	else {
		// PCM 以外
		// デコード開始
		BYTE bRead[WAV_FILE_BUFF_LEN];
		DWORD cbInBuf, cbRead;
		while (TRUE) {
			// 停止フラグのチェック
			if (m_fStop)
				return RET_STOP;

			// 出力バッファの確保
			if (m_pOutHdr) {
				OutputBuffer(m_pOutHdr, m_cbOutBuf - m_cbOutBufLeft);
				m_cbOutBufLeft = 0;
				m_pOutHdr = NULL;
			}
			if (m_fSuppress)
				return RET_EOF;
			m_pOutHdr = m_Output.GetBuffer();
			m_cbOutBufLeft = m_cbOutBuf;

			{
				// Critical Sectionのセット
				CAutoLock lock(&m_csecThread);
				if (m_fSeek) {
					if (m_Status == MAP_STATUS_PLAY)
						m_fPlay = TRUE;

					m_Reverb.Reset();
					m_Echo.Reset();
					m_BassBoost.Reset();
					m_Output.Reset();
					m_fSeek = FALSE;
					m_pOutHdr = NULL;
					acmStreamReset(m_hAcm, 0);
					continue;
				}

				acmStreamSize(m_hAcm, m_cbOutBufLeft, &cbRead, 
								ACM_STREAMSIZEF_DESTINATION);

				// 読み込み
				cbRead = min(m_dwDataSize - m_dwCurrentSize, cbRead);
				cbRead = min(WAV_FILE_BUFF_LEN, cbRead);
				if (!m_Reader.Read(bRead, cbRead, &cbInBuf) || !cbInBuf) {
					if (GetLastError() != ERROR_SUCCESS)
						return RET_ERROR;

					fFlush = TRUE;
				}
				m_dwCurrentSize += cbInBuf;
			}
			if (fFlush)
				return RET_EOF;

			// ヘッダの確保
			ACMSTREAMHEADER ash;
			memset(&ash, 0, sizeof(ash));
			ash.cbStruct = sizeof(ash); //構造体のサイズ
			ash.pbSrc = bRead;
			ash.cbSrcLength = cbInBuf;
			ash.pbDst = (LPBYTE)m_pOutHdr->lpData + (m_cbOutBuf - m_cbOutBufLeft);
			ash.cbDstLength = m_cbOutBufLeft;
			if (acmStreamPrepareHeader(m_hAcm, &ash, 0))
				break;

			// 変換
			if (acmStreamConvert(m_hAcm, &ash, 0))
				break;

			Preamp((LPBYTE)m_pOutHdr->lpData + (m_cbOutBuf - m_cbOutBufLeft), ash.cbDstLengthUsed);
			m_cbOutBufLeft -= ash.cbDstLengthUsed;

			// 後処理
			if (acmStreamUnprepareHeader(m_hAcm, &ash, 0))
				break;

			if (m_dwCurrentSize >= m_dwDataSize)
				return RET_EOF;
		}

		return RET_ERROR;
	}
}

int CPlayer::WavRender(LPBYTE pbInBuf, DWORD cbInBuf, LPDWORD pcbProceed)
{
	*pcbProceed = 0;
	return 0;
}
#endif