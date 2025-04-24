/**
 * @file como.c
 *
 * Como - Command line parsing library.
 */


#include <stdlib.h>
#include <string.h>
#include "como.h"



/*
 * ------------------------------------------------------------
 * Como external vars.
 */

/* autoc:version */
const char* como_version = "0.3";
como_cmd_t  como_cmd = NULL;
como_cmd_t  como_main = NULL;
pl_i64_t    como_argc = 0;
char**      como_argv = NULL;


/* Initial memory resource for allocations. */
#define COMO_INIT_MEM_SIZE 256 * 1024
static char   como_init_mem[ COMO_INIT_MEM_SIZE ];
static plam_s como_mem;


/*
 * ------------------------------------------------------------
 * Como internal vars.
 */

/** Command line argument index. */
static pl_i64_t arg_idx;

/** Global list of commands. */
static plcm_s cmd_list;

/** Main command configuration. */
static como_config_t como_conf = NULL;


/*
 * ------------------------------------------------------------
 * Como internal functions.
 */


/**
 * Report fatal (internal) error.
 *
 */
void como_fatal( const char* format, ... )
{
    va_list ap;
    va_start( ap, format );
    fputs( "COMO FATAL: ", stderr );
    vfprintf( stderr, format, ap );
    va_end( ap );
}


/**
 * Create como_cmd_s data structure.
 *
 * @return Structure.
 */
static como_cmd_t cmd_create( void )
{
    como_cmd_t cmd;

    cmd = plcm_get_ref( &cmd_list, sizeof( como_cmd_s ) );
    cmd->name = NULL;
    cmd->longname = NULL;
    cmd->author = NULL;
    cmd->year = NULL;
    cmd->givencnt = 0;
    cmd->given = pl_false;
    cmd->errors = 0;
    cmd->parent = NULL;
    plcm_empty( &cmd->subcmds, 32 * sizeof( como_cmd_t ) );

    cmd->opts = NULL;

    return cmd;
}


/**
 * Create and setup como_opt_s data structure.
 *
 * @param type Option type.
 * @param name Option name.
 * @param opt Option mnemonic.
 * @param doc Option documentaion.
 *
 * @return Structure.
 */
static como_opt_t opt_create( como_opt_type_t type,
                              const char*     name,
                              const char*     opt,
                              const char*     doc )
{
    como_opt_t co;

    co = plam_get( &como_mem, sizeof( como_opt_s ) );

    if ( type == COMO_DEFAULT ) {
        /* Force these for default type. */
        co->name = "<default>";
        co->shortopt = "<default>";
    } else {
        co->name = name;
        co->shortopt = opt;
    }

    /* Convert type definition to primitives. */
    switch ( type ) {
        case COMO_SWITCH:
            type = COMO_P_NONE | COMO_P_OPT;
            break;
        case COMO_SINGLE:
            type = COMO_P_ONE;
            break;
        case COMO_MULTI:
            type = COMO_P_ONE | COMO_P_MANY;
            break;
        case COMO_OPT_SINGLE:
            type = COMO_P_ONE | COMO_P_OPT;
            break;
        case COMO_OPT_MULTI:
            type = COMO_P_ONE | COMO_P_MANY | COMO_P_OPT;
            break;
        case COMO_OPT_ANY:
            type = COMO_P_NONE | COMO_P_ONE | COMO_P_MANY | COMO_P_OPT;
            break;
        case COMO_DEFAULT:
            type = COMO_P_NONE | COMO_P_ONE | COMO_P_MANY | COMO_P_OPT | COMO_P_DEFAULT;
            break;
        case COMO_EXCLUSIVE:
            type = COMO_P_NONE | COMO_P_ONE | COMO_P_MANY | COMO_P_OPT | COMO_P_MUTEX;
            break;
        case COMO_SILENT:
            type = COMO_P_NONE | COMO_P_OPT | COMO_P_HIDDEN;
            break;
        default:
            break;
    }

    co->type = type;
    co->doc = doc;

    co->longopt = plam_format( &como_mem, "--%s", co->name );

    plcm_use_plam( &co->value_store, &como_mem, 32 * sizeof( char* ) );
    plcm_terminate( &co->value_store, sizeof( char* ) );
    co->value = NULL;
    co->given = pl_false;

    return co;
}


