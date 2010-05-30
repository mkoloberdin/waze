#include <windows.h>
#include "maplay.h"
#include "helper.h"
#include "output.h"
#include "effect.h"

#define FADE_BITS	16
#define FADE_TIME	750
#define FADE_BASE	0

COutput::COutput()
{
	m_fScanPeek = FALSE;
	m_hwo = NULL;
	m_pHdr = NULL;
	m_hEvent = NULL;
	m_cWritten = 0;
	m_dwWritten = 0;
	m_pbBuf = NULL;
	m_nCurrent = 0;

	m_dwBufLen = 1000;
	m_cBuf = 8;
	m_cbBuf = 16 * 1024;

	m_nLPeek = 0;
	m_nRPeek = 0;
	memset(&m_pcm, 0, sizeof(PCMWAVEFORMAT));

	m_fFade = FALSE;
	m_nFadeSamples = 0;

	m_dwTotalSamples = 0;
	m_fPaused = FALSE;
	m_fDoubleBuf = FALSE;
	m_hEventThread = NULL;
	m_hThread = NULL;
	m_dwThreadId = 0;
	m_pbSubBuf = NULL;
	m_nHdrOut = 0;
	m_nWriteCur = 0;
	memset(m_HdrOut, 0, sizeof(m_HdrOut));
	m_dwVolume = MAX_WAVEOUTVOLUME;
}

COutput::~COutput()
{
}

BOOL COutput::SetOutputParam(DWORD dwBufLen, BOOL fDoubleBuf, BOOL fFade)
{
	CAutoLock lock(&m_csecDevice);
	if (m_hwo)
		return FALSE;

	if (dwBufLen < 10)
		return FALSE;

	m_dwBufLen = dwBufLen;
	m_fFade = fFade;
	m_fDoubleBuf = fDoubleBuf;
	return TRUE;
}

void COutput::GetOutputParam(LPDWORD pdwBufLen, LPBOOL pfDoubleBuf, LPBOOL pfFade)
{
	*pdwBufLen = m_dwBufLen;
	*pfFade = m_fFade;
	*pfDoubleBuf = m_fDoubleBuf;
}

BOOL COutput::Open(int nChannels, int nSamplingRate, int nBitsPerSample)
{
	MMRESULT mmr;
	int i, nCount = 0;
	
	CAutoLock lock(&m_csecDevice);
	if (!waveOutGetNumDevs())
		goto fail;

	if (nChannels == m_pcm.wf.nChannels &&
		(int)m_pcm.wf.nSamplesPerSec == nSamplingRate &&
		m_pcm.wBitsPerSample == nBitsPerSample)
		return TRUE;

	CloseAll();

	m_cbBuf = BUFLEN_BASE;
	if (nSamplingRate > 11025)
		m_cbBuf *= 2;
	if (nSamplingRate > 22050)
		m_cbBuf *= 2;
	if (nChannels > 1)
		m_cbBuf *= 2;
	if (nBitsPerSample > 8)
		m_cbBuf *= 2;

	m_pcm.wf.wFormatTag = WAVE_FORMAT_PCM;
	m_pcm.wf.nChannels = nChannels;
	m_pcm.wf.nSamplesPerSec = nSamplingRate;
	m_pcm.wf.nAvgBytesPerSec = nBitsPerSample * nSamplingRate * nChannels / 8;
	m_pcm.wf.nBlockAlign = nBitsPerSample * nChannels / 8;
	m_pcm.wBitsPerSample = nBitsPerSample;

	for (i = 0; i < 10; i++) {
		if (m_fDoubleBuf)
			mmr = waveOutOpen(&m_hwo, WAVE_MAPPER, (LPWAVEFORMATEX)&m_pcm, (DWORD)WaveOutCallback2, 0, CALLBACK_FUNCTION);
		else
			mmr = waveOutOpen(&m_hwo, WAVE_MAPPER, (LPWAVEFORMATEX)&m_pcm, (DWORD)WaveOutCallback, 0, CALLBACK_FUNCTION);

		if (mmr == MMSYSERR_NOERROR)
			break;
		else if (mmr != MMSYSERR_ALLOCATED)
			goto fail;
		Sleep(100);
	}
	m_fPaused = FALSE;
	m_dwWritten = 0;

	waveOutSetVolume(m_hwo, m_dwVolume);

	if (!PrepareBuffer())
		goto fail;

	if (m_fDoubleBuf && !PrepareSubBuffer())
		goto fail;

	m_nLPeek = 0;
	m_nRPeek = 0;

	if (m_fFade) {
		m_nFadeCurrent = FADE_BASE << FADE_BITS;
		m_nFadeSamples = m_pcm.wf.nSamplesPerSec * FADE_TIME / 1000;
		m_nFadeRate = (int)((((double)1 - FADE_BASE) / m_nFadeSamples) * (1 << FADE_BITS));
		m_nFadeRate += 1;
	}

	return TRUE;

fail:
	CloseAll();
	return FALSE;
}

