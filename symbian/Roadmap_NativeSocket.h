/* RoadmapNativeSocket.h - Symbian sound implementation for Roadmap
 *
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps Ltd.
 *   Copyright 2008 Ehud Shabtai
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SYNOPSYS:
 *
 *   See roadmap_net.h
 */

#ifndef _ROADMAP_NATIVE_SOCKET__H_
#define _ROADMAP_NATIVE_SOCKET__H_

extern "C" {
//#include "roadmap_io.h"
#include "roadmap_main.h"
}

#include "Roadmap_NativeNet.h"


//
//  This file contains all handling of native Net implementation
//  We're basically using RSockets and wrapping them "synchronously". 
//

#define NATIVE_SOCK_BUFSIZE 256

typedef enum 
{
  ESockStatusNotStarted = -1,
  ESockStatusReady,
  ESockStatusSending,
  ESockStatusWaiting,
  ESockStatusConnecting,
  ESockStatusConnected,
  ESockStatusDNSLookup,
  ESockStatusDNSDone,
  ESockStatusPolling,
  ESockStatusTimedOut,
} ENativeSockStatus;

typedef struct roadmap_main_io {
   RoadMapIO *io;
   RoadMapInput callback;
   int is_valid;
} roadmap_main_io;

class MTimeoutObserver
{
public: 
  virtual void OnConnectTimeout() = 0;
};

class MSocketNotifier
{
  //  Callback for notifier that data arrived on socket
  //  The notifier should not free the data. 
  virtual void DataReady(const void* apData) = 0;
};

//  Base data class for Read/Write sockets
class TNativeSocket
{
public: 
  TNativeSocket(RSocket* apSocket, MSocketNotifier& aNotifier) 
    : m_pSocket(apSocket), m_Notifier(aNotifier)
  {
  
  }
  
  RSocket* m_pSocket;
  ENativeSockType m_eSockType;
  TInetAddr m_Address;
  MSocketNotifier& m_Notifier;
};


/*
//
//  Notifier implementation
//

class CNetObserver : public MSocketNotifier 
{
public: 
  CNetObserver()  {}
  virtual ~CNetObserver()  {}
  virtual void DataReady(const void* apData)  {}
};
*/

class CTimeoutTimer : public CTimer 
{
public: 
  virtual ~CTimeoutTimer();
  static CTimeoutTimer* NewL(MTimeoutObserver& aTimeoutObserver);
  void ConstructL(MTimeoutObserver &aTimeoutObserver);
protected:
  CTimeoutTimer();
  virtual void RunL();
  
  MTimeoutObserver* m_pTimeoutObserver;
};

//
//  Socket Reader
//

class CReadSocket : public CActive, public TNativeSocket
{
public: 
  ~CReadSocket();
  static CReadSocket* NewL(RSocket* apSocket, MSocketNotifier& aNotifier);
  int IssueReceiveRequest(void* apData, int aDataLen);
  void IssuePollRequest(void* apInputCallback, void* apIO);
  
private: 
  CReadSocket(RSocket* apSocket, MSocketNotifier& aNotifier);
  static CReadSocket* NewLC(RSocket* apSocket, MSocketNotifier& aNotifier);
  void ConstructL();//RSocket* apSocket, MSocketNotifier& aNotifier);
  void InvokePollingCallback(void* io);

  virtual void DoCancel();
  virtual void RunL();
  
  TPtr8 m_ReadBuffer;
  TSockXfrLength m_Len;
  
  //  For "select" on socket
  void* m_pInputCallback;
  TPckgBuf<TUint32> m_SelectFlags;
  ENativeSockStatus m_eStatus;
  void* m_pIO;
}; 


//
//  Socket Writer
//

class CWriteSocket : public CActive, public TNativeSocket
{
public: 
  ~CWriteSocket();
  static CWriteSocket* NewL(RSocket* apSocket, MSocketNotifier& aNotifier);
  int IssueWriteRequest(const void* apData, int aDataLen);
  
private: 
  CWriteSocket(RSocket* apSocket, MSocketNotifier& aNotifier);
  static CWriteSocket* NewLC(RSocket* apSocket, MSocketNotifier& aNotifier);
  void ConstructL();//RSocket* apSocket, MSocketNotifier& aNotifier);

  virtual void DoCancel();
  virtual void RunL();
  
  TPtrC8 m_WriteBuffer;
  ENativeSockStatus m_eStatus;
}; 



class CRoadMapNativeSocket :  public CActive, 
                              public CRoadMapNativeNet, 
                              public MTimeoutObserver, 
                              public MSocketNotifier
{
public:
  virtual ~CRoadMapNativeSocket();

  static CRoadMapNativeSocket* NewL(const char *apHostname, int aPort, RoadMapNetConnectCallback apCallback, void* apContext);//, MSocketNotifier& aNotifier);
  static CRoadMapNativeSocket* NewLC(const char *apHostname, int aPort, RoadMapNetConnectCallback apCallback, void* apContext);//, MSocketNotifier& aNotifier);

  int GetPortFromService (const char * apService /*, add protocol here for a generic function*/);
  
  virtual void OpenL(ENativeSockType aSockType);
  virtual void ConnectL(const TDesC& aHostname, int aPort);  
  
  virtual int Read(void *data, int length);
  virtual int Write(const void *data, int length);
  virtual void Close();
  
  virtual void StartPolling(void* apInputCallback, void* apIO);
  virtual void StopPolling();  
  
  virtual void DataReady(const void* apData);
  virtual void OnConnectTimeout(); //  MTimerObserver

  void ConnectL(TInetAddr& addr);

protected:
  CRoadMapNativeSocket(const char *apHostname, int aPort);
  void ConstructL(RoadMapNetConnectCallback apCallback, void* apContext);//, MSocketNotifier& aNotifier);
  
  void InvokeIOCallback(int aConnectionStatus);
  void StartConnectionL();
  
  void ConnectWithParamsL();
  TInt ConnAsyncStart(TConnPref &aConnPrefs);
  TInt ConnAsyncStart();
  void OpenSession(); //  not implemented for Socket conn
  
  RHostResolver m_HostResolver;
  RSocket m_Socket; //  used for all ops
  
  //  We need 2 socket CActive
  CReadSocket* m_pReadSocket;
  CWriteSocket* m_pWriteSocket;
  ENativeSockStatus m_eNetStatus;
  TNameEntry m_NameEntry;
  TInetAddr m_Addr;
  
  void RunL();
  void DoCancel();
  
  void* m_pIO;
  CTimeoutTimer* m_pTimeoutTimer;
};

#endif  //  #ifndef _ROADMAP_NATIVE_SOCKET__H_
