/* roadmap_ssl.c - Open SSL integration
 *
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
 *   See roadmap_ssl.h
 *       roadmap_net.c
 *
 */

#include "roadmap.h"
#include "roadmap_ssl.h"
#include "roadmap_net.h"
#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>

//======================== Local defines ========================

//======================== Local types ========================

struct roadmap_ssl_io_t {
   RoadMapSocket  s;
   void           *data;
   SSL_CTX *ctx;
   SSL *ssl;
   RoadMapNetSslConnectCallback on_connect; // TODO: Necessary ???
};

//======================== Local Declarations ========================

static const char* last_err_string( void );
static void log_ssl( void );

//======================== Globals ========================


/*
 ******************************************************************************
 */
int roadmap_ssl_open ( RoadMapSocket s, void *data, RoadMapNetSslConnectCallback callback )
{
   static BOOL initialized = FALSE;
   const SSL_METHOD *method;
   SSL_CTX *ctx;
   SSL *ssl;

   // Initialize the library
   if ( !initialized )
   {
      SSL_library_init();
   }
   /*
    * Initialize the ssl context
    */
   OpenSSL_add_all_algorithms();      /* Load cryptos, et.al. */
   SSL_load_error_strings();          /* Bring in and register error messages */
   method = SSLv23_client_method();    /* Create new client-method instance */
   ctx = SSL_CTX_new( method );       /* Create new context */
   if ( ctx == NULL )
   {
      roadmap_log( ROADMAP_ERROR, "Error obtaining ssl context: %s !!", last_err_string() );
      return 0;
   }

   ssl = SSL_new( ctx );                /* SSL connection state */
   SSL_set_fd( ssl, roadmap_net_get_fd( s ) );             /* Attach the socket */

   /*
    * Initialize the io context
    */

   RoadMapSslIO ssl_io_ctx = malloc( sizeof( struct roadmap_ssl_io_t ) );
   roadmap_check_allocated( ssl_io_ctx );

   ssl_io_ctx->ctx = ctx;
   ssl_io_ctx->ssl = ssl;
   ssl_io_ctx->s = s;
   ssl_io_ctx->data = data;
   ssl_io_ctx->on_connect = callback;

   if ( SSL_connect(ssl) == -1 )       /* perform the connection */
   {
      roadmap_log( ROADMAP_ERROR, "Error connecting ssl!!" );
      log_ssl();
      callback( s, data, ssl_io_ctx, err_net_failed );
      return 0;
   }

   callback( s, data, ssl_io_ctx, succeeded );

   return 1;
}

/*
 ******************************************************************************
 */

int roadmap_ssl_read (void *context, void *buffer, int buffer_size)
{
   int read;
   RoadMapSslIO ssl_io_ctx = (RoadMapSslIO) context;
   int received = 0;
   int err;
   do
   {
      read = SSL_read( ssl_io_ctx->ssl, buffer, buffer_size );

      buffer_size -= read;
      buffer += read;

      if ( ( err = SSL_get_error( ssl_io_ctx->ssl, read ) ) == SSL_ERROR_NONE )
      {
         received += read;
      }
      else
      {
         roadmap_log ( ROADMAP_ERROR, "Error (%d) in SSL_read: %s !!", err, ERR_error_string( err, NULL ) );
         received = 0;
         break;
      }

   } while ( SSL_pending( ssl_io_ctx->ssl ) && ( buffer_size > 0 ) );

   return received;
}

/*
 ******************************************************************************
 */

int roadmap_ssl_send ( RoadMapSslIO io, const void *data, int length)
{
   RoadMapSslIO ssl_io_ctx = io;
   int written = 0;
   int ret_code = SSL_write( ssl_io_ctx->ssl, data, length );

   switch( SSL_get_error( ssl_io_ctx->ssl, ret_code ) )
   {
             /* We wrote something*/
             case SSL_ERROR_NONE:
               written = ret_code;
               break;

             /* We would have blocked */
             case SSL_ERROR_WANT_WRITE:
                written = 0;
               break;
             case SSL_ERROR_WANT_READ:
                written = 0;
               break;

             /* Some other error */
             default:
                written = -1;
   }
   return written;
}

/*
 ******************************************************************************
 */
void roadmap_ssl_close ( void *context )
{
   RoadMapSslIO ssl_io_ctx = (RoadMapSslIO) context;
   if (context){
      SSL_free( ssl_io_ctx->ssl );                        /* release connection state */
      SSL_CTX_free( ssl_io_ctx->ctx );
   }
}

/*
 ******************************************************************************
 */
static const char* last_err_string( void )
{
   unsigned long last_error = ERR_peek_last_error();
   return ERR_error_string( last_error, NULL );
}
/*
 ******************************************************************************
 */
static void log_ssl( void )
{
  char buf[256];
  u_long err;
  while ( (err = ERR_get_error()) != 0 )
  {
     ERR_error_string_n(err, buf, sizeof(buf));
     roadmap_log( ROADMAP_ERROR, "SSL ERROR: *** %s", buf );
  }
}