void COutput::Close()
{
	CAutoLock lock(&m_csecDevice);

	if (!m_fDoubleBuf) {
		CloseAll();
		return;
	}

	Reset();
}

WAVEHDR* COutput::GetBuffer()
{
	WaitForSingleObject(m_hEvent, INFINITE);
	return &m_pHdr[m_nCurrent];
}

void COutput::PutBuffer(WAVEHDR* pHdr)
{
	if (m_fScanPeek)
		CheckPeek(pHdr);

	CAutoLock lock(&m_csecBuff);
	SetEvent(m_hEvent);
	m_cWritten--;
}

void COutput::OutputBuffer(WAVEHDR* pHdr)
{
	if (!pHdr->dwBytesRecorded) {
		CAutoLock lock(&m_csecBuff);
		SetEvent(m_hEvent);
		return;
	}

	if (m_fDoubleBuf) {
		FadeIn((LPBYTE)pHdr->lpData, pHdr->dwBytesRecorded);
		CAutoLock lockBuf(&m_csecBuff); // !!!!
		m_cWritten++;
		if (m_cWritten >= m_cBuf)
			ResetEvent(m_hEvent);

		m_nCurrent = (m_nCurrent + 1) % m_cBuf;
		m_dwWritten += pHdr->dwBytesRecorded / m_pcm.wf.nBlockAlign;
	}
	else {
		CAutoLock lockDev(&m_csecDevice);
		FadeIn((LPBYTE)pHdr->lpData, pHdr->dwBytesRecorded);
		waveOutWrite(m_hwo, pHdr, sizeof(WAVEHDR));

		CAutoLock lockBuf(&m_csecBuff);
		m_cWritten++;
		if (m_cWritten >= m_cBuf)
			ResetEvent(m_hEvent);

		m_nCurrent = (m_nCurrent + 1) % m_cBuf;
		m_dwWritten += pHdr->dwBytesRecorded / m_pcm.wf.nBlockAlign;
	}
}

BOOL COutput::PrepareBuffer()
{
	CAutoLock lock(&m_csecBuff);

	if (m_pHdr)
		return FALSE;

	m_cBuf = m_dwBufLen * m_pcm.wf.nAvgBytesPerSec / m_cbBuf / 1000 + 1;
	if (m_cBuf < 2) m_cBuf = 2;

	m_pHdr = new WAVEHDR[m_cBuf];
	if (!m_pHdr)
		return FALSE;

	m_pbBuf = new BYTE[m_cBuf * m_cbBuf];
	if (!m_pbBuf)
		return FALSE;

	memset(m_pbBuf, 0, m_cBuf * m_cbBuf);
	for (UINT i = 0; i < m_cBuf; i++) {
		memset(&m_pHdr[i], 0, sizeof(WAVEHDR));
		m_pHdr[i].lpData = (LPSTR)m_pbBuf + (m_cbBuf * i);
		m_pHdr[i].dwBufferLength = m_cbBuf;
		if (m_fDoubleBuf)
			m_pHdr[i].dwUser = 0;
		else {
			m_pHdr[i].dwUser = (DWORD)this;
			waveOutPrepareHeader(m_hwo, &m_pHdr[i] , sizeof(WAVEHDR));
		}
	}
	m_nCurrent = 0;
	m_cWritten = 0;
	m_hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	return TRUE;
}

