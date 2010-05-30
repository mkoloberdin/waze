/* RoadmapNativeHTTP.h - Symbian HTTP implementation for Roadmap
 *
 * LICENSE:
 *
 *   Copyright 2009 Maxim Kalaev
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

#include <httpstringconstants.h>
#include <http/RHTTPSession.h>
#include <http/MHTTPDataSupplier.h>
#include <http/MHTTPTransactionCallback.h>

#include <time.h>
#include <zlib.h>

class PostDataSupplier: public MHTTPDataSupplier
{
public:
    PostDataSupplier( void );
    virtual ~PostDataSupplier( void );

    // MHTTPDataSupplier interface
    virtual TBool GetNextDataPart(TPtrC8& aDataPart);
    virtual void  ReleaseData();
    virtual TInt  OverallDataSize();
    virtual TInt  Reset();

    // Interface to add data to be posted
    void          InitL( RHTTPTransaction& transaction );
    void          AddDataL( const TDesC8& aDes );

protected:
    CBufSeg*         m_Data;
    TBool            m_finalized; 
    size_t           m_overAllSize;
    RHTTPTransaction m_Transaction;
};


class CRoadMapNativeHTTP :  public CActive, 
                            public CRoadMapNativeNet,
                            public MHTTPTransactionCallback
{

public: 
  //  ctor/dtor
  virtual ~CRoadMapNativeHTTP();
  static CRoadMapNativeHTTP* CRoadMapNativeHTTP::NewL(const char *http_type, const char *apHostname, int aPort, int flags, time_t tUpdate, RoadMapNetConnectCallback apCallback, void* apContext);
  
  //  From CRoadMapNativeNet
  virtual void OpenL(ENativeSockType aSockType);  
  virtual void ConnectL(const TDesC& aHostname, int aPort);
  virtual int Read(void *data, int length);
  virtual int Write(const void *data, int length);
  virtual void Close();
  virtual void StartPolling(void* apInputCallback, void* apIO);
  virtual void StopPolling();  
  
  //  From MHTTPTransactionCallback
  virtual void MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent &aEvent);
  virtual TInt MHFRunError(TInt aError, RHTTPTransaction aTransaction, const THTTPEvent &aEvent);
  
  inline bool ReadyToSendData()  { return m_IsReadyToSendData; }
  void SetRequestProperty(const char* aKey, const char* aValue);
  void SetReadyToSendData(bool aIsReady);
  
protected:
  CRoadMapNativeHTTP(const char *http_type, const char *apHostname, int aPort, int flags, time_t tUpdate);
  static CRoadMapNativeHTTP* CRoadMapNativeHTTP::NewLC(const char *http_type, const char *apHostname, int aPort, int flags, time_t tUpdate, RoadMapNetConnectCallback apCallback, void* apContext);
  void ConstructL(RoadMapNetConnectCallback apCallback, void* apContext);
  
  void ConnectWithParamsL();
  TInt ConnAsyncStart(TConnPref &aConnPrefs);
  TInt ConnAsyncStart();
  TInt ConnStart( TConnPref &aConnPrefs );	// Connection start. Implementation dependent can be synchronous or asynchronous
  TInt ConnSyncStart(TConnPref &aConnPrefs);

  void ProcessReceivedHttpHeader( RHTTPTransaction aTransaction );
  void ProcessReceivedHttpBodyChunk( RHTTPTransaction aTransaction );
  void SelfSignalDataEvent();

  TInt ReadHttpHeader( void* buf, TInt maxSize );
  TInt ReadHttpBodyChunk( void* buf, TInt maxSize );
  void IssueCallback();
  
  void RunL();
  void DoCancel();
  
  void SetHeaderL(RHTTPHeaders aHeaders, TInt aHdrField, const TDesC8& aHdrValue);
  TInt TranslateToStringField(const char* aField);
  void SetConnectionParams();
  void SetModifiedSinceL();
  virtual void OpenSession();
  
  enum EConnStatus{
    EConnStatusNotStarted      = -1,
    EConnStatusConnected       = 4,
    EConnStatusDataReceived    = 5,
    EConnStatusTransactionDone = 6,
    EConnStatusClosed = -2,
    EConnStatusError  = -100
  }m_eConnStatus;
  
  // Symbian HTTP
  RHTTPSession m_Session;
  RHTTPTransaction m_Transaction;
  
  // Posting data data
  CUri8*             m_pUri8;
  HTTP::TStrings     m_HttpMethod;
  PostDataSupplier   m_PostData;
  bool               m_IsReadyToSendData;

  // Receiving reply
  int                m_ContentLength; // Not used much
  int                m_Status;
  TPtrC8             m_contentEncoding;
  MHTTPDataSupplier* m_pBodySup;
  TBuf8<128>         m_CurReplyHttpHeader;
  TPtrC8             m_CurBodyChunk;
  
  void*                m_RecvCallback;  /* RoadMapIO */
  void*                m_apIO;          /* roadmap_main_io */
  int                  m_flags;
  time_t               m_tModifiedSince;  

  CActiveSchedulerWait iSchedulerWait;

  // Decompressing Gzip Content Encoding
  struct deflate_ctx_t{
      z_stream stream;
      bool     gotHeaders;
      TUint32  crc;
      void InitL( void );
  }m_DeflateCtxt;
};


#endif  //  _ROADMAP_NATIVE_HTTP__H_
