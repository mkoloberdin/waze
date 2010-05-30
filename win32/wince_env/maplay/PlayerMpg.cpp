#include <windows.h>
#include "maplay.h"
#include "helper.h"
#include "player.h"

BOOL CPlayer::MpgOpenFile(LPCTSTR pszFile)
{
	if (!IsValidFile(pszFile))
		return FALSE;

	if (!m_Reader.Open(pszFile))
		return FALSE;

	_tcscpy(m_szFile, pszFile);
	if (!MpgScanFile()) {
		m_Reader.Close();
		return FALSE;
	}

	m_fOpen = OPEN_MPG_FILE;
	return TRUE;
}

BOOL CPlayer::MpgScanFile()
{
	if (m_Options.fScanMpegCompletely)
		return MpgScanFileCompletely();
	else
		return MpgScanFileNormally();
}

#if 0
BOOL CPlayer::MpgScanFileNormally()
{
	// 特定の位置のフレームのみをスキャンする
	int nFrameSize = 0;
	int nFrameCount = 0;
	int nAvgBitrate = 0;
	int nCount;
	DWORD cbRead;
	LONGLONG llCur, llPrev;
	MPEG_AUDIO_INFO infoCur, infoPrev;
	BYTE bBuff[MPG_SCAN_BUFF_LEN];
	memset(&m_Info, 0, sizeof(MPEG_AUDIO_INFO));

#define MPG_MAX_SCAN_FRAME	16
#define MAX_SCAN_SEP		8
	LONGLONG llMax = 0;
	for (int i = 0; i < MAX_SCAN_SEP; i++) {
		// スキャン位置の確定
		llCur = m_Reader.GetSize() * (i + 1) / (MAX_SCAN_SEP + 1);
		if (llCur < llMax)
			continue;
		
		m_Reader.SetPointer(llCur, FILE_BEGIN);
		llCur = m_Reader.ScanAudioHeader();
		if (llCur == MAXLONGLONG)
			return FALSE;

		memset(&infoCur, 0, sizeof(MPEG_AUDIO_INFO));
		memset(&infoPrev, 0, sizeof(MPEG_AUDIO_INFO));
		
		nCount = 0;
		llPrev = llCur;
		while (TRUE) {
			if (!m_Reader.Read(bBuff, MPG_SCAN_BUFF_LEN, &cbRead))
				break;

			if (cbRead < 4)
				break;

			if (!ParseHeader(bBuff, &infoCur)) {
				nCount = max(0, nCount - 1);
				nFrameSize -= infoPrev.nFrameSize;
				nAvgBitrate -= infoPrev.nBitRate;

				m_Reader.SetPointer(llPrev + 1, FILE_BEGIN);
				llCur = m_Reader.ScanAudioHeader();
				if (llCur == MAXLONGLONG)
					break;

				llPrev = llCur;
				continue;
			}

			if (nCount) {
				if (infoCur.nVersion != infoPrev.nVersion ||
					infoCur.nLayer != infoPrev.nLayer ||
					infoCur.nSamplingRate != infoPrev.nSamplingRate) {
					nCount = max(0, nCount - 1);
					nFrameSize -= infoPrev.nFrameSize;
					nAvgBitrate -= infoPrev.nBitRate;

					m_Reader.SetPointer(llPrev + 1, FILE_BEGIN);
					llCur = m_Reader.ScanAudioHeader();
					if (llCur == MAXLONGLONG)
						break;

					llPrev = llCur;
					continue;
				}

				if (nCount == 1) m_Info = infoPrev;
			}

			nCount++;
			nFrameSize += infoCur.nFrameSize;
			nAvgBitrate += infoCur.nBitRate;

			infoPrev = infoCur;
			llPrev = llCur;
			llCur += infoCur.nFrameSize;
			llMax = llCur;

			if (nCount >= MPG_MAX_SCAN_FRAME)
				break;

			if (m_Reader.SetPointer(llCur, FILE_BEGIN) == MAXLONGLONG)
				break;
		}



		if (!nCount)
			break;

		nFrameCount += nCount;
	}
	
	if (!nFrameCount)
		return FALSE;
	
	m_Info.nFrameSize = nFrameSize / nFrameCount; // フレームサイズの平均
	m_Info.nBitRate = (int)((double)nAvgBitrate / nFrameCount + 0.5);

	LONGLONG llSize =  m_Reader.GetSize();
	nFrameCount = int(llSize / m_Info.nFrameSize);
	m_nDuration = (DWORD)m_Info.nSamplesPerFrame * nFrameCount;

	m_Reader.SetPointer(0, FILE_BEGIN);
	return TRUE;
}
#else

