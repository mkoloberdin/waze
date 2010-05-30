/* roadmap_canvas.c - manage the canvas that is used to draw the map.
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *
 *   Based on an implementation by Pascal F. Martin.
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
 *   See roadmap_canvas.h.
 */

#include <windows.h>

#include "../roadmap.h"
#include "../roadmap_types.h"
#include "../roadmap_gui.h"
#include "../roadmap_canvas.h"
#include "roadmap_wincecanvas.h"
#include "colors.h"

struct roadmap_canvas_pen {	
	struct roadmap_canvas_pen *next;
	char  *name;
	COLORREF color;
	int thinkness;
};


static struct roadmap_canvas_pen *RoadMapPenList = NULL;

static HWND			RoadMapDrawingArea;
static HDC			RoadMapDrawingBuffer;
static RoadMapPen	CurrentPen;
static RECT			ClientRect;
static HPEN			OldHPen = NULL;
static HBITMAP		OldBitmap = NULL;

static void roadmap_canvas_ignore_button (RoadMapGuiPoint *point) {}
static RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonPressed =
	roadmap_canvas_ignore_button;
static RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonReleased =
	roadmap_canvas_ignore_button;
static RoadMapCanvasMouseHandler RoadMapCanvasMouseMove =
	roadmap_canvas_ignore_button;

static void roadmap_canvas_ignore_configure (void) {}

static RoadMapCanvasConfigureHandler RoadMapCanvasConfigure =
	roadmap_canvas_ignore_configure;

static void roadmap_canvas_convert_points (POINT *winpoints,
			RoadMapGuiPoint *points, int count)
{
    RoadMapGuiPoint *end = points + count;
	
    while (points < end) {
        winpoints->x = points->x;
        winpoints->y = points->y;
        winpoints += 1;
        points += 1;
    }
}


void roadmap_canvas_get_text_extents (const char *text, int *width,
			int *ascent, int *descent)
{
	TEXTMETRIC metric;
	LPWSTR text_unicode = ConvertToWideChar(text, CP_UTF8);
	SIZE text_size;
	GetTextExtentPoint(RoadMapDrawingBuffer, text_unicode,
		wcslen(text_unicode), &text_size);
	*width = text_size.cx;
	GetTextMetrics(RoadMapDrawingBuffer, &metric);
	*ascent = metric.tmAscent;
	*descent = metric.tmDescent;
}


void roadmap_canvas_select_pen (RoadMapPen pen)
{
	HPEN old;
	CurrentPen = pen;


	old = SelectObject(RoadMapDrawingBuffer, CreatePen(PS_SOLID,
		CurrentPen->thinkness, CurrentPen->color));  
	
	if (OldHPen == NULL) OldHPen = old;
	else DeleteObject(old);
	
}


RoadMapPen roadmap_canvas_create_pen (const char *name)
{
	struct roadmap_canvas_pen *pen;
	
	for (pen = RoadMapPenList; pen != NULL; pen = pen->next) {
		if (strcmp(pen->name, name) == 0) break;
	}
	
	if (pen == NULL) {
		
		pen = (struct roadmap_canvas_pen *)
			malloc (sizeof(struct roadmap_canvas_pen));
		roadmap_check_allocated(pen);
		
		pen->name = strdup (name);
		pen->color = RGB(0, 0, 0);
		pen->thinkness = 1;
		pen->next = RoadMapPenList;
		
		RoadMapPenList = pen;
	}
	
	roadmap_canvas_select_pen (pen);
	
	return pen;
}


void roadmap_canvas_set_foreground (const char *color)
{
	int high, i, low;
	COLORREF c;
	
	if (*color == '#') {
		int r, g, b;
		sscanf(color, "#%2x%2x%2x", &r, &g, &b);
		c = RGB(r, g, b);
	} else {
		/* Do binary search on color table */
		for (low=(-1), high=sizeof(color_table)/sizeof(color_table[0]);
		high-low > 1;) {
			i = (high+low) / 2;
			if (strcmp(color, color_table[i].name) <= 0) high = i;
			else low = i;
		}
		
		if (!strcmp(color, color_table[high].name)) {
			c = RGB(color_table[high].r, color_table[high].g, color_table[high].b);
		} else {
			c = RGB(0, 0, 0);
		}
	}
	
	CurrentPen->color = c;
	roadmap_canvas_select_pen(CurrentPen);
}


void roadmap_canvas_set_thickness(int thickness)
{
	if (CurrentPen != NULL) {
		CurrentPen->thinkness = thickness;
	}
	roadmap_canvas_select_pen(CurrentPen);
}


void roadmap_canvas_erase (void)
{
	HBRUSH brush = CreateSolidBrush(CurrentPen->color);
	RECT buff_rect = ClientRect;
	if (buff_rect.top != 0) {
		buff_rect.bottom -= buff_rect.top;
		buff_rect.top = 0;
	}
	FillRect(RoadMapDrawingBuffer, &buff_rect, brush);
	DeleteObject(brush);
}


