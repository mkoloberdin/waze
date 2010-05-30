/* roadmap_location.m - iPhone location services
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi R.
 *   Copyright 2008 Ehud Shabtai
 *   Copyright 2009, Waze Ltd
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
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
 *   See roadmap_location.h
 */

#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_location.h";
#include "roadmap_iphonelocation.h";
#include "roadmap_state.h";
#include "roadmap_types.h";
#include "roadmap_trip.h";
#include "roadmap_screen.h";
#include "roadmap_math.h";
#include "roadmap_messagebox.h";
#include "roadmap_main.h";

#ifdef FLURRY
#import "FlurryAPI.h"
#endif //FLURRY


enum { LOCATION_MODE_OFF = 0,
       LOCATION_MODE_ON
};

static CLLocationManager *RoadMapLocationManager   = NULL;
static int RoadMapLocationNavigating               = 0;
static int RoadMapLoactionActive                   = 0;
static int RoadMapLocationServiceDenied            = 0;
static int RoadMapLocationSpeed                    = 0;
static int RoadMapLocationSteering                 = 0;

#define MAX_HOR_ACCURACY   1000 //was 80
#define MAX_VER_ACCURACY   1000 //was 120
#define MIN_DISTANCE       15 //meters
#define SPEED_STEP         5  //aprox 10Kph
#define UNIT_MPS_TO_KNOTS  1.9438
#define LOCATION_TIMEOUT   7000 //7 seconds timeout

static RoadMapGpsdNavigation RoadmapLocationNavigationListener = NULL;
static RoadMapGpsdDilution   RoadmapLocationDilutionListener   = NULL;
static RoadMapGpsdSatellite RoadmapLocationSatelliteListener  = NULL;




static void roadmap_location_start (void){
   [RoadMapLocationManager startUpdatingLocation];
}

static void roadmap_location_stop (void){
   [RoadMapLocationManager stopUpdatingLocation];
}

static void roadmap_location_initialize_internal (void){
   roadmap_main_remove_periodic(roadmap_location_initialize_internal);
   
   [[RoadMapLocationController alloc] init];
   roadmap_location_start();
}

void roadmap_location_initialize (void){
   roadmap_main_set_periodic(1000, roadmap_location_initialize_internal);
}

static void report_fix_loss() {
   RoadmapLocationNavigationListener ('V', ROADMAP_NO_VALID_DATA, ROADMAP_NO_VALID_DATA, 
                                      ROADMAP_NO_VALID_DATA, ROADMAP_NO_VALID_DATA,
                                      ROADMAP_NO_VALID_DATA, ROADMAP_NO_VALID_DATA);
}

void location_timeout (void) {
   if (RoadMapLocationNavigating && RoadMapLoactionActive) {
      report_fix_loss();
      RoadMapLoactionActive = 0;
   }
}

void roadmap_location_subscribe_to_navigation (RoadMapGpsdNavigation navigation) {

   if (RoadMapLocationNavigating == 0) {
      RoadMapLocationNavigating = 1;
      roadmap_main_set_periodic (LOCATION_TIMEOUT, location_timeout);
   }
   RoadmapLocationNavigationListener = navigation;
}


void roadmap_location_subscribe_to_dilution (RoadMapGpsdDilution dilution) {

   RoadmapLocationDilutionListener = dilution;
}

void roadmap_location_subscribe_to_satellites (RoadMapGpsdSatellite satellite) {

   RoadmapLocationSatelliteListener = satellite;
}

int roadmap_location_denied () {
   return RoadMapLocationServiceDenied;
}




@implementation RoadMapLocationController

@synthesize locationManager;


- (id) init {
    self = [super init];
    if (self != nil) {
        self.locationManager = [[CLLocationManager alloc] init];
        self.locationManager.delegate = self;
        self.locationManager.distanceFilter = kCLDistanceFilterNone;
        self.locationManager.desiredAccuracy = kCLLocationAccuracyBest;
        
        RoadMapLocationManager = self.locationManager;
    }
   
    return self;
}

