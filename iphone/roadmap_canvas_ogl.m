/* roadmap_canvas_ogl.m - manage the canvas that is used to draw the map.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2008 Morten Bek Ditlevsen
 *   Copyright 2008 Avi R.
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

#define ROADMAP_CANVAS_OGL_C

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIView.h>
#import <Foundation/NSString.h>



#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_device_events.h"

#include "roadmap_canvas.h"
#include "roadmap_canvas3d.h"
#include "roadmap_canvas_ogl.h"
#include "roadmap_iphonecanvas.h"

#include "roadmap_path.h"
#include "roadmap_lang.h"
#include "iphone/roadmap_libpng.h"
#include "roadmap_wchar.h"

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

static BOOL                gs_bShouldAcceptLayout = TRUE;
static BOOL                needsResize = FALSE;

static RoadMapCanvasView   *RoadMapDrawingArea;
static EAGLContext         *RoadMapGc;

/* OpenGL names for the renderbuffer and framebuffers used to render to this view */
static GLuint viewRenderbuffer, viewFramebuffer, depthRenderbuffer;

// The pixel dimensions of the CAEAGLLayer
GLint backingWidth;
GLint backingHeight;

static CAEAGLLayer *RoadMapEAGLLayer;

static BOOL       skipDraw = FALSE;


static NSMutableArray *CordingPoints; //Number of cording fingers. We actually use max of 3

//extern int next_pot(int num);
/*
#if !defined(INLINE_DEC)
#define INLINE_DEC
#endif

INLINE_DEC int ogl_next_pot(int num) {
   int outNum = num;
   
   --outNum;
   outNum |= outNum >> 1;
   outNum |= outNum >> 2;
   outNum |= outNum >> 4;
   outNum |= outNum >> 8;
   outNum |= outNum >> 16;
   outNum++;
   
#ifdef OPENGL_DEBUG
   if ( outNum > 2049 )
   {
	   roadmap_log( ROADMAP_FATAL, "Allocation %d", outNum );
   }
#endif //OPENGL_DEBUG
   
   return outNum;
};
*/

/*************************************************************************************************
 * roadmap_canvas_ogl_to_wchar
 * Converts multibyte to the wchar
 */
int roadmap_canvas_ogl_to_wchar (const char *text, wchar_t *output, int size)
{
   int length = mbstowcs( ( RM_WCHAR_T *) output, text, size - 1);
   output[length] = 0;
   return length;
}


RoadMapImage roadmap_canvas_ogl_load_bmp (const char *full_name)
{
	roadmap_log( ROADMAP_ERROR, "Cannot load bmp image %s", full_name );
	return NULL;
}



RoadMapImage roadmap_canvas_ogl_load_png (const char *full_name) {
   
   int width;
   int height;
   int img_w;
   int img_h;
   int stride;
   
   RoadMapImage image;
   
   
   unsigned char *buf = read_png_file(full_name, &img_w, &img_h, &stride);
   
   roadmap_log( ROADMAP_INFO, "Loading file : %s\n", full_name );
   
   if (!buf) return NULL;
   
   //create texture
   image = malloc(sizeof(roadmap_canvas_image));
   roadmap_check_allocated (image);
   
   image->width = img_w;
   image->height = img_h;
   
   width = next_pot(img_w);
   height = next_pot(img_h);
   
   GLubyte* temp_buf= malloc(4 * width * height);
   roadmap_check_allocated(temp_buf);
   
   for(int j = 0; j < height; j++) {
      for(int i = 0; i < width; i++){
         temp_buf[4*(i+j*width)]     = (i >= img_w || j >= img_h) ? 0 : (int)buf[4*(i+j*img_w)   ];
         temp_buf[4*(i+j*width) +1]  = (i >= img_w || j >= img_h) ? 0 : (int)buf[4*(i+j*img_w) +1];
         temp_buf[4*(i+j*width) +2]  = (i >= img_w || j >= img_h) ? 0 : (int)buf[4*(i+j*img_w) +2];
         temp_buf[4*(i+j*width) +3]  = (i >= img_w || j >= img_h) ? 0 : buf[4*(i+j*img_w) +3];
      }
   }
   
   free (buf);
   
   image->buf = temp_buf;   
   
   return image;
}


