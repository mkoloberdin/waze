/* tts_db_sqlite.c - implementation of the Text To Speech (TTS) database layer
 *                       SQlite based
 *
 * LICENSE:
 *   Copyright 2011, Waze Ltd      Alex Agranovich (AGA)
 *
 *
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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
 *   See tts_db_sqlite.h
 *       tts_db.h
 *       tts.h
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_performance.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "tts_db_sqlite.h"


//======================== Local defines ========================

#define   TTS_DB_SQLITE_QUERY_MAXSIZE        1024
#define   TTS_DB_SQLITE_PATH_MAXSIZE      512
#define   TTS_DB_SQLITE_TRANS_TIMEOUT        2500L       // Transaction timeout in msec
#define   TTS_DB_SQLITE_TRANS_STMTS_CNT      200         // Transaction timeout in msec

#define   TTS_DB_SQLITE_FILE_NAME            "tts.db"

#define   TTS_DB_SQLITE_STMT_CREATE_TABLE   "CREATE TABLE IF NOT EXISTS '%s' (text TEXT, data BLOB, path TEXT, storage_type INTEGER, text_type INTEGER, PRIMARY KEY(text))"
#define   TTS_DB_SQLITE_STMT_STORE          "INSERT OR REPLACE INTO '%s' values (?,?,?,?,?);"
#define   TTS_DB_SQLITE_STMT_LOAD           "SELECT data, storage_type, path FROM '%s' WHERE text=?;"
#define   TTS_DB_SQLITE_STMT_LOAD_INFO      "SELECT storage_type, path, text_type FROM '%s' WHERE text=?;"
#define   TTS_DB_SQLITE_STMT_REMOVE         "DELETE FROM '%s' WHERE text=?;"
#define   TTS_DB_SQLITE_STMT_DROP_TABLE     "DROP TABLE IF EXISTS '%s';"
#define   TTS_DB_SQLITE_STMT_EXISTS         "SELECT EXISTS ( SELECT 1 FROM '%s' WHERE text=?);"
#define   TTS_DB_SQLITE_STMT_SYNC_OFF       "PRAGMA synchronous = OFF"
#define   TTS_DB_SQLITE_STMT_CNT_OFF        "PRAGMA count_changes = OFF"
#define   TTS_DB_SQLITE_STMT_TMP_STORE_MEM  "PRAGMA temp_store = MEMORY"
#define   TTS_DB_SQLITE_STMT_CACHE_SIZE     "PRAGMA cache_size = 2000"
#define   TTS_DB_SQLITE_STMT_PAGE_SIZE      "PRAGMA page_size = 8192"
#define   TTS_DB_SQLITE_STMT_ENCODING       "PRAGMA encoding = 'UTF-8'"

#define check_sqlite_error( errstr, code ) \
   _check_sqlite_error_line( errstr, code, __LINE__ )

//======================== Local types ========================

typedef enum
{
   _con_lifetime_session = 0x0,     // Connection remains active only within one statement execution
   _con_lifetime_application,       // Connection remains active within application
} RMTtsStorageConLifetime;

//======================== Globals ========================

static sqlite3* sgSQLiteDb  = NULL;       // The current db handle
static RMTtsStorageConLifetime sgConLifetime = _con_lifetime_session;     // The type of connection lifetime

static BOOL sgIsInTransaction = FALSE;
static int  sgTransStmtsCount = 0;

//======================== Local Declarations ========================

static sqlite3*  _trans_open( const char* table_name );
static void _trans_commit( void );
static void _trans_rollback( void );
static void _trans_timeout( void );
static BOOL _check_sqlite_error_line( const char* errstr, int code, int line );
static sqlite3* _get_db( void );
static void _close_db( void );
static const char* _get_db_file( void );
static const char* _table_name( const char* _name );

/*
 ******************************************************************************
 */

