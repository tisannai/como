/**
 * @file mcc.c
 *
 * Automatic allocation for array of mcc.
 */

/*
 * Common headers:
 */
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <mc.h>
#include "mcc.h"


/* autoc:version. */
const char* mcc_version = "0.0.1";



/*
 * *************************************************************
 * Auto-resize memory allocation.
 */


mc_bool_t mcc_default_resizer( mcc_p aa, mc_size_t newsize )
{
  if ( newsize > aa->size )
    {
      while ( newsize > aa->size )
        aa->size = aa->size*2;
      mcc_resize_to( aa, aa->size );
      return mc_true;
    }
  else if ( newsize < aa->size/2 )
    {
      /* Ensure that size is same or more than 1 and ensure that size is
         always bigger than aa->used. */
      while ( ( aa->size / 2 ) >= 1 &&
              ( aa->size / 2 ) > newsize )
        aa->size /= 2;
      mcc_resize_to( aa, aa->size );
      return mc_true;
    }
  else
    return mc_false;
}


mc_bool_t mcc_enlarge_resizer( mcc_p aa, mc_size_t newsize )
{
  if ( newsize > aa->size )
    {
      while ( newsize > aa->size )
        aa->size = aa->size*2;
      mcc_resize_to( aa, aa->size );
      return mc_true;
    }
  else
    return mc_false;
}


/** Resize function pointer for Autoarr Memory. */
mcc_resize_func_t mcc_resize_func = mcc_default_resizer;


mcc_p mcc_new_size( mc_size_t size )
{
  mcc_p aa;

  assert( size >= 1 );

  aa = mc_new_n( mcc_t, 1 );
  aa->size = size;
  aa->used = 0;
  aa->data = mc_new_n( char, size );
  aa->resize = mcc_default_resizer;

  return aa;
}


mcc_p mcc_new( void )
{
  return mcc_new_size( MCC_DEFAULT_SIZE );
}


mcc_p mcc_del( mcc_p aa )
{
  mc_del( (void*) aa->data );
  mc_del( aa );
  return NULL;
}


void mcc_copy( mcc_p aa, mcc_p to )
{
  if ( aa->used > to->used )
    mcc_resize_to( to, aa->used );
  to->used = aa->used;
//  memcpy( (void*) to->data, (void*) aa->data, mcc_usedsize(aa) );
  mc_memcpy( aa->data, to->data, mcc_usedsize(aa) );
}


mcc_p mcc_dup( mcc_p aa )
{
  mcc_p dup;
  dup = mcc_new_size( aa->size );
  mcc_copy( aa, dup );
  return dup;
}


void mcc_reset( mcc_p aa )
{
  aa->used = 0;
}


void mcc_delete_all( mcc_p aa )
{
  aa->used = 0;
}



void mcc_resize( mcc_p aa, mc_size_t size )
{
  aa->resize( aa, size );
}



void mcc_resize_to( mcc_p aa, mc_size_t size )
{
  aa->size = size;
  aa->data = ( char* ) mc_realloc( (void*) aa->data, mcc_bytesize(aa) );
}


void mcc_compact( mcc_p aa )
{
  aa->size = aa->used;
  aa->data = ( char* ) mc_realloc( (void*) aa->data, mcc_bytesize(aa) );
}


void mcc_insert_n_to( mcc_p aa, mc_size_t pos, char* data, mc_size_t len )
{

  /* Disallow holes. */
  assert( pos <= aa->used );

  mcc_resize( aa, aa->used+len );

  /* Move data to make room for new. */
  if ( pos < aa->used )
    {
      mc_memmove( &( aa->data[pos] ),
                  &( aa->data[ pos+len ] ),
                  ( aa->used - pos ) * mcc_sizeof );
    }

  mc_memcpy( data, &( aa->data[pos] ), len * mcc_sizeof );

  aa->used += len;
}


void mcc_insert_to( mcc_p aa, mc_size_t pos, char data )
{
  mcc_insert_n_to( aa, pos, &data, 1 );
}


void mcc_delete_n_at( mcc_p aa, mc_size_t pos, mc_size_t len )
{

  /* Disallow holes. */
  assert( pos <= aa->used );
  assert( pos+len <= aa->size );

  /* Move data to make room for new. */
  if ( pos < aa->used )
    mc_memmove( &( aa->data[ pos+len ] ),
                &( aa->data[ pos ] ),
                ( aa->used - pos - len ) * mcc_sizeof );

  aa->used -= len;

  mcc_resize( aa, aa->used-len );
}


void mcc_delete_at( mcc_p aa, mc_size_t pos )
{
  mcc_delete_n_at( aa, pos, 1 );
}


void mcc_delete_n_end( mcc_p aa, mc_size_t len )
{
  /* Check for holes in rem. */
  assert( len <= aa->used );

  aa->used -= len;
  mcc_resize( aa, aa->used );
}


void mcc_assign_to( mcc_p aa, mc_size_t pos, char* data, mc_size_t len )
{
  /* Overwrite. */
  mc_size_t ow;

  ow = aa->used - pos;

  /* Check for holes in set. */
  assert( ow >= 0 );

  if ( ( len - ow ) > 0 )
    {
      /* Grow before copy. */
      mcc_resize( aa, ( aa->used + len - ow ) );
      aa->used += ( len - ow );
    }

  /* Copy new data. */
  mc_memcpy( data, &( aa->data[ pos ] ), len * mcc_sizeof );

}


void mcc_assign( mcc_p aa, char* data, mc_size_t len )
{
  mcc_reset( aa );
  mcc_insert_n_to( aa, 0, data, len );
}


void mcc_append( mcc_p aa, char data )
{
  mcc_insert_n_to( aa, aa->used, &data, 1 );
}


void mcc_append_n( mcc_p aa, char* data, mc_size_t len )
{
  mcc_insert_n_to( aa, aa->used, data, len );
}


mc_bool_t mcc_append_unique( mcc_p aa, char data )
{
  if ( !mcc_find( aa, data ) )
    {
      mcc_append( aa, data );
      return mc_true;
    }
  else
    {
      return mc_false;
    }
}


void mcc_prepend( mcc_p aa, char data )
{
  mcc_insert_n_to( aa, 0, &data, 1 );
}


void mcc_prepend_n( mcc_p aa, char* data, mc_size_t len )
{
  mcc_insert_n_to( aa, 0, data, len );
}


mc_size_t mcc_find_idx( mcc_p aa, char data )
{
  for ( mc_size_t i = 0; i < aa->used; i++ )
    {
      if ( aa->data[ i ] == data )
        return i;
    }

  return MCC_INVALID_INDEX;
}


mc_bool_t mcc_find( mcc_p aa, char data )
{
  mc_size_t idx;

  idx = mcc_find_idx( aa, data );

  if ( idx != MCC_INVALID_INDEX )
    return mc_true;
  else
    return mc_false;
}


void mcc_terminate( mcc_p aa )
{
  if ( aa->data[ aa->used-1 ] != 0 )
    {
      mcc_append( aa, 0 );
      aa->used--;
    }
}


void mcc_push( mcc_p s, char item )
{
  mcc_append( s, item );
}


char mcc_pop( mcc_p s )
{
  char d;

  d = s->data[ s->used-1 ];
  mcc_delete_n_end( s, 1 );
  return d;
}


char mcc_peek( mcc_p s )
{
  return s->data[ s->used-1 ];
}


mc_bool_t mcc_empty( mcc_p aa )
{
  if ( aa->used == 0 )
    return mc_true;
  else
    return mc_false;
}
