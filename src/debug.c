/*
  debug.c

  This file keeps the debug() efun. All debug information and
  debug switches are managed from here.

*/
#include <varargs.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>		/* sys/types.h and netinet/in.h are here to enable include of comm.h below */
#include <sys/stat.h>
/* #include <netinet/in.h> Included in comm.h below */
#include <memory.h>

#include "config.h"
#include "lint.h"
#include "mstring.h"
#include "exec.h"
#include "interpret.h"
#include "swap.h"
#include "udpsvc.h"

#ifdef RUSAGE			/* Defined in config.h */
#ifdef SOLARIS
#include <sys/times.h>
#include <limits.h>
#else
#include <sys/resource.h>
extern int getrusage (int, struct rusage *);
#ifdef sun
extern int getpagesize (void);
#endif
#ifndef RUSAGE_SELF
#define RUSAGE_SELF	0
#endif
#endif /* SOLARIS */
#ifdef PROFILE_OBJS
static struct vector *make_cpu_array (int,struct program *[]); 
#endif
#endif


#include "object.h"
#include "instrs.h"
#include "patchlevel.h"
#include "comm.h"
#include "mapping.h"
#include "mudstat.h"
#include "bibopmalloc.h"
#include "simulate.h"
#include "comm1.h"
#include "super_snoop.h"

#include "inline_svalue.h"

#define DUMP_FILE "OBJECT_DUMP"

/*
 * The array below is the available subcommands to the debug() efun.
 *
 */
       			/*   Name		   Number   Params  */
static	char	*debc[] = { "index",		/* 0 */
			    "malloc", 		/* 1 */
			    "status", 		/* 2 */
			    "status tables",	/* 3 */
			    "mudstatus",	/* 4 	    on/off eval time */
			    "functionlist",	/* 5 	    object */
			    "rusage",		/* 6 */
			    "top_ten_cpu",	/* 7 */
			    "object_cpu",	/* 8 	    object */
			    "swap",		/* 9 	    object */
			    "version",		/* 10 */
			    "trace",		/* 11 	    bitmask */
			    "traceprefix",	/* 12       pathstart */
			    "call_out_info",	/* 13       object */
			    "inherit_list",	/* 14	    object */
			    "load_average",	/* 15 */
			    "shutdown",		/* 16 */
			    "object_info",	/* 17 	    num object */
			    "opcdump",		/* 18 */
			    "send_udp",		/* 19       host, port, msg */
			    "mud_port",		/* 20       */
			    "udp_port",		/* 21       */
			    "ob_flags",	        /* 22       ob */
			    "get_variables",	/* 23       ob null/varname */
			    "get_eval_cost",	/* 24 */
                            "debug malloc",     /* 25 */
			    "getprofile",	/* 26	    object */
			    "get_avg_response",	/* 27 */
			    "destruct",         /* 28       object */
			    "destroy",          /* 29       object */
			    "update snoops",    /* 30 */
			    "dump_objects",     /* 31 */
			    "set_swap",         /* 32
			    ({min_mem, max_mem, min_time, max_time}) */
			    "query_swap",       /* 33 */
			    "functions",	/* 34 */
			    "inhibitcallouts",  /* 35       on/off */
			    "warnobsolete",     /* 36       on/off */
			    "shared_strings",	/* 37 */
			    0
			  };

extern struct vector *inherit_list (struct object *);
#ifdef FUNCDEBUG
void dumpfuncs(void);
#endif
#ifdef OPCPROF
void opcdump(void);
#endif

