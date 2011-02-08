/* roadmap_geo_config.h
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


#ifndef ROADMAP_GEO_CONFIG_H_
#define ROADMAP_GEO_CONFIG_H_

#include "websvc_trans/string_parser.h"

void roadmap_geo_config(RoadMapCallback callback);

void roadmap_geo_config_transaction_failed();
const char *on_geo_server_config (/* IN  */   const char*       data,
                                  /* IN  */   void*             context,
                                  /* OUT */   BOOL*             more_data_needed,
                                  /* OUT */   roadmap_result*   rc);

const char *on_server_config      (/* IN  */   const char*       data,
                                  /* IN  */   void*             context,
                                  /* OUT */   BOOL*             more_data_needed,
                                  /* OUT */   roadmap_result*   rc);

const char *on_update_config      (/* IN  */   const char*       data,
                                  /* IN  */   void*             context,
                                  /* OUT */   BOOL*             more_data_needed,
                                  /* OUT */   roadmap_result*   rc);

void roadmap_geo_config_il(RoadMapCallback callback);
void roadmap_geo_config_usa(RoadMapCallback callback);
void roadmap_geo_config_other(RoadMapCallback callback);
void roadmap_geo_config_stg(RoadMapCallback callback);
void roadmap_geo_config_generic(char * name);
const char *roadmap_geo_config_get_version(void);
const char *roadmap_geo_config_get_server_id(void);
#endif /* ROADMAP_GEO_CONFIG_H_ */
