/*
   config.h
   
   Some configuration parameters
*/
#ifndef __lpc_config_h_
#define __lpc_config_h_

/*
 * Max size of a file allowed to be read by 'read_file()'.
 */
#define READ_FILE_MAX_SIZE	50000

/* Version of the game in the form xx.xx.xx (leading zeroes) gc.
 * Two digits will be appended, that is the patch level.
 */
#define GAME_VERSION "CD.04."

/*
 * Should we swap out objects to disk?
 */
#define USE_SWAP

/*
 * Should we use stdio to access the swap file?
 */
#define SWAP_STDIO

/*
 * Should we try to ask objects to remove themselves before swapping
 * them to disk?
 */
#undef DO_CLEANUP

/*
 * If you define this explode() will strip any leading delimiters.
 */
#undef OLD_EXPLODE

/*
 * Should the indent code in "ed" be enabled?
 */
#define	ED_INDENT

/*
 * Should we warn if the same file is included multiple times?
 */
#define	WARN_INCLUDES

/* If you define this add_actions to static functions can be called
 * with the efun command().
 */
#define STATIC_ADD_ACTIONS

/* If you define this add_action(,,1) will not require whitespace
 * between the verb and the arguments
 */
#define COMPAT_ADD_ACTIONS

/*
 * The resolution of a float argument to call_out
 */
#define TIME_RES 10

/*
 * The number of entries in the call out array.
 */
#define NUM_SLOTS 0x4000 /* MUST be a multiple of 2 */

/*
 * The maximum number of call outs per object
 */
#define	MAX_CALL_OUT	128

/*
 * How to extract an unsigned char from a char *.
 * If your compiler has the type 'unsigned char', then the cast-version
 * is best. If you do not know, use the simple version, and the game will
 * immediately terminate with a message if bad.
#define EXTRACT_UCHAR(p) (*p < 0 ? *p + 0x100 : *p)
 */
#define EXTRACT_UCHAR(p) (*(unsigned char *)(p))

/*
 * Define the maximum stack size of the stack machine. This stack will also
 * contain all local variables and arguments.
 */
#define EVALUATOR_STACK_SIZE	4000

/*
 * Define the maximum call depth for functions.
 */
#define MAX_TRACE		0x100

/*
 * Define the size of the compiler stack. This defines how complex
 * expressions the compiler can parse. The value should be big enough.
 */
#define COMPILER_STACK_SIZE	4000

/*
 * With this option defined you can trace mudlib execution from within the
 * mud. Do not keep it switched on constantly because it consumes quite a bit
 */
#undef TRACE_CODE

/*
 * Does the system have a getrusage call?
 */
#define RUSAGE

/*
 * Do we use a shadow binary directory
 *
 * If this is used then LPC files can declare: #pragma save_binary
 * This will save the compiled stackcode under the binary file which
 * is the filepath prepended with the path below.
 *
 * Also if this is defined a binary is always searched for first
 * before doing the compile.
 * 
 */
#define BINARIES "binaries/"

/*
 * How big, if we want it, should the global cache be?
 *
 * This cache, written by Amylaar, can increase the speed of calls
 * to functions. The cache is allocated globally and indexed with
 * a number hashed from the functionname / programname.
 *
 * NOTE!!!
 *    The size given be below must be an even power of 2 and _if_ defined
 *    must not be less than 2.
 */
#define GLOBAL_CACHE 0x4000

/* Enable extensive cache statistics */
#undef CACHE_STATS

/*
  If this is defined then the normal 'init' is not called in an object with
  this_player set to the encountering living object. Instead 'encounter' is
  is called in the living object, this_player is set to the living object.
  The normal encounter: encounter(object ob) { ob->init() }
*/
#define USE_ENCOUNTER_NOT_INIT

/*
 * Maximum number of bits in a bit field. They are stored in printable
 * strings, 6 bits per byte.
 */
#define MAX_BITS		4002 /* even 6 */

/*
 * There is a hash table for living objects, used by find_living().
 */
#define LIVING_HASH_SIZE	0x1000

/*
 * Define what port number the game is to use.
 */
#define PORTNO			2300

/*
 * Max number of local variables in a function.
 */
#define MAX_LOCAL	30

/*
 * The hashing function used for pointers to shared strings.
 * The >> shift is because pointers are often aligned to multiples of 4.
 */
#define PTR_HASH(ptr, size)\
    ( ( (((unsigned short *) &(ptr))[0] / sizeof(void *)) ^ \
        (((unsigned short *) &(ptr))[1] / sizeof(void *)) ) % (unsigned)(size) )

/* Maximum number of evaluated nodes/loop.
 * If this is exceeded, current function is halted.
 */
#define MAX_COST	500000

/*
 * EXTRA_COST
 *
 * Extra eval cost that will be added when doing a catch().
 * Note that in the event of recursive catch() calls, only
 * the top level catch() will get the extra limit applied.
 */
#define	EXTRA_COST	1000

/*
 * The maximum string length. That should be enough for everyone.
 */
#define MAX_STRING_LENGTH 60000

/*
 * Maximum length of inherit chain
 */
#define MAX_INHERIT	42

/*
 * Maximum number of nested includes
 */
#define	MAX_INCLUDE	10

/*
 * Where to swap out objects. This file is not used if NUM_RESET_TO_SWAP
 * is 0.
 */
#define SWAP_FILE		"LP_SWAP.4"

/*
 * This is the maximum array size allowed for one single array.
 */
#define MAX_ARRAY_SIZE 5000

/*
 * Maximum size of a mapping.
 */
#define MAX_MAPPING_SIZE 5000

