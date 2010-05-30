#include <windows.h>
#include "maplay.h"
#include "helper.h"
#include "effect.h"
#include <math.h>

short Clip16(int s)
{
	int nRet = s;
	if (nRet > 32767)
		nRet = 32767;
	else if (nRet < -32768)
		nRet = -32768;
	return (short)nRet;
}

unsigned char Clip8(int s)
{
	int nRet = s;
	if (nRet > 255)
		nRet = 255;
	else if (nRet < 0)
		nRet = 0;
	return (unsigned char)nRet;
}

#define SURROUND_PREAMP		(-0.035)

CSurroundEffect::CSurroundEffect()
{
	m_fEnable = FALSE;
	m_nRate = 20;
	m_nPreamp = (int)(pow(10, SURROUND_PREAMP * m_nRate / 20) * 100);
}

CSurroundEffect::~CSurroundEffect()
{
}

void CSurroundEffect::SetParameter(EFFECT* value)
{
	m_fEnable = value->fEnable;
	m_nRate = max(min(value->nRate, 100), 0);
	m_nPreamp = (int)(pow(10, SURROUND_PREAMP * m_nRate / 20) * 100);
}

void CSurroundEffect::GetParameter(EFFECT* value)
{
	value->fEnable = m_fEnable;
	value->nRate = m_nRate;
	value->nDelay = 0;
}

void CSurroundEffect::Process(int nResolution, void* pBuf, DWORD cbSize)
{
	if (!m_fEnable || !m_nRate)
		return;

	if (nResolution == 8) {
		unsigned char* pSample = (unsigned char*)pBuf;
		int nSamples = cbSize / (sizeof(unsigned char) * 2);
		while (nSamples) {
			int l = UINT8_TO_INT16((int)*pSample);
			int r = UINT8_TO_INT16((int)*(pSample + 1));
			*pSample++ = Clip8(INT16_TO_UINT8((l + (l - r) * m_nRate / 100)) * m_nPreamp / 100);
			*pSample++ = Clip8(INT16_TO_UINT8((r + (r - l) * m_nRate / 100)) * m_nPreamp / 100);
			nSamples--;
		}
	}
	else if (nResolution == 16) {
		short* pSample = (short*)pBuf;
		int nSamples = cbSize / (sizeof(short) * 2);
		while (nSamples) {
			int l = *pSample;
			int r = *(pSample + 1);
			*pSample++ = Clip16((l + (l - r) * m_nRate / 100) * m_nPreamp / 100);
			*pSample++ = Clip16((r + (r - l) * m_nRate / 100) * m_nPreamp / 100);
			nSamples--;
		}
	}	
}

CDelayEffect::CDelayEffect(BOOL fEcho)
{
	m_nChannels = 0;
	m_nSamplingRate = 0;
	m_nReadPos = 0;
	m_nWritePos = 0;
	m_cBuf = 0;
	m_pBuf = NULL;

	m_fEcho = fEcho;
	m_fEnable = FALSE;
	m_nDelay = 100;
	m_nRate = 20;
}

CDelayEffect::~CDelayEffect()
{
}

void CDelayEffect::Open(int nChannels, int nSamplingRate)
{
	m_nChannels = nChannels;
	m_nSamplingRate = nSamplingRate;
	if (!nChannels || !nSamplingRate)
		return;
	
	if (!m_fEnable)
		return;
		
	if (!m_nRate || !m_nDelay)
		return;

	m_cBuf = (m_nDelay * nSamplingRate / 1000) * nChannels;
	if (m_cBuf & 1)
		m_cBuf = (m_cBuf & 0xFFFFFFF0) + 1;
	m_cBuf += 1;

	m_pBuf = new int[m_cBuf];
	memset(m_pBuf, 0, sizeof(int) * m_cBuf);
	m_nWritePos = 0;
	m_nReadPos = 1;
}

void CDelayEffect::Close()
{
	if (!m_pBuf) {
		delete [] m_pBuf;
		m_pBuf = NULL;
	}
}

void CDelayEffect::Process(int nResolution, void* pBuf, DWORD cbSize)
{
	CAutoLock lock(&m_csec);
	if (!m_fEnable)
		return;

	if (!m_nChannels || !m_nSamplingRate)
		return;

	if (!m_nDelay || !m_nRate)
		return;

	if (!m_pBuf)
		return;

	if (nResolution == 8) {
		unsigned char* pSample = (unsigned char*)pBuf;
		int nSamples = cbSize / sizeof(unsigned char) / m_nChannels;
		while (nSamples--) {
			for (int i = 0; i < m_nChannels; i++) {
				*pSample = INT16_TO_UINT8(DelayEffect(UINT8_TO_INT16(*pSample), i));
				pSample++;
			}
		}
	}
	else if (nResolution == 16) {
		short* pSample = (short*)pBuf;
		int nSamples = cbSize / sizeof(short) / m_nChannels;
		while (nSamples--) {
			for (int i = 0; i < m_nChannels; i++) {
				*pSample = DelayEffect(*pSample, i);
				pSample++;
			}
		}
	}
}

