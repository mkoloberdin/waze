/*
 * resolver.h
 * pthread based async resolver interface
 *
 * LICENSE:
 *   Copyright 2011, Waze Ltd      Alex Agranovich (AGA)
 *
 */
#ifndef INCLUDE__RESOLVER__H
#define INCLUDE__RESOLVER__H

#include <arpa/inet.h>

typedef void (*ResolverRequestCb) ( const void* context, in_addr_t ip_addr );

in_addr_t resolver_request( const char* domain, ResolverRequestCb callback, const void* context );
void resolver_handler( int msg );
void resolver_shutdown( void );
void resolver_init( void );
in_addr_t resolver_find( const char* domain );

#endif /* INCLUDE__RESOLVER__H */