struct svalue *
debug_command(char *debcmd, int argc, struct svalue *argv)
{
    static struct svalue retval;
    int dbnum, dbi, il;

    for (dbi = -1, dbnum = 0; debc[dbnum]; dbnum++)
    {
	if (strcmp(debcmd, debc[dbnum]) == 0)
	    dbi = dbnum;
    }
    if (dbi < 0)
    {
	retval.type = T_NUMBER;
	retval.u.number = 0;
	return &retval;
    }

    switch (dbi)
    {
    case 0: /* index */
	retval.type = T_POINTER;
	retval.u.vec = allocate_array(dbnum);
	for (il = 0; il < dbnum; il++)
	{
	    retval.u.vec->item[il].type = T_STRING;
	    retval.u.vec->item[il].string_type = STRING_CSTRING;
	    retval.u.vec->item[il].u.string = debc[il];
	}
	return &retval;
    case 1: /* malloc */
	retval.type = T_STRING;
	retval.string_type = STRING_MSTRING;
	retval.u.string = make_mstring((char *)dump_malloc_data());
	return &retval;
    case 2: /* status */
    case 3: /* status tables */
	retval.type = T_STRING;
	retval.string_type = STRING_MSTRING;
	retval.u.string = (char *)get_gamedriver_info(debc[dbi]);
	return &retval;
    case 4: /* mudstatus on/off eval_lim time_lim */
	if (argc < 3 || 
	    argv[0].type != T_STRING ||
	    argv[1].type != T_NUMBER ||
	    argv[2].type != T_NUMBER)
	    break;
	if (strcmp(argv[0].u.string, "on") == 0)
	    mudstatus_set(1, argv[1].u.number, argv[2].u.number);
	else if (strcmp(argv[0].u.string, "off") == 0)
	    mudstatus_set(0, argv[1].u.number, argv[2].u.number);
	else
	    break;
	retval.type = T_NUMBER;
	retval.u.number = 1;
	return &retval;
    case 5: /* functionlist object */
	if (argc < 1 || argv[0].type != T_OBJECT)
	    break;
#ifdef USE_SWAP
	access_program(argv->u.ob->prog);
#endif
	retval.type = T_POINTER;
	retval.u.vec = allocate_array(argv[0].u.ob->prog->num_functions);
	for (il = 0; il < (int)argv[0].u.ob->prog->num_functions; il++)
	{
	    retval.u.vec->item[il].type = T_STRING;
	    retval.u.vec->item[il].string_type = STRING_SSTRING;
	    retval.u.vec->item[il].u.string = 
		reference_sstring(argv[0].u.ob->prog->functions[il].name);
	}
#ifdef USE_SWAP
	access_program(current_prog);
#endif
	return &retval;
#ifdef RUSAGE /* Only defined if we compile GD with RUSAGE */
    case 6: /* rusage */
    {
	char buff[1000];
#ifdef SOLARIS
	struct tms buffer;

	if (times(&buffer) == -1)
	    buff[0] = 0;
	else
	    (void)sprintf(buff, "%ld %ld (s/%ld)",
		    buffer.tms_utime, buffer.tms_stime, CLK_TCK);
#else
	struct rusage rus;
	long utime, stime;
	long maxrss;
	
	if (getrusage(RUSAGE_SELF, &rus) < 0)
	    buff[0] = 0;
	else {
	    utime = rus.ru_utime.tv_sec * 1000 + rus.ru_utime.tv_usec / 1000;
	    stime = rus.ru_stime.tv_sec * 1000 + rus.ru_stime.tv_usec / 1000;
	    maxrss = rus.ru_maxrss;
#ifdef sun
	    maxrss *= getpagesize() / 1024;
#endif
	    (void)sprintf(buff, "user time used\t\t\t%ld\nsystem time used\t\t%ld\n",
		    utime, stime);
	    (void)sprintf(buff, "%smaximum resident set size\t%ld\n",	    
		     buff, maxrss);
	    (void)sprintf(buff, "%sintegral shared memory size\t%ld\n",
		     buff, rus.ru_ixrss);
	    (void)sprintf(buff, "%sintegral unshared memory size\t%ld\n",
		     buff, rus.ru_idrss);
	    (void)sprintf(buff, "%sintegral unshared stack size\t%ld\n",
		     buff, rus.ru_isrss);
	    (void)sprintf(buff, "%spage reclaims\t\t\t%ld\npage faults\t\t\t%ld\n",
		     buff, rus.ru_minflt, rus.ru_majflt);
	    (void)sprintf(buff, "%sswaps\t\t\t\t%ld\nblock input operations\t\t%ld\n",
		     buff, rus.ru_nswap, rus.ru_inblock);
	    (void)sprintf(buff, "%sblock output operations\t\t%ld\nmessages sent\t\t\t%ld\n",
		     buff, rus.ru_oublock, rus.ru_msgsnd);
	    (void)sprintf(buff, "%smessages received\t\t%ld\nsignals received\t\t%ld\n",
		     buff, rus.ru_msgrcv, rus.ru_nsignals);
	    (void)sprintf(buff, "%svoluntary context switches\t%ld\ninvoluntary context switches\t%ld\n",
		     buff, rus.ru_nvcsw, rus.ru_nivcsw);
	    (void)sprintf(buff, "%s\n", buff);
	}
#endif /* SOLARIS */
	retval.type = T_STRING;
	retval.string_type = STRING_MSTRING;
	retval.u.string = make_mstring(buff);
	return &retval;
    }
#ifdef PROFILE_OBJS
    case 7: /* top_ten_cpu */
    {
#define NUMBER_OF_TOP_TEN 10
	struct program *p[NUMBER_OF_TOP_TEN];
	struct object *ob; 
	struct vector *v;
	int i, j;
	for(i = 0; i < NUMBER_OF_TOP_TEN; i++) 
	    p[i] = (struct program *)0L;
	ob = obj_list;
	do
	{
	    if (!ob->prog)  /* That this case exists is just weird /JnA */
		continue;

	    for(i = NUMBER_OF_TOP_TEN-1; i >= 0; i--) 
	    {
		if ( p[i] && (ob->prog->cpu <= p[i]->cpu))
		    break;
	    }

	    if (i < (NUMBER_OF_TOP_TEN - 1)) 
		for (j = 0; j <= i; j++)
		    if (strcmp(p[j]->name,ob->prog->name) == 0)
		    {
			i = NUMBER_OF_TOP_TEN-1;
			break;
		    }

	    if (i < (NUMBER_OF_TOP_TEN - 1)) 
	    {
		j = NUMBER_OF_TOP_TEN - 2;
		while(j > i) 
		{
		    p[j + 1] = p[j];
		    j--;
		}
		p[i + 1] = ob->prog;
	    }
	} while (obj_list != (ob = ob->next_all));
	v = make_cpu_array(NUMBER_OF_TOP_TEN, p);        
	if (v) 
	{                                                   
	    retval.type = T_POINTER;
	    retval.u.vec = v;
	    return &retval;
	}
	break;
#undef NUMBER_OF_TOP_TEN
    }
#else
    case 7:
	retval.type = T_STRING;
	retval.string_type = STRING_CSTRING;
	retval.u.string = "Only valid if GD compiled with PROFILE_OBJS flag.\n";
	return &retval;
#endif
    case 8: /* object_cpu object */
    {
	int c_num;

	if (argc && (argv[0].type == T_OBJECT)) 
	{
#ifdef PROFILE_OBJS
	    long cpu = argv[0].u.ob->prog->cpu;
	    c_num = (int)cpu;
#else
	    retval.type = T_STRING;
	    retval.string_type = STRING_CSTRING;
	    retval.u.string = "Only valid if GD compiled with PROFILE_OBJS flag.\n";
	    return &retval;
#endif
	} 
	else 
	{
#ifdef SOLARIS
	    struct tms buffer;
	    
	    if (times(&buffer) == -1)
		c_num = -1;
	    else
		c_num = buffer.tms_utime + buffer.tms_stime;
#else
	    struct rusage rus;         

	    if (getrusage(RUSAGE_SELF, &rus) < 0) 
	    {  
		c_num = -1;
	    }
	    else 
	    {                                                               
		c_num =  (int)(rus.ru_utime.tv_sec * 1000 + 
			       rus.ru_utime.tv_usec / 1000 +
			       rus.ru_stime.tv_sec * 1000 + 
			       rus.ru_stime.tv_usec / 1000);
	    }
#endif
        }
	retval.type = T_NUMBER;
	retval.u.number = c_num;
	return &retval;
    }
#else /* RUSAGE */
    case 6:
    case 7: /* rusage, top_ten_cpu and object_cpu */
    case 8:
	retval.type = T_STRING;
	retval.string_type = STRING_CSTRING;
	retval.u.string = "Only valid if GD compiled with RUSAGE flag.\n";
	return &retval;
#endif /* RUSAGE */
    
    case 9:  /*	swap,		object 		*/
#if 0        /* can not swap while executing */
	if (argc && (argv[0].type == T_OBJECT))
	    (void)swap(argv[0].u.ob);
#endif
	retval = const1;
	return &retval;
    case 10: /*	version,		  	*/
    {
	char buff[11];
	(void)sprintf(buff, "%6.8s%02d", GAME_VERSION, PATCH_LEVEL);
	retval.type = T_STRING;
	retval.string_type = STRING_MSTRING;
	retval.u.string = make_mstring(buff);
	return &retval;
    }
    case 11: /* trace, 		bitmask		*/
    {
#ifdef TRACE_CODE
	int ot = -1;
	extern struct object *current_interactive;
	if (current_interactive && current_interactive->interactive) 
	{
	    if (argc && (argv[0].type == T_NUMBER))
	    {
		ot = current_interactive->interactive->trace_level;
		current_interactive->interactive->trace_level = argv[0].u.number;
	    }
	}

	retval.type = T_NUMBER;
	retval.u.number = ot;
	return &retval;
#else
	retval.type = T_STRING;
	retval.string_type = STRING_CSTRING;
	retval.u.string = "Only valid if GD compiled with TRACE_CODE flag.\n";
	return &retval;
#endif
    }
    case 12: /* traceprefix, 	pathstart	*/
    {
#ifdef TRACE_CODE
	char *old = 0;

	extern struct object *current_interactive;
	if (current_interactive && current_interactive->interactive) 
	{
	    if (argc)
	    {
		old = current_interactive->interactive->trace_prefix;
		if (argv[0].type == T_STRING) 
		{
		    current_interactive->interactive->trace_prefix = 
			make_sstring(argv[0].u.string);
		} 
		else
		    current_interactive->interactive->trace_prefix = 0;
	    }
	}

	if (old) 
	{
	    retval.type = T_STRING;
	    retval.string_type = STRING_SSTRING;
	    retval.u.string = old;
	} 
	else 
	    retval = const0;

	return &retval;
#else
	retval.type = T_STRING;
	retval.string_type = STRING_CSTRING;
	retval.u.string = "Only valid if GD compiled with TRACE_CODE flag.\n";
	return &retval;
#endif
    }
    case 13: /*	call_out_info,	  		*/
	{
	    extern struct vector *get_calls(struct object *);
	    if (argv[0].type != T_OBJECT)
		break;
	    retval.type = T_POINTER;
	    retval.u.vec =  get_calls(argv[0].u.ob);
	    return &retval;
	}
    case 14: /* inherit_list, 	object		*/
	if (argc && (argv[0].type == T_OBJECT))
	{
	    retval.type = T_POINTER;
	    retval.u.vec = inherit_list(argv[0].u.ob);
	    return &retval;
	}
	else
	{
	    retval = const0;
	    return &retval;
	}
    case 15: /*	load_average,	  		*/
	retval.type = T_STRING;
	retval.string_type = STRING_MSTRING;
	retval.u.string = make_mstring(query_load_av());
	return &retval;

    case 16: /*	shutdown,		  	*/
	startshutdowngame(0);
	retval = const1;
	return &retval;
	    
    case 17: /* "object_info",	num object 	*/
    {
	struct object *ob;
	char db_buff[1024], tdb[200];
	int i;
	
	if (argc < 2 || argv[0].type != T_NUMBER || argv[1].type != T_OBJECT)
	    break;

	if (argv[0].u.number == 0) 
	{
	    int flags;
	    struct object *obj2;
	    
	    if ( argv[1].type != T_OBJECT)
		break;
	    ob = argv[1].u.ob;
	    flags = ob->flags;
	    (void)sprintf(db_buff,"O_ENABLE_COMMANDS : %s\nO_CLONE           : %s\nO_DESTRUCTED      : %s\nO_SWAPPED         : %s\nO_ONCE_INTERACTIVE: %s\nO_CREATED         : %s\n",
			flags&O_ENABLE_COMMANDS ?"TRUE":"FALSE",
			flags&O_CLONE           ?"TRUE":"FALSE",
			flags&O_DESTRUCTED      ?"TRUE":"FALSE",
			flags&O_SWAPPED          ?"TRUE":"FALSE",
			flags&O_ONCE_INTERACTIVE?"TRUE":"FALSE",
			flags&O_CREATED		?"TRUE":"FALSE");
	    
	    (void)sprintf(tdb,"time_of_ref : %d\n", ob->time_of_ref);
	    (void)strcat(db_buff, tdb);
	    (void)sprintf(tdb,"ref         : %d\n", ob->ref);
	    (void)strcat(db_buff, tdb);
#ifdef DEBUG
	    (void)sprintf(tdb,"extra_ref   : %d\n", ob->extra_ref);
	    (void)strcat(db_buff, tdb);
#endif
	    (void)sprintf(tdb,"swap_num    : %d\n", ob->swap_num);
	    (void)strcat(db_buff, tdb);
	    (void)sprintf(tdb,"name        : '%s'\n", ob->name);
	    (void)strcat(db_buff, tdb);
	    (void)sprintf(tdb,"next_all    : OBJ(%s)\n",
			ob->next_all?ob->next_all->name:"NULL");
	    (void)strcat(db_buff, tdb);
	    if (obj_list == ob) 
	    {
		(void)strcat(db_buff, "This object is the head of the object list.\n");
	    }

	    obj2 = obj_list;
	    i = 1;
	    do
		if (obj2->next_all == ob) 
		{
		    (void)sprintf(tdb,"Previous object in object list: OBJ(%s)\n",
			    obj2->name);
		    (void)strcat(db_buff, tdb);
		    (void)sprintf(tdb, "position in object list:%d\n",i);
		    (void)strcat(db_buff, tdb);
		    
		}
	    while (obj_list != (obj2 = obj2->next_all));
	}
        else if (argv[0].u.number == 1) 
        {
	    if (argv[1].type != T_OBJECT)
		break;
	    ob = argv[1].u.ob;
	    
	    (void)sprintf(db_buff,"program ref's %d\n", ob->prog->ref);
	    (void)sprintf(tdb,"Name %s\n", ob->prog->name);
	    (void)strcat(db_buff, tdb);
	    (void)sprintf(tdb,"program size %d\n", ob->prog->program_size);
	    (void)strcat(db_buff, tdb);
	    (void)sprintf(tdb, "num func's %u (%u) \n", ob->prog->num_functions
			,ob->prog->num_functions * (unsigned) sizeof(struct function));
	    (void)strcat(db_buff, tdb);
	    (void)sprintf(tdb,"sizeof rodata %d\n", ob->prog->rodata_size);
	    (void)strcat(db_buff, tdb);
	    (void)sprintf(tdb,"num vars %u (%u)\n", ob->prog->num_variables
			,ob->prog->num_variables * (unsigned) sizeof(struct variable));
	    (void)strcat(db_buff, tdb);
	    (void)sprintf(tdb,"num inherits %u (%u)\n", ob->prog->num_inherited
			,ob->prog->num_inherited * (unsigned) sizeof(struct inherit));
	    (void)strcat(db_buff, tdb);
	    (void)sprintf(tdb,"total size %d\n", ob->prog->total_size);
	    (void)strcat(db_buff, tdb);
	}
        else
	{
	    (void)sprintf(db_buff, "Bad number argument to object_info: %d\n",
		    argv[0].u.number);
        }
	retval.type = T_STRING;
	retval.string_type = STRING_MSTRING;
	retval.u.string = make_mstring(db_buff);
	return &retval;
    }
    case 18: /* opcdump,	18	    */
    {
#ifdef OPCPROF
	opcdump();
	retval = const1;
	return &retval;
#else
	retval.type = T_STRING;
	retval.string_type = STRING_CSTRING;
	retval.u.string = "Only valid if GD compiled with OPCPROF flag.\n";
	return &retval;
#endif
    }
    case 19: /* send_udp,	19     		host, port, msg */
    {
	int tmp;

	if (argc < 3 || 
	    argv[0].type != T_STRING ||
	    argv[1].type != T_NUMBER ||
	    argv[2].type != T_STRING)
	    break;
#ifdef CATCH_UDP_PORT
	tmp = udpsvc_send(argv[0].u.string, argv[1].u.number, argv[2].u.string);
	if (tmp)
	    retval = const1;
	else
#endif
	    retval = const0;
	return &retval;
    }
    case 20: /* mud_port,	20  */
    {
	extern int port_number;
	retval.type = T_NUMBER;
	retval.u.number = port_number;
	return &retval;
    }
    case 21: /* udp_port,	21  */
    {
#ifdef CATCH_UDP_PORT
	extern int udp_port;
	retval.u.number = udp_port;
#else
	retval.u.number = -1;
#endif
	retval.type = T_NUMBER;
	return &retval;
    }
    case 22: /* ob_flags,	22 ob  */
    {
	if (argc && (argv[0].type == T_OBJECT))
	{
	    retval.type = T_NUMBER;
	    retval.u.number = argv[0].u.ob->flags;
	    return &retval;
	}
	retval = const0;
	return &retval;
    }
    case 23: /* get_variables, 23       object NULL/string */
    {
	struct svalue get_variables(struct object *);
	struct svalue get_variable(struct object *, char *);
	
 	switch (argc)
 	{
 	case 1:
 	    if ( argv[0].type != T_OBJECT)
 	    {
 		retval = const0;
 		return &retval;
 	    }
 	    retval = get_variables(argv[0].u.ob);
 	    return &retval;
 	case 2:
 	    if ( argv[0].type != T_OBJECT || argv[1].type != T_STRING)
 	    {
 		retval = const0;
 		return &retval;
 	    }
 	    retval = get_variable(argv[0].u.ob, argv[1].u.string);
 	    return &retval;
 	case 3:
	    if ( argv[0].type == T_OBJECT && argv[1].type == T_STRING)
	    {
		retval = get_variable(argv[0].u.ob, argv[1].u.string);
		return &retval;
	    }
 	    if ( argv[0].type == T_OBJECT)
	    {
		retval = get_variables(argv[0].u.ob);
		return &retval;
	    }
	    retval = const0;
	    return &retval;
 	default:
 	    retval = const0;
 	    return &retval;
 	    
 	}
    }
    case 24: /* get_eval_cost,	24  */
    {
	extern int eval_cost;
	retval.type = T_NUMBER;
	retval.u.number = eval_cost;
	return &retval;
    }

    case 25: /* debug malloc, 25 */
    {
#ifdef DEBUG_MALLOC
        debug_malloc();
#endif
        retval = const1;
        return &retval;
    }    
    case 26: /* getprofile, 26	object */
    {
#ifndef PROFILE_FUNS
	retval.type = T_STRING;
	retval.string_type = STRING_CSTRING;
	retval.u.string = "Only valid if GD compiled with PROFILE_FUNS flag.\n";
	return &retval;
#else
	if (argc < 1 || argv[0].type != T_OBJECT)
	    break;
	retval.type = T_POINTER;
	retval.u.vec = allocate_array(argv[0].u.ob->prog->num_functions);
#ifdef USE_SWAP
	access_program(argv[0].u.ob->prog);
#endif
	for (il = 0; il < (int)argv[0].u.ob->prog->num_functions; il++)
	{
	    char buff[200]; /* I know longer funnames crashes the GD... */

	    (void)sprintf(buff, "%09d:%09ld: %s",
		    argv[0].u.ob->prog->functions[il].num_calls,
		    argv[0].u.ob->prog->functions[il].time_spent / 100,
		    argv[0].u.ob->prog->functions[il].name);
	    retval.u.vec->item[il].type = T_STRING;
	    retval.u.vec->item[il].string_type = STRING_MSTRING;
	    retval.u.vec->item[il].u.string = make_mstring(buff);
	}
#ifdef USE_SWAP
	access_program(current_prog);
#endif
	return &retval;
#endif
    }
    case 27: /* get_avg_response, 27 */
    {
	extern int get_msecs_response(int);
	extern int msr_point;
	int sum, num, tmp;

	if (msr_point >=0)
	{
	    sum = 0;
	    num = 0;
	    for (il = 0; il < 100; il++)
	    {
		if ((tmp = get_msecs_response(il)) >=0)
		{
		    sum += tmp;
		    num++;
		}
	    }
	    retval.type = T_NUMBER;
	    retval.u.number = (num > 0) ? sum / num : 0;
	    return &retval;
	}
	break;
    }
    case 28: /* destruct, 28 */
    case 29: /* destroy, 29 */
    {
	extern void destruct_object(struct object *);

	if (argc && argv[0].type == T_OBJECT &&
            !(argv[0].u.ob->flags & O_DESTRUCTED))
            destruct_object(argv[0].u.ob);
	break;
    }
    case 30: /* update snoops, 30 */
#ifdef SUPER_SNOOP
	update_snoop_file();
#else
	retval.type = T_STRING;
	retval.string_type = STRING_CSTRING;
	retval.u.string = "Only valid if GD compiled with SUPER_SNOOP flag.\n";
#endif
	break;
    case 31: /* dump objects */
    {
	FILE *ufile;
	char *mem_variables(struct object *);
	struct object *ob, *nob;
	char line[255];      
       
	if ((ufile = fopen(DUMP_FILE, "w")) == NULL)
	{
	    retval.type = T_NUMBER;
	    retval.u.number = -1;
	    return &retval;
	}
       
	ob = obj_list;
	do
	{
	    if (ob)
	    {
		extern int num_call_outs(struct object *);
		int num_co, num_inv;
		struct object *o;

		num_co = num_call_outs(ob);

		for (num_inv = 0, o = ob->contains; o;
		     num_inv++, o = o->next_inv) ;

		(void)sprintf(line, "%s %s in %s callouts %d inventory %d\n",
			mem_variables(ob), ob->name,
			ob->super ? ob->super->name : "(void)",
			num_co, num_inv);
	    }
	    if (fputs(line, ufile) < 0)
	    {
		(void)fclose(ufile);
		break;
	    }
	    nob = ob->next_all;
	}
	while (obj_list != (ob = nob));
	(void)fclose(ufile);
	break;
    }
    case 32: /* set_swap */
#ifdef USE_SWAP
	{
	    extern int max_swap_memory;
	    extern int min_swap_memory;
	    extern int min_swap_time;
	    extern int max_swap_time;
	    struct vector *v;
	    
	    if (!argc || argv[0].type != T_POINTER ||
		argv[0].u.vec->size != 4)
		break;
	    v = argv[0].u.vec;
	    if (v->item[0].type != T_NUMBER ||
		v->item[0].u.number < 0) 
		break; /* Too low min_swap_memory */
	    
	    if (v->item[1].type != T_NUMBER ||
		v->item[1].u.number < v->item[0].u.number)
		break;
	    
	    if (v->item[2].type != T_NUMBER ||
		v->item[2].u.number < 0)
		break;
	    
	    if (v->item[3].type != T_NUMBER ||
		v->item[3].u.number < v->item[2].u.number)
		break;
	    min_swap_memory = v->item[0].u.number;
	    max_swap_memory = v->item[1].u.number;
	    min_swap_time = v->item[2].u.number;
	    max_swap_time = v->item[3].u.number;
	    retval.type = T_NUMBER;
	    retval.u.number = 1;
	    return &retval;
	}
#else
	retval.type = T_STRING;
	retval.string_type = STRING_CSTRING;
	retval.u.string = "Only valid if GD compiled with USE_SWAP flag.\n";
	return &retval;
#endif
	
    case 33: /* query_swap */
#ifdef USE_SWAP
	{
	    if (argc == 0 || (argv[0].type == T_NUMBER && argv[0].u.number == 0))
	    {
		extern int max_swap_memory;
		extern int min_swap_memory;
		extern int min_swap_time;
		extern int max_swap_time;
		struct vector *v;
		
		v = allocate_array(4);
		v->item[0].type = T_NUMBER;
		v->item[0].u.number = min_swap_memory;
		v->item[1].type = T_NUMBER;
		v->item[1].u.number = max_swap_memory;
		v->item[2].type = T_NUMBER;
		v->item[2].u.number = min_swap_time;
		v->item[3].type = T_NUMBER;
		v->item[3].u.number = max_swap_time;
		
		retval.type = T_POINTER;
		retval.u.vec = v;
		return &retval;
	    }
	    else if (argv[0].type == T_NUMBER)
	    {
		extern int used_memory;
		extern int swap_out_obj_sec, swap_out_obj_min;
		extern int swap_out_prog_sec, swap_out_prog_min;
		extern int swap_in_obj_sec, swap_in_obj_min;
		extern int swap_in_prog_sec, swap_in_prog_min;
		extern int swap_out_obj_hour, swap_in_obj_hour;
		extern int swap_out_prog_hour, swap_in_prog_hour;
		extern struct object *swap_ob;
		extern struct program *swap_prog;

		switch(argv[0].u.number)
		{
		case 1:
		    {
			struct vector *v, *v0;
			
			v = allocate_array(4);
			
			v0 = allocate_array(3);
			v0->item[0].type = T_NUMBER;
			v0->item[0].u.number = swap_in_obj_sec;
			v0->item[1].type = T_NUMBER;
			v0->item[1].u.number = swap_in_obj_min;
			v0->item[2].type = T_NUMBER;
			v0->item[2].u.number = swap_in_obj_hour;
			v->item[0].type = T_POINTER;
			v->item[0].u.vec = v0;
			
			v0 = allocate_array(3);
			v0->item[0].type = T_NUMBER;
			v0->item[0].u.number = swap_in_prog_sec;
			v0->item[1].type = T_NUMBER;
			v0->item[1].u.number = swap_in_prog_min;
			v0->item[2].type = T_NUMBER;
			v0->item[2].u.number = swap_in_prog_hour;
			v->item[1].type = T_POINTER;
			v->item[1].u.vec = v0;
			
			v0 = allocate_array(3);
			v0->item[0].type = T_NUMBER;
			v0->item[0].u.number = swap_out_obj_sec;
			v0->item[1].type = T_NUMBER;
			v0->item[1].u.number = swap_out_obj_min;
			v0->item[2].type = T_NUMBER;
			v0->item[2].u.number = swap_out_obj_hour;
			v->item[2].type = T_POINTER;
			v->item[2].u.vec = v0;
			
			v0 = allocate_array(3);
			v0->item[0].type = T_NUMBER;
			v0->item[0].u.number = swap_out_prog_sec;
			v0->item[1].type = T_NUMBER;
			v0->item[1].u.number = swap_out_prog_min;
			v0->item[2].type = T_NUMBER;
			v0->item[2].u.number = swap_out_prog_hour;
			v->item[3].type = T_POINTER;
			v->item[3].u.vec = v0;
			
			retval.type = T_POINTER;
			retval.u.vec = v;
			return &retval;
		    }
		case 2:
		    retval.type = T_NUMBER;
		    retval.u.number =
			(swap_prog->time_of_ref < swap_ob->time_of_ref ?
			 swap_prog->time_of_ref :
			 swap_prog->time_of_ref);
		    return &retval;
		case 3:
		    retval.type = T_NUMBER;
		    retval.u.number = used_memory;
		    return &retval;
		}    
	    }
	    break;
	}		
#else
	retval.type = T_STRING;
	retval.string_type = STRING_CSTRING;
	retval.u.string = "Only valid if GD compiled with USE_SWAP flag.\n";
	return &retval;
#endif
#ifdef FUNCDEBUG
    case 34:
	dumpfuncs();
	retval = const0;
	return &retval;
#endif
    case 35: /* inhibitcallouts */
	if (argc && (argv[0].type == T_STRING))
	{
	    extern int inhibitcallouts;
	    int old;

	    old = inhibitcallouts;
	    if (strcmp(argv[0].u.string, "on") == 0)
		inhibitcallouts = 1;
	    else
		inhibitcallouts = 0;
	    retval.type = T_NUMBER;
	    retval.u.number = old;
	    return &retval;
	}
	else
	{
	    retval.type = T_NUMBER;
	    retval.u.number = -1;
	    return &retval;
	}
    case 36: /* warnobsolete */
	if (argc && (argv[0].type == T_STRING))
	{
	    extern int warnobsoleteflag;
	    int old;

	    old = warnobsoleteflag;
	    if (strcmp(argv[0].u.string, "on") == 0)
		warnobsoleteflag = 1;
	    else
		warnobsoleteflag = 0;
	    retval.type = T_NUMBER;
	    retval.u.number = old;
	    return &retval;
	}
	else
	{
	    retval.type = T_NUMBER;
	    retval.u.number = -1;
	    return &retval;
	}
    case 37: /* shared_strings */
#ifdef DEBUG
	dump_sstrings();
	retval = const0;
#else
	retval.type = T_STRING;
	retval.string_type = STRING_CSTRING;
	retval.u.string = "Only valid if GD compiled with DEBUG flag.\n";
#endif
	return &retval;
    }
    retval = const0;
    return &retval;
}

