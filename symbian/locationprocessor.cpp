/* LocationProcessor.cpp - Symbian GPS implementation for Roadmap
 * 
 * LICENSE:
 * Copyright 2008 Giant Steps Ltd
 * Copyright 2008 Ehud Shabtai
 * Based on WhereAmI code Copyright (C) 2006-2007 Adam Boardman
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License V2 as published by
 * the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>./
 */

#ifdef GPS_BLUETOOTH
#include <bt_sock.h>
#include <btdevice.h>
#include <bttypes.h>
#include <btextnotifiers.h>
#include "btattribparser.h"
#else
#include <lbs.h>
#endif
#ifdef PLATFORM_S60
#include <aknnotewrappers.h>
#endif
//#include <math.h>
#include <e32std.h>

#include "locationprocessor.h"

#ifdef GPS_BLUETOOTH
const TInt KUuidSerialPort=0x1101;
const TInt KMajorServicePositioning = 0x08;
_LIT(KStrRFCOMM,"RFCOMM");
#else
const TInt KSecond = 1000000;
_LIT(KRequestor,"Roadmap");
#endif

#define FLOAT_MULTIPLIER  1000000

CLocationProcessor::CLocationProcessor()
  : CActive(CActive::EPriorityLow)
{
  m_pNavCallback = NULL;
  m_pSatCallback = NULL;
  m_pDilCallback = NULL;
  Mem::FillZ(&iPosData, sizeof(RoadMapGpsPosition));
  m_LastUpdatedTime = 0;
}

CLocationProcessor* CLocationProcessor::NewL()
{
  CLocationProcessor* self=new(ELeave) CLocationProcessor();
  CleanupStack::PushL(self);
  self->ConstructL();
  CleanupStack::Pop();
  return self;
}

int CLocationProcessor::GetAssistedModule(TPositionModuleInfo& aModuleInfo)
{
  TUint numModules = 0;
  TInt assistedModule = -1; 
  iPositionServer.GetNumModules(numModules);
  for ( TUint i = 0; i < numModules ; i++ )
  {
    iPositionServer.GetModuleInfoByIndex(i, aModuleInfo);
    if ( aModuleInfo.TechnologyType() & TPositionModuleInfo::ETechnologyAssisted )
    {
      assistedModule = i;
      break;
    }
  }

  if (!aModuleInfo.IsAvailable())
      return -1;
  else
     return assistedModule;
}

void CLocationProcessor::ConstructL()
{
  LOG("CLocationProcessor::ConstructL");
#ifndef GPS_BLUETOOTH
  User::LeaveIfError(iPositionServer.Connect());
  TPositionModuleInfo moduleInfo;
  if ( GetAssistedModule(moduleInfo) < 0 )
  {
    User::LeaveIfError(iPositioner.Open(iPositionServer));
  }
  else
  {
    User::LeaveIfError(iPositioner.Open(iPositionServer, moduleInfo.ModuleId()));
  }
  User::LeaveIfError(iPositioner.SetRequestor(CRequestor::ERequestorService, CRequestor::EFormatApplication, KRequestor));
  iUpdateops.SetUpdateInterval(TTimeIntervalMicroSeconds(KSecond));
  iUpdateops.SetAcceptPartialUpdates(ETrue);
  User::LeaveIfError(iPositioner.SetUpdateOptions(iUpdateops));
#endif
  CActiveScheduler::Add(this);
}

CLocationProcessor::~CLocationProcessor()
{
  LOG("CLocationProcessor::~CLocationProcessor");
  Cancel();
#ifdef GPS_BLUETOOTH
  delete iMatchList;
  delete iSdpSearchPattern;
  delete iSdpAgent;
  delete iNmeaParser;
#else
  iPositioner.Close();
  iPositionServer.Close();
#endif
}