/**
 * Create como_config_s data structure.
 *
 * @return Structure.
 */
static como_config_t config_create( void )
{
    como_config_t conf;

    // conf = pl_alloc_memory( sizeof( como_config_s ) );
    conf = plam_get( &como_mem, sizeof( como_config_s ) );

    /* Setup config defaults. */
    conf->autohelp = pl_true;
    conf->header = NULL;
    conf->footer = NULL;
    conf->subcheck = pl_true;
    conf->check_missing = pl_true;
    conf->check_invalid = pl_true;
    conf->tab = 12;
    conf->help_exit = pl_true;

    return conf;
}


/**
 * Duplicate configuration.
 *
 * @param src Source data.
 *
 * @return New config.
 */
static como_config_t config_dup( como_config_t src )
{
    como_config_t conf;

    conf = config_create();

    /* Setup config defaults. */
    conf->autohelp = src->autohelp;
    conf->header = plam_strdup( &como_mem, src->header );
    conf->footer = plam_strdup( &como_mem, src->footer );
    conf->subcheck = src->subcheck;
    conf->check_missing = src->check_missing;
    conf->check_invalid = src->check_invalid;
    conf->tab = src->tab;
    conf->help_exit = src->help_exit;

    return conf;
}


/**
 * Find command by name.
 *
 * @param name Name.
 *
 * @return Command.
 */
static como_cmd_t find_cmd_by_name( char* name )
{
    como_cmd_t cmd;

    cmd = plcm_data( &cmd_list );
    while ( (pl_t)cmd < plcm_end( &cmd_list ) ) {
        if ( strcmp( cmd->name, name ) == 0 ) {
            return cmd;
        }
        cmd++;
    }

    return NULL;
}


/**
 * Add subcmd command to parents list.
 *
 * @param parent Host for subcmd.
 * @param subcmd Subcmd to add.
 */
static void add_subcmd( como_cmd_t parent, como_cmd_t subcmd )
{
    plcm_store( &parent->subcmds, &subcmd, sizeof( como_cmd_t ) );
}


/**
 * Find option by type.
 *
 * @param cmd Command including option.
 * @param type Option type.
 *
 * @return Option (or NULL).
 */
static como_opt_t find_opt_by_type( como_cmd_t cmd, como_opt_type_t type )
{
    pl_i64_t i = 0;

    while ( cmd->opts[ i ] ) {
        if ( cmd->opts[ i ]->type & type ) {
            return cmd->opts[ i ];
        }
        i++;
    }

    return NULL;
}


/**
 * Find option by name.
 *
 * @param cmd Command including option.
 * @param name Option name (or NULL for COMO_DEFAULT arg).
 *
 * @return Option (or NULL).
 */
static como_opt_t find_opt_by_name( como_cmd_t cmd, char* name )
{
    pl_i64_t i = 0;

    if ( name == NULL ) {
        /* Find default arg. */
        return find_opt_by_type( cmd, COMO_P_DEFAULT );
    } else {
        while ( cmd->opts[ i ] ) {
            if ( strcmp( cmd->opts[ i ]->name, name ) == 0 ) {
                return cmd->opts[ i ];
            }
            i++;
        }
    }

    return NULL;
}


/**
 * Find option by characterics:
 *  - Default type is searched if str is NULL.
 *  - Long option is searched if str starts with "--".
 *  - Short option is searched if str starts with "-".
 *  - Named option is searched otherwise.
 *
 * @param cmd Command including option.
 * @param str Option tag to search for.
 *
 * @return Option (or NULL).
 */
