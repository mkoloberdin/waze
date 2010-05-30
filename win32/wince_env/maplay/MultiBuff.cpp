#include <windows.h>
#include "multibuff.h"

//#define __CHECK_VALUE

LPCOMPAREFUNC g_pfnCompare = NULL;
DWORD g_dwParam = NULL;

CMultiBuff::CMultiBuff()
{
	m_pData = NULL;
	m_nSize = 0;
}

CMultiBuff::~CMultiBuff()
{
	RemoveAll();
}

DWORD CMultiBuff::GetAt(int iPos) //zero base
{
#ifdef __CHECK_VALUE
	if (iPos >= m_nSize || iPos < 0)
		return NULL;
#endif
	return m_pData[iPos];
}

void CMultiBuff::SetAt(int iPos, DWORD dwValue)
{
#ifdef __CHECK_VALUE
	if (iPos >= m_nSize || iPos < 0)
		return;
#endif
	m_pData[iPos] = dwValue;
}

DWORD CMultiBuff::RemoveAt(int iPos) //‚±‚ÌŠÖ”‚Å‚ÍŽÀÛ‚Éƒƒ‚ƒŠ‚ÍŠJ•ú‚µ‚È‚¢
{
#ifdef __CHECK_VALUE
	if (iPos >= m_nSize || iPos < 0)
		return NULL;
#endif

	DWORD dwValue = m_pData[iPos];

	//move block
	if (m_nSize > iPos && m_nSize > 1)
	{
		memmove(&m_pData[iPos], &m_pData[iPos + 1], sizeof(DWORD)*(m_nSize - 1 - iPos));
	}

	if (!--m_nSize)
		RemoveAll();

	return dwValue;
}

void CMultiBuff::RemoveAll()
{
	if (m_pData)
		free(m_pData);
	m_pData = NULL;
	m_nSize = 0;
}

void CMultiBuff::Insert(DWORD dwValue, int nIndex)
{
	m_pData = (DWORD*)realloc(m_pData, sizeof(DWORD)*++m_nSize);
	memmove(&m_pData[nIndex + 1], &m_pData[nIndex], sizeof(DWORD)*(m_nSize - 1));
	m_pData[nIndex] = dwValue;
}

int CMultiBuff::Add(DWORD dwValue)
{
	m_pData = (DWORD*)realloc(m_pData, sizeof(DWORD)*++m_nSize);
	m_pData[m_nSize - 1] = dwValue;
	return m_nSize - 1;
}

int CMultiBuff::GetCount()
{
	return m_nSize;
}

BOOL CMultiBuff::IsEmpty()
{
	return m_pData ? FALSE : TRUE;
}

#ifdef _WIN32_WCE_EMULATION
int __cdecl SortCompareCallback(const void *arg1, const void *arg2)
#else
int SortCompareCallback(const void *arg1, const void *arg2)
#endif
{
	DWORD* pdwValue1 = (DWORD*)arg1;
	DWORD* pdwValue2 = (DWORD*)arg2;
	return g_pfnCompare(*pdwValue1, *pdwValue2, g_dwParam);
}

BOOL CMultiBuff::Sort(LPCOMPAREFUNC pfnCompare, DWORD dwParam) // execute quick sort
{
	if (!m_nSize || !pfnCompare) return FALSE;

	g_pfnCompare = pfnCompare;
	g_dwParam = dwParam;
	qsort(m_pData, m_nSize, sizeof(DWORD), SortCompareCallback);
	g_pfnCompare = NULL;
	g_dwParam = NULL;

	return TRUE;
}

int CMultiBuff::Find(LPFINDFUNC pfnFind, int iStart, DWORD dwParam)
{
	for (int i=0; i<m_nSize; i++)
	{
		if (!pfnFind(m_pData[i], dwParam))
			return i;
	}
	return -1;
}