void CLocationProcessor::DoCancel()
{
  LOG("CLocationProcessor::DoCancel");
  switch (iLocationProcessorState)
  {
#ifdef GPS_BLUETOOTH
  case ELocationProcessorStateRecievedBuffer:
  case ELocationProcessorStateConnectSocket:
    iRecievingSocket.CancelAll();
    iSocketServer.Close();
    iLocationProcessorState = ELocationProcessorStateCancelled;
    break;
  case ELocationProcessorStateSelectDevice:
    iNotifier.CancelNotifier(KDeviceSelectionNotifierUid);
    iNotifier.Close();
    iLocationProcessorState = ELocationProcessorStateCancelled;
    break;
#else
  case ELocationProcessorStateRequestedLastKnown:
    iPositioner.CancelRequest(EPositionerGetLastKnownPosition);
    iLocationProcessorState = ELocationProcessorStateCancelled;
    break;
  case ELocationProcessorStateRequestedUpdate:
    iPositioner.CancelRequest(EPositionerNotifyPositionUpdate);
    iLocationProcessorState = ELocationProcessorStateCancelled;
    break;
#endif
  default:
    iLocationProcessorState = ELocationProcessorStateNone;
    break;
  };
}

TBool CLocationProcessor::ConnectToDeviceL()
{
  LOG("CLocationProcessor::ConnectToDeviceL");
#ifdef GPS_BLUETOOTH
  if ((iLocationProcessorState == ELocationProcessorStateNone) && !IsActive())
  {
#ifdef PLATFORM_UIQ_V3
    //The RNotifier method causes the UIQ hardware to reboot! (unless you only want to search for phones) so use specific UIQ dialog
    CBTDeviceArray* selectedDeviceArray = new (ELeave)CBTDeviceArray(1);
    BTDeviceArrayCleanupStack::PushL(selectedDeviceArray);
    TInt majorFilter=KQBTDeviceFilterAll;
    TInt minorFilter=KQBTDeviceFilterAll;
    TInt serviceFilter=KQBTDeviceFilterAll;
    LOG("CLocationProcessor::ConnectToDeviceL - construct btui");
    CQBTUISelectDialog* dialog = CQBTUISelectDialog::NewL(selectedDeviceArray, majorFilter, minorFilter, serviceFilter);
    LOG("CLocationProcessor::ConnectToDeviceL - launch single select");
    TInt dialogret = dialog->LaunchSingleSelectDialogLD();
    LOG1("CLocationProcessor::ConnectToDeviceL - ret: %d",dialogret);
    if (dialogret)
    {
      LOG("CLocationProcessor::ConnectToDeviceL - sdpagent");
      delete iSdpSearchPattern;
      iSdpSearchPattern = NULL;
      delete iSdpAgent;
      iSdpAgent = NULL;

      iSdpSearchPattern = CSdpSearchPattern::NewL();
      TUUID serviceClass(KUuidSerialPort);
      iSdpSearchPattern->AddL(serviceClass);
      iDeviceFoundPckg().SetDeviceAddress(selectedDeviceArray->At(0)->BDAddr());
      iSdpAgent = CSdpAgent::NewL(*this, selectedDeviceArray->At(0)->BDAddr());
      LOG("CLocationProcessor::ConnectToDeviceL - set record filter");
      iSdpAgent->SetRecordFilterL(*iSdpSearchPattern);
      LOG("CLocationProcessor::ConnectToDeviceL - next record request");
      iSdpAgent->NextRecordRequestL();
      return ETrue;
    }
    else
    {
      LOG("CLocationProcessor::ConnectToDeviceL - user not pick");
    }
    LOG("CLocationProcessor::ConnectToDeviceL - pop and destroy");
    CleanupStack::PopAndDestroy(selectedDeviceArray);

#else
    TBTDeviceSelectionParams parameters;
    TUUID serviceClass(KUuidSerialPort);
    parameters.SetUUID(serviceClass);
#ifdef PLATFORM_S60_V2_FP1
    TBTMajorServiceClass majorServiceClass = EMajorServicePositioning;
    TBTDeviceClass deviceClass((TUint16)majorServiceClass, 0, 0);
    parameters.SetDeviceClass(deviceClass);
#endif
#ifdef PLATFORM_S80
    TBTMajorServiceClass majorServiceClass = STATIC_CAST(TBTMajorServiceClass,KMajorServicePositioning);
    TBTDeviceClass deviceClass((TUint16)majorServiceClass, 0, 0);
    parameters.SetDeviceClass(deviceClass);
#endif
    TBTDeviceSelectionParamsPckg pckg(parameters);
    User::LeaveIfError(iNotifier.Connect());
    iNotifier.StartNotifierAndGetResponse(iStatus,KDeviceSelectionNotifierUid,pckg,iDeviceFoundPckg);

    iLocationProcessorState = ELocationProcessorStateSelectDevice;
    iValidPort = EFalse;
    SetActive();
    return ETrue;
#endif		
  }
#else
  if ((iLocationProcessorState == ELocationProcessorStateNone) && !IsActive())
  {
#ifdef PLATFORM_S60
    CAknInformationNote *note = new (ELeave) CAknInformationNote();
    note->ExecuteLD(_L("Connecting..."));
#else
    User::InfoPrint(_L("Connecting..."));
#endif
    RequestLocation();
    return ETrue;
  }
#endif
  return EFalse;
}

