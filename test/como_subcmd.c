/** @file como_options.c
 *
 * Test option types.
 */


#include "../src/mc.h"
#include "../src/como.h"


/**
 * Hierarchically show results for options.
 */
void display_options( como_cmd_t* cmd )
{
  como_cmd_t* subcmd;
  como_opt_t** opts, *o;

  printf( "Options for: %s\n", cmd->name );

  opts = cmd->opts;
  while ( *opts )
    {
      o = *opts;

      printf( "  Given \"%s\": %s\n", o->name, o->given ? "true" : "false" );
       
      if ( o->given && o->value )
        {
          printf( "  Value \"%s\": ", o->name );
          como_display_values( stdout, o );
          printf( "\n" );
        }

      opts++;
    }

  subcmd = como_cmd_given_subcmd( cmd );
  if ( subcmd )
    display_options( subcmd );
}


int main( int argc, char** argv )
{
  como_maincmd( "como_subcmd", "Como Tester", "2013",
                { COMO_OPT_SINGLE, "password",   "-p", "User password." },
                { COMO_OPT_MULTI,  "username",   "-u", "Username(s)." },
                { COMO_SILENT,     "terminator", "-",  "Terminator." },
                { COMO_SUBCMD,     "add",        NULL, "Add file to repo." },
                { COMO_SUBCMD,     "rm",         NULL, "Remove file from repo." },
                { COMO_SUBCMD,     "commit",     NULL, "Commit (pending) changes to repo." }
                );

  como_subcmd( "add", "como_subcmd",
               { COMO_SWITCH,      "force",     "-fo", "Force operation." },
               { COMO_OPT_SINGLE,  "username",  "-u",  "Username." },
               { COMO_SINGLE,      "file",      "-f",  "File." }
               );

  como_subcmd( "rm", "como_subcmd",
               { COMO_SWITCH,      "force",     "-fo", "Force operation." },
               { COMO_OPT_SINGLE,  "file",      "-f",  "File." }
               );
 
  como_subcmd( "commit", "como_subcmd",
               { COMO_SWITCH,      "quiet",     "-q", "Quiet operation." },
               { COMO_OPT_SINGLE,  "username",  "-u", "Username." },
               { COMO_DEFAULT,     NULL,        NULL, "File(s) to commit." }
               );

  como_finish();


  display_options( como_cmd );


  if ( como_cmd->external )
    {
      char** value;
      bool_t first = true;

      printf( "External: [" );

      value = como_cmd->external;

      while ( *value )
        {
          if ( !first )
            printf( ", " );
          printf( "\"%s\"", *value );
          first = false;
          value++;
        }
      
      printf( "]\n" );
    }
  
  como_end();

  return 0;
}
