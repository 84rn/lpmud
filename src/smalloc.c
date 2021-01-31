
/* Satoria's malloc intended to be optimized for lpmud.
** this memory manager distinguishes between two sizes
** of blocks: small and large.  It manages them separately
** in the hopes of avoiding fragmentation between them.
** It expects small blocks to mostly be temporaries.
** It expects an equal number of future requests as small
** block deallocations.
**
** support for atari st/tt and FAST_FIT by amylaar @cs.tu-berlin.de 
**
** adapted by Blackthorn@Genocide to work with MudOS 0.9.15 - 93/01/26
** quick&dirty by tintin to make it run with the CD gamedriver 930203
*/

#if defined(sun)
#include <sys/types.h>
#endif

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "lint.h"
#include "simulate.h"

#ifdef USE_SWAP
extern int used_memory;
#endif

#if 0
/*defined(sparc)*/
#define MALLOC_ALIGN 8
#define MALLOC_ALIGN_8
#else
#define MALLOC_ALIGN 4
#endif

/* #undeffing SBRK_OK will just screw things up, since it tries to
 use malloc() to get memory, and smalloc() is renamed to malloc()  here
 to be compatible with MudOS...hmm   -Blackthorn */
#define SBRK_OK
#if defined(__OpenBSD__) || defined(__bsdi__) || defined(__NetBSD__) || \
    defined(__FreeBSD__)
#define POINTER void *
#else
#define POINTER char *
#endif
#define FREE_RETURN_TYPE void
#define FREE_RETURN return;
#define SFREE_RETURN_TYPE FREE_RETURN_TYPE
#define SFREE_RETURN FREE_RETURN

#define FIT_STYLE_FAST_FIT

#undef LARGE_TRACE

#define fake(s)

#define smalloc malloc
#define sfree   free
#define srealloc realloc

#define SMALL_BLOCK_MAX_BYTES	128
#define SMALL_CHUNK_SIZE	0x4000
#define CHUNK_SIZE		0x40000

#define SINT sizeof(int)
#define SMALL_BLOCK_MAX (SMALL_BLOCK_MAX_BYTES/SINT)

#define PREV_BLOCK	0x80000000
#define THIS_BLOCK	0x40000000
#define NO_REF  	0x20000000 /* check this in gcollect.c */
#define MASK		0x0FFFFFFF

#define MAGIC		0x17952932

/* SMALL BLOCK info */

#if defined( atarist ) || defined( linux ) || defined( AMIGA )
typedef unsigned long u;
#else
typedef unsigned int u;
#endif

static u *last_small_chunk = 0;
static u *sfltable[SMALL_BLOCK_MAX]={0,0,0,0,0,0,0,0};	/* freed list */
static u *next_unused=0;
static u unused_size=0;			/* until we need a new chunk */

/* LARGE BLOCK info */

#ifndef FIT_STYLE_FAST_FIT
static u *free_list=0;
#endif /* FIT_STYLE_FAST_FIT */
static u *start_next_block=0;

#ifdef SMALLOC_STATISTICS
/* This is unused, but we may want it later */
/* STATISTICS */

static long small_count[SMALL_BLOCK_MAX]={0,0,0,0,0,0,0,0};
static long small_total[SMALL_BLOCK_MAX]={0,0,0,0,0,0,0,0};
static long small_max[SMALL_BLOCK_MAX]  ={0,0,0,0,0,0,0,0};
static long small_free[SMALL_BLOCK_MAX] ={0,0,0,0,0,0,0,0};
#endif

typedef struct { unsigned counter, size; } t_stat;
#define count(a,b) { a.size+=(b); if ((b)<0) --a.counter; else ++a.counter; }
#define count_up(a,b)   { a.size+=(b); ++a.counter; }
#define count_back(a,b) { a.size-=(b); --a.counter; }

int debugmalloc=0;	/* Only used when debuging malloc() */

/********************************************************/
/*  SMALL BLOCK HANDLER					*/
/********************************************************/

static char *large_malloc(u);
static void large_free(char *);
static void build_block(u *, u);
static void mark_block(u *);
static char *esbrk(u);
static char *esbrk(u);
static void add_to_free_list(u *);
static void remove_from_free_list(u *);
#ifndef FIT_STYLE_FAST_FIT
static void show_block(u *);
#endif
char *dump_malloc_data(void);
int malloc_size_mask(void);
int malloced_size(POINTER);

#define s_size_ptr(p)	(p)
#define s_next_ptr(p)	((u **) (p+1))

t_stat small_alloc_stat={0,0};
t_stat small_free_stat={0,0};
t_stat small_chunk_stat={0,0};

