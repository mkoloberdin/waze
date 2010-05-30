/* roadmap_tile_storage_sqlite.c - Tiles storage management using sqlite engine
 *
 * LICENSE:
 *
 *   Copyright 2010 Alex Agranovich (AGA),     Waze Ltd
 *
 *   This file is part of Waze.
 *
 *   Waze is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Waze is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Waze; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "roadmap.h"
#include "roadmap_locator.h"
#include "roadmap_performance.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_main.h"

typedef enum
{
	_con_lifetime_session = 0x0,		// Connection remains active only within one statement execution
	_con_lifetime_application,			// Connection remains active within application
} RMTileStorageConLifetime;


#define	  RM_TILE_STORAGE_QUERY_MAXSIZE 		1024
#define	  RM_TILE_STORAGE_DB_NAME_SIZE 			32
#define	  RM_TILE_STORAGE_DB_PATH_MAXSIZE 		512
#define	  RM_TILE_STORAGE_TRANS_TIMEOUT			2000L			// Transaction timeout in msec
#define	  RM_TILE_STORAGE_TRANS_STMTS_CNT		200			// Transaction timeout in msec
#define   RM_TILE_STORAGE_DB_PREFIX 			"tiles_"
#define   RM_TILE_STORAGE_DB_SUFFIX 			".db"
#define   RM_TILE_STORAGE_TILES_TABLE 			"tiles_table"
#define   RM_TILE_STORAGE_TILES_TABLE_ID		"id"
#define   RM_TILE_STORAGE_TILES_TABLE_DATA		"data"

#define   RM_TILE_STORAGE_STMT_CREATE_TABLE		"CREATE TABLE IF NOT EXISTS tiles_table(id INTEGER PRIMARY KEY, data BLOB)"
#define   RM_TILE_STORAGE_STMT_STORE		    "INSERT OR REPLACE INTO tiles_table values (%d,?);"
#define   RM_TILE_STORAGE_STMT_LOAD		        "SELECT data FROM tiles_table WHERE id=?;"
#define   RM_TILE_STORAGE_STMT_REMOVE	        "DELETE FROM tiles_table WHERE id=?;"
#define   RM_TILE_STORAGE_STMT_SYNC_OFF 		"PRAGMA synchronous = OFF"
#define   RM_TILE_STORAGE_STMT_CNT_OFF			"PRAGMA count_changes = OFF"
#define   RM_TILE_STORAGE_STMT_TMP_STORE_MEM	"PRAGMA temp_store = MEMORY"
#define   RM_TILE_STORAGE_STMT_CACHE_SIZE		"PRAGMA cache_size = 2000"
#define   RM_TILE_STORAGE_STMT_PAGE_SIZE		"PRAGMA page_size = 8192"

static int sgCurrentFips	= -1;			// Current fips - used to avoid database
static BOOL sgTableExists   = FALSE;		// Indicates if the table exits ( in order to avoid unnecessary queries)
static sqlite3* sgSQLiteDb  = NULL;			// The current db handle
static RMTileStorageConLifetime sgConLifetime = _con_lifetime_session;		// The type of connection lifetime

static BOOL sgIsInTransaction = FALSE;
static int  sgTransStmtsCount = 0;

#define check_sqlite_error( errstr, code ) \
	check_sqlite_error_line( errstr, code, __LINE__ )

static void trans_timeout( void );
static void trans_commit( void );

/***********************************************************/
/*  Name        : roadmap_camera_image_capture()
 *  Purpose     : Auxiliary function. Returns the full path of the global tile file.
 *                  Pointer to the statically allocated memory is returned
 *  Params		: [in] fips
 *				:
 */
static const char * get_global_filename( int fips ) {

   const char *map_path = roadmap_db_map_path ();
   char name[30];
   char path[RM_TILE_STORAGE_DB_PATH_MAXSIZE];
   static char filename[RM_TILE_STORAGE_DB_PATH_MAXSIZE] = {0};

     /* Global square id */
   if ( !filename[0])
   {
	   const char *suffix = "index";
	   snprintf (name, sizeof (name), "%05d_%s%s", fips, suffix,
            ROADMAP_DATA_TYPE );
	   roadmap_path_format (filename, sizeof (filename), map_path, name);
   }
   return filename;
}


