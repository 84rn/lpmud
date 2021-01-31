#include <stdio.h>
#include <memory.h>
#include <machine/reg.h>
#include <string.h>
#include "lint.h"


#define fake(s)

#ifndef DEBUG_MALLOC
#define smalloc malloc  /* Dworkin removal */

#define sfree free
#define srealloc realloc
#else
#define my_malloc malloc
#define my_realloc realloc
#define my_free free
#endif

#define SMALL_BLOCK_MAX_BYTES	32
#define SMALL_CHUNK_SIZE	0x4000
#define CHUNK_SIZE		0x40000

#define SINT sizeof(int)
#define SMALL_BLOCK_MAX (SMALL_BLOCK_MAX_BYTES/SINT)

#define PREV_BLOCK	0x80000000
#define THIS_BLOCK	0x40000000
#define USED_BLOCK	0x20000000
#define MARKED_BLOCK	0x10000000
#define MASK		0x0FFFFFFF

#define MAGIC		0x17952932

/* SMALL BLOCK info */

typedef unsigned int u;

static u *sfltable[SMALL_BLOCK_MAX];	/* freed list */
static u *next_unused;
static u unused_size;			/* until we need a new chunk */

/* LARGE BLOCK info */

static u *free_list;
static u *start_next_block;

/* STATISTICS */

static int small_count[SMALL_BLOCK_MAX];
static int small_total[SMALL_BLOCK_MAX];
static int small_max[SMALL_BLOCK_MAX];
static int small_free[SMALL_BLOCK_MAX];

typedef struct { unsigned counter, size; } stat;
#define count(a,b) { a.size+=(b); if ((b)<0) --a.counter; else ++a.counter; }

int debugmalloc;	/* Only used when debuging malloc() */
    
    /********************************************************/
    /*  SMALL BLOCK HANDLER					*/
    /********************************************************/
    
    static char *large_malloc();
    static void large_free();
    
#define s_size_ptr(p)	(p)
#define s_next_ptr(p)	((u **) (p+1))
    
    stat small_alloc_stat;
    stat small_free_stat;
    stat small_chunk_stat;
    
char *
smalloc(u size)
{
    int i;
    u *temp;
    
    if (size == 0)
	fatal("Malloc size 0.\n");
    if (size>SMALL_BLOCK_MAX_BYTES)
	return large_malloc(size,0);
    
    i = (size - 1) >> 2;
    size = i+2;				/* block size in ints */
    count(small_alloc_stat,size << 2);
    
    small_count[i] += 1;			/* update statistics */
    small_total[i] += 1;
    if (small_count[i] >= small_max[i])
	small_max[i] = small_count[i];
    
    if (sfltable[i]) 
    {					/* allocate from the free list */
	count(small_free_stat, -(int) (size << 2));
	temp = sfltable[i];
	sfltable[i] = * (u **) (temp+1);
	fake("From free list.");
	return (char *) (temp+1);
    }					/* else allocate from the chunk */
    
    if (unused_size<size)			/* no room in chunk, get another */
    {
	if (unused_size >= 2)
	{
	    /* put remaining unused on free-list */
	    int i = unused_size - 2;
	    count(small_free_stat, unused_size << 2);
	    small_total[i]++;
	    small_free[i]++;
	    small_count[i]++;
	    if (small_count[i] >= small_max[i])
		small_max[i] = small_count[i];
	    *s_size_ptr(next_unused) = unused_size;
	    *s_next_ptr(next_unused) = sfltable[i];
	    sfltable[i] = next_unused;
	}
	
	
	next_unused = (u *) large_malloc(SMALL_CHUNK_SIZE,1);
	if (next_unused == 0)
	    return 0;
	count(small_chunk_stat, SMALL_CHUNK_SIZE+SINT);
	unused_size = SMALL_CHUNK_SIZE / SINT;
    }
    else fake("Allocated from chunk.");
    
    
    temp = (u *) s_next_ptr(next_unused); 
    
    *s_size_ptr(next_unused) = size;
    next_unused += size;
    unused_size -= size;
    
    return (char *) temp;
}

char *debug_free_ptr;

