/*
 * roadmap_canvas3d.c
 *
 *  Created on: Jul 20, 2010
 *      Author: tomer
 */

#include <math.h> // M_PI
#include <stdlib.h> // malloc/free
#include "roadmap.h" // TRUE/FALSE, roadmap_log
#ifndef OPENGL
#error "Platform not recognized"
#endif// OPENGL

#ifdef GTK2_OGL
#include <GL/glu.h>
#else
#include "glu/glu.h"
#endif// GTK2_OGL

#include "roadmap_canvas3d.h"
#include "roadmap_canvas.h"
#include "roadmap_canvas_ogl.h" // check_gl_error()
#include "roadmap_math.h" //roadmap_math_get_context

#include "roadmap_glmatrix.h"

#ifdef  _WIN32 // should be defined in <math.h>
#define M_PI 3.1417
#endif

// defined also in ogl/roadmap_canvas.c
#define Z_LEVEL	-6

static int isInitialized= FALSE;
static int is3DMode;
static RMGlMatrixType modelMat3D[16];
static RMGlMatrixType projMat3D[16];

static GLint viewport3D[4];
static GLdouble *projDepthVec;
static const GLdouble z_level= Z_LEVEL;


// control camera movement
static float angle=M_PI/3.0,ratio;
static float x=0.0f,y=0.0f,z=-50.0f;
static float lx=0.0f,ly=0.0f,lz=1.0f;
static GLdouble max_model_coord= 150000;
//static GLint snowman_display_list;


#define is_canvas_ready()  ( \
	roadmap_canvas_is_ready() ? TRUE : (roadmap_log( ROADMAP_MESSAGE_WARNING, __FILE__, __LINE__,  \
            "Canvas module 3D is not ready!!! Can not call OGL functions!" ), FALSE  ))

static void orientMe(float ang) {

	ly = -sinf(ang);
	lz = cosf(ang);
	roadmap_glmatrix_identity();
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
   roadmap_glmatrix_load_matrix( _gl_matrix_mode_modelview, modelMat3D );
   roadmap_glmatrix_load_matrix( _gl_matrix_mode_projection, projMat3D );

	glGetIntegerv(GL_VIEWPORT,viewport3D);
	if ( viewport3D[2] == 0 || viewport3D[3] == 0 )
	{
	   viewport3D[2] = roadmap_canvas_width();
	   viewport3D[3] = roadmap_canvas_height();
	}
	project(objX,objY,objZ,winX,winY,winZ);

}

static double calcProjDepths(GLint w,GLint h) {
	GLdouble x,y, wy, base_step, distH;
	GLint prev_iwy, repeat_cnt = 0, iter, step, nskipped;
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
         //roadmap_log(ROADMAP_DEBUG,"\n%lg:",y);
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
      //roadmap_log(ROADMAP_DEBUG," %lg",wy);
	}
	distH= h-y;

	return distH;
}

static double setCameraAndOrient(float ang,int w, int h, int isUpdated) {
	GLdouble wx,wy,wz;
	double cameraDist;
	double y_c;
	double distH;
	static double ang_0= M_PI/3.0;
	static double y_v0= 1.6;
	static double z_v0= .8;
	int iter= 0;

	if (!isUpdated) {
		orientMe(ang);
		return 0;
	}
	cameraDist= z_v0/cos(ang_0);
	y_c= y_v0-z_v0*tan(ang_0);
	y= (y_c+cameraDist*sinf(angle))*h;
	z= -cameraDist*cosf(angle)*h;
	x= w/2;
	orientMe(ang);
	readMatsProject(x,h,z_level,&wx,&wy,&wz);
	while ( fabs(wy-h)> 1 ) {
//		if (iter>2)
//			printf ("different wy= %g y= %g\n",wy,y);
		y+= (wy-h);
		orientMe(ang);
		readMatsProject(x,h,z_level,&wx,&wy,&wz);
		++iter;
	}
	distH= calcProjDepths(w,h);

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
   roadmap_glmatrix_mode( _gl_matrix_mode_projection );
   roadmap_glmatrix_identity();

   roadmap_glmatrix_mode( _gl_matrix_mode_modelview );
   roadmap_glmatrix_identity();

#ifdef GTK2_OGL
	glOrtho( 0.0f, w,   // left,right
			h, 0.0f, // bottom, top
			-100.0f, 100.0f ); // near, far
#else
	glOrthof( 0.0f, w,   // left,right
			h, 0.0f, // bottom, top
			-100.0f, 100.0f ); // near, far
#endif// GTK2_OGL
	//glDisable(GL_DEPTH_TEST); //disable the depth testing
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

	double distH;

	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
	if(h == 0)
		h = 1;

	ratio = 1.0f * w / h;
	// Reset the coordinate system before modifying
   roadmap_glmatrix_mode( _gl_matrix_mode_projection );
   roadmap_glmatrix_identity();


	// Set the viewport to be the entire window
   glViewport(0, 0, w, h);

	// Set the clipping volume
	gluPerspective(45,ratio,1,7500);
   roadmap_glmatrix_mode( _gl_matrix_mode_modelview );
   roadmap_glmatrix_identity();

	//glDisable(GL_DEPTH_TEST); //disable the depth testing
	distH= setCameraAndOrient(angle,w,h,isUpdated);

	return distH;
	//gluLookAt(x, y, z,
	//	      x + lx,y + ly,z + lz,
	//		  0.0f,-1.0f,-1.0f);


}

double roadmap_canvas_ogl_rotateMe(float ang_diff) {
    double distH;

   if ( !is_canvas_ready() )
      return 0;

	angle+= ang_diff;
    distH= set3D(roadmap_canvas_width(),
			roadmap_canvas_height(),1);
    if (!is3DMode) {
		set2D(roadmap_canvas_width(),
				roadmap_canvas_height());
    }

	return distH;
}

void roadmap_canvas3_ogl_updateScale(zoom_t zoom) {
	float angRad;
	//static float maxAngle= 67.3;
   static float maxAngle= 62;
	float ang= 24;
	//float angZoom= 384;
   float angZoom= 3000;

   if (!isInitialized || zoom== 0)
      return;
   if ( !is_canvas_ready() )
      return;

   zoom = zoom * 100 / roadmap_screen_get_screen_scale();

	while (angZoom > zoom) {
		angZoom/= 1.1;
		ang= ang* 1.026; // (5/4)^(ln(1.1)/ln(2))
	}
	if (ang > maxAngle)
		ang= maxAngle;
	angRad= ang*M_PI/180;
	if (angRad!= angle) {
		double distH;
		angle= angRad;
		distH= roadmap_canvas_ogl_rotateMe(0);
		roadmap_log(ROADMAP_DEBUG,"zoom= %d angle= %lf y dist= %lg\n",(int)zoom,ang,distH);
	}
}

void roadmap_canvas3_set3DMode(int is3D) {
	if (is3D== is3DMode)
		return;
   if ( !is_canvas_ready() )
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

void roadmap_canvas3_glmatrix_enable( void ) {
   roadmap_glmatrix_enable();
}

void roadmap_canvas3_ogl_prepare() {
   
   if (!isInitialized) {
		isInitialized= TRUE;
   }
	if ( !is_canvas_ready() )
	   return;

   roadmap_canvas3_ogl_updateScale(roadmap_math_get_zoom());
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
   
   if (point->y < 0 ||
       point->y > viewport3D[3])
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

    point->x= ox;
   	point->y= oy;
}

float roadmap_canvas3_get_angle(void) {
   return angle;
}
