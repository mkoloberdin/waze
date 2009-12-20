/* roadmap_sunrise.c - calculate sunrise/sunset time
 *
 * License:
 *
 *   Copyright 2003 Eric Domazlicky
 *   Copyright 2003 Pascal Martin (changed module interface)
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
 *   see roadmap_sunrise.h
 *
 *   Sun and moon position calculations are (simplified) from:
 *      http://www.satellite-calculations.com/Satellite/suncalc.htm
 *   who got the actual formulas from the terrific site:
 *      http://www.stjarnhimlen.se/comp/tutorial.html
 *   (This implementation by Morten Bek Ditlevsen, with the written
 *   permission of the author of the javascript version on
 *   satellite-calculations.com, Jens T. Satre.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "roadmap_sunrise.h"

#define PI                  3.14159265358979323846
#define DEGREES             (180 / PI)
#define RADIANS             (PI / 180)
#define LU_PER_DEG          1000000     /* file lat/lon units per degree */
#define DEG_TO_LU(deg)      ( (long) ((deg) * LU_PER_DEG) )
#define LU_TO_DEG(lu)       ( (float) (lu) / LU_PER_DEG )

#define ROADMAP_SUNRISE 1
#define ROADMAP_SUNSET -1


/* returns an angle in range of 0 to (2 * PI) */

static double roadmap_sunrise_getrange(double x) {

   double temp1,temp2;

   temp1 = x / (2 * PI);
   temp2 = (2 * PI) * (temp1 - floor(temp1));
   if(temp2<0) {
       temp2 = (2*PI) + temp2;
   }

   return(temp2);
}


static time_t roadmap_sunrise_get_gmt(double decimaltime,
                                      struct tm *curtime) {

   time_t gmt;
#ifndef _WIN32
   char   *tz;
#endif

   double temp1;
   int temp2;

   if (decimaltime < 0)  {
      decimaltime += 24;
   }
   if (decimaltime > 24) {
      decimaltime = decimaltime - 24;
   }
   temp1 = fabs(decimaltime);
   temp2 = (int)(floor(temp1));
   temp1 = 60 * (temp1 - floor(temp1));

   /* fill in the tm structure */
   curtime->tm_hour = temp2;
   curtime->tm_min = (int) temp1;

#if !defined(_WIN32) && !defined(__SYMBIAN32__)
   tz = getenv("TZ");
   if (tz == NULL || tz[0] != 0) {
      setenv("TZ", "", 1);
      tzset();
   }
#endif

   gmt = mktime(curtime);

#if !defined(_WIN32) && !defined(__SYMBIAN32__)
   if (tz != NULL) {
      if (tz[0] != 0) {
         setenv("TZ", tz, 1);
      }
   } else {
      unsetenv("TZ");
   }
   tzset();
#endif

   return gmt;
}

static double rad(double deg)
{
   return (deg * RADIANS);
}

static double deg(double rad)
{
    return( rad * DEGREES);
}

static double Rev(double number)
{
    double x;
    x= number - floor(number/360.0)*360 ;
    return x;
}

static int dayofyear(int year, int month, int day)
{
  int x;
  int monthday[] = {0,31,59,90,120,151,181,212,243,273,304,334};

  x = monthday[month-1] + day;
  if ((year%4 == 0) && ((year%100!=0)||(year%400==0)) && (month>2))
    x++;
  return x;
}

void roadmap_sunposition(
        const RoadMapGpsPosition *position,
        double *azimuth, double *height)
{
    time_t gmt;
    struct tm curtime;
    int year, month, day, hour, min, sec;
    double J, J2;
    double Zgl, MOZ, WOZ, w;
    double decl;
    double sunaz, sunhi;
    double asinGs;
    double acosAs;
    double lat  = LU_TO_DEG(position->latitude);
    double lon  = LU_TO_DEG(position->longitude);

    if (azimuth == NULL || height == NULL)
        return;

    time(&gmt);

    curtime = *(gmtime(&gmt));
    year = curtime.tm_year + 1900;
    month = curtime.tm_mon+1;
    day = curtime.tm_mday;
    hour = curtime.tm_hour;
    min = curtime.tm_min;
    sec = curtime.tm_sec;

    J2 = 365;
    if (year % 4 == 0) J2++;
    J = dayofyear(year, month, day);
    MOZ = hour + min/60.0 + sec/3600.0;
    MOZ += lon / 15.0;
    J = J * 360 / J2 + MOZ / 24;
    decl = 0.3948 - 23.2559 * cos(rad(J + 9.1)) - 0.3915 * cos(rad(2 * J + 5.4)) - 0.1764 * cos(rad(3 * J + 26.0));
    Zgl = 0.0066 + 7.3525 * cos(rad(J + 85.9)) + 9.9359 * cos(rad(2 * J + 108.9)) + 0.3387 * cos(rad(3 * J + 105.2));
    WOZ = MOZ + Zgl / 60;
    w = (12 - WOZ) * 15;
    asinGs = cos(rad(w)) * cos(rad(lat)) * cos(rad(decl)) + sin(rad(lat)) * sin(rad(decl));
    if (asinGs > 1) asinGs = 1;
    if (asinGs < -1) asinGs = -1;
    sunhi = deg(asin(asinGs));
    acosAs = (sin(rad(sunhi)) * sin(rad(lat)) - sin(rad(decl))) / (cos(rad(sunhi)) * cos(rad(lat)));
    if (acosAs > 1) acosAs = 1;
    if (acosAs < -1) acosAs = -1;
    sunaz = deg(acos(acosAs));
    if ((WOZ > 12) || (WOZ < 0)) {
        sunaz = 180 + sunaz;
    } else {
        sunaz = 180 - sunaz;
    };
    *azimuth = sunaz;
    *height = sunhi;
}