POINTER 
smalloc(u size)
{
  /*int i;*/
  u *temp;

#ifdef DEBUG
  if (size == 0)
      fatal("Malloc size 0.\n");
#endif
  if (size > SMALL_BLOCK_MAX_BYTES)
    return large_malloc(size);

  size = (size+7) & ~3;			/* block size in bytes */
#define SIZE_INDEX(u_array, size) 	(*(u*) ((char*)u_array-8+size))
#define SIZE_PNT_INDEX(u_array, size)	(*(u**)((char*)u_array-8+size))
  /*i = (size - 8) >> 2;*/
  count_up(small_alloc_stat,size);


#if 0
  SIZE_INDEX(small_count, size) += 1;			/* update statistics */
  SIZE_INDEX(small_total, size) += 1;
  if (SIZE_INDEX(small_count, size) > SIZE_INDEX(small_max, size))
    SIZE_INDEX(small_max, size) = SIZE_INDEX(small_count, size);
#endif
  if ((temp = SIZE_PNT_INDEX(sfltable, size)) != NULL)
    {					/* allocate from the free list */
      count_back(small_free_stat, size);
      temp++;
      SIZE_PNT_INDEX(sfltable, size) = * (u **) temp;
#ifdef USE_SWAP
      used_memory += size;
#endif
      fake("From free list.");
      return (char *) temp;
    }					/* else allocate from the chunk */

  if (unused_size<size)			/* no room in chunk, get another */
    {
      fake("Allocating new small chunk.");
      if (unused_size) {
        if (unused_size < 8) {
          *s_size_ptr(next_unused) = 0;
        } else {
          *s_size_ptr(next_unused) = unused_size>>2;
          *s_next_ptr(next_unused) = SIZE_PNT_INDEX(sfltable, unused_size);
          SIZE_PNT_INDEX(sfltable, unused_size) = next_unused;
          count_up(small_free_stat, unused_size);
        }
      }
      next_unused = (u *) large_malloc(SMALL_CHUNK_SIZE + sizeof(u*));
#ifdef USE_SWAP
      used_memory -= SMALL_CHUNK_SIZE + sizeof(u*);
#endif
      if (next_unused == 0)
	return 0;
      *next_unused = (u)last_small_chunk;
      last_small_chunk = next_unused++;
      count_up(small_chunk_stat, SMALL_CHUNK_SIZE+SINT+sizeof(u*));
      unused_size = SMALL_CHUNK_SIZE;
    }
    else {
      fake("Allocated from chunk.");
    }


  temp = (u *) s_next_ptr(next_unused); 

  *s_size_ptr(next_unused) = size>>2;
  next_unused += size>>2;
  unused_size -= size;
#ifdef USE_SWAP
  used_memory += size;
#endif

fake("allocation from chunk successful\n");
  return (char *) temp;
}

#ifdef DEBUG
char *debug_free_ptr;
#endif /* DEBUG */

int malloc_size_mask() { return MASK; }

int malloced_size(ptr)
POINTER ptr;
{
    return ((u *)ptr)[-1] & MASK;
}

SFREE_RETURN_TYPE sfree(ptr)
POINTER ptr;
{
    u *block;
    u i;

#ifdef DEBUG
    debug_free_ptr = ptr;
#endif /* DEBUG */
    block = (u *) ptr;
    block -= 1;
    i = (*s_size_ptr(block) & MASK);
#ifdef USE_SWAP
  used_memory -= i << 2;
#endif
    
    if (i > SMALL_BLOCK_MAX + 1) {
	fake("sfree calls large_free");
	large_free(ptr);
	SFREE_RETURN
    }

  count_back(small_alloc_stat, i << 2);
  count_up(small_free_stat, i << 2);
  i -= 2;
  *s_next_ptr(block) = sfltable[i];
  sfltable[i] = block;
#if 0
  small_free[i] += 1;
#endif
fake("Freed");
  SFREE_RETURN
}

/************************************************/
/*	LARGE BLOCK HANDLER			*/
/************************************************/

#define BEST_FIT	0
#define FIRST_FIT	1
#define HYBRID		2

#define fit_style BEST_FIT
/* if this is a constant, evaluate at compile-time.... */
#ifndef fit_style
int fit_style =BEST_FIT;
#endif

#define l_size_ptr(p)		(p)
#define l_next_ptr(p)		(*((u **) (p+1)))
#define l_prev_ptr(p)		(*((u **) (p+2)))
#define l_next_block(p)		(p + (MASK & (*(p))) )
#define l_prev_block(p) 	(p - (MASK & (*(p-1))) )
#define l_prev_free(p)		(!(*p & PREV_BLOCK))
#define l_next_free(p)		(!(*l_next_block(p) & THIS_BLOCK))

#ifdef FIT_STYLE_FAST_FIT

#if defined(atarist) || defined (sun) || defined(AMIGA)
/* there is a type signed char */
    typedef /*signed*/ char balance_t;
#   define BALANCE_T_BITS 8
#else
    typedef short balance_t;
#   define BALANCE_T_BITS 16
#endif
#if (defined(atarist) && !defined(ATARI_TT)) || defined(sparc) || defined(AMIGA)
    /* try to avoid multiple shifts, because these are costly */
