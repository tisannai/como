#ifndef COMO_H
#define COMO_H

/**
 * @file como.h
 *
 * @brief Como library provides low manifest command line parsing.
 *
 * @mainpage
 *
 * # Como
 *
 * ## Introduction
 *
 * Como provides low manifest command line option parsing and
 * deployment. The command line options are described in compact table
 * format and option values are stored to conveniently named
 * properties. Como builds command usage information based on the
 * option table (+ generic program info) and displays it automatically
 * if necessary. Como supports also subcommands.
 *
 *
 * ## Usage Examples
 *
 * Two simple examples are presented in this section. First one
 * includes a straight forward command definition and the second is a
 * bit more complicated with subcommand feature in use.
 *
 * ### Simple example
 *
 * Below is a small example program ("como_simple") that demonstrates
 * typical usage.
 *
 * #### Program listing
 *
 * @include como_simple.c
 *
 * First Como and other essential headers are included.
 *
 * "como_command" method takes 4 arguments:
 * - progname: Name of the program (or command).
 * - author: Author of the program.
 * - year: Year (or any date) for the program.
 * - option table: Description of the command options.
 *
 * Each option table entry (row/sub-array) includes 4 fields and
 * specifies one option:
 *   @code
 *   [ type, name, mnemonic, doc ]
 *   @endcode
 *
 * Two different types are present in the example:
 * - single: Single means that the option requires one argument (and
 *           only one).
 * - switch: Switch is an optional flag (default value is false).
 *
 * Option name is used to reference the option value that user has
 * given.  The command line option values are stored
 * automatically. For example the file option value is returned by
 * (array with NULL termination):
 *
 * @code
 *   opt->value
 * @endcode
 *
 * or without the option reference directly by option name:
 * @code
 *   como_value( "file" )
 * @endcode
 *
 * A single value can be directly accessed with:
 * @code
 *   opt->value[0]
 * @endcode
 *
 *
 * The option name also doubles as long option format, i.e. one could
 * use <pre>--file \<filename\></pre> on the command line.
 *
 * Existence of optional options can be tested using the "given"
 * method. For example:
 *
 * @code
 *   como_given( "debug" )
 * @endcode
 *
 * would return "non-null" if "-d" was given on the command line.
 *
 * Mnemonic is the short form option specification e.g. "-f". If short
 * form is replaced with "NULL", the long option format is only
 * available.
 *
 * Doc includes documentation for the option. It is displayed when
 * "help" ("-h") option is given. Help option is added to the command
 * automatically as default behavior.
 *
 * #### Simple example executions
 *
 * Normal behavior would be achieved by executing:
 *
 * @code
 *   shell> como_simple -f example -d
 * @endcode
 *
 * The program would execute with the following output:
 *
 @verbatim
 Option "file" is given: 1
 Option "file" values:
 example
 Option "debug" is given: 1
 @endverbatim
 *
 * Same output would be achieved with:
 *
 * @code
 *   shell> como_simple --file example --debug
 * @endcode
 *
 * Since option name doubles as long option.
 *
 * Como includes certain "extra" behavior out-of-box. Required
 * arguments are checked for existence and error is displayed if
 * arguments are not given.
 *
 * For example given the command:
 *
 * @code
 *   shell> como_simple
 * @endcode
 *
 * The following is displayed on the screen:
 *
 @verbatim
 como_simple error: Option "-f" missing for "como_simple"...

 Usage:
 como_simple -f <file> [-d]

 -f          File argument.
 -d          Enable debugging.


 Copyright (c) 2013 by Programmer
 @endverbatim
 *
 * Missing option error is displayed since "file" is a mandatory
 * option. The error message is followed by "usage" display (Usage
 * Help). Documentation string is taken from the option specification to
 * "usage" display.
 *
 * Given the command:
 * @code
 *   shell> como_simple -h
 * @endcode
 *
 * would display the same "usage" screen except without the error
 * line.
 *
 *
 * ### Subcommand example
 *
 * Subcmd example includes a program which has subcommands. Subcommands
 * can have their own command line switches and options.
 *
 * #### Program listing
 *
 * @include como_subcmd.c
 *
 * "como_maincmd" method defines a program (command) with possible
 * subcommands. Program name, author and date are provided as
 * parameters. The rest of the parameters defined the options and/or
 * subcmds.
 *
 * The "como-subcmd" methods define subcommands for the parent
 * command. This example includes one subcommand level, but multiple
 * levels are allowed.
 *
 * "como_finish" is marker for complete program options definion. It
 * will start parsing and checking for options. After "como_finish"
 * the user can query the options.
 *
 * Main (root) commands can be referenced through variables:
 * <pre>
 *   como_main or como_cmd
 * </pre>
 *
 * The subcommands can be referenced through "como_main" (etc.)
 * @code
 *   como_given_subcmd
 * @endcode
 * or by name
 * @code
 *   como_subcmd( "add" )
 * @endcode
 *
 * The queries have too versions: "como_<query>" and
 * "como_cmd_<query>". For "como_<query>" it is assumed that the query
 * is targeted to como_main. For "como_cmd_<query>" the first argument
 * is always a "como_cmd_t" which defines the scope of query.
 *
 *
 * #### Subcommand example executions
 *
 * Normal behavior would be achieved by executing:
 * @code
 *   shell> como_subcmd add -fo -f example
 * @endcode
 *
 * The program would execute with the following output:
 * @code
 Options for: como_subcmd
 Given "help": false
 Given "add": true
 Given "rm": false
 Options for: add
 Given "help": false
 Given "force": true
 Given "password": false
 Given "username": false
 Given "file": true
 Value "file": example
 * @endcode
 *
 *
 * Help is automatically provided on each command level, thus these are
 * both valid.
 *   shell> como_subcmd -h
 * and
 *   shell> como_subcmd rm -h
 *
 *
 *
 * ## Option specification
 *
 * ### Overview
 *
 * Option specification includes the minimum set of information
 * required for command line parsing. It is used to:
 * - Parse the command line.
 * - Check for wrong options and report.
 * - Check for mandatory arguments and report.
 * - Set the options given/non-given state.
 * - Set the options value. Array/String for all except no value for
 *   switches (check given property instead).
 * - Generate Usage Help printout.
 *
 *
 * ### Option types
 *
 * The following types can be defined for the command line options:
 * - COMO_SUBCMD: Subcmd option. Subcmd specific options are provided
 *           separately.
 * - COMO_SWITCH: Single switch option (no arguments).
 * - COMO_SINGLE: Mandatory single argument option.
 * - COMO_MULTI: Mandatory multiple argument option (one or many). Option
 *          values in array.
 * - COMO_OPT_SINGLE: Optional single argument option.
 * - COMO_OPT_MULTI: Optional multiple argument option (one or many). Option
 *              values in array.
 * - COMO_OPT_ANY: Optional multiple argument option (also none
 *            accepted). Option values in array.
 * - COMO_DEFAULT: Default option (no switch associated). Name and
 *            option String values can be left out, since only the
 *            document string is used. Default option is referred with
 *            NULL for name.
 * - COMO_EXCLUSIVE: Option that does not coexist with other options.
 * - COMO_SILENT: Option that does not coexist with other options and is not
 *           displayed as an option in Usage Help display. In effect a
 *           sub-option of :exclusive.
 *
 * Options use all the 4 option fields:
 * @code
 *   [ type, name, mnemonic, doc ]
 * @endcode
 *
 * "type" field is mandatory for all options.
 *
 * "name" field is also mandatory for all options. "mnemonic" can be
 * left out (set to NULL), but then option accepts only long option format.
 *
 * "COMO_DEFAULT" uses only "doc" and "COMO_SUBCMD" doesn't use the "mnemonic"
 * field. Those fields should be set to "NULL", however.
 *
 * "COMO_MULTI", "COMO_OPT_MULTI", and "COMO_OPT_ANY" option arguments are
 * terminated only when an option specifier is found. This can be a
 * problem if "COMO_DEFAULT" option follows. The recommended solution is to
 * use a "COMO_SILENT" option that can be used to terminate the argument
 * list. For example:
 * @code
 *   { COMO_SILENT, "terminator", "-", "The terminator." },
 * @endcode
 *
 *
 * ### Option type primitives
 *
 * Como converts option types into option type primitives. Option types
 * are not completely orthogonal, but primitives are.
 *
 * Primitives:
 *
 * - COMO_P_NONE: No arguments (i.e. switch).
 * - COMO_P_ONE: One argument.
 * - COMO_P_MANY: More than one argument.
 * - COMO_P_OPT: Optional argument(s).
 * - COMO_P_DEFAULT: Default option.
 * - COMO_P_MUTEX: Mutually exclusive option.
 * - COMO_P_HIDDEN: Hidden option (no usage doc).
 *
 * Types to primitives mapping:
 *
 * - COMO_SWITCH: COMO_P_NONE, COMO_P_OPT
 * - COMO_SINGLE: COMO_P_ONE
 * - COMO_MULTI: COMO_P_ONE, COMO_P_MANY
 * - COMO_OPT_SINGLE: COMO_P_ONE, COMO_P_OPT
 * - COMO_OPT_MULTI: COMO_P_ONE, COMO_P_MANY, COMO_P_OPT
 * - COMO_OPT_ANY: COMO_P_NONE, COMO_P_ONE, COMO_P_MANY, COMO_P_OPT
 * - COMO_DEFAULT: COMO_P_NONE, COMO_P_ONE, COMO_P_MANY, COMO_P_OPT, COMO_P_DEFAULT
 * - COMO_EXCLUSIVE: COMO_P_NONE, COMO_P_ONE, COMO_P_MANY, COMO_P_OPT, COMO_P_MUTEX
 * - COMO_SILENT: COMO_P_NONE, COMO_P_OPT, COMO_P_HIDDEN
 *
 * Primitives can be used in place of types if exotic options are
 * needed. Instead of a single type, ored combination of primitives
 * are given for option type. Order of primitives is not significant.
 *
 * For example:
 * @code
 *   { COMO_P_NONE | COMO_P_HIDDEN | COMO_P_OPT, "terminator", "-", "The terminator." },
 * @endcode
 *
 * Como does not check the primitive combinations, thus care and
 * consideration should be applied.
 *
 *
 * ### Option specification method configuration
 *
 * Option behavior can be controlled with several configuration options.
 *
 * The configuration options are set by execution configuration
 * function. These are the called after option has been specified and
 * before como_finish. Setting the configuration at "como_maincmd"
 * will propagate the config options to all the subcommands as
 * well. Configuration can be given to each subcommand separately to
 * override the inherited config values. Subcommand settings are not
 * inherited, but apply only in the subcommand.
 *
 * The usable configuration keys:
 * - autohelp: Add help option automatically (default: true). Custom
 *             help option can be provided and it can be made also
 *             visible to user.
 * - header: Header lines before standard usage printout.
 * - footer: Footer lines after standard usage printout.
 * - subcheck: Automatically check that a subcommand is provided
 *             (default: true).
 * - check_missing: Check for missing arguments (default: true).
 * - check_invalid: Error for unknown options (default: true).
 * - tab: Tab stop column for option documentation (default: 12).
 * - help_exit: Exit program if help displayed (default: true).
 *
 *
 *
 * ## Option referencing
 *
 * ### Existence and values
 *
 * como_opt_t includes the parsed option values. All options can be
 * tested whether they are specified on the command line using:
 * @code
 *   como_given( "name" )
 * @endcode
 * or
 * @code
 *   como_cmd_given( cmd, "name" )
 * @endcode
 *
 * Provided value(s) is returned by:
 * @code
 *   como_value( "name" )
 * @endcode
 * or
 * @code
 *   como_cmd_value( cmd, "name" )
 * @endcode
 *
 * For "COMO_SWITCH" there is no value and for the other types they are
 * string (array of one) or an array of multiple strings.
 *
 * With "COMO_OPT_ANY" type, the user should first check if the option was given:
 *
 * @code
 *   como_cmd_given( cmd, "many_files_or_none" )
 * @endcode
 *
 * Then check how many arguments where given, and finally decide what
 * to do. The value array is terminated with NULL.
 *
 * Header file "como.h" includes user definitions and documentation
 * for user interface functions.
 *
 *
 * ### Subcommand options
 *
 * The given subcommand for the parent command is return by
 * "como_given_subcmd" or "como_cmd_given_subcmd". Commonly the
 * program creator should just check directly which subcommand has
 * been selected and check for any subcommand options set.
 *
 *
 * ### Program external options
 *
 * If the user gives the "\--" option (double-dash), the arguments after
 * that option is stored as an array to "como_external".
 *
 *
 * ## Customization
 *
 * If the default behavior is not satisfactory, changes can be
 * implemented simply by complementing the existing functions. Some
 * knowledge of the internal workings of Como is required though.
 *
 *
 * ## Background
 *
 * Como (for C-lang) implements a subset of Como (for Ruby).
 *
 * C-lang version limitations:
 * - No rule checking support.
 *
 *
 * ## User API function index
 *
 * ### Option specification
 *
 * - #como_command( prog,author,year,... )
 * - #como_maincmd( prog,author,year,... )
 * - #como_subcmd( name,parentname,... )
 * - void como_finish( void );
 * - void como_end( void );
 *
 *
 * ### Option queries
 *
 * - como_opt_t como_opt( char* name );
 * - char**     como_value( char* name );
 * - como_opt_t como_given( char* name );
 * - como_opt_t como_cmd_opt( como_cmd_t cmd, char* name );
 * - char**     como_cmd_value( como_cmd_t cmd, char* name );
 * - como_opt_t como_cmd_given( como_cmd_t cmd, char* name );
 * - como_cmd_t como_cmd_subcmd( como_cmd_t, char* name );
 * - como_cmd_t como_given_subcmd( void );
 * - como_cmd_t como_cmd_given_subcmd( como_cmd_t parent );
 *
 *
 * ### Configuration option setting functions
 *
 * - void como_conf_autohelp( pl_bool_t val );
 * - void como_conf_header( char* val );
 * - void como_conf_footer( char* val );
 * - void como_conf_subcheck( pl_bool_t val );
 * - void como_conf_check_missing( pl_bool_t val );
 * - void como_conf_check_invalid( pl_bool_t val );
 * - void como_conf_tab( pl_i32_t val );
 * - void como_conf_help_exit( pl_bool_t val );
 *
 *
 * ### Generic functions
 *
 * - void como_error( const char* format, ... );
 * - void como_usage( void );
 * - void como_cmd_usage( como_cmd_t cmd );
 *
 */


