/**  
 * WazeEglManager.java
 * This class provides the EGL management routines   
 * 
 * 
 * LICENSE:
 *
 *   Copyright 2010     @author Alex Agranovich  AGA
 *   
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
 *   @see 
 */

package com.waze;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGL11;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;

import android.content.DialogInterface;
import android.util.Log;
import android.view.SurfaceView;

public final class WazeEglManager {
    /*************************************************************************************************
     *================================= Public interface section =================================
     */
	
	
	public WazeEglManager( SurfaceView aSurfaceView )
	{	
		mSurfaceView = aSurfaceView;
	}
	
	
	// AGA DEBUG
	boolean mIsFirstSwap = false; 
	
    /*************************************************************************************************
     * EGL engine initialization routine. Initializes egl to be used both on native side and java layers
     * Should be called in the thread context where it will be used.  
     */
    public boolean InitEgl()
    {
    	boolean res = true;
    	/*
         * Get an EGL instance
         */
    	mEgl = (EGL10) EGLContext.getEGL();
 
        /*
         * Get to the default display.
         */
        mEglDisplay = mEgl.eglGetDisplay( EGL10.EGL_DEFAULT_DISPLAY );
        EglCheck( "InitEgl eglGetDisplay" );
        
        /*
         * We can now initialize EGL for that display
         */
        int[] version = new int[2];
        mEgl.eglInitialize( mEglDisplay, version );
        EglCheck( "InitEgl eglInitialize" );
        
        int[] configAttrs = new int[] {
                EGL10.EGL_RED_SIZE, 	5,
                EGL10.EGL_GREEN_SIZE, 	6,
                EGL10.EGL_BLUE_SIZE, 	5,
                EGL10.EGL_ALPHA_SIZE, 	0,
                EGL10.EGL_DEPTH_SIZE, 	0,                
                EGL10.EGL_STENCIL_SIZE, 0, 
                EGL10.EGL_NONE};
        
        EGLConfig[] configs = new EGLConfig[1];
        mEglConfig = ChooseEglConfig( configAttrs );
        
        if ( EGL_DEBUG )
        {
			 int[] r = new int[1];
			 int[] g = new int[1];
			 int[] b = new int[1];
			 int[] a = new int[1];
			 int[] d = new int[1];
			 int[] cfg = new int[1];
			 int[] buf_size = new int[1]; 
			 int[] surf_type = new int[1]; 
			 
			 mEgl.eglGetConfigAttrib( mEglDisplay, configs[0], EGL10.EGL_RED_SIZE,   r );
			 mEgl.eglGetConfigAttrib( mEglDisplay, configs[0], EGL10.EGL_GREEN_SIZE, g );
			 mEgl.eglGetConfigAttrib( mEglDisplay, configs[0], EGL10.EGL_BLUE_SIZE,  b );
			 mEgl.eglGetConfigAttrib( mEglDisplay, configs[0], EGL10.EGL_ALPHA_SIZE, a );
			 mEgl.eglGetConfigAttrib( mEglDisplay, configs[0], EGL10.EGL_BUFFER_SIZE, buf_size );
			 mEgl.eglGetConfigAttrib( mEglDisplay, configs[0], EGL10.EGL_SURFACE_TYPE, surf_type );
			 mEgl.eglGetConfigAttrib( mEglDisplay, configs[0], EGL10.EGL_DEPTH_SIZE, d );
			 mEgl.eglGetConfigAttrib( mEglDisplay, configs[0], EGL10.EGL_CONFIG_ID, cfg );
			 
			 Log.w( "WAZE DEBUG", "Color: " + r[0] + ", " + g[0] + ", " + b[0] + ", " + 
					 a[0]  + ", " + buf_size[0]  + ", " + surf_type[0]  + ", " + d[0] + ", " + cfg[0] );
        }
         
        /*
        * Create an OpenGL ES context. This must be done only once, an
        * OpenGL context is a somewhat heavy object.
        */
//        final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
//        int egl_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 1, EGL10.EGL_NONE };
        mEglContext = mEgl.eglCreateContext( mEglDisplay, mEglConfig, EGL10.EGL_NO_CONTEXT, null );
        EglCheck( "InitEgl eglCreateContext" );
        
        res = CreateEglSurface();
        
        if ( EGL_DEBUG )
        {
	        int[] w = new int[1];
	        if ( !mEgl.eglQuerySurface( mEglDisplay, mEglSurface, EGL10.EGL_WIDTH, w ) ) 
	        {
	            throw new RuntimeException("eglQuerySurface failed.");
	        }
	        else
	        {
	        	Log.w( "WAZE", "EGL query width: " + w[0] );
	        }
        }
        
        return res;
    }
    
