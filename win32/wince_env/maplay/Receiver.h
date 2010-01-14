#if !defined(__RECEIVER_H_INCLUDED)
#define __RECEIVER_H_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winsock.h>
#include "helper.h"
#include "MultiBuff.h"

#define	RECV_BUFF_SIZE			2048
#define RECV_HTTP				80
#define RECV_INTERVAL			200000		// microseconds
#define RECV_RECEIVE_TIMEOUT	5000000		// 5sec
#define RECV_CONNECT_TIMEOUT	15000000	// 15sec
#define DEF_USER_AGENT			_T("Unknown")

#define ICY_REDIRECT	302
#define ICY_OK			200

typedef struct _tagRecvBuf
{
	BYTE pbBuf[RECV_BUFF_SIZE];
	DWORD cbRecv;
	DWORD cbUsed;
	_tagRecvBuf() {cbRecv = 0; cbUsed = 0;}
}RECV_BUF;

class CReceiver
{
public:
	CReceiver();
	~CReceiver();

	BOOL Open(LPCTSTR pszURL);
	BOOL Connect();
	void Disconnect();
	void Close();
	void Stop() {m_fStop = TRUE;}
	void SetMessageWindow(HWND hwndMessage) {m_hwndMessage = hwndMessage;}
	BOOL IsConnected() {return m_hSock != INVALID_SOCKET;}
	BOOL IsEos() {return m_fEos;}
	BOOL IsShoutcast() {return m_fShoutcast;}

	void SetProxy(BOOL fUseProxy, LPCTSTR pszProxy);
	BOOL GetProxy(LPTSTR pszProxy);
	void SetUserAgent(LPCTSTR pszAgent);
	void GetUserAgent(LPTSTR pszAgent);

	BOOL SetBufferCount(int nCount);
	int GetBufferCount();
	BOOL SetPrebufferingCount(int nCount);
	int GetPrebufferingCount();
	int GetBufferingCount();
	int GetStatus();
	void GetBufferInfo(DWORD* pcbTotal, DWORD* pcbBuffered);

	BOOL Read(LPBYTE pbBuffer, DWORD cbBuffer, LPDWORD pcbRead);
	BOOL GetStreamInfo(LPTSTR pszName, LPTSTR pszGenre, LPTSTR pszURL);
	BOOL GetStreamTitle(LPTSTR pszTitle);
	void SetStreamName(LPTSTR pszName);

protected:
	DWORD ReceiverThread();
	static DWORD WINAPI ReceiverThreadProc(LPVOID pParam);

	BOOL ConnectServer(LPCTSTR pszURL, LPTSTR pszRedirect);
	void DisconnectServer();
	BOOL PrepareBuffer();
	void UnprepareBuffer();
	int CheckMetaData(LPBYTE pbBuf, int cbBuf);
	void NotifyMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);

protected:
	HWND		m_hwndMessage;
	BOOL		m_fInitialized;
	CCritSec	m_csecBuf;
	CCritSec	m_csecConnect;
	HANDLE	m_hThread;
	DWORD	m_dwThread;
	int		m_hSock;
	int		m_cbMetaData;
	int		m_cbMetaDataInterval;

	TCHAR	m_szURL[MAX_URL];
	TCHAR	m_szProxy[MAX_URL];
	TCHAR	m_szUserAgent[MAX_URL];
	BOOL	m_fUseProxy;

	TCHAR	m_szIcyName[MAX_URL];
	TCHAR	m_szIcyGenre[MAX_URL];
	TCHAR	m_szIcyURL[MAX_URL];
	BOOL	m_fShoutcast;

	BYTE	m_bMetaDataBuf[257];
	int		m_cbMetaDataBuf;

	BOOL	m_fStop;
	BOOL	m_fEos;

	int		m_cBuf;
	int		m_cPreBuf;
	CEvent		m_eventStart;
	CMultiBuff	m_listBuf;
	CEvent		m_eventBuf;
	CMultiBuff	m_listBufFree;
	CEvent		m_eventBufFree;
};

#endif // !defined(__RECEIVER_H_INCLUDED)
