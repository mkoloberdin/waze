/* roadmap_nmea.c - Decode a NMEA sentence.
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
 * SYNOPSYS:
 *
 *   See roadmap_nmea.h
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if ! defined (_WIN32) && ! defined (J2ME)
#include <errno.h>
#endif

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_preferences.h"
#include "roadmap_nmea.h"


#define TIGER_COORDINATE_UNIT 1000000


struct RoadMapNmeaAccountRecord {

   const char *name;
   int count;

   RoadMapNmeaListener listener[1]; /* Allocated with more than one ... */
};


static RoadMapNmeaFields RoadMapNmeaReceived;

static char RoadMapNmeaDate[16];

static RoadMapDynamicStringCollection RoadMapNmeaCollection;


static int hex2bin (char c) {

   if ((c >= '0') && (c <= '9')) {
      return c - '0';
   } else if ((c >= 'A') && (c <= 'F')) {
      return c - ('A' - 10);
   } else if ((c >= 'a') && (c <= 'f')) {
      return c - ('a' - 10);
   }

   /* Invalid character. */
   roadmap_log (ROADMAP_ERROR, "invalid hexadecimal character '%c'", c);
   return 0;
}


static int dec2bin (char c) {

   if ((c >= '0') && (c <= '9')) {
      return c - '0';
   }

   /* Invalid character. */
   roadmap_log (ROADMAP_ERROR, "invalid decimal character '%c'", c);
   return 0;
}


static time_t roadmap_nmea_decode_time (const char *hhmmss,
                                        const char *ddmmyy) {

   static struct tm tm;

#ifdef J2ME
   if (strlen(hhmmss) < 6) return -1;

   tm.tm_hour = (hhmmss[0] - 48) * 10 + hhmmss[1] - 48;
   tm.tm_min  = (hhmmss[2] - 48) * 10 + hhmmss[3] - 48;
   tm.tm_sec  = (hhmmss[4] - 48) * 10 + hhmmss[5] - 48;
   if ((tm.tm_hour < 0) || (tm.tm_hour > 23)) return -1;
   if ((tm.tm_min < 0) || (tm.tm_min > 59)) return -1;
   if ((tm.tm_sec < 0) || (tm.tm_sec > 59)) return -1;

#else

   if (sscanf (hhmmss, "%02d%02d%02d",
               &(tm.tm_hour), &(tm.tm_min), &(tm.tm_sec)) != 3) {
      return -1;
   }
#endif

   if ((ddmmyy != NULL) && *ddmmyy) {

#ifdef J2ME
      if (strlen(ddmmyy) < 6) return -1;

      tm.tm_mday = (ddmmyy[0] - 48) * 10 + ddmmyy[1] - 48;
      tm.tm_mon  = (ddmmyy[2] - 48) * 10 + ddmmyy[3] - 48;
      tm.tm_year  = (ddmmyy[4] - 48) * 10 + ddmmyy[5] - 48;
      if ((tm.tm_mday < 0) || (tm.tm_mday > 31)) return -1;
      if ((tm.tm_mon < 0) || (tm.tm_mon > 12)) return -1;
      if (tm.tm_year < 0) return -1;

#else

      if (sscanf (ddmmyy, "%02d%02d%02d",
                  &(tm.tm_mday), &(tm.tm_mon), &(tm.tm_year)) != 3) {
         return -1;
      }
#endif

      if (tm.tm_year < 50) {
         tm.tm_year += 100; /* Y2K. */
      }
      tm.tm_mon -= 1;

   } else if (tm.tm_year == 0) {
      /* The date is not yet known.
       * Use the system clock but return failure.
       * This gives a chance for the GPS to update
       * the date before the next cycle.
       */

      time_t cur_time;
      struct tm *cur_tm;

      time (&cur_time);
      cur_tm = gmtime (&cur_time);

      tm.tm_mday = cur_tm->tm_mday;
      tm.tm_mon  = cur_tm->tm_mon;
      tm.tm_year = cur_tm->tm_year;

      return -1;
   }

   /* FIXME: th time zone might change if we are moving !. */

   return timegm(&tm);
}


