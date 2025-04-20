#ifndef COMO_H
#define COMO_H

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

    //   /** Number of values. */
    //   pl_i64_t valuecnt;

    //   /** Array of given option values. */
    //   plsr_t value;

    /** Array of given option values. */
    plcm_s value;

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

//     /** Number of subcmds. */
//     pl_i64_t subcmdcnt;

    /** Array of subcmds. */
    // como_cmd_p subcmds;
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
