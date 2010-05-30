/* RoadmapNativeHTTP.cpp - Symbian HTTP implementation for Roadmap
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

#include <Uri8.h>
#include <UriUtils.h>
#include <http/RHTTPHeaders.h>
#include <tinternetdate.h>
#include <string.h>
#include <stdlib.h>
#include <e32cmn.h> // Min(x,y)

#include "GSConvert.h"
#include "Roadmap_NativeHTTP.h"
extern "C" {
#include "Roadmap_NativeSocket.h"
#include "roadmap_main.h"
#include "roadmap_net_mon.h"
}

CRoadMapNativeHTTP::CRoadMapNativeHTTP(const char *http_type, const char *apHostname, int aPort, int flags, time_t tUpdate)
  : CActive(EPriorityNormal),
    CRoadMapNativeNet(apHostname, aPort),
    m_eConnStatus(EConnStatusNotStarted),
    m_ContentLength(-1),
    m_Status(0),
    m_flags(flags),
	m_tModifiedSince(tUpdate)
{
    m_isHttp = 1;
    m_pUri8 = NULL;
    m_pBodySup = NULL;

    // We only support GET and POST
    m_HttpMethod = (strcmp(http_type, "GET") == 0 ? HTTP::EGET : HTTP::EPOST ); 
    
    memset( &m_DeflateCtxt, 0x00, sizeof(m_DeflateCtxt) );
}

CRoadMapNativeHTTP::~CRoadMapNativeHTTP()
{
    inflateEnd( &m_DeflateCtxt.stream );

    m_Transaction.Close();
    m_Session.Close();

    // Sanity
    m_pBodySup = NULL;
    m_RecvCallback = NULL;
    m_apIO = NULL;
}

CRoadMapNativeHTTP* CRoadMapNativeHTTP::NewL( const char *http_type,
                                              const char *apHostname,
                                              int aPort,
                                              int flags,
											  time_t tUpdate,
                                              RoadMapNetConnectCallback apCallback,
                                              void* apContext )
{
    CRoadMapNativeHTTP* self = new ( ELeave ) CRoadMapNativeHTTP(http_type, apHostname, aPort, flags, tUpdate);
      self->ConstructL(apCallback, apContext);
      return self;
}

void CRoadMapNativeHTTP::deflate_ctx_t::InitL( void )
{
    const TInt ZLIB_WIN_SIZE = 15;
    if( inflateInit2( &stream, -1 * ZLIB_WIN_SIZE ) != Z_OK ) {
        inflateEnd(&stream);
        User::LeaveNoMemory();
    }

    gotHeaders = false;
    crc        = 0x0;
}

void CRoadMapNativeHTTP::ConstructL(RoadMapNetConnectCallback apCallback, void* apContext)
{
    CRoadMapNativeNet::ConstructL(apCallback, apContext);

    // Prepare decompressor, just in case..
    m_DeflateCtxt.InitL();

    CActiveScheduler::Add(this);

    StartL();
}

void CRoadMapNativeHTTP::OpenL(ENativeSockType aSockType)
{
    if( aSockType != ESockTypeHttp )
        User::Panic( _L("Unsupported protocol type"), aSockType );
}

void CRoadMapNativeHTTP::ConnectWithParamsL()
{
    TBuf<256> hostName;
    GSConvert::CharPtrToTDes16(m_hostname, hostName);
    ConnectL(hostName, m_port);

    m_eConnStatus = EConnStatusConnected; 
}

void CRoadMapNativeHTTP::ConnectL(const TDesC& aHostname, int)
{
  delete m_pUri8;
  TRAPD(err, m_pUri8 = UriUtils::CreateUriL(aHostname));
  if ( err != KErrNone || m_pUri8 == NULL ) {
    roadmap_log(ROADMAP_ERROR, "Could not parse URL %s", aHostname.Ptr());
  }

  RStringF method = m_Session.StringPool().StringF(m_HttpMethod, RHTTPSession::GetTable());
  SetConnectionParams();
  SetModifiedSinceL();
  
  m_Transaction = m_Session.OpenTransactionL(m_pUri8->Uri(), *this, method);
  
  m_PostData.InitL( m_Transaction );
  
  //  Inform our client that we have a connection
  if( m_pConnectCallback != NULL ) {
      (*m_pConnectCallback)( dynamic_cast<CRoadMapNativeNet*>(this), m_context, succeeded );
  }
}

void CRoadMapNativeHTTP::SetReadyToSendData(bool aIsReady)
{
    if (TEST_NET_COMPRESS( m_flags ) ) {
        SetRequestProperty("Accept-Encoding", "gzip, deflate");
    }
    
    m_IsReadyToSendData = aIsReady;
    if( m_HttpMethod == HTTP::EPOST ) {
        m_Transaction.Request().SetBody( m_PostData );
    }

    // Submit the transaction. After this the framework will give transaction
    // events via MHFRunL and MHFRunError.
    m_Transaction.SubmitL();

    roadmap_net_mon_connect();
}

/** The function returns data received via HTTP, including HTTP Headers and Body.
 *  The function can work in both syncronious and asynchronious mode
 *   (if StartPolling() was called).
 *
 *  In either case the function always returns at least 1 byte of data unless
 *   error has happened or HTTP transaction was ended and end of data was reached.
 *  If there is no data to return, the function BLOCKS caller if used in syncronous mode
 *   and panics if asyncronous (is not supposed to be called when no data available in async mode).
 *
 *  The function returns:
 *   >0 - Size of Data returned.
 *   0  - End of data, HTTP transaction ended.
 *   <0 - (E.g., negative value) any kind of error has happened.
 */