#   define NO_BARREL_SHIFT
#endif

struct free_block {
    u size;
    struct free_block *parent, *left, *right;
    balance_t balance;
    short align_dummy;
};

/* prepare two nodes for the free tree that will never be removed,
   so that we can always assume that the tree is and remains non-empty. */
/* some compilers don't understand forward declarations of static vars. */
extern struct free_block dummy2;
static struct free_block dummy =
	{ /*size*/0, /*parent*/&dummy2, /*left*/0, /*right*/0, /*balance*/0 };
       struct free_block dummy2 =
	{ /*size*/0, /*parent*/0, /*left*/&dummy, /*right*/0, /*balance*/-1 };

static struct free_block *free_tree = &dummy2;

#ifdef DEBUG_AVL
static int inconsistency = 0;

static int check_avl(parent, p)
struct free_block *parent, *p;
{
    int left, right;

    if (!p) return 0;
    left  = check_avl(p, p->left );
    right = check_avl(p, p->right);
    if (p->balance != right - left || p->balance < -1 || p->balance > 1) {
        (void)printf("Inconsistency in avl node!\n");
        (void)printf("node:%x\n",p);
        (void)printf("size: %d\n", p->size);
        (void)printf("left node:%x\n",p->left);
        (void)printf("left  height: %d\n",left );
        (void)printf("right node:%x\n",p->right);
        (void)printf("right height: %d\n",right);
        (void)printf("alleged balance: %d\n",p->balance);
        inconsistency = 1;
    }
    if (p->parent != parent) {
        (void)printf("Inconsistency in avl node!\n");
        (void)printf("node:%x\n",p);
        (void)printf("size: %d\n", p->size);
        (void)printf("parent: %x\n", parent);
        (void)printf("parent size: %d\n", parent->size);
        (void)printf("alleged parent: %x\n", p->parent);
        (void)printf("alleged parent size: %d\n", p->parent->size);
        (void)printf("left  height: %d\n",left );
        (void)printf("right height: %d\n",right);
        (void)printf("alleged balance: %d\n",p->balance);
        inconsistency = 1;
    }
    return left > right ? left+1 : right+1;
}

/* this function returns a value so that it can be used in ,-expressions. */
static int do_check_avl() {
    check_avl(0, free_tree);
    if (inconsistency) {
        (void)fflush(stderr);
        (void)fflush(stdout);
        fatal("Inconsistency could crash the driver\n");
    }
    return 0;
}
#endif /* DEBUG_AVL */