void roadmap_moonposition(
        const RoadMapGpsPosition *position,
        double *azimuth, double *elevation)
{
    double HourAngle,SIDEREALTIME;
    double w,a,e,M,N,oblecl,E,x,y,r,v,sunlon,z,xequat,yequat,zequat,GMST0,UT;
    double xhor,yhor,zhor;
    double E0,E1,xeclip,yeclip,zeclip,Ls;
    double Moon_RA,Moon_Decl;
    double xh,yh,zh,Iterations,E_error,Ebeforeit,Eafterit,E_ErrorBefore;

    double lat  = LU_TO_DEG(position->latitude);
    double lon  = LU_TO_DEG(position->longitude);
    int d_int;

    time_t gmt;
    struct tm curtime;
    int year, month, day, hour, min, sec;
    double wsun, Msun, d, i;
    double moon_longitude, moon_latitude;

    if (azimuth == NULL || elevation == NULL)
        return;

    time(&gmt);

    curtime = *(gmtime(&gmt));
    year = curtime.tm_year + 1900;
    month = curtime.tm_mon+1;
    day = curtime.tm_mday;
    hour = curtime.tm_hour;
    min = curtime.tm_min;
    sec = curtime.tm_sec;

    d_int = 367*year - (7*(year + ((month+9)/12)))/4 + (275*month)/9 + day - 730530;
    d = d_int + (hour * 3600 + min * 60 + sec) / 86400.0;

    //*********CALCULATE Moon DATA *********************
    N=125.1228-0.0529538083*d;
    i=5.1454;

    w=318.0634 + 0.1643573223 * d ;
    a=60.2666;

    e= 0.054900;
    M= 115.3654 + 13.0649929509 * d;

    w=Rev(w);
    M=Rev(M);
    N=Rev(N);



    E=M+ DEGREES*e*sin(RADIANS * M)*(1+e * cos(RADIANS*M));
    E=Rev(E);

    Ebeforeit=E;

    // now iterate until difference between E0 and E1 is less than 0.005_deg
    // use E0, calculate E1

    Iterations=0;
    E_error=9;

    while ((E_error>0.0005) && (Iterations<20))
    {
        Iterations=Iterations+1;
        E0=E;
        E1= E0-(E0-DEGREES*e*sin(RADIANS*E0)-M) / (1-e * cos(RADIANS*E0)) ;
        E=Rev(E1);

        Eafterit=E;

        if (E<E0) E_error=E0-E;
        else E_error=E-E0;


        if (E<Ebeforeit) E_ErrorBefore=Ebeforeit-E;
        else E_ErrorBefore=E-Ebeforeit;

    }

    x= a*(cos(RADIANS*E) -e) ;
    y= a*sin(RADIANS * (Rev(E)))*sqrt(1-e*e);
    r=sqrt(x*x+y*y);
    v=deg(atan2(y,x));

    sunlon=Rev(v+w);

    x=r*cos(RADIANS * sunlon);
    y=r*sin(RADIANS * sunlon);
    z=0;

    xeclip=r*( cos(rad(N))*cos(rad(v+w)) - sin(rad(N)) * sin(rad(v+w))*cos(rad(i))   );
    yeclip=r*( sin(rad(N))*cos(rad(v+w)) + cos(rad(N)) * sin(rad(v+w))*cos(rad(i))   );
    zeclip=r*sin(rad(v+w))*sin(rad(i));

    moon_longitude=Rev(deg(atan2(yeclip,xeclip)));
    moon_latitude=deg(atan2(zeclip,sqrt(xeclip*xeclip +yeclip*yeclip )  ));

//    printf ("lon: %f, lat: %f\n", moon_longitude, moon_latitude);
    // get the Eliptic coordinates

    oblecl=23.4393 -3.563E-7 * d ;
    xh=r*cos(rad(moon_longitude))*cos(rad(moon_latitude));
    yh=r*sin(rad(moon_longitude))*cos(rad(moon_latitude));
    zh=r*sin(rad(moon_latitude));
    // rotate to rectangular equatorial coordinates
    xequat=xh;
    yequat=yh*cos(rad(oblecl))-zh*sin(rad(oblecl));
    zequat=yh*sin(rad(oblecl))+zh*cos(rad(oblecl));

    Moon_RA=Rev(deg(atan2(yequat,xequat)));
    Moon_Decl=deg(atan2(zequat,sqrt(xequat*xequat + yequat*yequat )  ));

    wsun=282.9404 + 4.70935E-5 * d ;
    Msun= 356.0470 + 0.9856002585 * d;
    Ls=wsun+Rev(Msun);

    Ls=Rev(Ls);

    GMST0=(Ls+180);


    //*********CALCULATE TIME *********************

    UT=d-floor(d);

    SIDEREALTIME=GMST0+UT*360+lon;
    HourAngle=SIDEREALTIME-Moon_RA;


    x=cos(HourAngle*RADIANS) * cos(Moon_Decl*RADIANS);
    y=sin(HourAngle*RADIANS) * cos(Moon_Decl*RADIANS);
    z=sin(Moon_Decl*RADIANS);

    xhor=x*sin(lat*RADIANS) - z*cos(lat*RADIANS);
    yhor=y;
    zhor=x*cos(lat*RADIANS) + z*sin(lat*RADIANS);

    *elevation=deg(asin(zhor));
    *elevation-=deg(asin( 1/r * cos(rad(*elevation))));

    *azimuth=deg(atan2(yhor,xhor));
    //if (lat<0)
    *azimuth+=180;

}

