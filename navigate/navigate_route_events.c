/* navigate_route_events.c - Events on route
 *
 * LICENSE:
 *
 *   Copyright 2007 Avi Ben-Shoshan
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
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_res.h"
#include "navigate_route_events.h"
#include "ssd/ssd_widget.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_progress.h"
#include "ssd/ssd_separator.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_text.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeAltRoutes.h"
#include "Realtime/RealtimeAlerts.h"

static event_on_routes_table g_events_on_route;

static RoadMapConfigDescriptor RoadMapConfigFeatureEnabled =
      ROADMAP_CONFIG_ITEM("Events on Route", "Feature enabled");


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL events_on_route_feature_enabled (void) {
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigFeatureEnabled), "yes")){
      return TRUE;
   }
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
void events_on_route_init(void) {
	int i;

   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigFeatureEnabled, NULL, "yes", "no", NULL);

	for (i = 0; i < MAX_ALERTS_ON_ROUTE; i++)
		g_events_on_route.events_on_route[i] = NULL;

	g_events_on_route.iCount = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
int events_on_route_count(void) {
	return g_events_on_route.iCount;
}

///////////////////////////////////////////////////////////////////////////////////////////
int events_on_route_count_alternative(int iAltRouteID) {
	int i;
	int count = 0;
	if (iAltRouteID == -1)
		return events_on_route_count();

	for (i = 0; i < g_events_on_route.iCount; i++){
		if (g_events_on_route.events_on_route[i] && g_events_on_route.events_on_route[i]->iAltRouteID == iAltRouteID)
			count++;
	}
	return count;
}

///////////////////////////////////////////////////////////////////////////////////////////
event_on_route_info * events_on_route_at_index(int index) {

	if (index >= events_on_route_count()){
		return NULL;
	}

	return g_events_on_route.events_on_route[index];
}

///////////////////////////////////////////////////////////////////////////////////////////
event_on_route_info * events_on_route_at_index_alternative(int index, int iAltRouteID) {
	int i;
	int current_index = 0;

	if (iAltRouteID == -1)
		return events_on_route_at_index(index);


	if (index >= events_on_route_count_alternative(iAltRouteID))
		return NULL;

	for (i = 0; i < events_on_route_count(); i++){
		if (g_events_on_route.events_on_route[i] && g_events_on_route.events_on_route[i]->iAltRouteID == iAltRouteID){
			if (current_index == index)
				return g_events_on_route.events_on_route[i];
			else
				current_index++;
		}
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////
void events_on_route_clear(void) {
	int i;
	for (i = 0; i < events_on_route_count(); i++) {
		if (g_events_on_route.events_on_route[i])
			free(g_events_on_route.events_on_route[i]);

		g_events_on_route.events_on_route[i] = NULL;
	}
	g_events_on_route.iCount = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
void events_on_route_clear_record(event_on_route_info *event) {
	event->iAltRouteID = -1;
	event->iAlertId = -1;
	event->iType = -1;
	event->iSubType = -1;
	event->iSeverity = -1;
	event->iStart = -1;
	event->iEnd = -1;
	event->iPrecentage = 0;
	event->positionStart.latitude = -1;
	event->positionStart.longitude = -1;
	event->positionEnd.latitude = -1;
	event->positionEnd.longitude = -1;
	event->sAddOnName[0] = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
void events_on_route_add(event_on_route_info *event) {
	event_on_route_info *new_event;
	roadmap_log(ROADMAP_DEBUG, "events_on_route_add  type=%d, alt_route=%d", event->iType, event->iAltRouteID);

	if (g_events_on_route.iCount >= MAX_ALERTS_ON_ROUTE - 1) {
		roadmap_log(ROADMAP_ERROR, "events_on_route_add - table is full!");
		return;
	}

	new_event = calloc(1, sizeof(event_on_route_info));
	event->iPrecentage = event->iEnd - event->iStart;
	if ((event->iPrecentage) > 100)
	event->iPrecentage = 100;

	(*new_event) = (*event);


	g_events_on_route.events_on_route[g_events_on_route.iCount++] = new_event;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void _draw_bar(SsdWidget widget, RoadMapGuiRect *rect, int flags) {

	static RoadMapImage bar_image = NULL;

	if (!bar_image)
		bar_image = roadmap_res_get(RES_BITMAP, RES_SKIN | RES_NOCACHE, "events_on_route_bar");

	if (bar_image) {
		RoadMapGuiPoint sign_bottom, sign_top;
		RoadMapGuiPoint position;
		sign_top.x = rect->minx + ADJ_SCALE(15);
		sign_top.y = rect->miny;
		position.x = roadmap_canvas_image_width(bar_image) / 2;
		position.y = roadmap_canvas_image_height(bar_image) / 2;
		sign_bottom.x = rect->maxx - ADJ_SCALE(15);
		sign_bottom.y = rect->miny + roadmap_canvas_image_height(bar_image);
		if (flags & SSD_GET_SIZE) {
			rect->maxy = rect->miny + roadmap_canvas_image_height(bar_image) + 5;
			rect->minx = rect->minx + ADJ_SCALE(15);
			rect->maxx = rect->maxx - ADJ_SCALE(15);
			return;
		}
		else {
			roadmap_canvas_draw_image_stretch(bar_image, &sign_top, &sign_bottom, &position, 0,
					IMAGE_NORMAL);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
static void _draw_traffic(SsdWidget widget, RoadMapGuiRect *rect, int flags) {
	event_on_route_info *data = (event_on_route_info *) widget->data;
	static RoadMapImage events_divider;
	static RoadMapImage events_traffic_1 = NULL;
	static RoadMapImage events_traffic_2 = NULL;
	static RoadMapImage events_traffic_3 = NULL;
	RoadMapImage event_traffic;

	int width = widget->parent->cached_size.width - ADJ_SCALE(34);

	if (ssd_widget_rtl(NULL))
		rect->minx -= ADJ_SCALE(17);
	else
		rect->minx += ADJ_SCALE(17);

	if (!events_divider)
		events_divider = roadmap_res_get(RES_BITMAP, RES_SKIN | RES_NOCACHE, "events_divider");

	if (!events_traffic_1)
		events_traffic_1 = roadmap_res_get(RES_BITMAP, RES_SKIN | RES_NOCACHE, "events_traffic_1");
	if (!events_traffic_2)
		events_traffic_2 = roadmap_res_get(RES_BITMAP, RES_SKIN | RES_NOCACHE, "events_traffic_2");

	if (!events_traffic_3)
		events_traffic_3 = roadmap_res_get(RES_BITMAP, RES_SKIN | RES_NOCACHE, "events_traffic_3");

	switch (data->iSeverity) {
	case JAM_TYPE_MODERATE_TRAFFIC:
		event_traffic = events_traffic_1;
		break;
	case JAM_TYPE_HEAVY_TRAFFIC:
		event_traffic = events_traffic_2;
		break;
	case JAM_TYPE_STAND_STILL_TRAFFIC:
		event_traffic = events_traffic_3;
		break;
	default:
		return;
	}

	if (flags & SSD_GET_SIZE) {
		rect->maxx = rect->minx;//.+ 2 ;
		rect->maxy = rect->miny + roadmap_canvas_image_height(events_divider);
	}
	else {
		if ((data->iPrecentage * width / 100) > roadmap_canvas_image_width(events_divider)*2+2) {
			RoadMapGuiPoint position;
			RoadMapGuiPoint sign_top;
			RoadMapGuiPoint sign_bottom;

			if (ssd_widget_rtl(NULL))
				position.x = rect->minx - (width * data->iStart / 100) - roadmap_canvas_image_width(
						events_divider);
			else
				position.x = rect->minx + (width * data->iStart / 100);
			position.y = rect->miny;
			roadmap_canvas_draw_image(events_divider, &position, 0, IMAGE_NORMAL);

			if (ssd_widget_rtl(NULL))
				position.x = rect->minx - (width * data->iStart / 100) - (width * data->iPrecentage / 100)
						+ roadmap_canvas_image_width(events_divider);
			else
				position.x = rect->minx + (width * data->iStart / 100) + (width * data->iPrecentage / 100)
						- 2 * roadmap_canvas_image_width(events_divider);
			position.y = rect->miny;
			roadmap_canvas_draw_image(events_divider, &position, 0, IMAGE_NORMAL);

			if (ssd_widget_rtl(NULL))
				sign_bottom.x = rect->minx - (width * data->iStart / 100) - roadmap_canvas_image_width(
						events_divider);
			else
				sign_top.x = rect->minx + (width * data->iStart / 100) + roadmap_canvas_image_width(
						events_divider);
			sign_top.y = rect->miny;
			position.x = roadmap_canvas_image_width(event_traffic) / 2;
			position.y = roadmap_canvas_image_height(event_traffic) / 2;
			if (ssd_widget_rtl(NULL))
				sign_top.x = rect->minx - (width * data->iStart / 100) - (width * data->iPrecentage
						/ 100) + 2 * roadmap_canvas_image_width(events_divider);
			else
				sign_bottom.x = rect->minx + (width * data->iStart / 100) + (width * data->iPrecentage
						/ 100) - 2 * roadmap_canvas_image_width(events_divider);
			sign_bottom.y = rect->miny + roadmap_canvas_image_height(event_traffic);
			roadmap_canvas_draw_image_stretch(event_traffic, &sign_top, &sign_bottom, &position, 0,
					IMAGE_NORMAL);
		}
	}

}

///////////////////////////////////////////////////////////////////////////////////////////
static void _draw_alert(SsdWidget widget, RoadMapGuiRect *rect, int flags) {
	event_on_route_info *data = (event_on_route_info *) widget->data;
	RoadMapImage image;
	int width = widget->parent->cached_size.width - ADJ_SCALE(38);

	image = roadmap_res_get(RES_BITMAP, RES_SKIN, RTAlerts_Get_Map_Icon_By_Type(data->iType));
	if (!image)
		return;

	if (flags & SSD_GET_SIZE) {
		rect->maxx = rect->minx;
		rect->maxy = rect->miny + roadmap_canvas_image_height(image);
	}
	else {
		RoadMapGuiPoint position;
		if (ssd_widget_rtl(NULL))
			position.x = rect->minx - (width * data->iStart / 100) - roadmap_canvas_image_width(image)
					/ 2 ;
		else
			position.x = rect->minx + (width * data->iStart / 100) - roadmap_canvas_image_width(image)
					/ 2 + ADJ_SCALE(10);
		position.y = rect->miny - ADJ_SCALE(13);
		roadmap_canvas_draw_image(image, &position, 0, IMAGE_NORMAL);
	}

}

///////////////////////////////////////////////////////////////////////////////////////////
static SsdWidget _traffic_widget(event_on_route_info *event) {
	SsdWidget w;
	event_on_route_info *data = (event_on_route_info *) calloc(1, sizeof(*data));
	*data = *event;
	w = ssd_container_new("traffic_widget", "", 1, ADJ_SCALE(10), SSD_ALIGN_VCENTER);
	w->data = data;
	w->draw = _draw_traffic;
	return w;
}

///////////////////////////////////////////////////////////////////////////////////////////
static SsdWidget _alert_widget(event_on_route_info *event) {
	SsdWidget w;
	event_on_route_info *data = (event_on_route_info *) calloc(1, sizeof(*data));
	*data = *event;
	w = ssd_widget_new("alert_widget", NULL, SSD_ALIGN_VCENTER);
	w->data = data;
	w->draw = _draw_alert;
	return w;
}

///////////////////////////////////////////////////////////////////////////////////////////
SsdWidget events_on_route_widget(int altId, int flags) {
	SsdWidget container;
	SsdWidget bar;
	SsdWidget bitmap;
	int i;
	int count = 0;
	int non_traffic_count = 0;
	int height = ADJ_SCALE(50);

	if (!events_on_route_feature_enabled())
		return NULL;
	for (i = 0; i < events_on_route_count_alternative(altId); i++) {
		event_on_route_info *event = events_on_route_at_index_alternative(i, altId);
		if (event->iType != RT_ALERT_TYPE_TRAFFIC_INFO)
			non_traffic_count++;
		count++;
	}

	if (non_traffic_count == 0)
		height = ADJ_SCALE(40);

	container = ssd_container_new("events_container", NULL, SSD_MAX_SIZE, height, SSD_WIDGET_SPACE
			| SSD_END_ROW | flags);
	ssd_widget_set_color(container, NULL, NULL);

	bar = ssd_container_new("events_container", NULL, SSD_MAX_SIZE, ADJ_SCALE(10), SSD_WIDGET_SPACE
			| SSD_END_ROW | SSD_ALIGN_BOTTOM);
	bar->draw = _draw_bar;
	ssd_widget_add(container, bar);

	if (ssd_widget_rtl(NULL)) {
		bitmap = ssd_bitmap_new("Start", "events_arrow_left", 0);
		ssd_widget_set_offset(bitmap, ADJ_SCALE(3), non_traffic_count ? ADJ_SCALE(13) : ADJ_SCALE(3));
	}
	else {
		bitmap = ssd_bitmap_new("Start", "events_arrow_right", 0);
		ssd_widget_set_offset(bitmap, ADJ_SCALE(3), non_traffic_count ? ADJ_SCALE(13) : ADJ_SCALE(3));
	}
	ssd_widget_add(container, bitmap);

	if (ssd_widget_rtl(NULL)) {
		bitmap = ssd_bitmap_new("End", "events_flag_left", SSD_ALIGN_RIGHT);
		ssd_widget_set_offset(bitmap, ADJ_SCALE(3), non_traffic_count ? ADJ_SCALE(10) : ADJ_SCALE(0));
	}
	else {
		bitmap = ssd_bitmap_new("End", "events_flag_right", SSD_ALIGN_RIGHT);
		ssd_widget_set_offset(bitmap, ADJ_SCALE(-3), non_traffic_count ? ADJ_SCALE(10) : ADJ_SCALE(0));
	}
	ssd_widget_add(container, bitmap);

	for (i = 0; i < count; i++) {
		event_on_route_info *event = (event_on_route_info *) events_on_route_at_index_alternative(i, altId);
		if (event->iType != RT_ALERT_TYPE_TRAFFIC_INFO) {
			ssd_widget_add(container, _alert_widget(event));
		}
		else {
			ssd_widget_add(bar, _traffic_widget(event));
		}
	}

	return container;
}