#include <stdio.h>
#include <plinth.h>


/** Como-library version. */
extern const char* como_version;


/** Subcmd option. */
#define COMO_SUBCMD ( 1 << 0 )
/** Switch option. */
#define COMO_SWITCH ( 1 << 1 )
/** Single value option. */
#define COMO_SINGLE ( 1 << 2 )
/** Multi value option. */
#define COMO_MULTI ( 1 << 3 )
/** Optional single value option. */
#define COMO_OPT_SINGLE ( 1 << 4 )
/** Optional multi value option. */
#define COMO_OPT_MULTI ( 1 << 5 )
/** 0, 1, or more value option. */
#define COMO_OPT_ANY ( 1 << 6 )
/** No id option. */
#define COMO_DEFAULT ( 1 << 7 )
/** Disables the other options. */
#define COMO_EXCLUSIVE ( 1 << 8 )
/** Non-documented option. */
#define COMO_SILENT ( 1 << 9 )

/** No arguments (i.e. switch). */
#define COMO_P_NONE ( 1 << 10 )
/** One argument. */
#define COMO_P_ONE ( 1 << 11 )
/** More than one argument. */
#define COMO_P_MANY ( 1 << 12 )
/** Optional argument(s). */
#define COMO_P_OPT ( 1 << 13 )
/** Default option. */
#define COMO_P_DEFAULT ( 1 << 14 )
/** Mutually exclusive option. */
#define COMO_P_MUTEX ( 1 << 15 )
/** Hidden option (no usage doc). */
#define COMO_P_HIDDEN ( 1 << 16 )