void CDelayEffect::SetParameter(EFFECT* value)
{	
	CAutoLock lock(&m_csec);
	int nChannels, nSamplingRate;
	nChannels = m_nChannels; nSamplingRate = m_nSamplingRate;
	Close();

	m_fEnable = value->fEnable; 
	m_nDelay = value->nDelay;
	m_nRate = value->nRate;

	Open(m_nChannels, m_nSamplingRate);
}

void CDelayEffect::GetParameter(EFFECT* value)
{	
	value->fEnable = m_fEnable; 
	value->nDelay = m_nDelay;
	value->nRate = m_nRate;
}

short CDelayEffect::DelayEffect(short s, int nChannel)
{
	int* p;
	int nRet;

	p = (int*)m_pBuf;

	if (m_fEcho)
		*(p + m_nWritePos++) = s;

	nRet = s + *(p + m_nReadPos++) * m_nRate / 100;

	nRet = Clip16(nRet);
	if (!m_fEcho)
		*(p + m_nWritePos++) = nRet;

	if (m_nReadPos == m_cBuf)
		m_nReadPos = 0;
	if (m_nWritePos == m_cBuf)
		m_nWritePos = 0;
	return (short)nRet;
}

void CDelayEffect::Reset()
{
	if (!m_pBuf)
		return;

	CAutoLock lock(&m_csec);
	memset(m_pBuf, 0, sizeof(int) * m_cBuf);
}

CBassBoost::CBassBoost()
{
	m_nChannels = m_nSampleRate = 0;
	m_nLevel = 0;
	m_nPreamp = 100;
}

CBassBoost::~CBassBoost()
{
}

#define BASSBOOST_PREAMP		(-0.4)
#define BASSBOOST_FREQUENCY		145
#ifdef FIXED_BASSBOOOST
#define FIXED_FIG				24
#define FIXED_BASE				(1 << FIXED_FIG)
#define TO_FIXED_POINT(x)		((BB_FIXED_T)((x) * FIXED_BASE))
#define FROM_FIXED_POINT(x)		((BB_FIXED_T2)((x) >> FIXED_FIG))
#define ADD_PREAMP(x)			((x) << 8)
#define REMOVE_PREAMP(x)		((x) >> 8)
#else
#define TO_FIXED_POINT(x)		((BB_FIXED_T)(x))
#define FROM_FIXED_POINT(x)		((BB_FIXED_T2)(x))
#define ADD_PREAMP(x)			((BB_FIXED_T2)((x) << 8))
#define REMOVE_PREAMP(x)		((int)(x) >> 8)
#endif

void CBassBoost::Open(int nChannels, int nSampleRate)
{
	m_nChannels = nChannels;
	m_nSampleRate = nSampleRate;

	if (!m_nLevel || !m_nChannels || !m_nSampleRate)
		return;

	double omega = 2 * 3.141592653589 * BASSBOOST_FREQUENCY / nSampleRate;
	double sn = sin(omega);
	double cs = cos(omega);
	double ax = exp(log(10) * (double)m_nLevel * 0.35 / 40);
	double shape = 7.0;
	double beta = sqrt((ax * ax + 1) / shape - (pow((ax - 1), 2)));
	double b0 = ax * ((ax + 1) - (ax - 1) * cs + beta * sn);
	double b1 = 2 * ax * ((ax - 1) - (ax + 1) * cs);
	double b2 = ax * ((ax + 1) - (ax - 1) * cs - beta * sn);
	double a0 = ((ax + 1) + (ax - 1) * cs + beta * sn);
	double a1 = -2 * ((ax - 1) + (ax + 1) * cs);
	double a2 = (ax + 1) + (ax - 1) * cs - beta * sn;

	b[0] = TO_FIXED_POINT(b0 / a0);
	b[1] = TO_FIXED_POINT(b1 / a0);
	b[2] = TO_FIXED_POINT(b2 / a0);
	a[1] = TO_FIXED_POINT(a1 / a0);
	a[2] = TO_FIXED_POINT(a2 / a0);

	memset(xn, 0, sizeof(xn));
	memset(yn, 0, sizeof(yn));
	m_nPreamp = (int)(pow(10, BASSBOOST_PREAMP * m_nLevel / 20) * 100);
}