- (void)locationManager:(CLLocationManager *)manager
    didUpdateToLocation:(CLLocation *)newLocation
    fromLocation:(CLLocation *)oldLocation {

   int    dimension_of_fix = 3;
   double pdop             = 0.0;
   double hdop             = 0.0;
   double vdop             = 0.0;
   
   char status = 'A';
   int gmt_time;
   int altitude;
   int speed;
   int steering;
   
   int i;
   NSTimeInterval IntervalSinceOld;
   NSTimeInterval interval;
   static NSTimeInterval LastUpdateInterval = -30.0f;
   
   static BOOL first_time = TRUE;
   
   
   RoadMapPosition position;
   int latitude;
   int longitude;

      
   
   interval = [newLocation.timestamp timeIntervalSinceNow];
   
   //Check age of the new location and location validity
   if ((interval - LastUpdateInterval < -5.0f) || newLocation.horizontalAccuracy < 0) {
      roadmap_log(ROADMAP_DEBUG,
                  "location invalid , interval: %lf ; hor acc: %lf",
                  [newLocation.timestamp timeIntervalSinceNow], newLocation.horizontalAccuracy);
      LastUpdateInterval = interval;
      return;
   }
   
   LastUpdateInterval = interval;
   
#ifdef FLURRY
   if (first_time) {
      [FlurryAPI setLocation:newLocation];
      first_time = FALSE;
   }
#endif //FLURRY
   
   //GPS handling
   if (RoadMapLocationNavigating) {
      roadmap_main_remove_periodic (location_timeout); //this is safe even if not set before
      roadmap_main_set_periodic (LOCATION_TIMEOUT, location_timeout);
      RoadMapLoactionActive = 1;
      
      //Do we have valid GPS data?
      if (newLocation.verticalAccuracy >= 0 &&
            newLocation.horizontalAccuracy <= MAX_HOR_ACCURACY &&
            newLocation.verticalAccuracy <= MAX_VER_ACCURACY) {
         //Dilution data
         if (newLocation.horizontalAccuracy <= 20) {
            hdop = 1;
         } else if (newLocation.horizontalAccuracy <= 50) {
            hdop = 2;
         } else if (newLocation.horizontalAccuracy <= 100) {
            hdop = 3;
         } else {
            hdop = 4;
         }
         RoadmapLocationDilutionListener (dimension_of_fix, pdop, hdop, vdop);
         
         //Sattelite data
         for (i = 1; i <= 3; i++) {
               RoadmapLocationSatelliteListener (i, 0, 0, 0, 0, 1);
         }
         if (hdop < 3) {
            RoadmapLocationSatelliteListener (4, 0, 0, 0, 0, 1);
         }
         RoadmapLocationSatelliteListener (0, 0, 0, 0, 0, 0);
         
         //Navigation data
         gmt_time = [newLocation.timestamp timeIntervalSince1970];
         
         position.latitude = floor (newLocation.coordinate.latitude* 1000000);
         position.longitude = floor (newLocation.coordinate.longitude* 1000000);
         
         IntervalSinceOld = [newLocation.timestamp 
                              timeIntervalSinceDate:oldLocation.timestamp];
                     
            
            if (newLocation.speed >= 0)
               RoadMapLocationSpeed = newLocation.speed * UNIT_MPS_TO_KNOTS;
         
            if (newLocation.course >= 0)
               RoadMapLocationSteering = newLocation.course;
                    
            
         speed = RoadMapLocationSpeed;
         steering = RoadMapLocationSteering;

         altitude = floor (newLocation.altitude);
         
         RoadmapLocationNavigationListener (status, gmt_time, position.latitude, 
                                             position.longitude, altitude, speed, steering);

         return;
      } else { //No valid GPS data
         roadmap_log(ROADMAP_DEBUG, "reporting fix loss");
         report_fix_loss();
      }
   }

   longitude = floor (newLocation.coordinate.longitude* 1000000);
   latitude = floor (newLocation.coordinate.latitude* 1000000);
  
   
#ifdef DEBUG
   longitude = 34878593;
   latitude = 32196208;
    //longitude = -78549870;
  //  latitude = -259970;
#endif
    
  
   roadmap_gps_coarse_fix(latitude, longitude);
}


- (void)locationManager:(CLLocationManager *)manager
       didFailWithError:(NSError *)error {
       
   if ([error code] == kCLErrorLocationUnknown) {
      roadmap_log (ROADMAP_WARNING, "Could not find location\n");
      if (RoadMapLocationNavigating) {
         report_fix_loss();
      RoadMapLoactionActive = 0;
      }
   } else if ([error code] == kCLErrorDenied) {
      roadmap_log (ROADMAP_WARNING, "Location Services denied\n");
      RoadMapLocationServiceDenied = 1;
      roadmap_location_stop();
      roadmap_messagebox ("Error", "No access to Location Services. Please make sure Location Services is ON");
   }
}

@end
