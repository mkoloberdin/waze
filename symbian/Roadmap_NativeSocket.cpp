/* RoadmapNativeSocket.cpp - Symbian socket implementation for Roadmap
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

extern "C"{
#include "roadmap_main.h"
#include "roadmap_net_mon.h"
#include "roadmap_io.h"
}

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "Roadmap_NativeSocket.h"
#include "GSConvert.h"
#include <in_sock.h>


const int conn_timeout = 60000000;  //  1 minute

///////////////////////////////////////////////////////////////////////
//
//  Socket Reader
//

CTimeoutTimer::CTimeoutTimer() : CTimer(EPriorityHigh)
{
  m_pTimeoutObserver = NULL;
}

CTimeoutTimer::~CTimeoutTimer()
{
  Cancel();
}

CTimeoutTimer* CTimeoutTimer::NewL(MTimeoutObserver& aTimeoutObserver)
{
  CTimeoutTimer* self = new (ELeave) CTimeoutTimer();
  CleanupStack::PushL(self);
  self->ConstructL(aTimeoutObserver);
  CleanupStack::Pop(self);
  return self;
}

void CTimeoutTimer::ConstructL(MTimeoutObserver &aTimeoutObserver)
{
  m_pTimeoutObserver = &aTimeoutObserver;
  CTimer::ConstructL();
  CActiveScheduler::Add(this);
}

void CTimeoutTimer::RunL()
{
  m_pTimeoutObserver->OnConnectTimeout();
}

///////////////////////////////////////////////////////////////////////
//
//  Socket Reader
//

CReadSocket::CReadSocket(RSocket* apSocket, MSocketNotifier& aNotifier) 
  : CActive(EPriorityStandard), TNativeSocket(apSocket, aNotifier), m_ReadBuffer(NULL, 0)
{
  m_pInputCallback = NULL;
  m_pIO = NULL;
}

CReadSocket::~CReadSocket() 
{
}

CReadSocket* CReadSocket::NewL(RSocket* apSocket, MSocketNotifier& aNotifier)
{
  CReadSocket* self = NewLC(apSocket, aNotifier);
  CleanupStack::Pop();
  return self;
}

CReadSocket* CReadSocket::NewLC(RSocket* apSocket, MSocketNotifier& aNotifier)
{
  CReadSocket* self = new(ELeave) CReadSocket(apSocket, aNotifier);
  CleanupStack::PushL(self);
  self->ConstructL();
  return self;
}

void CReadSocket::ConstructL()
{
  CActiveScheduler::Add(this);
}

void CReadSocket::DoCancel()
{
  m_pSocket->CancelRead();
  m_pSocket->CancelIoctl();
}

void CReadSocket::IssuePollRequest(void* apInputCallback, void* mainIO)
{
  m_pInputCallback = apInputCallback;
  if ( m_pInputCallback == NULL )
  {
    m_pSocket->CancelIoctl();
    m_eStatus = ESockStatusReady;
    m_pIO = NULL;
    if (IsActive())
  	{
      Cancel();
  	}
  }
  else
  {
    m_pIO = mainIO;
    m_eStatus = ESockStatusPolling;
    m_SelectFlags = KSockSelectRead;
    m_pSocket->Ioctl(KIOctlSelect, iStatus, &m_SelectFlags, KSOLSocket);
    SetActive();
  }
}

void CReadSocket::RunL()
{
  roadmap_main_io *data = (roadmap_main_io *) m_pIO;
  RoadMapIO *io = data->io;

  if ( iStatus == KErrNone )
  {
    switch ( m_eStatus )
    {
    case ESockStatusPolling:
      InvokePollingCallback(io);

      if (io->subsystem == ROADMAP_IO_NET) {
        m_pSocket->Ioctl(KIOctlSelect, iStatus, &m_SelectFlags, KSOLSocket);
        SetActive();
      } else {
        if (data->is_valid) {
          data->is_valid = 0;
        } else {
          free (data);
        }
      }

      break;
    default:
      break;
    }
  }
  else 
  {
		/* Check if this input was unregistered while we were
		 * sleeping.
		 */
		if (io->subsystem == ROADMAP_IO_INVALID) {
         if (data->is_valid) {
            data->is_valid = 0;
         } else {
            free (data);
         }
		}
    //TODO roadmap_log(ROADMAP_ERROR, "Socket read error!");
  }
}

void CReadSocket::InvokePollingCallback(void *io)
{
  if ( m_pInputCallback != NULL )
  {
    //  Inform our client that we have data waiting to be read
    (*(RoadMapInput)m_pInputCallback)((RoadMapIO*)io);
  }
}