    /*************************************************************************************************
     * EGL engine destruction procedure
     * Should be called when application is going to the background  
     */
    public void DestroyEgl()
    {
        /*
         * Unbind and destroy the old EGL surface, if
         * there is one.
         */
    	if ( mEglDisplay == null )
    		return;
    	
    	DestroyEglSurface();
    	
//    	Log.i("WAZE", "Destroying EGL Context" );
    	mEgl.eglDestroyContext( mEglDisplay, mEglContext );
    	EglCheck( "DestroyEgl eglDestroyContext" );
    	
//    	Log.w("WAZE", "Destroying EGL Display" );
    	mEgl.eglTerminate( mEglDisplay );
    	EglCheck( "DestroyEgl eglTerminate" );
    	
    	mEglContext = null;
    	mEglSurface = null;
    	mEglDisplay = null;
//    	Log.w("WAZE", "Destroying EGL has finalized" );
    }
    
    /*************************************************************************************************
     * EGL config chooser
     *   @param aCfgAttribs - the array of the configuration attributes
     *   @return the matching EGL config object
     */
    public  EGLConfig ChooseEglConfig( int[] aCfgAttribs )
    {
    	EGLConfig resCfg = null;
    	int[] num_config = new int[1];
        if ( !mEgl.eglChooseConfig( mEglDisplay, aCfgAttribs, null, 0,
                num_config ) ) {
        	
        	EglCheck( "InitEgl eglChooseConfig" );
            WazeLog.w( "eglChooseConfigfailed - failed getting configuration number" );
            return null;
        }

        int numConfigs = num_config[0];

        if ( numConfigs <= 0 ) 
        {
            WazeLog.w( "eglChooseConfigfailed - failed getting configuration number" );
            return null;
        }

        EGLConfig[] resConfigs = new EGLConfig[numConfigs];
        if ( !mEgl.eglChooseConfig( mEglDisplay, aCfgAttribs, resConfigs, numConfigs,
                num_config ) ) 
        {
        	EglCheck( "InitEgl eglChooseConfig II" );
            WazeLog.w( "eglChooseConfigfailed - failed getting configuration number" );
            return null;
        }
        /*
         * Find the matching configuration
         */
        EGLConfig config;
        for (  int i = 0; i < resConfigs.length; ++i ) 
        {
        	config = resConfigs[i];
        	int d = getCfgValue( config, EGL10.EGL_DEPTH_SIZE, 0 );
            int s = getCfgValue( config, EGL10.EGL_STENCIL_SIZE, 0 );
            if ( ( d >= getAttrValue( aCfgAttribs, EGL10.EGL_DEPTH_SIZE ) ) && 
            		( s >= getAttrValue( aCfgAttribs, EGL10.EGL_STENCIL_SIZE ) ) ) 
            {
                int r = getCfgValue( config, EGL10.EGL_RED_SIZE, 0 );
                int g = getCfgValue( config, EGL10.EGL_GREEN_SIZE, 0 );
                int b = getCfgValue( config, EGL10.EGL_BLUE_SIZE, 0 );
                int a = getCfgValue( config, EGL10.EGL_ALPHA_SIZE, 0 );
                if ( ( r == getAttrValue( aCfgAttribs, EGL10.EGL_RED_SIZE ) ) 
                		&& ( g == getAttrValue( aCfgAttribs, EGL10.EGL_GREEN_SIZE ) )
                        && ( b == getAttrValue( aCfgAttribs, EGL10.EGL_BLUE_SIZE ) ) 
                        && ( a == getAttrValue( aCfgAttribs, EGL10.EGL_ALPHA_SIZE ) ) ) 
                {
                		resCfg = config;
                		break;
                }
            }
        }
        /*
         * Get the default one
         */
        if ( resCfg == null )
        {
            int[] configAttrs = new int[] {
                    EGL10.EGL_NONE};
            
            EGLConfig[] configs = new EGLConfig[1];
            numConfigs = 1;
            mEgl.eglChooseConfig( mEglDisplay, configAttrs, configs, numConfigs, num_config );
            EglCheck( "InitEgl eglChooseConfig III" );
            resCfg = configs[0];                        

        }
    	return resCfg;    	
    }
    
