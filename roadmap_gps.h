/* roadmap_gps.h - GPS interface for the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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

#ifndef INCLUDE__ROADMAP_GPS__H
#define INCLUDE__ROADMAP_GPS__H

#include <time.h>
#include "roadmap_io.h"

#define INVALID_STEERING 0x1000

enum {GPS_RECEPTION_NA = 0,
      GPS_RECEPTION_NONE,
      GPS_RECEPTION_POOR,
      GPS_RECEPTION_GOOD
};


void roadmap_gps_initialize (void);


/* The listener is a function to be called each time a valid GPS coordinate
 * has been received. There can be more than one listener at a given time.
 */
typedef struct {

   int longitude;  /* TIGER format (decimal degrees multiplied by 1000000) */
   int latitude;
   int altitude;   /* Using the selected unit system (metric or imperial) */
   int speed;      /* knots */
   int steering;   /* Degrees */

} RoadMapGpsPosition;

typedef struct {

   int    dimension;        /* <2: none, 2: 2D fix, 3: 3D fix. */
   double dilution_position;
   double dilution_horizontal;
   double dilution_vertical;

} RoadMapGpsPrecision;

#define ROADMAP_GPS_NULL_POSITION {0, 0, 0, 0, 0}

typedef void (*roadmap_gps_listener)
                  (time_t gps_time,
                   const RoadMapGpsPrecision *dilution,
                   const RoadMapGpsPosition *position);

void roadmap_gps_register_listener (roadmap_gps_listener listener);
void roadmap_gps_unregister_listener(roadmap_gps_listener listener);

/* The monitor is a function to be called each time a valid GPS satellite
 * status has been received. There can be more than one monitor at a given
 * time.
 */
typedef struct {

   char  id;
   char  status;     /* 0: not detected, 'F': fixing, 'A': active. */
   char  elevation;
   char  reserved;
   short azimuth;
   short strength;

} RoadMapGpsSatellite;

typedef void (*roadmap_gps_monitor) (const RoadMapGpsPrecision *precision,
                                     const RoadMapGpsSatellite *satellites,
                                     int count);

void roadmap_gps_register_monitor (roadmap_gps_monitor monitor);



typedef void (*roadmap_fix_listener)
                  (int longitude,
                   int latitude);

void roadmap_gps_register_fix_listener (roadmap_fix_listener listener);
void roadmap_gps_unregister_fix_listener (roadmap_fix_listener listener);


/* The link and periodic control functions are hooks designed to let the GPS
 * link to be managed from within an application's GUI main loop.
 *
 * When data is detected from the GPS link, roadmap_gps_input() should be
 * called. If the GPS link is down, roadmap_gps_open() should be called
 * periodically.
 *
 * The functions below provide this module with a way for managing these
 * callbacks in behalf of the application.
 */
typedef void (*roadmap_gps_link_control) (RoadMapIO *io);
typedef void (*roadmap_gps_periodic_control) (RoadMapCallback handler);

void roadmap_gps_register_link_control
                 (roadmap_gps_link_control add,
                  roadmap_gps_link_control remove);

void roadmap_gps_register_periodic_control
                 (roadmap_gps_periodic_control add,
                  roadmap_gps_periodic_control remove);


/* The logger is a function to be called each time data has been
 * received from the GPS link (good or bad). Its should record the data
 * for later analysis or replay.
 */
typedef void (*roadmap_gps_logger)   (const char *sentence);

void roadmap_gps_register_logger (roadmap_gps_logger logger);


void roadmap_gps_open   (void);
void roadmap_gps_input  (RoadMapIO *io);
int  roadmap_gps_active (void);
const RoadMapPosition* roadmap_gps_get_fix (void);
BOOL roadmap_gps_have_reception(void);
int  roadmap_gps_estimated_error (void);
int  roadmap_gps_speed_accuracy  (void);

int  roadmap_gps_is_nmea (void);

void roadmap_gps_raw (time_t tm, int longitude, int latitude,
                      int steering, int speed);

void roadmap_gps_shutdown (void);
int roadmap_gps_reception_state (void);
void roadmap_gps_detect_receiver (void);
void roadmap_gps_csv_tracker_initialize(void);
void roadmap_gps_csv_tracker_set_enable( BOOL value );
BOOL roadmap_gps_csv_tracker_get_enable( void );
void roadmap_gps_csv_tracker_shutdown( void );
void roadmap_gps_coarse_fix( int latitude, int longitude, int accuracy, time_t time );
void roadmap_gps_detect_receiver_callback(RoadMapCallback callback);
int roadmap_gps_satelite_count(void);

time_t roadmap_gps_get_received_time(void);
void roadmap_gps_reset_show_coordinates(void);

/* Generic protocols */
#define ROADMAP_NO_VALID_DATA    -512000000

typedef void (*RoadMapGpsdNavigation) (char status,
                                       int gps_time,
                                       int latitude,
                                       int longitude,
                                       int altitude,
                                       int speed,
                                       int steering);

typedef void (*RoadMapGpsdSatellite)  (int sequence,
                                       int id,
                                       int elevation,
                                       int azimuth,
                                       int strength,
                                       int active);

typedef void (*RoadMapGpsdDilution )  (int dimension,
                                       double position,
                                       double horizontal,
                                       double vertical);


#endif // INCLUDE__ROADMAP_GPS__H