int CRoadMapNativeHTTP::Read( void *data, int length )
{
    if( m_RecvCallback == NULL ) {
        // Synchronious read mode
        // If no data is available wait while there is a chance for it to arrive
        while( m_eConnStatus > 0 &&
               m_eConnStatus != EConnStatusTransactionDone &&
               m_CurReplyHttpHeader.Size() == 0 && m_CurBodyChunk.Size() == 0 )
        {
            iSchedulerWait.Start();
        }
    }

    // Problems?
    if( m_eConnStatus < 0 )
        return -1;

    // HTTP headers to return?
    if( m_CurReplyHttpHeader.Size() > 0 )
        return ReadHttpHeader( data, length );

    // No data and no more expected?
    if( m_CurBodyChunk.Size() == 0 and m_eConnStatus == EConnStatusTransactionDone ) {
        return 0;
    }
    
    __ASSERT_ALWAYS( m_CurBodyChunk.Size() > 0, User::Panic(_L("CRoadMapNativeHTTP::Read underflow!"), -1) );

    // Return body chunk (decoded)
    return ReadHttpBodyChunk( data, length );
}

int CRoadMapNativeHTTP::Write( const void *data, int length )
{
    TRAPD( ret, m_PostData.AddDataL( TPtrC8((TUint8*)data, length) ) );
    if( ret != KErrNone )
        return -1;

    return length;
}

#define SELF_DESTRUCT_STATUS        666
#define DATA_EVENT_AVAILABLE_STATUS 200

void CRoadMapNativeHTTP::Close()
{
    m_eConnStatus = EConnStatusClosed;
    
    m_RecvCallback = NULL; // Sanity
  
    TRequestStatus* status = &iStatus;

    if ( IsActive() )
	{	
		roadmap_log( ROADMAP_INFO, "Trying to activate already active object. Overriding the status" );
		/*
		 * Just override the status and use previous request
		 */		
		*status = SELF_DESTRUCT_STATUS;
	}
    else
	{
		SetActive();
		User::RequestComplete( status, SELF_DESTRUCT_STATUS );
	}
}