void roadmap_canvas_draw_string (RoadMapGuiPoint *position,
                                 int corner,
                                 const char *text)
{
	int x;
	int y;
	int text_width;
	int text_ascent;
	int text_descent;
	int text_height;
	RECT rect;
	LPWSTR text_unicode;
	
	roadmap_canvas_get_text_extents 
        (text, &text_width, &text_ascent, &text_descent);
	
	text_height = text_ascent + text_descent;
	
	switch (corner) {
		
	case ROADMAP_CANVAS_TOPLEFT:
		y = position->y;
		x = position->x;
		break;
		
	case ROADMAP_CANVAS_TOPRIGHT:
		y = position->y;
		x = position->x - text_width;
		break;
		
	case ROADMAP_CANVAS_BOTTOMRIGHT:
		y = position->y - text_height;
		x = position->x - text_width;
		break;
		
	case ROADMAP_CANVAS_BOTTOMLEFT:
		y = position->y - text_height;
		x = position->x;
		break;
		
	case ROADMAP_CANVAS_CENTER:
		y = position->y - (text_height / 2);
		x = position->x - (text_width / 2);
		break;
		
	default:
		return;
	}
	
	rect.left = x;
	rect.top = y;
	rect.right = x + text_width + 1;
	rect.bottom = y + text_height + 1;
	
	text_unicode = ConvertToWideChar(text, CP_UTF8);
	DrawText(RoadMapDrawingBuffer, text_unicode, wcslen(text_unicode),
		&rect, DT_CENTER);
	free(text_unicode);
}


void roadmap_canvas_draw_multiple_points (int count, RoadMapGuiPoint *points)
{
	int i;
	
	for (i=0; i<count; i++) {
		SetPixel(RoadMapDrawingBuffer, points[i].x, points[i].y,
			CurrentPen->color);
	}
}


void roadmap_canvas_draw_multiple_lines (int count, int *lines,
				RoadMapGuiPoint *points)
{
	int i;
	int count_of_points;

	POINT winpoints[1024];

	HBRUSH oldBrush = SelectObject(RoadMapDrawingBuffer, 
	                     CreateSolidBrush(CurrentPen->color));
	
	for (i = 0; i < count; ++i) {

      RoadMapGuiPoint end_points[2];
      int first = 1;
		
		count_of_points = *lines;
		
      if (count_of_points < 2) continue;

		while (count_of_points > 1024) {

         if (first) {
            first = 0;
            end_points[0] = *points;
         }
			roadmap_canvas_convert_points (winpoints, points, 1024);
			Polyline(RoadMapDrawingBuffer, winpoints, 1024);
			/* We shift by 1023 only, because we must link the lines. */
			points += 1023;
			count_of_points -= 1023;
		}

      if (first) {
         first = 0;
         end_points[0] = *points;
      }

      end_points[1] = points[count_of_points - 1];
		roadmap_canvas_convert_points (winpoints, points, count_of_points);
		Polyline(RoadMapDrawingBuffer, winpoints, count_of_points);

      if (CurrentPen->thinkness > 5) {

         HPEN oldPen = SelectObject(RoadMapDrawingBuffer,
            GetStockObject(NULL_PEN));

         int radius = CurrentPen->thinkness / 2;

         Ellipse(RoadMapDrawingBuffer,
			   end_points[0].x - radius, end_points[0].y - radius,
			   radius + end_points[0].x + 1,
			   radius + end_points[0].y + 1);

         Ellipse(RoadMapDrawingBuffer,
			   end_points[1].x - radius, end_points[1].y - radius,
			   radius + end_points[1].x + 1,
			   radius + end_points[1].y + 1);

         SelectObject(RoadMapDrawingBuffer, oldPen);
	   }
		
		points += count_of_points;
		lines += 1;
	}

   DeleteObject(SelectObject(RoadMapDrawingBuffer, oldBrush));
}


void roadmap_canvas_draw_multiple_polygons (int count, int *polygons,
				RoadMapGuiPoint *points, int filled)
{
	int i;
	int count_of_points;
	POINT winpoints[1024];
	HBRUSH oldBrush;
	
	if (filled) {
		oldBrush = SelectObject(RoadMapDrawingBuffer, 
			CreateSolidBrush(CurrentPen->color));
	} else {
		oldBrush = SelectObject(RoadMapDrawingBuffer,
			GetStockObject(NULL_BRUSH));
	}
	
	for (i = 0; i < count; ++i) {
		
		count_of_points = *polygons;
		
		while (count_of_points > 1024) {
			roadmap_canvas_convert_points (winpoints, points, 1024);
			Polygon(RoadMapDrawingBuffer, winpoints, 1024);
			
			/* We shift by 1023 only, because we must link the lines. */
			points += 1023;
			count_of_points -= 1023;
		}
		
		roadmap_canvas_convert_points (winpoints, points, count_of_points);
		Polygon(RoadMapDrawingBuffer, winpoints, count_of_points);
		polygons += 1;
		points += count_of_points;
	}
	DeleteObject(SelectObject(RoadMapDrawingBuffer, oldBrush));
}


