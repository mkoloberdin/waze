/* generic_search.h
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi Ben-Shoshan
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
 *
 */


#ifndef GENERIC_SEARCH_H_
#define GENERIC_SEARCH_H_
#include "../roadmap.h"
#include "../address_search/address_search_defs.h"
#include "../websvc_trans/websvc_trans.h"

typedef void(*CB_OnAddressResolved)(   void*                context,
                                       address_candidate*   array, 
                                       int                  size, 
                                       roadmap_result       rc);

roadmap_result generic_search_resolve_address(
                  wst_handle           websvc,
                  wst_parser           *data_parser,
                  int                  parser_count,
                  const char           *service_name,
                  void*                context,
                  CB_OnAddressResolved cbOnAddressResolved,
                  const char*          address);

void generic_address_add(address_candidate ac);
const address_candidate* generic_search_results();
const address_candidate* generic_search_result( int index);
int generic_search_result_count();
const address_candidate* address_search_result( int index);
void address_candidate_init( address_candidate* this);

BOOL navigate_with_coordinates( BOOL take_me_there, search_types type, int   selected_list_item);
const char* get_house_number__str( int i);
void generic_search_add_to_favorites();

void generic_search_add_address_to_history( int               category,
                                    const char*       city,
                                    const char*       street,
                                    const char*       house,
                                    const char*       state,
                                    const char*       name,
                                    RoadMapPosition*  position);
#endif /* GENERIC_SEARCH_H_ */
