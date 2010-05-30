#ifndef __EFFECT_H__
#define __EFFECT_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

///////////////////////////////////////////////////////////////
class CSurroundEffect  
{
public:
	CSurroundEffect();
	virtual ~CSurroundEffect();

	void SetParameter(EFFECT* value);
	void GetParameter(EFFECT* value);
	void Process(int nResolution, void* pBuf, DWORD cbSize);

protected:
	BOOL m_fEnable;
	int m_nRate;
	int m_nPreamp;
};

///////////////////////////////////////////////////////////////
class CDelayEffect
{
public:
	CDelayEffect(BOOL fEcho);
	~CDelayEffect();
	void Open(int nChannels, int nSamplingRate);
	void Close();
	void Process(int nResolution, void* pBuf, DWORD cbSize);
	void GetParameter(EFFECT* value);
	void SetParameter(EFFECT* value);
	void Reset();

protected:
	short DelayEffect(short s, int nChannel);
	
	int m_fEcho;
	int m_nChannels;
	int m_nSamplingRate;
	int m_nReadPos;
	int m_nWritePos;
	int m_cBuf; 
	int* m_pBuf;
	CCritSec m_csec;

	BOOL m_fEnable;
	int m_nDelay;
	int m_nRate;
};

#define UCHAR_MID	0x80
#define UINT8_TO_INT16(x) (((x) - UCHAR_MID) << 8)
#define INT16_TO_UINT8(x) (((x) >> 8) + UCHAR_MID)
///////////////////////////////////////////////////////////////

//#ifdef _WIN32_WCE
#define FIXED_BASSBOOOST // ââéZê∏ìxÇ™ïÇìÆè¨êîì_ÇÊÇËÇÊÇ≠Ç»Ç¡ÇƒÇµÇ‹Ç¡ÇΩ...
//#endif

#ifdef FIXED_BASSBOOOST
#define BB_FIXED_T	LONGLONG
#define BB_FIXED_T2	int
#else
#define BB_FIXED_T	double
#define BB_FIXED_T2 double
#endif
class CBassBoost
{
public:
	CBassBoost();
	~CBassBoost();

	int GetLevel() {return m_nLevel;}
	void SetLevel(int nLevel);

	void Open(int nChannels, int nSampleRate);
	void Close();
	void Reset();
	void Process(int nResolution, void* pBuf, DWORD cbSize);

protected:
	BB_FIXED_T	a[3];
	BB_FIXED_T	b[3];
	BB_FIXED_T2	xn[2][2];
	BB_FIXED_T2	yn[2][2];
	
	int m_nChannels;
	int m_nSampleRate;
	int m_nLevel;
	int m_nPreamp;

	CCritSec m_csec;
};

class C3DChorus
{
public:
	C3DChorus();
	~C3DChorus();
	void Open(int nSampleRate);
	void Process(int nResolution, LPBYTE pbBuf, DWORD cbSize);
	void Close();
	void Reset();
	void SetParameter(EFFECT* value);
	void GetParameter(EFFECT* value);

protected:
	short Chorus(short s, int nChannel);

	CCritSec m_csec;
	BOOL	m_fEnable;

	int		m_nRate;
	int		m_nPreamp;
	int		m_nSampleRate;
	int		m_nWritePos[2];
	int		m_nCyclePos[2];
	int		m_nDelayOffset;
	int		m_nDepth;

	int*	m_pBuf[2];
};

#endif // __EFFECT_H__