static int roadmap_nmea_decode_numeric (char *value, int unit) {

   int result;

   if (strchr (value, '.') != NULL) {
      result = (int) (atof(value) * unit);
   } else {
      result = atoi (value) * unit;
   }
   return result;
}


static int roadmap_nmea_decode_coordinate
              (char *value, char *side, char positive, char negative) {

   /* decode longitude & latitude from the nmea format (ddmm.mmmmm)
    * to the format used by the census bureau (dd.dddddd):
    */

   int result;
   char *dot = strchr (value, '.');


   if (dot == NULL) {
      if (value[0] == 0) return 0;
      dot = value + strlen(value);
   }

   dot -= 2;

   result = 0;
   while (value < dot) {
      result = dec2bin(*value) + (10 * result);
      value += 1;
   }
   result *= TIGER_COORDINATE_UNIT;

   result += roadmap_nmea_decode_numeric (dot, TIGER_COORDINATE_UNIT) / 60;

   if (side[1] == 0) {

      if (side[0] == negative) {
         return 0 - result;
      }
      if (side[0] == positive) {
         return result;
      }
   }

   return 0;
}


static char *roadmap_nmea_decode_unit (const char *original) {

    if( original && (*original) && (strcasecmp (original, "M") == 0)) {
        return "cm";
    }

    roadmap_log (ROADMAP_WARNING, "unknown distance unit (%s)", original);
    return "??";
}

typedef int (*RoadMapNmeaDecoder) (int argc, char *argv[]);


static int roadmap_nmea_rmc (int argc, char *argv[]) {

   if (argc <= 9) return 0;

   RoadMapNmeaReceived.rmc.status = *(argv[2]);

   if (RoadMapNmeaReceived.rmc.status == 'V')
      return 1;//no fix, other fields should be ignored 

   RoadMapNmeaReceived.rmc.fixtime =
      roadmap_nmea_decode_time (argv[1], argv[9]);

   strncpy_safe (RoadMapNmeaDate, argv[9], sizeof(RoadMapNmeaDate));

   if (RoadMapNmeaReceived.rmc.fixtime < 0) return 0;


   RoadMapNmeaReceived.rmc.latitude =
      roadmap_nmea_decode_coordinate  (argv[3], argv[4], 'N', 'S');

   if (RoadMapNmeaReceived.rmc.latitude == 0) return 0;

   RoadMapNmeaReceived.rmc.longitude =
      roadmap_nmea_decode_coordinate (argv[5], argv[6], 'E', 'W');

   if (RoadMapNmeaReceived.rmc.longitude == 0) return 0;


   RoadMapNmeaReceived.rmc.speed =
      roadmap_nmea_decode_numeric (argv[7], 1);

   RoadMapNmeaReceived.rmc.steering =
      roadmap_nmea_decode_numeric (argv[8], 1);

   return 1;
}


static int roadmap_nmea_gga (int argc, char *argv[]) {

   if (argc <= 10) return 0;

   switch (*argv[6]) {

      case 0:
      case '0':
         RoadMapNmeaReceived.gga.quality = ROADMAP_NMEA_QUALITY_INVALID;
         break;

      case '1':
         RoadMapNmeaReceived.gga.quality = ROADMAP_NMEA_QUALITY_GPS;
         break;

      case '2':
         RoadMapNmeaReceived.gga.quality = ROADMAP_NMEA_QUALITY_DGPS;
         break;

      case '3':
         RoadMapNmeaReceived.gga.quality = ROADMAP_NMEA_QUALITY_PPS;
         break;

      default:
         RoadMapNmeaReceived.gga.quality = ROADMAP_NMEA_QUALITY_OTHER;
         break;
   }

   if (RoadMapNmeaReceived.gga.quality == ROADMAP_NMEA_QUALITY_INVALID)
      return 1; //no fix, other fields should be ignored

   RoadMapNmeaReceived.gga.fixtime =
      roadmap_nmea_decode_time (argv[1], RoadMapNmeaDate);

   if (RoadMapNmeaReceived.gga.fixtime < 0) return 0;

   RoadMapNmeaReceived.gga.latitude =
      roadmap_nmea_decode_coordinate  (argv[2], argv[3], 'N', 'S');

   RoadMapNmeaReceived.gga.longitude =
      roadmap_nmea_decode_coordinate (argv[4], argv[5], 'E', 'W');

   RoadMapNmeaReceived.gga.count =
      roadmap_nmea_decode_numeric (argv[7], 1);

   RoadMapNmeaReceived.gga.dilution =
      roadmap_nmea_decode_numeric (argv[8], 100);

   RoadMapNmeaReceived.gga.altitude =
      roadmap_nmea_decode_numeric (argv[9], 100);

   strcpy (RoadMapNmeaReceived.gga.altitude_unit,
           roadmap_nmea_decode_unit (argv[10]));

   return 1;
}