void CLocationProcessor::RequestLocation()
{
  LOG("CLocationProcessor::RequestLocation");
#ifndef GPS_BLUETOOTH
  if (iPosInfoBase)
  {
    TPositionModuleInfo moduleInfo; 
    TPositionModuleId usedPsy = iPosInfoBase->ModuleId();
    iPositionServer.GetModuleInfoById(usedPsy,moduleInfo);
      
    TInt32 moduleInfoFamily = moduleInfo.ClassesSupported(EPositionInfoFamily);
    if (moduleInfoFamily&EPositionSatelliteInfoClass)
    {
      iPosInfoBase = &iSatelliteInfo;	
    }
    else if (moduleInfoFamily&EPositionCourseInfoClass)
    {
      iPosInfoBase = &iCourseInfo;	
    }
    else
    {
      iPosInfoBase = &iPosInfo;	
    }
  }
  else
  {
    iPosInfoBase = &iPosInfo;	
  }
  iPositioner.NotifyPositionUpdate(*iPosInfoBase, iStatus);
  iLocationProcessorState = ELocationProcessorStateRequestedUpdate;
#endif
  SetActive();
}

void CLocationProcessor::DisconnectL(TBool /*aRemoteDisconnection*/)
{
  LOG("CLocationProcessor::Disconnect");
  Cancel();
  iLocationProcessorState = ELocationProcessorStateNone;
  CallNavigationCallback(false);
  //@@	iLocationReciever.DisconnectedL(aRemoteDisconnection);
}

