#include <windows.h>
#include "maplay.h"
#include "helper.h"
#include "player.h"

void CPlayer::NetStreamingThread()
{
	int i;
	BYTE bRead[NET_READ_BUFF_LEN];
	DWORD cbInBuf;

	m_StreamingStatus = MAP_STREAMING_CONNECTING;
	UpdateStatus(MAP_STATUS_WAIT);

	if (!m_Receiver.Connect()) {
		UnpreparePlayback(FALSE, TRUE);
		return;
	}

	m_StreamingStatus = MAP_STREAMING_BUFFERING;

	if (!WaitForPrebuffering(m_Receiver.GetPrebufferingCount())) {
		UnpreparePlayback(FALSE, TRUE);
		return;
	}

	for (i = 0; i < m_Receiver.GetPrebufferingCount(); i++) {
		if (!m_Receiver.Read(bRead, NET_READ_BUFF_LEN, &cbInBuf)) {
			UnpreparePlayback(FALSE, TRUE);
			return;
		}

		NetCheckStreamId3Tag(bRead, cbInBuf);

		if (NetParseMpegStream(bRead, cbInBuf)) {
			m_fNet = NET_OPEN_MPEG;
			break;
		}
		else if (NetParseOvStream(bRead, cbInBuf)) {
			m_fNet = NET_OPEN_OV;
			break;
		}
		else if (PlugInParseStream(bRead, cbInBuf)) {
			m_fNet = NET_OPEN_PLUGIN;
			break;
		}
	}

	if (!PreparePlayback()) {
		UnpreparePlayback(FALSE, TRUE);
		return;
	}

	m_StreamingStatus = MAP_STREAMING_CONNECTED;
	UpdateStatus(MAP_STATUS_PLAY);

	if (m_fNet == NET_OPEN_OV)
		NetOvStreaming(bRead, cbInBuf);
	else if (m_fNet == NET_OPEN_PLUGIN)
		PlugInStreaming(bRead, cbInBuf);
	else 
		NetMpegStreaming(bRead, cbInBuf);
}

void CPlayer::NetStop()
{
	if (m_pOvd_buf) {
		ovd_uninit_stream(m_pOvd_buf);
		m_pOvd_buf = NULL;
	}
	*m_szOvdTitle = NULL;

	PlugInStopStreaming();
}

void CPlayer::NetClose()
{
	m_fNet = NET_OPEN_NONE;
	m_StreamingStatus = MAP_STREAMING_DISABLED;
}

BOOL CPlayer::NetParseMpegStream(LPBYTE pbBuf, DWORD cbBuf)
{
	DWORD i = 0;
	MPEG_AUDIO_INFO info;
	while (i < cbBuf - 4) {
		if (ParseHeader(pbBuf + i, &info)) {
			if (i + info.nFrameSize < cbBuf - 4) {
				if (CheckAudioHeader(pbBuf + i + info.nFrameSize)) {
					m_Info = info;
					break;
				}
			}
		}
		i++;
	}
	if (m_Info.nVersion == 0 || m_Info.nLayer == 0 ||
		m_Info.nSamplingRate == 0 || m_Info.nChannels == 0)
		return FALSE;

	return TRUE;
}

BOOL CPlayer::NetReconnect()
{
	if (m_Receiver.IsEos()) {
		UnpreparePlayback(TRUE);
		return FALSE;
	}

	if (!m_Receiver.IsShoutcast()) {
		UnpreparePlayback(TRUE);
		return FALSE;
	}

	MAP_STATUS fStatus = m_Status;
	m_StreamingStatus = MAP_STREAMING_CONNECTING;
	UpdateStatus(MAP_STATUS_WAIT);

	m_Receiver.Disconnect();
	if (!m_Receiver.Connect()) {
		UnpreparePlayback(FALSE, TRUE);
		return FALSE;
	}

	m_StreamingStatus = MAP_STREAMING_BUFFERING;

	if (!WaitForPrebuffering()) {
		UnpreparePlayback(FALSE, TRUE);
		return FALSE;
	}

	m_StreamingStatus = MAP_STREAMING_CONNECTED;
	UpdateStatus(m_Status == MAP_STATUS_WAIT ? fStatus : m_Status);
	return TRUE;
}

void CPlayer::NetMpegStreaming(LPBYTE pbBuf, DWORD cbBuf)
{
	DWORD cbInBuf, cbInBufLeft = 0;
	cbInBuf = cbBuf;

	while (TRUE) {
		if (m_fStop) {
			UnpreparePlayback();
			return;
		}

		if (m_Receiver.GetBufferingCount() < 2) {
			while (TRUE) {
				Sleep(1);
				if (m_Output.GetBufferingCount() < 1)
					break;
				if (m_Receiver.GetBufferingCount() > 1)
					goto read;
				if (m_fStop) {
					UnpreparePlayback();
					return;
				}
			}

			if (!NetReconnect())
				return;

			m_Output.Pause(TRUE);
			m_fPlay = TRUE;
		}

read:
		if (!m_Receiver.Read(pbBuf + cbInBufLeft, NET_READ_BUFF_LEN - cbInBufLeft, &cbInBuf) || !cbInBuf) {
			UnpreparePlayback(TRUE);
			return;
		}

		cbInBufLeft += cbInBuf;
		cbInBuf = 0;

		if (MpgRender(pbBuf, cbInBufLeft, &cbInBuf) == MAD_FATAL_ERR) {
			UnpreparePlayback(FALSE, TRUE);
			return;
		}
		
		memmove(pbBuf, pbBuf + cbInBuf, cbInBufLeft - cbInBuf);
		cbInBufLeft -= cbInBuf;
	}
}