static int roadmap_nmea_gsa (int argc, char *argv[]) {

   int i;
   int index;
   int last_satellite;

   if (argc <= 2) return 0;

   RoadMapNmeaReceived.gsa.automatic = *(argv[1]);
   RoadMapNmeaReceived.gsa.dimension = atoi(argv[2]);

   /* The last 3 arguments (argc-3 .. argc-1) are not satellites. */
   last_satellite = argc - 4;

   for (index = 2, i = 0;
        index < last_satellite && i < ROADMAP_NMEA_MAX_SATELLITE; ++i) {

      RoadMapNmeaReceived.gsa.satellite[i] = atoi(argv[++index]);
   }
   while (i < ROADMAP_NMEA_MAX_SATELLITE) {
      RoadMapNmeaReceived.gsa.satellite[i++] = 0;
   }

   RoadMapNmeaReceived.gsa.dilution_position   = (float) atof(argv[++index]);
   RoadMapNmeaReceived.gsa.dilution_horizontal = (float) atof(argv[++index]);
   RoadMapNmeaReceived.gsa.dilution_vertical   = (float) atof(argv[++index]);

   return 1;
}


static int roadmap_nmea_gll (int argc, char *argv[]) {

   char mode;
   int  valid_fix;


   if (argc <= 6) return 0;

   /* We have to be extra cautious, as some people report that GPGLL
    * returns a mode field, but some GPS do not have this field at all.
    */
   valid_fix = (strcmp (argv[6], "A") == 0);
   if (argc > 7) {
      valid_fix = (valid_fix && (strcmp(argv[7], "N") != 0));
      mode = *(argv[7]);
   } else {
      mode = 'A'; /* Sensible default. */
   }

   if (valid_fix) {

      RoadMapNmeaReceived.gll.latitude =
         roadmap_nmea_decode_coordinate  (argv[1], argv[2], 'N', 'S');

      RoadMapNmeaReceived.gll.longitude =
         roadmap_nmea_decode_coordinate (argv[3], argv[4], 'E', 'W');

      /* The UTC does not seem to be provided by all GPS vendors,
       * ignore it.
       */

      RoadMapNmeaReceived.gll.status = 'A';
      RoadMapNmeaReceived.gll.mode   = mode;

   } else {
       RoadMapNmeaReceived.gll.status = 'V'; /* bad. */
       RoadMapNmeaReceived.gll.mode   = 'N';
   }

   return 1;
}


static int roadmap_nmea_vtg (int argc, char *argv[]) {

   if (argc <= 5) return 0;
   if (!argv[1][0] || !argv[5][0]) return 0;

   RoadMapNmeaReceived.vtg.steering =
      roadmap_nmea_decode_numeric (argv[1], 1);

   RoadMapNmeaReceived.vtg.speed =
      roadmap_nmea_decode_numeric (argv[5], 1);

   return 1;
}