t_stat large_free_stat;     
void remove_from_free_list(ptr)
u *ptr;
{
    struct free_block *p, *q, *r, *s, *t;

    fake((do_check_avl(),"remove_from_free_list called"));
    p = (struct free_block *)(ptr+1);
    count_back(large_free_stat, p->size << 2);
#ifdef DEBUG_AVL
    (void)printf("node:%x\n",p);
    (void)printf("size:%d\n",p->size);
#endif
    if (p->left) {
        if ((q = p->right) != NULL) {
	    fake("two childs");
	    s = q;
	    for ( ; r = q, q = r->left; );
	    if (r == s) {
		r->left = s = p->left;
		s->parent = r;
		if ((r->parent = s = p->parent) != NULL) {
		    if (p == s->left) {
			s->left  = r;
		    } else {
			s->right = r;
		    }
		} else {
		    free_tree = r;
		}
		r->balance = p->balance;
		p = r;
		goto balance_right;
	    } else {
		t = r->parent;
		if ((t->left = s = r->right) != NULL) {
		    s->parent  = t;
		}
		r->balance = p->balance;
		r->left  = s = p->left;
		s->parent = r;
		r->right = s = p->right;
		s->parent = r;
		if ((r->parent = s = p->parent) != NULL) {
		    if (p == s->left) {
			s->left  = r;
		    } else {
			s->right = r;
		    }
		} else {
		    free_tree = r;
		}
		p = t;
		goto balance_left;
	    }
        } else { /* no right child, but left child */
            /* We set up the free list in a way so that there will remain at
               least two nodes, and the avl property ensures that the left
               child is a leaf ==> there is a parent */
            fake("no right child, but left child");
	    s = p;
	    p = s->parent;
            r = s->left;
            r->parent = p;
	    if (s == p->left) {
	        p->left  = r;
	        goto balance_left;
	    } else {
	        p->right = r;
	        goto balance_right;
	    }
        }
    } else { /* no left child */
        /* We set up the free list in a way so that there is a node left
           of all used nodes, so there is a parent */
	fake("no left child");
	s = p;
	p = s->parent;
        if ((q = r = s->right) != NULL) {
            r->parent = p;
        }
	if (s == p->left) {
	    p->left  = r;
	    goto balance_left;
	} else {
	    p->right = r;
	    goto balance_right;
	}
    }
balance_q:
    r = p;
    p = q;
    if (r == p->right) {
        balance_t b;
balance_right:
        b = p->balance;
        if (b > 0) {
            p->balance = 0;
            if ((q = p->parent) != NULL)
		goto balance_q;
            return;
        } else if (b < 0) {
	    r = p->left;
	    b = r->balance;
	    if (b <= 0) {
		/* R-Rotation */
#ifdef DEBUG_AVL
		fake("R-Rotation.");
		(void)printf("r->balance: %d\n", r->balance);
#endif
		if ((p->left = s = r->right) != NULL) {
		    s->parent = p;
		}
		r->right = p;
		s = p->parent;
		p->parent = r;
		b += 1;
		r->balance = b;
		b = -b;
#ifdef DEBUG_AVL
		(void)printf("node r: %x\n", r);
		(void)printf("r->balance: %d\n", r->balance);
		(void)printf("node p: %x\n", p);
		p->balance = b;
		(void)printf("p->balance: %d\n", p->balance);
		(void)printf("r-height: %d\n", check_avl(r->parent, r));
#endif
		if ((r->parent = s) != NULL) {
		    if ((p->balance = b) != NULL) {
		        if (p == s->left) {
			    s->left  = r;
			    return;
		        } else {
			    s->right = r;
			    return;
		        }
		    }
		    if (p == s->left) {
			fake("left from parent");
			goto balance_left_s;
		    } else {
			fake("right from parent");
			p = s;
			p->right = r;
			goto balance_right;
		    }
		}
		p->balance = b;
		free_tree = r;
		return;
	    } else { /* r->balance == +1 */
	        /* LR-Rotation */
	        balance_t b2;

	        fake("LR-Rotation.");
	        t = r->right;
	        b = t->balance;
	        if ((p->left = s = t->right) != NULL) {
	            s->parent = p;
	        }
	        if ((r->right = s = t->left) != NULL) {
	            s->parent = r;
	        }
	        t->left  = r;
	        t->right = p;
	        r->parent = t;
	        s = p->parent;
	        p->parent = t;
#ifdef NO_BARREL_SHIFT
		b = -b;
		b2 = b >> 1;
		r->balance = b2;
		b -= b2;
		p->balance = b;
#else
	        b2 = (unsigned char)b >> 7;
	        p->balance = b2;
	        b2 = -b2 -b;
	        r->balance = b2;
#endif
	        t->balance = 0;
#ifdef DEBUG_AVL
	        (void)printf("t-height: %d\n", check_avl(t->parent, t));
#endif
	        if ((t->parent = s) != NULL) {
	            if (p == s->left) {
	                p = s;
	                s->left  = t;
                        goto balance_left;
	            } else {
	                p = s;
                        s->right = t;
                        goto balance_right;
	            }
	        }
	        free_tree = t;
	        return;
	    }
        } else { /* p->balance == 0 */
            p->balance = -1;
            return;
        }
    } else { /* r == p->left */
        balance_t b;

	goto balance_left;
balance_left_s:
	p = s;
	s->left  = r;
balance_left:
        b = p->balance;
        if (b < 0) {
            p->balance = 0;
            if ((q = p->parent) != NULL)
		goto balance_q;
            return;
        } else if (b > 0) {
	    r = p->right;
	    b = r->balance;
	    if (b >= 0) {
		/* L-Rotation */
#ifdef DEBUG_AVL
		fake("L-Rotation.");
		(void)printf("r->balance: %d\n", r->balance);
#endif
		if ((p->right = s = r->left) != NULL) {
		    s->parent = p;
		}
		fake("subtree relocated");
		r->left = p;
		s = p->parent;
		p->parent = r;
		b -= 1;
		r->balance = b;
		b = -b;
#ifdef DEBUG_AVL
		fake("balances calculated");
		(void)printf("node r: %x\n", r);
		(void)printf("r->balance: %d\n", r->balance);
		(void)printf("node p: %x\n", p);
		p->balance = b;
		(void)printf("p->balance: %d\n", p->balance);
		(void)printf("r-height: %d\n", check_avl(r->parent, r));
#endif
		if ((r->parent = s) != NULL) {
		    if ((p->balance = b) != NULL) {
		        if (p == s->left) {
			    s->left  = r;
			    return;
		        } else {
			    s->right = r;
			    return;
		        }
		    }
		    if (p == s->left) {
			fake("left from parent");
			goto balance_left_s;
		    } else {
			fake("right from parent");
			p = s;
			p->right = r;
			goto balance_right;
		    }
		}
		p->balance = b;
		free_tree = r;
		return;
	    } else { /* r->balance == -1 */
	        /* RL-Rotation */
	        balance_t b2;

	        fake("RL-Rotation.");
	        t = r->left;
	        b = t->balance;
	        if ((p->right = s = t->left) != NULL) {
	            s->parent = p;
	        }
	        if ((r->left = s = t->right) != NULL) {
	            s->parent = r;
	        }
	        t->right = r;
	        t->left  = p;
	        r->parent = t;
	        s = p->parent;
	        p->parent = t;
#ifdef NO_BARREL_SHIFT
		b = -b;
		b2 = b >> 1;
		p->balance = b2;
		b -= b2;
		r->balance = b;
#else
	        b2 = (unsigned char)b >> 7;
	        r->balance = b2;
	        b2 = -b2 -b;
	        p->balance = b2;
#endif
	        t->balance = 0;
	        if ((t->parent = s) != NULL) {
	            if (p == s->left) {
	                p = s;
	                s->left  = t;
                        goto balance_left;
	            } else {
                        s->right = t;
	                p = s;
                        goto balance_right;
	            }
	        }
	        free_tree = t;
	        return;
	    }
        } else { /* p->balance == 0 */
            p->balance++;
            return;
        }
    }
}