void CRoadMapNativeHTTP::SelfSignalDataEvent()
{
    // Synchronous mode, Read() was blocked, - signal data arrival.
    if( m_RecvCallback == NULL ) {
        if( iSchedulerWait.IsStarted() )
            iSchedulerWait.AsyncStop();
        return;
    }
    
    // Asynchronous mode, Read() is non-blocking.
    TRequestStatus* status = &iStatus;
    if ( IsActive() )
	{						
		roadmap_log( ROADMAP_INFO, "Trying to activate already active object. Overriding the status" );
		/*
		 * Just override the status and use previous request 
		 */
		*status = DATA_EVENT_AVAILABLE_STATUS;
	}
	else
	{
		SetActive();
		User::RequestComplete( status, DATA_EVENT_AVAILABLE_STATUS );
	}
	
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
void CRoadMapNativeHTTP::IssueCallback()
{
    roadmap_main_io *main_io = (roadmap_main_io *) m_apIO;
    RoadMapIO *io = main_io->io;        // TODO: test NULL

    // Feed available data
    if( main_io->is_valid && (io->subsystem != ROADMAP_IO_INVALID) ) {

        if (global_FreeMapLock() != 0) { //TBD: What de hack is this?
           SYMBIAN_HACK_NET = 1; 
        }

        // TODO: test NULL
        ((RoadMapInput)m_RecvCallback) ((RoadMapIO *)io);

        if (SYMBIAN_HACK_NET) {
            SYMBIAN_HACK_NET = 0;
        } else {
            global_FreeMapUnlock();
        }

        // More data available or end of HTTP stream
        if( m_CurBodyChunk.Size() > 0 || m_eConnStatus == EConnStatusTransactionDone ) { 
            // Schedule another notification
            SelfSignalDataEvent();
            return;
        }
    }
    
    if ((io->subsystem == ROADMAP_IO_INVALID) || !main_io->is_valid) {
       if (main_io->is_valid) {
          main_io->is_valid = 0;
       } else {
          free (main_io);
       }
    }
}


void CRoadMapNativeHTTP::MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent &aEvent)
{
    switch (aEvent.iStatus) {
    case THTTPEvent::EGotResponseHeaders:
        // Got HTTP response headers
        
        // Update connection state
        m_eConnStatus = EConnStatusDataReceived;

        (void)ProcessReceivedHttpHeader( aTransaction );
        break;
    
    case THTTPEvent::EGotResponseBodyData:
        // Part or entire of response's body data has been received
        ProcessReceivedHttpBodyChunk( aTransaction );   
        break;
        
    /// One the following two events is always sent.
    /// The transaction can be closed now.

    case THTTPEvent::ESucceeded:
        roadmap_log(ROADMAP_DEBUG, "CRoadMapNativeHTTP: Got THTTPEvent: %d", aEvent.iStatus );
        m_eConnStatus = EConnStatusTransactionDone;
        SelfSignalDataEvent();
        break;

    case THTTPEvent::EFailed:
        roadmap_log(ROADMAP_WARNING, "CRoadMapNativeHTTP: Got THTTPEvent: %d", aEvent.iStatus );
        m_eConnStatus = EConnStatusError;
        SelfSignalDataEvent();
        break;

    default:
        roadmap_log(ROADMAP_WARNING, "CRoadMapNativeHTTP: Ignored event: %d", aEvent.iStatus );
        // All others are ignored.
        break;
    }  
}