static int roadmap_nmea_gsv (int argc, char *argv[]) {

   int i;
   int end;
   int index;


   if (argc <= 3) return 0;

   RoadMapNmeaReceived.gsv.total = (char) atoi(argv[1]);
   RoadMapNmeaReceived.gsv.index = (char) atoi(argv[2]);
   RoadMapNmeaReceived.gsv.count = (char) atoi(argv[3]);

   if (RoadMapNmeaReceived.gsv.count < 0) {
      roadmap_log (ROADMAP_ERROR, "%d is an invalid number of satellites",
                   RoadMapNmeaReceived.gsv.count);
      return 0;
   }

   if (RoadMapNmeaReceived.gsv.count > ROADMAP_NMEA_MAX_SATELLITE) {

      roadmap_log (ROADMAP_ERROR, "%d is too many satellite, %d max supported",
                   RoadMapNmeaReceived.gsv.count,
                   ROADMAP_NMEA_MAX_SATELLITE);
      RoadMapNmeaReceived.gsv.count = ROADMAP_NMEA_MAX_SATELLITE;
   }

   end = RoadMapNmeaReceived.gsv.count
            - ((RoadMapNmeaReceived.gsv.index - 1) * 4);

   if (end > 4) end = 4;

   if (argc <= (end * 4) + 3) {
      return 0;
   }

   for (index = 3, i = 0; i < end; ++i) {

      RoadMapNmeaReceived.gsv.satellite[i] = atoi(argv[++index]);
      RoadMapNmeaReceived.gsv.elevation[i] = atoi(argv[++index]);
      RoadMapNmeaReceived.gsv.azimuth[i]   = atoi(argv[++index]);
      RoadMapNmeaReceived.gsv.strength[i]  = atoi(argv[++index]);
   }

   for (i = end; i < 4; ++i) {
      RoadMapNmeaReceived.gsv.satellite[i] = 0;
      RoadMapNmeaReceived.gsv.elevation[i] = 0;
      RoadMapNmeaReceived.gsv.azimuth[i]   = 0;
      RoadMapNmeaReceived.gsv.strength[i]  = 0;
   }

   return 1;
}


static int roadmap_nmea_pgrmm (int argc, char *argv[]) {

    if (argc <= 1) return 0;

    strncpy_safe (RoadMapNmeaReceived.pgrmm.datum,
             		argv[1], sizeof(RoadMapNmeaReceived.pgrmm.datum));

    return 1;
}


static int roadmap_nmea_pgrme (int argc, char *argv[]) {

    if (argc <= 6) return 0;

    RoadMapNmeaReceived.pgrme.horizontal =
        roadmap_nmea_decode_numeric (argv[1], 100);
    strcpy (RoadMapNmeaReceived.pgrme.horizontal_unit,
            roadmap_nmea_decode_unit (argv[2]));

    RoadMapNmeaReceived.pgrme.vertical =
        roadmap_nmea_decode_numeric (argv[3], 100);
    strcpy (RoadMapNmeaReceived.pgrme.vertical_unit,
            roadmap_nmea_decode_unit (argv[4]));

    RoadMapNmeaReceived.pgrme.three_dimensions =
        roadmap_nmea_decode_numeric (argv[5], 100);
    strcpy (RoadMapNmeaReceived.pgrme.three_dimensions_unit,
            roadmap_nmea_decode_unit (argv[6]));

    return 1;
}


static int roadmap_nmea_pxrmadd (int argc, char *argv[]) {

    if (argc <= 3) return 0;

    RoadMapNmeaReceived.pxrmadd.id =
       roadmap_string_new_in_collection (argv[1], &RoadMapNmeaCollection);

    RoadMapNmeaReceived.pxrmadd.name =
       roadmap_string_new_in_collection (argv[2], &RoadMapNmeaCollection);

    RoadMapNmeaReceived.pxrmadd.sprite =
       roadmap_string_new_in_collection (argv[3], &RoadMapNmeaCollection);

    return 1;
}


static int roadmap_nmea_pxrmmov (int argc, char *argv[]) {

    if (argc <= 7) return 0;

    RoadMapNmeaReceived.pxrmmov.id =
       roadmap_string_new_in_collection (argv[1], &RoadMapNmeaCollection);

    RoadMapNmeaReceived.pxrmmov.latitude =
        roadmap_nmea_decode_coordinate  (argv[2], argv[3], 'N', 'S');

    RoadMapNmeaReceived.pxrmmov.longitude =
        roadmap_nmea_decode_coordinate (argv[4], argv[5], 'E', 'W');

    RoadMapNmeaReceived.pxrmmov.speed =
       roadmap_nmea_decode_numeric (argv[6], 1);

    RoadMapNmeaReceived.pxrmmov.steering =
       roadmap_nmea_decode_numeric (argv[7], 1);

    return 1;
}


static int roadmap_nmea_pxrmdel (int argc, char *argv[]) {

    if (argc <= 1) return 0;

    RoadMapNmeaReceived.pxrmdel.id =
       roadmap_string_new_in_collection (argv[1], &RoadMapNmeaCollection);

    return 1;
}


