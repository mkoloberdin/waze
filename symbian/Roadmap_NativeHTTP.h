/* RoadmapNativeHTTP.h - Symbian HTTP implementation for Roadmap
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

#ifndef _ROADMAP_NATIVE_HTTP__H_
#define _ROADMAP_NATIVE_HTTP__H_

#include "Roadmap_NativeNet.h"

#include <http/RHTTPSession.h>
#include <http/MHTTPDataSupplier.h>
#include <http/MHTTPTransactionCallback.h>

#include <time.h>

#define MAX_WRITE_BUFFERS 20

typedef enum 
{
  EConnStatusNotStarted = -1,
  EConnStatusConnected = 4,
  EConnStatusClosed = -2,
  EConnStatusError = 100
} EConnStatus;

class CRoadMapNativeHTTP :  public CActive, 
                            public CRoadMapNativeNet,
                            public MHTTPDataSupplier,
                            public MHTTPTransactionCallback
{

public: 
  //  ctor/dtor
  virtual ~CRoadMapNativeHTTP();
  static CRoadMapNativeHTTP* CRoadMapNativeHTTP::NewL(const char *http_type, const char *apHostname, int aPort, time_t tUpdate, RoadMapNetConnectCallback apCallback, void* apContext);
  
  //  From CRoadMapNativeNet
  virtual void OpenL(ENativeSockType aSockType);  
  virtual void ConnectL(const TDesC& aHostname, int aPort);
  
  virtual int Read(void *data, int length);
  virtual int Write(const void *data, int length);
  virtual void Close();
  
  virtual void StartPolling(void* apInputCallback, void* apIO);
  virtual void StopPolling();  
  
  //  From MHTTPDataSupplier
  virtual void MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent &aEvent);
  virtual TInt MHFRunError(TInt aError, RHTTPTransaction aTransaction, const THTTPEvent &aEvent);
  virtual TBool GetNextDataPart(TPtrC8& aDataPart);
  virtual void ReleaseData();
  virtual TInt OverallDataSize();
  virtual TInt Reset();
  
  void SetRequestProperty(const char* aKey, const char* aValue);
  inline bool ReadyToSendData()  { return m_IsReadyToSendData; }
  void SetReadyToSendData(bool aIsReady);
  
protected:
  CRoadMapNativeHTTP(const char *http_type, const char *apHostname, int aPort, time_t tUpdate);
  static CRoadMapNativeHTTP* CRoadMapNativeHTTP::NewLC(const char *http_type, const char *apHostname, int aPort, time_t tUpdate, RoadMapNetConnectCallback apCallback, void* apContext);
  void ConstructL(RoadMapNetConnectCallback apCallback, void* apContext);
  int IssueCallback(TPtrC8 &data);
  
  void ConnectWithParamsL();
  TInt ConnAsyncStart(TConnPref &aConnPrefs);
  TInt ConnAsyncStart();
  
  void RunL();
  void DoCancel();
  EConnStatus m_eConnStatus;
  
private:
  void SetHeaderL(RHTTPHeaders aHeaders, TInt aHdrField, const TDesC8& aHdrValue);
  TInt TranslateToStringField(const char* aField);
  //void SetHttpVersion(HTTP::TStrings aHttpVersion);
  void SetConnectionParams();
  void SetModifiedSince();
  void OpenSession();
  
  RHTTPSession m_Session;
  RHTTPTransaction m_Transaction;
  
  HBufC8* m_AccumWriteBuffer;
  HBufC8* m_WriteBuffers[MAX_WRITE_BUFFERS];
  int m_NumWriteBuffers;
  int m_NumWriteSent;

  HBufC8* m_ReadBuffer;
  
  bool m_IsLastChunk;
  bool m_IsReadyToSendData;
  int m_ContentLength;
  TPtrC8 m_DataChunk;
  int m_Status;
  
  void *m_RecvCallback;
  void *m_apIO;	  
  CUri8* m_pUri8;
  const char *m_httpType;
  time_t m_tModifiedSince;
  CActiveSchedulerWait iSchedulerWait;
};

class CDestroyTimer : public CTimer 
{
public: 
  virtual ~CDestroyTimer();
  static CDestroyTimer* NewL(CRoadMapNativeHTTP& aObj);
  void ConstructL(CRoadMapNativeHTTP &aObj);
protected:
	CDestroyTimer();
  virtual void RunL();
  
  CRoadMapNativeHTTP* m_pObj;
};


#endif  //  _ROADMAP_NATIVE_HTTP__H_