    /*************************************************************************************************
     * EGL surface destruction procedure
     * Should be called when the surface dimensions are changed  
     */
    public void DestroyEglSurface()
    {
        /*
         * Unbind and destroy the old EGL surface, if
         * there is one.
         */
    	if ( mEglDisplay == null || mEglSurface == null || mEglSurface == EGL10.EGL_NO_SURFACE )
    	{
    		Log.e( "WAZE", "Surface parameters are not initialized" );
    		WazeLog.e( "Surface parameters are not initialized" );
    		return;
    	}
    	
        mEgl.eglMakeCurrent( mEglDisplay, EGL10.EGL_NO_SURFACE,
                EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT );
        EglCheck( "DestroyEglSurface eglMakeCurrent" );
        
    	mEgl.eglDestroySurface( mEglDisplay, mEglSurface );
    	EglCheck( "DestroyEglSurface eglDestroySurface" );

    	mEglSurface = null;    	
    }
    
    /*************************************************************************************************
     * EGL surface destruction procedure
     * Should be called when the surface dimensions are changed  
     */
    public boolean CreateEglSurface()
    {
        /*
         * Destroy if not destroyed before 
         */
    	if ( mEglSurface != null && mEglSurface == EGL10.EGL_NO_SURFACE )
    	{
    		DestroyEglSurface();
    	}
        mEglSurface = mEgl.eglCreateWindowSurface( mEglDisplay, mEglConfig, mSurfaceView, null );
        if ( EglCheck( "CreateEglSurface eglCreateWindowSurface" ) != EGL10.EGL_SUCCESS )
        {
        	return false;
        }
        
        mEgl.eglMakeCurrent( mEglDisplay, mEglSurface, mEglSurface, mEglContext );
    	if ( EglCheck( "CreateEglSurface eglMakeCurrent" )  != EGL10.EGL_SUCCESS )
    	{
    		return false;
    	}
    	    	    
//    	Log.i("WAZE", "Creating EGL surface has finalized" );
    	return true;
    }
    