static como_opt_t find_opt( como_cmd_t cmd, char* str )
{
    pl_i64_t i = 0;

    if ( str == NULL ) {
        /* Default option. */
        while ( cmd->opts[ i ] ) {
            if ( cmd->opts[ i ]->type & COMO_P_DEFAULT ) {
                return cmd->opts[ i ];
            }
            i++;
        }
    } else if ( strncmp( str, "--", 2 ) == 0 ) {
        /* Long option. */
        while ( cmd->opts[ i ] ) {
            if ( strcmp( cmd->opts[ i ]->longopt, str ) == 0 ) {
                return cmd->opts[ i ];
            }
            i++;
        }
    } else if ( str[ 0 ] == '-' ) {
        /* Short option. */
        while ( cmd->opts[ i ] ) {
            if ( cmd->opts[ i ]->shortopt && strcmp( cmd->opts[ i ]->shortopt, str ) == 0 ) {
                return cmd->opts[ i ];
            }
            i++;
        }
    } else {
        /* By name. */
        return find_opt_by_name( cmd, str );
    }

    return NULL;
}


/**
 * Get current argument from command line.
 *
 * @return Argument.
 */
static char* get_arg( void )
{
    return como_argv[ arg_idx ];
}


/**
 * Advance command line argument index.
 *
 */
static void next_arg( void )
{
    arg_idx++;
}


/**
 * Is current command line argument an option?
 *
 *
 * @return True if is.
 */
static pl_bool_t is_opt( void )
{
    char* s;

    s = get_arg();

    if ( s[ 0 ] == '-' ) {
        return pl_true;
    } else {
        return pl_false;
    }
}


/**
 * Is option of switch type? Note that default type is also consided a
 * switch.
 *
 * @param opt Option to check.
 *
 * @return True if is.
 */
static pl_bool_t has_switch_style_doc( como_opt_t opt )
{
    if ( ( ( opt->type & COMO_P_NONE ) && !( opt->type & COMO_P_MANY ) ) ||
         ( opt->type & COMO_P_DEFAULT ) ) {
        return pl_true;
    } else {
        return pl_false;
    }
}


/**
 * Add item to char pointer array pointer. Create new array is it
 * doesn't exist.
 *
 * @param [out] storage Pointer where allocation is stored.
 * @param [in] item Item to store.
 */
// static void add_value( char*** storage, char* item )
static void add_value( plcm_t storage, char* item )
{
    plcm_resize( storage, storage->used + 1 );
    plcm_store( storage, &item, sizeof( char* ) );
    plcm_terminate( storage, sizeof( char* ) );
}


/**
 * Check for missing required arguments. Checking ends if exclusive
 * argument is given. Checking continues with subcmd if encountered.
 *
 * @param cmd Command to check.
 * @param errcmd Command that had missing options.
 *
 * @return True if no missing.
 */
static pl_bool_t check_missing( como_cmd_t cmd, como_cmd_p errcmd )
{
    como_opt_s **opts, *o;
    pl_bool_t    ret = pl_true;
    pl_bool_t    subcheck;
    como_cmd_t   subcmd;

    if ( !cmd->conf->check_missing ) {
        return pl_true;
    }

    /* Check for any exclusive args first. Missing are not checked if has exclusives. */
    opts = cmd->opts;
    while ( *opts ) {
        o = *opts;
        if ( ( o->type & COMO_P_MUTEX ) && o->given ) {
            return ret;
        }
        opts++;
    }

    /* Check for missing options. */
    opts = cmd->opts;
    while ( *opts ) {
        o = *opts;
        if ( ( o->type != COMO_SUBCMD ) && !( o->type & COMO_P_OPT ) && !o->given ) {
            como_error( "Option \"%s\" missing for \"%s\"...", como_opt_id( o ), cmd->longname );
            *errcmd = cmd;
            return pl_false;
        }
        opts++;
    }

    /* Check for missing subcmds. */
    if ( !plcm_is_empty( &cmd->subcmds ) ) {
        /* Has subcmds. */
        if ( cmd->conf->subcheck ) {
            subcheck = pl_true;
        } else {
            subcheck = pl_false;
        }
    } else {
        subcheck = pl_false;
    }

    subcmd = como_cmd_given_subcmd( cmd );

    if ( subcmd ) {
        /* Go to level subcmd level. */
        return check_missing( subcmd, errcmd );
    } else if ( subcheck ) {
        como_error( "Subcommand required for \"%s\"...", cmd->name );
        *errcmd = cmd;
        return pl_false;
    }

    return ret;
}