/***********************************************************/
/*  Name        : check_sqlite_error_line()
 *  Purpose     : Auxiliary function. Checks error code
 *                  and prints sqlite message to the log inluding the line of the error statement
 *  Params		: [in] errstr - custom string
 *  			: [in] code - error code
 *  			: [in] line - line in the source code
 *				:
 */
static BOOL check_sqlite_error_line( const char* errstr, int code, int line )
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

static const char* get_db_file( int fips )
{
   static char full_path[RM_TILE_STORAGE_DB_PATH_MAXSIZE] = {0};


   if ( !full_path[0] || ( fips != sgCurrentFips ) )
   {
#ifndef IPHONE_NATIVE
	  const char *map_path = roadmap_db_map_path ();
#else
      const char *map_path = roadmap_path_preferred("maps");
#endif //!IPHONE_NATIVE
      char db_name[RM_TILE_STORAGE_DB_NAME_SIZE];
      snprintf( db_name, RM_TILE_STORAGE_DB_NAME_SIZE, "%s%d%s", RM_TILE_STORAGE_DB_PREFIX, fips, RM_TILE_STORAGE_DB_SUFFIX );
      roadmap_path_format( full_path, sizeof( full_path ), map_path, db_name );
   }
   return full_path;
}
/***********************************************************/
/*  Name        : get_db()
 *  Purpose     : Auxiliary function. Opens the database file and returns the sqlite database handle
 *  Params		: [in] fips
 *  			:
 *				:
 */
static sqlite3* get_db( int fips )
{

	char* error_msg;
	const char* full_path = get_db_file( fips );     //  One time only for the current fips

	sgCurrentFips = fips;


	if ( ( sgConLifetime == _con_lifetime_application || sgIsInTransaction ) && sgSQLiteDb )
		return sgSQLiteDb;

	if ( !full_path[0] )
	{
		roadmap_log( ROADMAP_ERROR, "Can't define the database filename" );
		return sgSQLiteDb;
	}

	check_sqlite_error( "opening database", sqlite3_open( full_path, &sgSQLiteDb ) );

	check_sqlite_error( "pragma synchronous off", sqlite3_exec( sgSQLiteDb, RM_TILE_STORAGE_STMT_SYNC_OFF, NULL, 0, &error_msg ) );

	check_sqlite_error( "pragma count changes off", sqlite3_exec( sgSQLiteDb, RM_TILE_STORAGE_STMT_CNT_OFF, NULL, 0, &error_msg ) );

	check_sqlite_error( "pragma temp storage memory", sqlite3_exec( sgSQLiteDb, RM_TILE_STORAGE_STMT_TMP_STORE_MEM, NULL, 0, &error_msg ) );

	check_sqlite_error( "pragma cache size", sqlite3_exec( sgSQLiteDb, RM_TILE_STORAGE_STMT_CACHE_SIZE, NULL, 0, &error_msg ) );

	if ( sgSQLiteDb != 0 && !sgTableExists )
	{
		check_sqlite_error( "pragma page size", sqlite3_exec( sgSQLiteDb, RM_TILE_STORAGE_STMT_PAGE_SIZE, NULL, 0, &error_msg ) );

		if ( check_sqlite_error( "creating table", sqlite3_exec( sgSQLiteDb, RM_TILE_STORAGE_STMT_CREATE_TABLE, NULL, 0, &error_msg ) ) )
		{
			sgTableExists = TRUE;
		}
	}

	return sgSQLiteDb;
}

/***********************************************************/
/*  Name        : close_db()
 *  Purpose     : Auxiliary function. Closes the database
 *  Params		: void
 *  			:
 *				:
 */
static void close_db( void )
{
	if ( sgSQLiteDb )
	{
		check_sqlite_error( "Close DB", sqlite3_close( sgSQLiteDb ) );
		sgSQLiteDb = NULL;
	}
}
/***********************************************************/
/*  Name        : trans_open( void )
 *  Purpose     : Auxiliary function. Opens the transactions. Sets the state to indicate that in transaction now
 *  Params		:
 *  			:
 *				:
 */