void add_to_free_list(ptr)
u *ptr;
{
    u size;
    struct free_block *p, *q, *r;
    /* When there is a distinction between data and address registers and/or
       accesses, gcc will choose data type for q, so an assignmnt to q will
       faciliate branching
     */

    fake((do_check_avl(),"add_to_free_list called"));
    size = *ptr & MASK;
#ifdef DEBUG_AVL
    (void)printf("size:%d\n",size);
#endif
    q = (struct free_block *)size; /* this assignment is a hint for register
    				      choice */
    r = (struct free_block *)(ptr+1);
    count_up(large_free_stat, size << 2);
    q = free_tree;
    for ( ; ; /*p = q*/) {
        p = (struct free_block *)q;
#ifdef DEBUG_AVL
        (void)printf("checked node size %d\n",p->size);
#endif
        if (size < p->size) {
            if ((q = p->left) != NULL) {
                continue;
            }
            fake("add left");
            p->left = r;
            break;
        } else { /* >= */
            if ((q = p->right) != NULL) {
                continue;
            }
            fake("add right");
            p->right = r;
            break;
        }
    }
    r->size    = size;
    r->parent  = p;
    r->left    = 0;
    r->right   = 0;
    r->balance = 0;
#ifdef DEBUG_AVL
    fake("built new leaf.");
    (void)printf("p->balance:%d\n",p->balance);
#endif
    do {
        struct free_block *s;

        if (r == p->left) {
            balance_t b;

            if ( !(b = p->balance) ) {
#ifdef DEBUG_AVL
		(void)printf("p->size: %d\n", p->size);
		(void)printf("p->balance: %d\n", p->balance);
                (void)printf("p->right-h: %d\n", check_avl(p, p->right));
                (void)printf("p->left -h: %d\n", check_avl(p, p->left ));
		fake("growth propagation from left side");
#endif
		p->balance = -1;
            } else if (b < 0) {
#ifdef DEBUG_AVL
                (void)printf("p->balance:%d\n",p->balance);
#endif
                if (r->balance < 0) {
                    /* R-Rotation */
                    fake("R-Rotation");
                    if ((p->left = s = r->right) != NULL) {
                        s->parent = p;
                    }
                    r->right = p;
                    p->balance = 0;
                    r->balance = 0;
                    s = p->parent;
                    p->parent = r;
                    if ((r->parent = s) != NULL) {
			if ( s->left == p) {
			    s->left  = r;
			} else {
			    s->right = r;
			}
                    } else {
                        free_tree = r;
                    }
                } else { /* r->balance == +1 */
                    /* LR-Rotation */
		    balance_t b2;
                    struct free_block *t = r->right;

#ifdef DEBUG_AVL
                    fake("LR-Rotation");
                    (void)printf("t = %x\n",t);
                    (void)printf("r->balance:%d\n",r->balance);
#endif
                    if ((p->left = s = t->right) != NULL) {
                        s->parent = p;
                    }
                    fake("relocated right subtree");
                    t->right = p;
                    if ((r->right = s = t->left) != NULL) {
                        s->parent = r;
                    }
                    fake("relocated left subtree");
                    t->left  = r;
		    b = t->balance;
#ifdef NO_BARREL_SHIFT
		    b = -b;
		    b2 = b >> 1;
		    r->balance = b2;
		    b -= b2;
		    p->balance = b;
#else
		    b2 = (unsigned char)b >> 7;
		    p->balance = b2;
		    b2 = -b2 -b;
		    r->balance = b2;
#endif
                    t->balance = 0;
                    fake("balances calculated");
                    s = p->parent;
                    p->parent = t;
                    r->parent = t;
                    if ((t->parent = s) != NULL) {
			if ( s->left == p) {
			    s->left  = t;
			} else {
			    s->right = t;
			}
                    } else {
                        free_tree = t;
                    }
#ifdef DEBUG_AVL
                    (void)printf("p->balance:%d\n",p->balance);
                    (void)printf("r->balance:%d\n",r->balance);
                    (void)printf("t->balance:%d\n",t->balance);
                    fake((do_check_avl(),"LR-Rotation completed."));
#endif
                }
                break;
            } else { /* p->balance == +1 */
                p->balance = 0;
                fake("growth of left side balanced the node");
                break;
            }
        } else { /* r == p->right */
            balance_t b;

            if ( !(b = p->balance) ) {
		fake("growth propagation from right side");
		p->balance++;
            } else if (b > 0) {
                if (r->balance > 0) {
                    /* L-Rotation */
                    fake("L-Rotation");
                    if ((p->right = s = r->left) != NULL) {
                        s->parent = p;
                    }
                    r->left  = p;
                    p->balance = 0;
                    r->balance = 0;
                    s = p->parent;
                    p->parent = r;
                    if ((r->parent = s) != NULL) {
			if ( s->left == p) {
			    s->left  = r;
			} else {
			    s->right = r;
			}
                    } else {
                        free_tree = r;
                    }
                } else { /* r->balance == -1 */
                    /* RL-Rotation */
		    balance_t b2;
                    struct free_block *t = r->left;

#ifdef DEBUG_AVL
                    fake("RL-Rotation");
                    (void)printf("t = %x\n",t);
                    (void)printf("r->balance:%d\n",r->balance);
#endif
                    if ((p->right = s = t->left) != NULL) {
                        s->parent = p;
                    }
                    fake("relocated left subtree");
                    t->left  = p;
                    if ((r->left = s = t->right) != NULL) {
                        s->parent = r;
                    }
                    fake("relocated right subtree");
                    t->right = r;
		    b = t->balance;
#ifdef NO_BARREL_SHIFT
		    b = -b;
		    b2 = b >> 1;
		    p->balance = b2;
		    b -= b2;
		    r->balance = b;
#else
		    b2 = (unsigned char)b >> 7;
		    r->balance = b2;
		    b2 = -b2 -b;
		    p->balance = b2;
#endif
                    t->balance = 0;
                    s = p->parent;
                    p->parent = t;
                    r->parent = t;
                    if ((t->parent = s) != NULL) {
			if ( s->left == p) {
			    s->left  = t;
			} else {
			    s->right = t;
			}
                    } else {
                        free_tree = t;
                    }
                    fake("RL-Rotation completed.");
                }
                break;
            } else { /* p->balance == -1 */
#ifdef DEBUG_AVL
                (void)printf("p->balance: %d\n", p->balance);
                (void)printf("p->right-h: %d\n", check_avl(p, p->right));
                (void)printf("p->left -h: %d\n", check_avl(p, p->left ));
#endif
                p->balance = 0;
                fake("growth of right side balanced the node");
                break;
            }
        }
        r = p;
        p = p->parent;
    } while ((q = p) != NULL);
    fake((do_check_avl(),"add_to_free_list successful"));
}

