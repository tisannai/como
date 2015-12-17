#include <stdio.h>
#include <como.h>

int main( int argc, char** argv )
{
  como_opt_t* opt;

  /* Specify program and all the options. Parsing and help is managed
     automatically. */
  como_command( "como_simple", "Programmer", "2014",
                { COMO_SINGLE, "file",  "-f", "File argument." },
                { COMO_SWITCH, "debug", "-d", "Enable debugging." }
                );

  /* Get handle of option "file". */
  opt = como_opt( "file" );

  /* Print "file" option's properties. */
  printf( "Option \"%s\" is given: %d\n", opt->name, opt->given );
  
  /* Print "file" option's value if option was given. */
  if ( opt->given )
    {
      char** value;

      printf( "Option \"%s\" values:\n", opt->name );

      value = opt->value;
      while ( *value )
        {
          printf( "  %s\n", *value );
          value++;
        }
    }

  /* Print "debug" option's "given" property. */
  printf( "Option \"%s\" is given: %d\n", "debug", como_given( "debug" ) );

  return 0;
}