BOOL CPlayer::MpgScanFileNormally()
{
	// 特定の位置のフレームのみをスキャンする
	DWORD cbRead;
	LONGLONG llCur, llSize;
	MPEG_AUDIO_INFO infoCur, infoNext;
	BYTE bBuff[MPG_SCAN_BUFF_LEN];
	int nFrameCount, nSkip;

	memset(&m_Info, 0, sizeof(MPEG_AUDIO_INFO));
	memset(&infoCur, 0, sizeof(MPEG_AUDIO_INFO));
	memset(&infoNext, 0, sizeof(MPEG_AUDIO_INFO));

	m_Reader.SetPointer(0, FILE_BEGIN);
	llCur = m_Reader.ScanAudioHeader();
	if (llCur == MAXLONGLONG)
		return FALSE;
		
	while (TRUE) {
		if (!m_Reader.Read(bBuff, MPG_SCAN_BUFF_LEN, &cbRead))
			return FALSE;

		if (cbRead < 4)
			return FALSE;

		if (!ParseHeader(bBuff, &infoCur)) {
			m_Reader.SetPointer(llCur + 1, FILE_BEGIN);
			llCur = m_Reader.ScanAudioHeader();
			if (llCur == MAXLONGLONG)
				return FALSE;

			continue;
		}

		if (m_Reader.SetPointer(llCur + infoCur.nFrameSize, FILE_BEGIN) == MAXLONGLONG)
			return FALSE;

		if (!m_Reader.Read(bBuff, MPG_SCAN_BUFF_LEN, &cbRead))
			return FALSE;

		if (cbRead < 4)
			return FALSE;

		if (!ParseHeader(bBuff, &infoNext)) {
			m_Reader.SetPointer(llCur + 1, FILE_BEGIN);
			llCur = m_Reader.ScanAudioHeader();
			if (llCur == MAXLONGLONG)
				return FALSE;

			continue;
		}

		if (infoCur.nVersion != infoNext.nVersion ||
			infoCur.nLayer != infoNext.nLayer ||
			infoCur.nSamplingRate != infoNext.nSamplingRate) {
			m_Reader.SetPointer(llCur + 1, FILE_BEGIN);
			llCur = m_Reader.ScanAudioHeader();
			if (llCur == MAXLONGLONG)
				return FALSE;

			continue;
		}

		m_Info = infoCur;

		if (m_Info.nVersion == 2)
			nSkip = m_Info.nChannels == 2 ? 17 : 9;
		else
			nSkip = m_Info.nChannels == 2 ? 32 : 17;

		if (m_Reader.SetPointer(llCur + nSkip + 4, FILE_BEGIN) == MAXLONGLONG)
			return FALSE;

		if (!m_Reader.Read(bBuff, MPG_SCAN_BUFF_LEN, &cbRead))
			return FALSE;

		if (cbRead < 4)
			return FALSE;

		// Xing header
		if (bBuff[0] == 'X' && bBuff[1] == 'i' && bBuff[2] == 'n' && bBuff[3] == 'g') {
			if (m_Reader.SetPointer(4, FILE_CURRENT) == MAXLONGLONG)
				return FALSE;
			
			if (!m_Reader.Read(bBuff, MPG_SCAN_BUFF_LEN, &cbRead))
				return FALSE;

			if (cbRead < 4)
				return FALSE;

			nFrameCount = bBuff[0] << 24 | bBuff[1] << 16 | bBuff[2] << 8 | bBuff[3];
			llSize =  m_Reader.GetSize();
			m_nDuration = (DWORD)m_Info.nSamplesPerFrame * nFrameCount;

			m_Info.nFrameSize = (int)(llSize / nFrameCount);
			m_Info.nBitRate = (int)((llSize * 8/ (m_nDuration / m_Info.nSamplingRate) + 500) / 1000);
		}
		// VBRI header
		else if (bBuff[0] == 'V' && bBuff[1] == 'B' && bBuff[2] == 'R' && bBuff[3] == 'I') {
			if (m_Reader.SetPointer(10, FILE_CURRENT) == MAXLONGLONG)
				return FALSE;
			
			if (!m_Reader.Read(bBuff, MPG_SCAN_BUFF_LEN, &cbRead))
				return FALSE;

			if (cbRead < 4)
				return FALSE;

			nFrameCount = bBuff[0] << 24 | bBuff[1] << 16 | bBuff[2] << 8 | bBuff[3];
			llSize =  m_Reader.GetSize();
			m_nDuration = (DWORD)m_Info.nSamplesPerFrame * nFrameCount;

			m_Info.nFrameSize = (int)(llSize / nFrameCount);
			m_Info.nBitRate = (int)((llSize * 8/ (m_nDuration / m_Info.nSamplingRate) + 500) / 1000);
		}
		else {
			// CBR
			llSize =  m_Reader.GetSize();
			nFrameCount = int(llSize / m_Info.nFrameSize);
			m_nDuration = (DWORD)m_Info.nSamplesPerFrame * nFrameCount;
		}
		m_Reader.SetPointer(0, FILE_BEGIN);
		return TRUE;
	}
}