static int roadmap_nmea_pxrmsub (int argc, char *argv[]) {

    int i;
    int j;

    if (argc <= 1) return 0;

    for (i = 1, j = 0; i < argc; ++i, ++j) {
       RoadMapNmeaReceived.pxrmsub.subscribed[j].item =
          roadmap_string_new_in_collection (argv[i], &RoadMapNmeaCollection);
    }

    RoadMapNmeaReceived.pxrmsub.count = j;

    return 1;
}


static int roadmap_nmea_pxrmcfg (int argc, char *argv[]) {

    if (argc < 4) return 0;

    RoadMapNmeaReceived.pxrmcfg.category =
       roadmap_string_new_in_collection (argv[1], &RoadMapNmeaCollection);

    RoadMapNmeaReceived.pxrmcfg.name =
       roadmap_string_new_in_collection (argv[2], &RoadMapNmeaCollection);

    RoadMapNmeaReceived.pxrmcfg.value =
       roadmap_string_new_in_collection (argv[3], &RoadMapNmeaCollection);

    return 1;
}


static int roadmap_nmea_pgrmz (int argc, char *argv[]) {

   /* Altitude, 'f' for feet, 2 (altimeter) or 3 (GPS). */
   return 0; /* TBD */
}


/* Empty implementations: save from testing NULL. */

static int roadmap_nmea_null_decoder (int argc, char *argv[]) {

   return 0;
}



static struct {

   char               *vendor; /* NULL --> NMEA standard sentence. */
   char               *sentence;
   RoadMapNmeaDecoder  decoder;

} RoadMapNmeaPhrase[] = {

   {NULL, "RMC", roadmap_nmea_rmc},
   {NULL, "GGA", roadmap_nmea_gga},
   {NULL, "GSA", roadmap_nmea_gsa},
   {NULL, "GSV", roadmap_nmea_gsv},
   {NULL, "GLL", roadmap_nmea_gll},
   {NULL, "VTG", roadmap_nmea_vtg},

   /* We don't care about these ones (waypoints). */
   {NULL, "RTE", roadmap_nmea_null_decoder},
   {NULL, "RMB", roadmap_nmea_null_decoder},
   {NULL, "BOD", roadmap_nmea_null_decoder},

   /* Garmin extensions: */
   {"GRM", "E", roadmap_nmea_pgrme},
   {"GRM", "M", roadmap_nmea_pgrmm},
   {"GRM", "Z", roadmap_nmea_pgrmz},

   /* RoadMap's own extensions: */
   {"XRM", "ADD", roadmap_nmea_pxrmadd},
   {"XRM", "MOV", roadmap_nmea_pxrmmov},
   {"XRM", "DEL", roadmap_nmea_pxrmdel},
   {"XRM", "SUB", roadmap_nmea_pxrmsub},
   {"XRM", "CFG", roadmap_nmea_pxrmcfg},

   { NULL, "", NULL}
};


RoadMapNmeaAccount  roadmap_nmea_create(const char *name) {

   int count;
   RoadMapNmeaAccount account;

   /* Just count how many sentences we support. */

   for (count = 0; RoadMapNmeaPhrase[count].decoder != NULL; ++count) ;

   account = (RoadMapNmeaAccount)
      malloc (sizeof(struct RoadMapNmeaAccountRecord)
                 + (sizeof(RoadMapNmeaListener) * count));
   roadmap_check_allocated(account);

   account->name  = strdup(name);
   account->count = count;

   while (--count >= 0) {
      account->listener[count] = NULL;
   }

   return account;
}


