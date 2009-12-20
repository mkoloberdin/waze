
/*
 * LICENSE:
 *
 *   Copyright 2008 PazO
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
 */
 
#ifndef  __ADDRESS_SEARCH_DEFS_H__
#define  __ADDRESS_SEARCH_DEFS_H__

#define  ADSR_WEBSVC_CFG_FILE          ("preferences")
#define  ADSR_WEBSVC_CFG_TAB           ("Address Search")
#define  ADSR_WEBSVC_ADDRESS           ("Web-Service Address")
#define  ADSR_WEBSVC_DEFAULT_ADDRESS   ("")

#define  LSR_WEBSVC_CFG_FILE          ("preferences")
#define  LSR_WEBSVC_CFG_TAB           ("Local Search")
#define  LSR_WEBSVC_ADDRESS           ("Web-Service Address")
#define  LSR_WEBSVC_DEFAULT_ADDRESS   ("")

#define  ADSR_CITY_STRING_MAX_SIZE        (63)
#define  ADSR_STREET_STRING_MAX_SIZE      (128)
#define  ADSR_NAME_STRING_MAX_SIZE        (250)
#define  ADSR_STATE_STRING_MAX_SIZE       (32)
#define  ADSR_COUNTY_STRING_MAX_SIZE      (32)
#define  ADSR_PHONE_STRING_MAX_SIZE       (32)
#define  ADSR_URL_STRING_MAX_SIZE         (250)

#define  ADSR_ADDRESS_MAX_SIZE         (  ADSR_CITY_STRING_MAX_SIZE  +  \
                                          ADSR_STREET_STRING_MAX_SIZE+  \
                                          ADSR_STATE_STRING_MAX_SIZE +  \
                                          ADSR_COUNTY_STRING_MAX_SIZE+  \
                                          12 /* house number */   )
#define  ADSR_ADDRESS_MIN_SIZE         (3)
#define  LSR_ADDRESS_MIN_SIZE         (2)
#define  ADSR_MAX_RESULTS              (13)

#define  ADDRESS_STRING_MAX_SIZE          (112)
#define  ADDRESS_HISTORY_CATEGORY         ('A')
#define  ADDRESS_FAVORITE_CATEGORY        ('F')
#define  ADDRESS_HISTORY_STATE            ("IL")
#define  ADDRESS_STREET_NAME_MAX_SIZE     (112)

// Definitions for 'roadmap_history.h' API
typedef enum tag_search_type
{
   search_address,
   search_local,
   __search_types_count
}  search_types;

typedef struct tag_address_info
{
   const char* state;
   const char* country;
   const char* city;
   const char* street;
   const char* house;
   
}  address_info, *address_info_ptr;


// Definitions for 'roadmap_history.h' API
typedef enum tag_address_history_item
{
   ahi_house_number,
   ahi_street,
   ahi_city,
   ahi_state,
   ahi_name,
   ahi_latitude,
   ahi_longtitude,
   
   ahi__count,
   ahi__invalid

}  address_history_item;

typedef struct tag_address_candidate
{
   double   longtitude;
   double   latitude;
   char     state [ADSR_STATE_STRING_MAX_SIZE   + 1];
   char     county[ADSR_COUNTY_STRING_MAX_SIZE  + 1];
   char     city  [ADSR_CITY_STRING_MAX_SIZE    + 1];
   char     street[ADSR_STREET_STRING_MAX_SIZE  + 1];
   int      house;   
   char     name[ADSR_NAME_STRING_MAX_SIZE  + 1];
   char     phone[ADSR_PHONE_STRING_MAX_SIZE  + 1];
   char     url[ADSR_URL_STRING_MAX_SIZE+1];
   // Full string:
   char     address[ADSR_ADDRESS_MAX_SIZE  + 1];
   
}     address_candidate;
void  address_candidate_init( address_candidate* this);
BOOL  address_candidate_build_address_string( address_candidate* this);

#endif   // __ADDRESS_SEARCH_DEFS_H__