#endif

BOOL CPlayer::MpgScanFileCompletely()
{
	// フレームを完全にスキャンする
	int nCount = 0;
	int nFrameSize = 0;
	int nAvgBitrate = 0;
	DWORD cbRead;
	MPEG_AUDIO_INFO infoCur, infoPrev;
	BYTE bBuff[MPG_SCAN_BUFF_LEN];
	LONGLONG llCur, llPrev;

	memset(&m_Info, 0, sizeof(MPEG_AUDIO_INFO));
	memset(&infoCur, 0, sizeof(MPEG_AUDIO_INFO));
	memset(&infoPrev, 0, sizeof(MPEG_AUDIO_INFO));

	m_Reader.SetPointer(0, FILE_BEGIN);
	llCur = m_Reader.ScanAudioHeader();
	if (llCur == MAXLONGLONG)
		return FALSE;

	llPrev = llCur;
	while (TRUE) {
		if (!m_Reader.Read(bBuff, MPG_SCAN_BUFF_LEN, &cbRead))
			break;

		if (cbRead < 4)
			break;

		if (!ParseHeader(bBuff, &infoCur)) {
			nCount = max(0, nCount - 1);
			nFrameSize -= infoPrev.nFrameSize;
			nAvgBitrate -= infoPrev.nBitRate;

			m_Reader.SetPointer(llPrev + 1, FILE_BEGIN);
			llCur = m_Reader.ScanAudioHeader();
			if (llCur == MAXLONGLONG)
				break;

			llPrev = llCur;
			continue;
		}

		if (nCount) {
			if (infoCur.nVersion != infoPrev.nVersion ||
				infoCur.nLayer != infoPrev.nLayer ||
				infoCur.nSamplingRate != infoPrev.nSamplingRate) {
				nCount = max(0, nCount - 1);
				nFrameSize -= infoPrev.nFrameSize;
				nAvgBitrate -= infoPrev.nBitRate;

				m_Reader.SetPointer(llPrev + 1, FILE_BEGIN);
				llCur = m_Reader.ScanAudioHeader();
				if (llCur == MAXLONGLONG)
					break;

				llPrev = llCur;
				continue;
			}

			if (nCount == 1) m_Info = infoPrev;
		}

		nCount++;
		nFrameSize += infoCur.nFrameSize;
		nAvgBitrate += infoCur.nBitRate;

		infoPrev = infoCur;
		llPrev = llCur;
		llCur += infoCur.nFrameSize;

		if (m_Reader.SetPointer(llCur, FILE_BEGIN) == MAXLONGLONG)
			break;
	}

	if (!nCount)
		return FALSE;

	m_Info.nFrameSize = nFrameSize / nCount; // フレームサイズの平均
	m_Info.nBitRate = (int)((double)nAvgBitrate / nCount + 0.5);
	m_nDuration = (DWORD)m_Info.nSamplesPerFrame * nCount;

	m_Reader.SetPointer(0, FILE_BEGIN);
	return TRUE;
}

