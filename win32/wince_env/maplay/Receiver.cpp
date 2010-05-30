#include <windows.h>
#include "maplay.h"
#include "receiver.h"
#include <stdio.h>

CReceiver::CReceiver() : 
m_eventStart(FALSE), m_eventBuf(TRUE), m_eventBufFree(TRUE)
{
	m_hwndMessage = NULL;
	m_fInitialized = FALSE;
	m_hThread = NULL;
	m_dwThread = 0;

	m_szURL[0] = NULL;
	m_szProxy[0] = NULL;
	m_szURL[0] = NULL;
	m_szProxy[0] = NULL;
	m_szIcyName[0] = NULL;
	m_szIcyGenre[0] = NULL;
	m_szIcyURL[0] = NULL;
	m_fUseProxy = FALSE;

	_tcscpy(m_szUserAgent, DEF_USER_AGENT);
	m_hSock = INVALID_SOCKET;
	m_cbMetaData = 0;
	m_cbMetaDataInterval = 0;
	m_cbMetaDataBuf = 0;

	m_fShoutcast = FALSE;
	m_fStop = FALSE;
	m_fEos = FALSE;
	m_cBuf = 64;
	m_cPreBuf = 32;
	
	// Winsockの初期化
	WSADATA WsaData;
	if (!WSAStartup(MAKEWORD(1, 1), &WsaData))
		m_fInitialized = TRUE;
}

CReceiver::~CReceiver()
{
	Close();
	if (m_fInitialized) {
		WSACleanup();
	}
}

BOOL CReceiver::Open(LPCTSTR pszURL)
{
	if (_tcsncmp(pszURL, _T("http://"), 7) != 0)
		return FALSE;

	if (!m_fInitialized)
		return FALSE;

	_tcscpy(m_szURL, pszURL);
	return TRUE;
}

BOOL CReceiver::Connect()
{
	if (!m_fInitialized)
		return FALSE;

	CAutoLock lock(&m_csecConnect);
	Disconnect();

	// バッファを作成
	if (!PrepareBuffer())
		return FALSE;

	m_fShoutcast = FALSE;
	m_fStop = FALSE;
	m_fEos = FALSE;

	// 受信用スレッドを作成
	m_eventStart.Reset();
	m_hThread = CreateThread(NULL, 0, ReceiverThreadProc, this, 0, &m_dwThread);
	if (!m_hThread)
		return FALSE;

	// 開始処理が完了するまで待機
	HANDLE handles[] = {m_eventStart, m_hThread};
	int nRet = WaitForMultipleObjects(sizeof(handles)/ sizeof(HANDLE), handles, FALSE, INFINITE);
	if (nRet != WAIT_OBJECT_0) {
		UnprepareBuffer();
		return FALSE;
	}
	return TRUE;
}

