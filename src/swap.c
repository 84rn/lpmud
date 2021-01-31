/* (c) Copyright by Anders Chrigstroem 1993, All rights reserved */
/* Permission is granted to use this source code and any executables
 * created from this source code as part of the CD Gamedriver as long
 * as it is not used in any way whatsoever for monetary gain. */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"

int total_num_prog_blocks = 0;
int total_prog_block_size = 0;
int total_program_size = 0;
int tot_alloc_variable_size = 0;
int used_memory;

#ifdef USE_SWAP
#ifdef DO_CLEANUP
int total_clean_ups = 0;
int clean_up_return0 = 0;
int clean_up_return1 = 0;
int clean_up_destruct = 0;
#endif

#include "lint.h"
#include "mstring.h"
#include "interpret.h"
#include "object.h"
#include "exec.h"
#include "mudstat.h"
#include "mapping.h"
#include "main.h"
#include "simulate.h"

#include "inline_svalue.h"

#if defined(sun) || defined(__osf__)
#include <alloca.h>
#endif

#ifndef SEEK_SET
#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2
#endif

#ifndef NBBY
#define NBBY		8			/* bits per byte */
#endif

#ifndef NBULONG
#define	NBULONG		(NBBY * sizeof(u_long))	/* bits per u_long */
#endif

#define BUFFER_DATA_SIZE 256
#define BUFFER_PTR_SIZE (BUFFER_DATA_SIZE / sizeof(unsigned int))

struct buffer
{
    u_int	bufnum;
    u_short	offset;
    union
    {
	char	data[BUFFER_DATA_SIZE];
	u_int	addr[BUFFER_PTR_SIZE];
    } u;
};

#define	SWAPIO_UNDEFINED	0
#define	SWAPIO_READ		1
#define	SWAPIO_WRITE		2

#ifdef SWAP_STDIO
static FILE *swap_fp;
static long swap_pos = -1;
static int swap_io = SWAPIO_UNDEFINED;
#else
static int swap_fd;
static off_t swap_off = -1;
#endif
static char swap_file[80];
static u_long *swap_bitmap;
u_int swap_cursize;
u_int swap_minfree;
u_int swap_maxfree;
#ifdef DEBUG
u_int swap_maxalloc;
#endif

int last_address = 0;
int total_bytes_swapped = 0;
int total_swap_chains = 0;
int total_free_blocks = 0;

#define ffs _ffs

#if defined(__i386__) && defined(__GNUC__)
static __inline__ int
ffs(u_long input)
{
    int result;

    __asm("bsfl %1, %0" : "=r" (result) : "r" (input));
    return result;
}
#else
static
#ifdef __GNUC__
__inline__
#endif
int
ffs(register u_long mask)
{
    register int bit;

    for (bit = 0; !(mask & 1); bit++)
	mask >>= 1;
    return(bit);
}
#endif

