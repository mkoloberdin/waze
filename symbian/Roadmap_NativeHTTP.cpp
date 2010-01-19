/* RoadmapNativeHTTP.cpp - Symbian HTTP implementation for Roadmap
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

#include <Uri8.h>
#include <UriUtils.h>
#include <tinternetdate.h>
#include <httpstringconstants.h>
#include <http/RHTTPHeaders.h>
#include <string.h>
#include <stdlib.h>

#include "GSConvert.h"
#include "Roadmap_NativeHTTP.h"
extern "C" {
#include "Roadmap_NativeSocket.h"
#include "roadmap_main.h"
#include "roadmap_net_mon.h"
}

CRoadMapNativeHTTP::CRoadMapNativeHTTP(const char *http_type, const char *apHostname, int aPort, time_t tUpdate)
  : CActive(EPriorityNormal), CRoadMapNativeNet(apHostname, aPort), m_httpType(http_type), m_tModifiedSince(tUpdate)
{
  m_isHttp = 1;
  m_IsLastChunk = false;
  m_NumWriteBuffers = 0;
  m_AccumWriteBuffer = NULL;
  m_NumWriteSent = 0;
  m_ReadBuffer = NULL;
  m_Status = 0;
  m_eConnStatus = EConnStatusNotStarted;
  m_pUri8 = NULL;
}

CRoadMapNativeHTTP::~CRoadMapNativeHTTP()
{
	m_Session.Close();
	while (m_NumWriteBuffers > 0) {
		m_NumWriteBuffers--;
		delete m_WriteBuffers[m_NumWriteBuffers];
	}
	
	if (m_AccumWriteBuffer) delete m_AccumWriteBuffer;
	if (m_ReadBuffer) delete m_ReadBuffer;
}

CRoadMapNativeHTTP* CRoadMapNativeHTTP::NewL(const char *http_type, const char *apHostname, int aPort, time_t tUpdate, RoadMapNetConnectCallback apCallback, void* apContext)
{
  CRoadMapNativeHTTP* self = CRoadMapNativeHTTP::NewLC(http_type, apHostname, aPort, tUpdate, apCallback, apContext);
  CleanupStack::Pop(self);
  return self;
}

CRoadMapNativeHTTP* CRoadMapNativeHTTP::NewLC(const char *http_type, const char *apHostname, int aPort, time_t tUpdate, RoadMapNetConnectCallback apCallback, void* apContext)
{
  CRoadMapNativeHTTP* self = new ( ELeave ) CRoadMapNativeHTTP(http_type, apHostname, aPort, tUpdate);
  CleanupStack::PushL(self);
  self->ConstructL(apCallback, apContext);
//TODO use TRAPD with  User::LeaveIfError(aErrorCode);
  return self;
}

void CRoadMapNativeHTTP::ConstructL(RoadMapNetConnectCallback apCallback, void* apContext)
{
  CRoadMapNativeNet::ConstructL(apCallback, apContext);  
  if ( OfflineProfileSwitch() )
  {//TODO is this necessary here? We're ConstructLing....
    roadmap_log(ROADMAP_DEBUG, "CLOSING SESSION");
    m_Session.Close();
  }

  CActiveScheduler::Add(this);  
  
  StartL();
}

void CRoadMapNativeHTTP::OpenL(ENativeSockType aSockType)
{
  if ( aSockType != ESockTypeHttp )
  {
    //roadmap_log(ROADMAP_ERROR, "Wrong protocol");
    return;
  }
  /*
  TUriParser8 uri; 
  uri.Parse(aUri);
  m_Transaction = m_Session.OpenTransactionL(uri, *this, aMethod);
  RHTTPHeaders hdr = iTrans.Request().GetHeaderCollection();
*/
}

void CRoadMapNativeHTTP::ConnectWithParamsL()
{
  TBuf<256> hostName;
  GSConvert::CharPtrToTDes16(m_hostname, hostName);
  ConnectL(hostName, m_port);
}