void 
sfree(char *ptr)
{
    u *block;
    u i;
    
    debug_free_ptr = ptr;
    block = (u *) ptr;
    block -= 1;
    if ((*s_size_ptr(block) & MASK) > SMALL_BLOCK_MAX + 1)
    { large_free(ptr); return; }
    
    i = *block - 2;
    count(small_alloc_stat, - (int) ((i+2) << 2));
    count(small_free_stat, (i+2) << 2);
    *s_next_ptr(block) = sfltable[i];
    sfltable[i] = block;
    small_free[i] += 1;
    fake("Freed");
}

/************************************************/
/*	LARGE BLOCK HANDLER			*/
/************************************************/

#define BEST_FIT	0
#define FIRST_FIT	1
#define HYBRID		2
int fit_style =BEST_FIT;

#define l_size_ptr(p)		(p)
#define l_next_ptr(p)		(*((u **) (p+1)))
#define l_prev_ptr(p)		(*((u **) (p+2)))
#define l_next_block(p)		(p + (*(p)))
#define l_prev_block(p) 	(p - (*(p-1)))
#define l_prev_free(p)		(!(*p & PREV_BLOCK))
#define l_next_free(p)		(!(*l_next_block(p) & THIS_BLOCK))

void show_block(ptr)
    u *ptr;
{
    (void)printf("[%c%d: %d]  ",(*ptr & THIS_BLOCK ? '+' : '-'),
		 (int) ptr, *ptr & MASK);
}

void 
show_free_list()
{
    u *p;
    p = free_list;
    while (p) {
	show_block(p);
	p = l_next_ptr(p);
    }
    (void)printf("\n");
}

stat large_free_stat;     
void 
remove_from_free_list(u ptr)
{
    count(large_free_stat, - (int) (*ptr & MASK) << 2);
    
    if (l_prev_ptr(ptr))
	l_next_ptr(l_prev_ptr(ptr)) = l_next_ptr(ptr);
    else
	free_list = l_next_ptr(ptr);
    
    if (l_next_ptr(ptr))
	l_prev_ptr(l_next_ptr(ptr)) = l_prev_ptr(ptr);
}
u *last_block = 0;
u *first_block = 0;

void 
add_to_free_list(u *ptr)
{
    extern int puts();
    count(large_free_stat, (*ptr & MASK) << 2);
    if ((u)ptr > (u)last_block) last_block = ptr;
    if (!first_block) first_block = ptr;
    
    if (free_list && l_prev_ptr(free_list)) 
	puts("Free list consistency error.");
    
    l_next_ptr(ptr) = free_list;
    if (free_list) 
	l_prev_ptr(free_list) = ptr;
    l_prev_ptr(ptr) = 0;
    free_list = ptr;
}

void 
build_block(u *ptr, u size)	/* build a properly annotated unalloc block */
{
    *(ptr) = (*ptr & PREV_BLOCK) | size;		/* mark this block as free */
    *(ptr+size-1) = size;
    *(ptr+size) &= (MASK | THIS_BLOCK); /* unmark previous block */
}

static void 
mark_block(u *ptr)		/* mark this block as allocated */
{
    *l_next_block(ptr) |= PREV_BLOCK;
    *ptr |= THIS_BLOCK;
}

/*
 * It is system dependent how sbrk() aligns data, so we simpy use brk()
 * to insure that we have enough.
 */
stat sbrk_stat;
static char *current_break;
static char *
esbrk(u size)
{
    extern char *sbrk();
    extern int brk();
    
    if (current_break == 0)
	current_break = sbrk(0);
    if (brk(current_break + size) == -1)
	return 0;
    count(sbrk_stat,size);
    current_break += size;
    return current_break - size;
}

