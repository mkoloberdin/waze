/* roadmap_gpsd2.c - a module to interact with gpsd using its library.
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
 *
 *
 * DESCRIPTION:
 *
 *   This module hides the gpsd library API (version 2).
 */

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_gpsd2.h"


static RoadMapGpsdNavigation RoadmapGpsd2NavigationListener = NULL;
static RoadMapGpsdSatellite  RoadmapGpsd2SatelliteListener = NULL;
static RoadMapGpsdDilution   RoadmapGpsd2DilutionListener = NULL;


static int roadmap_gpsd2_decode_numeric (const char *input) {

   if (input[0] == '?') return ROADMAP_NO_VALID_DATA;

   while (*input == '0') ++input;
   if (*input == 0) return 0;

   return atoi(input);
}


static double roadmap_gpsd2_decode_float (const char *input) {

   if (input[0] == '?') return 0.0;

   return atof(input);
}


static int roadmap_gpsd2_decode_coordinate (const char *input) {

   char *point = strchr (input, '.');

   if (point != NULL) {

      /* This is a floating point value: patch the input to multiply
       * it by 1000000 and then make it an integer (TIGER format).
       */
      const char *from;

      int   i;
      char *to;
      char  modified[16];

      to = modified;

      /* Copy the integer part. */
      for (from = input; from < point; ++from) {
         *(to++) = *from;
      }

      /* Now copy the decimal part. */
      for (from = point + 1, i = 0; *from > 0 && i < 6; ++from, ++i) {
         *(to++) = *from;
      }
      while (i++ < 6) *(to++) = '0';
      *to = 0;

      return atoi(modified);
   }

   return roadmap_gpsd2_decode_numeric (input);
}


RoadMapSocket roadmap_gpsd2_connect (const char *name) {

   RoadMapSocket socket = roadmap_net_connect ("tcp", name, 0, 2947, NULL);

   if (ROADMAP_NET_IS_VALID(socket)) {

      /* Start watching what happens. */

      static const char request[] = "w+\n";

      if (roadmap_net_send
            (socket, request, sizeof(request)-1, 1) != sizeof(request)-1) {

         roadmap_log (ROADMAP_WARNING, "Lost gpsd server session");
         roadmap_net_close (socket);

         return ROADMAP_INVALID_SOCKET;
      }
   }

   return socket;
}


void roadmap_gpsd2_subscribe_to_navigation (RoadMapGpsdNavigation navigation) {

   RoadmapGpsd2NavigationListener = navigation;
}


void roadmap_gpsd2_subscribe_to_satellites (RoadMapGpsdSatellite satellite) {

   RoadmapGpsd2SatelliteListener = satellite;
}


void roadmap_gpsd2_subscribe_to_dilution (RoadMapGpsdDilution dilution) {

   RoadmapGpsd2DilutionListener = dilution;
}