void CReceiver::Disconnect()
{
	CAutoLock lock(&m_csecConnect);
	// スレッドが終了するまで待つ
	if (m_hThread) {
		m_fStop = TRUE;
		m_eventBufFree.Set();
		m_eventBuf.Set();
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	// バッファを破棄
	UnprepareBuffer();
	m_fShoutcast = FALSE;
	m_fStop = FALSE;
	m_fEos = FALSE;

	m_cbMetaDataBuf = 0;
}

void CReceiver::Close()
{
	Disconnect();

	m_szURL[0] = NULL;
	m_szIcyName[0] = NULL;
	m_szIcyGenre[0] = NULL;
	m_szIcyURL[0] = NULL;
	m_cbMetaDataBuf = 0;
}

BOOL CReceiver::Read(LPBYTE pbBuffer, DWORD cbBuffer, LPDWORD pcbRead)
{
	*pcbRead = 0;
	while (*pcbRead < cbBuffer) {
		// バッファを待つ
		while (!m_eventBuf.Lock(100)) {
			// Socketが閉じられていて、バッファがない場合はもう受信できるデータはない
			if (m_hSock == INVALID_SOCKET) 
				return FALSE;
		}

		if (m_listBuf.GetCount() == 0)
			return FALSE;

		// コピー
		CAutoLock lock(&m_csecBuf);
		RECV_BUF* pBuf = (RECV_BUF*)m_listBuf.GetAt(0);
		DWORD cb = min(pBuf->cbRecv - pBuf->cbUsed, cbBuffer - *pcbRead);
		memcpy(pbBuffer + *pcbRead, pBuf->pbBuf + pBuf->cbUsed, cb);
		*pcbRead += cb;
		pBuf->cbUsed += cb;
		if (pBuf->cbUsed == pBuf->cbRecv) {
			m_listBuf.RemoveAt(0);
			m_listBufFree.Add((DWORD)pBuf);
			m_eventBufFree.Set();
		}

		if (m_listBuf.GetCount() == 0)
			m_eventBuf.Reset();
	}

	return TRUE;
}

void CReceiver::SetProxy(BOOL fUseProxy, LPCTSTR pszProxy)
{
	m_fUseProxy = fUseProxy;
	_tcscpy(m_szProxy, pszProxy);
}

BOOL CReceiver::GetProxy(LPTSTR pszProxy)
{
	_tcscpy(pszProxy, m_szProxy);
	return m_fUseProxy;
}

void CReceiver::SetUserAgent(LPCTSTR pszAgent)
{
	_tcscpy(m_szUserAgent, pszAgent);
}

void CReceiver::GetUserAgent(LPTSTR pszAgent)
{
	_tcscpy(pszAgent, m_szUserAgent);
}

BOOL CReceiver::SetBufferCount(int nCount)
{
	//　バッファが既に作られている場合は不可
	if (m_listBuf.GetCount() || m_listBufFree.GetCount())
		return FALSE;

	// 1以下はダメ
	if (nCount < 2)
		return FALSE;

	m_cBuf = nCount;
	if (m_cPreBuf > m_cBuf)
		m_cPreBuf = m_cBuf;
	return TRUE;
}

int CReceiver::GetBufferCount()
{
	return m_cBuf;
}

BOOL CReceiver::SetPrebufferingCount(int nCount)
{
	if (nCount > m_cBuf)
		return FALSE;

	// 1以下はダメ
	if (nCount < 2)
		return FALSE;

	m_cPreBuf = nCount;
	return TRUE;
}

int CReceiver::GetPrebufferingCount()
{
	return m_cPreBuf;
}

int CReceiver::GetBufferingCount()
{
	return m_listBuf.GetCount();
}

void CReceiver::GetBufferInfo(DWORD* pcbTotal, DWORD* pcbBuffered)
{
	if (m_hSock == INVALID_SOCKET) {
		*pcbTotal = *pcbBuffered = 0;
		return;
	}

	*pcbTotal = m_cBuf * RECV_BUFF_SIZE;
	*pcbBuffered = m_listBuf.GetCount() * RECV_BUFF_SIZE;
}

BOOL CReceiver::GetStreamInfo(LPTSTR pszName, LPTSTR pszGenre, LPTSTR pszURL)
{
	if (!_tcslen(m_szIcyName) && !_tcslen(m_szIcyGenre) && !_tcslen(m_szIcyURL))
		return FALSE;

	BOOL fRet = FALSE;
	if (pszName) {
		*pszName = NULL;
		if (_tcslen(m_szIcyName)) {
			_tcscpy(pszName, m_szIcyName);
			fRet = TRUE;
		}
	}
	if (pszGenre) {
		*pszGenre = NULL;
		if (_tcslen(m_szIcyGenre)) {
			_tcscpy(pszGenre, m_szIcyGenre);
			fRet = TRUE;
		}
	}
	if (pszURL) {
		*pszURL = NULL;
		if (_tcslen(m_szIcyURL)) {
			_tcscpy(pszURL, m_szIcyURL);
			fRet = TRUE;
		}
	}
	return fRet;
}

BOOL CReceiver::GetStreamTitle(LPTSTR pszTitle)
{
	*pszTitle = NULL;
	
	char szTitle[MAX_URL] = {0};
	if (m_cbMetaDataBuf && m_cbMetaDataBuf == m_bMetaDataBuf[0] * 16 + 1) {
		char* psz = strstr((char*)m_bMetaDataBuf, "StreamTitle=");
		if (psz) {
			psz += strlen("StreamTitle='");
			char* psz2 = strchr(psz, ';');
			if (psz2) {
				if (*(psz2 - 1) == '\'')
					psz2--;

				strncpy(szTitle, psz, psz2 - psz);
#ifdef _UNICODE
				MultiByteToWideChar(CP_ACP, 0, szTitle, -1, pszTitle, MAX_URL);
#else
				strcpy(pszTitle, szTitle);
#endif
				return TRUE;
			}
		}
	}
	return FALSE;
}

// protected
DWORD WINAPI CReceiver::ReceiverThreadProc(LPVOID pParam)
{
	return ((CReceiver*)pParam)->ReceiverThread();
}

DWORD CReceiver::ReceiverThread()
{
	{
		TCHAR szURL[MAX_URL];
		TCHAR szRedirect[MAX_URL];
		_tcscpy(szURL, m_szURL);
		while (!ConnectServer(szURL, szRedirect)) {
			if (!_tcslen(szRedirect))
				return 0;
			_tcscpy(szURL, szRedirect);
		}
	}

	fd_set fdRecv;
	int	nRecv, nTimeout;
	RECV_BUF* pBuf = NULL;
	RECV_BUF Buf;
	struct timeval tv = {0, RECV_INTERVAL};
	pBuf = (RECV_BUF*)m_listBufFree.RemoveAt(0);
	if (m_listBufFree.GetCount() == 0)
		m_eventBufFree.Reset();

	m_eventStart.Set();
	while (TRUE) {
		// バッファを取得する
		if (!pBuf) {
			m_eventBufFree.Lock();
			if (m_fStop)
				goto done;

			CAutoLock lock(&m_csecBuf);
			pBuf = (RECV_BUF*)m_listBufFree.RemoveAt(0);
			pBuf->cbRecv = 0;
			pBuf->cbUsed = 0;

			if (m_listBufFree.GetCount() == 0)
				m_eventBufFree.Reset();
		}
		while (pBuf->cbRecv < RECV_BUFF_SIZE) {
			nTimeout = 0;
			while (TRUE) {
				if (m_fStop)
					goto done;
				// タイムアウトするまで待つ
				FD_ZERO(&fdRecv);
				FD_SET(m_hSock, &fdRecv);
				if (select(NULL, &fdRecv, 0, 0, &tv) != SOCKET_ERROR && FD_ISSET(m_hSock, &fdRecv))
					break;

				nTimeout += RECV_INTERVAL;
				if (nTimeout > RECV_RECEIVE_TIMEOUT)
					goto done;
			}

			// 受信する
			nRecv = recv(m_hSock, (char*)pBuf->pbBuf + pBuf->cbRecv, 
										RECV_BUFF_SIZE - pBuf->cbRecv, 0);
			if (nRecv == 0) {
				m_fEos = TRUE;
				break;
			}
			if (nRecv == SOCKET_ERROR)
				goto done;

			// MetaDataのチェックを行う
			nRecv -= CheckMetaData(pBuf->pbBuf + pBuf->cbRecv, nRecv);
			pBuf->cbRecv += nRecv;
		}
		if (m_fStop)
			goto done;

		// 出力処理
		if (pBuf && pBuf->cbRecv) {
			CAutoLock lock(&m_csecBuf);
			m_listBuf.Add((DWORD)pBuf);
			pBuf = NULL;
			m_eventBuf.Set();
		}
		if (m_fEos)
			goto done;
	}
done:
	DisconnectServer();
	return 0;
}

BOOL CReceiver::ConnectServer(LPCTSTR pszURL, LPTSTR pszRedirect)
{
	int nPort, nLen = 0, nRet = 0, nTimeout = 0;
	unsigned long addr, val = 1;
	struct sockaddr_in sAddr;
	struct hostent* pHost;
	char* psz;
	char szURL[MAX_URL];
	char szProxy[MAX_URL];
	char szHostName[MAX_URL] = {0};
	char szUserAgent[MAX_URL];
	char szRequest[MAX_URL * 4];
	char szRecv[RECV_BUFF_SIZE];
	fd_set	fdRecv, fdError;
	struct timeval tv = {0, RECV_INTERVAL};
	RECV_BUF* pBuf;

	*pszRedirect = NULL;
#ifdef _UNICODE
	WideCharToMultiByte(CP_ACP, 0, pszURL, -1, szURL, MAX_URL, 0, 0);
	WideCharToMultiByte(CP_ACP, 0, m_szProxy, -1, szProxy, MAX_URL, 0, 0);
	WideCharToMultiByte(CP_ACP, 0, m_szUserAgent, -1, szUserAgent, MAX_URL, 0, 0);
#else
	strcpy(szURL, pszURL);
	strcpy(szProxy, m_szProxy);
	strcpy(szUserAgent, m_szUserAgent);
#endif
	if (!strlen(szURL))
		return FALSE;

	// URLを解析する
	// Proxyサーバーが設定されているときはサーバーアドレスを解析
	if (m_fUseProxy && strlen(szProxy)) {
		if (strncmp(szProxy, "http://", strlen("http://"))) {
			if (sscanf(szProxy, "%[^:/]:%d", szHostName, &nPort) < 2)
				nPort = RECV_HTTP;
		}
		else {
			if (sscanf(szProxy, "http://%[^:/]:%d", szHostName, &nPort) < 2)
				nPort = RECV_HTTP;
		}
		psz = szURL;	
	}
	else {
		if (strncmp(szURL, "http://", strlen("http://"))) {
			if (sscanf(szURL, "%[^:/]:%d", szHostName, &nPort) < 2)
				nPort = RECV_HTTP;
			psz = strchr(szURL, '/');
		}
		else {
			if (sscanf(szURL, "http://%[^:/]:%d", szHostName, &nPort) < 2)
				nPort = RECV_HTTP;
			psz = strchr(szURL + 7, '/');
		}
	}
	if (!strlen(szHostName))
		return FALSE;

	// socketの作成
	m_hSock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_hSock == INVALID_SOCKET)
		return FALSE;

	// アドレスの取得
	addr = inet_addr(szHostName);
	if (addr == SOCKET_ERROR) {
		pHost = gethostbyname(szHostName);
		if (!pHost)
			return FALSE;
		addr = *((unsigned long*)(pHost->h_addr_list)[0]);
	}

	// 非同期受信に切り替え
	if (ioctlsocket(m_hSock, FIONBIO, &val) == SOCKET_ERROR)
		goto fail;

	// 接続
	memset(&sAddr, 0, sizeof(struct sockaddr_in));
	sAddr.sin_family = AF_INET;
	sAddr.sin_port = htons((u_short)nPort);
	sAddr.sin_addr.s_addr = addr;
	connect(m_hSock, (const sockaddr*)&sAddr, sizeof(sAddr));

	// タイムアウトするまで待つ
	nTimeout = 0;
	while (TRUE) {
		if (m_fStop)
			goto fail;
		FD_ZERO(&fdRecv);
		FD_SET(m_hSock, &fdRecv);
		FD_ZERO(&fdError);
		FD_SET(m_hSock, &fdError);
		if (select(NULL, 0, &fdRecv, &fdError, &tv) != SOCKET_ERROR) {
			if (FD_ISSET(m_hSock, &fdError))
				goto fail;
			else if (FD_ISSET(m_hSock, &fdRecv))
				break;
		}

		nTimeout += RECV_INTERVAL;
		if (nTimeout > RECV_CONNECT_TIMEOUT)
			goto fail;
	}

	// Request(HTTP)の送信
	sprintf(szRequest, "GET %s HTTP/1.0\r\nHost: %s\r\nAccept: */*\r\nIcy-MetaData:1\r\nUser-Agent: %s\r\n\r\n",
	psz ? psz : "/", szHostName, szUserAgent);
	//x-audiocast-udpport: %d\r\n まだ未対応

	if (send(m_hSock, szRequest, strlen(szRequest), 0) == SOCKET_ERROR)
		goto fail;

	// ICY (HTTP)ヘッダをすべて受信する
	psz = NULL;
	while (!psz && RECV_BUFF_SIZE - nLen - 1 > 0) {
		// タイムアウトするまで待つ
		nTimeout = 0;
		while (TRUE) {
			if (m_fStop)
				goto fail;
			FD_ZERO(&fdRecv);
			FD_SET(m_hSock, &fdRecv);
			if (select(NULL, &fdRecv, 0, 0, &tv) != SOCKET_ERROR && FD_ISSET(m_hSock, &fdRecv)) 
				break;

			nTimeout += RECV_INTERVAL;
			if (nTimeout > RECV_RECEIVE_TIMEOUT)
				goto fail;
		}

		int nRecv = recv(m_hSock, szRecv + nLen, RECV_BUFF_SIZE - nLen, 0);
		if (!nRecv || nRecv == SOCKET_ERROR)
			goto fail;

		nLen += nRecv;
		if (nRet == 0) {
			// エラーコードをチェック
			if (!sscanf(szRecv, " %*s %d", &nRet))
				goto fail;

			if (nRet != ICY_OK && nRet != ICY_REDIRECT)
				goto fail;
		}

		szRecv[nLen] = NULL;
		psz = strstr(szRecv, "\r\n\r\n");
	}

	if (!psz)	// ヘッダが終わっていない
		goto fail;

	// ヘッダを解析する
	if (nRet == ICY_REDIRECT) {
		// リダイレクト
		DisconnectServer();
		psz = strstr(szRecv, "Location: ");
		if (sscanf(psz, "Location: %256[^\r\n]", szURL) == 1) {
#ifdef _UNICODE
			MultiByteToWideChar(CP_ACP, 0, szURL, -1, pszRedirect, MAX_URL);
#else
			strncpy(pszRedirect, szURL, MAX_URL);
			pszRedirect[MAX_URL - 1] = NULL;
#endif
			return FALSE;
		}
	}

	// name
	psz = strstr(szRecv, "icy-name");
	if (psz) {
		m_fShoutcast = TRUE;
		psz += strlen("icy-name:");
		if (sscanf(psz, "%256[^\r\n]", szURL)) {
			LPSTR p = szURL;
			while (*p == ' ') p++;
#ifdef _UNICODE
			MultiByteToWideChar(CP_ACP, 0, p, -1, m_szIcyName, MAX_URL);
#else
			strcpy(m_szIcyName, p);
#endif
		}
	}
	psz = strstr(szRecv, "ice-name");
	if (psz) {
		m_fShoutcast = TRUE;
		psz += strlen("ice-name:");
		if (sscanf(psz, "%256[^\r\n]", szURL)) {
			LPSTR p = szURL;
			while (*p == ' ') p++;
#ifdef _UNICODE
			MultiByteToWideChar(CP_ACP, 0, p, -1, m_szIcyName, MAX_URL);
#else
			strcpy(m_szIcyName, p);
#endif
		}
	}

	// genre
	psz = strstr(szRecv, "icy-genre");
	if (psz) {
		m_fShoutcast = TRUE;
		psz += strlen("icy-genre:");
		if (sscanf(psz, "%256[^\r\n]", szURL)) {
			LPSTR p = szURL;
			while (*p == ' ') p++;
#ifdef _UNICODE
			MultiByteToWideChar(CP_ACP, 0, p, -1, m_szIcyGenre, MAX_URL);
#else
			strcpy(m_szIcyGenre, p);
#endif
		}
	}
	psz = strstr(szRecv, "ice-genre");
	if (psz) {
		m_fShoutcast = TRUE;
		psz += strlen("ice-genre:");
		if (sscanf(psz, "%256[^\r\n]", szURL)) {
			LPSTR p = szURL;
			while (*p == ' ') p++;
#ifdef _UNICODE
			MultiByteToWideChar(CP_ACP, 0, p, -1, m_szIcyGenre, MAX_URL);
#else
			strcpy(m_szIcyGenre, p);
#endif
		}
	}

	// url
	psz = strstr(szRecv, "icy-url");
	if (psz) {
		m_fShoutcast = TRUE;
		psz += strlen("icy-url:");
		if (sscanf(psz, "%256[^\r\n]", szURL)) {
			LPSTR p = szURL;
			while (*p == ' ') p++;
#ifdef _UNICODE
			MultiByteToWideChar(CP_ACP, 0, p, -1, m_szIcyURL, MAX_URL);
#else
			strcpy(m_szIcyURL, p);
#endif
		}
	}
	psz = strstr(szRecv, "ice-url");
	if (psz) {
		m_fShoutcast = TRUE;
		psz += strlen("ice-url:");
		if (sscanf(psz, "%256[^\r\n]", szURL)) {
			LPSTR p = szURL;
			while (*p == ' ') p++;
#ifdef _UNICODE
			MultiByteToWideChar(CP_ACP, 0, p, -1, m_szIcyURL, MAX_URL);
#else
			strcpy(m_szIcyURL, p);
#endif
		}
	}
	
	//psz = strstr(szRecv, "icy-pub");
	//psz = strstr(szRecv, "x-audiocast-udpport:");

	// MetaData
	psz = strstr(szRecv, "icy-metaint");
	if (psz) {
		psz += strlen("icy-metaint:");
		m_cbMetaData = atoi(psz);
	}
	else {
		m_cbMetaData = 0;
	}
	m_cbMetaDataInterval = m_cbMetaData;

	// ヘッダ以外の受信済みバッファをコピーする
	pBuf = (RECV_BUF*)m_listBufFree.GetAt(0);
	psz = strstr(szRecv, "\r\n\r\n") + 4;
	pBuf->cbRecv = nLen - (psz - szRecv);
	memcpy(pBuf->pbBuf, psz, pBuf->cbRecv);

	// MetaDataのチェック
	pBuf->cbRecv -= CheckMetaData(pBuf->pbBuf, pBuf->cbRecv);

	return TRUE;
fail:
	DisconnectServer();
	return FALSE;
}