/**
 * Parse command line and store given option values to options objects
 * until subcmd is encountered or end.
 *
 * @param cmd Command to update.
 * @param subcmd Command to update next.
 *
 * @retval 0 if all arguments have been parsed.
 * @retval 1 if should continue with subcmd.
 * @retval 2 if errors in parsing.
 */
static pl_i64_t parse_opts( como_cmd_t cmd, como_cmd_p subcmd )
{
    como_opt_t o;
    como_cmd_t c;

    while ( get_arg() ) {

        /* Option terminator?. */
        if ( strcmp( "--", get_arg() ) == 0 ) {
            /*  Rest of the args do not belong to this program. */
            next_arg();
            como_cmd->external = &( como_argv[ arg_idx ] );
            break;
        }

        else if ( is_opt() ) {

            /* Normal option. */

            o = find_opt( cmd, get_arg() );

            if ( !o ) {

                /* Not found, might be default. */

                if ( cmd->conf->check_invalid ) {
                    /* Report missing. */
                    como_error( "Unknown option \"%s\"...", get_arg() );
                    break;
                } else {
                    /* Default option. */
                    o = find_opt_by_type( cmd, COMO_P_DEFAULT );
                    if ( !o ) {
                        como_error( "No default option specified to allow \"%s\"...", get_arg() );
                        break;
                    } else {
                        if ( !plcm_is_empty( &o->value_store ) ) {
                            cmd->givencnt++;
                        }
                        add_value( &( o->value_store ), get_arg() );
                    }
                }
            } else if ( o && ( ( o->type & COMO_P_ONE ) || ( o->type & COMO_P_MANY ) ) ) {

                /* Option with arguments. */

                next_arg();

                if ( ( get_arg() == NULL || is_opt() ) && !( o->type & COMO_P_NONE ) ) {
                    como_error( "No argument given for \"%s\"...", como_opt_id( o ) );
                    break;
                } else {

                    char* arg;

                    if ( o->type & COMO_P_MANY ) {
                        /* Get all arguments for multi-option. */
                        while ( get_arg() && !is_opt() ) {
                            arg = get_arg();
                            plcm_store( &( o->value_store ), &arg, sizeof( char* ) );
                            next_arg();
                        }
                    } else {
                        if ( o->given ) {
                            como_error( "Too many arguments for option (\"%s\")...",
                                        como_opt_id( o ) );
                            break;
                        }
                        arg = get_arg();
                        plcm_store( &( o->value_store ), &arg, sizeof( char* ) );
                        next_arg();
                    }

                    o->given = pl_true;
                    cmd->givencnt++;
                }
            } else {

                /* Switch option. */
                o->given = pl_true;
                cmd->givencnt++;
                next_arg();
            }
        } else {

            /* Subcmd or default. Check for Subcmd first. */
            o = find_opt( cmd, get_arg() );

            if ( !o || o->type != COMO_SUBCMD ) {

                /* Default argument. */

                o = find_opt_by_type( cmd, COMO_P_DEFAULT );

                if ( !o ) {
                    if ( !plcm_is_empty( &cmd->subcmds ) ) {
                        como_error( "Unknown subcmd: \"%s\"...", get_arg() );
                    } else {
                        como_error( "No default option specified to allow \"%s\"...", get_arg() );
                    }
                    next_arg();
                } else {
                    if ( !plcm_is_empty( &o->value_store ) ) {
                        cmd->givencnt++;
                    }
                    add_value( &( o->value_store ), get_arg() );
                    o->given = pl_true;
                    next_arg();
                }
            } else {

                /* Subcmd. */

                /* Search for Subcmd. */
                c = find_cmd_by_name( get_arg() );
                o->given = pl_true;
                c->given = pl_true;
                next_arg();
                *subcmd = c;
                return 1;
            }
        }
    }

    if ( como_cmd->errors > 0 ) {
        cmd->errors = como_cmd->errors;
        *subcmd = cmd;
        return 2;
    } else {
        return 0;
    }
}