void CLocationProcessor::RunL()
{
  LOG2("CLocationProcessor::RunL - status: %d, state: %d",iStatus.Int(),iLocationProcessorState);
  switch (iStatus.Int())
  {
  case KErrNone:
    break;
#ifdef GPS_BLUETOOTH
  case KErrCancel:
    break;
#else
  case KPositionPartialUpdate:
    iLocationProcessorState = ELocationProcessorStatePartialUpdate;
    break;
  case KPositionQualityLoss:
    iLocationProcessorState = ELocationProcessorStateQualityLoss;
    break;
#endif
  case KErrDisconnected:
  {
    LOG("CLocationProcessor::RunL - GPS Disconnected");
#ifdef PLATFORM_S60
    CAknWarningNote *note = new (ELeave) CAknWarningNote();
    note->ExecuteLD(_L("GPS Disconnected"));
#else
    User::InfoPrint(_L("GPS Disconnected"));
#endif
    iLocationProcessorState = ELocationProcessorStateNone;
    DisconnectL(ETrue);
  }
  return;
  case KErrAccessDenied:
  {
#ifdef PLATFORM_S60
    CAknWarningNote *note = new (ELeave) CAknWarningNote();
    note->ExecuteLD(_L("Permission Denied"));
#else
    User::InfoPrint(_L("Permission Denied"));
#endif
    iLocationProcessorState = ELocationProcessorStateNone;
    DisconnectL(EFalse);
  }
  return;
  case KErrNotSupported:
  {
#ifdef PLATFORM_S60
    CAknWarningNote *note = new (ELeave) CAknWarningNote();
    note->ExecuteLD(_L("Not Supported"));
#else
    User::InfoPrint(_L("Not Supported"));
#endif
    iLocationProcessorState = ELocationProcessorStateNone;
    DisconnectL(EFalse);
  }
  return;
  default:
    LOG2("CLocationProcessor::RunL - iStatus: %d, iLocationProcessorState: %d",iStatus.Int(),iLocationProcessorState);
    iLocationProcessorState = ELocationProcessorStateNone;
    DisconnectL(EFalse);
    return;
  }

  switch (iLocationProcessorState)
  {
#ifdef GPS_BLUETOOTH
  case ELocationProcessorStateSelectDevice:
    iNotifier.CancelNotifier(KDeviceSelectionNotifierUid);
    iNotifier.Close();
    if (!(iDeviceFoundPckg().IsValidDeviceName()))
    {
      DisconnectL(EFalse);
    }
    else
    {
      delete iSdpSearchPattern;
      iSdpSearchPattern = NULL;
      delete iSdpAgent;
      iSdpAgent = NULL;

      iSdpSearchPattern = CSdpSearchPattern::NewL();
      TUUID serviceClass(KUuidSerialPort);
      iSdpSearchPattern->AddL(serviceClass); 
      iSdpAgent = CSdpAgent::NewL(*this, iDeviceFoundPckg().BDAddr());
      iSdpAgent->SetRecordFilterL(*iSdpSearchPattern);
      iSdpAgent->NextRecordRequestL();
    }
    break;
  case ELocationProcessorStateConnectSocket:
  {
#ifdef PLATFORM_S60
    CAknConfirmationNote *note = new (ELeave) CAknConfirmationNote();
    note->ExecuteLD(_L("Connected"));
#else
    User::InfoPrint(_L("Connected"));
#endif
  }
  case ELocationProcessorStateRecievedBuffer:
  {
    if (iBuffer.Length())
    {
      iLocationReciever.RawLoggerL(iBuffer);
      if (!iNmeaParser)
      {
        iNmeaParser = new(ELeave)CNmeaParser(iLocationReciever);
      }
      TRAPD(error,iNmeaParser->ParseNmeaL(iBuffer));
      LOG1("CLocationProcessor::RunL - parse error: %d",error);
    }
    iBuffer.Zero();
    LOG("CLocationProcessor::RunL - RecvOneOrMore");
    iRecievingSocket.RecvOneOrMore(iBuffer, 0, iStatus, iLen);
    iLocationProcessorState = ELocationProcessorStateRecievedBuffer;
    LOG("CLocationProcessor::RunL - SetActive");
    SetActive();
  }
  break;
#else
  case ELocationProcessorStatePartialUpdate:
  case ELocationProcessorStateRequestedUpdate:
  {
    LOG("CWhereAmIController::RunL - EStateSelectDevice");
    if (iPosInfoBase == &iSatelliteInfo)
      StoreSatelliteAndCourseAndPositionL(iSatelliteInfo);
    else if (iPosInfoBase == &iCourseInfo)
      StoreCourseAndPositionL(iCourseInfo);
    else
      StorePositionL(iPosInfo);
    RequestLocation();
  }
  break;
  case ELocationProcessorStateQualityLoss:
    CallNavigationCallback(false);  //  update on faulty status 
    RequestLocation();
    break;
#endif
#ifdef GPS_BLUETOOTH
  case ELocationProcessorStateNextRecord:
    iLocationProcessorState = ELocationProcessorStateSelectDevice;
    iSdpAgent->NextRecordRequestL();
    SetActive();
    break;
#endif
  case ELocationProcessorStateDisconnect:
    iLocationProcessorState = ELocationProcessorStateNone;
    DisconnectL(EFalse);
    break;
  default:
    LOG1("CLocationProcessor::RunL - default - %d",iLocationProcessorState);
    iLocationProcessorState = ELocationProcessorStateNone;
    break;
  }
}

