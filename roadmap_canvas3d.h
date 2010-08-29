/* roadmap_canvas3d.h - 3D OpenGL
 *
 * LICENSE:
 *
 *   Copyright 2010 Tomer
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
 */

#ifndef ROADMAP_CANVAS3D_H_
#define ROADMAP_CANVAS3D_H_

#include "roadmap_gui.h" // RoadMapGuiPoint

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus
// non 3D functions- added missing prototypes
void roadmap_canvas_prepare();
// init function- invoke once from configureCb
void roadmap_canvas3_ogl_prepare();
// increase/decrease camera viewing angle
double roadmap_canvas_ogl_rotateMe(float ang_diff);
// update camera viewing angle according zoom
void roadmap_canvas3_ogl_updateScale(int zoom);
// set 3D mode
void roadmap_canvas3_set3DMode(int is3D);
// project 3D model to 2D screen
void roadmap_canvas3_project(RoadMapGuiPoint *point);
// unproject 2D screen to 3D model
void roadmap_canvas3_unproject(RoadMapGuiPoint *point);
   
float roadmap_canvas3_get_angle(void);

/*
 * change the 3D camera angle and position
 * currently omitted cause camera is fixed
void roadmap_canvas_ogl_rotateMe(float ang_diff);
void roadmap_canvas_ogl_moveMeFlat(int i);
void roadmap_canvas_ogl_slideMeFlat(int i);
*/
#ifdef __cplusplus
}
#endif// __cplusplus


#endif /* ROADMAP_CANVAS3D_H_ */