void CRoadMapNativeHTTP::ProcessReceivedHttpHeader( RHTTPTransaction aTransaction )
{
    // HTTP response headers have been received.

    RHTTPResponse resp = aTransaction.Response();
    m_Status = resp.StatusCode(); // HTTP status code from header (e.g. 200)
    
    // Parse HTTP headers, retrieve content size and content encoding
    RHTTPHeaders headers = resp.GetHeaderCollection();
    RStringPool pool = m_Session.StringPool();
    THTTPHdrVal lastModified;
    
    RStringF field = m_Session.StringPool().StringF(HTTP::EContentEncoding, RHTTPSession::GetTable());
    m_contentEncoding.Set( NULL, 0 );
    (void)headers.GetRawField(field, m_contentEncoding);

    THTTPHdrVal contentLength;
    field = pool.StringF(HTTP::EContentLength, RHTTPSession::GetTable());
    m_ContentLength = -1;
    TBuf8<64> contentLengthStr = _L8("");
    TInt err=headers.GetField( field, 0, contentLength );
    
    if ((err == KErrNone) && (contentLength.Type() == THTTPHdrVal::KTIntVal)) {
        m_ContentLength = contentLength.Int();

        // Since ContentLength of compressed body doesn't reflect real size of content data,
        //  don't send it in this case.			        
        if( m_ContentLength >= 0 &&
        	m_contentEncoding.Compare(_L8("gzip") ) != 0 &&
        	m_contentEncoding.Compare(_L8("deflate") ) != 0 )
        {
			contentLengthStr.Format(_L8("Content-Length: %d\r\n"), m_ContentLength);
        }
    }
    
    TBuf8<64> lastModifiedStr = _L8("");;
	err=headers.GetField(pool.StringF(HTTP::ELastModified, RHTTPSession::GetTable()),0,lastModified);	
	if ((err == KErrNone) && (lastModified.Type()==THTTPHdrVal::KDateVal)) {
		TDateTime date = lastModified.DateTime();
		TInternetDate internetDate(date);
		HBufC8* textDate = internetDate.InternetDateTimeL(TInternetDate::ERfc1123Format);
		
		lastModifiedStr.Format(_L8("Last-Modified: %s\r\n"), textDate->Des().PtrZ());
	}
		       
    roadmap_log( ROADMAP_DEBUG,
                 "CRoadMapNativeHTTP: Got HTTP headers. Status=%d Content length=%d TE: %s\n",
                 m_Status, m_ContentLength, m_contentEncoding.Ptr() );
    
    m_CurReplyHttpHeader.Format( _L8("HTTP/1.1 %d OK\r\n"
                                     "%s"
                                     "%s"
                                     "\r\n"),
                                 m_Status,                                     
                                 contentLengthStr.PtrZ(),
                                 lastModifiedStr.PtrZ());
  
    SelfSignalDataEvent();
}


void CRoadMapNativeHTTP::ProcessReceivedHttpBodyChunk( RHTTPTransaction aTransaction )
{
    // Get ptr to the data supplier
    m_pBodySup = aTransaction.Response().Body();

    (void)m_pBodySup->GetNextDataPart( m_CurBodyChunk );

    roadmap_log(ROADMAP_DEBUG, "CRoadMapNativeHTTP: Got HTTP body chunk chunk length=%d\n", m_CurBodyChunk.Size() );

    // Shit happens.. Symbian HTTP framework could do this check for us, but it doesn't.
    if( m_CurBodyChunk.Size() == 0 ) {
        m_pBodySup->ReleaseData();
        m_pBodySup = NULL;
        return;
    }

    // Count bytes received
    roadmap_net_mon_recv( m_CurBodyChunk.Size() );
    
    SelfSignalDataEvent();
}

TInt CRoadMapNativeHTTP::ReadHttpHeader( void* aBuf, TInt aBufSz )
{
    TInt sz = Min( m_CurReplyHttpHeader.Size(), aBufSz );
    memcpy( aBuf, m_CurReplyHttpHeader.Ptr(), sz );
    
    // Eat consumed data
    m_CurReplyHttpHeader.Delete( 0, sz );

    return sz;
}


