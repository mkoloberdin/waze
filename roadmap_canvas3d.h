/*
 * roadmap_canvas3d.h
 *
 *  Created on: Jul 20, 2010
 *      Author: tomer
 */

#ifndef ROADMAP_CANVAS3D_H_
#define ROADMAP_CANVAS3D_H_

#include "roadmap_gui.h" // RoadMapGuiPoint
#include "roadmap_math.h" //zoom_t

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
void roadmap_canvas3_ogl_updateScale(zoom_t zoom);
// set 3D mode
void roadmap_canvas3_set3DMode(int is3D);
// project 3D model to 2D screen
void roadmap_canvas3_project(RoadMapGuiPoint *point);
// unproject 2D screen to 3D model
void roadmap_canvas3_unproject(RoadMapGuiPoint *point);
// Enable tracking of the transformation matrix
void roadmap_canvas3_glmatrix_enable( void );

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