/** Option type. */
typedef pl_u64_t como_opt_type_t;


/**
 * Option specification entry.
 */
pl_struct( como_opt_spec )
{
    como_opt_type_t type; /**< Option type. */
    const char*     name; /**< Option name (for reference). */
    const char*     opt;  /**< Short switch ("-x" or NULL). Longopt is used if NULL. */
    const char*     doc;  /**< Option documentation. */
};



/**
 * Parsed option content. Includes option info for the user.
 */
pl_struct( como_opt )
{

    /** Option type. */
    como_opt_type_t type;

    /** Option name (for reference). */
    const char* name;

    /** Short switch ("-x" or NULL). Longopt is used if NULL. */
    const char* shortopt;

    /** Option documentation. */
    const char* doc;

    /** Generated longopt name: "--#{name}". */
    char* longopt;

    /** Array of given option values. */
    plcm_s value_store; /* Only for internal use. */
    char** value;

    /** True if option was set on CLI. */
    pl_bool_t given;
};


/**
 * Command configuration options. User can change the values with
 * "como_conf_<option>" functions, e.g. como_conf_header.
 */
pl_struct( como_config )
{

    /**
     * Generate help option automatically.
     * default: true
     */
    pl_bool_t autohelp;

    /**
     * Usage help header. User have to include all newlines.
     * default: NULL
     */
    char* header;

    /**
     * Usage help footer. User have to include all newlines.
     * default: NULL
     */
    char* footer;

    /**
     * Check for missing sub-commands.
     * default: true
     */
    pl_bool_t subcheck;

    /**
     * Check for missing options.
     * default: true
     */
    pl_bool_t check_missing;

    /**
     * Check for invalid options.
     * default: true
     */
    pl_bool_t check_invalid;

    /**
     * Option mnemonic tab stop for option doc.
     * default: 12
     */
    pl_i64_t tab;

    /**
     * Exit after usage help display.
     * default: true
     */
    pl_bool_t help_exit;
};