TInt CRoadMapNativeHTTP::ReadHttpBodyChunk( void* aBuf, TInt aBufSz )
{
    if( aBufSz <= 0 or aBuf == NULL )
        return -1;
    
    
    if( m_CurBodyChunk.Size() == 0 )
        return 0;

    TInt zRC = Z_OK;
    TInt consumedBlkSz = 0;
    TInt returnedBlkSz = 0;
    if( m_contentEncoding.Compare(_L8("gzip")) == 0 ) {
        if( !m_DeflateCtxt.gotHeaders ) {
            static const char deflate_magic[2] = {'\037', '\213' };
            // Gzip Header size is 10 bytes.
            // We don'curretnly support replies with header is split across several chunks
            if( m_CurBodyChunk.Size() < 10 )
                return -1;
            
            // Check Gzip header. We don't support also ext. header.
            if( memcmp( m_CurBodyChunk.Ptr(), deflate_magic, 2 ) != 0 or m_CurBodyChunk.Ptr()[3] != 0 )
                return -1;
            
            // Ok, eat the header!
            m_CurBodyChunk.Set( m_CurBodyChunk.Right(m_CurBodyChunk.Size() - 10 ));
            m_DeflateCtxt.gotHeaders = true;
        }
        
        // Decompress received data
        m_DeflateCtxt.stream.next_in   = (TUint8*)m_CurBodyChunk.Ptr(); // const_cast..
        m_DeflateCtxt.stream.avail_in  = m_CurBodyChunk.Size();
        m_DeflateCtxt.stream.next_out  = (TUint8*)aBuf;
        m_DeflateCtxt.stream.avail_out = aBufSz;
        
        zRC = inflate( &m_DeflateCtxt.stream, Z_SYNC_FLUSH );
        if( zRC == Z_STREAM_END || zRC == Z_OK ) {
            // We are not verifying CRC, ignoring data leftovers, etc.
            // Being lazy saves power :)
            consumedBlkSz = m_CurBodyChunk.Size() - m_DeflateCtxt.stream.avail_in;
            returnedBlkSz = aBufSz - m_DeflateCtxt.stream.avail_out;
        }else{
            return -1;
        }
        
        // Ignore compressed stream trailer
        if( zRC == Z_STREAM_END )
            consumedBlkSz = m_CurBodyChunk.Size();
    }else{
        // Assume plain transfer encoding, simply provide the received data
        returnedBlkSz = consumedBlkSz = Min( m_CurBodyChunk.Size(), aBufSz );
        memcpy( aBuf, m_CurBodyChunk.Ptr(), consumedBlkSz );
    }
    // Eat consumed data
    m_CurBodyChunk.Set( m_CurBodyChunk.Right(m_CurBodyChunk.Size() - consumedBlkSz ));

    // Release HTTP Body supplier if body chunk has been consumed completely 
    if( m_CurBodyChunk.Size() == 0 and m_pBodySup != NULL ) {
        m_pBodySup->ReleaseData();
        m_pBodySup = NULL;  //Safety: avoid double 'free'
    }
    
    return returnedBlkSz;
}