#ifdef GPS_BLUETOOTH

void CLocationProcessor::NextRecordRequestComplete(TInt aError, TSdpServRecordHandle aHandle, TInt aTotalRecordsCount)
{
  LOG2("CLocationProcessor::NextRecordRequestComplete - Error: %d, RecCount: %d",aError,aTotalRecordsCount);
  if (aError == KErrEof)
  {
  }
  else if ((aError != KErrNone) || (aTotalRecordsCount == 0))
  {
    iLocationProcessorState = ELocationProcessorStateDisconnect;
    iStatus=KRequestPending;
    SetActive();
    TRequestStatus* status=&iStatus;
    User::RequestComplete(status,0);
  }
  else
  {
    delete iMatchList;
    iMatchList = NULL;
    TRAPD(error, 
        iMatchList = CSdpAttrIdMatchList::NewL();
    iMatchList->AddL(KSdpAttrIdProtocolDescriptorList);
    iSdpAgent->AttributeRequestL(aHandle, *iMatchList);
    );
    if (error != KErrNone)
    {
      LOG1("CLocationProcessor::NextRecordRequestComplete - SdpAgent Error: %d",error);
      iLocationProcessorState = ELocationProcessorStateDisconnect;
      iStatus=KRequestPending;
      SetActive();
      TRequestStatus* status=&iStatus;
      User::RequestComplete(status,0);
    }
  }
}

void CLocationProcessor::AttributeRequestResult(TSdpServRecordHandle aHandle, TSdpAttributeID aAttrID, CSdpAttrValue* aAttrValue)
{
  TRAPD(err,AttributeRequestResultL(aHandle, aAttrID, aAttrValue));
  if (err!=KErrNone)
    Panic(EWhereAmIPanicsAttributeRequestResultFailed);
}

void CLocationProcessor::AttributeRequestResultL(TSdpServRecordHandle /*aHandle*/, TSdpAttributeID aAttrID, CSdpAttrValue* aAttrValue)
{
  LOG1("CLocationProcessor::AttributeRequestResultL - attrId: %d",aAttrID);
  CleanupStack::PushL(aAttrValue);

  if (aAttrID == KSdpAttrIdProtocolDescriptorList)
  {
    TBtAttribParser attribParser(iPort, iValidPort);
    aAttrValue->AcceptVisitorL(attribParser);
  }
  CleanupStack::PopAndDestroy(aAttrValue);
}

void CLocationProcessor::AttributeRequestComplete(TSdpServRecordHandle /*aHandle*/, TInt aError)
{
  LOG2("CLocationProcessor::AttributeRequestComplete - avail: %d, valid: %d", iAvailable, iValidPort);
  if (iValidPort)
  {
    TInt err=iSocketServer.Connect();
    if (err != KErrNone)
      Panic(EWhereAmIPanicsSocketServerConnectFailed);
    err=iRecievingSocket.Open(iSocketServer, KStrRFCOMM);
    if (err != KErrNone)
      Panic(EWhereAmIPanicsRecievingSocketOpenFailed);
    TBTSockAddr address;
    address.SetBTAddr(iDeviceFoundPckg().BDAddr());
    address.SetPort(iPort);
    iRecievingSocket.Connect(address, iStatus);
    iLocationProcessorState = ELocationProcessorStateConnectSocket;
    SetActive();
  }
  else
  {
    if (aError == KErrNone)
    {
      iLocationProcessorState = ELocationProcessorStateNextRecord;
    }
    else
    {
      iLocationProcessorState = ELocationProcessorStateDisconnect;
    }
    iStatus=KRequestPending;
    SetActive();
    TRequestStatus* status=&iStatus;
    User::RequestComplete(status,0);
  }
}

#else // not defined GPS_BLUETOOTH