pl_struct_type( como_cmd );

/**
 * Program level option information including program information and
 * parsing results.
 */
pl_struct_body( como_cmd )
{

    /** Command name. */
    char* name;

    /** Name with hierachy. */
    char* longname;

    /** Program author. */
    char* author;

    /** Year (or data for program). */
    char* year;

    /** Number of options. */
    pl_i64_t optcnt;

    /** Array of options (objects). */
    como_opt_p opts;

    /** Parent (host) for this subcmd. */
    como_cmd_t parent;

    /** Array of subcmds. */
    plcm_s subcmds;

    /** Array of program external options. */
    char** external;

    /** Is this subcmd given?. */
    pl_bool_t given;

    /** Number of given arguments. */
    pl_i64_t givencnt;

    /** Number of option errors. */
    pl_i64_t errors;

    /** Command configuration. */
    como_config_t conf;
};


/** Active como command (under processing). Same as main after option
    parsing. */
extern como_cmd_t como_cmd;

/** Main command, i.e. root of command hierarchy. */
extern como_cmd_t como_main;


/** Number of arguments for como. */
extern pl_i64_t como_argc;

/** Array of arguments for como. */
extern char** como_argv;



/* User interface macros: */

/**
 * User interface (macro) for command and option specification
 * (including parsing).
 */