int CReadSocket::IssueReceiveRequest(void* apData, int aDataLen)
{
  if (!IsActive())
  {
    TRequestStatus aStatus;
    m_Len = -1;
    if (apData != NULL) {
    	m_ReadBuffer.Set((unsigned char *)apData, aDataLen, aDataLen);
    }
    if ( m_eSockType == ESockTypeTcp )
    {
      m_pSocket->RecvOneOrMore(m_ReadBuffer, 0, aStatus, m_Len);
    }
    else
    {
      m_pSocket->RecvFrom(m_ReadBuffer, m_Address, NULL, aStatus);
    }
    User::WaitForRequest(aStatus);
    
    if (aStatus == KErrNone) {
       roadmap_net_mon_recv(m_Len());
       return m_Len();
    } else {
       roadmap_net_mon_error("Error in recv.");
       return aStatus.Int();
    }
  }
  
  roadmap_net_mon_error("Error in recv.");
  return -1;
}

///////////////////////////////////////////////////////////////////////
//
//  Socket Writer
//

CWriteSocket::CWriteSocket(RSocket* apSocket, MSocketNotifier& aNotifier) 
  : CActive(EPriorityStandard), TNativeSocket(apSocket, aNotifier)
{
}

CWriteSocket* CWriteSocket::NewL(RSocket* apSocket, MSocketNotifier& aNotifier)
{
  CWriteSocket* self = NewLC(apSocket, aNotifier);
  CleanupStack::Pop();
  return self;
}

CWriteSocket* CWriteSocket::NewLC(RSocket* apSocket, MSocketNotifier& aNotifier)
{
  CWriteSocket* self = new(ELeave) CWriteSocket(apSocket, aNotifier);
  CleanupStack::PushL(self);
  self->ConstructL();
  return self;
}

void CWriteSocket::ConstructL()
{
  CActiveScheduler::Add(this);
}

CWriteSocket::~CWriteSocket()
{
}

void CWriteSocket::DoCancel()
{   
  m_pSocket->CancelWrite();
}

void CWriteSocket::RunL()
{
  if ( iStatus == KErrNone )
  {
    if ( m_eStatus == ESockStatusSending )
    {
      m_eStatus = ESockStatusWaiting;
    }
  }
  else 
  {
    //TODO roadmap_log(ROADMAP_ERROR, "Could not write to socket!");
  }
}

int CWriteSocket::IssueWriteRequest(const void* apData, int aDataLen)
{
  m_WriteBuffer.Set((const TUint8*)apData, aDataLen);
  TRequestStatus aStatus;
  if ( m_eSockType == ESockTypeTcp )
  {
    m_pSocket->Write(m_WriteBuffer, aStatus);
  }
  else
  {
    m_pSocket->SendTo(m_WriteBuffer, m_Address, NULL, aStatus);
  }
  User::WaitForRequest(aStatus);
  if (aStatus == KErrNone) {
     roadmap_net_mon_send(aDataLen);
     return aDataLen;
  } else {
     roadmap_net_mon_error("Error in send.");
     return aStatus.Int();
  }
  //SetActive();
  //m_eStatus = ESockStatusSending;
}

CRoadMapNativeSocket::CRoadMapNativeSocket(const char *apHostname, int aPort) 
  : CActive(EPriorityNormal), CRoadMapNativeNet(apHostname, aPort)
{
  m_isHttp = 0;
  m_pReadSocket = NULL;
  m_pWriteSocket = NULL;
  m_pTimeoutTimer = NULL;
}

CRoadMapNativeSocket::~CRoadMapNativeSocket()
{
  Close();
  delete m_pWriteSocket; 
  delete m_pReadSocket;
  delete m_pTimeoutTimer;
  roadmap_net_mon_disconnect();
}

CRoadMapNativeSocket* CRoadMapNativeSocket::NewL(const char *apHostname, int aPort, 
                                                  RoadMapNetConnectCallback apCallback, void* apContext)//, MSocketNotifier& aNotifier)
{
  CRoadMapNativeSocket* self = NewLC(apHostname, aPort, apCallback, apContext);//, aNotifier);
  CleanupStack::Pop(self);
  return self;
}

CRoadMapNativeSocket* CRoadMapNativeSocket::NewLC(const char *apHostname, int aPort, 
                                                  RoadMapNetConnectCallback apCallback, void* apContext)//, MSocketNotifier& aNotifier)
{
  CRoadMapNativeSocket* self = new ( ELeave ) CRoadMapNativeSocket(apHostname, aPort);
  CleanupStack::PushL(self);
  self->ConstructL(apCallback, apContext);//, aNotifier);
//TODO use TRAPD with  User::LeaveIfError(aErrorCode);
  return self;
}