static sqlite3*  trans_open( int fips )
{
	int ret_val;
	sqlite3*  db = sgSQLiteDb;
	roadmap_log( ROADMAP_DEBUG, "Transaction open %d, %d ", sgIsInTransaction, sgTransStmtsCount );
	if ( sgIsInTransaction )
	{
		sgTransStmtsCount++;
		if ( sgTransStmtsCount >= RM_TILE_STORAGE_TRANS_STMTS_CNT )
		{
			roadmap_log( ROADMAP_DEBUG, "Transaction statements number exceeded - committing" );
			roadmap_main_remove_periodic( trans_timeout );
			trans_commit();
		}
		else
		{
			return db;
		}
	}

	db = get_db( fips );
	if ( !db )
	{
		roadmap_log( ROADMAP_ERROR, "Begin transaction failed - cannot open database" );
		return NULL;
	}
	roadmap_log( ROADMAP_DEBUG, "Transaction open %d, %d ", sgIsInTransaction, sgTransStmtsCount );

	ret_val = sqlite3_exec( db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

	if ( check_sqlite_error( "Begin transaction", ret_val ) )
	{
		sgIsInTransaction = TRUE;
	}
	sgTransStmtsCount = 0;
	roadmap_main_set_periodic( RM_TILE_STORAGE_TRANS_TIMEOUT, trans_timeout );

	return db;
}

/***********************************************************/
/*  Name        : trans_commit( void )
 *  Purpose     : Auxiliary function. Commits the transaction to the DB
 *  Params		:
 *  			:
 *				:
 */
static void trans_commit( void )
{

	int ret_val;

	if ( !sgSQLiteDb )
	{
		roadmap_log( ROADMAP_ERROR, "Commit transaction failed - cannot open database" );
		return;
	}
	check_sqlite_error( "Commit transaction", sqlite3_exec( sgSQLiteDb, "COMMIT;", NULL, NULL, NULL) );

	/*
	 * Close the database
	 */
	if ( sgConLifetime == _con_lifetime_session )
	{
		close_db();
	}

	sgIsInTransaction = FALSE;
	sgTransStmtsCount = 0;
}

/***********************************************************/
/*  Name        : trans_rollback( void )
 *  Purpose     : Auxiliary function. Rollbacks the transaction
 *  Params		:
 *  			:
 *				:
 */
static void trans_rollback( void )
{

	int ret_val;

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
		close_db();
	}
	sgIsInTransaction = FALSE;
	sgTransStmtsCount = 0;
}

/***********************************************************/
/*  Name        : trans_timeout( void )
 *  Purpose     : Auxiliary function. Commits the transaction on timer timeout
 *  Params		:
 *  			:
 *				:
 */
static void trans_timeout( void )
{
	roadmap_log( ROADMAP_DEBUG, "Transaction timeout expired - committing" );
	if ( sgIsInTransaction )
	{
		roadmap_main_remove_periodic( trans_timeout );
		trans_commit();
	}
}

/***********************************************************/
/*  Name        : roadmap_tile_store()
 *  Purpose     : Interface function. Stores the tile to the database
 *  Params		: [in] fips
 *  			: [in] tile_index - primary key
 *  			: [in] data - the pointer to the blob data
 *  			: [in] data - the size of the blob data block
 *				:
 */
int roadmap_tile_store( int fips, int tile_index, void *data, size_t size )
{
	int res = 0;
	sqlite3* db = NULL;
	sqlite3_stmt *stmt = NULL;
	int ret_val;
	char stmt_string[RM_TILE_STORAGE_QUERY_MAXSIZE];

	// db = get_db( fips );
	db = trans_open( fips );

	if ( !db )
	{
		roadmap_log( ROADMAP_ERROR, "Tile storage failed - cannot open database" );
		return -1;
	}
	/*
	 * Prepare the string
	 */
	sprintf( stmt_string, RM_TILE_STORAGE_STMT_STORE, tile_index );

	/*
	 * Prepare the sqlite statement
	 */
	ret_val = sqlite3_prepare( db, stmt_string, -1, &stmt, NULL );
	if ( !check_sqlite_error( "preparing the SQLITE statement", ret_val ) )
	{
		return -1;
	}
	/*
	 * Binding the data
	 */
	ret_val = sqlite3_bind_blob( stmt, 1, data, size, NULL );
	if ( !check_sqlite_error( "binding the blob statement", ret_val ) )
	{
		return -1;
	}
	/*
	 * Evaluate
	 */
	sqlite3_step( stmt );
	if ( !check_sqlite_error( "finishing", sqlite3_finalize( stmt ) ) )
	{
		return -1;
	}

	/*
	 * Close the database
	 */
	if ( sgConLifetime == _con_lifetime_session && !sgIsInTransaction )
	{
		sqlite3_close( db );
	}

	return res;
}