BOOL tts_db_sqlite_store( const TtsDbEntry* entry, TtsDbDataStorageType storage_type, const TtsData* db_data, const TtsPath* db_path )
{
   sqlite3* db = NULL;
   sqlite3_stmt *stmt = NULL;
   int ret_val;
   char stmt_string[TTS_DB_SQLITE_QUERY_MAXSIZE];
   const char* path = db_path ? db_path->path : "";

   db = _trans_open( entry->voice_id );

   if ( !db )
   {
      roadmap_log( ROADMAP_ERROR, "TTS storage failed - cannot open database" );
      return FALSE;
   }
   /*
    * Prepare the string
    */
   sprintf( stmt_string, TTS_DB_SQLITE_STMT_STORE, _table_name( entry->voice_id ) );

   roadmap_log( ROADMAP_DEBUG, "TTS SQLite Store. Query string: %s", stmt_string );

   /*
    * Prepare the sqlite statement
    */
   ret_val = sqlite3_prepare( db, stmt_string, -1, &stmt, NULL );
   if ( !check_sqlite_error( "preparing the SQLITE statement", ret_val ) )
   {
      return FALSE;
   }

   /*
    * Binding the data
    */
   ret_val = sqlite3_bind_text( stmt, 1, entry->text, strlen( entry->text ), NULL );
   if ( !check_sqlite_error( "binding the text statement", ret_val ) )
   {
      return FALSE;
   }
   if ( db_data && db_data->data )
   {
      ret_val = sqlite3_bind_blob( stmt, 2, db_data->data, db_data->data_size, NULL );
      if ( !check_sqlite_error( "binding the blob statement", ret_val ) )
      {
         return FALSE;
      }
   }
   if ( db_path )
   {
      ret_val = sqlite3_bind_text( stmt, 3, path, strlen( db_path->path ), NULL );
      if ( !check_sqlite_error( "binding the path statement", ret_val ) )
      {
         return FALSE;
      }
   }
   ret_val = sqlite3_bind_int( stmt, 4, storage_type );
   if ( !check_sqlite_error( "binding the storage type statement", ret_val ) )
   {
      return FALSE;
   }
   ret_val = sqlite3_bind_int( stmt, 5, entry->text_type );
   if ( !check_sqlite_error( "binding the text type statement", ret_val ) )
   {
      return FALSE;
   }
   /*
    * Evaluate
    */
   sqlite3_step( stmt );
   if ( !check_sqlite_error( "finishing", sqlite3_finalize( stmt ) ) )
   {
      return FALSE;
   }

   /*
    * Close the database
    */
   if ( sgConLifetime == _con_lifetime_session && !sgIsInTransaction )
   {
      sqlite3_close( db );
   }

   return TRUE;
}

/*
 ******************************************************************************
 */
BOOL tts_db_sqlite_remove( const TtsDbEntry* entry )
{
   sqlite3* db = NULL;
   sqlite3_stmt *stmt = NULL;
   int ret_val;
   char stmt_string[TTS_DB_SQLITE_QUERY_MAXSIZE];

   db = _trans_open( NULL );

   if ( !db )
   {
      roadmap_log( ROADMAP_ERROR, "TTS cache remove failed - cannot open database" );
   }

   /*
    * Prepare the string
    */
   sprintf( stmt_string, TTS_DB_SQLITE_STMT_REMOVE, _table_name( entry->voice_id ) );

   /*
    * Prepare the sqlite statement
    */
   ret_val = sqlite3_prepare( db, stmt_string, -1, &stmt, NULL );
   if ( !check_sqlite_error( "preparing the SQLITE statement", ret_val ) )
   {
      return FALSE;
   }

   /*
    * Binding the data
    */
   ret_val = sqlite3_bind_text( stmt, 1, entry->text, strlen( entry->text ), NULL );
   if ( !check_sqlite_error( "binding the text statement", ret_val ) )
   {
      return FALSE;
   }

   /*
    * Evaluate
    */
   ret_val = sqlite3_step( stmt );
   if ( ret_val != SQLITE_DONE )
   {
      check_sqlite_error( "statement evaluation", ret_val );
   }

   /*
    * Finalize
    */
   sqlite3_finalize( stmt );
   /*
    * Close the database
    */
   if ( sgConLifetime == _con_lifetime_session  && !sgIsInTransaction )
   {
      sqlite3_close( db );
   }
   return TRUE;
}

/*
 ******************************************************************************
 */
