/*
 *  gluPerspective.c
 *  waze
 *
 *  Created by Ehud Shabtai on 7/26/10.
 *  Copyright 2010 waze. All rights reserved.
 *
 */
#include <math.h>
#include "glu.h"
#include "roadmap_glmatrix.h"

static void __gluMakeIdentityf(GLfloat m[16])
{
	m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
	m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
	m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
	m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
}

void GLAPIENTRY gluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
{
	GLfloat m[4][4];
	GLfloat sine, cotangent, deltaZ;
	GLfloat radians = fovy / 2 * 3.14 / 180;
	deltaZ = zFar - zNear;
	sine = sin(radians);
	if ((deltaZ == 0) || (sine == 0) || (aspect == 0))
	{
		return;
	}
	cotangent = cos(radians) / sine;
	
	__gluMakeIdentityf(&m[0][0]);
	m[0][0] = cotangent / aspect;
	m[1][1] = cotangent;
	m[2][2] = -(zFar + zNear) / deltaZ;
	m[2][3] = -1;
	m[3][2] = -2 * zNear * zFar / deltaZ;
	m[3][3] = 0;

	roadmap_glmatrix_mult( &m[0][0] );
}