void CReceiver::DisconnectServer()
{
	if (m_hSock != INVALID_SOCKET) {
		shutdown(m_hSock, -2);
		closesocket(m_hSock);
		m_hSock = INVALID_SOCKET;
	}
}

BOOL CReceiver::PrepareBuffer()
{
	CAutoLock lock(&m_csecBuf);
	for (int i = 0; i < (int)m_cBuf; i++) {
		RECV_BUF* p = new RECV_BUF;
		if (!p) goto fail;
		m_listBufFree.Add((DWORD)p);
	}
	m_eventBufFree.Set();
	m_eventBuf.Reset();
	return TRUE;

fail:
	UnprepareBuffer();
	return FALSE;
}

void CReceiver::UnprepareBuffer()
{
	CAutoLock lock(&m_csecBuf);
	int i;
	for (i = 0; i < m_listBufFree.GetCount(); i++) {
		RECV_BUF* p = (RECV_BUF*)m_listBufFree.GetAt(i);
		delete p;
	}
	m_listBufFree.RemoveAll();
	for (i = 0; i < m_listBuf.GetCount(); i++) {
		RECV_BUF* p = (RECV_BUF*)m_listBuf.GetAt(i);
		delete p;
	}
	m_listBuf.RemoveAll();

	m_eventBufFree.Set();
	m_eventBuf.Set();
}

