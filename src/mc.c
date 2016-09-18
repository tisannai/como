/**
 * @file mc.c
 *
 * @brief Mc library is small c-lib with basic memory allocation
 *   functions and definitions.
 */

#include "mc.h"


/* autoc:version. */
const char* mc_version = "0.0.1";


mc_p mc_xmalloc( mc_size_t size )
{
  register mc_p mem;

  if ( size == 0 )
    return nil;

  mem = malloc( size );

  if ( mem == 0 )
    fputs(  "Virtual memory exhausted!", stderr );

  /* Init memory with 0. */
  memset( mem, 0, size );

  return mem;
}


mc_p mc_memdup( const mc_p source, mc_size_t size )
{
  register mc_p mem;

  mem = malloc( size );
  memcpy( mem, source, size );
  return mem;
}


mc_bool_t mc_memdiff( const mc_p d1, const mc_p d2, mc_size_t len )
{
  register char* c1 = (char*) d1;
  register char* c2 = (char*) d2;
  mc_size_t i = 0;

  while ( *c1 == *c2 && i < len )
    {
      c1++; c2++; i++;
    }
      
  if ( i == len )
    return mc_true;
  else
    return mc_false;
}


mc_p mc_free( mc_p item )
{
  free( item );
  return nil;
}


void mc_msg_with_prefix( FILE* io,
                         const char* prefix,
                         const char* format,
                         va_list ap )
{
  fputs( prefix, io );
  vfprintf( io, format, ap );
  fputs( "\n", io );
}


void mc_break( void ) { return; }