int roadmap_canvas_width (void) {
   
   return backingWidth;
}

int roadmap_canvas_agg_is_landscape()
{
   
   return ( roadmap_canvas_width() > roadmap_canvas_height() );
}

int roadmap_canvas_height (void) {
   return backingHeight;
}

void roadmap_canvas_refresh (void) {
   //if (needsResize)
   //   [RoadMapDrawingArea resize];
   
   [RoadMapDrawingArea drawView];
   [RoadMapDrawingArea setupView];
}



void roadmap_canvas_get_cording_pt (RoadMapGuiPoint points[MAX_CORDING_POINTS])
{
	int i;
	UITouch *touch;
	
	for (i = 0; i < MAX_CORDING_POINTS; ++i) {
		if ([CordingPoints count] > i) {
			touch = [CordingPoints objectAtIndex:i];
			points[i].x = [touch locationInView:RoadMapDrawingArea].x;
			points[i].y = [touch locationInView:RoadMapDrawingArea].y;
		} else {
			points[i].x = -1;
			points[i].y = -1;
		}
	}
}

int roadmap_canvas_is_cording()
{
   return ([CordingPoints count] > 1);
}

void roadmap_canvas_cancel_touches()
{
   [RoadMapDrawingArea cancelAllTouches];
}


void roadmap_canvas_show_view (UIView *view) {
	[RoadMapDrawingArea addSubview:view];
}

void roadmap_canvas_should_accept_layout (int bAcceptLayout) {
   //gs_bShouldAcceptLayout = bAcceptLayout;
   gs_bShouldAcceptLayout = TRUE; //TODO: remove
}


@implementation RoadMapCanvasView

- (BOOL) resize
{
   //roadmap_main_flush(); //this is a workaround. Need to understand why
   //roadmap_log(ROADMAP_WARNING, "resize from layer, current later bounds: %f, %f", RoadMapEAGLLayer.bounds.size.width, RoadMapEAGLLayer.bounds.size.height);
   glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
   // Allocate color buffer backing based on the current layer size
   if (viewRenderbuffer != 0)
      glDeleteRenderbuffersOES(1, &viewRenderbuffer);
	glGenRenderbuffersOES(1, &viewRenderbuffer);
   glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
   glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbuffer);
   if (![RoadMapGc renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:RoadMapEAGLLayer])
      roadmap_log (ROADMAP_FATAL, "Failed to resize renderbufferStorage");
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
   glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
   
   if (depthRenderbuffer != 0)
      glDeleteRenderbuffersOES(1, &depthRenderbuffer);
   glGenRenderbuffersOES(1, &depthRenderbuffer);
   glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
   glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, backingWidth, backingHeight);
   glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
   
   if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES)
	{
		roadmap_log (ROADMAP_FATAL, "Failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
      return NO;
   }
   needsResize = FALSE;
   
   [self setupView];
   
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
   roadmap_canvas_refresh();
   
   roadmap_canvas_ogl_configure (0.0, 1.2, 2.0);
   
   (*RoadMapCanvasConfigure) ();
   
   return YES;
   
   
   
   /*
   roadmap_log (ROADMAP_WARNING, "changing from viewRenderbuffer %d", viewRenderbuffer);
   if (viewRenderbuffer != 0)
      glDeleteRenderbuffersOES(1, &viewRenderbuffer);
	glGenRenderbuffersOES(1, &viewRenderbuffer);
	roadmap_log (ROADMAP_WARNING, "changing to viewRenderbuffer %d", viewRenderbuffer);
   
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
	if (![RoadMapGc renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:RoadMapEAGLLayer])
      roadmap_log (ROADMAP_FATAL, "resizeFromLayer: failed to set renderbufferStorage fromDrawable");
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbuffer);
	
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
   roadmap_log(ROADMAP_WARNING, "Layer size is: (%d, %d)", backingWidth, backingHeight);
   
   roadmap_log (ROADMAP_WARNING, "changing from depthRenderbuffer %d", depthRenderbuffer);
   if (depthRenderbuffer != 0)
      glDeleteRenderbuffersOES(1, &depthRenderbuffer);
   glGenRenderbuffersOES(1, &depthRenderbuffer);
   roadmap_log (ROADMAP_WARNING, "changing to depthRenderbuffer %d", depthRenderbuffer);
   glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
   glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, backingWidth, backingHeight);
   glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
	
	if(glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
		roadmap_log (ROADMAP_FATAL, "failed to make complete framebuffer object 0x%04x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
		return NO;
	}
	
	return YES;
   */
   /*
   // Allocate color buffer backing based on the current layer size
   glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
   [RoadMapGc renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:layer];
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
   glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
   
   glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
   glRenderbufferStorageOES(GL_RENDERBUFFER_OES, 
                            GL_DEPTH_COMPONENT16_OES, 
                            backingWidth, backingHeight);
   //glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
	
   if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES)
	{
      roadmap_log (ROADMAP_FATAL, "Failed to make complete framebuffer object 0x%04x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
      //NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
      return NO;
   }
   
   return YES;
    */
}