/* Calling parameters:
 * A roadmapposition for the point you'd like to get the rise or set time for
 * A timezone/gmt offset in integer form. example: cst is -6. some provisions
 * need to be made for daylight savings time.
 * riseorset: use the constants ROADMAP_SUNRISE or ROADMAP_SUNSET.
 * the caller must have set at least the current date in the tm structure:
 * - on success this structure's members tm_hour and tm_min will be set to
 *   the time of the sunrise or sunset.
 * - on failure the function returns 0.
 *
 * Notes: this function won't work for latitudes at 63 and above and -63 and
 * below
 */
static time_t roadmap_sunrise_getriseorset
                 (const RoadMapGpsPosition *position, int riseorset, time_t gmt) {

//   time_t gmt;
   time_t result;
   struct tm curtime;
   struct tm curtime_gmt;

   double sinalt,sinphi,cosphi;
   double longitude,l,gx,gha,correction,ec,lambda,delta,e,obl,cosc;
   double utold,utnew;
   double days,t;
   int count = 0;
   int ephem2000day;
   double roadmap_lat;
   double roadmap_alt;

   /* not accurate for places too close to the poles */
   if(abs(position->latitude) > 63000000) {
       return -1;
   }

   /* check for valid riseorset parameter */
   if((riseorset!=1)&&(riseorset!=-1)) {
       return -1;
   }

//   time(&gmt);
   curtime = *(localtime(&gmt));
   curtime_gmt = *(gmtime(&gmt));

   

   ephem2000day = 367 * (curtime.tm_year+1900)
                      - (7 * ((curtime.tm_year+1900)
                                  + ((curtime.tm_mon+1) + 9) / 12) / 4)
                      + 275 * (curtime.tm_mon+1) / 9
                      + curtime.tm_mday - 730531;
   utold = PI;
   utnew = 0;

   roadmap_alt = LU_TO_DEG(position->altitude);
   //sinalt = -0.0174456; /* tbd: sets according to position->altitude. */
   sinalt = sin(roadmap_alt * RADIANS);
   
   roadmap_lat  = LU_TO_DEG(position->latitude);
   sinphi = sin(roadmap_lat * RADIANS);
   cosphi = cos(roadmap_lat * RADIANS);
   longitude = LU_TO_DEG(position->longitude+180000000) * RADIANS;

   while((fabs(utold-utnew) > 0.001) && (count < 35))  {

      count++;
      utold=utnew;
      days = (1.0 * ephem2000day - 0.5) + utold / (2 * PI);
      t = days / 36525;
      l  = roadmap_sunrise_getrange(4.8949504201433 + 628.331969753199 * t);
      gx = roadmap_sunrise_getrange(6.2400408 + 628.3019501 * t);
      ec = .033423 * sin(gx) + .00034907 * sin(2. * gx);
      lambda = l + ec;
      e = -1 * ec + .0430398 * sin(2. * lambda) - .00092502 * sin(4. * lambda);
      obl = .409093 - .0002269 * t;

      delta = sin(obl) * sin(lambda);
      delta = atan(delta / (sqrt(1 - delta * delta)));

      gha = utold - PI + e;
      cosc = (sinalt - sinphi * sin(delta)) / (cosphi * cos(delta));
      if(cosc > 1) {
         correction = 0;
      } else if(cosc < -1) {
         correction = PI;
      } else {
         correction = atan((sqrt(1 - cosc * cosc)) / cosc);
      }

      utnew = roadmap_sunrise_getrange
                  (utold - (gha + longitude + riseorset * correction));
   }

   /* utnew must now be converted into gmt. */

   result = roadmap_sunrise_get_gmt (utnew * DEGREES / 15, &curtime_gmt);

   if (result < gmt) {
      result += (24 * 3600);
   }

   return result;
}


