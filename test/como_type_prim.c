/**
 * @file como_options.c
 *
 * Test option types.
 */

#include "../src/mc.h"
#include "../src/como.h"

int main( int argc, char** argv )
{
  como_opt_t** opts, *o;
  
  como_command( "como_type_prim", "Como Tester", "2013",
                { COMO_P_NONE | COMO_P_OPT | COMO_P_MUTEX,
                    "doc", NULL, "Documentation for option\n\twith too much description\n\tfor one line." },
                { COMO_P_ONE, "file", "-f", "File argument." },
                { COMO_SWITCH, "debug", NULL, "Enable debugging." },
                { COMO_P_ONE | COMO_P_OPT, "mode", "-m", "Mode." },
                { COMO_P_ONE | COMO_P_MANY | COMO_P_OPT,
                    "params", NULL, "Parameters." },
                { COMO_P_NONE | COMO_P_ONE | COMO_P_MANY | COMO_P_OPT,
                    "types", "-t", "Types." },
                { COMO_P_NONE | COMO_P_OPT | COMO_P_HIDDEN, "terminator", "-", "The terminator." },
                { COMO_P_ONE | COMO_P_MANY, "dir", "-d", "Directory argument(s)." },
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
      mc_bool_t first = mc_true;

      printf( "External: [" );

      value = como_cmd->external;

      while ( *value )
        {
          if ( !first )
            printf( ", " );
          printf( "\"%s\"", *value );
          first = mc_false;
          value++;
        }

      printf( "]\n" );
    }

  como_end();

  return 0;
}
