#include <windows.h>
#include "maplay.h"
#include "helper.h"
#include "player.h"
#include "libovd.h"

DWORD CPlayer::OvPlayerThread()
{	
	while (TRUE) {
		// 停止フラグのチェック
		if (m_fStop)
			return RET_STOP;

		// 出力バッファの確保
		if (!m_pOutHdr || m_cbOutBufLeft < OVD_PCMBUF_LEN) {
			if (m_pOutHdr) {
				OutputBuffer(m_pOutHdr, m_cbOutBuf - m_cbOutBufLeft);
				m_cbOutBufLeft = 0;
				m_pOutHdr = NULL;
			}
			if (m_fSuppress)
				return RET_EOF;
			m_pOutHdr = m_Output.GetBuffer();
			m_cbOutBufLeft = m_cbOutBuf;
			if (m_fSeek)
				continue;
		}

		int nRet, nOutput;
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

			// デコード
			nRet = ovd_read(m_hOvd, (LPBYTE)m_pOutHdr->lpData + (m_cbOutBuf - m_cbOutBufLeft)
							, m_cbOutBufLeft, &nOutput);

			// プリアンプ
			Preamp((LPBYTE)m_pOutHdr->lpData + (m_cbOutBuf - m_cbOutBufLeft), nOutput);

			m_cbOutBufLeft -= nOutput;
		}
		switch (nRet)
		{
		case OVD_FATAL_ERR:
		case OVD_ERR:
			return RET_ERROR;
		case OVD_EOF:
			return RET_EOF;
		}
	}
}

BOOL CPlayer::OvOpenFile(LPCTSTR pszFile)
{
	m_hOvd = ovd_open_file(pszFile);
	if (!m_hOvd) return FALSE;

	_tcscpy(m_szFile, pszFile);
	if (!OvScanFile()) {
		ovd_close(m_hOvd);
		m_hOvd = NULL;
		return FALSE;
	}

	m_fOpen = OPEN_OV_FILE;
	return TRUE;
}
BOOL CPlayer::OvScanFile()
{
	m_nDuration = (DWORD)ovd_get_duration(m_hOvd);
	if (!m_nDuration)
		return FALSE;

	ovd_info oi;
	memset(&oi, 0, sizeof(oi));
	memset(&m_Info, 0, sizeof(m_Info));
	if (!ovd_get_info(m_hOvd, &oi))
		return FALSE;

	m_Info.nChannels = oi.channels;
	m_Info.nSamplingRate = oi.rate;
	m_Info.nBitRate = oi.bitrate_nominal / 1000;
	m_Info.nFrameSize = 0;
	m_Info.nSamplesPerFrame = 0;

	return TRUE;
}

BOOL CPlayer::OvSeekFile(long lTime)
{
	if (!ovd_seek(m_hOvd, ((LONGLONG)lTime * m_Info.nSamplingRate) / 1000))
		return FALSE;

	// シーク後の後処理
	m_fSeek = TRUE;
	m_nSeek = (DWORD)ovd_get_current(m_hOvd);

	m_Output.Reset(); // サウンドバッファはクリアする
	return TRUE;
}

BOOL CPlayer::OvIsValidFile(LPCTSTR pszFile)
{
	return ovd_test_file(pszFile);
}

void CPlayer::OvStop()
{
	if (m_hOvd) OvSeekFile(0);
}

void CPlayer::OvClose()
{
	if (m_hOvd) {
		ovd_close(m_hOvd);
		m_hOvd = NULL;
	}
}

BOOL CPlayer::OvGetId3Tag(ID3TAGV1* pTag)
{
	ovd_comment oc = {0};
	if (!ovd_get_comment(m_hOvd, &oc))
		return FALSE;

	pTag->nTrackNum = _tcstol(oc.szTrackNum, NULL, 10);
	_tcsncpy(pTag->szTrack, oc.szTitle, MAX_TAG_STR);
	*(pTag->szTrack + MAX_TAG_STR - 1) = NULL;
	_tcsncpy(pTag->szArtist, oc.szArtist, MAX_TAG_STR);
	*(pTag->szArtist + MAX_TAG_STR - 1) = NULL;
	_tcsncpy(pTag->szAlbum, oc.szAlbum, MAX_TAG_STR);
	*(pTag->szAlbum + MAX_TAG_STR - 1) = NULL;
	_tcsncpy(pTag->szComment, oc.szComment, MAX_TAG_STR);
	*(pTag->szComment + MAX_TAG_STR - 1) = NULL;
	_tcsncpy(pTag->szGenre, oc.szGenre, MAX_TAG_STR);
	*(pTag->szGenre + MAX_TAG_STR - 1) = NULL;

	return TRUE;
}

BOOL CPlayer::OvGetId3TagFile(LPCTSTR pszFile, ID3TAGV1* pTag)
{
	ovd_comment oc = {0};
	if (!ovd_get_comment_from_file(pszFile, &oc))
		return FALSE;

	pTag->nTrackNum = _tcstol(oc.szTrackNum, NULL, 10);
	_tcsncpy(pTag->szTrack, oc.szTitle, MAX_TAG_STR);
	*(pTag->szTrack + MAX_TAG_STR - 1) = NULL;
	_tcsncpy(pTag->szArtist, oc.szArtist, MAX_TAG_STR);
	*(pTag->szArtist + MAX_TAG_STR - 1) = NULL;
	_tcsncpy(pTag->szAlbum, oc.szAlbum, MAX_TAG_STR);
	*(pTag->szAlbum + MAX_TAG_STR - 1) = NULL;
	_tcsncpy(pTag->szComment, oc.szComment, MAX_TAG_STR);
	*(pTag->szComment + MAX_TAG_STR - 1) = NULL;
	_tcsncpy(pTag->szGenre, oc.szGenre, MAX_TAG_STR);
	*(pTag->szGenre + MAX_TAG_STR - 1) = NULL;

	return TRUE;
}