#else /* FIT_STYLE_FAST_FIT */

void show_block(ptr)
u *ptr;
{
  (void)printf("[%c%d: %d]  ",(*ptr & THIS_BLOCK ? '+' : '-'),
		(int) ptr, *ptr & MASK);
}

void show_free_list()
{
   u *p;
   p = free_list;
   while (p) {
     show_block(p);
     p = l_next_ptr(p);
   }
   (void)printf("\n");
}

t_stat large_free_stat;     
void remove_from_free_list(ptr)
u *ptr;
{
   count_back(large_free_stat, (*ptr & MASK) << 2);

   if (l_prev_ptr(ptr))
     l_next_ptr(l_prev_ptr(ptr)) = l_next_ptr(ptr);
   else
     free_list = l_next_ptr(ptr);

   if (l_next_ptr(ptr))
     l_prev_ptr(l_next_ptr(ptr)) = l_prev_ptr(ptr);
}

void add_to_free_list(ptr)
u *ptr;
{
  extern int puts();
  count_up(large_free_stat, (*ptr & MASK) << 2);

#ifdef DEBUG
  if (free_list && l_prev_ptr(free_list)) 
    puts("Free list consistency error.");
#endif

  l_next_ptr(ptr) = free_list;
  if (free_list) 
    l_prev_ptr(free_list) = ptr;
  l_prev_ptr(ptr) = 0;
  free_list = ptr;
}
#endif /* FIT_STYLE_FAST_FIT */

static void build_block(ptr, size)	/* build a properly annotated unalloc block */
u *ptr;
u size;
{
  u tmp;

  tmp = (*ptr & PREV_BLOCK) | size;
  *(ptr+size-1) = size;
  *(ptr) = tmp;		/* mark this block as free */
  *(ptr+size) &= ~PREV_BLOCK; /* unmark previous block */
}

static void mark_block(ptr)		/* mark this block as allocated */
u *ptr;
{
  *l_next_block(ptr) |= PREV_BLOCK;
  *ptr |= THIS_BLOCK;
}

