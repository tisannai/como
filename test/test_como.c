#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <plinth.h>

char buf_command[ 1024 ];
char buf_result[ 1024 ];
char buf_golden[ 1024 ];


int validate( const char* file1, const char* file2 )
{
#if 1
    /* Testing mode. */
    sprintf( buf_command, "diff %s %s", file1, file2 );
    return system( buf_command );
#else
    /* Training mode. */
    sprintf( buf_command, "cp %s %s", file1, file2 );
    system( buf_command );
    return 0;
#endif
}

int run_test( char* test_name )
{
    plcm_s command;
    plcm_s result;
    plcm_s golden;

    plcm_use( &command, buf_command, 1024 );
    plcm_use( &result, buf_result, 1024 );
    plcm_use( &golden, buf_golden, 1024 );

    /* Compile test program. */
    plss_reformat_string( &command,
                          "gcc -Wall -g test/como_%s.c src/como.c -lplinth -o test/como_%s",
                          test_name,
                          test_name );
    system( plss_string( &command ) );
    system( "mkdir -p test/result" );

    /* Prepare test result file. */
    plss_reformat_string( &result, "test/result/%s.txt", test_name );
    plss_reformat_string( &golden, "test/golden/%s.txt", test_name );

    plss_reformat_string(
        &command, "rm -f %s; touch %s", plss_string( &result ), plss_string( &result ) );
    system( plss_string( &command ) );

    /* Read commands from test file, line by line, and push results to
       result file. */
    plcm_s    cmd_file_content;
    plsr_s    cmd_file;
    plsr_s    line;
    pl_size_t offset;

    plcm_empty( &cmd_file_content, 0 );
    plss_reformat_string( &command, "test/test_%s", test_name );
    plss_read_file( &cmd_file_content, plss_string( &command ) );
    cmd_file = plsr_from_plcm( &cmd_file_content );
    line = plsr_null();

    offset = 0;
    line = plsr_next_line( cmd_file, &offset );
    while ( !plsr_is_null( line ) ) {
        /* Filter out empty command lines. */
        if ( !plsr_is_empty( line ) ) {
            plss_reformat_string( &command,
                                 "echo \"---- CMD: %.*s\" >> %s",
                                 plsr_length( line ),
                                 plsr_string( line ),
                                 plss_string( &result ) );
            system( plss_string( &command ) );
            plss_reformat_string( &command,
                                 "test/%.*s >> %s 2>&1",
                                 plsr_length( line ),
                                 plsr_string( line ),
                                 plss_string( &result ) );
            system( plss_string( &command ) );
        }
        line = plsr_next_line( cmd_file, &offset );
    }

    /* Check that result matches golden. */
    TEST_ASSERT( validate( plss_string( &result ), plss_string( &golden ) ) == 0 );

    return 0;
}


void test_options( void )
{
    run_test( "options" );
}


void test_config( void )
{
    run_test( "config" );
}


void test_subcmd( void )
{
    run_test( "subcmd" );
}


void test_type_prim( void )
{
    run_test( "type_prim" );
}