/**
 * Proxy for parse_opts. Checks for status after each subcmd and
 * recurses further if no errors.
 *
 * @param cmd Command to parse.
 * @param errcmd Command having errors.
 *
 * @return True if no errors.
 */
static pl_bool_t setup_and_parse( como_cmd_t cmd, como_cmd_p errcmd )
{
    pl_i64_t   ret;
    como_cmd_t subcmd;
    como_opt_t o;

    ret = parse_opts( cmd, &subcmd );

    for ( pl_u64_t i = 0; i < cmd->optcnt; i++ ) {
        /* Update the "value" pointers of options to refer the
           collected option values. */
        o = cmd->opts[ i ];
        if ( o->type != COMO_SUBCMD ) {
            if ( plcm_used( &o->value_store ) > 0 ) {
                o->value = plcm_data( &o->value_store );
            } else if ( ( o->type & COMO_P_MANY ) || ( o->type & COMO_P_DEFAULT ) ) {
                o->value = plcm_data( &o->value_store );
            }
        }
    }

    if ( ret == 0 ) {
        /* done.*/
        return pl_true;
    } else if ( ret == 1 ) {
        /* continue. */
        return setup_and_parse( subcmd, errcmd );
    } else if ( ret == 2 ) {
        /* error. */
        *errcmd = subcmd;
        return pl_false;
    } else {
        return pl_true;
    }
}


/**
 * Add option's command line formatting (usage) to str.
 *
 * @param [out] str String where command line is stored.
 * @param [in] o Option to add.
 */
static void opt_cmdline( plcm_t str, como_opt_t o )
{
    if ( o->type & COMO_P_HIDDEN ) {
        return;
    }

    if ( o->type & COMO_P_OPT ) {
        plss_append( str, plsr_from_c( "[" ) );
    }

    plss_format( str, "%s", como_opt_id( o ) );

    if ( !has_switch_style_doc( o ) ) {
        plss_format( str, " <%s>", o->name );

        if ( ( o->type & COMO_P_NONE ) && ( o->type & COMO_P_MANY ) ) {
            plss_append( str, plsr_from_c( "*" ) );
        } else if ( o->type & COMO_P_MANY ) {
            plss_append( str, plsr_from_c( "+" ) );
        }
    }

    if ( o->type & COMO_P_OPT ) {
        plss_append( str, plsr_from_c( "]" ) );
    }
}


/**
 * Add option documentation line.
 *
 * @param str String where document is stored.
 * @param o Option to document.
 * @param cmd Containing command (for configuration lookup).
 */
static void opt_doc( plcm_t str, como_opt_t o, como_cmd_t cmd )
{
    char        format_str_mem[ 128 ];
    plcm_s      format_str;
    const char* format;
    pl_i64_t    s, e;
    pl_bool_t   first;

    if ( o->type & COMO_P_HIDDEN ) {
        return;
    }

    plcm_use( &format_str, format_str_mem, 128 );
    plss_format( &format_str, "  %%-%ds", cmd->conf->tab );
    format = plss_string( &format_str );

    /* Reformat doc str, so that newlines start a new line and tab
     * chars align the start to previous line. */

    first = pl_true;
    s = 0;
    e = 0;
    for ( ;; ) {
        if ( o->doc[ e ] == '\n' || o->doc[ e ] == 0 ) {
            if ( first ) {
                plss_format( str, format, como_opt_id( o ) );
                plss_append( str, plsr_from_c_length( (char*)&( o->doc[ s ] ), ( e - s ) ) );
                plss_append_ch( str, '\n' );

                if ( o->doc[ e ] == 0 ) {
                    return;
                } else {
                    e++;
                }

                first = pl_false;
            } else {
                if ( o->doc[ s ] == '\t' ) {
                    s++;
                    plss_format( str, format, "" );
                }
                plss_append( str, plsr_from_c_length( (char*)&( o->doc[ s ] ), ( e - s ) ) );
                plss_append_ch( str, '\n' );

                if ( o->doc[ e ] == 0 ) {
                    return;
                } else {
                    e++;
                }
            }
            s = e;

        } else {
            e++;
        }
    }
}


