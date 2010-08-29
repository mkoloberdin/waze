/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

/*
 *  gluUnproject.c
 *  waze
 */

#include "glu.h"

static void __gluMultMatrixVecd(const GLfloat matrix[16], const GLfloat in[4], 
                                GLfloat out[4]) 
{ 
    int i; 
    for( i=0; i<4; ++i ) 
    { 
        out[i] =  
        in[0] * matrix[0*4+i] + 
        in[1] * matrix[1*4+i] + 
        in[2] * matrix[2*4+i] + 
        in[3] * matrix[3*4+i]; 
    } 
} 
/* 
 ** Invert 4x4 matrix. 
 ** Contributed by David Moore (See Mesa bug #6748) 
 */ 
static int __gluInvertMatrixd(const GLfloat m[16], GLfloat invOut[16]) 
{ 
    GLfloat inv[16]; 
    inv[0]    =  m[5]*m[10]*m[15]    - m[5]*m[11]*m[14]    - m[9]*m[6]*m[15] 
	+ m[9]*m[7]*m[14]        + m[13]*m[6]*m[11]    - m[13]*m[7]*m[10]; 
    inv[4]    = - m[4]*m[10]*m[15]    + m[4]*m[11]*m[14]    + m[8]*m[6]*m[15] 
	- m[8]*m[7]*m[14]        - m[12]*m[6]*m[11]    + m[12]*m[7]*m[10]; 
    inv[8]    =  m[4]*m[9]*m[15]        - m[4]*m[11]*m[13]    - m[8]*m[5]*m[15] 
	+ m[8]*m[7]*m[13]        + m[12]*m[5]*m[11]    - m[12]*m[7]*m[9]; 
    inv[12] = - m[4]*m[9]*m[14]        + m[4]*m[10]*m[13]    + m[8]*m[5]*m[14] 
	- m[8]*m[6]*m[13]        - m[12]*m[5]*m[10]    + m[12]*m[6]*m[9]; 
    inv[1]    = - m[1]*m[10]*m[15]    + m[1]*m[11]*m[14]    + m[9]*m[2]*m[15] 
	- m[9]*m[3]*m[14]        - m[13]*m[2]*m[11]    + m[13]*m[3]*m[10]; 
    inv[5]    =  m[0]*m[10]*m[15]    - m[0]*m[11]*m[14]    - m[8]*m[2]*m[15] 
	+ m[8]*m[3]*m[14]        + m[12]*m[2]*m[11]    - m[12]*m[3]*m[10]; 
    inv[9]    = - m[0]*m[9]*m[15]        + m[0]*m[11]*m[13]    + m[8]*m[1]*m[15] 
	- m[8]*m[3]*m[13]        - m[12]*m[1]*m[11]    + m[12]*m[3]*m[9]; 
    inv[13] =  m[0]*m[9]*m[14]        - m[0]*m[10]*m[13]    - m[8]*m[1]*m[14] 
	+ m[8]*m[2]*m[13]        + m[12]*m[1]*m[10]    - m[12]*m[2]*m[9]; 
    inv[2]    =  m[1]*m[6]*m[15]        - m[1]*m[7]*m[14]    - m[5]*m[2]*m[15] 
	+ m[5]*m[3]*m[14]        + m[13]*m[2]*m[7]    - m[13]*m[3]*m[6]; 
    inv[6]    = - m[0]*m[6]*m[15]        + m[0]*m[7]*m[14]    + m[4]*m[2]*m[15] 
	- m[4]*m[3]*m[14]        - m[12]*m[2]*m[7]    + m[12]*m[3]*m[6]; 
    inv[10] =  m[0]*m[5]*m[15]        - m[0]*m[7]*m[13]    - m[4]*m[1]*m[15] 
	+ m[4]*m[3]*m[13]        + m[12]*m[1]*m[7]    - m[12]*m[3]*m[5]; 
    inv[14] = - m[0]*m[5]*m[14]        + m[0]*m[6]*m[13]    + m[4]*m[1]*m[14] 
	- m[4]*m[2]*m[13]        - m[12]*m[1]*m[6]    + m[12]*m[2]*m[5]; 
    inv[3]    = - m[1]*m[6]*m[11]        + m[1]*m[7]*m[10]    + m[5]*m[2]*m[11] 
	- m[5]*m[3]*m[10]        - m[9]*m[2]*m[7]    + m[9]*m[3]*m[6]; 
    inv[7]    =  m[0]*m[6]*m[11]        - m[0]*m[7]*m[10]    - m[4]*m[2]*m[11] 
	+ m[4]*m[3]*m[10]        + m[8]*m[2]*m[7]    - m[8]*m[3]*m[6]; 
    inv[11] = - m[0]*m[5]*m[11]        + m[0]*m[7]*m[9]    + m[4]*m[1]*m[11] 
	- m[4]*m[3]*m[9]        - m[8]*m[1]*m[7]    + m[8]*m[3]*m[5]; 
    inv[15] =  m[0]*m[5]*m[10]        - m[0]*m[6]*m[9]    - m[4]*m[1]*m[10] 
	+ m[4]*m[2]*m[9]        + m[8]*m[1]*m[6]    - m[8]*m[2]*m[5]; 
    GLfloat det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12]; 
    if( det == 0 ) 
    { 
        return GL_FALSE; 
    } 
    det = 1.0 / det; 
    for( int i=0; i<16; ++i ) 
    { 
        invOut[i] = inv[i] * det; 
    } 
    return GL_TRUE; 
} 
static void __gluMultMatricesd(const GLfloat a[16], const GLfloat b[16], 
							   GLfloat r[16]) 
{ 
    int i, j; 
    for( i=0; i<4; ++i )  
    { 
        for( j=0; j<4; ++j )  
        { 
            r[i*4+j] =  
            a[i*4+0]*b[0*4+j] + 
            a[i*4+1]*b[1*4+j] + 
            a[i*4+2]*b[2*4+j] + 
            a[i*4+3]*b[3*4+j]; 
        } 
    } 
}
GLAPI GLint GLAPIENTRY gluUnProjectf(GLfloat winx, GLfloat winy, GLfloat winz, 
				  const GLfloat modelMatrix[16],  
				  const GLfloat projMatrix[16], 
				  const GLint viewport[4], 
				  GLfloat *objx, GLfloat *objy, GLfloat *objz) 
{ 
    GLfloat finalMatrix[16]; 
    GLfloat in[4]; 
    GLfloat out[4]; 
    __gluMultMatricesd( modelMatrix, projMatrix, finalMatrix ); 
    if( !__gluInvertMatrixd( finalMatrix, finalMatrix ) )  
    { 
        return GL_FALSE; 
    } 
    in[0] = winx; 
    in[1] = winy; 
    in[2] = winz; 
    in[3] = 1.0; 
    /* Map x and y from window coordinates */ 
    in[0] = ( in[0] - viewport[0] ) / viewport[2]; 
    in[1] = ( in[1] - viewport[1] ) / viewport[3]; 
    /* Map to range -1 to 1 */ 
    in[0] = in[0] * 2 - 1; 
    in[1] = in[1] * 2 - 1; 
    in[2] = in[2] * 2 - 1; 
    __gluMultMatrixVecd(finalMatrix, in, out); 
    if( out[3] == 0.0 ) 
    { 
        return GL_FALSE; 
    } 
    out[0] /= out[3]; 
    out[1] /= out[3]; 
    out[2] /= out[3]; 
    *objx = out[0]; 
    *objy = out[1]; 
    *objz = out[2];
	
    return GL_TRUE; 
} 