BOOL CPlayer::MpgSeekFile(long lTime)
{
	BOOL fRet = FALSE;
	BOOL fPause = FALSE;

	// 時間の変換
	int nNew = (int)((double)m_Info.nSamplingRate * lTime / 1000);
	int nFrames = nNew / m_Info.nSamplesPerFrame;

	LONGLONG llOld;
	if (m_Options.fScanMpegCompletely) {
		llOld = m_Reader.SetPointer(0, FILE_BEGIN);
		if (m_Reader.ScanAudioHeader(m_Info.nVersion, m_Info.nLayer) != MAXLONGLONG) {
			int nCount = 0;
			BYTE bBuff[4];
			DWORD dwRead;
			while (m_Reader.Read(bBuff, sizeof(bBuff), &dwRead) && dwRead == 4)	{
				// scan header
				MPEG_AUDIO_INFO info;
				if (ParseHeader(bBuff, &info)) {
					if (++nCount >= nFrames) {
						m_Reader.SetPointer(-4, FILE_CURRENT);
						fRet = TRUE;
						break;
					}
					else {
						m_Reader.SetPointer(info.nFrameSize - 4, FILE_CURRENT);
					}
				}
				else {
					m_Reader.SetPointer(-3, FILE_CURRENT);
					if (m_Reader.ScanAudioHeader(m_Info.nVersion, m_Info.nLayer) == MAXLONGLONG)
						break;
				}
			}
		}
	}
	else {
		LONGLONG llOffset = m_Info.nFrameSize * nFrames;

		// seek the offset
		llOld = m_Reader.SetPointer(llOffset, FILE_BEGIN);
		if (llOld != MAXLONGLONG) {
			if (m_Reader.ScanAudioHeader(m_Info.nVersion, m_Info.nLayer) != MAXLONGLONG)
				fRet = TRUE;
		}
	}

	if (fRet) {
		// シーク後の後処理
		m_fSeek = TRUE;
		m_nSeek = nNew;

		m_Output.Reset(); // サウンドバッファはクリアする
	}
	else {
		m_Reader.SetPointer(llOld, FILE_BEGIN);
	}
	return fRet;
}

