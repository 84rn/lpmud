#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#ifdef RANDOM
#include <math.h>
#endif

#include "config.h"
#include "lint.h"

#ifdef DRAND48
#include "drand48.h"
#endif

#ifdef sun
time_t time (time_t *);
#endif

double random_float(void);

extern int current_time;
extern int alarm_time;

/*
 * This file defines things that may have to be changed when porting
 * LPmud to new environments. Hopefully, there are #ifdef's that will take
 * care of everything.
 */

#ifdef RANDOM
static unsigned long newstate[] = {
    3,
    0x9a319039, 0x32d9c024, 0x9b663182, 0x5da1f342,
    0x7449e56b, 0xbeb1dbb0, 0xab5c5918, 0x946554fd,
    0x8c2e680f, 0xeb3d799f, 0xb11ee0b7, 0x2d436b86,
    0xda672e2a, 0x1588ca88, 0xe369735d, 0x904f35f7,
    0xd7158fd6, 0x6fa6f051, 0x616e6b96, 0xac94efdc,
    0xde3b81e0, 0xdf0a6fb5, 0xf103bc02, 0x48f340fb,
    0x36413f93, 0xc622c298, 0xf5a42ab8, 0x8a88d77b,
    0xf5ad9d0e, 0x8999220b, 0x27fb47b9
};
#endif

/*
 * Return a random argument in the range 0 .. n-1.
 * If a new seed is given, apply that before computing the random number.
 */
int
random_number(int n, int seed)
{
#ifdef RANDOM
    long rnd;
    char *oldstate;
#else
#ifdef DRAND48
    unsigned short newseed[3];
#endif
#endif

    /* n should not be zero or negative
    */
    if (n < 1)
	n = 1;

#ifdef RANDOM
    if (seed == 0)
	return (int)(random() % n);
    else
    {
	/*
	 * Cavort and prance. All in order to preserve the old seed.
	 */
	
	initstate(seed, (char *)newstate, 128);
	oldstate = (char*)setstate((char *)newstate);
	rnd = random() % n;
	setstate(oldstate);
	return (int)rnd;
    }
#else /* RANDOM */
#ifdef DRAND48
    if (seed == 0)
	return (int)(drand48() * n);
    else
    {
	newseed[0] = seed & 0xffff;
	newseed[2] = ((unsigned)seed >> 16) & 0xffff;
	newseed[1] = newseed[0] ^ newseed[2];
	return (int)(erand48(newseed) * n);
    }
#else /* DRAND48 */
    if (seed == 0)
	return current_time % n;	/* Suit yourself */
    else
	return seed % n;
#endif /* DRAND48 */
#endif /* RANDOM */
}

double
random_float()
{
#ifdef RANDOM
    return ldexp((double)random(), -32);
#else
    return drand48();
#endif
}
/*
 * The function time() can't really be trusted to return an integer.
 * But this game uses the 'current_time', which is an integer number
 * of seconds. To make this more portable, the following functions
 * should be defined in such a way as to retrun the number of seconds since
 * some chosen year. The old behaviour of time(), is to return the number
 * of seconds since 1970.
 *
 * alarm_time must never move backwards.
 */

void 
set_current_time() 
{
    static long alarm_base_time, alarm_last_time;
    struct timeval tv;

    (void)gettimeofday(&tv, NULL);

    if (alarm_base_time == 0)
	alarm_base_time = tv.tv_sec;

    alarm_time = (tv.tv_sec - alarm_base_time) * TIME_RES +
		 tv.tv_usec * TIME_RES / 1000000;
    if (alarm_time < alarm_last_time) {
	alarm_base_time -= (alarm_last_time - alarm_time) / TIME_RES;
	alarm_time += alarm_last_time - alarm_time + TIME_RES;
    }
    alarm_last_time = alarm_time;
    current_time = tv.tv_sec;
}

char *
time_string(int t)
{
    return ctime((time_t *)&t);
}