/**
 * Display help if help option is given for any of the commands in the
 * hierarchy (recursion).
 *
 * @param cmd Command to search given help.
 */
static void usage_if_help( como_cmd_t cmd )
{
    como_cmd_t subcmd;

    if ( como_cmd_given( cmd, "help" ) ) {
        como_cmd_usage( cmd );
    } else if ( !plcm_is_empty( &cmd->subcmds ) && ( subcmd = como_cmd_given_subcmd( cmd ) ) ) {
        usage_if_help( subcmd );
    }
}


static void quit( int status )
{
    como_end();
    exit( status );
}



/*
 * ------------------------------------------------------------
 * Como public functions.
 */


void como_finish( void )
{
    como_cmd_t cmd;
    pl_bool_t  success;
    como_cmd_t errcmd;

    /* Parse all arguments and fill information to options. */

    como_cmd = como_main;
    cmd = como_main;

    success = setup_and_parse( cmd, &errcmd );

    if ( !success ) {
        como_cmd_usage( errcmd );
        quit( EXIT_FAILURE );
    } else if ( !check_missing( cmd, &errcmd ) ) {
        como_cmd_usage( errcmd );
        quit( EXIT_FAILURE );
    } else {
        usage_if_help( como_cmd );
    }
}


como_opt_t como_opt( char* name )
{
    return find_opt_by_name( como_cmd, name );
}


char** como_value( char* name )
{
    como_opt_t co;
    co = find_opt_by_name( como_cmd, name );
    return plcm_data( &co->value_store );
}


como_opt_t como_given( char* name )
{
    como_opt_t co;
    co = find_opt_by_name( como_cmd, name );
    if ( co->given ) {
        return co;
    } else {
        return NULL;
    }
}


como_opt_t como_cmd_opt( como_cmd_t cmd, char* name )
{
    return find_opt_by_name( cmd, name );
}


char** como_cmd_value( como_cmd_t cmd, char* name )
{
    como_opt_t co;
    co = find_opt_by_name( cmd, name );
    return plcm_data( &co->value_store );
}


como_opt_t como_cmd_given( como_cmd_t cmd, char* name )
{
    como_opt_t co;
    co = find_opt_by_name( cmd, name );
    if ( co->given ) {
        return co;
    } else {
        return NULL;
    }
}


como_cmd_t como_cmd_subcmd( como_cmd_t cmd, char* name )
{
    como_cmd_t subcmd;

    subcmd = plcm_data( &cmd->subcmds );
    while ( (pl_t)subcmd < plcm_end( &cmd->subcmds ) ) {
        if ( strcmp( subcmd->name, name ) == 0 ) {
            return subcmd;
        }
        subcmd++;
    }

    return NULL;
}


como_cmd_t como_given_subcmd( void )
{
    return como_cmd_given_subcmd( como_cmd );
}


como_cmd_t como_cmd_given_subcmd( como_cmd_t parent )
{
    como_cmd_p cmd;

    if ( plcm_is_empty( &parent->subcmds ) ) {
        return NULL;
    }

    cmd = plcm_data( &parent->subcmds );
    while ( (pl_t)cmd < plcm_end( &parent->subcmds ) ) {
        if ( ( *cmd )->given ) {
            return *cmd;
        }
        cmd++;
    }

    return NULL;
}


char** como_external( void )
{
    return como_main->external;
}


const char* como_opt_id( como_opt_t opt )
{
    if ( opt->type != COMO_SUBCMD ) {
        return opt->shortopt ? opt->shortopt : opt->longopt;
    } else {
        return opt->name;
    }
}


/*
 * Functions to set configuration items.
 */

void como_conf_autohelp( pl_bool_t val )
{
    como_cmd->conf->autohelp = val;
}

void como_conf_header( char* val )
{
    como_cmd->conf->header = plam_strdup( &como_mem, val );
}

