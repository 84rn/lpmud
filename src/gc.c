
#include <stdio.h>

extern char *gc_malloc(), *gc_realloc();
extern void gc_free();
struct block_status 
{
    void *alloc_address;
    int size;
};

#define align(size, data) ( (size) + sizeof(data) * \
			   !!( (size) % sizeof(data) ) \
    - ( (size) % sizeof(data) ) )

static unsigned int testp = 0;
char *dump_malloc_data() 
{
    return "";
}

int report_leak(p, sz)
    char *p;
    int sz;
{
    struct block_status *address;
    int size;
    /* Negative size ==> pointer-free (atomic) object */
    /* sz is in words.				      */
    if (sz < 0) sz = -sz;
    size = sz<<2;
    address = (struct block_status *)
	((char *)(p + size - sizeof(struct block_status)));
    
    gc_printf("Found unfreed block at address %#x, real_size %#x,"
	    "alloc. size %#x, allocated from address %#x.\n",
            p, size, address->size,address->alloc_address);
    return 0;
}


char *
malloc(unsigned size)
{
    register int *who_called asm ("%i7");
    struct block_status *save_addr;
    int real_size = align(size + sizeof(struct block_status), int *);

    char *p = gc_malloc(real_size);
    if (p == (char *) (testp * 10))
	(void)printf("FOUND malloc!\n");
    
    save_addr = (struct block_status *)
	((char *)(p + real_size - sizeof(struct block_status)));
    save_addr->alloc_address = who_called;
    save_addr->size = size;
    gc_printf("Allocated data at address %#x, size %#x,"
	      " real_size %#x, from address %#x.\n",
	      p, size, real_size, who_called);
    
    return p;
}

void 
free(char *p)
{
    if (p == (char *)(testp*10))
	(void)printf("FOUND free!\n");
    gc_free(p);
}

char *
calloc(unsigned a, unsigned b)
{
    char *p;
    register int *who_called asm ("%i7");
    struct block_status *save_addr;
    int real_size = align(a*b + sizeof(struct block_status), int *);
    
    p = gc_malloc(real_size);
    
    save_addr = (struct block_status *)
	((char *)(p + real_size - sizeof(struct block_status)));
    save_addr->alloc_address = who_called;
    save_addr->size = a*b;
    gc_printf("Allocated data at address %#x, size %#x,"
	      " real_size %#x, from address %#x.\n",
	      p, a*b, real_size, who_called);
    
    return p;
}

char *
realloc(char *p, unsigned size)
{
    char *p2;
    register int *who_called asm ("%i7");
    struct block_status *save_addr;
    int real_size = align(size + sizeof(struct block_status), int *);
    p2 = gc_realloc(real_size);
    save_addr = (struct block_status *)
	((char *)(p2 + real_size - sizeof(struct block_status)));
    save_addr->alloc_address = who_called;
    save_addr->size = size;
    gc_printf("Rellocated data at address %#x to address %#x, size %#x,"
	      " real_size %#x, from address %#x.\n",
	      p2, p, size, real_size, who_called);
    
    return p2;
    
}