- (BOOL)createFramebuffer
{
   roadmap_log (ROADMAP_DEBUG, "Creating new Framebuffer");
   
   
   // Create default framebuffer object. The backing will be allocated for the current layer in -resizeFromLayer
   glGenFramebuffersOES(1, &viewFramebuffer);
   glGenRenderbuffersOES(1, &viewRenderbuffer);
   glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
   glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
   glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbuffer);
   
   glGenRenderbuffersOES(1, &depthRenderbuffer);
   glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
   glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
   
   return YES;
   
   
   /*
	glGenFramebuffersOES(1, &viewFramebuffer);
	glGenRenderbuffersOES(1, &viewRenderbuffer);
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
   if (![RoadMapGc renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:RoadMapEAGLLayer])
      roadmap_log (ROADMAP_FATAL, "failed to set renderbufferStorage fromDrawable");
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbuffer);
	
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
   roadmap_log(ROADMAP_WARNING, "Layer size is: (%d, %d)", backingWidth, backingHeight);
   
   glGenRenderbuffersOES(1, &depthRenderbuffer);
   glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthRenderbuffer);
   glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, backingWidth, backingHeight);
   glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthRenderbuffer);
	
	if(glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
		roadmap_log (ROADMAP_FATAL, "failed to make complete framebuffer object 0x%04x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
		return NO;
	}
	
	return YES;
    */
}


- (void)destroyFramebuffer
{
	glDeleteFramebuffersOES(1, &viewFramebuffer);
	viewFramebuffer = 0;
	glDeleteRenderbuffersOES(1, &viewRenderbuffer);
	viewRenderbuffer = 0;
	
	if(depthRenderbuffer) {
		glDeleteRenderbuffersOES(1, &depthRenderbuffer);
		depthRenderbuffer = 0;
	}
}

-(void) setContext
{
   RoadMapGc = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
 	
	if(!RoadMapGc || ![EAGLContext setCurrentContext:RoadMapGc] || ![self createFramebuffer]) {
		roadmap_log(ROADMAP_FATAL, "Unable to create framebuffer !!!");
	}
   [self resize];
}