void como_conf_footer( char* val )
{
    como_cmd->conf->footer = plam_strdup( &como_mem, val );
}

void como_conf_subcheck( pl_bool_t val )
{
    como_cmd->conf->subcheck = val;
}

void como_conf_check_missing( pl_bool_t val )
{
    como_cmd->conf->check_missing = val;
}

void como_conf_check_invalid( pl_bool_t val )
{
    como_cmd->conf->check_invalid = val;
}

void como_conf_tab( pl_i64_t val )
{
    como_cmd->conf->tab = val;
}

void como_conf_help_exit( pl_bool_t val )
{
    como_cmd->conf->help_exit = val;
}


void como_error( const char* format, ... )
{
    como_cmd->errors++;

    va_list ap;
    fputc( '\n', stderr );
    fputs( como_cmd->name, stderr );
    fputs( " error: ", stderr );
    va_start( ap, format );
    vfprintf( stderr, format, ap );
    va_end( ap );
    fputc( '\n', stderr );
}


void como_usage( void )
{
    como_cmd_usage( como_cmd );
}


void como_cmd_usage( como_cmd_t cmd )
{
    char   storage[ 8192 ];
    plcm_t str;
    plcm_s str_handle;

    como_opt_p co;
    pl_bool_t  main_cmd, has_visible;

    str = &str_handle;
    plcm_use( str, storage, 8192 );

    if ( cmd->conf->header ) {
        plss_format( str, "%s", cmd->conf->header );
    } else {
        plss_append_ch( str, '\n' );
    }

    if ( !cmd->parent ) {
        /* Main command. */
        main_cmd = pl_true;
    } else {
        /* Subcmd. */
        main_cmd = pl_false;
    }


    if ( main_cmd ) {
        plss_format( str, "  %s", cmd->name );
    } else {
        plss_format( str, "  Subcommand \"%s\" usage:\n    ", cmd->name );
        plss_format( str, "%s", cmd->longname );
    }

    /* Command line. */
    has_visible = pl_false;
    co = cmd->opts;
    while ( *co ) {
        if ( !( ( *co )->type & COMO_P_HIDDEN ) ) {

            has_visible = pl_true;

            plss_append_ch( str, ' ' );
            if ( ( *co )->type != COMO_SUBCMD ) {
                opt_cmdline( str, *co );
            } else {
                plss_append_c( str, "<<subcommand>>" );
                break;
            }
        }
        co++;
    }

    plss_append_c( str, "\n\n" );

    /* If cmd has subcmds, use categories: Options, Subcommands. */

    if ( !plcm_is_empty( &cmd->subcmds ) && has_visible ) {
        plss_append_c( str, "  Options:\n" );
    }

    /* Option documents. */
    co = cmd->opts;
    while ( *co ) {
        if ( ( *co )->type != COMO_SUBCMD ) {
            opt_doc( str, *co, cmd );
        }
        co++;
    }

    if ( !plcm_is_empty( &cmd->subcmds ) ) {
        plss_append_c( str, "\n  Subcommands:\n" );
    }

    /* Subcmd documents. */
    co = cmd->opts;
    while ( *co ) {
        if ( ( *co )->type == COMO_SUBCMD ) {
            opt_doc( str, *co, cmd );
        }
        co++;
    }

    if ( main_cmd ) {
        plss_format( str, "\n\n  Copyright (c) %s by %s\n", cmd->year, cmd->author );
    } else {
        plss_append_c( str, "\n" );
    }

    if ( cmd->conf->footer ) {
        plss_format( str, "%s", cmd->conf->footer );
    } else {
        plss_append_ch( str, '\n' );
    }

    fprintf( stdout, plss_string( str ) );

    plcm_del( str );

    if ( cmd->conf->help_exit ) {
        quit( EXIT_FAILURE );
    }
}


