/**
 * @file como_options.c
 *
 * Test option types.
 */

#include <plinth.h>
#include "../src/como.h"

int main( int argc, char** argv )
{
  como_opt_p opts;
  como_opt_t o;
  
  como_command( "como_options", "Como Tester", "2013",
                 { COMO_EXCLUSIVE, "doc", NULL, "Documentation for option\n\twith too much description\n\tfor one line." },
                 { COMO_SINGLE, "file", "-f", "File argument." },
                 { COMO_SWITCH, "debug", NULL, "Enable debugging." },
                 { COMO_OPT_SINGLE, "mode", "-m", "Mode." },
                 { COMO_OPT_MULTI, "params", NULL, "Parameters." },
                 { COMO_OPT_ANY, "types", "-t", "Types." },
                 { COMO_SILENT, "terminator", "-", "The terminator." },
                 { COMO_MULTI, "dir", "-d", "Directory argument(s)." },
                 { COMO_DEFAULT, NULL, NULL, "Leftovers." },
                 );

  opts = como_cmd->opts;
  while ( *opts )
    {
      o = *opts;

      printf( "Given \"%s\": %s\n", o->name, o->given ? "true" : "false" );
       
      if ( o->given && o->value )
        {
          printf( "Value \"%s\": ", o->name );
          como_display_values( stdout, o );
          printf( "\n" );
        }

      opts++;
    }

  if ( como_cmd->external )
    {
      char** value;
      pl_bool_t first = pl_true;

      printf( "External: [" );

      value = como_cmd->external;

      while ( *value )
        {
          if ( !first )
            printf( ", " );
          printf( "\"%s\"", *value );
          first = pl_false;
          value++;
        }

      printf( "]\n" );
    }

  como_end();

  return 0;
}