int CPlayer::MpgRender(LPBYTE pbInBuf, DWORD cbInBuf, LPDWORD pcbProceed)
{
	int nRet;
	BOOL fNeedOutput = FALSE;
	DWORD cbInput, cbOutput;
	*pcbProceed = 0;
	do {
		// 停止フラグのチェック
		if (m_fStop)
			return MAD_OK;

		// 出力バッファの確保
		if (!m_pOutHdr || !m_cbOutBufLeft) {
			if (m_pOutHdr) {
				OutputBuffer(m_pOutHdr, m_cbOutBuf - m_cbOutBufLeft);
				m_cbOutBufLeft = 0;
				m_pOutHdr = NULL;
			}
			m_pOutHdr = m_Output.GetBuffer();
			m_cbOutBufLeft = m_cbOutBuf;
			if (m_fSeek)
				return MAD_OK;
			if (m_fStop)
				return MAD_OK;
		}

		// データのチェック
		if (!fNeedOutput && !CheckAudioHeader(pbInBuf + *pcbProceed)) {
			while (++*pcbProceed < cbInBuf - 4) {
				MPEG_AUDIO_INFO info;
				if (ParseHeader(pbInBuf + *pcbProceed, &info) &&
					info.nVersion == m_Info.nVersion &&
					info.nLayer == m_Info.nLayer && 
					info.nSamplingRate == m_Info.nSamplingRate)
					break;
			}

			if (*pcbProceed > cbInBuf - 4)
				return MAD_NEED_MORE_INPUT;
		}	

		// デコード (1フレーム)
		nRet = m_Decoder.Decode(pbInBuf + *pcbProceed, fNeedOutput ? 0 : cbInBuf - *pcbProceed, 
			(LPBYTE)m_pOutHdr->lpData + (m_cbOutBuf - m_cbOutBufLeft), 
			m_cbOutBufLeft, &cbOutput, &cbInput, 16, FALSE);
		switch (nRet) {
		case MAD_FATAL_ERR:
			// リセットする
			m_Decoder.Reset();
		case MAD_ERR:
			*pcbProceed += 1; // 1バイトだけ進める
			break;
		case MAD_NEED_MORE_OUTPUT:
			fNeedOutput = TRUE;
			*pcbProceed += cbInput;
			m_cbOutBufLeft -= cbOutput;
			break;
		case MAD_OK:
			fNeedOutput = FALSE;
			*pcbProceed += cbInput;
			m_cbOutBufLeft -= cbOutput;
			break;
		}
	}
	while (nRet != MAD_NEED_MORE_INPUT || fNeedOutput);
	return nRet;
}

DWORD CPlayer::MpgPlayerThread()
{
#define MPG_FILE_READ_SIZE	(MPG_FILE_BUFF_LEN * 8)
	BOOL fRet;
	BOOL fFlush = FALSE;
	DWORD cbBufSize, cbInBuf, cbInBufLeft = 0;
	LPBYTE pbRead = new BYTE[MPG_FILE_READ_SIZE];
	cbBufSize = MPG_FILE_READ_SIZE;
	if (!pbRead) {
		pbRead = new BYTE[MPG_FILE_BUFF_LEN];
		cbBufSize = MPG_FILE_BUFF_LEN;
		if (!pbRead) return RET_ERROR;
	}

	// デコード開始
	while (TRUE) {
		// 停止フラグのチェック
		if (m_fStop) {
			delete [] pbRead;
			return RET_STOP;
		}

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
				m_Decoder.Reset();

				
				cbInBufLeft = 0;
				m_fSeek = FALSE;
				m_pOutHdr = NULL;
				continue;
			}

			// 読み込み
			fRet = m_Reader.Read(pbRead + cbInBufLeft, cbBufSize - cbInBufLeft, &cbInBuf);
			if (!fRet || !cbInBuf) {
				if (!fRet && GetLastError() != ERROR_SUCCESS) {
					delete [] pbRead;
					return RET_ERROR;
				}
				fFlush = TRUE;
			}
			cbInBufLeft += cbInBuf;
			cbInBuf = 0;
		}

		if (fFlush) {
			if (!UnpreparePlayback(TRUE)) {
				fFlush = FALSE;
				continue;
			}
			delete [] pbRead;
			return RET_EOF;
		}

		if (MpgRender(pbRead, cbInBufLeft, &cbInBuf) == MAD_FATAL_ERR) {
			delete [] pbRead;
			return RET_ERROR;
		}
		if (m_fSuppress) {
			delete [] pbRead;
			return RET_EOF;
		}
		memmove(pbRead, pbRead + cbInBuf, cbInBufLeft - cbInBuf);
		cbInBufLeft -= cbInBuf;
	}
}

void CPlayer::MpgStop()
{
	m_Reader.SetPointer(0, FILE_BEGIN);
}

void CPlayer::MpgClose()
{
	m_Reader.Close();
	m_Receiver.Close();
}

BOOL CPlayer::MpgGetId3Tag(ID3TAGV1* pTag)
{
	return ::GetId3Tag(m_szFile, pTag);
}