void como_display_values( FILE* fh, como_opt_t o )
{
    char**    value;
    pl_bool_t first = pl_true;

    if ( ( o->type & COMO_P_MANY ) || ( o->type & COMO_P_DEFAULT ) ) {
        fprintf( fh, "[" );
        value = plcm_data( &o->value_store );
        while ( *value ) {
            if ( !first ) {
                fprintf( fh, ", " );
            }
            fprintf( fh, "\"%s\"", *value );
            first = pl_false;
            value++;
        }
        fprintf( fh, "]" );
    } else {
        value = plcm_data( &o->value_store );
        fprintf( fh, "%s", *value );
    }
}


void como_init( pl_i64_t argc, char** argv, char* author, char* year )
{
    pl_i64_t i;

    arg_idx = 0;
    como_argc = argc - 1;

    plam_use( &como_mem, como_init_mem, COMO_INIT_MEM_SIZE );

    /* Null-terminate como_argv. */
    como_argv = plam_get( &como_mem, ( como_argc + 1 ) * sizeof( char* ) );
    for ( i = 0; i < como_argc; i++ ) {
        como_argv[ i ] = argv[ i + 1 ];
    }
    como_argv[ i ] = NULL;

    plcm_use_plam( &cmd_list, &como_mem, 16 * sizeof( como_cmd_s ) );

    como_cmd = cmd_create();

    como_cmd->author = plam_strdup( &como_mem, author );
    como_cmd->year = plam_strdup( &como_mem, year );

    como_cmd->conf = config_create();
    como_conf = como_cmd->conf;
}


void como_spec_subcmd( char* name, char* parentname, como_opt_spec_t spec, pl_i64_t size )
{
    como_opt_spec_t ts;
    como_opt_p      opts;
    como_cmd_t      cmd;
    pl_i64_t        i, i2;
    como_cmd_t      parent;

    if ( !parentname ) {
        /* Main cmd, i.e. como_cmd_s is already initially setup. */
        cmd = como_cmd;
        como_main = como_cmd;
        cmd->conf = como_conf;

        /* For main both names are the same. */
        cmd->name = plam_strdup( &como_mem, name );
        cmd->longname = plam_strdup( &como_mem, name );
    } else {
        cmd = cmd_create();
        parent = find_cmd_by_name( parentname );
        if ( !parent ) {
            como_fatal( "Parent \"%s\" does not exist!", parentname );
        }
        cmd->parent = parent;
        // register_cmd( cmd );
        add_subcmd( parent, cmd );
        cmd->conf = config_dup( parent->conf );

        /* For subcmd both longname is based on its ancestors. */
        cmd->name = plam_strdup( &como_mem, name );
        cmd->longname = plam_format( &como_mem, "%s %s", parent->longname, name );
    }

    cmd->optcnt = size;

    if ( cmd->conf->autohelp ) {
        /* Add space for help. */
        cmd->optcnt++;
    }

    /* optcnt + NULL. */
    opts = plam_get( &como_mem, ( cmd->optcnt + 1 ) * sizeof( como_opt_t ) );

    /* Insert help. */
    i = 0;
    if ( cmd->conf->autohelp ) {
        opts[ i ] = opt_create( COMO_SILENT, "help", "-h", "Display usage info." );
        opts[ i ]->type |= COMO_P_MUTEX;
        i++;
    }

    /* Create options (after help). */
    i2 = 0;
    while ( i < cmd->optcnt ) {
        ts = &spec[ i2 ];
        opts[ i ] = opt_create( ts->type, ts->name, ts->opt, ts->doc );
        i++;
        i2++;
    }
    opts[ i ] = NULL;

    cmd->opts = opts;
}


void como_cmd_end( como_cmd_t cmd )
{
    for ( pl_u64_t i = 0; i < cmd->optcnt; i++ ) {
        plcm_del( &cmd->opts[ i ]->value_store );
    }
    plcm_del( &cmd->subcmds );
}


void como_end( void )
{
    como_cmd_t cmd;

    cmd = plcm_data( &cmd_list );
    while ( (pl_t)cmd < plcm_end( &cmd_list ) ) {
        como_cmd_end( cmd );
        cmd++;
    }
    plcm_del( &cmd_list );
    plam_del( &como_mem );
}
