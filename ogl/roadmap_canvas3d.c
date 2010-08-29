/* roadmap_canvas3d.c - 3D OpenGL
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
 * SYNOPSYS:
 *
 *   See roadmap_canvas3d.h.
 */

#include <math.h> // M_PI
#include <stdlib.h> // malloc/free
#include "roadmap.h" // TRUE/FALSE, roadmap_log
#ifdef OPENGL
#ifdef GTK2_OGL
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include "glu/glu.h"
#endif// GTK2_OGL
#else
#error "Platform not recognized"
#endif// OPENGL
#include "roadmap_canvas3d.h"
#include "roadmap_canvas.h"
#include "roadmap_canvas_ogl.h" // check_gl_error()
#include "roadmap_math.h" //roadmap_math_get_context

// defined also in ogl/roadmap_canvas.c
#define Z_LEVEL	-6

static int isInitialized= FALSE;
static int is3DMode;
#ifdef GTK2_OGL
static GLdouble modelMat3D[16];
static GLdouble projMat3D[16];
#else
static GLfloat modelMat3D[16];
static GLfloat projMat3D[16];
#endif// GTK2_OGL
static GLint viewport3D[4];
static GLdouble *projDepthVec;
static const GLdouble z_level= Z_LEVEL;


// control camera movement
static float angle=M_PI/3.0,ratio;
static float x=0.0f,y=0.0f,z=-50.0f;
static float lx=0.0f,ly=0.0f,lz=1.0f;
static GLdouble max_model_coord= 150000;
//static GLint snowman_display_list;

static void orientMe(float ang) {

	ly = -sin(ang);
	lz = cos(ang);
	glLoadIdentity();
	gluLookAt(x, y, z,
		      x + lx,y + ly,z + lz,
			  0.0f,-1.0f,-1.0f);
}

/*
static void moveMeFlat(int i) {
	x = x + i*(lx)*5;
	z = z + i*(lz)*5;
	glLoadIdentity();
	gluLookAt(x, y, z,
		      x + lx,y + ly,z + lz,
			  0.0f,-1.0f,-1.0f);
}

static void slideMeFlat(int i) {

	y = y+i*5;
	glLoadIdentity();
	gluLookAt(x, y, z,
		      x + lx,y + ly,z + lz,
			  0.0f,-1.0f,-1.0f);
}
*/

static void project(GLdouble objX,GLdouble objY,GLdouble objZ,
		GLdouble *winX, GLdouble *winY, GLdouble *winZ) {

#ifdef GTK2_OGL
	gluProject(objX,	//objX,
	 	objY, //objY,
	 	objZ, //objZ,
	 	modelMat3D, //model,
	 	projMat3D, //proj,
	 	viewport3D,    //view,
	 	winX,  	//winX,
	 	winY, 	//winY,
	 	winZ);  	//winZ);
#else
	GLfloat winXf,winYf,winZf;
	gluProjectf(objX,	//objX,
			   objY, //objY,
			   objZ, //objZ,
			   modelMat3D, //model,
			   projMat3D, //proj,
			   viewport3D,    //view,
			   &winXf,  	//winX,
			   &winYf, 	//winY,
			   &winZf);  	//winZ);
	*winX= winXf;
	*winY= winYf;
	*winZ= winZf;
#endif// GTK2_OGL
	/*
	 * Invert Y coordinate- because
	 *
http://lwjgl.org/forum/index.php?action=printpage;topic=3050.0
It's usually important to note that OpenGL's coordinate system has its origin in the bottom left corner (which is the mathematical standard). Some libraries like Slick (and I think Java's imaging classes) put the origin in the top left corner and invert the y coordinate. Not totally sure why, though. I always wondered what the rationale might be to invert the y axis. Maybe it's due to regional tradition. Anyway, gluOrtho2D(0, resolutionWidth, resolutionHeight, 0);  also sets up a top left origin. gluOrtho2D(0, resolutionWidth, 0, resolutionHeight) sets up a bottom left origin.
That's a source of confusion and takes some extra work to get code from one representation to the other. Just so you know, in case you wonder why objects or textures suddenly appear upside down.
   	 */
   	*winY= viewport3D[3]- (*winY);

}

static void readMatsProject(GLdouble objX,GLdouble objY,GLdouble objZ,
		GLdouble *winX, GLdouble *winY, GLdouble *winZ) {
#ifdef GTK2_OGL
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMat3D);
	glGetDoublev(GL_PROJECTION_MATRIX, projMat3D);
