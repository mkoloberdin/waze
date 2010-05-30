#if !defined(__DECODER_H_INCLUDED)
#define __DECODER_H_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../libmad/libmad.h"

///////////////////////////////////////////////////////////////
class CDecoder
{
public:
	CDecoder();
	~CDecoder();
	void Init();
	void Destroy();
	void Reset();
	int Decode(LPBYTE pbInput, DWORD cbInput, LPBYTE pbOutput, DWORD cbOutput, 
				LPDWORD pcbOutput, DWORD* pcbRead, int nResolution, BOOL fHalfSampleRate);
	void SetEqualizer(EQUALIZER* value);
	void GetEqualizer(EQUALIZER* value);
protected:
	HANDLE		m_hMad;
	EQUALIZER	m_Equalizer;
};

#endif //!__DECODER_H_INCLUDED