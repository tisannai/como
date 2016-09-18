/**
 * @file como.c
 *
 * Como - Command line parsing library.
 */


#include <mc.h>
#include <mcc.h>

#include "como.h"



/*
 * ------------------------------------------------------------
 * Como external vars.
 */

/* autoc:version */
const char* como_version = "0.1";
como_cmd_t* como_cmd = NULL;
como_cmd_t* como_main = NULL;
int como_argc = 0;
char** como_argv = NULL;



/*
 * ------------------------------------------------------------
 * Como internal vars.
 */

/** Command line argument index. */
static int arg_idx;

/** Global list of commands. */
static como_cmd_t* cmd_list[ 128 ];

/** Index for command list append. Index to list end. */
static int cmd_list_idx = 0;

/** Main command configuration. */
static como_config_t* como_conf = NULL;




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
  mc_msg_with_prefix( stderr, "COMO FATAL: ", format, ap );
  va_end( ap );
}


/**
 * Create como_cmd_t data structure.
 * 
 * @return Structure.
 */
static como_cmd_t* cmd_create( void )
{
  como_cmd_t* cmd;
  
  cmd = mc_new( como_cmd_t );
  cmd->name = NULL;
  cmd->longname = NULL;
  cmd->author = NULL;
  cmd->year = NULL;
  cmd->givencnt = 0;
  cmd->given = mc_false;
  cmd->errors = 0;
  cmd->parent = NULL;
  cmd->subcmdcnt = 0;
  cmd->subcmds = NULL;

  cmd->opts = NULL;

  return cmd;
}


/**
 * Create and setup como_opt_t data structure.
 *
 * @param type Option type.
 * @param name Option name.
 * @param opt Option mnemonic.
 * @param doc Option documentaion.
 * 
 * @return Structure.
 */
static como_opt_t* opt_create( como_opt_type_t type,
                               const char* name,
                               const char* opt,
                               const char* doc
                               )
{
  como_opt_t* co;  
  
  co = mc_new( como_opt_t );

  if ( type == COMO_DEFAULT )
    {
      /* Force these for default type. */
      co->name = "<default>";
      co->shortopt = "<default>";
    }
  else
    {
      co->name = name;
      co->shortopt = opt;
    }

  /* Convert type definition to primitives. */
  switch ( type )
    {
    case COMO_SWITCH:      type = COMO_P_NONE | COMO_P_OPT; break;
    case COMO_SINGLE:      type = COMO_P_ONE; break;
    case COMO_MULTI:       type = COMO_P_ONE | COMO_P_MANY; break;
    case COMO_OPT_SINGLE:  type = COMO_P_ONE | COMO_P_OPT; break;
    case COMO_OPT_MULTI:   type = COMO_P_ONE | COMO_P_MANY | COMO_P_OPT; break;
    case COMO_OPT_ANY:     type = COMO_P_NONE | COMO_P_ONE | COMO_P_MANY | COMO_P_OPT; break;
    case COMO_DEFAULT:     type = COMO_P_NONE | COMO_P_ONE | COMO_P_MANY | COMO_P_OPT | COMO_P_DEFAULT; break;
    case COMO_EXCLUSIVE:   type = COMO_P_NONE | COMO_P_ONE | COMO_P_MANY | COMO_P_OPT | COMO_P_MUTEX; break;
    case COMO_SILENT:      type = COMO_P_NONE | COMO_P_OPT | COMO_P_HIDDEN; break;
    default: break;
    }

  co->type = type;
  co->doc = doc;

  co->longopt = mcc_str_concat( "--", co->name, NULL );

  co->valuecnt = 0;
  co->value = NULL;
  co->given = mc_false;

  return co;
}


/**
 * Create como_config_t data structure.
 * 
 * @return Structure.
 */
static como_config_t* config_create( void )
{
  como_config_t* conf;

  conf = mc_new( como_config_t );

  /* Setup config defaults. */
  conf->autohelp = mc_true;
  conf->header = NULL;
  conf->footer = NULL;
  conf->subcheck = mc_true;
  conf->check_missing = mc_true;
  conf->check_invalid = mc_true;
  conf->tab = 12;
  conf->help_exit = mc_true;

  return conf;
}


