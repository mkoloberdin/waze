#if !defined(__WAVEOUTPUT_H_INCLUDED)
#define __WAVEOUTPUT_H_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define PREAMP_FIXED_BITS		8
#define PREAMP_FIXED_FLAT		(1 << PREAMP_FIXED_BITS)
#define BUFLEN_BASE				1024
#define SUBBUF_SIZE				512
#ifdef _WIN32_WCE
#define SUBBUF_COUNT			8
#else
#define SUBBUF_COUNT			4
#endif

class COutput
{
public:
	COutput();
	~COutput();
	BOOL Open(int nChannels, int nSamplingRate, int nBitsPerSample);
	void Close();
	void CloseAll();
	DWORD GetBufferSize() {return m_cbBuf;}
	WAVEHDR* GetBuffer();
	void OutputBuffer(WAVEHDR*);
	void ReleaseBuffer(WAVEHDR*);

	void Pause(BOOL fPause);
	void Reset();
	DWORD GetCurrent();
	BOOL IsFlushed();
	void Preamp(LPBYTE pbBuf, DWORD cbBuf, int nRate);
	DWORD ScanZeroSamples(BOOL fForward, LPBYTE pbBuf, DWORD cbBuf);
	void SetFadeOff() {m_nFadeSamples = 0;}

	BOOL SetOutputParam(DWORD dwBufLen, BOOL fDoubleBuf, BOOL fFade);
	void GetOutputParam(LPDWORD pdwBufLen, LPBOOL pfDoubleBuf, LPBOOL pfFade);
	BOOL GetScanPeek() {return m_fScanPeek;}
	void SetScanPeek(BOOL fScan) {m_fScanPeek = fScan;}

	DWORD GetBufferCount() {return m_cBuf;}
	DWORD GetBufferingCount() {return m_cWritten;}
	DWORD GetBufferingSamples();
	void GetBufferInfo(DWORD* pcbTotal, DWORD* pcbBuffered);

	int			m_nLPeek;
	int			m_nRPeek;

protected:
	void PutBuffer(WAVEHDR*);
	BOOL PrepareBuffer();
	void FreeBuffer();
	void CheckPeek(WAVEHDR* pHdr);
	static void CALLBACK WaveOutCallback(HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

	DWORD		m_dwBufLen;
	BOOL		m_fScanPeek;
	HWAVEOUT	m_hwo;
	HANDLE		m_hEvent;
	WAVEHDR*	m_pHdr;
	DWORD		m_cBuf;
	DWORD		m_cWritten;
	DWORD		m_nCurrent;
	LPBYTE		m_pbBuf;
	DWORD		m_cbBuf;
	DWORD		m_dwWritten;

	CCritSec	m_csecBuff;
	CCritSec	m_csecDevice;

	PCMWAVEFORMAT m_pcm;

	BOOL	m_fFade;
	int		m_nFadeRate;
	int		m_nFadeCurrent;
	int		m_nFadeSamples;
	void FadeIn(LPBYTE pbBuf, DWORD cbBuf);

protected:
	static DWORD WINAPI WaveOutThreadProc(LPVOID pParam);
	static void CALLBACK WaveOutCallback2(HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
	void WaveOutThread();
	BOOL PrepareSubBuffer();
	void FreeSubBuffer();

	BOOL		m_fPaused;
	DWORD		m_dwTotalSamples;
	BOOL		m_fDoubleBuf;
	HANDLE		m_hEventThread;
	HANDLE		m_hThread;
	DWORD		m_dwThreadId;
	DWORD		m_nWriteCur;
	DWORD		m_nSubBuf;
	WAVEHDR		m_HdrOut[SUBBUF_COUNT];
	BOOL		m_HdrClear[SUBBUF_COUNT];
	int			m_nHdrOut;
	LPBYTE		m_pbSubBuf;

public:
	DWORD GetVolume();
	void SetVolume(DWORD dwVolume);

protected:
	DWORD		m_dwVolume;
};

#endif //!__WAVEOUTPUT_H_INCLUDED