int roadmap_gpsd2_decode (void *user_context,
                          void *decoder_context, char *sentence, int length) {

   int got_navigation_data;

   int got_o = 0;
   int got_q = 0;

   int status;
   int latitude;
   int longitude;
   int altitude;
   int speed;
   int steering;

   char *reply[256];
   int   reply_count;
   int   i;

   int   s;
   int   count;
   int   satellite_count;

   int    dimension_of_fix = -1;
   double pdop = 0.0;
   double hdop = 0.0;
   double vdop = 0.0;

   int  gps_time = 0;


   reply_count = roadmap_input_split (sentence, ',', reply, 256);

   if ((reply_count <= 1) || strcmp (reply[0], "GPSD")) {
      return 0;
   }

   /* default value (invalid value): */
   status = 'V';
   latitude  = ROADMAP_NO_VALID_DATA;
   longitude = ROADMAP_NO_VALID_DATA;
   altitude  = ROADMAP_NO_VALID_DATA;
   speed     = ROADMAP_NO_VALID_DATA;
   steering  = ROADMAP_NO_VALID_DATA;


   got_navigation_data = 0;

   for(i = 1; i < reply_count; ++i) {

      char *item = reply[i];
      char *value = item + 2;
      char *argument[32];
      char *tuple[5];


      if (item[1] != '=') {
         continue;
      }

      switch (item[0]) {

         case 'A':

            if (got_o) continue; /* Consider the 'O' answer only. */

            altitude = roadmap_gpsd2_decode_numeric (value);
            break;

         case 'M':

            dimension_of_fix = roadmap_gpsd2_decode_numeric(value);

            if (dimension_of_fix > 1) {
               status = 'A';
            } else {
               status = 'V';
               got_navigation_data = 1;
               goto end_of_decoding;
            }

            if (got_q) {

               RoadmapGpsd2DilutionListener
                  (dimension_of_fix, pdop, hdop, vdop);

               got_q = 0; /* We already "used" it. */
            }
            break;

         case 'O':

            if (roadmap_input_split (value, ' ', argument, 14) != 14) {
               continue;
            }

            got_o = 1;
            got_navigation_data = 1;

            gps_time  = roadmap_gpsd2_decode_numeric    (argument[1]);
            latitude  = roadmap_gpsd2_decode_coordinate (argument[3]);
            longitude = roadmap_gpsd2_decode_coordinate (argument[4]);
            altitude  = roadmap_gpsd2_decode_numeric    (argument[5]);
            steering  = roadmap_gpsd2_decode_numeric    (argument[8]);
            speed     = roadmap_gpsd2_decode_numeric    (argument[9]);

            break;

         case 'P':

            if (got_o) continue; /* Consider the 'O' answer only. */

            if (roadmap_input_split (value, ' ', argument, 2) != 2) {
               continue;
            }

            latitude = roadmap_gpsd2_decode_coordinate (argument[0]);
            longitude = roadmap_gpsd2_decode_coordinate (argument[1]);

            if ((longitude != ROADMAP_NO_VALID_DATA) &&
                  (latitude != ROADMAP_NO_VALID_DATA)) {
               got_navigation_data = 1;
               status = 'A';
            }
            break;

         case 'Q':

            if (RoadmapGpsd2DilutionListener == NULL) continue;

            if (roadmap_input_split (value, ' ', argument, 6) < 4) {
               continue;
            }

            pdop = roadmap_gpsd2_decode_float (argument[1]);
            hdop = roadmap_gpsd2_decode_float (argument[2]);
            vdop = roadmap_gpsd2_decode_float (argument[3]);

            if (dimension_of_fix > 0) {

               RoadmapGpsd2DilutionListener
                  (dimension_of_fix, pdop, hdop, vdop);

            } else if ((dimension_of_fix < 0) &&
                (roadmap_gpsd2_decode_numeric(argument[0]) > 0)) {

               dimension_of_fix = 2;
               got_q = 1;
            }
            break;

         case 'T':

            if (got_o) continue; /* Consider the 'O' answer only. */

            steering = roadmap_gpsd2_decode_numeric (value);
            break;

         case 'V':

            if (got_o) continue; /* Consider the 'O' answer only. */

            speed = roadmap_gpsd2_decode_numeric (value);
            break;

         case 'Y':

            if (RoadmapGpsd2SatelliteListener == NULL) continue;

            count = roadmap_input_split (value, ':', argument, 32);
            if (count <= 0) continue;

            switch (roadmap_input_split (argument[0], ' ', tuple, 5)) {

               case 1:
                  
                  satellite_count = roadmap_gpsd2_decode_numeric(tuple[0]);
                  break;

               case 3:

                  gps_time = roadmap_gpsd2_decode_numeric(tuple[1]);
                  satellite_count = roadmap_gpsd2_decode_numeric(tuple[2]);
                  break;

               default: continue; /* Invalid. */
            }

            if (satellite_count != count - 2) {
               roadmap_log (ROADMAP_WARNING,
                     "invalid gpsd 'Y' answer: count = %d, but %d satellites",
                     count, satellite_count);
               continue;
            }

            for (s = 1; s <= satellite_count; ++s) {

               if (roadmap_input_split (argument[s], ' ', tuple, 5) != 5) {
                  continue;
               }
               (*RoadmapGpsd2SatelliteListener)
                    (s,
                     roadmap_gpsd2_decode_numeric(tuple[0]),
                     roadmap_gpsd2_decode_numeric(tuple[1]),
                     roadmap_gpsd2_decode_numeric(tuple[2]),
                     roadmap_gpsd2_decode_numeric(tuple[3]),
                     roadmap_gpsd2_decode_numeric(tuple[4]));
            }
            (*RoadmapGpsd2SatelliteListener) (0, 0, 0, 0, 0, 0);

            break;
      }
   }

end_of_decoding:

   if (dimension_of_fix >= 0 && got_q) {
      RoadmapGpsd2DilutionListener (dimension_of_fix, pdop, hdop, vdop);
   }

   if (got_navigation_data) {

      RoadmapGpsd2NavigationListener
         (status, gps_time, latitude, longitude, altitude, speed, steering);

      return 1;
   }

   return 0;
}