#if defined(RUSAGE) && defined(PROFILE_OBJS)
static struct vector *
make_cpu_array(int i, struct program *prog[])
{
    int num;
    struct vector *ret;
    char buff[1024]; /* should REALLY be enough */

    if (i <= 0) 
	return 0;
    ret = allocate_array(i);

    for(num = 0; num < i; num++) 
    {
	(void)sprintf(buff, "%8.8ld:%s", prog ? prog[num]->cpu : 0L, 
		prog ? prog[num]->name : "");
	free_svalue(&ret->item[num]);
	ret->item[num].type = T_STRING;
	ret->item[num].string_type = STRING_MSTRING;
	ret->item[num].u.string = make_mstring(buff);
    }
    return ret;
}
#endif

struct svalue 
get_variables(struct object *ob)
{
    int i, j;
    struct vector *names;
    struct vector *values;
    struct svalue res;
    int num_var;
    
    if (ob->flags & O_DESTRUCTED)
	return const0;

#ifdef USE_SWAP
    access_object(ob);
#endif

    if (!ob->variables)
	return const0;
    
    num_var = ob->prog->num_variables + ob->prog->inherit[ob->prog->num_inherited - 1]
	.variable_index_offset;

    names = allocate_array(num_var);
    values = allocate_array(num_var);
    
    for (j = ob->prog->num_inherited - 1; j >= 0; j--)
	if (!(ob->prog->inherit[j].type & TYPE_MOD_SECOND) &&
	    ob->prog->inherit[j].prog->num_variables > 0)
	{
#ifdef USE_SWAP
	    access_program(ob->prog->inherit[j].prog);
#endif
	    for (i = 0; i < (int)ob->prog->inherit[j].prog->num_variables; i++)
	    {
		if (num_var == 0)
		    error("Wrong number of variables in object.\n");
		names->item[--num_var].type = T_STRING;
		names->item[num_var].string_type = STRING_SSTRING;
		names->item[num_var].u.string =
		    reference_sstring(ob->prog->inherit[j].prog->
				      variable_names[i].name);
		assign_svalue_no_free(&values->item[num_var],
				      &ob->variables[ob->prog->inherit[j]
						     .variable_index_offset + i]);
	    }
	}
    res.type = T_MAPPING;
    res.u.map = make_mapping(names, values);
    free_vector(names);
    free_vector(values);
#ifdef USE_SWAP
    access_object(current_object);
    access_program(current_prog);
#endif
    return res;
}