#define como_command( prog, author, year, ... ) \
    do {                                        \
        como_init( argc, argv, author, year );  \
        como_subcmd( prog, NULL, __VA_ARGS__ ); \
        como_finish();                          \
    } while ( 0 )

/**
 * User interface (macro) for command and option specification
 * (including parsing).
 */
#define como_complete( prog, author, year, ... ) \
    do {                                         \
        como_init( argc, argv, author, year );   \
        como_subcmd( prog, NULL, __VA_ARGS__ );  \
        como_finish();                           \
    } while ( 0 )


/**
 * User interface (macro) for main cmd (program) specification.
 */
#define como_maincmd( prog, author, year, ... ) \
    do {                                        \
        como_init( argc, argv, author, year );  \
        como_subcmd( prog, NULL, __VA_ARGS__ ); \
    } while ( 0 )


/**
 * User interface (macro) for sub-command specification.
 */
#define como_subcmd( name, parentname, ... ) \
    como_spec_subcmd(                        \
        name, parentname, (como_opt_spec_s[]){ __VA_ARGS__ }, como_spec_size( __VA_ARGS__ ) )

/**
 * Option specification list (array) size.
 */
#define como_spec_size( ... ) \
    ( sizeof( (como_opt_spec_s[]){ __VA_ARGS__ } ) / sizeof( como_opt_spec_s ) )



/*
 * User interface functions:
 */

/**
 * Finalize setup and parse all options.
 */
void como_finish( void );



/*
 * Option query functions:
 */


/**
 * Get main command option (by name).
 *
 * @param name Option name (NULL for default arg).
 *
 * @return Option.
 */
como_opt_t como_opt( char* name );

/**
 * Get value of main command option.
 *
 * @param name Option name.
 *
 * @return Option value.
 */
char** como_value( char* name );

/**
 * Get given status of main command option.
 *
 * @param name Option name.
 *
 * @return Option if given.
 */
como_opt_t como_given( char* name );

/**
 * Get command option (by name).
 *
 * @param cmd Command containing option.
 * @param name Option name (NULL for default arg).
 *
 * @return Option.
 */
como_opt_t como_cmd_opt( como_cmd_t cmd, char* name );