void CRoadMapNativeHTTP::ConnectL(const TDesC& aHostname, int aPort)
{
  if ( m_pUri8 != NULL )
  {
    delete m_pUri8;
    m_pUri8 = NULL;
  }
  TRAPD(err, m_pUri8 = UriUtils::CreateUriL(aHostname));
  if ( err != KErrNone || m_pUri8 == NULL )
  {
    roadmap_log(ROADMAP_ERROR, "Could not parse URL %s", aHostname.Ptr());
  }

  HTTP::TStrings http_method = HTTP::EPOST;
  
  if (!strcmp(m_httpType, "GET")) http_method = HTTP::EGET;
  RStringF method = m_Session.StringPool().StringF(http_method, RHTTPSession::GetTable());
  SetConnectionParams();
  SetModifiedSince();
  
  m_Transaction = m_Session.OpenTransactionL(m_pUri8->Uri(), *this, method);
  if ( m_pConnectCallback != NULL )
  {// since we got here it's obviously not null, but checking anyway
    //  Set up the IO object
    //  Inform our client that we have a connection
   // roadmap_log(ROADMAP_DEBUG, "callback %x", m_context);
    (*m_pConnectCallback)(dynamic_cast<CRoadMapNativeNet*>(this), m_context, succeeded);
  }
}

void CRoadMapNativeHTTP::SetReadyToSendData(bool aIsReady)
{
	MHTTPDataSupplier* dataSupplier = this;
	m_IsReadyToSendData = aIsReady;
	if (!strcmp(m_httpType, "POST")) {
		m_Transaction.Request().SetBody(*dataSupplier);
	}
	// Submit the transaction. After this the framework will give transaction
	// events via MHFRunL and MHFRunError.
	m_Transaction.SubmitL();
	
	roadmap_net_mon_connect();
}

int CRoadMapNativeHTTP::Read(void *data, int length)
{
  if ((m_DataChunk.Size() == 0) && (!m_RecvCallback)) {
     iSchedulerWait.Start(); 
  }
  if (m_DataChunk.Size() == 0) return -1;
  
  if (length > m_DataChunk.Size()) length = m_DataChunk.Size();
  
  memcpy(data, (char *)m_DataChunk.Ptr(), length);
  m_DataChunk.Set(m_DataChunk.Right(m_DataChunk.Size() - length));
  
  return length;
}

int CRoadMapNativeHTTP::Write(const void *data, int length)
{
  if (length == 0) return 0;
  
  if (m_NumWriteBuffers == MAX_WRITE_BUFFERS) {
     return -1;
  }
  
  m_WriteBuffers[m_NumWriteBuffers] = HBufC8::NewL(length); 
  m_WriteBuffers[m_NumWriteBuffers]->Des().Copy((TUint8 *)data, length);
  m_NumWriteBuffers++;
//  m_Transaction.NotifyNewRequestBodyPartL();
  //m_Asw.Start();
  return length;
}

void CRoadMapNativeHTTP::Close()
{
  m_Transaction.Close();
  m_eConnStatus = EConnStatusClosed;
  CDestroyTimer::NewL(*this)->After(1);  
}

void CRoadMapNativeHTTP::StartPolling(void* apInputCallback, void* apIO)
{
	m_RecvCallback = apInputCallback;
	m_apIO = apIO;	
}

void CRoadMapNativeHTTP::StopPolling() 
{
  
}

extern "C" int SYMBIAN_HACK_NET;
int CRoadMapNativeHTTP::IssueCallback(TPtrC8 &data)
{

	if (m_eConnStatus == EConnStatusClosed) return 1;
	
    if (!m_RecvCallback) {
    	/* Synchronous mode */
    	if (m_ReadBuffer) delete(m_ReadBuffer);
	    m_ReadBuffer = HBufC8::NewL(data.Size()); 
	    m_ReadBuffer->Des().Copy(data);
	    m_DataChunk.Set(m_ReadBuffer->Des());

        iSchedulerWait.AsyncStop();
        return 1;
    }

    roadmap_main_io *main_io = (roadmap_main_io *) m_apIO;
	RoadMapIO *io = main_io->io;

    m_DataChunk.Set(data);
	int size = m_DataChunk.Size();
	
	if (global_FreeMapLock() != 0) {
	   SYMBIAN_HACK_NET = 1;
	}
	if (size > 0) {		
	
		while (main_io->is_valid && (io->subsystem != ROADMAP_IO_INVALID) &&
				m_DataChunk.Size()) {
				(*(RoadMapInput)m_RecvCallback) ((RoadMapIO *)io);
		}
	} else {
		(*(RoadMapInput)m_RecvCallback) ((RoadMapIO *)io);
	}
	
	if (SYMBIAN_HACK_NET) {
		SYMBIAN_HACK_NET = 0;
	} else {
		global_FreeMapUnlock();
	}
	if ((io->subsystem == ROADMAP_IO_INVALID) || !main_io->is_valid) {
       if (main_io->is_valid) {
          main_io->is_valid = 0;
       } else {
          free (main_io);
       }
       
       return 0;
	} else {
	   return 1;
	}
}