struct svalue 
get_variable(struct object *ob, char *var_name)
{
    int i;
    struct svalue res = const0;
    
   if (ob->flags & O_DESTRUCTED)
	return res;

#ifdef USE_SWAP
    access_object(ob);
#endif
    
    if (!ob->variables)
	return const0;
    
    if ((i = find_status(ob->prog, var_name,0)) != -1)
	assign_svalue_no_free(&res, &ob->variables[i]);
#ifdef USE_SWAP
    access_object(current_object);
#endif
    return res;
}

/* Using globals is a bit unclean. I should return and tidy up later */
static int mapping_elem, array_elem, string_size, num_string;
static int num_map, num_arr, num_num, num_ob, num_float, num_func;
static int function_size;

void mem_mapping(struct mapping *);
void mem_array(struct vector *);

INLINE void
mem_incr(struct svalue *var)
{
    switch(var->type)
    {
    case T_FUNCTION:
	function_size += sizeof (struct closure);
	num_func++;
	if (var->u.func->funargs)
	    mem_array(var->u.func->funargs);
	break;
    case T_MAPPING:
	mapping_elem += var->u.map->size;
	num_map++;
	mem_mapping(var->u.map);
	break;
    case T_POINTER:
	array_elem += var->u.vec->size;
	num_arr++;
	mem_array(var->u.vec);
	break;
    case T_STRING:
	string_size += strlen(var->u.string);
	num_string++;
	break;
    case T_OBJECT:
	/* Check for destructed objects while we'r at it :) */
	if (var->u.ob->flags & O_DESTRUCTED)
	{
	    num_num++;
	    free_svalue(var);
	    break;
	}
	num_ob++;
	break;
    case T_FLOAT:
	num_float++;
	break;
    case T_NUMBER:
	num_num++;
	break;
    default:
	;
    }
}

