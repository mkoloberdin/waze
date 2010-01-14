#include <windows.h>
#include "maplay.h"
#include "decoder.h"

CDecoder::CDecoder()
{
	m_hMad = NULL;

	for (int i = 0; i < 10; i++) {
		m_Equalizer.data[i] = 31;
	}
	m_Equalizer.preamp = 31;
	m_Equalizer.fEnable = FALSE;
}

CDecoder::~CDecoder()
{
}

void CDecoder::Init()
{
	m_hMad = mad_init();
	mad_seteq(m_hMad, (equalizer_value*)&m_Equalizer);
}

void CDecoder::Destroy()
{
	if (m_hMad) {
		mad_uninit(m_hMad);
		m_hMad = NULL;
	}
}

void CDecoder::Reset()
{
	if (m_hMad) {
		Destroy();
		Init();
	}
}

int CDecoder::Decode(LPBYTE pbInput, DWORD cbInput, LPBYTE pbOutput, 
					 DWORD cbOutput, LPDWORD pcbOutput, DWORD* pcbRead, int nResolution, BOOL fHalfSampleRate)
{
	*pcbRead = 0;
	*pcbOutput = 0;
	int cbProceed, cbRead;

	if (!m_hMad)
		return 0;

	int nRet = mad_decode(m_hMad, (char*)pbInput, cbInput, (char*)pbOutput, 
							cbOutput, &cbRead, &cbProceed, nResolution, fHalfSampleRate);
	*pcbRead += cbRead;
	*pcbOutput += cbProceed;
	return nRet;
}

void CDecoder::SetEqualizer(EQUALIZER* value)
{
	m_Equalizer = *value;
	if (m_hMad) {
		mad_seteq(m_hMad, (equalizer_value*)&m_Equalizer);
	}
}

void CDecoder::GetEqualizer(EQUALIZER* value)
{
	*value = m_Equalizer;
}