int CReceiver::CheckMetaData(LPBYTE pbBuf, int cbBuf)
{
	if (!m_cbMetaData)
		return 0;

	static TCHAR sz[MAX_URL];
	if (m_cbMetaDataInterval < 0) {
		// 途中まではMetaData
		if (cbBuf + m_cbMetaDataInterval <= 0) {
			memcpy(m_bMetaDataBuf + m_cbMetaDataBuf, pbBuf, cbBuf);
			m_cbMetaDataBuf += cbBuf;
			m_cbMetaDataInterval += cbBuf; // また途中までしかない...
			return cbBuf;
		}
		int cbMetaData = -m_cbMetaDataInterval;
		memcpy(m_bMetaDataBuf + m_cbMetaDataBuf, pbBuf, cbMetaData);
		m_cbMetaDataBuf += cbMetaData;
		memmove(pbBuf, pbBuf - m_cbMetaDataInterval, cbBuf - cbMetaData);
		m_cbMetaDataInterval = m_cbMetaData - (cbBuf - cbMetaData);

		if (m_cbMetaDataInterval < 0) { // 2005/01/16
			m_cbMetaDataInterval = m_cbMetaData;
			return CheckMetaData(pbBuf, cbBuf - cbMetaData) + cbMetaData;
		}

		// タイトルの通知を行う
		if (GetStreamTitle(sz)) {
			NotifyMessage(MAP_MSG_STREAM_TITLE, (WPARAM)sz, 0);
		}
		return cbMetaData;
	}

	if (m_cbMetaDataInterval - cbBuf < 0) {
		LPBYTE pMetaData = pbBuf + m_cbMetaDataInterval;
		int nSize = *pMetaData * 16 + 1;
		if (m_cbMetaDataInterval + nSize > cbBuf) {
			// 途中までしかない...
			if (*pMetaData) {
				m_cbMetaDataBuf = cbBuf - m_cbMetaDataInterval;
				memcpy(m_bMetaDataBuf, pMetaData, m_cbMetaDataBuf);
			}
			m_cbMetaDataInterval = cbBuf - m_cbMetaDataInterval - nSize; // マイナス値になる
			return nSize + m_cbMetaDataInterval;
		}
		
		// MetaDataの内容を取り出す
		if (*pMetaData) {
			m_cbMetaDataBuf = nSize;
			memcpy(m_bMetaDataBuf, pMetaData, m_cbMetaDataBuf);
		}

		// バッファを詰める
		cbBuf = cbBuf - m_cbMetaDataInterval - nSize;
		memmove(pMetaData, pMetaData + nSize, cbBuf);
		m_cbMetaDataInterval = m_cbMetaData - cbBuf;

		if (m_cbMetaDataInterval < 0) { // 2005/01/16
			m_cbMetaDataInterval = m_cbMetaData;
			return CheckMetaData(pMetaData, cbBuf) + nSize;
		}

		// タイトルの通知を行う
		if (nSize > 1 && GetStreamTitle(sz)) {
			NotifyMessage(MAP_MSG_STREAM_TITLE, (WPARAM)sz, 0);
		}
		return nSize;
	}
	
	m_cbMetaDataInterval -= cbBuf;
	return 0;
}

void CReceiver::NotifyMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_hwndMessage) {
		PostMessage(m_hwndMessage, uMsg, wParam, lParam);
	}
}

void CReceiver::SetStreamName(LPTSTR pszName)
{
	if (_tcslen(m_szIcyName) == 0) {
		_tcscpy(m_szIcyName, pszName);
	}
}