void CBassBoost::Close()
{
	m_nChannels = m_nSampleRate = 0;
}

void CBassBoost::Reset()
{
	memset(xn, 0, sizeof(xn));
	memset(yn, 0, sizeof(yn));
}

void CBassBoost::Process(int nResolution, void* pBuf, DWORD nSize)
{
	CAutoLock lock(&m_csec);
	if (!m_nLevel || !m_nChannels || !m_nSampleRate)
		return;

	if (nResolution == 8) {
		unsigned char* pSample = (unsigned char*)pBuf;
		int nSamples = nSize / sizeof(unsigned char) / m_nChannels;

		BB_FIXED_T2 in, out;
		for (int i = 0; i < nSamples; i++) {
			in = (BB_FIXED_T2)ADD_PREAMP(UINT8_TO_INT16((int)*pSample));
			out = FROM_FIXED_POINT((b[0] * in)
									+ (b[1] * xn[0][0])
									+ (b[2] * xn[1][0])
									- (a[1] * yn[0][0])
									- (a[2] * yn[1][0]));

			xn[1][0] = xn[0][0];
			xn[0][0] = in;
			yn[1][0] = yn[0][0];
			yn[0][0] = out;
			*pSample++ = Clip8(INT16_TO_UINT8(REMOVE_PREAMP(out)* m_nPreamp / 100));

			if (m_nChannels == 2) {
				in = (BB_FIXED_T2)ADD_PREAMP(UINT8_TO_INT16((int)*pSample));
				out = FROM_FIXED_POINT((b[0] * in)
									+ (b[1] * xn[0][0])
									+ (b[2] * xn[1][0])
									- (a[1] * yn[0][0])
									- (a[2] * yn[1][0]));

				xn[1][1] = xn[0][0];
				xn[0][1] = in;
				yn[1][1] = yn[0][0];
				yn[0][1] = out;
				*pSample++ = Clip8(INT16_TO_UINT8(REMOVE_PREAMP(out) * m_nPreamp / 100));
			}
		}
	}
	else if (nResolution == 16) {
		short* pSample = (short*)pBuf;
		int nSamples = nSize / sizeof(short) / m_nChannels;

		BB_FIXED_T2 in, out;
		for (int i = 0; i < nSamples; i++) {
			in = (BB_FIXED_T2)ADD_PREAMP(*pSample);
			out = FROM_FIXED_POINT((b[0] * in)
									+ (b[1] * xn[0][0])
									+ (b[2] * xn[1][0])
									- (a[1] * yn[0][0])
									- (a[2] * yn[1][0]));

			xn[1][0] = xn[0][0];
			xn[0][0] = in;
			yn[1][0] = yn[0][0];
			yn[0][0] = out;
			*pSample++ = Clip16(REMOVE_PREAMP(out) * m_nPreamp / 100);

			if (m_nChannels == 2) {
				in = (BB_FIXED_T2)ADD_PREAMP(*pSample);
				out = FROM_FIXED_POINT((b[0] * in)
										+ (b[1] * xn[0][0])
										+ (b[2] * xn[1][0])
										- (a[1] * yn[0][0])
										- (a[2] * yn[1][0]));
				xn[1][1] = xn[0][0];
				xn[0][1] = in;
				yn[1][1] = yn[0][0];
				yn[0][1] = out;
				*pSample++ = Clip16(REMOVE_PREAMP(out)* m_nPreamp / 100);
			}
		}
	}
}

void CBassBoost::SetLevel(int nLevel)
{
	CAutoLock lock(&m_csec);
	int nChannels = m_nChannels;
	int nSampleRate = m_nSampleRate;
	Close();

	m_nLevel = min(max(0, nLevel), 20);
	Open(nChannels, nSampleRate);
}

#define CHORUS_PREAMP		(-0.035)
#define CHORUS_DELAY	6
#define CHORUS_DEPTH	3
#ifdef _WIN32_WCE
#define CHORUS_FIXED_T	int
#define CHORUS_BITS		8
#define CHORUS_BASE		((CHORUS_FIXED_T)1 << CHORUS_BITS)
#else
#define CHORUS_FIXED_T	LONGLONG
#define CHORUS_BITS		31
#define CHORUS_BASE		((CHORUS_FIXED_T)1 << CHORUS_BITS)
#endif

C3DChorus::C3DChorus()
{
	m_fEnable = FALSE;
	m_nRate = 20;

	m_nSampleRate = 0;
	m_pBuf[0] = m_pBuf[1] = NULL;

	Reset();
}

