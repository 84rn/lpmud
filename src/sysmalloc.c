/*
 * This is a dummy file, that will force the standard malloc
 * to be used.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(__NetBSD__) && !defined(__FreeBSD__) && !defined(__bsdi__) && \
    !defined(__OpenBSD__)
#include <malloc.h>
#endif

char *dump_malloc_data(void);
void sysmalloc_init(void);

void
sysmalloc_init()
{
#if !defined(__NetBSD__) && !defined(__FreeBSD__) && !defined(__bsdi__) && \
    !defined(__OpenBSD__)
    mallopt(M_MXFAST, 100);
    mallopt(M_NLBLKS, 1000);
    mallopt(M_GRAIN, 8);
#endif
}

#define dump_stat(str) (void)strcat(mbuf, str)
#define dump_stat1(str,p) (void)sprintf(smbuf,str,p); (void)strcat(mbuf, smbuf)
#define dump_stat2(str,stat,stat2) (void)sprintf(smbuf,str,stat,stat2); (void)strcat(mbuf,smbuf)

char *
dump_malloc_data()
{
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__bsdi__) || \
    defined(__OpenBSD__)
    return "";
#else
    static char mbuf[1024];
    char smbuf[100];
    struct mallinfo mem;

    mem = mallinfo();

#if defined(__osf__) || defined(_SEQUENT_) || defined(SOLARIS)
    (void)strcpy(mbuf,"Type                   Count      Space (bytes)\n");
    dump_stat2("sbrk requests:     %8d        %10d (a)\n",
	       mem.ordblks, mem.arena);
    dump_stat2("large blocks:      %8d        %10d (b)\n",
	       0, mem.uordblks);
    dump_stat2("large free blocks: %8d        %10d (c)\n\n",
	       0, mem.fordblks);
    dump_stat2("small blocks:      %8d        %10d (e)\n",
	       mem.smblks, mem.usmblks);
    dump_stat2("small free blocks: %8d        %10d (f)\n",
	       0, mem.fsmblks);
#else

    (void)strcpy(mbuf,"Type                   Count      Space (bytes)\n");
    dump_stat2("sbrk requests:     %8d        %10d (a)\n",
               mem.ordblks, mem.arena);
    dump_stat2("large blocks:      %8d        %10d (b)\n",
               mem.allocated, mem.uordblks);
    dump_stat2("large free blocks: %8d        %10d (c)\n\n",
               mem.ordblks - mem.allocated, mem.fordblks);
    dump_stat2("small blocks:      %8d        %10d (e)\n",
               0, mem.usmblks);
    dump_stat2("small free blocks: %8d        %10d (f)\n",
               0, mem.fsmblks);
#endif
    return mbuf;
#endif
}