BOOL tts_db_sqlite_get_info( const TtsDbEntry* entry, TtsTextType* text_type, TtsDbDataStorageType* storage_type, TtsPath* db_path )
{
   int res = TRUE;
   sqlite3* db = NULL;
   sqlite3_stmt *stmt = NULL;
   int ret_val;
   char stmt_string[TTS_DB_SQLITE_QUERY_MAXSIZE];

   db = _trans_open( NULL );

   if ( !db )
   {
      roadmap_log( ROADMAP_ERROR, "Tile loading failed - cannot open database" );
      return FALSE;
   }

   /*
    * Prepare the string
    */
   sprintf( stmt_string, TTS_DB_SQLITE_STMT_LOAD_INFO, _table_name( entry->voice_id ) );

   /*
    * Prepare the sqlite statement
    */
   ret_val = sqlite3_prepare( db, stmt_string, -1, &stmt, NULL );
   if ( !check_sqlite_error( "preparing the SQLITE statement", ret_val ) )
   {
      return FALSE;
   }

   /*
    * Binding the data
    */
   ret_val = sqlite3_bind_text( stmt, 1, entry->text, strlen( entry->text ), NULL );
   if ( !check_sqlite_error( "binding the text statement", ret_val ) )
   {
      return FALSE;
   }

   /*
    * Evaluate
    */
   ret_val = sqlite3_step( stmt );

   if ( ret_val == SQLITE_ROW )
   {
      // Storage type
      if ( storage_type )
         *storage_type = sqlite3_column_int( stmt, 0 );
      // Path data
      const char* path_data = sqlite3_column_text( stmt, 1 );
      if ( db_path )
      {
         if ( path_data )
         {
            strncpy_safe( db_path->path, path_data, TTS_PATH_MAXLEN );
            res = TRUE;
         }
         else
         {
            db_path->path[0] = 0;
         }
      }
      // Text type
      if ( text_type )
         *text_type = sqlite3_column_int( stmt, 2 );
   }
   else
   {
      res = FALSE;
      if ( ret_val != SQLITE_DONE )
      {
         check_sqlite_error( "select evaluation", ret_val );
      }
   }

   /*
    * Finalize
    */
   sqlite3_finalize( stmt );
   /*
    * Close the database
    */
   if ( sgConLifetime == _con_lifetime_session && !sgIsInTransaction )
   {
      sqlite3_close( db );
   }

   return res;
}

/*
 ******************************************************************************
 */
BOOL tts_db_sqlite_get( const TtsDbEntry* entry, TtsDbDataStorageType* storage_type, TtsData *db_data, TtsPath* db_path )
{
   int res = TRUE;
   sqlite3* db = NULL;
   sqlite3_stmt *stmt = NULL;
   int ret_val;
   char stmt_string[TTS_DB_SQLITE_QUERY_MAXSIZE];

   db = _trans_open( NULL );

   if ( !db )
   {
      roadmap_log( ROADMAP_ERROR, "Tile loading failed - cannot open database" );
      return FALSE;
   }

   /*
    * Prepare the string
    */
   sprintf( stmt_string, TTS_DB_SQLITE_STMT_LOAD, _table_name( entry->voice_id ) );

   /*
    * Prepare the sqlite statement
    */
   ret_val = sqlite3_prepare( db, stmt_string, -1, &stmt, NULL );
   if ( !check_sqlite_error( "preparing the SQLITE statement", ret_val ) )
   {
      return FALSE;
   }

   /*
    * Binding the data
    */
   ret_val = sqlite3_bind_text( stmt, 1, entry->text, strlen( entry->text ), NULL );
   if ( !check_sqlite_error( "binding the text statement", ret_val ) )
   {
      return FALSE;
   }

   /*
    * Evaluate
    */
   ret_val = sqlite3_step( stmt );

   if ( ret_val == SQLITE_ROW )
   {
      const void* blob_data;
      const char* path_data;
      int blob_size = sqlite3_column_bytes( stmt, 0 );
      if ( db_data && ( blob_size > 0 ) )
      {
         db_data->data_size = blob_size;
         db_data->data = malloc( blob_size );
         roadmap_check_allocated( db_data->data );
         blob_data = sqlite3_column_blob( stmt, 0 );
         memcpy( db_data->data, blob_data, db_data->data_size );
      }

      if ( storage_type )
      {
         *storage_type = sqlite3_column_int( stmt, 1 );
      }

      if ( db_path )
      {
         path_data = sqlite3_column_text( stmt, 2 );
         if ( path_data )
         {
            strncpy_safe( db_path->path, path_data, sizeof( db_path->path ) );
            res = TRUE;
         }
         else
         {
            db_path->path[0] = 0;
         }
      }
   }
   else
   {
      res = FALSE;
      if ( ret_val != SQLITE_DONE )
      {
         check_sqlite_error( "select evaluation", ret_val );
      }
   }

   /*
    * Finalize
    */
   sqlite3_finalize( stmt );
   /*
    * Close the database
    */
   if ( sgConLifetime == _con_lifetime_session && !sgIsInTransaction )
   {
      sqlite3_close( db );
   }

   return res;
}