stat large_alloc_stat;
static char *
large_malloc(u size, int force_more)
{
    u best_size, real_size;
    u *first, *best, *ptr;
    
    size = (size + 7) >> 2; 		/* plus overhead */
    
    first = best = 0;
    best_size = MASK;
    
    if (force_more)
	ptr = 0;
    else
	ptr = free_list;
    
    while (ptr) {
	u tempsize;
	/* Perfect fit? */
	tempsize = *ptr & MASK;
	if (tempsize == size) {
	    best = first = ptr;
	    break;		/* always accept perfect fit */
	}
	
	/* does it really even fit at all */
	if (tempsize > size)
	{
	    /* try first fit */
	    if (!first) 
	    {
		first = ptr;
		if (fit_style == FIRST_FIT)
		    break;			/* just use this one! */
	    }
	    /* try best fit */
	    tempsize -= size;
	    if (tempsize>0 && tempsize<=best_size) 
	    {
		best = ptr;
		best_size = tempsize;
	    }
	}
	ptr = l_next_ptr(ptr);
    } /* end while */
    
    if (fit_style==BEST_FIT) ptr = best;
    else ptr = first;	/* FIRST_FIT and HYBRID both leave it in first */
    
    if (!ptr)		/* no match, allocate more memory */
    {
	u chunk_size, block_size;
	block_size = size*SINT;
	if (force_more || (block_size>CHUNK_SIZE))
	    chunk_size = block_size;
	else
	    chunk_size = CHUNK_SIZE;
	
	if (!start_next_block) {
	    start_next_block = (u *) esbrk(SINT);
	    if (!start_next_block)
		return 0;
	    *(start_next_block) = PREV_BLOCK;
	    fake("Allocated little fake block");
	}
	
	ptr = (u *) esbrk(chunk_size);
	if (ptr == 0)
	    return 0;
	ptr -= 1;		/* overlap old memory block */
	block_size = chunk_size / SINT;
	
	/* configure header info on chunk */
	
	build_block(ptr,block_size);
	if (force_more)
	    fake("Build little block");
	else
	    fake("Built memory block description.");
	*l_next_block(ptr)=THIS_BLOCK;
	add_to_free_list(ptr);
    }    /* end of creating a new chunk */
    remove_from_free_list(ptr);
    real_size = *ptr & MASK;
    
    if (real_size - size > SMALL_BLOCK_MAX) {
	/* split block pointed to by ptr into two blocks */
	build_block(ptr+size, real_size-size);
	fake("Built empty block");
	add_to_free_list(ptr+size);
	build_block(ptr, size);
	real_size = size;
    }
    count(large_alloc_stat, real_size << 2);
    
    mark_block(ptr);
    fake("built allocated block");
    return (char *) (ptr + 1);
}

static void 
large_free(char *ptr)
{
    u size, *p;
    p = (u *) ptr;
    p-=1;
    size = *p & MASK;
    count(large_alloc_stat, - (int) (size << 2));
    
    if (l_next_free(p)) {
	remove_from_free_list(l_next_block(p));
	size += (*l_next_block(p) & MASK);
	*p = (*p & PREV_BLOCK) | size;
    }
    
    if (l_prev_free(p)) {
	remove_from_free_list(l_prev_block(p));
	size += (*l_prev_block(p) & MASK);
	p = l_prev_block(p);
    }
    
    build_block(p, size);
    
    add_to_free_list(p);
}

char *
srealloc(char *p, u size)
{
    unsigned *q, old_size;
    char *t;
    
    q = (unsigned *) p;
    --q;
    old_size = ((*q & MASK)-1)*sizeof(int);
    if (old_size >= size)
	return p;
    
    t = malloc(size);
    if (t == 0) return (char *) 0;
    
    (void)memcpy(t, p, old_size);
    free(p);
    return t;
}

#define dump_stat(str) (void)strcat(mbuf, str)
#define dump_stat1(str,p) (void)sprintf(smbuf,str,p); (void)strcat(mbuf, smbuf)
#define dump_stat2(str,stat) (void)sprintf(smbuf,str,stat.counter,stat.size); (void)strcat(mbuf,smbuf)
char *
dump_malloc_data()
{
    static char mbuf[1024];
    char smbuf[100];
    
    (void)strcpy(mbuf,"Type                   Count      Space (bytes)\n");
    dump_stat2("sbrk requests:     %8d        %10d (a)\n",sbrk_stat);
    dump_stat2("large blocks:      %8d        %10d (b)\n",large_alloc_stat);
    dump_stat2("large free blocks: %8d        %10d (c)\n\n",large_free_stat);
    dump_stat2("small chunks:      %8d        %10d (d)\n",small_chunk_stat);
    dump_stat2("small blocks:      %8d        %10d (e)\n",small_alloc_stat);
    dump_stat2("small free blocks: %8d        %10d (f)\n",small_free_stat);
    
    dump_stat1("unused from current chunk          %10d (g)\n\n",unused_size<<2);
    dump_stat("    Small blocks are stored in small chunks, which are allocated as\n");
    dump_stat("large blocks.  Therefore, the total large blocks allocated (b) plus\n");
    dump_stat("the large free blocks (c) should equal total storage from sbrk (a).\n");
    dump_stat("Similarly, (e) + (f) + (g) equals (d).  The total amount of storage\n");
    dump_stat("wasted is (c) + (f) + (g); the amount allocated is (b) - (f) - (g).\n");
    return mbuf;
}

