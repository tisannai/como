/**
 * @file mcs.c
 *
 * @brief Extensions for easy string creation within mcc.
 *
 */


#include <mc.h>
#include "mcc.h"


mcc_p mcc_printf( mcc_p aa, const char* format, ... )
{
  va_list ap;
  va_start( ap, format );
  return mcc_vprintf( aa, format, ap );
}


mcc_p mcc_reprintf( mcc_p aa, const char* format, ... )
{
  va_list ap;
  va_start( ap, format );
  if ( aa )
    mcc_reset( aa );
  return mcc_vprintf( aa, format, ap );
}


mcc_p mcc_vprintf( mcc_p aa, const char* format, va_list ap )
{
  int len;
  va_list coap;

  va_copy( coap, ap );

  len = vsnprintf( NULL, 0, format, ap );
  va_end( ap );

  if ( aa )
    {
      mcc_resize( aa, aa->used+len+1 );
    }
  else
    {
      aa = mcc_new_size( len+1 );
      aa->resize = mcc_enlarge_resizer;
    }

  vsprintf( &( aa->data[aa->used] ), format, coap );
  va_end( coap );

  aa->used += len;

  return aa;
}



char* mcc_strip( mcc_p aa )
{
  char* data = aa->data;
  mcc_terminate( aa );
  mcc_compact( aa );
  free( aa );
  return data;
}


mcc_p mcc_from_str( const char* str )
{
  mcc_p aa;
  mc_size_t strsize = strlen( str );
  mc_size_t size;

  size = strsize / mcc_sizeof;
  if ( strsize % mcc_sizeof != 0 )
    size += 1;
  aa = mcc_new_size( size );
  mc_memcpy( str, aa->data, strsize );
  return aa;
}


const char* mcc_as_str( mcc_p aa )
{
  return ( const char* ) aa->data;
}


const char* mcc_to_str( mcc_p aa )
{
  mcc_resize( aa, aa->used+1 );
  aa->data[ aa->used ] = 0;
  return ( const char* ) aa->data;
}


const char* mcc_to_cstr( mcc_p aa, char nuller )
{
  for ( int i = 0; i < aa->used; i++ )
    if ( aa->data[ i ] == 0 )
      aa->data[ i ] = nuller;

  return mcc_to_str( aa );
}


void mcc_chomp( mcc_p aa )
{
  if ( aa->data[ aa->used-1 ] == (char) '\n' )
    aa->used--;
}


void mcc_trim_with( mcc_p aa, char trim )
{
  if ( aa->data[ aa->used-1 ] == trim )
    aa->used--;
}


mc_size_t mcc_format_size( const char* format, ... )
{
  va_list ap;
  int len;

  va_start( ap, format );
  len = vsnprintf( NULL, 0, format, ap );
  va_end( ap );

  return len;
}


char* mcc_str_concat( const char* first, ... )
{
  mc_size_t len;	
  char* ret;
  va_list argp;
  char* ts;

  if ( first == NULL )
    return NULL;

  len = strlen( first );

  /* First scan. "first" is the last required argument (ironic). */
  va_start( argp, first );

  while ( ( ts = va_arg( argp, char* ) ) != NULL )
    len += strlen( ts );

  va_end( argp );

  ret = mc_new_n( char, len+1 );

  if ( ret == NULL )
    return NULL;

  strcpy( ret, first );

  /* Second scan. */
  va_start( argp, first );

  while ( ( ts = va_arg( argp, char* ) ) != NULL )
    strcat( ret, ts );

  va_end( argp );

  return ret;
}