void CRoadMapNativeHTTP::MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent &aEvent)
{
int i;
switch (aEvent.iStatus)
  {
  case THTTPEvent::EGotResponseHeaders:
    {
    // HTTP response headers have been received. Use
    // aTransaction.Response() to get the response. However, it's not
    // necessary to do anything with the response when this event occurs.

    // Get HTTP status code from header (e.g. 200)
    RHTTPResponse resp = aTransaction.Response();
    m_Status = resp.StatusCode();
    RHTTPHeaders headers = resp.GetHeaderCollection();
	THTTPHdrVal contentLength;
	THTTPHdrVal lastModified;
	RStringPool pool = m_Session.StringPool();
	TInt err=headers.GetField(pool.StringF(HTTP::EContentLength, RHTTPSession::GetTable()),0,contentLength);
	m_ContentLength = 0;
	char ptr[255];
	if (err==KErrNone)
	{
		if (contentLength.Type()==THTTPHdrVal::KTIntVal)
		{
			m_ContentLength = contentLength.Int();
		}
	}

	err=headers.GetField(pool.StringF(HTTP::ELastModified, RHTTPSession::GetTable()),0,lastModified);
	if (err == KErrNotFound){
		snprintf((char *)ptr, sizeof(ptr), "HTTP/1.1 %d OK\r\nContent-Length: %d\r\n\r\n", m_Status, m_ContentLength);
	}
	else{
	    if (lastModified.Type()==THTTPHdrVal::KDateVal){
	        char char_date[128];
			TDateTime date = lastModified.DateTime();
			TInternetDate internetDate(date);
			HBufC8* textDate = internetDate.InternetDateTimeL(TInternetDate::ERfc1123Format);
			TInt len = textDate->Length(); 
			strncpy(char_date, (const char*)textDate->Ptr(), len);
			char_date[len] = 0;
			snprintf((char *)ptr, sizeof(ptr), "HTTP/1.1 %d OK\r\nContent-Length: %d\r\nLast-Modified: %s\r\n\r\n", m_Status, m_ContentLength, char_date);
	    }
	}
	
    TPtrC8 d;
    d.Set((TUint8 *)ptr, strlen(ptr));
	IssueCallback(d);

    // Get status text (e.g. "OK")
    //TBuf<KStatustextBufferSize> statusText;
    //statusText.Copy(resp.StatusText().DesC());

    //HBufC* resHeaderReceived = StringLoader::LoadLC(R_HTTP_HEADER_RECEIVED, statusText, status);
    //CleanupStack::PopAndDestroy(resHeaderReceived);
    }
    break;

  case THTTPEvent::EGotResponseBodyData:
    {
    // Part (or all) of response's body data received. Use
    // aTransaction.Response().Body()->GetNextDataPart() to get the actual
    // body data.

    // Get the body data supplier
    MHTTPDataSupplier* body = aTransaction.Response().Body();

    // GetNextDataPart() returns ETrue, if the received part is the last
    // one.
    TPtrC8 dataChunk;
    TBool isLast = body->GetNextDataPart(dataChunk);
    
    if (IssueCallback(dataChunk)) {
    	body->ReleaseData();
    }

    }
    break;

  case THTTPEvent::EResponseComplete:
    {
    i = 7;
    // Indicates that header & body of response is completely received.
    // No further action here needed.
    //HBufC* resTxComplete = StringLoader::LoadLC(R_HTTP_TX_COMPLETE);
    //iObserver.ClientEvent(*resTxComplete);
    //CleanupStack::PopAndDestroy(resTxComplete);
    }
    break;

  case THTTPEvent::ESucceeded:
    {
    // Indicates that transaction succeeded.
    // Transaction can be closed now. It's not needed anymore.
    aTransaction.Close();
    }
    break;

  case THTTPEvent::EFailed:
    {
    // Transaction completed with failure.
    aTransaction.Close();
    TPtrC8 null(NULL, 0);
    IssueCallback(null);
    }
    break;

  default:
    // There are more events in THTTPEvent, but they are not usually
    // needed. However, event status smaller than zero should be handled
    // correctly since it's error.
    {
    if (aEvent.iStatus < 0)
      {
        // Close the transaction on errors
        //aTransaction.Close();
        TPtrC8 null(NULL, 0);
        IssueCallback(null);
      }
    else
      {
      // Other events are not errors (e.g. permanent and temporary
      // redirections)
      i = 8;
      }
    }
    break;
  }  
}