/*
 * calloc() is provided because some stdio packages uses it.
 */
char *
calloc(int nelem, int sizel)
{
    char *p;
    
    if (nelem == 0 || sizel == 0)
	return 0;
    p = malloc(nelem * sizel);
    if (p == 0)
	return 0;
    (void)memset(p, '\0', nelem * sizel);
    return p;
}

/*
 * Functions below can be used to debug malloc.
 */

#if 0

int debugmalloc;
/*
 * Verify that the free list is correct. The upper limit compared to
 * is very machine dependant.
 */
verify_sfltable() 
{
    u *p;
    int i, j;
    extern int end;
    
    if (!debugmalloc)
	return;
    if (unused_size > SMALL_CHUNK_SIZE / SINT)
	apa();
    for (i=0; i < SMALL_BLOCK_MAX; i++) {
	for (j=0, p = sfltable[i]; p; p = * (u **) (p + 1), j++) {
	    if (p < (u *)&end || p > (u *) 0xfffff)
		apa();
	    if (*p - 2 != i)
		apa();
	}
	if (p >= next_unused && p < next_unused + unused_size)
	    apa();
    }
    p = free_list;
    while (p) {
	if (p >= next_unused && p < next_unused + unused_size)
	    apa();
	p = l_next_ptr(p);
    }
}

verify_free(ptr)
    u *ptr;
{
    u *p;
    int i, j;
    
    if (!debugmalloc)
	return;
    for (i=0; i < SMALL_BLOCK_MAX; i++) {
	for (j=0, p = sfltable[i]; p; p = * (u **) (p + 1), j++) {
	    if (*p - 2 != i)
		apa();
	    if (ptr >= p && ptr < p + *p)
		apa();
	    if (p >= ptr && p < ptr + *ptr)
		apa();
	    if (p >= next_unused && p < next_unused + unused_size)
		apa();
	}
    }
    
    p = free_list;
    while (p) {
	if (ptr >= p && ptr < p + (*p & MASK))
	    apa();
	if (p >= ptr && p < ptr + (*ptr & MASK))
	    apa();
	if (p >= next_unused && p < next_unused + unused_size)
	    apa();
	p = l_next_ptr(p);
    }
    if (ptr >= next_unused && ptr < next_unused + unused_size)
	apa();
}

apa() {
    int i;
    i/0;
}

static char *ref;
test_malloc(p)
    char *p;
{
    if (p == ref)
	(void)printf("Found 0x%x\n", p);
}

#endif /* 0 (never) */


#if DEBUG_MALLOC
static int unmarked_blocks;
static int num_blocks;
static int used_blocks;
static int alloc_blocks;

static void mark_used(ptr)
    u *ptr;
{
    u *block = first_block;
    char *cptr = (char *)ptr;
    
    int i;
    
    if ( ((int)ptr & 1) ||
	(u)ptr <= (u)first_block ||
	(u)ptr >= (u)last_block + (*last_block & MASK) )
	return;

    while (ptr > block + (*block & MASK))
	block += *block & MASK;

    if (ptr != block + block[1] + 1  &&
	(cptr - sizeof(char *) - sizeof(short)) !=
	block + block[1] + 1) return;

    if ( *block & USED_BLOCK) return;
    *block |= USED_BLOCK;
    used_blocks++;
    unmarked_blocks++;
}

static void mark_blocks()
{
    u *block;

    u i;
    for (block = first_block; block <= last_block; block += (*block & MASK))
	if ((*block & (USED_BLOCK | MARKED_BLOCK)) == USED_BLOCK)
	{
	    *block |= MARKED_BLOCK;
	    unmarked_blocks--;
	    for (i = block[1] + 1; i < (*block & MASK); i++)
		mark_used(block[i]);
	}
}