/**
 * Duplicate configuration.
 *
 * @param src Source data.
 * 
 * @return New config.
 */
static como_config_t* config_dup( como_config_t* src )
{
  como_config_t* conf;

  conf = config_create();

  /* Setup config defaults. */
  conf->autohelp = src->autohelp;
  if ( src->header )
    conf->header = mc_strdup( src->header );
  if ( src->footer )
    conf->footer = mc_strdup( src->footer );
  conf->subcheck = src->subcheck;
  conf->check_missing = src->check_missing;
  conf->check_invalid = src->check_invalid;
  conf->tab = src->tab;
  conf->help_exit = src->help_exit;

  return conf;
}


/**
 * Register command to global lookup list.
 * 
 * @param cmd Command to add.
 */
static void register_cmd( como_cmd_t* cmd )
{
  cmd_list[ cmd_list_idx++ ] = cmd;
  cmd_list[ cmd_list_idx ] = NULL;
}


/**
 * Find command by name.
 * 
 * @param name Name.
 * 
 * @return Command.
 */
static como_cmd_t* find_cmd_by_name( char* name )
{
  for ( int i = 0; i < cmd_list_idx; i++ )
    {
      if ( strcmp( cmd_list[i]->name, name ) == 0 )
        return cmd_list[i];
    }
  return NULL;
}


/**
 * Add subcmd command to parents list.
 * 
 * @param parent Host for subcmd.
 * @param subcmd Subcmd to add.
 */
static void add_subcmd( como_cmd_t* parent, como_cmd_t* subcmd )
{
  if ( !parent->subcmds )
    {
      parent->subcmds = mc_new_n(como_cmd_t*,128);
    }

  if ( parent->subcmdcnt >= 128 )
    como_fatal( "Too many sub-commands for \"%s\"!", parent->longname );

  parent->subcmds[ parent->subcmdcnt++ ] = subcmd;
}


/**
 * Find option by type.
 * 
 * @param cmd Command including option.
 * @param type Option type.
 * 
 * @return Option (or NULL).
 */
