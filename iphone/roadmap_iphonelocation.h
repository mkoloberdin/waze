/* roadmap_location.h - iPhone location services
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi R.
 *   Copyright 2008 Ehud Shabtai
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
 */

#ifndef __ROADMAP_IPHONELOCATION__H
#define __ROADMAP_IPHONELOCATION__H

#import <CoreLocation/CoreLocation.h>


@interface RoadMapLocationController : NSObject <CLLocationManagerDelegate, UIAccelerometerDelegate> {
CLLocationManager *locationManager;
}

@property (nonatomic, retain) CLLocationManager *locationManager;


- (id)init;

- (void)locationManager:(CLLocationManager *)manager
    didUpdateToLocation:(CLLocation *)newLocation
           fromLocation:(CLLocation *)oldLocation;

- (void)locationManager:(CLLocationManager *)manager
       didFailWithError:(NSError *)error;
       
@end

#endif // __ROADMAP_IPHONELOCATION__H