TInt CRoadMapNativeHTTP::MHFRunError(TInt, RHTTPTransaction, const THTTPEvent&)
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
  else if ( strcasecmp("Accept-encoding", aField) == 0 )
  {
    return (TInt)HTTP::EAcceptEncoding;
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

void CRoadMapNativeHTTP::SetConnectionParams()
{
  RHTTPConnectionInfo connInfo = m_Session.ConnectionInfo();
  connInfo.SetPropertyL(m_Session.StringPool().StringF(HTTP::EHttpSocketServ, RHTTPSession::GetTable()), 
                        THTTPHdrVal(m_pSocketServer->Handle()));
  connInfo.SetPropertyL(m_Session.StringPool().StringF(HTTP::EHttpSocketConnection, RHTTPSession::GetTable()), 
                        THTTPHdrVal(reinterpret_cast<TInt>(m_pConnection)));
}

void CRoadMapNativeHTTP::SetModifiedSinceL()
{
  if (m_tModifiedSince == 0) return;
  
  RStringPool strP = m_Session.StringPool();
  RHTTPHeaders hdrs = m_Session.RequestSessionHeadersL(); //m_Transaction.Request().GetHeaderCollection();
  struct tm *tmVals = gmtime (&m_tModifiedSince);
  THTTPHdrVal dateVal (TDateTime (1900 + tmVals->tm_year, TMonth(TInt(tmVals->tm_mon)), tmVals->tm_mday - 1,
				                  tmVals->tm_hour, tmVals->tm_min, tmVals->tm_sec, 0));
  
  hdrs.SetFieldL(strP.StringF(HTTP::EIfModifiedSince,RHTTPSession::GetTable()), dateVal);
}


inline TInt CRoadMapNativeHTTP::ConnStart( TConnPref &aConnPrefs )
{
   return ConnAsyncStart( aConnPrefs );
}


TInt CRoadMapNativeHTTP::ConnSyncStart( TConnPref &aConnPrefs )
{
  TInt err = KErrNone;
  m_eConnStatus = EConnStatusNotStarted;
  err = m_pConnection->Start( aConnPrefs );
  if ( err == KErrNone )
  {
	  UpdateIAPid();
	  OpenSession();
	  ConnectWithParamsL();	  
  }
  else
  {
	  roadmap_log( ROADMAP_ERROR, "Error connection to the AP: %d", err );
  }
  
  return err;
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
    roadmap_log(ROADMAP_DEBUG, "CRoadMapNativeHTTP::RunL status=%d connStatus=%d\n", iStatus.Int() , (int)m_eConnStatus);
    switch( iStatus.Int() ) {
    case KErrNone:
        switch ( m_eConnStatus ) {
        case EConnStatusNotStarted:
            UpdateIAPid();
            OpenSession();
            ConnectWithParamsL();
            break;
        case EConnStatusConnected: 
            //  this should not happen...
            break;
        default:
            break;
        }
        return;
    case KErrCancel:
    	/* AGA NOTE: In touch screen it's not intuitive. Tap out of the dialog
    	 * causes this event. It loks like application crash. For this case we 
    	 * just show the connection screen
    	 */ 
#ifdef TOUCH_SCREEN
    	m_pConnection->Stop();
    	m_pConnection->Start(iStatus);
    	SetActive();
#else
    	roadmap_net_mon_offline();
        roadmap_main_exit();     
#endif        
        return;
    case KErrNotFound:
    case -30180: /* TBD: and everething between? */
    case -30170:
    // Exceptional cases. The errors returned in case of access point connection problems
    // In this case let the user to select the access point
    // last chosen AP has to be replaced
        SetChosenAP( 0 );
        StartL();
        return;

    case DATA_EVENT_AVAILABLE_STATUS: /* our internal status code */
        IssueCallback();
        return;

    case SELF_DESTRUCT_STATUS:        /* our internal status code */
        // A special one...
        delete this;
        return;
    default:
        //TODO  keep error code
        m_eConnStatus = EConnStatusError;  
    }
}

void CRoadMapNativeHTTP::DoCancel()
{
  m_pConnection->Close();
  Cancel();
}

PostDataSupplier::PostDataSupplier( void )
    : m_Data(NULL), m_finalized( ETrue ), m_overAllSize(0), m_Transaction()
{
}

PostDataSupplier::~PostDataSupplier( void )
{
    delete m_Data;
}

TBool PostDataSupplier::GetNextDataPart(TPtrC8& aDataPart)
{
    //mk TODO: add upstream transfer encoding support
    aDataPart.Set( m_Data->Ptr(0) );
  
    // Is the last POST data segment?
    return (aDataPart.Size() == m_Data->Size());
}

void PostDataSupplier::ReleaseData()
{
    // Deleting the first segment stored in the buffer
    size_t bytesToRelese = m_Data->Ptr(0).Size();
    m_Data->Delete( 0, bytesToRelese );
    
    // Count bytes sent
    roadmap_net_mon_send( bytesToRelese );

    // Notify if we still have more data
    if( m_Data->Size() > 0 ) {
        TRAPD( ret, m_Transaction.NotifyNewRequestBodyPartL() );
        (void)ret;
    }
}

TInt PostDataSupplier::OverallDataSize()
{
    m_finalized = ETrue; 
    return m_overAllSize; 
}

TInt PostDataSupplier::Reset()
{
    return KErrNotSupported;
}

void PostDataSupplier::AddDataL( const TDesC8& aDes )
{
    //  Sanity check: has the "overall size" been returned already?
    if( m_finalized ) {
        User::Leave( KErrGeneral );
    }
    
    m_Data->InsertL( m_Data->Size(), aDes );
    m_overAllSize += aDes.Size();
}

void PostDataSupplier::InitL( RHTTPTransaction& transaction )
{
    // Lazy init
    if( m_Data == NULL ) {
        m_Data = CBufSeg::NewL(128);
    }
    
    m_Data->Reset();
    m_finalized   = EFalse;
    m_overAllSize = 0;
    m_Transaction = transaction;
}
