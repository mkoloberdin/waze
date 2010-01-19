/**  
 * WazeUtils.java
 * This class represents general utilities for the application handling  
 * 
 * 
 * LICENSE:
 *
 *   Copyright 2009     @author Alex Agranovich
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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.Arrays;

import android.util.Log;

public class WazeUtils
{

    /*************************************************************************************************
     *================================= Public interface section =================================
     */
    
    /*************************************************************************************************
     * Copies the resources to the SD card
     * 
     */
    public static void CopyFile()
    {
//        Log.i( "CopyResources2SD", "Starting t copy the resources" );
//        String resDir = FreeMapResources.mAppDir;
//        String dstDir = FreeMapResources.mSDCardDir;
//        // Copying
//        CopyDir( resDir, dstDir );
        
    }
    /*************************************************************************************************
     * Copies the directory to the destination
     * 
     */
    public static void Copy( String srcPath, String dstPath )
    {
        File srcFile = new File( srcPath );
        File dstFile = new File( dstPath );
        String delim = new String( "/" );
        
        if ( !srcFile.exists() )
        {
            Log.e( "CopyDir", "Source does not exist!" );
            return;
        }
        if ( srcFile.isDirectory() )
        {
            dstFile.mkdirs();
            String files[] = srcFile.list();
            for ( int i = 0; i < files.length; ++i )
            {
                Copy( srcPath + delim + files[i], dstPath + delim + files[i] );
            }            
        }
        else
        {
            CopyFile( srcPath, dstPath );
        }
    }
        
     private static void CopyFile( String srcPath, String dstPath )
     {
         FileChannel srcChannel = null;
         FileChannel dstChannel = null;
         try
         {
             // Open files
             srcChannel = ( new FileInputStream( srcPath ) ).getChannel();
             dstChannel = ( new FileOutputStream( dstPath ) ).getChannel();
             // Transfer the data
             dstChannel.transferFrom( srcChannel, 0, srcChannel.size() );
             // Close the files
             if (srcChannel != null)
             {
                 srcChannel.close();
             }
             if (dstChannel != null)
             {
                 dstChannel.close();
             }
         }
         catch( FileNotFoundException ex )
         {
             Log.i( "CopyFile", "File not found " + ex.getMessage() );
             ex.printStackTrace();
         }
         catch( IOException ex )
         {
             Log.i( "CopyFile", "Transfer data error " + ex.getMessage() );
             ex.printStackTrace();
         }
    }
     /*************************************************************************************************
      * Deletes the directory or file
      * 
      */
     public static void Delete( File aFile)
     {
         Delete( aFile, null );
     }
     /*************************************************************************************************
      * Deletes the directory or file ( if not in the exception list )
      * 
      */
     private static void Delete( File aFile, ArrayList<ExceptionEntry> aExceptionList )
     {
         try
         {
         
             if ( !aFile.exists() )
             {
                 Log.e( "Delete", "Source does not exist! " + aFile.getAbsolutePath() );
                 return;
             }
             if ( CheckDeleteFileException( aExceptionList, aFile.getName() ) )
                 return;
             
             if ( aFile.isDirectory() )
             {
                 File  files[] = aFile.listFiles();
                 for ( int i = 0; i < files.length; ++i )
                 {
                     Delete( files[i], aExceptionList );
                 }                  
             }
             aFile.delete();
         }
         catch( Exception ex )
         {
             Log.e( "Delete", "Delete failed for: " + aFile.getAbsolutePath() + " " + ex.getMessage() );
             ex.printStackTrace();
         }
     }
     
     
     static private boolean CheckDeleteFileException( ArrayList<ExceptionEntry> aExceptionList, String aEntry )
     {
         boolean res = false;
         int index = -1;

         if ( aExceptionList != null )
         {
             ExceptionEntry exEntry = new ExceptionEntry( aEntry,
                                                             ExceptionEntry.COMPARE_ENDS_WITH,
                                                             ExceptionEntry.COMPARE_USE_ANOTHER_SIDE
                                                         );
             index = aExceptionList.indexOf( exEntry );
             
             res = ( index != -1 );
         }
         
         return res;
     }
     /*************************************************************************************************
      * Deletes the directory and all of its content
      * 
      */
     public static void DeleteDir( String aPath )
     {
         DeleteDir( aPath, false, null );
     }
     /*************************************************************************************************
      * Deletes the directory ( or its content )
      * 
      */
     public static void DeleteDir( String aPath, boolean aIsContentOnly )
     {
         DeleteDir( aPath, aIsContentOnly, null );
     }
     /*************************************************************************************************
      * Deletes the directory ( or its content )
      * 
      */
     public static void DeleteDir( String aPath, boolean aIsContentOnly, ExceptionEntry[] aExceptionList )
     {
         ArrayList<ExceptionEntry> exceptList = null;
         if ( aExceptionList != null )
             exceptList = new ArrayList<ExceptionEntry>( Arrays.asList( aExceptionList ) );

         File inputFile = new File( aPath );
         if ( aIsContentOnly )
         {
             File  files[] = inputFile.listFiles();
             if ( files  == null )
             {
                 return;
             }
             for ( int i = 0; i < files.length; ++i )
             {
                 Delete( files[i], exceptList );
             }
         }
         else
         {
             Delete( inputFile, exceptList );
         }
     }
     
     public static class ExceptionEntry    
     {
         public ExceptionEntry( String aEntry, int aType )
         {
             setEntry( aEntry, aType );
         }
         public ExceptionEntry( String aEntry, int aType, int aCmpType )
         {
             setEntry( aEntry, aType, aCmpType );
         }

         public void setEntry( String aEntry, int aType )
         {
             mEntry = new String( aEntry );
             mType = aType;
         }
         public void setEntry( String aEntry, int aType, int aCmpType )
         {
             mEntry = new String( aEntry );
             mType = aType;
             mCompareType = aCmpType;
         }
         public boolean equals( Object obj )
         {
             boolean res = false;
             // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
             // No polymorphism implementation 
             // Exceptional usage of instanceof
             if ( obj instanceof ExceptionEntry )
                 res = this.equalTo( ( ExceptionEntry ) obj );
             
             if ( obj instanceof String )
                 res = this.equalTo( ( String ) obj );
             
             return res;
         }
         private boolean equalTo( ExceptionEntry obj )
         {
             if ( mCompareType == COMPARE_USE_ANOTHER_SIDE )
                 return obj.equalTo( this.mEntry );     // Swap sides
             else
                 return this.equalTo( obj.mEntry );
         }
         private boolean equalTo( String obj )
         {
             boolean res = false;
             if ( mCompareType == COMPARE_EQUAL || mCompareType == COMPARE_USE_ANOTHER_SIDE )
                 res = mEntry.equals( obj );
             if ( mCompareType == COMPARE_START_OF )
                 res = obj.startsWith( mEntry );            
             if ( mCompareType == COMPARE_STARTS_WITH )
                 res = mEntry.startsWith( obj );
             if ( mCompareType == COMPARE_ENDS_WITH )
                 res = mEntry.endsWith( obj );
             
             return res;
         }
         public String mEntry = null;
         public int    mType = EXCLUDE_ALWAYS;
         public int    mCompareType = COMPARE_EQUAL;
         
         public final static int EXCLUDE_ALWAYS = 0;
         public final static int EXCLUDE_IF_EXIST = 1;
         
         
         public final static int COMPARE_USE_ANOTHER_SIDE = 0;              // Uses the compare type of the other side
         public final static int COMPARE_EQUAL = 1;
         public final static int COMPARE_START_OF = 2;
         public final static int COMPARE_STARTS_WITH = 3;
         public final static int COMPARE_ENDS_WITH = 4;
     }
     
     
    /*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */

    
}