BOOL CPlayer::NetParseOvStream(LPBYTE pbBuf, DWORD cbBuf)
{
	if (cbBuf > OVD_STREAM_BUF_LEN)
		return FALSE;

	m_pOvd_buf = ovd_init_stream();
	memcpy(m_pOvd_buf->buf, pbBuf, min(cbBuf, OVD_STREAM_BUF_LEN));
	if (!ovd_parse_stream(m_pOvd_buf))
		return FALSE;

	ovd_info oi;
	memset(&oi, 0, sizeof(oi));
	memset(&m_Info, 0, sizeof(m_Info));
	if (!ovd_get_stream_info(m_pOvd_buf, &oi))
		return FALSE;

	m_Info.nChannels = oi.channels;
	m_Info.nSamplingRate = oi.rate;
	m_Info.nBitRate = oi.bitrate_nominal / 1000;
	m_Info.nFrameSize = 0;
	m_Info.nSamplesPerFrame = 0;

	if (m_Info.nSamplingRate == 0 || m_Info.nChannels == 0)
		return FALSE;

	return TRUE;
}

void CPlayer::NetOvStreaming(LPBYTE pbBuf, DWORD cbBuf)
{
	int nRet, nOutput;
	if (cbBuf < OVD_STREAM_BUF_LEN) {
		UnpreparePlayback(FALSE, TRUE);
		return;
	}

	ovd_comment comment;
	
	while (TRUE) {
		ovd_get_stream_comment(m_pOvd_buf, &comment);
		if (_tcslen(comment.szTitle)) {
			if (_tcslen(comment.szArtist))
				wsprintf(m_szOvdTitle, _T("%s - %s"), comment.szArtist, comment.szTitle);
			else
				_tcscpy(m_szOvdTitle, comment.szTitle);
			PostCallbackMessage(MAP_MSG_STREAM_TITLE, (WPARAM)m_szOvdTitle, 0);
		}

		if (m_fStop) {
			UnpreparePlayback();
			return;
		}

		if (m_Receiver.GetBufferingCount() < 2) {
			while (TRUE) {
				Sleep(1);
				if (m_Output.GetBufferingCount() < 1)
					break;
				if (m_Receiver.GetBufferingCount() > 1)
					goto read;
				if (m_fStop) {
					UnpreparePlayback();
					return;
				}
			}

			if (!NetReconnect())
				return;

			m_Output.Pause(TRUE);
			m_fPlay = TRUE;

			DWORD dwRead;
			if (!m_Receiver.Read(pbBuf, cbBuf, &dwRead) || !dwRead) {
				UnpreparePlayback(TRUE);
				return;
			}

			ovd_uninit_stream(m_pOvd_buf);
			m_pOvd_buf = NULL;
			if (!NetParseOvStream(pbBuf, cbBuf)) {
				UnpreparePlayback(TRUE);
				return;
			}
		}

read:
		if (m_pOvd_buf->len) {
			if (!m_Receiver.Read((BYTE*)m_pOvd_buf->buf, OVD_STREAM_BUF_LEN, &m_pOvd_buf->len) || !m_pOvd_buf->len) {
				UnpreparePlayback(TRUE);
				return;
			}
		}

		if (!m_pOutHdr) {
			m_pOutHdr = m_Output.GetBuffer();
			m_cbOutBufLeft = m_cbOutBuf;
		}

		nRet = ovd_decode_stream(m_pOvd_buf, 
			(BYTE*)m_pOutHdr->lpData + (m_cbOutBuf - m_cbOutBufLeft), m_cbOutBufLeft, &nOutput);

		Preamp((LPBYTE)m_pOutHdr->lpData + (m_cbOutBuf - m_cbOutBufLeft), nOutput);

		m_cbOutBufLeft -= nOutput;

		switch (nRet) {
		case OVD_FATAL_ERR:
		case OVD_ERR:
			UnpreparePlayback(FALSE, TRUE);
			break;
		case OVD_EOF:
			UnpreparePlayback(TRUE);
			return;
		case OVD_NEED_MORE_OUTPUT:
			OutputBuffer(m_pOutHdr, m_cbOutBuf - m_cbOutBufLeft);
			m_pOutHdr = NULL;
			break;
		}
	}
}

void CPlayer::NetCheckStreamId3Tag(LPBYTE pbBuf, DWORD cbBuf)
{
	ID3TAGV1 tag = {0};
	TCHAR szName[MAX_URL] = {0};

	if (ParseId3TagV2(pbBuf, cbBuf, &tag)) {
		if (_tcslen(tag.szArtist) && _tcslen(tag.szTrack)) {
			if (_tcslen(tag.szArtist) + _tcslen(tag.szTrack) + _tcslen(_T(" - ")) < sizeof(szName) / sizeof(WCHAR)) {
				wsprintf(szName, _T("%s - %s"), tag.szArtist, tag.szTrack);
			}
			else {
				_tcsncpy(szName, tag.szTrack, sizeof(szName) / sizeof(TCHAR));
				szName[MAX_URL - 1] = NULL;
			}
		}
		else if (_tcslen(tag.szTrack)) {
			_tcsncpy(szName, tag.szTrack, sizeof(szName) / sizeof(TCHAR));
			szName[MAX_URL - 1] = NULL;
		}

		if (_tcslen(szName)) {
			m_Receiver.SetStreamName(szName);
		}
	}
}