time_t roadmap_sunrise (const RoadMapGpsPosition *position, time_t now) {

   static time_t validity = 0;
   static time_t sunrise = 0;

   // now = time(NULL);

   if (validity < now) {

      sunrise = roadmap_sunrise_getriseorset (position, ROADMAP_SUNRISE, now);

      if (sunrise < now) {

         /* This happened already: wait for the next sunrise. */

         validity = roadmap_sunrise_getriseorset (position, ROADMAP_SUNRISE, now);

         if (validity < now) {
            /* We want the next sunset, not the last one. */
            validity += (24 * 3600);
         }

      } else if (now + 3600 < sunrise) {

         /* The next sunrise is in more than one hour: recompute every 15mn. */
         validity = now + 900;

      } else {

         /* The next sunrise is quite soon: recompute it more frequently as
          * we get closer to the deadline.
          */
         validity = (sunrise + now) / 4;

         if (validity < now + 60) {
            /* Do not recompute the sunrise every second. */
            validity = now + 60;
         }
      }
   }

   return sunrise;
}

time_t roadmap_sunset (const RoadMapGpsPosition *position, time_t now) {

   static time_t validity = 0;
   static time_t sunset = 0;

   //time_t now = time(NULL);

   if (now > validity) {

      sunset = roadmap_sunrise_getriseorset (position, ROADMAP_SUNSET, now);

      if (sunset < now) {

         /* This happened already: wait for the next sunset. */

         validity = roadmap_sunrise_getriseorset (position, ROADMAP_SUNSET, now);

         if (validity < now) {
            /* We want the next sunrise, not the last one. */
            validity += (24 * 3600);
         }

      } else if (now + 3600 < sunset) {

         /* The next sunset is in more than one hour: recompute every 15mn. */
         validity = now + 900;

      } else {

         /* The next sunset is quite soon: recompute it more frequently as
          * we get closer to the deadline.
          */
         validity = (sunset + now) / 4;

         if (validity < now + 60) {
            /* Do not recompute the sunset every second. */
            validity = now + 60;
         }
      }
   }

   return sunset;
}


#ifdef SUNRISE_PROGRAM

/* To test this:
 * ./sunrise 0 51500000 :Europe/London
 * ./sunrise 2000000 49000000 :Europe/Paris
 * ./sunrise -118352033 33810550 :America/Los_Angeles
 *
 * and compare to http://www.sunrisesunset.com/
 */
int main(int argc, char **argv) {

   time_t rawtime;
   struct tm *realtime;
   RoadMapGpsPosition position;

   if (argc > 3) {
      setenv("TZ", argv[3], 1);
      tzset();
   } else if (argc != 3) {
      fprintf (stderr, "Usage: %s longitude latitude [timezone]\n"
                       "missing position\n", argv[0]);
      exit(1);
   }
   position.longitude = atoi(argv[1]);
   position.latitude = atoi(argv[2]);

   time(&rawtime);
   realtime = localtime(&rawtime);

   printf("current date/time: %s", asctime(realtime));

   double a, b;
   roadmap_sunposition(&position, &a, &b);
   printf ("Sun azimuth: %.2f\nSun elevation: %.2f\n", a, b);

   roadmap_moonposition(&position, &a, &b);
   printf ("Moon azimuth: %.2f\nMoon elevation: %.2f\n", a, b);

   rawtime = roadmap_sunrise (&position, time(NULL));
   realtime = localtime(&rawtime);

   printf("sunrise: %d:%d\n", realtime->tm_hour, realtime->tm_min);

   rawtime = roadmap_sunset (&position, time(NULL));
   realtime = localtime(&rawtime);

   printf("sunset:  %d:%d\n", realtime->tm_hour, realtime->tm_min);
   return 0;
}
#endif /* SUNRISE_PROGRAM */