int CRoadMapNativeSocket::GetPortFromService(const char * apService)
{
  RServiceResolver resolver;
  TBuf<256> servName;
  TPortNum portNum;
  
  GSConvert::CharPtrToTDes16(apService, servName);
  TInt error = resolver.Open(*m_pSocketServer, KAfInet, KSockStream, KProtocolInetTcp);

  error = resolver.GetByName(servName, portNum);  
  resolver.Close();
  if ( error == KErrNone )
  {
    return -1;
  }
  
  return portNum();
}

void CRoadMapNativeSocket::ConstructL(RoadMapNetConnectCallback apCallback, void* apContext)//, MSocketNotifier& aNotifier)
{
  CRoadMapNativeNet::ConstructL(apCallback, apContext);
  m_eNetStatus = ESockStatusReady;
  
  StartL();

  if ( m_pTimeoutTimer == NULL )
  {
    m_pTimeoutTimer = CTimeoutTimer::NewL(*this);
  }
  
  CActiveScheduler::Add(this);  
}

void CRoadMapNativeSocket::OpenL(ENativeSockType aSockType)
{
  TInt err = KErrNone;
  switch (aSockType)
  {
  case ESockTypeTcp:
    err = m_Socket.Open(*m_pSocketServer, KAfInet, KSockStream, KProtocolInetTcp, *m_pConnection);
    break;
  case ESockTypeUdp:
    err = m_Socket.Open(*m_pSocketServer, KAfInet, KSockDatagram, KProtocolInetUdp, *m_pConnection);
    break;
  default:
    err = KErrNotSupported;
    break;
  }
  User::LeaveIfError(err);
  
  m_pReadSocket = CReadSocket::NewL(&m_Socket, *this);
  m_pWriteSocket = CWriteSocket::NewL(&m_Socket, *this);
  ConnectWithParamsL();
}
/*
void CRoadMapNativeSocket::ConnectL(int aAddress, int aPort)
{
  TInetAddr address;
  address.SetPort(aPort);
  address.SetAddress(aAddress);
  m_Socket.Connect(address, iStatus);
  m_eNetStatus = ESockStatusConnecting;
  SetActive();
}
*/
void CRoadMapNativeSocket::ConnectL(const TDesC& aHostname, int aPort)
{
  roadmap_net_mon_connect();
  User::LeaveIfError(m_HostResolver.Open(*m_pSocketServer, KAfInet, KProtocolInetUdp));
  // Issue DNS request
  TRequestStatus aStatus;
  m_HostResolver.GetByName(aHostname, m_NameEntry, aStatus);
  m_eNetStatus = ESockStatusDNSLookup;
  User::WaitForRequest(aStatus);
  m_eNetStatus = ESockStatusDNSDone;

  m_Addr = m_NameEntry().iAddr;
  m_Addr.SetPort(aPort);
  // And connect to the IP address
  ConnectL(m_Addr);
}

void CRoadMapNativeSocket::ConnectL(TInetAddr& addr)
{
  m_eNetStatus = ESockStatusConnecting;
  m_pTimeoutTimer->After(conn_timeout);
  if ( m_pConnectCallback == NULL )
  {// no callback means a synchronous connect
    TRequestStatus aStatus;
    m_Socket.Connect(addr, aStatus);
    User::WaitForRequest(aStatus);
    //  Cancel the timer if sync connected/failed
    m_pTimeoutTimer->Cancel();
    if ( aStatus == KErrNone) 
    {
      m_eNetStatus = ESockStatusConnected;
    }
  }
  else
  {
    m_Socket.Connect(addr, iStatus);
    SetActive();
  }
}

int CRoadMapNativeSocket::Read(void *data, int length)
{
  if ((m_eNetStatus == ESockStatusConnected) && !m_pReadSocket->IsActive())
  {
    return m_pReadSocket->IssueReceiveRequest(data, length);
  }
  else
  {
    return -1;
  }
}

int CRoadMapNativeSocket::Write(const void *data, int length)
{
  if ((m_eNetStatus == ESockStatusConnected) && !m_pWriteSocket->IsActive())
  {
    return m_pWriteSocket->IssueWriteRequest(data, length);
  }
  else
  {
    return -1;
  }
}

void CRoadMapNativeSocket::Close()
{
  m_Socket.Close();
  roadmap_net_mon_disconnect();
  //m_pConnection->Close();
  //m_pSocketServer->Close();
//TODO not here  free(m_pIO);
}