TInt CRoadMapNativeHTTP::MHFRunError(TInt aError, RHTTPTransaction aTransaction, const THTTPEvent &aEvent)
{
  return KErrNone;
}
  
TBool CRoadMapNativeHTTP::GetNextDataPart(TPtrC8& aDataPart)
{
  if (m_NumWriteBuffers == 0) return ETrue;
  
  if (m_NumWriteBuffers == 1) {
	  aDataPart.Set(m_WriteBuffers[0]->Des());
	  return ETrue;
  }
  
  int i;
  int len = 0;
  
  for (i=0; i<m_NumWriteBuffers; i++) {
	  len += m_WriteBuffers[i]->Length();	  
  }
  
  m_AccumWriteBuffer = HBufC8::NewL(len);

  for (i=0; i<m_NumWriteBuffers; i++) {
	  m_AccumWriteBuffer->Des().Append(m_WriteBuffers[i]->Des());	  
  }  
  
  aDataPart.Set(m_AccumWriteBuffer->Des());
  
  return ETrue;  
}

void CRoadMapNativeHTTP::ReleaseData()
{
  //  Let the Write method continue its work
  int j;
  j = 5;
}

TInt CRoadMapNativeHTTP::OverallDataSize()
{
  return m_ContentLength;    //  known data size 
}

TInt CRoadMapNativeHTTP::Reset()
{
  return KErrNone;
}

void CRoadMapNativeHTTP::SetRequestProperty(const char* aKey, const char* aValue)
{
  TBuf8<256> val;
  RHTTPHeaders hdrs = m_Transaction.Request().GetHeaderCollection();
  TInt hdrField = TranslateToStringField(aKey);
  if ( hdrField < 0 )
  {
    //roadmap_log(ROADMAP_ERROR, "Header %s not added", aKey);
    return;
  }
  
  if (hdrField == HTTP::EContentLength) m_ContentLength = atoi(aValue);
  
  GSConvert::CharPtrToTDes8(aValue, val);
  TRAPD( err, SetHeaderL(hdrs, hdrField, val) );
  if ( err != KErrNone )
  {
    //roadmap_log(ROADMAP_ERROR, "HTTP header set failed for %s=%s", aKey, aValue);
  }
}

void CRoadMapNativeHTTP::SetHeaderL(RHTTPHeaders aHeaders, TInt aHdrField, const TDesC8& aHdrValue)
{
  RStringPool strP = m_Session.StringPool();
  RStringF valStr = strP.OpenFStringL(aHdrValue);
  THTTPHdrVal val(valStr);
  aHeaders.SetFieldL(m_Session.StringPool().StringF(aHdrField,RHTTPSession::GetTable()), val);
  valStr.Close();
}

TInt CRoadMapNativeHTTP::TranslateToStringField(const char* aField)
{
  if ( strcasecmp("User-Agent", aField) == 0 )
  {
    return (TInt)HTTP::EUserAgent;
  }
  else if ( strcasecmp("Content-type", aField) == 0 )
  {
    return (TInt)HTTP::EContentType;
  }
  else if ( strcasecmp("Content-Length", aField) == 0 )
  {
    return (TInt)HTTP::EContentLength;
  }
  else if ( strcasecmp("Authorization", aField) == 0 )
  {
    return (TInt)HTTP::EAuthorization;
  }
  else if (strcasecmp("Last-Modified", aField) == 0 )
  {
	return (TInt)HTTP::ELastModified;
  }
  else 
  {
    return -1;
  }
    
}
/*
void CRoadMapNativeHTTP::SetHttpVersion(HTTP::TStrings aHttpVersion)
{
  RHTTPConnectionInfo connInfo = m_Session.ConnectionInfo();
  RStringPool strP = m_Session.StringPool();
  connInfo.SetPropertyL(strP.StringF(HTTP::EHTTPVersion, RHTTPSession::GetTable()), 
                        THTTPHdrVal(strP.StringF(aHttpVersion)));
}
*/
void CRoadMapNativeHTTP::SetConnectionParams()
{
  RHTTPConnectionInfo connInfo = m_Session.ConnectionInfo();
  connInfo.SetPropertyL(m_Session.StringPool().StringF(HTTP::EHttpSocketServ, RHTTPSession::GetTable()), 
                        THTTPHdrVal(m_pSocketServer->Handle()));
  connInfo.SetPropertyL(m_Session.StringPool().StringF(HTTP::EHttpSocketConnection, RHTTPSession::GetTable()), 
                        THTTPHdrVal(reinterpret_cast<TInt>(m_pConnection)));
  //THTTPHdrVal hdrVal;
  //hdrVal.SetInt((TInt)(HTTP::EHttp10));
  //connInfo.SetPropertyL(m_Session.StringPool().StringF(HTTP::EHTTPVersion, RHTTPSession::GetTable()), 
  //                      THTTPHdrVal(hdrVal));  
//                        THTTPHdrVal(strP.StringF(HTTP::EHttp10)));  
}