/*
 * Some LPmuds on sun4 and sparcstations have had problems with the
 * call of inet_ntoa() in comm1.c.
 * If the game crash in query_ip_number() wen using inet_ntoa(),
 * then undefine the following symbol.
 * The query_ip_number() is called when doing the 'people' command
 * for example.
 * This should be defined on a NeXT
 */
#define INET_NTOA_OK

/*
 * Maximum number of players in the game.
 *
 * This is the absolute maximum, the mudlib will probably set a lower level. 
 */
#define MAX_PLAYERS	500

/*
 * When uploading files, we want fast response; however, normal players
 * shouldn't be able to hog the system in this way.  Define ALLOWED_ED_CMDS
 * to be the ratio of the no of ed cmds executed per player cmd, and
 * MAX_CMDS_PER_BEAT to be the bax no of buffered player commands the
 * system will accept in each heartbeat interval.
 */
#define	ALLOWED_ED_CMDS		20

/*
 * Reserve an extra memory area from malloc(), to free when we run out
 * of memory to get some warning and tell master about our memory troubles.
 * If this value is 0, no area will be reserved.
 */
#define RESERVED_SIZE		1000000
/*
#define RESERVED_SIZE		(1 << 17)
*/

/* Define the size of the shared string hash table.  This number should
 * a prime, probably between 1000 and 30000; if you set it to about 1/5
 * of the number of distinct strings you have, you will get a hit ratio
 * (number of comparisons to find a string) very close to 1, as found strings
 * are automatically moved to the head of a hash chain.  You will never
 * need more, and you will still get good results with a smaller table.
 * THIS IS NOT IMPLEMENTED YET.
 */
#define	HTABLE_SIZE	0x8000	/* there is a table of some primes too */

/*
 * Object hash table size.
 * Define this like you did with the strings; probably set to about 1/4 of
 * the number of objects in a game, as the distribution of accesses to
 * objects is somewhat more uniform than that of strings.
 */
#define OTABLE_SIZE	0x4000	/* we have several thousand obs usually */

/*
 * Define SYSV if you are running system V with a lower release level than
 * Sys V.4.
 */
#undef	SYSV

/*
 * Define FCHMOD_MISSING only if your system doesn't have fchmod().
 */
#undef FCHMOD_MISSING

/*
 * Define MAX_BYTE_TRANSFER to the number of bytes you allow to be read
 * and written with read_bytes and write_bytes
 */
#define MAX_BYTE_TRANSFER 10000

/*
 * CATCH_UDP_PORT
 *
 * Define this if the mud are to catch incoming udp messages on a
 * specific port. If == -1 it will not be used unless the mud is started
 * with the -u### flag. Where ### is the portnumber for the udp port.
 * If undefined the -u flag will be ignored.
 */
#define CATCH_UDP_PORT	2500

/*
 * UDP_SEND_HOSTNAME
 *
 * Define this if you want udp_send() to be able to send UDP datagrams
 * to both addresses hostnames.  When disabled, only IP addresses are
 * supported.  When enabled, both addresses and hostnames are supports.
 * Defining this may add additional lag to your MUD.
 */
#undef UDP_SEND_HOSTNAME

/*
 * SERVICE_PORT
 *
 * Define this if the mud are to answer service requests from other
 * programs. If == -1 it will not be used unless the mud is started
 * with the -p### flag. Where ### is the portnumber for the service
 * port. If undefined the -p flag will be ignored.
 */
#define SERVICE_PORT	3003

/*
 * ALLOWED_SERVICE
 *
 * Define this to the IP address of the machine that should be allowed
 * to connect to the service port.  Note that the local host can always
 * connect via the loopback address (127.0.0.1).  The address must be
 * specified in hexadecimal.
 */
/*
#define ALLOWED_SERVICE 0x8110E3CB
*/

/*
 * OPCPROF
 *
 * Define this to have the driver count calls to efuns
 */
#undef OPCPROF

/*
 * PROFILE_OBJS
 *
 * With this defined statistics of all objects are kept.
 * Only keeps time spent in them.
 *
 * The information is accessible through debug("top_ten_cpu") and
 * debug("object_cpu", object).
 *
 * This costs memory and cpu, only do it for mudlib profiling purposes.
 */
#undef PROFILE_OBJS

/*
 * PROFILE_FUNS
 *
 * With this defined statistics of all functions in all programs are kept.
 * Number of calls to them and time spent in them.
 *
 * The information is accessible through debug("getprofile", object)
 *
 * This costs memory and cpu, only do it for mudlib profiling purposes.
 */
#undef PROFILE_FUNS

/*
 * SUPER_SNOOP
 *
 * With this defined, you can add someone to a file called
 * snoop/snooped which will then be snooped in the
 * gamedriver.
 *
 * Rereading the file is done by calling debug("update snoops");
 */
#undef SUPER_SNOOP

/************************************************************************/
/*	END OF CONFIG -- DO NOT ALTER ANYTHING BELOW THIS LINE		*/
/************************************************************************/

/*
 * some generic large primes used by various hash functions in different files
 * You can alter these if you know of a better set of numbers!  Be sure
 * they are primes...
 */

#define	P1		701	/* 3 large, different primes */
#define	P2		14009	/* There's a file of them here somewhere :-) */
#define	P3		54001

/* Calculate the number of set bits in the argument n 
   (note the octal numbers) */
#define BITNUM(n) (( \
         ((n)&010101010101)+\
        (((n)&020202020202)>>1)+\
        (((n)&000404040404)>>2)+\
        (((n)&001010101010)>>3)+\
        (((n)&002020202020)>>4)+\
        (((n)&004040404040)>>5)\
) % 63)

#endif