void roadmap_nmea_subscribe (const char *vendor,
                             const char *sentence,
                             RoadMapNmeaListener listener,
                             RoadMapNmeaAccount  account) {

   int i;
   int found;


   for (i = 0; RoadMapNmeaPhrase[i].decoder != NULL; ++i) {

       if (strcmp (sentence, RoadMapNmeaPhrase[i].sentence) == 0) {

          if ((vendor == NULL) && (RoadMapNmeaPhrase[i].vendor == NULL)) {
             found = 1;
          }
          else if ((vendor != NULL) &&
                   (RoadMapNmeaPhrase[i].vendor != NULL) &&
                   (strcmp (vendor, RoadMapNmeaPhrase[i].vendor) == 0)) {
             found = 1;
          } else {
             found = 0;
          }

          if (found) {

             if (account->count <= i) {
                roadmap_log (ROADMAP_FATAL,
                             "invalid size for account '%s'", account->name);
             }

             account->listener[i] = listener;

             return;
          }
       }
   }

   if (vendor == NULL) {
      roadmap_log (ROADMAP_FATAL, "unsupported standard NMEA sentence '%s'",
                   sentence);
   } else {
      roadmap_log (ROADMAP_FATAL,
                   "unsupported NMEA sentence '%s' for vendor '%s'",
                   sentence, vendor);
   }
}


static int roadmap_nmea_call (void *user_context,
                              RoadMapNmeaAccount account,
                              int index, int count, char *field[]) {

   if (account == NULL || account->count <= index) {
      roadmap_log (ROADMAP_FATAL,
            "invalid account '%s'", account != NULL ? account->name : "(null)");
   }

   /* Skip sentences the user does not care about. */
   if (account->listener[index] == NULL) return 0;

   if ((*RoadMapNmeaPhrase[index].decoder) (count, field)) {

      (account->listener[index]) (user_context, &RoadMapNmeaReceived);

      roadmap_string_release_all (&RoadMapNmeaCollection);

      return 1; /* GPS information was successfully made available. */
   }

   return 0; /* Could not decode it. */
}


int roadmap_nmea_decode (void *user_context,
                         void *decoder_context, char *sentence, int length) {

   RoadMapNmeaAccount account = (RoadMapNmeaAccount) decoder_context;

   int i;
   char *p = sentence;

   int   count;
   char *field[80];

   unsigned char checksum = 0;


   /* We skip any leftover from previous transmission problems,
    * check that the '$' is really here, compute the checksum
    * and then decode the "csv" format.
    */
   while ((*p != '$') && (*p >= ' ')) ++p;

   if (*p != '$') return 0; /* Ignore this ill-formed sentence. */

   sentence = p++;
   //roadmap_log (ROADMAP_ERROR, "NMEA: '%s'", sentence);
   while ((*p != '*') && (*p >= ' ')) {
      checksum ^= *p;
      p += 1;
   }

   if (*p == '*') {

      unsigned char mnea_checksum = hex2bin(p[1]) * 16 + hex2bin(p[2]);

      if (mnea_checksum != checksum) {
         roadmap_log (ROADMAP_ERROR,
               "mnea checksum error for '%s' (nmea=%02x, calculated=%02x)",
               sentence,
               mnea_checksum,
               checksum);

         return 0;
      }
   }
   *p = 0;

   for (i = 0, p = sentence; p != NULL && *p != 0; ++i, p = strchr (p, ',')) {
      *p = 0;
      field[i] = ++p;
   }
   count = i;


   /* Now that we have separated each argument of the sentence, retrieve
    * the right decoder & listener functions and call them.
    */
   if (*(field[0]) == 'P') {

      /* This is a proprietary sentence. */

      for (i = 0; RoadMapNmeaPhrase[i].decoder != NULL; ++i) {

         /* Skip standard sentences. */
         if (RoadMapNmeaPhrase[i].vendor == NULL) continue;

         if ((strncmp(RoadMapNmeaPhrase[i].vendor, field[0]+1, 3) == 0) &&
             (strcmp(RoadMapNmeaPhrase[i].sentence, field[0]+4) == 0)) {

            return roadmap_nmea_call (user_context, account, i, count, field);
         }
      }
   } else {

      /* This is a standard sentence. */

      for (i = 0; RoadMapNmeaPhrase[i].decoder != NULL; ++i) {

         /* Skip proprietary sentences. */
         if (RoadMapNmeaPhrase[i].vendor != NULL) continue;

         if (strcmp (RoadMapNmeaPhrase[i].sentence, field[0]+2) == 0) {

            return roadmap_nmea_call (user_context, account, i, count, field);
         }
      }
   }

   roadmap_log (ROADMAP_DEBUG, "unknown nmea sentence %s", field[0]);

   return 0; /* Could not decode it. */
}

