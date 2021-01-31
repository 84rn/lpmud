
#include <stdio.h>
/*
	alloca -- (mostly) portable public-domain implementation -- D A Gwyn

	last edit:	86/05/30	rms
	   include config.h, since on VMS it renames some symbols.
	   Use xmalloc instead of malloc.

	This implementation of the PWB library alloca() function,
	which is used to allocate space off the run-time stack so
	that it is automatically reclaimed upon procedure exit, 
	was inspired by discussions with J. Q. Johnson of Cornell.

	It should work under any C implementation that uses an
	actual procedure stack (as opposed to a linked list of
	frames).  There are some preprocessor constants that can
	be defined when compiling for your specific system, for
	improved efficiency; however, the defaults should be okay.

	The general concept of this implementation is to keep
	track of all alloca()-allocated blocks, and reclaim any
	that are found to be deeper in the stack than the current
	invocation.  This heuristic does not reclaim storage as
	soon as it becomes invalid, but it will do so eventually.

	As a special case, alloca(0) reclaims storage without
	allocating any.  It is a good idea to use alloca(0) in
	your main control loop, etc. to force garbage collection.
*/
#ifndef lint
static char	SCCSid[] = "@(#)alloca.c	1.1";	/* for the "what" utility */
#endif

#define	NULL	0			/* null pointer constant */

extern void free();
extern void *malloc();

/*
	Define STACK_DIRECTION if you know the direction of stack
	growth for your system; otherwise it will be automatically
	deduced at run-time.

	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown
*/

#ifndef STACK_DIRECTION
#define	STACK_DIRECTION	0		/* direction unknown */
#endif

#if STACK_DIRECTION != 0

#define	STACK_DIR	STACK_DIRECTION	/* known at compile-time */

#else	/* STACK_DIRECTION == 0; need run-time code */

static int	stack_dir;		/* 1 or -1 once known */
#define	STACK_DIR	stack_dir

static void
find_stack_direction (/* void */)
{
    static char	*addr = NULL;	/* address of first
				   dummy, once known */
    auto char	dummy;		/* to get stack address */

    if (addr == NULL)
    {				/* initial entry */
	addr = &dummy;
	find_stack_direction ();	/* recurse once */
    }
    else				/* second entry */
	if (&dummy > addr)
	    stack_dir = 1;		/* stack grew upward */
	else
	    stack_dir = -1;		/* stack grew downward */
}

#endif	/* STACK_DIRECTION == 0 */

/*
	An "alloca header" is used to:
	(a) chain together all alloca()ed blocks;
	(b) keep track of stack depth.

	It is very important that sizeof(header) agree with malloc()
	alignment chunk size.  The following default should work okay.
*/

#ifndef	ALIGN_SIZE
#define	ALIGN_SIZE	sizeof(double)
#endif

typedef union hdr
{
    char	align[ALIGN_SIZE];	/* to force sizeof(header) */
    struct
    {
	union hdr *next;		/* for chaining headers */
	char *deep;		/* for stack depth measure */
    } h;
} header;

/*
	alloca( size ) returns a pointer to at least `size' bytes of
	storage which will be automatically reclaimed upon exit from
	the procedure that called alloca().  Originally, this space
	was supposed to be taken from the current stack frame of the
	caller, but that method cannot be made to work for some
	implementations of C, for example under Gould's UTX/32.
*/

static header *last_alloca_header = NULL; /* -> last alloca header */

void *
alloca (unsigned size)			/* returns pointer to storage */
                                        /* # bytes to allocate */
{
    auto char	probe;		/* probes stack depth: */
    register char	*depth = &probe;
    
#if STACK_DIRECTION == 0
    if (STACK_DIR == 0)		/* unknown growth direction */
	find_stack_direction ();
#endif

    /* Reclaim garbage, defined as all alloca()ed storage that
	was allocated from deeper in the stack than currently. */

    {
	register header	*hp;	/* traverses linked list */

	for (hp = last_alloca_header; hp != NULL;)
	    if (STACK_DIR > 0 && hp->h.deep > depth
		|| STACK_DIR < 0 && hp->h.deep < depth)
	    {
		register header	*np = hp->h.next;

		free ((void *) hp);	/* collect garbage */

		hp = np;		/* -> next header */
	    }
	    else
		break;			/* rest are not deeper */

	last_alloca_header = hp;	/* -> last valid storage */
    }

    if (size == 0)
	return NULL;		/* no allocation required */

    /* Allocate combined header + user data storage. */

    {
	register void *new = malloc (sizeof (header) + size);
	/* address of header */
	if (!new) { (void)fprintf(stderr,"alloca: failed!"); return NULL; }

	((header *)new)->h.next = last_alloca_header;
	((header *)new)->h.deep = depth;

	last_alloca_header = (header *)new;

	/* User storage begins just after header. */

	return (void *)((char *)new + sizeof(header));
    }
}