void CRoadMapNativeSocket::InvokeIOCallback(int aConnectionStatus)
{
  if ( m_pConnectCallback != NULL )
  {// since we got here it's obviously not null, but checking anyway
    //  Set up the IO object
    //  Inform our client that we have a connection
	if (aConnectionStatus == 0)
		(*m_pConnectCallback)(this, m_context, succeeded);
	else
		(*m_pConnectCallback)(this, m_context, err_net_failed);
  }
}

void CRoadMapNativeSocket::RunL()
{
  m_pTimeoutTimer->Cancel();
  if ( iStatus == KErrNone )
  {
    switch ( m_eNetStatus )
    {
      case ESockStatusNotStarted:
        UpdateIAPid();
        ConnectWithParamsL();
        break;
      case ESockStatusConnecting:
        m_eNetStatus = ESockStatusConnected;
        InvokeIOCallback(0);  //  (iStatus.Int());
        break;
      case ESockStatusConnected: 
        break;
      case ESockStatusTimedOut: 
        InvokeIOCallback(iStatus.Int());
        break;
      default:
        break;
    }
  }
  else 
  {
    SetActive();  //  go on with the state machine
  }
}

void CRoadMapNativeSocket::DoCancel()
{
  //TODO state machine?
  m_pTimeoutTimer->Cancel();
  m_Socket.CancelAll();
  
}

void CRoadMapNativeSocket::StartPolling(void* apInputCallback, void* apIO)
{
  if ( m_pReadSocket != NULL )
  {
    m_pReadSocket->IssuePollRequest(apInputCallback, apIO);
  }
}

void CRoadMapNativeSocket::StopPolling()
{
  if ( m_pReadSocket != NULL )
  {
    m_pReadSocket->IssuePollRequest(NULL, NULL);
  }
}

void CRoadMapNativeSocket::DataReady(const void* /*apData*/)
{
  
}

void CRoadMapNativeSocket::OnConnectTimeout()
{
  Cancel();
  if ( m_eNetStatus == ESockStatusConnecting || m_pConnectCallback != NULL )
  {
    m_Socket.CancelConnect();
  }
  m_eNetStatus = ESockStatusTimedOut;
  TRequestStatus* timerStatus = &iStatus;
  SetActive();
  User::RequestComplete(timerStatus, ESockStatusTimedOut);
}

void CRoadMapNativeSocket::ConnectWithParamsL()
{
  TInetAddr addr;
  TBuf<256> hostName;
  char *hostname = NULL;
  char *separator = NULL; 
  char *separator_slash = NULL;
  bool isIPAddr = false;
  int port = -1;
  int err = 0; 
  
  hostname = strdup(m_hostname);
  roadmap_check_allocated(hostname);
  isIPAddr = isdigit(m_hostname[0]);
  separator = strchr (m_hostname, ':');  

  // Get port number, either parsing or default to the one supplied
  if (separator != NULL) 
  {
    //TODO fix
    //port = s->GetPortFromService(separator+1);
    if (port < 0) 
    {
      if (isdigit(separator[1])) 
      {
        port = atoi(separator+1);
        if (port == 0) 
        {
          roadmap_log (ROADMAP_ERROR, "invalid port in '%s'", m_hostname);
          free(hostname);
          User::Leave(KErrArgument);  //  bad params
        }
      } 
      else 
      {
        roadmap_log (ROADMAP_ERROR, "invalid service in '%s'", m_hostname);
        free(hostname);
        User::Leave(KErrArgument);  //  bad params
      }
    } 
    *(strchr(hostname, ':')) = 0;
  } 
  else 
  {
    port = m_port;
  }
    
  //  init addr with port
  addr.SetPort(port);
  //  init addr with hostname (if any)
  GSConvert::CharPtrToTDes16(hostname, hostName);
  free(hostname);
  if (isIPAddr == true) 
  {
    addr.Input(hostName);
    ConnectL(addr);
  } 
  else 
  {
    ConnectL(hostName, port);
  }
}

TInt CRoadMapNativeSocket::ConnAsyncStart(TConnPref &aConnPrefs)
{
  TInt err = KErrNone;
  m_eNetStatus = ESockStatusNotStarted;
  m_pConnection->Start(aConnPrefs, iStatus);
  SetActive();
  if ( m_pConnectCallback == NULL )
  {// complete immediately
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);
  }
  return err;
}

TInt CRoadMapNativeSocket::ConnAsyncStart()
{
  TInt err = KErrNone;
  m_eNetStatus = ESockStatusNotStarted;
  m_pConnection->Start(iStatus);
  SetActive();
  if ( m_pConnectCallback == NULL )
  {// complete immediately
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);
  }
  return err;
}

void CRoadMapNativeSocket::OpenSession()
{
  //  NOT IMPLEMENTED; PLEASE KEEP EMPTY
}