/***********************************************************/
/*  Name        : roadmap_tile_remove
 *  Purpose     : Interface function. Removes the tile from the database in one transaction
 *  Params		: [in] fips
 *  			: [in] tile_index - primary key
 *  			:
 *				:
 */
void roadmap_tile_remove( int fips, int tile_index )
{
	sqlite3* db = NULL;
	sqlite3_stmt *stmt = NULL;
	int ret_val;

//	db = get_db( fips );
	db = trans_open( fips );

	if ( !db )
	{
		roadmap_log( ROADMAP_ERROR, "Tile remove failed - cannot open database" );
	}

	/*
	 * Prepare the sqlite statement
	 */
	ret_val = sqlite3_prepare( db, RM_TILE_STORAGE_STMT_REMOVE, -1, &stmt, NULL );
	if ( !check_sqlite_error( "preparing the SQLITE statement", ret_val ) )
	{
		return;
	}
	/*
	 * Binding the parameter
	 */
	ret_val = sqlite3_bind_int( stmt, 1, tile_index );
	if ( !check_sqlite_error( "binding int parameter", ret_val ) )
	{
		return;
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
}


static int roadmap_tile_file_load ( const char *full_name, void **base, size_t *size) {

   RoadMapFile		file;
   int				res;

   file = roadmap_file_open (full_name, "r");

   if (!ROADMAP_FILE_IS_VALID(file)) {
      return -1;
   }

#ifdef J2ME
   *size = favail(file);
#else
   *size = roadmap_file_length (NULL, full_name);
#endif
   *base = malloc (*size);

	   res = roadmap_file_read (file, *base, *size);
	   roadmap_file_close (file);

   if (res != (int)*size) {
      free (*base);
      return -1;
   }

   return 0;
}

/***********************************************************/
/*  Name        : roadmap_tile_load
 *  Purpose     : Interface function. Loads the tile data from the database.
 *                 Allocates the necessar heap space
 *  Params		: [in] fips
 *  			: [in] tile_index - primary key
 *  			: [out] base - the data storage address
 *				: [out] size - the size of the data block
 */
int roadmap_tile_load (int fips, int tile_index, void **base, size_t *size)
{
	int res = -1;
	sqlite3* db = NULL;
	sqlite3_stmt *stmt = NULL;
	int ret_val;

	if ( tile_index == -1 )
	{
		const char* file_name = get_global_filename( fips );
		res = roadmap_tile_file_load( file_name, base, size );
		return res;
	}

//	db = get_db( fips );
	db = trans_open( fips );

	if ( !db )
	{
		roadmap_log( ROADMAP_ERROR, "Tile loading failed - cannot open database" );
		return -1;
	}

	/*
	 * Prepare the sqlite statement
	 */
	ret_val = sqlite3_prepare( db, RM_TILE_STORAGE_STMT_LOAD, -1, &stmt, NULL );
	if ( !check_sqlite_error( "preparing the SQLITE statement", ret_val ) )
	{
		return -1;
	}
	/*
	 * Binding the parameter
	 */
	ret_val = sqlite3_bind_int( stmt, 1, tile_index );
	if ( !check_sqlite_error( "binding int parameter", ret_val ) )
	{
		return -1;
	}

	/*
	 * Evaluate
	 */
	ret_val = sqlite3_step( stmt );

	if ( ret_val == SQLITE_ROW )
	{
		const void* data;
		*size = sqlite3_column_bytes( stmt, 0 );
		*base = malloc( *size );
		roadmap_check_allocated( *base );
		data = sqlite3_column_blob( stmt, 0 );

		memcpy( *base, data, *size );
		res = 0;
	}
	else
	{
		res = -1;
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


/***********************************************************/
/*  Name        : roadmap_tile_remove_all
 *  Purpose     : Removes the entire database
 *
 *  Params     : [in] fips
 *             : [in] tile_index - primary key
 *             : [out] base - the data storage address
 *             : [out] size - the size of the data block
 */
void roadmap_tile_remove_all( int fips )
{
   const char *db_path = get_db_file( fips );

   // Close gracefully
   if ( sgIsInTransaction )
   {
      trans_rollback();
   }
   else
   {
      close_db();
   }


   // Reset state
   sgTableExists = FALSE;
   sgSQLiteDb = NULL;


   // Remove the db file
   roadmap_file_remove( db_path, NULL );
}



 