#else
	glGetFloatv(GL_MODELVIEW_MATRIX, modelMat3D);
	glGetFloatv(GL_PROJECTION_MATRIX, projMat3D);
#endif// GTK2_OGL
	glGetIntegerv(GL_VIEWPORT,viewport3D);
	project(objX,objY,objZ,winX,winY,winZ);

}

static double calcProjDepths(GLint w,GLint h) {
	GLdouble x,y, wy, base_step, distH;
	GLint prev_iwy, repeat_cnt, iter, step, nskipped;
	if (projDepthVec)
		free(projDepthVec);
	projDepthVec= (GLdouble *)calloc(h+1,sizeof(GLdouble));

	x= w/2;

	base_step= .25;
	y=h+1;
	step= 1;
	prev_iwy= h;
	for (nskipped= iter= 0; y> -max_model_coord; y-=step*base_step) {
		GLdouble wx,wz;
		GLint iwy;
		if (iter% 20== 0) {
			if (nskipped> 0) {
				roadmap_log(ROADMAP_ERROR,"calcProjDepths() y=%lg wy=%lg missing %d",y,wy,nskipped);
				nskipped= 0;
			}
			//printf("\n%lg:",y);
		}
		++iter;
		project(x,y,z_level,&wx,&wy,&wz);
		iwy= floor(wy);
		if (iwy> h)
			continue;
		if (iwy!= prev_iwy) {
			if (wy< prev_iwy-1) {
				// skipped window y
				//printf(".");
				++nskipped;
			}
			prev_iwy= iwy;
			repeat_cnt= 0;
		}
		else {
			++repeat_cnt;
			if (repeat_cnt> 1)
				step++;
		}
		if (iwy< 0)
			break;
		projDepthVec[iwy]= wz;
		//printf (" %lg",wy);
	}
	distH= h-y;

	return distH;
}

static double setCameraAndOrient(float ang,int w, int h, int isUpdated) {
	GLdouble wx,wy,wz;
	static double ang_0= M_PI/3.0;
	static double y_v0= 1.6;
	static double z_v0= .8;

	if (!isUpdated) {
		orientMe(ang);
		return 0;
	}
	double cameraDist= z_v0/cos(ang_0);
	double y_c= y_v0-z_v0*tan(ang_0);
	y= (y_c+cameraDist*sin(angle))*h;
	z= -cameraDist*cos(angle)*h;
	x= w/2;
	orientMe(ang);
	readMatsProject(x,h,z_level,&wx,&wy,&wz);
	int iter= 0;
	while (fabs(wy-h)> 1) {
		if (iter>2)
			printf ("different wy= %g y= %g\n",wy,y);
		y+= (wy-h);
		orientMe(ang);
		readMatsProject(x,h,z_level,&wx,&wy,&wz);
		++iter;
	}
	double distH= calcProjDepths(w,h);

	return distH;
}

static void set2D(int w, int h) {

	/*
	ly = -sin(0);
	lz = cos(0);
	glLoadIdentity();
	gluLookAt(xc, yc, zc,
		      xc + lx,yc + ly,zc + lz,
			  0.0f,-1.0f,-1.0f);
	*/
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#ifdef GTK2_OGL
	glOrtho( 0.0f, w,   // left,right
			h, 0.0f, // bottom, top
			-100.0f, 100.0f ); // near, far
#else
	glOrthof( 0.0f, w,   // left,right
			h, 0.0f, // bottom, top
			-100.0f, 100.0f ); // near, far
#endif// GTK2_OGL
	glDisable(GL_DEPTH_TEST); //disable the depth testing
}

/*
 * change the 3D camera angle and position
 * currently omitted because camera is fixed
void roadmap_canvas_ogl_rotateMe(float ang_diff) {
	roadmap_canvas_ogl_begin(0);
	angle += ang_diff;
	orientMe(angle);
	roadmap_canvas_ogl_end();
}
void roadmap_canvas_ogl_moveMeFlat(int i) {
	roadmap_canvas_ogl_begin(0);
	moveMeFlat(i);
	roadmap_canvas_ogl_end();
}
void roadmap_canvas_ogl_slideMeFlat(int i) {
	roadmap_canvas_ogl_begin(0);
	slideMeFlat(i);
	roadmap_canvas_ogl_end();
}
*/

