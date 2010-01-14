#if !defined(__READER_H_INCLUDED)
#define __READER_H_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "helper.h"

#define SCAN_BUFFER_SIZE	512

///////////////////////////////////////////////////////////////
class CReader
{
public:
	CReader();
	~CReader();
	BOOL Open(LPCTSTR pszFileName);
	void Close();
	LONGLONG SetPointer(LONGLONG llMove, DWORD dwMoveMethod);
	BOOL Read(LPBYTE pbBuff, DWORD cbBuff, LPDWORD pdwRead);
	LONGLONG GetSize();

	LONGLONG ScanAudioHeader(); /* scan MPEG Audio Header from Current Pointer */
	LONGLONG ScanAudioHeader(int nVersion, int nLayer);

	BOOL IsOpened()
	{
		return m_hFile == INVALID_HANDLE_VALUE ? FALSE : TRUE;
	}

protected:
	HANDLE m_hFile;
	CCritSec m_csec;
};
#endif //!__READER_H_INCLUDED