/*
 * It is system dependent how sbrk() aligns data, so we simpy use brk()
 * to insure that we have enough.
 */
t_stat sbrk_stat;

static char *esbrk(size)
u size;
{
#ifdef SBRK_OK
#if defined(__bsdi__) || defined(__NetBSD__) || defined(__FreeBSD__) || \
    defined(__OpenBSD__)
  extern char *sbrk(int);
  extern char *brk(const char *addr);
#else
#ifndef linux
  extern char *sbrk();
#endif
  extern int brk();
#endif
  static char *current_break=0;

  if (current_break == 0)
    current_break = sbrk(0);
  if (brk(current_break + size) == (POINTER)-1)
    return 0;
  count_up(sbrk_stat,size);
  current_break += size;
  return current_break - size;
#else  /* not SBRK_OK */
  count_up(sbrk_stat,size);
  return malloc(size);
#endif /* SBRK_OK */
}

t_stat large_alloc_stat;
static char *
large_malloc(u size)
{
    u real_size;
    u *ptr;

fake("large_malloc called");
#ifdef LARGE_TRACE
(void)printf("request:%d.",size);
#endif
    size = (size + 7) >> 2; 		/* plus overhead */
    count_up(large_alloc_stat, size << 2);

#if 0
retry:
#endif
    ptr = 0;
    {
#ifdef FIT_STYLE_FAST_FIT

	struct free_block *p, *q, *r;
	u minsplit;
	u tempsize;

	ptr++;
	minsplit = size + SMALL_BLOCK_MAX + 1;
	q = free_tree;
	for ( ; ; ) {
	    p = q;
#ifdef DEBUG_AVL
	    (void)printf("checked node size %d\n",p->size);
#endif
	    tempsize = p->size;
	    if (minsplit < tempsize) {
		ptr = (u*)p; /* remember this fit */
		if ((q = p->left) != NULL) {
		    continue;
		}
		/* We don't need that much, but that's the best fit we have */
		break;
	    } else if (size > tempsize) {
		if ((q = p->right) != NULL) {
		    continue;
		}
		break;
	    } else { /* size <= tempsize <= minsplit */
		if (size == tempsize) {
		    ptr = (u*)p;
		    break;
		}
		/* size < tempsize */
		if ((q = p->left) != NULL) {
		    r = p;
		    /* if r is used in the following loop instead of p,
		     * gcc will handle q very inefficient throughout the
		     * function large_malloc()
		     */
		    for (;;) {
			p = q;
			tempsize = p->size;
			if (size < tempsize) {
			    if ((q = p->left) != NULL) {
				continue;
			    }
			    break;
			} else if (size > tempsize ) {
			    if ((q = p->right) != NULL) {
			        continue;
			    }
			    break;
			} else {
			    ptr = (u*)p;
			    goto found_fit;
			}
		    }
		    p = r;
		}
		tempsize = p->size;
		if (minsplit > tempsize) {
		    if ((q = p->right) != NULL) {
			for (;;) {
			    p = q;
			    tempsize = p->size;
			    if (minsplit <= tempsize) {
				ptr = (u*)p; /* remember this fit */
				if ((q = p->left) != NULL) {
				    continue;
				}
				break;
			    } else { /* minsplit > tempsize */
				if ((q = p->right) != NULL) {
				    continue;
				}
				break;
			    }
			} /* end inner for */
			break;
		    }
		    break; /* no new fit */
		}
		/* minsplit == tempsize  ==> best non-exact fit */
		ptr = (u*)p;
		break;
	    }
	} /* end outer for */
found_fit:
	ptr--;
#else /* FIT_STYLE */
	u best_size;
	u *first, *best;
#ifdef LARGE_TRACE
	u search_length=0;
#endif

	first = best = 0;
	best_size = MASK;
	ptr = free_list;

	while (ptr) {
	    u tempsize;
#ifdef LARGE_TRACE
search_length++;
#endif
		/* Perfect fit? */
	    tempsize = *ptr & MASK;
	    if (tempsize == size) {
		best = first = ptr;
		break;
		/* always accept perfect fit */
	    }

		/* does it really even fit at all */
	    if (tempsize >= size + SMALL_BLOCK_MAX + 1)
	    {
		/* try first fit */
		if (!first) 
		{
		    first = ptr;
		    if (fit_style == FIRST_FIT)
		    break;
		    /* just use this one! */
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

#ifdef LARGE_TRACE
(void)printf("search length %d\n",search_length);
#endif
	if (fit_style==BEST_FIT) ptr = best;
	else ptr = first;
	/* FIRST_FIT and HYBRID both leave it in first */

#endif /* FIT_STYLE */
    } /* end of  block */
    if (!ptr)		/* no match, allocate more memory */
    {
      u chunk_size, block_size;
      block_size = size*SINT;
      if (block_size > CHUNK_SIZE)
	chunk_size = block_size;
      else
	chunk_size = CHUNK_SIZE;

#ifdef SBRK_OK
      if (!start_next_block) {
	start_next_block = (u *) esbrk(SINT);
	if (!start_next_block)
	  fatal("Couldn't malloc anything\n");
	*(start_next_block) = PREV_BLOCK;
fake("Allocated little fake block");
      }

      ptr = (u *) esbrk(chunk_size);
#else  /* not SBRK_OK */
      ptr = (u *) esbrk(chunk_size+SINT);
#endif /* SBRK_OK */
      if (ptr == 0) {
	return 0;
#if 0
	extern char *reserved_area;
	extern int slow_shut_down_to_do;
	static int going_to_exit=0;
	static char mess1[] = "Temporary out of MEMORY. Freeing reserve.\n";
	static char mess2[] = "Totally out of MEMORY.\n";

	if (going_to_exit)
	  exit(3);
	if (reserved_area) {
	    sfree(reserved_area);
	    reserved_area = 0;
	    (void)write(1, mess1, sizeof(mess1)-1);
	    slow_shut_down_to_do = 6;
	    force_more = 0;
	    goto retry;
	}
	going_to_exit = 1;
	(void)write(1, mess2, sizeof(mess2)-1);
	(void)dump_trace(0);
	exit(2);
#endif
      }
#ifdef SBRK_OK
      ptr -= 1;				/* overlap old memory block */
#else /* not SBRK_OK */
      if (start_next_block == ptr) {
	  ptr -= 1;			/* overlap old memory block */
	  chunk_size += SINT;
      } else
	  *ptr = PREV_BLOCK;
      start_next_block = (u*)((char *)ptr + chunk_size);
#endif /* SBRK_OK */
      block_size = chunk_size / SINT;

			/* configure header info on chunk */
      
      build_block(ptr,block_size);
fake("Built memory block description.");
      *l_next_block(ptr)=THIS_BLOCK;
      add_to_free_list(ptr);
    }    /* end of creating a new chunk */
  remove_from_free_list(ptr);
  real_size = *ptr & MASK;

  if (real_size - size) {
	/* split block pointed to by ptr into two blocks */
    build_block(ptr+size, real_size-size);
fake("Built empty block");
    /* When we allocate a new chunk, it might differ very slightly in size from
     * the desired size.
     */
    if (real_size - size >= SMALL_BLOCK_MAX + 1) {
	add_to_free_list(ptr+size);
    } else {
	mark_block(ptr+size);
    }
    build_block(ptr, size);
  }
#ifdef USE_SWAP
    used_memory += size << 2;
#endif

  mark_block(ptr);
fake("built allocated block");
  return (char *) (ptr + 1);
}

static void large_free(ptr)
char *ptr;
{
  u size, *p;
  p = (u *) ptr;
  p-=1;
  size = *p & MASK;
  if (!(*p & THIS_BLOCK))
      fatal("Freeing free block!\n");

  count_back(large_alloc_stat, (size << 2));

  if (!(*(p+size) & THIS_BLOCK)) {
    remove_from_free_list(p+size);
    size += (*(p+size) & MASK);
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

POINTER srealloc(p, size)
POINTER p; u size;
{
   unsigned *q, old_size;
   char *t;

   if (p == NULL)
	return malloc(size);

   q = (unsigned *) p;
	
#if MALLOC_ALIGN > 4
   while ( !(old_size = *--q) );
   old_size = ((old_size & MASK)-1)*sizeof(int);
#else
   --q;
   old_size = ((*q & MASK)-1)*sizeof(int);
#endif
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
POINTER calloc(nelem, sizel)
    u nelem, sizel;
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
verify_sfltable() {
    u *p;
    int i, j;
    extern int end;
    
    if (!debugmalloc)
	return;
    if (unused_size > SMALL_CHUNK_SIZE)
	apa();
    for (i=0; i < SMALL_BLOCK_MAX; i++) {
	for (j=0, p = sfltable[i]; p; p = * (u **) (p + 1), j++) {
	    if (p < (u *)&end || p > (u *) 0xfffff)
		apa();
	    if (*p - 2 != i)
		apa();
	}
	if (p >= next_unused && p < next_unused + (unused_size>>2))
	    apa();
    }
    p = free_list;
    while (p) {
	if (p >= next_unused && p < next_unused + (unused_size>>2))
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
	    if (p >= next_unused && p < next_unused + (unused_size>>2))
		apa();
	}
    }
    
    p = free_list;
    while (p) {
	if (ptr >= p && ptr < p + (*p & MASK))
	    apa();
	if (p >= ptr && p < ptr + (*ptr & MASK))
	    apa();
	if (p >= next_unused && p < next_unused + (unused_size>>2))
	    apa();
	p = l_next_ptr(p);
    }
    if (ptr >= next_unused && ptr < next_unused + (unused_size>>2))
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


