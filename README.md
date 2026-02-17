# Como

Como library provides low manifest command line option parsing and
deployment. The command line options are described in compact table
format and option values are stored to conveniently named
properties. Como builds command usage information based on the option
table (+ generic program info) and displays it automatically if
necessary. Como supports also subcommands.


A simple example program using Como:

    #include <stdio.h>
    #include <como.h>

    int main( int argc, char** argv )
    {
      como_opt_t opt;

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
      if ( opt->given ) {
          char** value;

          printf( "Option \"%s\" values:\n", opt->name );

          value = opt->value;
          while ( *value ) {
              printf( "  %s\n", *value );
              value++;
          }
      }

      /* Print "debug" option's "given" property. */
      printf( "Option \"%s\" is given: %d\n", "debug", como_given( "debug" ) );

      return 0;
    }


You have automatically:
  - `-h` option defined with usage printout.
  - Check that `-f` has one and only one argument.
  - Tests whether a particular option was given or not.
  - Long options based on option name.
  - Option values collected to an array.



# Building and installing

Como depends on the `plinth` library.

Como is built with the `sbin/do-build` script.

Install is performed with `sbin/do-install`. Please, edit the script
for setting the installation root directory.

The man page generation requires `a2x` for conversion (activate in
`sbin/do-build`).


# Documentation

Manual page for Como is included in the installation (see: `man/`).

The source code is documented in Doxygen format. Documentation can be
generated with:

    doxygen .doxygen

`como.h` related file will include the usage information (equal to man
page).


# Examples

There are two simple examples in the `examples/` directory:
`como_simple.c` and `como_subcmd.c`.

For a complete set of features, see the test programs in `test/`
directory (`*.c`).


## Testing

Ceedling based flow is in use:

    shell> ceedling

Testing:

    shell> ceedling test:all

User defines can be placed into `project.yml`. Please refer to
Ceedling documentation for details.


## Ceedling

Standard Ceedling files are not in GIT. These can be added by
executing:

    shell> ceedling new plinth

in the directory above Plinth. Ceedling prompts for file overwrites.
You should answer NO in order to use the customized files.


# License

See: COPYING



Como library by Tero Isannainen, (c) Copyright 2015, 2025.
