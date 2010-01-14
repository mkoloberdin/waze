#include <windows.h>
#include "maplay.h"
#include "mahelper.h"
#include "reader.h"

CReader::CReader()
{
	m_hFile = INVALID_HANDLE_VALUE;
}

CReader::~CReader()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
		Close();
}

BOOL CReader::Open(LPCTSTR pszFileName)
{
	CAutoLock lock(&m_csec);
	m_hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, 
						NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	return TRUE;
}

void CReader::Close()
{
	CAutoLock lock(&m_csec);
	if (m_hFile != INVALID_HANDLE_VALUE)
		CloseHandle(m_hFile);

	m_hFile = INVALID_HANDLE_VALUE;
}

LONGLONG CReader::SetPointer(LONGLONG llMove, DWORD dwMoveMethod)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return MAXLONGLONG;

	CAutoLock lock(&m_csec);
	LARGE_INTEGER nMove;
	nMove.QuadPart = llMove;
	nMove.LowPart = SetFilePointer(m_hFile, nMove.LowPart, &nMove.HighPart, dwMoveMethod);
	return nMove.QuadPart;
}

BOOL CReader::Read(LPBYTE pbBuff, DWORD cbBuff, LPDWORD pdwRead)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	CAutoLock lock(&m_csec);
	BOOL fRet = ReadFile(m_hFile, pbBuff, cbBuff, pdwRead, NULL);
	return fRet;
}

LONGLONG CReader::ScanAudioHeader(int nVersion, int nLayer)
{
	CAutoLock lock(&m_csec);

	DWORD cbLeft, cbRead;
	BYTE bInput[SCAN_BUFFER_SIZE];

	LPBYTE pbCurrent = bInput;
	LONGLONG llRet = SetPointer(0, FILE_CURRENT);

	Read(pbCurrent, SCAN_BUFFER_SIZE, &cbRead);
	cbLeft = cbRead; 
	while (cbLeft > 3)
	{
		while (cbLeft > 3)
		{
			if (CheckAudioHeader(pbCurrent))
			{
				int v = (*(pbCurrent + 1) & 0x10) ?
					2 - ((*(pbCurrent + 1) & 0x08) >> 3): 3;
				int l = 4 - (((*(pbCurrent + 1)) & 0x06) >> 1);
				if (v == nVersion && l == nLayer)
				{
					llRet = SetPointer(0, FILE_CURRENT) - cbLeft;
					SetPointer(llRet, FILE_BEGIN);
					return llRet;
				}
			}
			pbCurrent++;
			cbLeft--;
		}
		memmove(bInput, pbCurrent, cbLeft);
		pbCurrent = bInput + cbLeft;
		cbRead = SCAN_BUFFER_SIZE - cbLeft;
		Read(pbCurrent, cbRead, &cbRead);
		cbLeft += cbRead;
		pbCurrent = bInput;
	}
	return MAXLONGLONG;
}

LONGLONG CReader::ScanAudioHeader()
{
	CAutoLock lock(&m_csec);

	DWORD cbLeft, cbRead;
	BYTE bInput[SCAN_BUFFER_SIZE];

	LPBYTE pbCurrent = bInput;
	LONGLONG llRet = SetPointer(0, FILE_CURRENT);

	Read(pbCurrent, SCAN_BUFFER_SIZE, &cbRead);
	cbLeft = cbRead; 
	while (cbLeft > 3)
	{
		while (cbLeft > 3)
		{
			if (CheckAudioHeader(pbCurrent))
			{
				llRet = SetPointer(0, FILE_CURRENT) - cbLeft;
				SetPointer(llRet, FILE_BEGIN);
				return llRet;
			}
			pbCurrent++;
			cbLeft--;
		}
		memmove(bInput, pbCurrent, cbLeft);
		pbCurrent = bInput + cbLeft;
		cbRead = SCAN_BUFFER_SIZE - cbLeft;
		Read(pbCurrent, cbRead, &cbRead);
		cbLeft += cbRead;
		pbCurrent = bInput;
	}
	return MAXLONGLONG;
}

LONGLONG CReader::GetSize()
{
	ULARGE_INTEGER nSize;
	nSize.LowPart = GetFileSize(m_hFile, &nSize.HighPart);
	return nSize.QuadPart;
}