/*
 ******************************************************************************
 */
BOOL tts_db_sqlite_exists( const TtsDbEntry* entry )
{
   int res = TRUE;
   sqlite3* db = NULL;
   sqlite3_stmt *stmt = NULL;
   int ret_val;
   char stmt_string[TTS_DB_SQLITE_QUERY_MAXSIZE];

   db = _trans_open( entry->voice_id );

   if ( !db )
   {
      roadmap_log( ROADMAP_ERROR, "Tile loading failed - cannot open database" );
      return FALSE;
   }

   /*
    * Prepare the string
    */
   sprintf( stmt_string, TTS_DB_SQLITE_STMT_EXISTS, ( entry->voice_id ) );

   /*
    * Prepare the sqlite statement
    */
   ret_val = sqlite3_prepare( db, stmt_string, -1, &stmt, NULL );
   if ( !check_sqlite_error( "preparing the SQLITE statement", ret_val ) )
   {
      return FALSE;
   }

   /*
    * Binding the data
    */
   ret_val = sqlite3_bind_text( stmt, 1, entry->text, strlen( entry->text ), NULL );
   if ( !check_sqlite_error( "binding the text statement", ret_val ) )
   {
      return FALSE;
   }

   /*
    * Evaluate
    */
   ret_val = sqlite3_step( stmt );

   if ( ret_val == SQLITE_ROW )
   {
      res = sqlite3_column_int( stmt, 0 );;
   }
   else
   {
      res = FALSE;
      if ( ret_val != SQLITE_DONE )
      {
         check_sqlite_error( "select evaluation", ret_val );
      }
   }

   /*
    * Finalize
    */
   sqlite3_finalize( stmt );
   /*
    * Close the database
    */
   if ( sgConLifetime == _con_lifetime_session && !sgIsInTransaction )
   {
      sqlite3_close( db );
   }

   return res;
}
/*
 ******************************************************************************
 */
BOOL tts_db_sqlite_destroy_voice( const char* voice_id )
{
   sqlite3* db = NULL;
   sqlite3_stmt *stmt = NULL;
   int ret_val;
   char stmt_string[TTS_DB_SQLITE_QUERY_MAXSIZE];

   db = _trans_open( NULL );

   if ( !db )
   {
      roadmap_log( ROADMAP_ERROR, "TTS cache remove failed - cannot open database" );
   }

   /*
    * Prepare the string
    */
   sprintf( stmt_string, TTS_DB_SQLITE_STMT_DROP_TABLE, _table_name( voice_id ) );

   roadmap_log( ROADMAP_DEBUG, "TTS SQLite Destroy voice. Query string: %s. Voice: %s", stmt_string, SAFE_STR( voice_id ) );

   /*
    * Prepare the sqlite statement
    */
   ret_val = sqlite3_prepare( db, stmt_string, -1, &stmt, NULL );
   if ( !check_sqlite_error( "preparing the SQLITE statement", ret_val ) )
   {
      return FALSE;
   }

   /*
    * Evaluate
    */
   ret_val = sqlite3_step( stmt );
   if ( ret_val != SQLITE_DONE )
   {
      check_sqlite_error( "statement evaluation", ret_val );
   }

   /*
    * Finalize
    */
   sqlite3_finalize( stmt );

   /*
    * Explicitly commit drop table statement
    */
   _trans_commit();

   /*
    * Close the database
    */
   if ( sgConLifetime == _con_lifetime_session  && !sgIsInTransaction )
   {
      sqlite3_close( db );
   }

   return TRUE;
}

/*
 ******************************************************************************
 */
void tts_db_sqlite_destroy( void )
{
   const char *db_path = _get_db_file();

   // Close gracefully
   if ( sgIsInTransaction )
   {
      _trans_rollback();
   }
   else
   {
      _close_db();
   }

   // Reset state
   sgSQLiteDb = NULL;

   // Remove the db file
   roadmap_file_remove( db_path, NULL );
}