/**
 * Get value of command option.
 *
 * @param cmd Command containing option.
 * @param name Option name.
 *
 * @return Option value.
 */
char** como_cmd_value( como_cmd_t cmd, char* name );

/**
 * Get given status of command option.
 *
 * @param cmd Command containing option.
 * @param name Option name.
 *
 * @return Option's given status (true if given).
 */
como_opt_t como_cmd_given( como_cmd_t cmd, char* name );

/**
 * Get cmd's sub-command (by name).
 *
 * @param cmd Parent.
 * @param name Subcmd name.
 *
 * @return Subcmd.
 */
como_cmd_t como_cmd_subcmd( como_cmd_t cmd, char* name );

/**
 * Return given subcmd for como_main.
 *
 * @return Subcmd (or NULL).
 */
como_cmd_t como_given_subcmd( void );

/**
 * Return given subcmd for parent.
 *
 * @param parent Parent for subcmd.
 *
 * @return Subcmd (or NULL).
 */
como_cmd_t como_cmd_given_subcmd( como_cmd_t parent );

/**
 * Return program external argument list.
 *
 * @return External args.
 */
char** como_external( void );

/**
 * Generate id for option. Use short option if defined, otherwise
 * longopt. For sub-commands the name is returned.
 *
 * @param opt Option.
 *
 * @return Id.
 */
const char* como_opt_id( como_opt_t opt );



/*
 * Configuration option setting functions:
 */


/** Set autohelp configuration value. */
void como_conf_autohelp( pl_bool_t val );

/** Set header configuration value. */
void como_conf_header( char* val );

/** Set footer configuration value. */
void como_conf_footer( char* val );

/** Set subcheck configuration value. */
void como_conf_subcheck( pl_bool_t val );

/** Set check_missing configuration value. */
void como_conf_check_missing( pl_bool_t val );

/** Set check_invalid configuration value. */
void como_conf_check_invalid( pl_bool_t val );

/** Set tab configuration value. */
void como_conf_tab( pl_i64_t val );

/** Set help_exit configuration value. */
void como_conf_help_exit( pl_bool_t val );


/*
 * Generic functions
 */

/**
 * Report como error with command prefix. Increment error counter.
 *
 * @param format String formatter.
 * @param ... Args for formatter.
 */
void como_error( const char* format, ... );

/**
 * Display main command usage.
 *
 */
void como_usage( void );

/**
 * Display command usage.
 *
 * @param cmd Command to display.
 */
void como_cmd_usage( como_cmd_t cmd );

/**
 * Display options's value(s). Used for testing/debug.
 *
 * @param fh File stream to use.
 * @param o Option to display.
 */
void como_display_values( FILE* fh, como_opt_t o );


/*
 * Functions called by macros.
 */

/**
 * Initialize global como state.
 *
 * @param argc C-main argument count.
 * @param argv C-main argument array.
 * @param author Program author.
 * @param year Program creation date (year).
 */
void como_init( pl_i64_t argc, char** argv, char* author, char* year );


/**
 * Option specification for subcmd. If parentname is NULL, then this
 * command (subcmd) is made the main command. Subcmd is added to the
 * global command list.
 *
 * Subcmd can be configured after this function. If subcmd has a
 * parent, the configuration is inherited from the parent.
 *
 * Example:
 *
 * @code
 *  como_spec_subcmd( "como_simple", "Programmer", "2013",
 *                     (como_opt_spec_s []) {
 *                       { COMO_OPT_SINGLE, "file",  "-f", "File argument." },
 *                       { COMO_OPT_SWITCH, "debug", "-d", "Enable debugging." }
 *                     }, 2 );
 * @endcode
 *
 * @param name Name.
 * @param parentname Name of subcmd parent.
 * @param spec Array of option specifications.
 * @param size Size of the specification array.
 */
void como_spec_subcmd( char* name, char* parentname, como_opt_spec_t spec, pl_i64_t size );



/**
 * Cleanup for all allocations made by como. User does not normally
 * need to do this, since command line options are parsed only once,
 * and the user might want to refer to the options upto the very end.
 *
 * This function is provided for the sake of completion and to ensure
 * that como does not leak memory.
 *
 * @param cmd Command to clean.
 */
void como_cmd_end( como_cmd_t cmd );


/**
 * Same as @see como_cmd_end(), but for default
 *
 */
void como_end( void );

#endif