    /*************************************************************************************************
     * EGL engine swap buffers procedure
     * Should be called when canvas is rendered  
     */
    public void SwapBuffersEgl()
    {
    	boolean eglRes = true;

    	eglRes = mEgl.eglSwapBuffers( mEglDisplay, mEglSurface );
    	
    	if ( !eglRes )
    	{
    		int error = EglCheck( "EGL Swap buffers" );
    		switch( error ) 
    		{
    			case EGL11.EGL_CONTEXT_LOST:
    			{
    				if ( mContextRecoverRetry++ < EGL_CONTEXT_RECOVER_RETRY_NUM )
    				{
	    				/*
	    				 * Reset the surface
	    				 */
	    				DestroyEglSurface();
	    				CreateEglSurface();
	    				break;
    				}
    			}
    			case EGL10.EGL_SUCCESS:
    			{
    				break;
    			}
    			case EGL10.EGL_BAD_NATIVE_WINDOW:
    			default:
    			{
    				WazeMsgBox msgBox = WazeMsgBox.getInstance();
    				msgBox.setBlocking( true );
    				msgBox.Show( "Error", "Critical problem occured! Please restart waze", "Ok", null,
    						new EglErrorListener(), null );
    				FreeMapAppService.RestartApplication();
    			}
    		
    		}
        }
    	else
    	{
    		mContextRecoverRetry = 0;
    	}

    	 
//    	Log.w("AGA DEBUG", "Just swapped" );
//    	android.os.SystemClock.sleep( 200L );
    	
    }
    /*************************************************************************************************
     * EGL engine swap buffers procedure
     * Should be called when canvas is rendered  
     */
    private class EglErrorListener implements DialogInterface.OnClickListener
    {
    	public void onClick( DialogInterface dialog, int which )
    	{
    		final WazeMsgBox msgBox = WazeMsgBox.getInstance();
    		synchronized( msgBox ) {
    			msgBox.notify();
    		}    		
    	}
    }
    
    
    /*************************************************************************************************
     * Extracts EGL configuration value
     * Auxiliary  
     */
    private int getCfgValue( EGLConfig aConfig, int aAttr, int aDefault )
    {
    	int value[] = new int[1];
        if ( mEgl.eglGetConfigAttrib( mEglDisplay, aConfig, aAttr, value ) ) 
        {
            return value[0];
        }
        return aDefault;
    }
    /*************************************************************************************************
     * Extracts attribute value from the given set of attributes
     * Not efficient for large arrays. This supposed to be used only at the application start 
     *  
     * Auxiliary  
     */
    private int getAttrValue( int[] aAttrSet, int aAttr )
    {
    	int res = -1;    	
    	for ( int i = 0; i < aAttrSet.length; i+=2 )
    	{
    		if ( aAttrSet[i] == aAttr )
    		{
    			res = aAttrSet[i+1];
    			break;
    		}
    	}
        return res;
    }
    /*************************************************************************************************
     * EGL error checker
     * Auxiliary  
     */
    private int EglCheck( String aErrStr )
    {
    	int error = mEgl.eglGetError();
    	if ( error != EGL10.EGL_SUCCESS )
    	{
    		String errStr = new String( "EGL error in " + aErrStr + ". Error code: " + error );
    		Log.e("WAZE", errStr );
    		WazeLog.e( errStr );
    	}    	
    	return error;
    }
    
    /*************************************************************************************************
     * GL test procedure
     * Should be called in the thread context where egl is initialized
     */
    public void DrawGLTest()
    {
    	mGLTestStop = false;
    	int width = mSurfaceView.getMeasuredWidth();
    	int height = mSurfaceView.getMeasuredHeight();
    	
    	GL10 gl = ( GL10) mEglContext.getGL();
    	
    	gl.glMatrixMode( GL10.GL_PROJECTION );
    	gl.glLoadIdentity();
        
    	gl.glViewport( 0, 0, width, height );
        
    	gl.glOrthof( 0.0f, width, 0.0f, height, 0.0f, 1.0f );
        
    	gl.glShadeModel( GL10.GL_FLAT );
    	gl.glEnable( GL10.GL_BLEND );
//    	aGL.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);
	    gl.glEnable( GL10.GL_TEXTURE_2D );
    	
    	
    	int i = 3;
    	float r, g, b;
    	for ( ;!mGLTestStop && i < 50; ++i )
    	{
    		r = (float) i%3;
    		g = (float) (i+1)%3;
    		b = (float) (i+2)%3;
    		
//    		Log.w("WAZE DEBUG", "Swapping buffers" );
		    gl.glClearColor( r, g, b, 1 );
		    gl.glClear( GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT );
		    if ( !mEgl.eglSwapBuffers( mEglDisplay, mEglSurface ) )
		    {
		    	throw new RuntimeException("eglSwapBuffers failed.");
		    }
		    android.os.SystemClock.sleep( 300 );
    	}
    }
    /*************************************************************************************************
     * Wrapper for the EGL engine getGL
     * 
     */
    public GL10 getGL()
    {
    	final GL10 gl = ( mEglContext == null ) ? null : ( ( GL10 ) mEglContext.getGL() ); 
    	return gl;
    }


	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the
     * native side and should be called after!!! the shared library is loaded
     */

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    private int mContextRecoverRetry = 0;
    
	private SurfaceView mSurfaceView = null;
	private EGLDisplay mEglDisplay = null;
	private EGLContext mEglContext = null;
	private EGLSurface mEglSurface = null;
	private EGLConfig  mEglConfig  = null;
	private EGL10 mEgl = null;

    // DEBUG SESSION
    public boolean mGLTestStop = false;
    
    /*************************************************************************************************
     *================================= Constants section =================================
     */
    private static final boolean EGL_DEBUG = false;
    private static final int EGL_CONTEXT_RECOVER_RETRY_NUM = 5;
}