/***********************************************************/
/*  Name        : _check_sqlite_error_line()
 *  Purpose     : Auxiliary function. Checks error code
 *                  and prints sqlite message to the log inluding the line of the error statement
 *  Params     : [in] errstr - custom string
 *          : [in] code - error code
 *          : [in] line - line in the source code
 *          :
 */
static BOOL _check_sqlite_error_line( const char* errstr, int code, int line )
{

   if ( code != SQLITE_OK )
   {
      const char* errmsg = sgSQLiteDb ? sqlite3_errmsg( sgSQLiteDb ) : "";
      errstr = errstr ? errstr : "";
      roadmap_log( ROADMAP_MESSAGE_ERROR, __FILE__, line, "SQLite error in %s executing sqlite statement. Error : %d ( %s )",
            errstr, code, errmsg );
      return FALSE;
   }
   return TRUE;
}

static const char* _get_db_file( void )
{
   static char full_path[TTS_DB_SQLITE_PATH_MAXSIZE] = {0};

   if ( !full_path[0] )
   {
      const char *path = roadmap_path_tts();
      roadmap_path_format( full_path, sizeof( full_path ), path, TTS_DB_SQLITE_FILE_NAME );
   }
   return full_path;
}



/***********************************************************/
/*  Name        : _get_db()
 *  Purpose     : Auxiliary function. Opens the database file and returns the sqlite database handle
 *  Params     : [in] void
 *          :
 *          :
 */
static sqlite3* _get_db( void )
{

   char* error_msg;
   const char* full_path = _get_db_file();     //  One time only for the current fips

   if ( ( sgConLifetime == _con_lifetime_application || sgIsInTransaction ) && sgSQLiteDb )
      return sgSQLiteDb;

   if ( !full_path[0] )
   {
      roadmap_log( ROADMAP_ERROR, "Can't define the database filename" );
      return sgSQLiteDb;
   }

   check_sqlite_error( "opening database", sqlite3_open( full_path, &sgSQLiteDb ) );

   check_sqlite_error( "pragma synchronous off", sqlite3_exec( sgSQLiteDb, TTS_DB_SQLITE_STMT_SYNC_OFF, NULL, 0, &error_msg ) );

   check_sqlite_error( "pragma count changes off", sqlite3_exec( sgSQLiteDb, TTS_DB_SQLITE_STMT_CNT_OFF, NULL, 0, &error_msg ) );

   check_sqlite_error( "pragma temp storage memory", sqlite3_exec( sgSQLiteDb, TTS_DB_SQLITE_STMT_TMP_STORE_MEM, NULL, 0, &error_msg ) );

   check_sqlite_error( "pragma cache size", sqlite3_exec( sgSQLiteDb, TTS_DB_SQLITE_STMT_CACHE_SIZE, NULL, 0, &error_msg ) );

   check_sqlite_error( "pragma encoding", sqlite3_exec( sgSQLiteDb, TTS_DB_SQLITE_STMT_ENCODING, NULL, 0, &error_msg ) );

   if ( sgSQLiteDb != NULL )
   {
      check_sqlite_error( "pragma page size", sqlite3_exec( sgSQLiteDb, TTS_DB_SQLITE_STMT_PAGE_SIZE, NULL, 0, &error_msg ) );
   }

   return sgSQLiteDb;
}

/***********************************************************/
/*  Name        : _close_db()
 *  Purpose     : Auxiliary function. Closes the database
 *  Params     : void
 *          :
 *          :
 */
static void _close_db( void )
{
   if ( sgSQLiteDb )
   {
      check_sqlite_error( "Close DB", sqlite3_close( sgSQLiteDb ) );
      sgSQLiteDb = NULL;
   }
}
/***********************************************************/
/*  Name        : _trans_open( void )
 *  Purpose     : Auxiliary function. Opens the transactions. Sets the state to indicate that in transaction now
 *  Params      : [in] table - table to work on
 *              :
 *              :
 */
