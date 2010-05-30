/* LocationProcessor.h - Symbian GPS implementation for Roadmap
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

#ifndef __LOCATIONPROCESSOR_H
#define __LOCATIONPROCESSOR_H


//#define _MARK_USE_GPS_LOG
#ifndef _MARK_USE_GPS_LOG
#define LOG(x0)  
#define LOG1(x0,x1)  
#define LOG2(x0,x1,x2)  
#else
#define LOG(x0)         roadmap_log(ROADMAP_WARNING, x0)
#define LOG1(x0,x1)     roadmap_log(ROADMAP_WARNING, x0, x1)
#define LOG2(x0,x1,x2)  roadmap_log(ROADMAP_WARNING, x0, x1, x2)
#endif

extern "C"{
#include "roadmap_gps.h"
#include "roadmap.h"
}

#ifdef GPS_BLUETOOTH
#include <bttypes.h>
#include <btextnotifiers.h>
#include <btsdp.h>
#include <es_sock.h>
#else
#include <lbs.h>
#include <lbssatellite.h>
#endif


#ifdef GPS_BLUETOOTH
#include "nmeaparser.h"
#endif

class CLocationProcessor : public CActive
#ifdef GPS_BLUETOOTH
							, public MSdpAgentNotifier
#endif
	{
	public:
		CLocationProcessor();
		static CLocationProcessor* NewL();
		~CLocationProcessor();
		void ConstructL();

		int GetAssistedModule(TPositionModuleInfo& aModuleInfo); //TODO change to use specific supplied module id
		TBool ConnectToDeviceL();
		void RequestLocation();
		void DisconnectL(TBool aRemoteDisconnection);
		
		void SetNavigationCallback(RoadMapGpsdNavigation aNavCallback);
		void CallNavigationCallback(bool aStatus);
		
#ifdef GPS_BLUETOOTH
		void AttributeRequestResultL(TSdpServRecordHandle aHandle, TSdpAttributeID aAttrID, CSdpAttrValue* aAttrValue);
		void LogBufferToNmeaFileL();
#else
		void StoreSatelliteAndCourseAndPositionL(TPositionSatelliteInfo aPosInfo);
		void StoreCourseAndPositionL(TPositionCourseInfo aPosInfo, TBool aFixAtSet=EFalse);
		void StorePositionL(TPositionInfo aPosInfo, TBool aFixAtSet=EFalse);
#endif

	protected: // from CActive
		void DoCancel();
		void RunL();

#ifdef GPS_BLUETOOTH
	public: // from MSdpAgentNotifier
		void NextRecordRequestComplete(TInt aError, TSdpServRecordHandle aHandle, TInt aTotalRecordsCount);
		void AttributeRequestResult(TSdpServRecordHandle aHandle, TSdpAttributeID aAttrID, CSdpAttrValue* aAttrValue);
		void AttributeRequestComplete(TSdpServRecordHandle aHandle, TInt aError);
#endif

	private:
		enum TLocationProcessorState 
			{
			ELocationProcessorStateNone,
#ifdef GPS_BLUETOOTH
			ELocationProcessorStateSelectDevice,
			ELocationProcessorStateConnectSocket,
			ELocationProcessorStateRecievedBuffer,
#else
			ELocationProcessorStatePartialUpdate,
			ELocationProcessorStateQualityLoss,
			ELocationProcessorStateRequestedLastKnown,
			ELocationProcessorStateRequestedUpdate,
#endif
			ELocationProcessorStateCancelled,
#ifdef GPS_BLUETOOTH
			ELocationProcessorStateNextRecord,
#endif
			ELocationProcessorStateDisconnect
			} iLocationProcessorState;

#ifdef GPS_BLUETOOTH
		RSocket iRecievingSocket;
		RSocketServ iSocketServer;
		RNotifier iNotifier;
		TBTDeviceResponseParamsPckg iDeviceSelectionPckg;
		TBTDeviceResponseParamsPckg iDeviceFoundPckg;
		CSdpAgent* iSdpAgent;
		CSdpSearchPattern* iSdpSearchPattern;
		CSdpAttrIdMatchList* iMatchList;
		TUint iPort;
		TBool iValidPort;
		TBool iAvailable;
		TBuf8<256> iBuffer;
		TSockXfrLength iLen;
		CTimeOutTimer* iUpdateTimer;
		CNmeaParser* iNmeaParser;
#else
		RPositionServer iPositionServer;
		RPositioner iPositioner;
		TPositionUpdateOptions iUpdateops;
		TPositionInfo iPosInfo;
		TPositionCourseInfo iCourseInfo;
		TPositionSatelliteInfo iSatelliteInfo;
		TPositionInfoBase* iPosInfoBase;
#endif

		RoadMapGpsPosition iPosData;
		RoadMapGpsSatellite iSatelliteData;
		
		RoadMapGpsdNavigation m_pNavCallback;
		RoadMapGpsdSatellite m_pSatCallback;
		RoadMapGpsdDilution m_pDilCallback;
		
		int m_LastUpdatedTime;
	};

#endif //__LOCATIONPROCESSOR_H