C3DChorus::~C3DChorus()
{
	Close();
}

void C3DChorus::Open(int nSampleRate)
{
	m_pBuf[0] = new int[nSampleRate];
	m_pBuf[1] = new int[nSampleRate];
	
	m_nDelayOffset = nSampleRate * CHORUS_DELAY / 1000;
	m_nDepth = ((CHORUS_FIXED_T)CHORUS_DEPTH << (CHORUS_BITS + 1)) / 1000;
	m_nSampleRate = nSampleRate;

	Reset();
}

void C3DChorus::Process(int nResolution, LPBYTE pbBuf, DWORD cbSize)
{
	CAutoLock lock(&m_csec);
	if (!m_pBuf || !m_fEnable || m_nRate == 0)
		return;
	
	if (nResolution == 16) {
		short* pSample = (short*)pbBuf;
		int nSamples = cbSize / (sizeof(short) * 2);
		while (nSamples) {
			*pSample++ = Chorus(*pSample, 0);
			*pSample++ = Chorus(*pSample, 1);
			nSamples--;
		}
	}
	else if (nResolution == 8) {
		unsigned char* pSample = (unsigned char*)pbBuf;
		int nSamples = cbSize / (sizeof(unsigned char) * 2);
		while (nSamples) {
			*pSample++ = INT16_TO_UINT8(Chorus(UINT8_TO_INT16(*pSample), 0));
			*pSample++ = INT16_TO_UINT8(Chorus(UINT8_TO_INT16(*pSample), 1));
			nSamples--;
		}
	}
}

void C3DChorus::Close()
{
	if (m_pBuf[0]) {
		delete m_pBuf[0];
		m_pBuf[0] = NULL;
	}
	if (m_pBuf[1]) {
		delete m_pBuf[1];
		m_pBuf[1] = NULL;
	}
	m_nSampleRate = 0;
}

void C3DChorus::Reset()
{
	CAutoLock lock(&m_csec);
	m_nCyclePos[0] = 0;
	m_nCyclePos[1] = m_nSampleRate / 2;
	m_nWritePos[0] = m_nWritePos[1] = 0;
	if (m_pBuf[0])
		memset(m_pBuf[0], 0, sizeof(int) * m_nSampleRate);
	if (m_pBuf[1])
		memset(m_pBuf[1], 0, sizeof(int) * m_nSampleRate);

	m_nPreamp = (int)(pow(10, CHORUS_PREAMP * m_nRate / 20) * 100);
}

short C3DChorus::Chorus(short s, int nChannel)
{
	CHORUS_FIXED_T s2 = (CHORUS_FIXED_T)s << CHORUS_BITS;
	CHORUS_FIXED_T offset;

	offset = ((CHORUS_FIXED_T)m_nWritePos[nChannel] - m_nDelayOffset) << CHORUS_BITS;
	if (m_nCyclePos[nChannel] < m_nSampleRate / 2)
		offset -= (CHORUS_FIXED_T)m_nDepth * m_nCyclePos[nChannel];
	else
		offset -= (CHORUS_FIXED_T)m_nDepth * (m_nSampleRate - m_nCyclePos[nChannel]);

	if (offset < 0)
		offset += (CHORUS_FIXED_T)(m_nSampleRate - 1) << CHORUS_BITS;

	int offset1 = (int)(offset >> CHORUS_BITS);
	int offset2 = offset1 + 1;
	if (offset2 == m_nSampleRate - 1)
		offset2 = 0;

	int mod = (int)(offset % CHORUS_BASE);
	CHORUS_FIXED_T s3 = (CHORUS_FIXED_T)m_pBuf[nChannel][offset1] * (CHORUS_BASE - mod);
	s3 += (CHORUS_FIXED_T)m_pBuf[nChannel][offset2] * mod;

	s2 -= (s3 * m_nRate / 100);

	m_pBuf[nChannel][m_nWritePos[nChannel]] = s;

	m_nWritePos[nChannel]++;
	if (m_nWritePos[nChannel] == m_nSampleRate - 1)
		m_nWritePos[nChannel] = 0;
	
	m_nCyclePos[nChannel]++;
	if (m_nCyclePos[nChannel] == m_nSampleRate - 1)
		m_nCyclePos[nChannel] = 0;

	return Clip16((int)((s2 * m_nPreamp / 100) >> CHORUS_BITS));
}

void C3DChorus::SetParameter(EFFECT* value)
{
	m_fEnable = value->fEnable;
	m_nRate = value->nRate;
	Reset();
}

void C3DChorus::GetParameter(EFFECT* value)
{
	value->fEnable = m_fEnable;
	value->nRate = m_nRate;
}
