#ifndef __MULTIBUFF_H__
#define __MULTIBUFF_H__

typedef int (CALLBACK *LPCOMPAREFUNC)(LPARAM, LPARAM, LPARAM);
typedef BOOL (CALLBACK *LPFINDFUNC)(LPARAM, LPARAM);

#define __CHECK_VALUE

class CMultiBuff
{
protected:
	DWORD* m_pData;
	int m_nSize;
	
public:
	CMultiBuff();
	~CMultiBuff();

	DWORD GetAt(int iPos);
	void SetAt(int iPos, DWORD dwValue);
	DWORD RemoveAt(int iPos);
	void RemoveAll();
	
	void Insert(DWORD dwValue, int nIndex);
	int Add(DWORD dwValue);

	int GetCount();
	BOOL IsEmpty();
	BOOL Sort(LPCOMPAREFUNC pfnCompare, DWORD dwParam);
	int Find(LPFINDFUNC pfnFind, int iStart, DWORD dwParam);
};

#endif //!__MULTIBUFF_H__