#ifndef __HELPER_H__
#define __HELPER_H__

class CCritSec
{
public:
	CCritSec()
	{
		m_cRef = 0;
		InitializeCriticalSection(&m_csec);
	}
	~CCritSec()
	{
		DeleteCriticalSection(&m_csec);
	}
	void Lock()
	{
		EnterCriticalSection(&m_csec);
		m_cRef++;
	}
	void Unlock()
	{
		if (m_cRef) {
			LeaveCriticalSection(&m_csec);
			m_cRef--;
		}
	}
protected:
	CRITICAL_SECTION m_csec;
	int m_cRef;
};

class CAutoLock
{
public:
	CAutoLock(CCritSec* pcsec)
	{
		m_pcsec = pcsec;
		m_pcsec->Lock();
	}
	~CAutoLock()
	{
		m_pcsec->Unlock();
	}
protected:
	CCritSec* m_pcsec;
};

class CEvent
{
public:
	CEvent(BOOL fInitialState)
	{
		m_hEvent = CreateEvent(NULL, TRUE, fInitialState, NULL);
	}
	~CEvent()
	{
		CloseHandle(m_hEvent);
	}
	BOOL Set()
	{
		return SetEvent(m_hEvent);
	}
	BOOL Reset()
	{
		return ResetEvent(m_hEvent);
	}
	BOOL Pulse()
	{
		return PulseEvent(m_hEvent);
	}
	BOOL Lock(DWORD dwTimeOut = INFINITE)
	{
		return WaitForSingleObject(m_hEvent, dwTimeOut) != WAIT_TIMEOUT;
	}
	operator HANDLE() {return m_hEvent;}
protected:
	HANDLE m_hEvent;
};

#endif // __HELPER_H__