static como_opt_t* find_opt_by_type( como_cmd_t* cmd, como_opt_type_t type )
{
  int i = 0;

  while ( cmd->opts[ i ] )
    {
      if ( cmd->opts[i]->type & type )
        {
          return cmd->opts[i];
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
static como_opt_t* find_opt_by_name( como_cmd_t* cmd, char* name )
{
  int i = 0;

  if ( name == NULL )
    {
      /* Find default arg. */
      return find_opt_by_type( cmd, COMO_P_DEFAULT );
    }
  else
    {
      while ( cmd->opts[ i ] )
        {
          if ( strcmp( cmd->opts[i]->name, name ) == 0 )
            {
              return cmd->opts[i];
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
static como_opt_t* find_opt( como_cmd_t* cmd, char* str )
{
  int i = 0;

  if ( str == NULL )
    {
      /* Default option. */
      while ( cmd->opts[ i ] )
        {
          if ( cmd->opts[i]->type & COMO_P_DEFAULT )
            {
              return cmd->opts[i];
            }
          i++;
        }
    }
  else if ( strncmp( str, "--", 2 ) == 0 )
    {
      /* Long option. */
      while ( cmd->opts[ i ] )
        {
          if ( strcmp( cmd->opts[i]->longopt, str ) == 0 )
            {
              return cmd->opts[i];
            }
          i++;
        }
    }
  else if ( str[0] == '-' )
    {
      /* Short option. */
      while ( cmd->opts[ i ] )
        {
          if ( cmd->opts[i]->shortopt
               && ( strcmp( cmd->opts[i]->shortopt, str ) == 0 ) )
            {
              return cmd->opts[i];
            }
          i++;
        }
    }
  else
    {
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
static mc_bool_t is_opt( void )
{
  char* s;

  s = get_arg();

  if ( s[0] == '-' )
    return mc_true;
  else
    return mc_false;
}


/**
 * Is option of switch type? Note that default type is also consided a
 * switch.
 * 
 * @param opt Option to check.
 * 
 * @return True if is.
 */
static mc_bool_t has_switch_style_doc( como_opt_t* opt )
{
  if ( ( ( opt->type & COMO_P_NONE )
         && !( opt->type & COMO_P_MANY ) )
       || ( opt->type & COMO_P_DEFAULT ) )
    return mc_true;
  else
    return mc_false;
}


/**
 * Get multiple command line arguments upto to next option or end.
 * 
 * 
 * @return Array of arguments.
 */
static char** get_option_values( int* valuecnt )
{
  int cnt = 0;
  int org_idx = arg_idx;
  char** value;
  int i;

  /* Count the number of args provided. */
  while ( get_arg() && !is_opt() )
    {
      next_arg();
      cnt++;
    }
                      
  arg_idx = org_idx;

  /* Allocate space for value array. */
  value = mc_new_n(char*, cnt+1 );
                      
  for ( i = 0; i < cnt; i++ )
    {
      value[i] = get_arg();
      next_arg();
    }

  value[i] = NULL;
  *valuecnt = cnt;

  return value;
}


/**
 * Add item to char pointer array pointer. Create new array is it
 * doesn't exist.
 * 
 * @param [out] storage Pointer where allocation is stored.
 * @param [in] item Item to store.
 */
static void add_value( char*** storage, char* item )
{
  char** items;

  if ( *storage == NULL )
    {
      /* First. */
      items = mc_new_n(char*, 2 );
      items[0] = item;
      items[1] = NULL;
    }
  else
    {
      /* Non-first .*/
      int size = 0;

      items = *storage;
      while ( (*items) )
        {
          items++;
          size++;
        }

      items = *storage;
      items = mc_realloc( items, (size+2)*sizeof(char*) );
      items[size] = item;
      items[size+1] = NULL;
    }

  *storage = items;
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
static mc_bool_t check_missing( como_cmd_t* cmd, como_cmd_t** errcmd )
{
  como_opt_t** opts, *o;
  mc_bool_t ret = mc_true;
  mc_bool_t subcheck;
  como_cmd_t* subcmd;

  if ( !cmd->conf->check_missing )
    return mc_true;

  /* Check for any exclusive args first. Missing are not checked if has exclusives. */
  opts = cmd->opts;
  while ( *opts )
    {
      o = *opts;
      if ( ( o->type & COMO_P_MUTEX ) && o->given )
        return ret;
      opts++;
    }

  /* Check for missing options. */
  opts = cmd->opts;
  while ( *opts )
    {
      o = *opts;
      if ( ( o->type != COMO_SUBCMD ) && !( o->type & COMO_P_OPT ) && !o->given )
        {
          como_error( "Option \"%s\" missing for \"%s\"...", como_opt_id( o ), cmd->longname );
          *errcmd = cmd;
          return mc_false;
        }
      opts++;
    }

  /* Check for missing subcmds. */
  if ( cmd->subcmds )
    {
      /* Has subcmds. */
      if ( cmd->conf->subcheck )
        subcheck = mc_true;
      else
        subcheck = mc_false;
    }
  else
    subcheck = mc_false;

  subcmd = como_cmd_given_subcmd( cmd );

  if ( subcmd )
    {
      /* Go to level subcmd level. */
      return check_missing( subcmd, errcmd );
    }
  else if ( subcheck )
    {
      como_error( "Subcommand required for \"%s\"...", cmd->name );
      *errcmd = cmd;
      return mc_false;
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
static int parse_opts( como_cmd_t* cmd, como_cmd_t** subcmd )
{
  como_opt_t* o;
  como_cmd_t* c;

  while ( get_arg() )
    {

      /* Option terminator?. */
      if ( strcmp( "--", get_arg() ) == 0 )
        {
          /*  Rest of the args do not belong to this program. */
          next_arg();
          como_cmd->external = &( como_argv[ arg_idx ] );
          break;
        }

      else if ( is_opt() )
        {
          
          /* Normal option. */

          o = find_opt( cmd, get_arg() );

          if ( !o )
            {

              /* Not found, might be default. */

              if ( cmd->conf->check_invalid )
                {
                  /* Report missing. */
                  como_error( "Unknown option \"%s\"...", get_arg() );
                  break;
                }
              else
                {
                  /* Default option. */
                  o = find_opt_by_type( cmd, COMO_P_DEFAULT );
                  if ( !o )
                    {
                      como_error( "No default option specified to allow \"%s\"...", get_arg() );
                      break;
                    }
                  else
                    {
                      if ( !o->value )
                        cmd->givencnt++;
                      add_value( &( o->value ), get_arg() );
                      o->valuecnt++;
                    }
                }
            }
          else if ( o && ( ( o->type & COMO_P_ONE ) || ( o->type & COMO_P_MANY ) ) )
            {

              /* Option with arguments. */

              next_arg();

              if ( ( !get_arg() || is_opt() )
                   && !( o->type & COMO_P_NONE ) )
                {
                  como_error( "No argument given for \"%s\"...", como_opt_id( o ) );
                  break;
                }
              else
                {
                  if ( o->type & COMO_P_MANY )
                    {
                      /* Get all argument for multi-option. */
                      o->value = get_option_values( &( o->valuecnt ) );
                    }
                  else
                    {
                      if ( o->given )
                        {
                          como_error( "Too many arguments for option (\"%s\")...",
                                      como_opt_id( o ) );
                          break;
                        }

                      o->value = mc_new_n(char*, 2 );
                      o->value[0] = get_arg();
                      o->value[1] = NULL;
                      o->valuecnt++;
                      next_arg();
                    }

                  o->given = mc_true;
                  cmd->givencnt++;

                }
            }
          else
            {

              /* Switch option. */
              o->given = mc_true;
              cmd->givencnt++;
              next_arg();
            }
        }
      else
        {

          /* Subcmd or default. Check for Subcmd first. */
          o = find_opt( cmd, get_arg() );

          if ( !o || o->type != COMO_SUBCMD )
            {

              /* Default argument. */

              o = find_opt_by_type( cmd, COMO_P_DEFAULT );

              if ( !o )
                {
                  como_error( "No default option specified to allow \"%s\"...", get_arg() );
                  next_arg();
                }
              else
                {
                  if ( !o->value )
                    cmd->givencnt++;
                  add_value( &( o->value ), get_arg() );
                  o->valuecnt++;
                  o->given = mc_true;
                  next_arg();
                }
            }
          else
            {

              /* Subcmd. */

              /* Search for Subcmd. */
              c = find_cmd_by_name( get_arg() );
              o->given = mc_true;
              c->given = mc_true;
              next_arg();
              *subcmd = c;
              return 1;
            }

        }
    }

  if ( como_cmd->errors > 0 )
    {
      cmd->errors = como_cmd->errors;
      *subcmd = cmd;
      return 2;
    }
  else
    return 0;
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
static mc_bool_t setup_and_parse( como_cmd_t* cmd, como_cmd_t** errcmd )
{
  int ret;
  como_cmd_t* subcmd;

  ret = parse_opts( cmd, &subcmd );

  if ( ret == 0 )
    {
      /* done.*/
      return mc_true;
    }
  else if ( ret == 1 )
    {
      /* continue. */
      return setup_and_parse( subcmd, errcmd );
    }
  else if ( ret == 2 )
    {
      /* error. */
      *errcmd = subcmd;
      return mc_false;
    }
  else
    {
      return mc_true;
    }
}


/**
 * Add option's command line formatting (usage) to str.
 *
 * @param [out] str String where command line is stored.
 * @param [in] o Option to add.
 */
static void opt_cmdline( mcc_t* str, como_opt_t* o )
{
  if ( o->type & COMO_P_HIDDEN )
    return;

  if ( o->type & COMO_P_OPT )
    mcc_printf( str, "[" );

  mcc_printf( str, "%s", como_opt_id( o ) );

  if ( !has_switch_style_doc( o ) )
    {
      mcc_printf( str, " <%s>", o->name );

      if ( ( o->type & COMO_P_NONE ) && ( o->type & COMO_P_MANY ) )
        mcc_printf( str, "*" );
      else if ( o->type & COMO_P_MANY )
        mcc_printf( str, "+" );
    }

  if ( o->type & COMO_P_OPT )
    mcc_printf( str, "]" );
}


/**
 * Add option documentation line.
 *
 * @param str String where document is stored.
 * @param o Option to document.
 * @param cmd Containing command (for configuration lookup).
 */
static void opt_doc( mcc_t* str, como_opt_t* o, como_cmd_t* cmd )
{
  char format[128];
  int s, e;
  mc_bool_t first;

  if ( o->type & COMO_P_HIDDEN )
    return;

  sprintf( format, "  %%-%ds", cmd->conf->tab );
   
  /* Reformat doc str, so that newlines start a new line and tab
   * chars align the start to previous line. */

  first = mc_true;
  s = 0;
  e = 0;
  for (;;)
    {
      if ( o->doc[e] == '\n' || o->doc[e] == 0 )
        {
          if ( first )
            {
              mcc_printf( str, format, como_opt_id( o ) );
              mcc_append_n( str, (char*) &( o->doc[s] ), (e-s) );
              mcc_append( str, '\n' );

              if ( o->doc[e] == 0 ) return;
              else e++;

              first = mc_false;
            }
          else
            {
              if ( o->doc[s] == '\t' )
                {
                  s++;
                  mcc_printf( str, format, "" );
                }
              mcc_append_n( str, (char*) &( o->doc[s] ), (e-s) );
              mcc_append( str, '\n' );

              if ( o->doc[e] == 0 ) return;
              else e++;

            }
          s = e;

        }
      else
        e++;
    }
}


/**
 * Display help if help option is given for any of the commands in the
 * hierarchy (recursion).
 * 
 * @param cmd Command to search given help.
 */
static void usage_if_help( como_cmd_t* cmd )
{
  como_cmd_t* subcmd;

  if ( como_cmd_given( cmd, "help" ) )
    como_cmd_usage( cmd );
  else if ( cmd->subcmds
            && ( subcmd = como_cmd_given_subcmd( cmd ) ) )
    usage_if_help( subcmd );
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
  como_cmd_t* cmd;
  mc_bool_t success;
  como_cmd_t* errcmd;

  /* Parse all arguments and fill information to options. */

  como_cmd = como_main;
  cmd = como_main;

  success = setup_and_parse( cmd, &errcmd );

  if ( !success )
    {
      como_cmd_usage( errcmd );
      quit( EXIT_FAILURE );
    }
  else if ( !check_missing( cmd, &errcmd ) )
    {
      como_cmd_usage( errcmd );
      quit( EXIT_FAILURE );
    }
  else
    {
      usage_if_help( como_cmd );
    }
}


como_opt_t* como_opt( char* name )
{
  return find_opt_by_name( como_cmd, name );
}


char** como_value( char* name )
{
  como_opt_t* co;
  co = find_opt_by_name( como_cmd, name );
  return co->value;
}


como_opt_t* como_given( char* name )
{
  como_opt_t* co;
  co = find_opt_by_name( como_cmd, name );
  if ( co->given )
    return co;
  else
    return NULL;
}


como_opt_t* como_cmd_opt( como_cmd_t* cmd, char* name )
{
  return find_opt_by_name( cmd, name );
}


char** como_cmd_value( como_cmd_t* cmd, char* name )
{
  como_opt_t* co;
  co = find_opt_by_name( cmd, name );
  return co->value;
}


como_opt_t* como_cmd_given( como_cmd_t* cmd, char* name )
{
  como_opt_t* co;
  co = find_opt_by_name( cmd, name );
  if ( co->given )
    return co;
  else
    return NULL;
}


como_cmd_t* como_cmd_subcmd( como_cmd_t* cmd, char* name )
{
  for ( int i = 0; i < cmd_list_idx; i++ )
    {
      if ( strcmp( cmd->subcmds[i]->name, name ) == 0 )
        return cmd->subcmds[i];
    }
  return NULL;
}


como_cmd_t* como_given_subcmd( void )
{
  return como_cmd_given_subcmd( como_cmd );
}


como_cmd_t* como_cmd_given_subcmd( como_cmd_t* parent )
{
  for ( int i = 0; i < parent->subcmdcnt; i++ )
    {
      if ( parent->subcmds[i]->given )
        return parent->subcmds[i];
    }
  return NULL;
}


char** como_external( void )
{
  return como_main->external;
}


const char* como_opt_id( como_opt_t* opt )
{
  if ( opt->type != COMO_SUBCMD )
    return opt->shortopt ? opt->shortopt : opt->longopt;
  else
    return opt->name;
}


/*
 * Functions to set configuration items.
 */

void como_conf_autohelp( mc_bool_t val ) { como_cmd->conf->autohelp = val; }

void como_conf_header( char* val ) { como_cmd->conf->header = mc_strdup( val ); }

void como_conf_footer( char* val ) { como_cmd->conf->footer = mc_strdup( val ); }

void como_conf_subcheck( mc_bool_t val ) { como_cmd->conf->subcheck = val; }

void como_conf_check_missing( mc_bool_t val ) { como_cmd->conf->check_missing = val; }

void como_conf_check_invalid( mc_bool_t val ) { como_cmd->conf->check_invalid = val; }

void como_conf_tab( int val ) { como_cmd->conf->tab = val; }

void como_conf_help_exit( mc_bool_t val ) { como_cmd->conf->help_exit = val; }


void como_error( const char* format, ... )
{
  char prefix[128];

  como_cmd->errors++;
  sprintf( prefix, "\n%s error: ", como_cmd->name );
  va_list ap;
  va_start( ap, format );
  mc_msg_with_prefix( stderr, prefix, format, ap );
  va_end( ap );
}


void como_usage( void )
{
  como_cmd_usage( como_cmd );
}


void como_cmd_usage( como_cmd_t* cmd )
{
  mcc_t* str;
  como_opt_t** co;
  mc_bool_t main_cmd, has_visible;

  str = mcc_new();
  str->resize = mcc_enlarge_resizer;

  if ( cmd->conf->header )
    {
      mcc_printf( str, "%s", cmd->conf->header );
    }
  else
    {
      mcc_printf( str, "\n" );
    }

  if ( !cmd->parent )
    /* Main command. */
    main_cmd = mc_true;
  else
    /* Subcmd. */
    main_cmd = mc_false;


  if ( main_cmd )
    mcc_printf( str, "  %s", cmd->name );
  else
    {
      mcc_printf( str, "  Subcommand \"%s\" usage:\n    ", cmd->name );
      mcc_printf( str, "%s", cmd->longname );
    }

  /* Command line. */
  has_visible = mc_false;
  co = cmd->opts;
  while ( *co )
    {
      if ( !( (*co)->type & COMO_P_HIDDEN ) )
        {

          has_visible = mc_true;

          mcc_printf( str, " " );
          if ( (*co)->type != COMO_SUBCMD )
            opt_cmdline( str, *co );
          else
            {
              mcc_printf( str, "<<subcommand>>" );
              break;
            }
        }
      co++;
    }

  mcc_printf( str, "\n\n" );

  /* If cmd has subcmds, use categories: Options, Subcommands. */

  if ( cmd->subcmds && has_visible )
    mcc_printf( str, "  Options:\n" );
       
  /* Option documents. */
  co = cmd->opts;
  while ( *co )
    {
      if ( (*co)->type != COMO_SUBCMD )
        opt_doc( str, *co, cmd );
      co++;
    }

  if ( cmd->subcmds )
    mcc_printf( str, "\n  Subcommands:\n" );

  /* Subcmd documents. */
  co = cmd->opts;
  while ( *co )
    {
      if ( (*co)->type == COMO_SUBCMD )
        opt_doc( str, *co, cmd );
      co++;
    }

  if ( main_cmd )
    {
      mcc_printf( str, "\n\n  Copyright (c) %s by %s\n", cmd->year, cmd->author );
    }
  else
    {
      mcc_printf( str, "\n" );
    }

  if ( cmd->conf->footer )
    {
      mcc_printf( str, "%s", cmd->conf->footer );
    }
  else
    {
      mcc_printf( str, "\n" );
    }

  fprintf( stdout, mcc_to_str( str ) );

  mcc_del( str );

  if ( cmd->conf->help_exit )
    {
      quit( EXIT_FAILURE );
    }

}


void como_display_values( FILE* fh, como_opt_t* o )
{
  char** value;
  mc_bool_t first = mc_true;
   
  if ( ( o->type & COMO_P_MANY )
       || ( o->type & COMO_P_DEFAULT ) )
    {
      fprintf( fh, "[" );
      value = o->value;
      while ( *value )
        {
          if ( !first )
            fprintf( fh, ", " );
          fprintf( fh, "\"%s\"", *value );
          first = mc_false;
          value++;
        }
      fprintf( fh, "]" );
    }
  else
    fprintf( fh, "%s", o->value[0] );
}


void como_init( int argc, char** argv, char* author, char* year )
{
  int i;

  arg_idx = 0;
  como_argc = argc-1;
  
  /* Null-terminate como_argv. */
  como_argv = mc_new_n( char*, como_argc+1 );
  for ( i = 0; i < como_argc; i++ )
    {
      como_argv[ i ] = argv[ i+1 ];
    }

  como_argv[ i ] = NULL;

  como_cmd = cmd_create();
  register_cmd( como_cmd );

  como_cmd->author = mc_strdup( author );
  como_cmd->year = mc_strdup( year );

  como_cmd->conf = config_create();
  como_conf = como_cmd->conf;
}


void como_spec_subcmd( char* name,
                       char* parentname,
                       como_opt_spec_t* spec,
                       int size )
{
  como_opt_spec_t* ts;
  como_opt_t** opts;
  como_cmd_t* cmd;
  int i, i2;
  como_cmd_t* parent;

  if ( !parentname )
    {
      /* Main cmd, i.e. como_cmd_t is already initially setup. */
      cmd = como_cmd;
      como_main = como_cmd;
      cmd->conf = como_conf;

      /* For main both names are the same. */
      cmd->name = mc_strdup( name );
      cmd->longname = mc_strdup( name );
    }
  else
    {
      cmd = cmd_create();
      parent = find_cmd_by_name( parentname );
      if ( !parent )
        como_fatal( "Parent \"%s\" does not exist!", parentname );
      cmd->parent = parent;
      register_cmd( cmd );
      add_subcmd( parent, cmd );
      cmd->conf = config_dup( parent->conf );

      /* For subcmd both longname is based on its ancestors. */
      cmd->name = mc_strdup( name );
      cmd->longname = mcc_str_concat( parent->longname, " ", name, NULL );
    }

  cmd->optcnt = size;

  if ( cmd->conf->autohelp )
    /* Add space for help. */
    cmd->optcnt++;

  /* optcnt + NULL. */
  opts = mc_new_n( como_opt_t*, cmd->optcnt+1 );

  /* Insert help. */
  i = 0;
  if ( cmd->conf->autohelp )
    {
      opts[ i ] = opt_create( COMO_SILENT, "help", "-h", "Display usage info." );
      i++;
    }

  /* Create options (after help). */
  i2 = 0;
  while ( i < cmd->optcnt )
    {
      ts = &spec[i2];
      opts[ i ] = opt_create( ts->type, ts->name, ts->opt, ts->doc );
      i++;
      i2++;
    }
  opts[ i ] = NULL;

  cmd->opts = opts;
}


void como_cmd_end( como_cmd_t* cmd )
{
  mc_free( cmd->name );
  mc_free( cmd->longname );

  /* Author and year are missing from subcmds. */
  if ( cmd->author )
    mc_free( cmd->author );

  if ( cmd->year )
    mc_free( cmd->year );

  if ( cmd->conf )
    {
      if ( cmd->conf->header )
        mc_free( cmd->conf->header );
      if ( cmd->conf->footer )
        mc_free( cmd->conf->footer );
      mc_free( cmd->conf );
    }

  for_n( cmd->optcnt )
    {
      mc_free( cmd->opts[i]->longopt );
      mc_free( cmd->opts[i]->value );
      mc_free( cmd->opts[i] );
    }

  if ( cmd->subcmds )
    mc_free( cmd->subcmds );

  mc_free( cmd->opts );
  mc_free( cmd );
}


void como_end( void )
{
  como_cmd_t** cp = cmd_list;

  mc_free( como_argv );

  while ( *cp )
    {
      como_cmd_end( *cp );
      cp++;
    }
}
