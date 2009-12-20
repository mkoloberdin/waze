
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

#ifndef  __LOCAL_SEARCH_H__
#define  __LOCAL_SEARCH_H__

#include "../roadmap.h"
#include "../address_search/address_search_defs.h"
#include "../address_search/generic_search.h"

BOOL  local_search_init();
void  local_search_term();

roadmap_result local_search_resolve_address(
                  void*                context,
                  CB_OnAddressResolved cbOnAddressResolved,
                  const char*          address);

#endif   // __LOCAL_SEARCH_H__