-(RoadMapCanvasView *) initWithFrame: (struct CGRect) rect
{
   self = [super initWithFrame: rect];
   //[self setClearsContextBeforeDrawing: NO]; //this should improve performance

   RoadMapDrawingArea = self;
   
   RoadMapEAGLLayer = (CAEAGLLayer*)self.layer;
	RoadMapEAGLLayer.opaque = TRUE;
   RoadMapEAGLLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                   [NSNumber numberWithBool:TRUE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
   
   [self setContext];
	//roadmap_canvas3_ogl_prepare();

   //[self setupView];

   //roadmap_canvas_ogl_configure ();
   
   //(*RoadMapCanvasConfigure) ();
   
	[self setMultipleTouchEnabled:YES];
	CordingPoints = [[NSMutableArray alloc] initWithCapacity:MAX_CORDING_POINTS];
	
   return self;
}


+ (Class) layerClass
{
   
   return [CAEAGLLayer class];
   
}


- (void)layoutSubviews
{
   
   if ((!gs_bShouldAcceptLayout) ||
       (roadmap_canvas_height() == self.bounds.size.height &&
        roadmap_canvas_width() == self.bounds.size.width))
      return;
   
   //roadmap_log(ROADMAP_WARNING, "Changing layout from: (%d, %d) to: (%f, %f)", 
   //            roadmap_canvas_width(), roadmap_canvas_height(), self.bounds.size.width, self.bounds.size.height);
   
   if (![EAGLContext setCurrentContext:RoadMapGc])
      roadmap_log(ROADMAP_FATAL, "Error setting EAGLContext");
   
   if ((CAEAGLLayer*)self.layer != RoadMapEAGLLayer)
      roadmap_log(ROADMAP_FATAL, "Different layer !!!");
   
   //needsResize = TRUE;
   //return;
   
	//[self destroyFramebuffer];
	//[self setContext];
   [self resize];
   
   roadmap_canvas3_ogl_prepare();
   //needsResize = TRUE;

   
   //[self setupView];
   //skipDraw = TRUE;
   //(*RoadMapCanvasConfigure) ();
   //roadmap_device_event_notification( device_event_window_orientation_changed);
}

- (void)setupView {
   
   [EAGLContext setCurrentContext:RoadMapGc]; 
   
   
   //glEnable(GL_DEPTH_TEST);
   
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   
   glOrthof(0.0f, backingWidth,   // left,right
            backingHeight, 0.0f, // bottom, top
            -100.0f, 100.0f); // near, far
   
   
   
   
      
   glViewport(0, 0, backingWidth, backingHeight);
   
   
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_BLEND);

   glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);

   glEnableClientState(GL_VERTEX_ARRAY);
   //glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   
   /*
    // TODO: add this to setup
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 1/255.0);
    */
   
   /*
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);
   */
   roadmap_canvas_ogl_configure(0.0, 1.2, 2.0);
}


- (void)drawView {
   //roadmap_log(ROADMAP_WARNING, "\n\finishing drawing stuff\n\n");
   /*
   if (skipDraw) {
      skipDraw = FALSE;
      return;
   }
    */
   [EAGLContext setCurrentContext:RoadMapGc];
   
   //glDisableClientState(GL_VERTEX_ARRAY);
   //glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   
   glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
   
   [RoadMapGc presentRenderbuffer:GL_RENDERBUFFER_OES];
   //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);   
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	CGPoint p;
	RoadMapGuiPoint point;
	UITouch *touch;
	/*
	if ([CordingPoints count] > 0) {
		touch = [CordingPoints objectAtIndex:0];
		p = [touch locationInView:self];
		point.x = (int)p.x;
		point.y = (int)p.y;
		
      (*RoadMapCanvasMouseButtonReleased) (&point);	
	}
	*/
	[CordingPoints addObjectsFromArray:[touches allObjects]];
   
   if ([CordingPoints count] > 2) {
      //TODO: pick the most far fingers
   }
	
	touch = [CordingPoints objectAtIndex:0];
	p = [touch locationInView:self];
	point.x = (int)p.x;
	point.y = (int)p.y;
	(*RoadMapCanvasMouseButtonPressed) (&point);
	
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{  
	CGPoint p;
	RoadMapGuiPoint point;
	UITouch *touch;
	
	touch = [touches anyObject];
	p = [touch locationInView:self];
	point.x = (int)p.x;
	point.y = (int)p.y;
	
	[CordingPoints removeObjectsInArray:[touches allObjects]];
	/*
	if ([CordingPoints count] > 0) {
      (*RoadMapCanvasMouseButtonReleased) (&point);
		
		touch = [CordingPoints objectAtIndex:0];
		p = [touch locationInView:self];
		point.x = (int)p.x;
		point.y = (int)p.y;
		(*RoadMapCanvasMouseButtonPressed) (&point);
	} else */{
		(*RoadMapCanvasMouseButtonReleased) (&point);
	}
	
}


- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{ 	
	CGPoint p;
	RoadMapGuiPoint point;
	UITouch *touch;
	
   if ([CordingPoints count] < 1) {
      [self cancelAllTouches];
      return;
   }
   
	touch = [CordingPoints objectAtIndex:0];
	p = [touch locationInView:self];
	point.x = (int)p.x;
	point.y = (int)p.y;
	
	(*RoadMapCanvasMouseMoved) (&point);
}

- (void)cancelAllTouches
{
   [CordingPoints removeObjectsInArray:CordingPoints];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
   [CordingPoints removeObjectsInArray:[touches allObjects]];
}

@end