static double set3D(int w, int h, int isUpdated)
{

	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
	if(h == 0)
		h = 1;

	ratio = 1.0f * w / h;
	// Reset the coordinate system before modifying
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Set the viewport to be the entire window
    glViewport(0, 0, w, h);

	// Set the clipping volume
	gluPerspective(45,ratio,1,7500);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST); //disable the depth testing
	double distH= setCameraAndOrient(angle,w,h,isUpdated);

	return distH;
	//gluLookAt(x, y, z,
	//	      x + lx,y + ly,z + lz,
	//		  0.0f,-1.0f,-1.0f);


}

double roadmap_canvas_ogl_rotateMe(float ang_diff) {
	angle+= ang_diff;
    double distH= set3D(roadmap_canvas_width(),
			roadmap_canvas_height(),1);
    if (!is3DMode) {
		set2D(roadmap_canvas_width(),
				roadmap_canvas_height());
    }

	return distH;
}

void roadmap_canvas3_ogl_updateScale(int zoom) {
	//static float maxAngle= 67.3;
   static float maxAngle= 62;
	if (!isInitialized || zoom== 0)
		return;
	float ang= 24;
	//float angZoom= 384;
   float angZoom= 3000;
	while (angZoom > zoom) {
		angZoom/= 1.1;
		ang= ang* 1.026; // (5/4)^(ln(1.1)/ln(2))
	}
	if (ang > maxAngle)
		ang= maxAngle;
	float angRad= ang*M_PI/180;
	if (angRad!= angle) {
		angle= angRad;
		double distH= roadmap_canvas_ogl_rotateMe(0);
		roadmap_log(ROADMAP_DEBUG,"zoom= %d angle= %lf y dist= %lg\n",zoom,ang,distH);
	}
}

void roadmap_canvas3_set3DMode(int is3D) {
	if (is3D== is3DMode)
		return;
	if (is3D) {
		set3D(roadmap_canvas_width(),
				roadmap_canvas_height(),0);
	}
	else {
		set2D(roadmap_canvas_width(),
				roadmap_canvas_height());
	}
	is3DMode= is3D;
}

void roadmap_canvas3_ogl_prepare() {
	if (!isInitialized) {
		isInitialized= TRUE;
   }
   
   RoadMapPosition dummypos;
   int zoom;
   roadmap_math_get_context(&dummypos,&zoom);
   roadmap_canvas3_ogl_updateScale(zoom);
   roadmap_canvas_ogl_rotateMe(0);
}


void roadmap_canvas3_project(RoadMapGuiPoint *point) {
    GLdouble x, y, wx, wy, wz;
    x= point->x;
    y= point->y;
    project(x,y,z_level,&wx,&wy,&wz);
    point->x= floor(wx);
    point->y= floor(wy);
}

void roadmap_canvas3_unproject(RoadMapGuiPoint *point) {
	GLint iy;
	GLdouble x, y, z;
#ifdef GTK2_OGL
	GLdouble ox, oy, oz;
#else
	GLfloat ox, oy, oz;
#endif// GTK2_OGL
   
   if (point->y < -viewport3D[3] ||
       point->y >= viewport3D[3])
      return;

    x= point->x;
    y= viewport3D[3]- point->y;
	/*        
	 * Invert Y coordinate-
	 * see comment in roadmap_canvas3_project above
	 */
    iy= point->y;
    z= projDepthVec[iy];

/*
    glGetDoublev(GL_DEPTH_RANGE,depthRange);
    check_gl_error()
    glReadPixels (x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &zh);
    check_gl_error()
    z= zh;
*/

#ifdef GTK2_OGL
	gluUnProject(
	    x, //winX,
	    y, //winY,
	    z, //winZ,
	    modelMat3D, //model,
	    projMat3D, //proj,
	    viewport3D, //view,
	    &ox, //objX,
	    &oy, //objY,
	    &oz); //objZ)
#else
	gluUnProjectf(
				  x, //winX,
				  y, //winY,
				  z, //winZ,
				  modelMat3D, //model,
				  projMat3D, //proj,
				  viewport3D, //view,
				  &ox, //objX,
				  &oy, //objY,
				  &oz); //objZ)
#endif// GTK2_OGL	
    check_gl_error()

    point->x= ox;
   	point->y= oy;
}

float roadmap_canvas3_get_angle(void) {
   return angle;
}