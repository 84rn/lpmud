#include <signal.h>
#include <stdio.h>

#include "config.h"
#include "lint.h"
#include "interpret.h"

void init_signals(void);
void deliver_signals(void);

static struct sig_disp
{
    int sig;
    char *name;
} disp[] = {
    {SIGHUP, "HUP"},
    {SIGINT, "INT"},
/*    {SIGQUIT, "QUIT"},*/
    {SIGTERM, "TERM"},
    {SIGUSR1, "USR1"},
    {SIGUSR2, "USR2"},
    {SIGTSTP, "TSTP"},
    {SIGCONT, "CONT"},
    {0, "UNKNOWN"},
};

#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__)
static sig_atomic_t pending_signals = 0;
#else
static int pending_signals = 0;
#endif

static void
sig_handler(int sig)
{
    int i;

#if defined(SYSV) || defined(linux)
    (void)signal(sig, sig_handler);
#endif
    for (i = 0; disp[i].sig && disp[i].sig != sig; i++)
	;
    
    pending_signals |= 1 << i;
}

void
init_signals()
{
    int i;

    for (i = 0; disp[i].sig; i++)
	(void)signal(disp[i].sig, sig_handler);
}

void
deliver_signals()
{
    int i;

    for (i = 0; pending_signals; i++)
    {
	if (pending_signals & (1 << i))
	{
	    pending_signals &= ~(1 << i);
	    /* deliver the signal */
	    push_string(disp[i].name, STRING_CSTRING);
	    (void)apply_master_ob(M_EXTERNAL_SIGNAL, 1);
	}
    }
}