void
init_swap(void)
{
    char *p, hn[80];
    static int initialized = 0;

    if (initialized)
	return;

    initialized = 1;

    (void)gethostname(hn, sizeof(hn));
    if ((p = strchr(hn, '.')) != NULL)
	*p = '\0';

    (void)sprintf(swap_file, "%s.%s.%d", SWAP_FILE, hn, (int)getpid());

#ifdef SWAP_STDIO
    swap_fp = fopen(swap_file, "w+");
    if (swap_fp == NULL)
	fatal("Can't create swapfile.\n");
#else
    swap_fd = open(swap_file, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (swap_fd == -1)
	fatal("Can't create swapfile.\n");
#endif

    swap_cursize = getpagesize();

    swap_bitmap = xalloc(swap_cursize);
    (void)memset(swap_bitmap, 0xFF, swap_cursize);
    swap_bitmap[0] &= ~1;	/* XXX */

    swap_minfree = 0;
    swap_maxfree = swap_cursize * NBBY - 1;
#ifdef DEBUG
    swap_maxalloc = 0;
#endif
}

/*
 * Extend the swap allocation bitmap.
 */
static
void
extend_swap_bitmap(void)
{
    u_int size;
    u_char *cp;

    size = swap_cursize;
    swap_cursize <<= 1;

    cp = xalloc(swap_cursize);

    (void)memcpy(cp, swap_bitmap, size);
    (void)memset(cp + size, 0xFF, size);

    free(swap_bitmap);
    swap_bitmap = (u_long *)cp;

    swap_minfree = size * NBBY;
    swap_maxfree = swap_cursize * NBBY - 1;
}

/*
 * Allocate a block in the swap file.
 */
static
u_int
alloc_swap(void)
{
    u_int minb;
    u_int maxb;
    u_int b, c;
    u_long l, *lp;

#ifdef DEBUG
    if (swap_maxfree != swap_cursize * NBBY - 1)
	fatal("alloc_swap: swap_maxfree was stomped upon, is %u, should be %u\n",
	      swap_maxfree, swap_cursize * NBBY - 1);
#endif

    for (;;)
    {
	minb = swap_minfree / NBULONG;
	maxb = swap_maxfree / NBULONG;

	lp = &swap_bitmap[minb];

	for (b = minb; b <= maxb; b++)
	{
	    if ((l = *lp) != 0)
	    {
		c = ffs(l);
		*lp &= ~(1L << c);
		b = (b * NBULONG) + c;
		swap_minfree = b + 1;
		if (last_address < swap_minfree * BUFFER_DATA_SIZE)
		    last_address = swap_minfree * BUFFER_DATA_SIZE;
		if (total_free_blocks > 0)
		    total_free_blocks--;
#ifdef DEBUG
		if (swap_maxalloc < b)
		    swap_maxalloc = b;
#endif
		return b;
	    }
	    lp++;
	}

	extend_swap_bitmap();
    }
}

/*
 * Free a block in the swap file.
 */
static
void
free_swap(u_int blkno)
{
#ifdef DEBUG
    if (swap_maxfree != swap_cursize * NBBY - 1)
	fatal("alloc_swap: swap_maxfree was stomped upon, is %u, should be %u\n",
	      swap_maxfree, swap_cursize * NBBY - 1);
    if (blkno > swap_maxalloc)
	fatal("free_swap: freeing block %u past end of allocated blocks %u\n",
	      blkno, swap_maxalloc);
    if (swap_bitmap[blkno / NBULONG] & (1L << (blkno % NBULONG)))
	fatal("free_swap: block %u already free\n", blkno);
#endif

    swap_bitmap[blkno / NBULONG] |= 1L << (blkno % NBULONG);

    if (swap_minfree > blkno)
	swap_minfree = blkno;

    total_free_blocks++;
}

/*
 * Read a disk block.
 */
#ifdef SWAP_STDIO
static void
bread(u_int blkno, struct buffer *bp)
{
    long pos;

    pos = (long)blkno * BUFFER_DATA_SIZE;

    if (swap_pos != pos || swap_io != SWAPIO_READ) {
	if (fflush(swap_fp) == EOF)
	    fatal("bread: cannot fflush swap\n");
	if (fseek(swap_fp, pos, SEEK_SET) == -1)
	    fatal("bread: cannot seek to block %d: %s\n",
		blkno, strerror(ferror(swap_fp)));
    }

    if (fread(bp->u.data, BUFFER_DATA_SIZE, 1, swap_fp) != 1)
	fatal("bread: cannot read block %d: %s\n",
	    blkno, strerror(ferror(swap_fp)));

    swap_pos = pos + BUFFER_DATA_SIZE;
    swap_io = SWAPIO_READ;
}
#else
static void
bread(u_int blkno, struct buffer *bp)
{
    off_t off;
    char *buf;
    int cc, len;

    off = (off_t)blkno * BUFFER_DATA_SIZE;

    if (swap_off != off) {
	if (lseek(swap_fd, off, SEEK_SET) == (off_t)-1)
	    fatal("bread: cannot seek to block %d: %s\n",
		blkno, strerror(errno));
    }

    buf = bp->u.data;
    len = BUFFER_DATA_SIZE;

    while (len > 0)
    {
	cc = read(swap_fd, buf, len);
	if (cc == -1)
	    fatal("bread: cannot read block %d: %s\n",
		  blkno, strerror(errno));
	if (cc == 0)
	    fatal("bread: cannot read block %d: EOF\n", blkno);
	buf += cc;
	len -= cc;
    }

    swap_off = off + BUFFER_DATA_SIZE;
}
#endif

/*
 * Write a disk block.
 */
#ifdef SWAP_STDIO
static void
bwrite(u_int blkno, struct buffer *bp)
{
    long pos;

    pos = (long)blkno * BUFFER_DATA_SIZE;

    if (swap_pos != pos || swap_io != SWAPIO_WRITE) {
	if (fflush(swap_fp) == EOF)
	    fatal("bread: cannot fflush swap\n");
	if (fseek(swap_fp, pos, SEEK_SET) == -1)
	    fatal("bwrite: cannot seek to block %d: %s\n",
		blkno, strerror(ferror(swap_fp)));
    }

    if (fwrite(bp->u.data, BUFFER_DATA_SIZE, 1, swap_fp) != 1)
	fatal("bwrite: cannot write block %d: %s\n",
	    blkno, strerror(ferror(swap_fp)));

    swap_pos = pos + BUFFER_DATA_SIZE;
    swap_io = SWAPIO_WRITE;
}
#else
static void
bwrite(u_int blkno, struct buffer *bp)
{
    off_t off;
    char *buf;
    int cc, len;

    off = (off_t)blkno * BUFFER_DATA_SIZE;

    if (swap_off != off) {
	if (lseek(swap_fd, off, SEEK_SET) == (off_t)-1)
	    fatal("bwrite: cannot seek to block %d: %s\n",
		blkno, strerror(errno));
    }

    buf = bp->u.data;
    len = BUFFER_DATA_SIZE;

    while (len > 0)
    {
	cc = write(swap_fd, buf, len);
	if (cc == -1)
	    fatal("bwrite: cannot write block %d: %s\n",
		  blkno, strerror(errno));
	buf += cc;
	len -= cc;
    }

    swap_off = off + BUFFER_DATA_SIZE;
}
#endif

void
unlink_swap_file()
{
    (void)unlink(swap_file);
}

#ifdef DEALLOCATE_MEMORY_AT_SHUTDOWN
void
close_swap_file()
{
    free(swap_bitmap);
#ifdef SWAP_STDIO
    (void)fclose(swap_fp);
#else
    (void)close(swap_fd);
#endif
}
#endif

static struct buffer *
start_read(u_int blkno)
{
    struct buffer *bp;

    bp = (struct buffer *)xalloc(sizeof(struct buffer));
    bp->bufnum = blkno;
    bp->offset = sizeof(u_int);

    if (blkno == 0)
	fatal("start_read: bad chain block number.\n");

    bread(blkno, bp);

    return bp;
}

static void
swap_read(struct buffer *bp, char *buf, u_int len)
{
    size_t cc;

    total_bytes_swapped -= len;

    while (len > 0)
    {
	cc = BUFFER_DATA_SIZE - bp->offset;
	if (cc == 0)
	{
	    free_swap(bp->bufnum);
	    bp->bufnum = bp->u.addr[0];
	    bp->offset = sizeof(u_int);
	    if (bp->bufnum == 0)
		fatal("swap_read: block chain overrun.\n");
	    bread(bp->bufnum, bp);
	    cc = BUFFER_DATA_SIZE - bp->offset;
	}
	if (cc > len)
	    cc = len;
	(void)memcpy(buf, &bp->u.data[bp->offset], cc);
	buf += cc;
	len -= cc;
	bp->offset += cc;
    }
}

static void
end_read(struct buffer *bp)
{
    free_swap(bp->bufnum);

    if (bp->u.addr[0] != 0)
	fatal("end_read: swap chain underrun.\n");

    free(bp);

    total_swap_chains--;
}

static struct buffer *
start_write(u_int *blkno)
{
    struct buffer *bp;

    if (*blkno != 0)
	fatal("start_write: blkno = %d.\n", *blkno);

    *blkno = alloc_swap();

    bp = (struct buffer *)xalloc(sizeof(struct buffer));
    bp->bufnum = *blkno;
    bp->offset = sizeof(u_int);

    (void)memset(bp->u.data, '\0', BUFFER_DATA_SIZE);

    total_swap_chains++;

    return bp;
}

static void
swap_write(struct buffer *bp, char *buf, u_int len)
{
    size_t cc;

    total_bytes_swapped += len;

    while (len > 0)
    {
	cc = BUFFER_DATA_SIZE - bp->offset;
	if (cc == 0)
	{
	    if (bp->u.addr[0] != 0)
		fatal("swap_write: blkno = %d.\n", bp->u.addr[0]);
	    bp->u.addr[0] = alloc_swap();
	    bwrite(bp->bufnum, bp);
	    bp->bufnum = bp->u.addr[0];
	    bp->offset = sizeof(u_int);
	    (void)memset(bp->u.data, '\0', BUFFER_DATA_SIZE);
	    cc = BUFFER_DATA_SIZE - bp->offset;
	}
	if (cc > len)
	    cc = len;
	(void)memcpy(&bp->u.data[bp->offset], buf, cc);
	buf += cc;
	len -= cc;
	bp->offset += cc;
    }
}

static void
end_write(struct buffer *bp)
{
    if (bp->u.addr[0] != 0)
	fatal("end_write: blkno = %d.\n", bp->u.addr[0]);

    bwrite(bp->bufnum, bp);

    free(bp);
}

#define align(x) ( ((x) + (sizeof(double)-1) )  &  ~(sizeof(double)-1) )

struct object *swap_ob = 0;
struct program *swap_prog = 0;
int max_swap_memory = 0x40000001;
int min_swap_memory = 0x40000000;
int min_swap_time =   0x40000000;
int max_swap_time =   0x40000001;

extern struct program *prog_list;
extern struct object *obj_list;
extern int current_time;
extern int tot_alloc_object_size;
int total_lineno_swapped = 0;
int obj_swapped = 0;
int obj_bytes_swapped = 0;

int program_bytes_swapped = 0, programs_swapped = 0;

int swap_out_prog, swap_out_obj;
int swap_in_prog, swap_in_obj;

extern int d_flag;

int num_swapped;
int num_swapped_arrays, size_swapped_arrays;
int num_swapped_mappings, size_swapped_mappings;
int num_strings_swapped, size_strings_swapped;
int num_swapped_closures, size_swapped_closures;

static void
swap_svalue(struct buffer *buf, struct svalue *arg)
{
    switch(arg->type)
    {
    case T_OBJECT:
	if (arg->u.ob->flags & O_DESTRUCTED) {
	    /*
	     * Free object and clear it all out; swap a nil object
	     */
	    free_svalue(arg);
	}
	/* FALLTHROUGH */
    case T_NUMBER:
    case T_FLOAT:
	arg->type |= T_LVALUE;
	swap_write(buf, (char *)arg, sizeof(struct svalue));
	*arg = const0;
	break;
    case T_STRING:
	{
	    size_t len;
	    char *str;
	    
	    str = arg->u.string;
	    len = arg->u.number = strlen(str) + 1;
	    swap_write(buf, (char *)arg, sizeof(struct svalue));
	    swap_write(buf, str, len);
	    num_strings_swapped++;
	    size_strings_swapped += len;
	    switch(arg->string_type)
	    {
	    case STRING_MSTRING:
		free_mstring(str);
		break;
	    case STRING_SSTRING:
		free_sstring(str);
		break;
	    case STRING_CSTRING:
		break;
	    default:
#if 1
		(void)printf("SWAP ERROR buf=%lx\n", (unsigned long) buf);
		fatal("Invalid variable value (1).\n");
#else
		debug_message("SWAP FAILED: Invalid variable value (1) %d %d.\n", arg->type, arg->string_type);
		break;
#endif
	    }
	    *arg = const0;
	    break;
	}
    case T_POINTER:
	{
	    struct vector *v;

	    v = arg->u.vec;
	    if (v->ref != 1)
	    {
		arg->type |= T_LVALUE;
		swap_write(buf, (char *)arg, sizeof(struct svalue));
		*arg = const0;
	    }    
	    else
	    {
		int i;
		
		arg->u.number = v->size;
		swap_write(buf, (char *)arg, sizeof(struct svalue));
		for (i = 0; i < v->size; i++)
		    swap_svalue(buf, &v->item[i]);
		*arg = const0;
		num_swapped_arrays++;
		size_swapped_arrays += sizeof(struct vector) +
		    sizeof(struct svalue) * v->size - 1;
		free_vector(v);
	    }
	    break;
	}
    case T_MAPPING:
	{
	    struct mapping *m;
	    struct apair *pair;
	    int i, j;
	    
	    m = arg->u.map;
	    if (m->ref != 1)
	    {
		arg->type |= T_LVALUE;
		swap_write(buf, (char *)arg, sizeof(struct svalue));
		*arg = const0;
	    }    
	    else
	    {
		int size;
		
		size = arg->u.number = m->card;
		arg->string_type = m->size;
		
		swap_write(buf, (char *)arg, sizeof(struct svalue));

		for (i = j = 0; i < m->size; i++)
		    for(pair = m->pairs[i]; pair; pair = pair->next)
		    {
			swap_svalue(buf, &pair->arg);
			swap_svalue(buf, &pair->val);
			j++;
		    }
		if (j != size)
		    fatal("Wrong cardinality of mapping (%d != %d).\n", j, size);
		
		num_swapped_mappings++;
		size_swapped_mappings += sizeof(struct mapping) +
		    m->card * sizeof(struct apair) +
			m->size * sizeof(struct apair *);
		
		free_mapping(m);
		*arg = const0;
	    }
	    break;
	}
    case T_FUNCTION:
	{
#if 0		/* Swapping closures currently doesn't work all that well */
	    struct closure *f;

	    f = arg->u.func;
(void)printf("swap out closure(%lx) ref=%d %s\n", (unsigned long)f, f->ref, show_closure(f));
	    if (f->ref != 1) {
#endif
		arg->type |= T_LVALUE;
		swap_write(buf, (char *)arg, sizeof(struct svalue));
		*arg = const0;
#if 0		/* Swapping closures currently doesn't work all that well */
	    } else {
		struct svalue theobj, theargs, theno;

		arg->u.number = f->funtype;
		swap_write(buf, (char *)arg, sizeof(struct svalue));
		theno.type = T_NUMBER;
		theno.u.number = (f->funno << 16) | f->funinh;
		swap_svalue(buf, &theno);
		if (f->funtype == FUN_EFUN || f->funobj == 0) {
		    theobj = const0;
		} else {
		    theobj.type = T_OBJECT;
		    theobj.u.ob = f->funobj;
		}
		theargs.type = T_POINTER;
		theargs.u.vec = f->funargs;
		swap_svalue(buf, &theobj);
		swap_svalue(buf, &theargs);
		free_closure(f);
		*arg = const0;
		num_swapped_closures++;
		size_swapped_closures += sizeof(struct closure);
	    }
#endif
	}
	break;
    default:
	(void)printf("SWAP ERROR buf=%lx\n", (unsigned long) buf);
	fatal("Invalid variable type (2).\n");
    }	    
}

static void
unswap_svalue(struct buffer *buf, struct svalue *arg)
{
    swap_read(buf, (char *)arg, sizeof(struct svalue));
    if (arg->type & T_LVALUE)
    {
	arg->type &= ~T_LVALUE;
	if (arg->type == T_OBJECT && (arg->u.ob->flags & O_DESTRUCTED))
	    free_svalue(arg);
	if (!(arg->type &
	      (T_OBJECT | T_NUMBER | T_FLOAT | T_POINTER | T_MAPPING | T_FUNCTION)))
	{
	    (void)printf("SWAP ERROR buf=%lx\n", (unsigned long) buf);
	    fatal("Invalid variable lvalue (3).\n");
	}
	return;
    }
    switch(arg->type)
    {
    case T_STRING:
	{
	    size_t len;
	    char *str;
	    
	    len = arg->u.number;
	    switch (arg->string_type) {
		case STRING_SSTRING:
		case STRING_CSTRING:
		    str = alloca(len);
		    swap_read(buf, str, len);
		    arg->string_type = STRING_SSTRING;
		    arg->u.string = make_sstring(str);
		    break;
		case STRING_MSTRING:
		    arg->string_type = STRING_MSTRING;
		    arg->u.string = allocate_mstring(len - 1);
		    swap_read(buf, arg->u.string, len);
		    break;
		default:
		    debug_message("SWAP FAILED: Invalid variable value (5) %d %d.\n",
				  arg->type, arg->string_type);
	    }
	    num_strings_swapped--;
	    size_strings_swapped -= len;
	    break;
	}
    case T_POINTER:
	{
	    int size, i;
	    struct vector *v;
	    
	    size = arg->u.number;
	    v = allocate_array(size);
	    for (i = 0; i < size; i++)
		unswap_svalue(buf, &v->item[i]);
	    
	    arg->u.vec = v;
	    num_swapped_arrays--;
	    size_swapped_arrays -= sizeof(struct vector) +
		sizeof(struct svalue) * size - 1;
	    break;
	}
    case T_MAPPING:
	{
	    int size, i;
	    struct svalue marg, *mval;
	    
	    size = arg->u.number;
	    arg->u.map = allocate_map(size);
	    for (i = 0; i < size; i++)
	    {
		unswap_svalue(buf, &marg);
		mval = get_map_lvalue(arg->u.map, &marg, 1);
		unswap_svalue(buf, mval);
		free_svalue(&marg);
	    }
	    
	    num_swapped_mappings--;
	    size_swapped_mappings -= sizeof(struct mapping) +
		size * sizeof(struct apair) +
		    arg->string_type * sizeof(struct apair *);
	    break;
	}
#if 0
    case T_FUNCTION:
	{
	    struct svalue theobj, theargs, theno;
	    int ftype = arg->u.number;
	    struct closure *f = arg->u.func = alloc_closure(ftype);
	    
(void)printf("swap in closure\n");
	    unswap_svalue(buf, &theno);
	    f->funno = (unsigned)theno.u.number >> 16;
	    f->funinh = theno.u.number & 0xffff;
	    unswap_svalue(buf, &theobj);
	    unswap_svalue(buf, &theargs);
	    if (theobj.type != T_OBJECT)
		f->funobj = 0;
	    else
		f->funobj = theobj.u.ob;
	    f->funargs = theargs.u.vec;
	    num_swapped_closures--;
	    size_swapped_closures -= sizeof(struct closure);
	}
	break;
#endif
    default:
#if 1
	(void)printf("SWAP ERROR buf=%lx\n", (unsigned long) buf);
	fatal("Invalid variable value (4).\n");
#else
	debug_message("SWAP FAILED: Invalid variable value (2) %d.\n", arg->type);
	break;
#endif
    }
}

/*
 * Swap out an object. Only the program is swapped, not the struct object.
 *
 */
void 
swap_object(struct object *ob)
{
    int size;
    int num_vars, i;
    struct buffer *buf;

    if (ob->flags & (O_DESTRUCTED | O_SWAPPED))
	return;
    
    if (d_flag & DEBUG_SWAP)
    {
	(void)fprintf(stderr, "Swap object %s (ref %d) from 0x%lx\n",
		      ob->name, ob->ref, (unsigned long) ob->variables);
    }
    if (!ob->variables)
	return;
    
    ob->variables--;
    num_vars = ob->prog->inherit[ob->prog->num_inherited - 1].
	variable_index_offset +	ob->prog->num_variables + 1;
    size = num_vars * sizeof(struct svalue);
    
    buf = start_write((u_int *)&ob->swap_num);

    for (i = 0; i < num_vars; i++)
	swap_svalue(buf, &ob->variables[i]);
    end_write(buf);

    free((char *)ob->variables);
    ob->variables = NULL;
    obj_bytes_swapped += size;
    obj_swapped++;
    tot_alloc_variable_size -= size;

    num_swapped++;
    swap_out_obj++;
    ob->flags |= O_SWAPPED;
}

void
load_ob_from_swap(struct object *ob)
{
    int i;
    size_t size;
    int num_var;
    struct buffer *buf;

    if (!(ob->flags & O_SWAPPED))
	fatal("Swapping in not swapped out object!\n");

    
    if (d_flag & DEBUG_SWAP)
	{
	    (void)fprintf(stderr,"Unswap object %s (ref %d) from 0x%lx\n", ob->name, ob->ref, (unsigned long) ob->variables);
	}
    size = (num_var = ob->prog->inherit[ob->prog->num_inherited - 1].variable_index_offset +
	    ob->prog->num_variables + 1) * sizeof(struct svalue);

    ob->variables = (struct svalue *)xalloc(size);

    buf = start_read((u_int)ob->swap_num);
    for (i = 0; i < num_var; i++)
	unswap_svalue(buf, &ob->variables[i]);
    end_read(buf);
    ob->swap_num = 0;

    ob->flags &= ~O_SWAPPED;
    obj_bytes_swapped -= size;
    obj_swapped--;
    tot_alloc_variable_size += size;
    swap_in_obj++;
    num_swapped--;
    ob->variables++;
}

static int
swap_segment(char *hdr, struct segment_desc *seg)
{
    struct section_desc *sect;
    char *block = *(char **)(hdr + seg->ptr_offset);
    int i;

    if (*(int *)(hdr + seg->swap_idx_offset) == 0)
    {
	struct buffer *buf;

	buf = start_write((u_int *)(hdr + seg->swap_idx_offset));
	swap_write(buf, block, *(u_int *)(hdr + seg->size_offset));
	end_write(buf);
    }

    if (*(int *)(hdr + seg->swap_idx_offset) == -1)
	fatal("error while swapping segment.\n");

    for (sect = seg->sections, i = 0; sect->section != -1; sect++, i++)
	if (sect->ptr_offset != -1)
	    *(long *)(hdr + sect->ptr_offset) =
		*(char **)(hdr + sect->ptr_offset) - block;


    free(block);
    return *(int *)(hdr + seg->size_offset);
}

static int
unswap_segment(char *hdr, struct segment_desc *seg)
{
    struct section_desc *sect;
    char *block;
    int i;
    struct buffer *buf;

    block = xalloc((size_t)*(int *)(hdr + seg->size_offset));

    buf = start_read(*(u_int *)(hdr + seg->swap_idx_offset));
    swap_read(buf, block, *(u_int *)(hdr + seg->size_offset));
    end_read(buf);
    *(u_int *)(hdr + seg->swap_idx_offset) = 0;

    for (sect = seg->sections, i = 0; sect->section != -1; sect++, i++)
	if (sect->ptr_offset != -1)
	    *(char **)(hdr + sect->ptr_offset) =
		block + (int)*(long *)(hdr + sect->ptr_offset);
    
    *(char **)(hdr + seg->ptr_offset) = block;
    return *(int *)(hdr + seg->size_offset);
}

/*
 * Swap out lineno info for a program to the swap file
 * The linenoinfo is separated from the programblock thereby saving memory.
 * This is called directly after compilation from epilog() in postlang.y
 */
void
swap_lineno(struct program *prog)
{
    int size;
    
    if (d_flag & DEBUG_SWAP)  /* marion */
	debug_message("Swap lineno for %s (ref %d)\n", prog->name);

    if (!prog->line_numbers)
	return;
    size = swap_segment((char *)prog, segm_desc + S_DBG);
    total_lineno_swapped += size;
    num_swapped++;
}

/*
  Load the lineno info, this is only done when a runtime error occurs
*/
void 
load_lineno_from_swap(struct program *prog)
{
    int size;
    
    if (prog->line_numbers)
	return;

    if (prog->swap_lineno_index == 0)
	fatal("Loading not swapped linenoinfo.\n");

    size = unswap_segment((char *)prog, segm_desc + S_DBG);
    total_lineno_swapped -= size;
    num_swapped--;

}

static void
swap_program(struct program *prog)
{
    int size;

    if (prog->line_numbers)
	swap_lineno(prog);
    if (prog->program == (char *)0)
	return;
    if (d_flag & DEBUG_SWAP) { /* marion */
	debug_message("Swap program %s (ref %d)\n", prog->name, prog->ref);
    }

    size = swap_segment((char *)prog, segm_desc + S_EXEQ);
    total_program_size -= size;
    program_bytes_swapped += size;
    programs_swapped++;
    swap_out_prog++;
    num_swapped++;
}

void
load_prog_from_swap(struct program *prog)
{
    int size;
    
    if (prog->program != (char *)0 || prog->swap_num == 0)
	return;

    if (d_flag & DEBUG_SWAP) { /* marion */
	debug_message("Unswap program %s (ref %d)\n", prog->name, prog->ref);
    }

    size = unswap_segment((char *)prog, segm_desc + S_EXEQ);
    swap_in_prog++;
    total_program_size += size;
    program_bytes_swapped -= size;
    programs_swapped--;
    num_swapped--;
}

void
remove_ob_from_swap(struct object *ob)
{
    if (ob->flags & O_SWAPPED)
	load_ob_from_swap(ob);
}

void
remove_prog_from_swap(struct program *prog)
{
    if (prog->swap_num != 0)
	if (prog->program == (char *)0)
	    load_prog_from_swap(prog);

    if (prog->swap_lineno_index != 0)
	load_lineno_from_swap(prog);
}

#ifdef DO_CLEANUP
int
try_clean_up(struct object *ob)
{
    struct gdexception exception_frame;
    extern int eval_cost;
    struct svalue *ret;

    /*
     * Only if the clean_up returns a non-zero value, will it be called
     * again.
     */
    if (ob->flags & O_WILL_CLEAN_UP)
    {
	total_clean_ups++;

	if (d_flag & DEBUG_CLEAN_UP)
	    fprintf(stderr, "clean up %s\n", swap_ob->name);

	exception_frame.e_exception = exception;
	exception_frame.e_catch = 0;
	exception = &exception_frame;

	/*
	 * Supply a flag to the object that says if this program
	 * is inherited by other objects. Cloned objects might as well
	 * believe they are not inherited. Swapped objects will not
	 * have a ref count > 1 (and will have an invalid ob->prog
	 * pointer).
	 */
	if (setjmp(exception_frame.e_context))
	{
	    extern void clear_state(void);

	    clear_state();
	    debug_message("Error in clean_up.\n");
	    ret = NULL;
	}
	else
	{
	    eval_cost = 0;
	    push_number(ob->flags & (O_CLONE|O_SWAPPED) ? 0 : ob->prog->ref);
	    ret = apply("clean_up", ob, 1, 0);
	}
	exception = exception_frame.e_exception;
	if (ob->flags & O_DESTRUCTED)
	{
	    clean_up_destructs++;
	    return FALSE;
	}
	if (ret == NULL ||
	    (ret->type == T_NUMBER && ret->u.number == 0))
	{
	    clean_up_return0++;
	    ob->flags &= ~O_WILL_CLEAN_UP;
	}
	else
	{
	    clean_up_return1++;
	}
    }
    return TRUE;
}
#endif /* DO_CLEANUP */

void
try_to_swap(volatile int *interupted)
{
    int swap_time;
    
/*return;*/				/* turn off swap to test -- LA */

    if (swap_ob && swap_prog && used_memory >= min_swap_memory)
	while (swap_ob != obj_list && swap_prog != prog_list)
	{
	    swap_time = (used_memory < max_swap_memory) ?
		max_swap_time : min_swap_time;
	    
	    if (swap_ob->time_of_ref > swap_prog->time_of_ref)
	    {
		if (current_time - swap_prog->time_of_ref < swap_time)
		    break;
		swap_program(swap_prog);
		swap_prog = swap_prog->prev_all;
	    }
	    else
	    {
		if (current_time - swap_ob->time_of_ref < swap_time)
		    break;

#ifdef DO_CLEANUP
		if (try_clean_up(swap_ob))
		    swap_object(swap_ob);
#else /* DO_CLEANUP */
		swap_object(swap_ob);
#endif /* DO_CLEANUP */

		swap_ob = swap_ob->prev_all;
	    }
	    if (*interupted)
		return;
	}	
}
#endif