static sqlite3*  _trans_open( const char* table_name )
{
   int ret_val;
   sqlite3*  db = sgSQLiteDb;
   roadmap_log( ROADMAP_DEBUG, "Transaction open %d, %d ", sgIsInTransaction, sgTransStmtsCount );
   if ( sgIsInTransaction )
   {
      sgTransStmtsCount++;
      if ( sgTransStmtsCount >= TTS_DB_SQLITE_TRANS_STMTS_CNT )
      {
         roadmap_log( ROADMAP_DEBUG, "Transaction statements number exceeded - committing" );
         roadmap_main_remove_periodic( _trans_timeout );
         _trans_commit();
      }
      else
      {
         return db;
      }
   }

   db = _get_db();
   if ( db != NULL  )
   {
      if ( table_name != NULL )
      {
         char create_table_query[TTS_DB_SQLITE_QUERY_MAXSIZE];
         char* error_msg;

         // Creating the table if not exists
         sprintf( create_table_query, TTS_DB_SQLITE_STMT_CREATE_TABLE, table_name );
         check_sqlite_error( "creating table", sqlite3_exec( db, create_table_query, NULL, 0, &error_msg ) );
      }

      roadmap_log( ROADMAP_DEBUG, "Transaction open %d, %d ", sgIsInTransaction, sgTransStmtsCount );

      ret_val = sqlite3_exec( db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

      if ( check_sqlite_error( "Begin transaction", ret_val ) )
      {
         sgIsInTransaction = TRUE;
      }
      sgTransStmtsCount = 0;
      roadmap_main_set_periodic( TTS_DB_SQLITE_TRANS_TIMEOUT, _trans_timeout );
   }
   else
   {
      roadmap_log( ROADMAP_ERROR, "Begin transaction failed - cannot open database" );
   }

   return db;
}

/***********************************************************/
/*  Name        : _trans_commit( void )
 *  Purpose     : Auxiliary function. Commits the transaction to the DB
 *  Params     :
 *          :
 *          :
 */
static void _trans_commit( void )
{

//   int ret_val;

   if ( !sgSQLiteDb )
   {
      roadmap_log( ROADMAP_ERROR, "Commit transaction failed - cannot open database" );
      return;
   }
   check_sqlite_error( "Commit transaction", sqlite3_exec( sgSQLiteDb, "COMMIT;", NULL, NULL, NULL) );

   roadmap_log( ROADMAP_DEBUG, "Transaction committed %d, %d", sgIsInTransaction, sgTransStmtsCount );

   /*
    * Close the database
    */
   if ( sgConLifetime == _con_lifetime_session )
   {
      _close_db();
   }

   sgIsInTransaction = FALSE;
   sgTransStmtsCount = 0;
}

/***********************************************************/
/*  Name        : _trans_rollback( void )
 *  Purpose     : Auxiliary function. Rollbacks the transaction
 *  Params     :
 *          :
 */
static void _trans_rollback( void )
{

//   int ret_val;

   if ( !sgSQLiteDb )
   {
      roadmap_log( ROADMAP_ERROR, "Rollback transaction failed - cannot open database" );
      return;
   }

   check_sqlite_error( "Rollback transaction", sqlite3_exec( sgSQLiteDb, "ROLLBACK;", NULL, NULL, NULL) );
   /*
    * Close the database
    */
   if ( sgConLifetime == _con_lifetime_session )
   {
      _close_db();
   }
   sgIsInTransaction = FALSE;
   sgTransStmtsCount = 0;
}

/***********************************************************/
/*  Name       : _trans_timeout( void )
 *  Purpose    : Auxiliary function. Commits the transaction on timer timeout
 *  Params     :
 *             :
  */
static void _trans_timeout( void )
{
   roadmap_log( ROADMAP_DEBUG, "Transaction timeout expired - committing" );
   if ( sgIsInTransaction )
   {
      roadmap_main_remove_periodic( _trans_timeout );
      _trans_commit();
   }
}
/***********************************************************/
/*  Name       : _table_name( void )
 *  Purpose    : Table name parser. Builds valid table names from the requested ones
 *  Params     :
 *             :
  */
static const char* _table_name( const char* _name )
{
   static char s_table_name[TTS_DB_SQLITE_QUERY_MAXSIZE];

   char* pCh = &s_table_name[0];

   strncpy_safe( s_table_name, _name, TTS_DB_SQLITE_QUERY_MAXSIZE );

//   for( ; *pCh; pCh++ )
//   {
//      if ( *pCh == '-')
//         *pCh = '_';
//   }
   return s_table_name;
}