void roadmap_canvas_draw_multiple_circles (int count,
				RoadMapGuiPoint *centers, int *radius, int filled)
{
	int i;
	HBRUSH oldBrush;
	
	if (filled) {
		oldBrush = SelectObject(RoadMapDrawingBuffer, 
			CreateSolidBrush(CurrentPen->color));
	} else {
		oldBrush = SelectObject(RoadMapDrawingBuffer,
						GetStockObject(NULL_BRUSH));
	}
	
	for (i = 0; i < count; ++i) {
		
		int r = radius[i];
		
		int x = centers[i].x - r;
		int y = centers[i].y - r;

      Ellipse(RoadMapDrawingBuffer,
			x, y,
			2 * r + x,
			2 * r + y);
	}
	
	DeleteObject(SelectObject(RoadMapDrawingBuffer, oldBrush));
}


void roadmap_canvas_register_configure_handler (
				RoadMapCanvasConfigureHandler handler)
{
	RoadMapCanvasConfigure = handler;
}


void roadmap_canvas_button_pressed(POINT *data)
{
	RoadMapGuiPoint point;
	
	point.x = (short)data->x;
	point.y = (short)data->y;
	
	(*RoadMapCanvasMouseButtonPressed) (&point);
	
}


void roadmap_canvas_button_released(POINT *data)
{
	RoadMapGuiPoint point;
	
	point.x = (short)data->x;
	point.y = (short)data->y;
	
	(*RoadMapCanvasMouseButtonReleased) (&point);
	
}


void roadmap_canvas_mouse_move(POINT *data)
{
	RoadMapGuiPoint point;
	
	point.x = (short)data->x;
	point.y = (short)data->y;
	
	(*RoadMapCanvasMouseMove) (&point);
	
}


void roadmap_canvas_register_button_pressed_handler (
				RoadMapCanvasMouseHandler handler)
{
	RoadMapCanvasMouseButtonPressed = handler;
}


void roadmap_canvas_register_button_released_handler (
				RoadMapCanvasMouseHandler handler)
{
	RoadMapCanvasMouseButtonReleased = handler;
}


void roadmap_canvas_register_mouse_move_handler (
				RoadMapCanvasMouseHandler handler)
{
	RoadMapCanvasMouseMove = handler;
}


int roadmap_canvas_width (void)
{
	if (RoadMapDrawingArea == NULL) {
		return 0;
	}
	return ClientRect.right - ClientRect.left + 1;
}


int roadmap_canvas_height (void)
{
	if (RoadMapDrawingArea == NULL) {
		return 0;
	}
	
	return ClientRect.bottom - ClientRect.top + 1;
}


void roadmap_canvas_refresh (void)
{
	HDC hdc;
	
	if (RoadMapDrawingArea == NULL) return;
	hdc = GetDC(RoadMapDrawingArea);
	BitBlt(hdc, ClientRect.left, ClientRect.top,
		ClientRect.right - ClientRect.left + 1,
		ClientRect.bottom - ClientRect.top + 1,
	   	RoadMapDrawingBuffer, 0, 0, SRCCOPY); 
	
	DeleteDC(hdc);
}


HWND roadmap_canvas_new (HWND hWnd, HWND tool_bar)
{
	HDC hdc;
	
	if ((RoadMapDrawingBuffer != NULL) && IsWindowVisible(tool_bar)) {
		if (OldHPen != NULL) {
			DeleteObject(OldHPen);
			OldHPen = NULL;
		}
		
		DeleteObject(SelectObject(RoadMapDrawingBuffer, OldBitmap));
		DeleteDC(RoadMapDrawingBuffer);
	}
	
	hdc = GetDC(RoadMapDrawingArea);
	
	RoadMapDrawingArea = hWnd;
	GetClientRect(hWnd, &ClientRect);
	if (tool_bar != NULL) {
		RECT tb_rect;
		GetClientRect(tool_bar, &tb_rect);
		ClientRect.top += tb_rect.bottom;
	}
	
	
	RoadMapDrawingBuffer = CreateCompatibleDC(hdc);
	OldBitmap = SelectObject(RoadMapDrawingBuffer, 
		CreateCompatibleBitmap(hdc, ClientRect.right - ClientRect.left + 1, 
		ClientRect.bottom - ClientRect.top + 1));
	DeleteDC(hdc);
	(*RoadMapCanvasConfigure) ();
	
	return RoadMapDrawingArea;
}


void roadmap_canvas_save_screenshot (const char* filename) {
   /* NOT IMPLEMENTED. */
}