static void prepare()
{
    u *block;
    num_blocks = 0;
    used_blocks = 0;
    unmarked_blocks = 0;
    alloc_blocks = 0;
    
    for (block = first_block; (u)block <= (u)last_block ; block += *block & MASK)
    {
	num_blocks++;
	if (*block & THIS_BLOCK) 
	{
	    alloc_blocks++;
	    *block &= ~(USED_BLOCK | MARKED_BLOCK);
	}
	else
	    *block |= USED_BLOCK | MARKED_BLOCK;
    }
}

static unsigned csp;
static unsigned tsp;

static void get_sp()
{
    register struct rwindow *sp asm("%sp");
    register struct rwindow *fp asm("%fp");
    struct rwindow* lsp = sp;
    struct rwindow* lfp = fp;

    asm("ta 3");
    for (lfp = fp; lfp->rw_fp; lfp = (struct rwindow *)lfp->rw_fp);
    tsp = (unsigned)lfp;
    csp = (unsigned)fp;
}

static void mark_data()
{
    u *i;
    extern unsigned environ, end;
    

    for(i = &environ; i < &end; i++) /* mark data and bss segment */
	mark_used(*i);

    get_sp(); /* mark stack */
    for(i = (unsigned *)csp; i < (unsigned *)tsp; i++)
	mark_used(*i);

    while (unmarked_blocks)   /* mark heap */
    {
	mark_blocks();
    }
    
}

static void found_leak(block)
    unsigned *block;
{
    char buff[2048];
    int i;
    
    (void)sprintf(buff,"block %#x size %#x backtrace", block,
	    ((block[-1] & MASK) - 1 - block[0]) * sizeof(unsigned));
    (void)write(1,buff,strlen(buff));
    for (i = 1; i < block[0] - 1; i++)
    {
	(void)sprintf(buff," %#x",block[i]);
	(void)write(1,buff,strlen(buff));
    }
    (void)write(1," end backtrace\n",15);
}

static void reclaim()
{
    u *block;
    for (block = first_block; (u)block <= (u)last_block; block += *block & MASK)
	if (!(*block & USED_BLOCK))
	    found_leak(block + 1);
}    

static unsigned *backtrack()
{
    register struct rwindow *fp asm("%fp");
    struct rwindow *lfp = fp;
    int buff[1024], i = 1;
    
    asm("ta 3");
    for (lfp = fp; lfp->rw_fp; lfp = (struct rwindow *)lfp->rw_fp)
    {
	buff[i++] = lfp->rw_rtn;
    }
    buff[i++] = 0;
    buff[i] = i + 1;
    buff[0] = i + 1;
    return buff;
}

char *my_malloc(size)
    unsigned size;
{
    unsigned *bt = backtrack();
    char *block;
    int nsize = size + bt[0] * sizeof(unsigned);

    if (nsize < 32)
	nsize = 32;
    block = large_malloc(nsize);
    if (block == NULL) return block;

#if defined(sun) && !defined(SOLARIS)
    bcopy(bt, block, sizeof(unsigned) * bt[0]);
#else
    memmove(block, bt, sizeof(unsigned) * bt[0]);
#endif
    return block + bt[0] * sizeof(unsigned);
}

void my_free(ptr)
    char *ptr;
{
    unsigned *uptr = (unsigned *)ptr;
    large_free(uptr - uptr[-1]);
}

char *my_realloc(ptr,size)
    char *ptr;
    unsigned size;
{
    unsigned *uptr = (unsigned *)ptr;
    unsigned *bt;
    char *block;
    unsigned nsize;
    unsigned osize = ((uptr[uptr[-1] - 1] & MASK) - 1) * sizeof(int);
    unsigned osize2 = osize - ptr[-1] * sizeof(unsigned);

    bt = backtrack();
    nsize = size + bt[0] * sizeof(unsigned);
    if (nsize < 32)
	nsize = 32;
    
    block = large_malloc(nsize);
    if (block == NULL) return block;

#if defined(sun) && !defined(SOLARIS)
    bcopy(bt, block, sizeof(unsigned) * bt[0]);
    bcopy(ptr, block + bt[0] * sizeof(unsigned),
	  size < osize2 ? size : osize2);
#else
    memmove(block, bt, sizeof(unsigned) * bt[0]);
    memmove(block + bt[0] * sizeof(unsigned), ptr, size < osize2 ? size : osize2);
#endif
    large_free(uptr - uptr[-1]);
    return block + bt[0] * sizeof(unsigned);
}

void debug_malloc()
{
    prepare();
    mark_data();
    reclaim();
}

#endif