void CLocationProcessor::StoreSatelliteAndCourseAndPositionL(TPositionSatelliteInfo aPosInfo)
{
  LOG("CLocationProcessor::StoreSatelliteAndCourseAndPositionL");
	for (TUint satCount=0; satCount<KPositionMaxSatellitesInView; satCount++)
	{
		TSatelliteData lbsSatelliteData;
		aPosInfo.GetSatelliteData(satCount, lbsSatelliteData);

		RoadMapGpsSatellite satelliteData;

		satelliteData.id = satCount;
// we do not need to set		  satelliteData.status here...;
		satelliteData.azimuth = STATIC_CAST(TInt,lbsSatelliteData.Azimuth());
		satelliteData.elevation = STATIC_CAST(TInt,lbsSatelliteData.Elevation());
		satelliteData.strength = lbsSatelliteData.SignalStrength();
//TODO	call callback for satellite data	iLocationReciever.Satellite(satelliteData);
	}

  StoreCourseAndPositionL(aPosInfo,ETrue);
}

void CLocationProcessor::StoreCourseAndPositionL(TPositionCourseInfo aPosInfo, TBool aFixAtSet)
{
  LOG("CLocationProcessor::StoreCourseAndPositionL");

  TCourse course;
  aPosInfo.GetCourse(course);
  iPosData.speed = course.Speed()/0.5144444; //knots to m/s
  iPosData.steering = course.Heading();//or Course()

  StorePositionL(aPosInfo,aFixAtSet);
}

void CLocationProcessor::StorePositionL(TPositionInfo aPosInfo, TBool /*aFixAtSet*/)
{
  LOG("CLocationProcessor::StorePositionL");

  TPosition position;
  aPosInfo.GetPosition(position);
  iPosData.latitude = (int)(position.Latitude()*FLOAT_MULTIPLIER);
  iPosData.longitude = (int)(position.Longitude()*FLOAT_MULTIPLIER);
  iPosData.altitude = (int)(position.Altitude());
  
  TTime utcTime = position.Time();
  
  TDateTime baseTime;
  baseTime.Set(1970,EJanuary,0,0,0,0,0);
  
  TTimeIntervalSeconds intervalSeconds;
  if ( utcTime.SecondsFrom(baseTime, intervalSeconds) != KErrNone )
  {
     m_LastUpdatedTime = 0;
  }
  m_LastUpdatedTime = intervalSeconds.Int();

  // Get module status
  TPositionModuleStatus modStatus;
  User::LeaveIfError(iPositionServer.GetModuleStatus(modStatus, aPosInfo.ModuleId()));

  // Use the status
  //TPositionModuleStatus::TDeviceStatus deviceStatus = modStatus.DeviceStatus();
  TPositionModuleStatus::TDataQualityStatus qualityStatus = modStatus.DataQualityStatus();
  
  //status is ok, since we got here, so call with 'true'
  CallNavigationCallback(qualityStatus == TPositionModuleStatus::EDataQualityNormal);
}


void CLocationProcessor::SetNavigationCallback(RoadMapGpsdNavigation aNavCallback)
{
  m_pNavCallback = aNavCallback;
}

void CLocationProcessor::CallNavigationCallback(bool aStatus)
{
  if ( m_pNavCallback == NULL )
  {
    //roadmap_log(ROADMAP_ERROR,"Navigation callback not initialized!");
    return;
  }
  char status = 0;
  if ( aStatus == true )
  {
    status = 'A';
  }
  else
  {
    status = 'V';
  }
  
  roadmap_gps_raw(m_LastUpdatedTime, iPosData.longitude, iPosData.latitude,
        iPosData.steering, iPosData.speed);
  
  if (global_FreeMapLock() != 0) return;
  (*m_pNavCallback)(status, 
                    m_LastUpdatedTime,
                    iPosData.latitude,
                    iPosData.longitude,
                    iPosData.altitude,
                    iPosData.speed,
                    iPosData.steering);
  global_FreeMapUnlock();
}

#endif // GPS_BLUETOOTH