void COutput::FreeBuffer()
{
	CAutoLock lock(&m_csecBuff);

	if (m_pHdr) {
		delete [] m_pHdr;
		m_pHdr =  NULL;
	}

	if (m_pbBuf) {
		delete [] m_pbBuf;
		m_pbBuf = NULL;
	}

	if (m_hEvent) {
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
	m_nCurrent = 0;
}

void COutput::Reset()
{
	if (!m_hwo)
		return;

	if (m_fDoubleBuf) {
		CAutoLock lockDev(&m_csecDevice);
		SetEvent(m_hEvent);
		m_cWritten = 0;
		m_dwWritten = 0;
		m_nCurrent = 0; 
		m_nWriteCur = 0;
		m_fPaused = TRUE;
		m_dwTotalSamples = 0;

		if (m_pHdr) {
			CAutoLock lock(&m_csecBuff);
			for (DWORD i = 0; i < m_cBuf; i++)
				m_pHdr[i].dwUser = 0;
		}
	}
	else {
		CAutoLock lock(&m_csecDevice);
		waveOutPause(m_hwo);
		waveOutReset(m_hwo);

		SetEvent(m_hEvent);
		m_cWritten = 0;
		m_dwWritten = 0;
		m_nCurrent = 0;
		m_nWriteCur = 0;
		waveOutPause(m_hwo);
	}

	m_nLPeek = 0;
	m_nRPeek = 0;
	if (m_fFade) {
		m_nFadeCurrent = FADE_BASE << FADE_BITS;
		m_nFadeSamples = m_pcm.wf.nSamplesPerSec * FADE_TIME / 1000;
		m_nFadeRate = (int)((((double)1 - FADE_BASE) / m_nFadeSamples) * (1 << FADE_BITS));
		m_nFadeRate += 1;
	}
}

void CALLBACK COutput::WaveOutCallback(HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if (uMsg == WOM_DONE) {
		COutput* pOutput = (COutput*)((WAVEHDR*)dwParam1)->dwUser;
		pOutput->PutBuffer((WAVEHDR*)dwParam1);
	}
}

void CALLBACK COutput::WaveOutCallback2(HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if (uMsg == WOM_DONE) {
		COutput* pOutput = (COutput*)((WAVEHDR*)dwParam1)->dwUser;
		if (pOutput->m_fScanPeek)
			pOutput->CheckPeek((WAVEHDR*)dwParam1);
		InterlockedIncrement((long*)&pOutput->m_nSubBuf);
		SetEvent(pOutput->m_hEventThread);
	}
}

void COutput::Pause(BOOL fPause)
{
	if (!m_hwo)
		return;

	if (m_fDoubleBuf)
		m_fPaused = fPause;
	else {
		CAutoLock lock(&m_csecDevice);
		if (fPause)
			waveOutPause(m_hwo);
		else
			waveOutRestart(m_hwo);
	}
}

DWORD COutput::GetCurrent()
{
	CAutoLock lock(&m_csecDevice);
	if (!m_hwo)
		return 0;

	if (m_fDoubleBuf)
		return m_dwTotalSamples;

	MMTIME time;
	memset(&time, 0, sizeof(MMTIME));
	time.wType = TIME_SAMPLES;
	waveOutGetPosition(m_hwo, &time, sizeof(MMTIME));
	return time.u.sample;
}

BOOL COutput::IsFlushed()
{
	return GetCurrent() >= m_dwWritten;
}

void COutput::GetBufferInfo(DWORD* pcbTotal, DWORD* pcbBuffered)
{
	if (!m_pHdr) {
		*pcbTotal = 0;
		*pcbBuffered = 0;
		return;
	}

	*pcbTotal = m_cBuf * m_cbBuf; 
	*pcbBuffered = m_cWritten * m_cbBuf; 
}

void COutput::CheckPeek(WAVEHDR* pHdr)
{
	// 絶対値とってないけどまあいいか（笑
	// オレっていいかげん（笑
	int nSamples;
	if (m_pcm.wBitsPerSample == 16) {
		short* p = (short*)pHdr->lpData;
		m_nLPeek = m_nRPeek = 0;
		nSamples = pHdr->dwBufferLength / 2 / m_pcm.wf.nChannels;

		if (m_pcm.wf.nChannels == 2) {
			for (int i = 0; i < nSamples; i++) {
				m_nLPeek = m_nLPeek > *p ? m_nLPeek : *p;
				p++;
				m_nRPeek = m_nRPeek > *p ? m_nRPeek : *p;
				p++;
			}
			m_nLPeek = m_nLPeek * 100 / 0x7FFF;
			m_nRPeek = m_nRPeek * 100 / 0x7FFF;
		}
		else {
			for (int i = 0; i < nSamples; i++) {
				m_nLPeek = m_nLPeek > *p ? m_nLPeek : *p;
				p++;
			}
			m_nLPeek = m_nLPeek * 100 / 0x8FFF;
			m_nRPeek = m_nLPeek;
		}
	}
	else {
		unsigned char* p = (unsigned char*)pHdr->lpData;
		m_nLPeek = m_nRPeek = 0x80;
		nSamples = pHdr->dwBufferLength / m_pcm.wf.nChannels;
		if (m_pcm.wf.nChannels == 2) {
			for (int i = 0; i < nSamples; i++) {
				m_nLPeek = m_nLPeek > *p ? m_nLPeek : *p;
				p++;
				m_nRPeek = m_nRPeek > *p ? m_nRPeek : *p;
				p++;
			}
			m_nLPeek = (m_nLPeek - 0x80) * 100 / 0x7F;
			m_nRPeek = (m_nRPeek - 0x80) * 100 / 0x7F;
		}
		else {
			for (int i = 0; i < nSamples; i++) {
				m_nLPeek = m_nLPeek > *p ? m_nLPeek : *p;
				p++;
			}
			m_nLPeek = (m_nLPeek - 0x80) * 100 / 0x7F;
			m_nRPeek = m_nLPeek;
		}
	}
}

void COutput::FadeIn(LPBYTE pbBuf, DWORD cbBuf)
{
	int n;
	DWORD cb = 0;
	while ((cbBuf - cb) && m_nFadeSamples) {
		for (int nChannel = 0; nChannel < m_pcm.wf.nChannels; nChannel++) {
			if (m_pcm.wBitsPerSample == 16) {
				n = ((int)*((short*)(pbBuf + cb))) * m_nFadeCurrent;
				*((short*)(pbBuf + cb)) = n >> FADE_BITS;
				cb += 2;
			}
			else {
				n = (UINT8_TO_INT16((int)*(pbBuf + cb))) * m_nFadeCurrent;
				*(pbBuf + cb) = INT16_TO_UINT8(n >> FADE_BITS);
				cb++;
			}
		}
		m_nFadeCurrent += m_nFadeRate;
		m_nFadeSamples--;

		if (m_nFadeCurrent >= (1 << FADE_BITS)) {
			m_nFadeSamples = 0;
			break;
		}
	}
}

short Clip16(int s);
unsigned char Clip8(int s);
void COutput::Preamp(LPBYTE pbBuf, DWORD cbBuf, int nRate)
{
	int n;

	if (nRate == PREAMP_FIXED_FLAT)
		return;

	if (m_pcm.wBitsPerSample == 16) {
		for (DWORD cb = 0; cb < cbBuf; ) {
			for (int nChannel = 0; nChannel < m_pcm.wf.nChannels; nChannel++) {
				n = (int)*((short*)(pbBuf + cb)) * nRate;
				*((short*)(pbBuf + cb)) = Clip16(n >> PREAMP_FIXED_BITS);
				cb += 2;
			}
		}
	}
	else {
		for (DWORD cb = 0; cb < cbBuf; ) {
			for (int nChannel = 0; nChannel < m_pcm.wf.nChannels; nChannel++) {
				n = UINT8_TO_INT16((int)*(pbBuf + cb)) * nRate;
				*(pbBuf + cb) = Clip8(INT16_TO_UINT8(n >> PREAMP_FIXED_BITS));
				cb++;
			}
		}
	}
}

#define ZERO_SAMPLE_MAX		16
DWORD COutput::ScanZeroSamples(BOOL fForward, LPBYTE pbBuf, DWORD cbBuf)
{
	int nZeroSamples = 0;
	int nBytesPerSample = m_pcm.wf.nChannels * m_pcm.wBitsPerSample / 8;
	int nSamples = cbBuf / nBytesPerSample;

	if (m_pcm.wBitsPerSample == 16) {
		short* pSample;
		if (fForward) {
			pSample = (short*)pbBuf;
		}
		else {
			pSample = (short*)(pbBuf + cbBuf - nBytesPerSample);
			nBytesPerSample = -nBytesPerSample;
		}

		if (m_pcm.wf.nChannels == 2) {
			for (int i = 0; i < nSamples; i++) {
				if ((*pSample > ZERO_SAMPLE_MAX) || (*pSample < -ZERO_SAMPLE_MAX) ||
					(*(pSample + 1) > ZERO_SAMPLE_MAX) || (*(pSample + 1) < -ZERO_SAMPLE_MAX))
					break;
				nZeroSamples++;
				pSample = (short*)((BYTE*)pSample + nBytesPerSample);
			}
		}
		else {
			for (int i = 0; i < nSamples; i++) {
				if ((*pSample > ZERO_SAMPLE_MAX) || (*pSample < -ZERO_SAMPLE_MAX))
					break;
				nZeroSamples++;
				pSample = (short*)((BYTE*)pSample + nBytesPerSample);
			}
		}
	}
	else {
		BYTE* pSample;
		if (fForward) {
			pSample = pbBuf;
		}
		else {
			pSample = pbBuf + cbBuf - nBytesPerSample;
			nBytesPerSample = -nBytesPerSample;
		}

		if (m_pcm.wf.nChannels == 2) {
			for (int i = 0; i < nSamples; i++) {
				if (*pSample != 0x80 || *(pSample + 1) != 0x80)
					break;
				nZeroSamples++;
				pSample += nBytesPerSample;
			}
		}
		else {
			for (int i = 0; i < nSamples; i++) {
				if (*pSample != 0x80)
					break;
				nZeroSamples++;
				pSample += nBytesPerSample;
			}
		}
	}

	return nZeroSamples * m_pcm.wf.nChannels * (m_pcm.wBitsPerSample / 8);
}

void COutput::CloseAll()
{
	if (m_hThread) {
		PostThreadMessage(m_dwThreadId, WM_QUIT, 0, 0);
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	if (m_hwo) {
		Reset();

		CAutoLock lockDev(&m_csecDevice);
		if (m_fDoubleBuf) {
			m_fPaused = TRUE;
			waveOutReset(m_hwo);

			CAutoLock lock(&m_csecBuff);
			for (UINT i = 0; i < SUBBUF_COUNT; i++)
				waveOutUnprepareHeader(m_hwo, &m_HdrOut[i] , sizeof(WAVEHDR));
		}
		else {
			CAutoLock lock(&m_csecBuff);
			for (UINT i = 0; i < m_cBuf; i++)
				waveOutUnprepareHeader(m_hwo, &m_pHdr[i] , sizeof(WAVEHDR));
		}

		waveOutClose(m_hwo);
		m_hwo = NULL;
	}

	if (m_hEventThread) {
		CloseHandle(m_hEventThread);
		m_hEventThread = NULL;
	}

	FreeSubBuffer();
	FreeBuffer();
	memset(&m_pcm, 0, sizeof(m_pcm));
}

DWORD WINAPI COutput::WaveOutThreadProc(LPVOID pParam)
{
	((COutput*)pParam)->WaveOutThread();
	return 0;
}

void COutput::WaveOutThread()
{
	DWORD cb;
	while (MsgWaitForMultipleObjects(1, &m_hEventThread, FALSE, INFINITE, QS_ALLEVENTS) == WAIT_OBJECT_0) {
		InterlockedDecrement((long*)&m_nSubBuf);
		if (!m_nSubBuf) ResetEvent(m_hEventThread);
		CAutoLock lockDev(&m_csecDevice);
		m_HdrOut[m_nHdrOut].dwBytesRecorded = 0;
		while (m_HdrOut[m_nHdrOut].dwBytesRecorded < m_HdrOut[m_nHdrOut].dwBufferLength) {
			if (!m_cWritten || m_fPaused) {
				if (!m_HdrClear[m_nHdrOut]) {
					if (!m_HdrOut[m_nHdrOut].dwBytesRecorded)
						m_HdrClear[m_nHdrOut] = 1;

					memset(m_HdrOut[m_nHdrOut].lpData + m_HdrOut[m_nHdrOut].dwBytesRecorded, 0, 
						m_HdrOut[m_nHdrOut].dwBufferLength - m_HdrOut[m_nHdrOut].dwBytesRecorded);
					m_HdrOut[m_nHdrOut].dwBytesRecorded = m_HdrOut[m_nHdrOut].dwBufferLength;
				}
				break;
			}
			
			CAutoLock lockBuf(&m_csecBuff);
			cb = min(m_HdrOut[m_nHdrOut].dwBufferLength - m_HdrOut[m_nHdrOut].dwBytesRecorded, 
					m_pHdr[m_nWriteCur].dwBytesRecorded - m_pHdr[m_nWriteCur].dwUser);
			memcpy(m_HdrOut[m_nHdrOut].lpData + m_HdrOut[m_nHdrOut].dwBytesRecorded, 
				m_pHdr[m_nWriteCur].lpData + m_pHdr[m_nWriteCur].dwUser, cb);
	
			m_pHdr[m_nWriteCur].dwUser += cb;
			m_HdrOut[m_nHdrOut].dwBytesRecorded += cb;
			m_HdrClear[m_nHdrOut] = 0;
			m_dwTotalSamples += cb / m_pcm.wf.nBlockAlign;
			if (m_pHdr[m_nWriteCur].dwUser == m_pHdr[m_nWriteCur].dwBytesRecorded) { 
				m_pHdr[m_nWriteCur].dwUser = 0;
				m_nWriteCur = (m_nWriteCur + 1) % m_cBuf;
				m_cWritten--;
				SetEvent(m_hEvent);
			}
		}

		waveOutWrite(m_hwo, &m_HdrOut[m_nHdrOut], sizeof(WAVEHDR));
		m_nHdrOut = (m_nHdrOut + 1) % SUBBUF_COUNT;
	}
}

BOOL COutput::PrepareSubBuffer()
{
	CAutoLock lock(&m_csecBuff);
	if (m_pbSubBuf)
		return FALSE;

	int cb = SUBBUF_SIZE;
	if (m_pcm.wf.nSamplesPerSec > 11025)
		cb *= 2;
	if (m_pcm.wf.nSamplesPerSec > 22050)
		cb *= 2;
	if (m_pcm.wBitsPerSample > 8)
		cb *= 2;
	cb *= m_pcm.wf.nChannels;

	m_pbSubBuf = new BYTE[cb * SUBBUF_COUNT];
	memset(m_pbSubBuf, 0, cb * SUBBUF_COUNT);
	for (UINT i = 0; i < SUBBUF_COUNT; i++) {
		memset(&m_HdrOut[i], 0, sizeof(WAVEHDR));
		m_HdrOut[i].lpData = (LPSTR)m_pbSubBuf + (cb * i);
		m_HdrOut[i].dwBufferLength = cb;
		m_HdrOut[i].dwUser = (DWORD)this;
		waveOutPrepareHeader(m_hwo, &m_HdrOut[i] , sizeof(WAVEHDR));
		m_HdrClear[i] = 1;
	}
	m_nHdrOut = 0;
	m_nWriteCur = 0;
	m_nSubBuf = SUBBUF_COUNT;
	m_dwTotalSamples = 0;

	m_hEventThread = CreateEvent(NULL, TRUE, TRUE, NULL);
	m_hThread = CreateThread(NULL, 0, WaveOutThreadProc, this, CREATE_SUSPENDED, &m_dwThreadId);
	if (!m_hThread)
		return FALSE;

#ifdef _WIN32_WCE
	SetThreadPriority(m_hThread, THREAD_PRIORITY_TIME_CRITICAL);
	HMODULE hModule = LoadLibrary(_T("Coredll.dll"));
	if (hModule) {
		BOOL (WINAPI *pCeSetThreadPriority)(HANDLE hThread, int nPriority); 
		(FARPROC&)pCeSetThreadPriority = GetProcAddress(hModule, _T("CeSetThreadPriority"));
		if (pCeSetThreadPriority) 
			pCeSetThreadPriority(GetCurrentThread(), 160);
		FreeLibrary(hModule);
	}
#else
	SetThreadPriority(m_hThread, THREAD_PRIORITY_TIME_CRITICAL);
#endif
	ResumeThread(m_hThread);
	return TRUE;
}

void COutput::FreeSubBuffer()
{
	CAutoLock lock(&m_csecBuff);
	if (m_pbSubBuf) {
		delete m_pbSubBuf;
		m_pbSubBuf = NULL;
	}
	memset(m_HdrOut, 0, sizeof(m_HdrOut));
}

DWORD COutput::GetBufferingSamples()
{
	DWORD dwCurrent = GetCurrent();
	return m_dwWritten - dwCurrent;
}

DWORD COutput::GetVolume()
{
	return m_dwVolume;
}

void COutput::SetVolume(DWORD dwVolume)
{
	m_dwVolume = dwVolume;
	if (m_hwo) {
		waveOutSetVolume(m_hwo, m_dwVolume);
	}
}