void CRoadMapNativeHTTP::SetModifiedSince()
{
  if (m_tModifiedSince == 0) return;
  
  RStringPool strP = m_Session.StringPool();
  RHTTPHeaders hdrs = m_Session.RequestSessionHeadersL(); //m_Transaction.Request().GetHeaderCollection();
  struct tm *tmVals = gmtime (&m_tModifiedSince);
  THTTPHdrVal dateVal (TDateTime (1900 + tmVals->tm_year, TMonth(TInt(tmVals->tm_mon)), tmVals->tm_mday - 1,
				                  tmVals->tm_hour, tmVals->tm_min, tmVals->tm_sec, 0));
  
  hdrs.SetFieldL(strP.StringF(HTTP::EIfModifiedSince,RHTTPSession::GetTable()), dateVal);
}

TInt CRoadMapNativeHTTP::ConnAsyncStart(TConnPref &aConnPrefs)
{
  TInt err = KErrNone;
  m_eConnStatus = EConnStatusNotStarted;
  m_pConnection->Start(aConnPrefs, iStatus);
  SetActive();
  if ( m_pConnectCallback == NULL )
  {// complete immediately
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);
  }
  return err;
}

TInt CRoadMapNativeHTTP::ConnAsyncStart()
{
  TInt err = KErrNone;
  m_eConnStatus = EConnStatusNotStarted;
  m_pConnection->Start(iStatus);
  SetActive(); 
  if ( m_pConnectCallback == NULL )
  {// complete immediately
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);
  }
  return err;
}

void CRoadMapNativeHTTP::OpenSession()
{
  m_Session.OpenL();  
}

void CRoadMapNativeHTTP::RunL()
{
  roadmap_log(ROADMAP_DEBUG, "CRoadMapNativeHTTP::RunL status=%d connStatus=%d", iStatus.Int() , (int)m_eConnStatus);
  if ( iStatus == KErrNone )
  {
    switch ( m_eConnStatus )
    {
      case EConnStatusNotStarted:
        m_eConnStatus = EConnStatusConnected; //  we won't be coming here again anyway... 
        UpdateIAPid();
        m_Session.OpenL();
        ConnectWithParamsL();
        break;
      case EConnStatusConnected: 
        //  this should not happen...
        break;
      default:
        break;
    }
  }
  else if ( iStatus == KErrCancel )
	  {
	  roadmap_net_mon_offline();
	  roadmap_main_exit();
	  }
  else if  ( iStatus == KErrNotFound ||
	  		( iStatus >= -30180 && iStatus <= -30170 ) ) // Exceptional cases. The errors returned in case of access point connection problems
  	{
  		// In this case let the user to select the access point
  		// last chosen AP has to be replaced
  		SetChosenAP( 0 );
  		StartL();
  	}
  else 
  {
    //TODO  keep error code
    m_eConnStatus = EConnStatusError;  
  }
}

void CRoadMapNativeHTTP::DoCancel()
{
  m_pConnection->Close();
  Cancel();
}

CDestroyTimer::CDestroyTimer() : CTimer(EPriorityHigh)
{
  m_pObj = NULL;
}

CDestroyTimer::~CDestroyTimer()
{
  Cancel();
}

CDestroyTimer* CDestroyTimer::NewL(CRoadMapNativeHTTP& aTimeoutObserver)
{
  CDestroyTimer* self = new (ELeave) CDestroyTimer();
  CleanupStack::PushL(self);
  self->ConstructL(aTimeoutObserver);
  CleanupStack::Pop(self);
  return self;
}

void CDestroyTimer::ConstructL(CRoadMapNativeHTTP &aTimeoutObserver)
{
  m_pObj = &aTimeoutObserver;
  CTimer::ConstructL();
  CActiveScheduler::Add(this);
}

void CDestroyTimer::RunL()
{
  delete m_pObj;
  delete this;
}