void
mem_mapping(struct mapping *m)
{
    struct apair **p;
    int i;
    int size;

    if (m->size < 0)
	return;
    size = m->size;
    m->size = -1;
    for (i = 0 ; i < size ; i++)
    {
        for(p = &m->pairs[i]; *p; )
        {
	    /* Check for destructed objects while we'r at it :) */
	    if ((*p)->arg.type == T_OBJECT &&
		((*p)->arg.u.ob->flags & O_DESTRUCTED))
	    {
		struct apair *f;

		f = *p;
		*p = f->next;
		f->next = 0;
		m->card -= free_apairs(f);
		mapping_elem--;
	    }
	    else
	    {
		mem_incr(&(*p)->arg);
		mem_incr(&(*p)->val);
		p = &(*p)->next;
	    }
        }
    }
    m->size = size;
}

void
mem_array(struct vector *v)
{
    int i;
    int size;

    if (v->size < 0)
	return;
    size = v->size;
    v->size = -1;
    for (i = 0; i < size; i++)
	mem_incr(&v->item[i]);
    v->size = size;
}

char *
mem_variables(struct object *ob)
{
    int i;
    int num_var;
    static char buf[128];
#ifdef USE_SWAP
    int was_swapped;
    extern void swap_object(struct object *);
#endif

    if (ob->flags & O_DESTRUCTED)
	return "DESTRUCTED                          ";

    if (ob->flags & O_SWAPPED)
#ifdef USE_SWAP
    {
	load_ob_from_swap(ob);
	was_swapped = 1;
    }
    else
	was_swapped = 0;
#else
	return "SWAPPED                             ";
#endif

    if (!ob->variables)
	return "NOT AVAILABLE                       ";

    mapping_elem = 0;
    array_elem = 0;
    string_size = 0;
    num_string = 0;
    num_arr = 0;
    num_map = 0;
    num_ob = 0;
    num_num = 0;
    num_float = 0;

    num_var = ob->prog->num_variables +
	ob->prog->inherit[ob->prog->num_inherited - 1].variable_index_offset;

    for (i = -1; i < num_var; i++)
    {
	mem_incr(&ob->variables[i]);
    }
    (void)sprintf(buf, "A %6d %6d M %6d %6d S %6d %6d O %6d N %6d F %6d",
	    num_arr, array_elem, num_map, mapping_elem,
	    num_string, string_size, num_ob, num_num, num_float);
#ifdef USE_SWAP
    if (was_swapped)
	swap_object(ob);
#endif
    return buf;
}
