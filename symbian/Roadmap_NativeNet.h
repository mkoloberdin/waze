/* RoadmapNativeNet.h - Symbian network implementation for Roadmap
 *  This is an abstract interface to the network functionality. 
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

#ifndef _ROADMAP_NATIVE_NET__H_
#define _ROADMAP_NATIVE_NET__H_

extern "C" {
#include "roadmap_net.h"
}

#include <es_sock.h>
#include <in_sock.h>
#include <es_enum.h>
#include <commdbconnpref.h>

typedef enum 
{
  ESockTypeInvalid = -1,
  ESockTypeTcp,
  ESockTypeUdp,
  ESockTypeIcmp,
  ESockTypeHttp,
} ENativeSockType;

class CRepository;
class CAknWaitDialog;
class CRoadMapNativeNet;

class CDialogTimer : public CTimer
{
public: 
  virtual ~CDialogTimer();
  static CDialogTimer* NewL();
  void ConstructL();
  void Start();
  
protected:
  CDialogTimer(TInt aPriority);
  void RunL();
  void DoCancel();

  CActiveSchedulerWait* m_pWaiter;
  TInt m_State;
  enum {
    EDismissAP,
    EShowWait,
  };
};


class CRoadMapNativeNet
{
public: 
  virtual void OpenL(ENativeSockType aSockType) = 0;
//  virtual void ConnectL(TInetAddr& addr) = 0;
  virtual void ConnectL(const TDesC& aHostname, int aPort) = 0;   
  
  virtual int Read(void *data, int length) = 0;
  virtual int Write(const void *data, int length) = 0;
  virtual void Close() = 0;
  
  virtual void StartPolling(void* apInputCallback, void* apIO) = 0;
  virtual void StopPolling() = 0;  
  
  static void SetChosenAP( TUint32 val ) { CRoadMapNativeNet::m_ChosenAP = val; }
  static int GetChosenAP() { return m_ChosenAP; }
  
  void StartL();
  static void Shutdown();  
  
  inline bool IsHttp()  { return (m_isHttp==1);  }
  
  virtual ~CRoadMapNativeNet();
  
protected: 
  void ConstructL(RoadMapNetConnectCallback apCallback, void* apContext); 
  CRoadMapNativeNet(const char *apHostname, int aPort);
  void StartConnectionL();
  void InternalStartConnectionL();
  virtual void ConnectWithParamsL() = 0;
  virtual TInt ConnAsyncStart(TConnPref &aConnPrefs) = 0;
  virtual TInt ConnAsyncStart() = 0;
  virtual void OpenSession() = 0;
  void UpdateIAPid();
  bool FindExistingConnection();  
  bool OfflineProfileSwitch();
  
  static RSocketServ* m_pSocketServer;
  static RConnection* m_pConnection;
  static bool m_isConnectionOpen;  
  static TUint32 m_ChosenAP;
  int m_LastProfileId;
  CRepository* m_pRepository;
  
  RoadMapNetConnectCallback m_pConnectCallback;
  void *m_context;
  char *m_hostname;
  int   m_port;
  
  CAknWaitDialog* m_pWaitDialog;
  CDialogTimer* m_pDialogTimer;
  
  int m_isHttp; //  =1 if this is an http protocol
};

/*
class CRoadMapNativeNet : public CRoadMapNativeNet, public TSocketConnection
{
public: 
  virtual void OpenL(ENativeSockType aSockType, RoadMapNetConnectCallback apCallback, void *context) = 0;
  virtual void ConnectL(TInetAddr& addr) = 0;
  virtual void ConnectL(const TDesC& aHostname, int aPort) = 0;   
  
  virtual int Read(void *data, int length) = 0;
  virtual int Write(const void *data, int length) = 0;
  virtual void Close() = 0;
  
  virtual void StartPolling(void* apInputCallback, void* apIO) = 0;
  virtual void StopPolling() = 0;   
};
*/
#endif  //  _ROADMAP_NATIVE_NET__H_
