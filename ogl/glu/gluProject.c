/*
 *  gluProject.c
 *  waze
 *
 *  Created by Ehud Shabtai on 7/26/10.
 *  Copyright 2010 waze. All rights reserved.
 *
 */

#include "glu.h"

/* M. Buffat 17/2/95 */

/*
 * Transform a point (column vector) by a 4x4 matrix.  I.e.  out = m * in
 * Input:  m - the 4x4 matrix
 *         in - the 4x1 vector
 * Output:  out - the resulting 4x1 vector.
 */
static void
transform_point(GLfloat out[4], const GLfloat m[16], const GLfloat in[4])
{
#define M(row,col)  m[col*4+row]
	out[0] = M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0, 3) * in[3];
	out[1] = M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1, 3) * in[3];
	out[2] = M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2, 3) * in[3];
	out[3] = M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3, 3) * in[3];
#undef M
}

/* projection du point (objx,objy,obz) sur l'ecran (winx,winy,winz) */

GLint GLAPIENTRY gluProjectf(GLfloat objx, GLfloat objy, GLfloat objz,
							const GLfloat model[16], const GLfloat proj[16],
							const GLint viewport[4],
							GLfloat * winx, GLfloat * winy, GLfloat * winz)
{
	/* matrice de transformation */
	GLfloat in[4], out[4];

	/* initilise la matrice et le vecteur a transformer */
	in[0] = objx;
	in[1] = objy;
	in[2] = objz;
	in[3] = 1.0f;
	transform_point(out, model, in);
	transform_point(in, proj, out);
	
	/* d'ou le resultat normalise entre -1 et 1 */
	if (in[3] == 0.0f)
		return GL_FALSE;
	
	in[0] /= in[3];
	in[1] /= in[3];
	in[2] /= in[3];
	
	/* en coordonnees ecran */
	*winx = viewport[0] + (1.0f + in[0]) * viewport[2] / 2.0f;
	*winy = viewport[1] + (1.0f + in[1]) * viewport[3] / 2.0f;
	/* entre 0 et 1 suivant z */
	*winz = (1.0f + in[2]) / 2.0f;
	return GL_TRUE;
}
