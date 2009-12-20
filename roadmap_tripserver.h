/* roadmap_tripserver.h
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


#ifndef ROADMAP_TRIPSERVER_H_
#define ROADMAP_TRIPSERVER_H_

typedef const char*  (*CB_TripServerResponse)(  int   NumParams, const char*  pData);

typedef struct tag_TripServeHandlers
{
   const char*       Response;
   CB_TripServerResponse  handler;

}  TripServeHandlers;

const char* roadmap_tripserver_response(int status, int NumParams, const char*  pData);

#endif /* ROADMAP_TRIPSERVER_H_ */
