/* (c) Copyright by Anders Chrigstroem 1993, All rights reserved */
/* Permission is granted to use this source code and any executables
 * created from this source code as part of the CD Gamedriver as long
 * as it is not used in any way whatsoever for monetary gain. */

#include <memory.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>

#include "config.h"
#include "lint.h"
#include "mstring.h"
#include "interpret.h"
#include "exec.h"
#include "object.h"
#include "mudstat.h"
#include "main.h"
#include "simulate.h"
#include "call_out.h"

void init_call_out(void);
void call_out(struct timeval *);

#ifdef USE_SWAP
#define FUNC_NAME(ob, cop)\
(access_program(ob->prog->inherit[cop->inherit].prog),\
 ob->prog->inherit[cop->inherit].prog->functions[cop->fun].name)
#else
#define FUNC_NAME(ob, cop)\
(ob->prog->inherit[cop->inherit].prog->functions[cop->fun].name)
#endif
extern char *string_copy(char *);
extern int eval_cost, s_flag;
extern int alarm_time;
extern struct svalue *sp;
extern struct object *previous_ob;

struct call {
    struct closure *func;
    int reload;
    unsigned int when;
    int id;
    struct call *next;
    struct object *command_giver;
};

static struct object *call_outs[NUM_SLOTS];
static unsigned int last;

int num_call;
int call_out_size;
static int call_id;

#if 0
/* ARGSUSED */
static void
timer_alarm(int s)
{
#ifdef USE_SWAP
    static int acc_swap_out_prog[60];
    static int acc_swap_out_obj[60];
    static int acc_swap_in_prog[60];
    static int acc_swap_in_obj[60];
    static int acc2_swap_out_prog[60];
    static int acc2_swap_out_obj[60];
    static int acc2_swap_in_prog[60];
    static int acc2_swap_in_obj[60];
    extern int swap_out_obj_sec, swap_out_obj_min, swap_out_obj;
    extern int swap_out_prog_sec, swap_out_prog_min, swap_out_prog;
    extern int swap_in_obj_sec, swap_in_obj_min, swap_in_obj;
    extern int swap_in_prog_sec, swap_in_prog_min, swap_in_prog;
    extern int swap_out_obj_hour, swap_in_obj_hour;
    extern int swap_out_prog_hour, swap_in_prog_hour;

    if (now % TIME_RES == 0)
    {
	int sec = now / TIME_RES % 60;

	swap_out_obj_min = swap_out_obj_min - acc_swap_out_obj[sec] +
	    (acc_swap_out_obj[sec] = swap_out_obj_sec = swap_out_obj);
	swap_out_obj = 0;

	swap_out_prog_min = swap_out_prog_min - acc_swap_out_prog[sec] +
	    (acc_swap_out_prog[sec] = swap_out_prog_sec = swap_out_prog);
	swap_out_prog = 0;


	swap_in_obj_min = swap_in_obj_min - acc_swap_in_obj[sec] +
	    (acc_swap_in_obj[sec] = swap_in_obj_sec = swap_in_obj);
	swap_in_obj = 0;

	swap_in_prog_min = swap_in_prog_min - acc_swap_in_prog[sec] +
	    (acc_swap_in_prog[sec] = swap_in_prog_sec = swap_in_prog);
	swap_in_prog = 0;
	if (now % (TIME_RES * 60) == 0)
	{
	    int min = now / (TIME_RES * 60) % 60;
	    
	    swap_out_obj_hour = swap_out_obj_hour - acc2_swap_out_obj[min] +
		(acc2_swap_out_obj[min] = swap_out_obj_min);
	    
	    swap_out_prog_hour = swap_out_prog_hour - acc2_swap_out_prog[min] +
		(acc2_swap_out_prog[min] = swap_out_prog_min);


	    swap_in_obj_hour = swap_in_obj_hour - acc2_swap_in_obj[min] +
		(acc2_swap_in_obj[min] = swap_in_obj_min);
	    
	    swap_in_prog_hour = swap_in_prog_hour - acc2_swap_in_prog[min] +
		(acc2_swap_in_prog[min] = swap_in_prog_min);
	}
    }
#endif
    now++;
}
#endif

void
init_call_out()
{
    call_id = 1;
    last = 0;
#if TIME_RES * 60 >= NUM_SLOTS
# error There must be more slots in the call_out table
#endif
#if 0
    (void)signal(SIGALRM, timer_alarm);
    (void)ualarm(1000000/TIME_RES, 1000000/TIME_RES);
#endif
}

int
num_call_outs(struct object *ob)
{
    struct call *c;
    int num_co;

    for (num_co = 0, c = ob->call_outs; c;
	 num_co++, c = c->next) ;
    return num_co;
}

void
call_out_swap_objects(struct object *ob1, struct object *ob2)
{
    int slot;
    struct object **obp;
    struct call *tmp_co;
    
    if (ob1->call_outs)
    {
	slot = ob1->call_outs->when & (NUM_SLOTS - 1);
	for (obp = &(call_outs[slot]); *obp && *obp != ob1;
	     obp = &(*obp)->next_call_out) ;
	if (!*obp)
	    fatal("Error in callout list.\n");
	*obp = ob1->next_call_out;
    }
    if (ob2->call_outs)
    {
	slot = ob2->call_outs->when & (NUM_SLOTS - 1);
	for (obp = &(call_outs[slot]); *obp && *obp != ob2;
	     obp = &(*obp)->next_call_out) ;
	if (!*obp)
	    fatal("Error in callout list.\n");
	*obp = ob2->next_call_out;
    }

    tmp_co = ob1->call_outs;
    ob1->call_outs = ob2->call_outs;
    ob2->call_outs = tmp_co;
    
    if (ob1->call_outs)
    {
	slot = ob1->call_outs->when & (NUM_SLOTS - 1);
	for (obp = &(call_outs[slot]); *obp &&
	     (*obp)->call_outs->when < ob1->call_outs->when;
	     obp = &(*obp)->next_call_out) ;
	ob1->next_call_out = *obp;
	*obp = ob1;
	
    }
    if (ob2->call_outs)
    {
	slot = ob2->call_outs->when & (NUM_SLOTS - 1);
	for (obp = &(call_outs[slot]); *obp &&
	     (*obp)->call_outs->when < ob2->call_outs->when;
	     obp = &(*obp)->next_call_out) ;
	ob2->next_call_out = *obp;
	*obp = ob2;
    }
    
}

static void
insert_call_out(struct object *ob, struct call *cop)
{
    struct call **copp;
    struct object **obp;
    int slot;
    
    
    /* Insert the callout */
    for (copp = &ob->call_outs; *copp && (*copp)->when < cop->when;
	 copp = &(*copp)->next) ;
    cop->next = *copp;
    *copp = cop;

    /* Are we inserting the callout first in the list? */
    if (copp == &ob->call_outs)
    {
	/* Did the object allready have some callouts */
	if (cop->next)
	{
	    slot = cop->next->when & (NUM_SLOTS - 1);
	    for (obp = &(call_outs[slot]); *obp && *obp != ob;
		 obp = &(*obp)->next_call_out) ;
	    if (!*obp)
		fatal("Error in callout list.\n");
	    *obp = ob->next_call_out;
	}
	
	/* Insert the object */
	slot = ob->call_outs->when & (NUM_SLOTS - 1);
	for (obp = &(call_outs[slot]);
	     *obp && (*obp)->call_outs->when < ob->call_outs->when;
	     obp = &(*obp)->next_call_out) ;
	ob->next_call_out = *obp;
	*obp = ob;
    }
}

static void
free_call(struct call *cop)
{
    free_closure(cop->func);
    if (cop->command_giver)
	free_object(cop->command_giver, "free_call");
    free((char *)cop);
    num_call--;
    call_out_size -= sizeof(struct call);
}

int
new_call_out(struct closure *fun, int delay, int reload)
{
    struct call *cop;

    if (delay <= 0)
	delay = 1;

    if (reload <= 0)
	reload = 0;

    if (num_call_outs(current_object) >= MAX_CALL_OUT) {
	delete_all_calls(current_object);
	error("too many alarms in object\n");
    }

    cop = (struct call *)xalloc(sizeof(struct call));
    num_call++;
    call_out_size += sizeof(struct call);

    cop->func = fun;
    INCREF(fun->ref);
    cop->reload = reload;
    cop->when = alarm_time + delay;
    
    if (cop->when < last)
	fatal("Error in callout!\n");
    cop->id = call_id++;
    cop->command_giver = command_giver;
    if (command_giver)
	INCREF(command_giver->ref);
    insert_call_out(current_object, cop);
    return cop->id;
}

void
delete_call(struct object *ob, int call_ident)
{
    struct call **copp, *cop;
    struct object **pob;

    for(copp = &ob->call_outs; *copp && (*copp)->id != call_ident;
	copp = &(*copp)->next) ;
    if (!*copp)
	return;
    
    cop = *copp;
    *copp = (*copp)->next;

    /* Is it the first callout in the list? */
    if (copp == &ob->call_outs)
    {
	int slot;
	
	slot = cop->when & (NUM_SLOTS - 1);
	for (pob = &call_outs[slot]; *pob && *pob != ob;
	     pob = &(*pob)->next_call_out) ;
	if (!*pob)
	    fatal("Corrupted callout list.\n");
	*pob = ob->next_call_out;

	/* Are there any callouts left? */
	if (ob->call_outs)
	{
	    slot = ob->call_outs->when & (NUM_SLOTS - 1);
	    for (pob = &call_outs[slot];
		 *pob && (*pob)->call_outs->when < ob->call_outs->when;
		 pob = &(*pob)->next_call_out) ;
	    ob->next_call_out = (*pob);
	    *pob = ob;
	}
    }
    free_call(cop);
}

void
delete_all_calls(struct object *ob)
{
    int slot;
    struct object **pob;
    struct call *next, *cop;
    
    if (!(ob->call_outs))
	return;
    slot = ob->call_outs->when & (NUM_SLOTS - 1);
    for (pob = &call_outs[slot]; *pob && *pob != ob;
	 pob = &(*pob)->next_call_out);
    if (!*pob)
	fatal("Corrupt callout list.\n");

    *pob = ob->next_call_out;
    for(cop = ob->call_outs; cop; cop = next)
    {
	next = cop->next;
	free_call(cop);
    }
    ob->call_outs = 0;
}

/*
  0 : call id;
  1 : function;
  2 : time left;
  3 : reload time;
  4 : argument;
*/
struct vector *
get_call(struct object *ob, int call_ident)
{
    struct vector *val;
    struct call *cop;
    
    for(cop = ob->call_outs; cop && cop->id != call_ident; cop = cop->next)
	;
    if (!cop)
	return 0;
    
    val = allocate_array(5);
    
    val->item[0].type = T_NUMBER;
    val->item[0].u.number = cop->id;
    
    val->item[1].type = T_STRING;
    val->item[1].string_type = STRING_MSTRING;
    val->item[1].u.string = make_mstring(getclosurename(cop->func));
    
    val->item[2].type = T_FLOAT;
    val->item[2].u.real = ((double)cop->when - alarm_time) / TIME_RES;
    if (val->item[2].u.real < 0.0)
	val->item[2].u.real = 0.0;
    val->item[3].type = T_FLOAT;
    val->item[3].u.real = ((double)cop->reload) / TIME_RES;
    
    val->item[4].type = T_POINTER;
    val->item[4].u.vec = cop->func->funargs;
    INCREF(cop->func->funargs->ref);
    
    return val;
}
    
/*
  0 : call id;
  1 : function;
  2 : time left;
  3 : reload time;
  4 : argument;
*/
struct vector *
get_calls(struct object *ob)
{
    struct vector *ret;
    struct call *cop;
    int i;

    for(i = 0, cop = ob->call_outs; cop; i++, cop = cop->next)
	;
    ret = allocate_array(i);
    
    for(i = 0, cop = ob->call_outs; cop; i++, cop = cop->next)
    {
	struct vector *val;
	
	val = allocate_array(5);

	val->item[0].type = T_NUMBER;
	val->item[0].u.number = cop->id;
	
	val->item[1].type = T_STRING;
	val->item[1].string_type = STRING_MSTRING;
	val->item[1].u.string = make_mstring(getclosurename(cop->func));
	
	val->item[2].type = T_FLOAT ;
	val->item[2].u.real = ((double)cop->when - alarm_time) / TIME_RES;
	if (val->item[2].u.real < 0.0)
	    val->item[2].u.real = 0.0;
	
	val->item[3].type = T_FLOAT;
	val->item[3].u.real = ((double)cop->reload) / TIME_RES;
	
	val->item[4].type = T_POINTER;
	val->item[4].u.vec = cop->func->funargs;
	INCREF(cop->func->funargs->ref);
	
	ret->item[i].type = T_POINTER;
	ret->item[i].u.vec = val;
    }
    return ret;
}

int current_call_out_id;
struct object *current_call_out_object;

int inhibitcallouts = 0;	/* Stop call-out processing. */

/*
 * last_alarm is at which slot we last executed an alarm
 */
static void
next_call_out(unsigned int last_alarm, struct timeval *tvp)
{
    unsigned int end, slot;

    /*
     * We only want to scan 60 seconds into the future
     */
    end = (alarm_time + 60 * TIME_RES) & (NUM_SLOTS - 1);
    slot = last_alarm & (NUM_SLOTS - 1);
    do {
	/*
	 * We found an alarm...
	 */
	if (call_outs[slot]) {
	    /*
	     * We found an alarm that was scheduled to run already
	     */
	    if (call_outs[slot]->call_outs->when < alarm_time) {
		tvp->tv_sec = 0;
		tvp->tv_usec = 0;
		break;
	    }
	    /*
	     * We found an alarm that is scheduled to run within the next
	     * 60 seconds
	     */
	    else if (call_outs[slot]->call_outs->when - alarm_time < TIME_RES * 60) {
		tvp->tv_sec = (call_outs[slot]->call_outs->when - alarm_time) / TIME_RES;
		tvp->tv_usec = (call_outs[slot]->call_outs->when - alarm_time) % TIME_RES * 1000000 / TIME_RES;
		break;
	    }
	}
	slot = (slot + 1) & (NUM_SLOTS - 1);
    } while (slot != end);
}

void
call_out(struct timeval *tvp)
{
    static struct call *cop, **copp;
    struct object *ob, **obp;
    struct gdexception exception_frame;
    extern struct object *command_giver;
    extern struct object *current_interactive;
    volatile int sum_eval = 0;
    volatile int sum_time = 0;
    volatile int sum_ptime = 0;
    volatile int num_done = 0;
    char caodesc[100];
    struct closure *thefun;

    tvp->tv_sec = 60;
    tvp->tv_usec = 0;

    if (inhibitcallouts)
	return;

    if (last >= alarm_time)
    {
	next_call_out(last, tvp);
	return;
    }

    exception_frame.e_exception = exception;
    exception_frame.e_catch = 0;

    current_interactive = 0;
    for (; last <= alarm_time; last++)
    {
	int slot, nslot;
	
	slot = last & (NUM_SLOTS - 1);
	
	while(call_outs[slot] &&
	      call_outs[slot]->call_outs->when <= last)
	{
	    /* Extract the object and the callout */
	    ob = call_outs[slot];
	    call_outs[slot] = ob->next_call_out;
	    cop = ob->call_outs;
	    ob->call_outs = cop->next;
	    
	    if (cop->reload > 0) /* Reschedule the callout */
	    {
		current_call_out_id = cop->id;
		cop->when = alarm_time + cop->reload;
		if (cop->when < last)
		    fatal("Error in callouts!\n");
		
		for (copp = &ob->call_outs;
		     *copp && (*copp)->when < cop->when;
		     copp = &(*copp)->next) ;
		cop->next = *copp;
		*copp = cop;
	    }
	    else
		current_call_out_id = 0;
	    
	    if (ob->call_outs) /* Reinsert the object */
	    {
		nslot = ob->call_outs->when & (NUM_SLOTS - 1);
		for (obp = &call_outs[nslot];
		     *obp && (*obp)->call_outs->when < ob->call_outs->when;
		     obp = &(*obp)->next_call_out) ;
		ob->next_call_out = *obp;
		*obp = ob;
	    }
	    
	    /* do the call */
	    if (cop->command_giver &&
		cop->command_giver->flags & O_DESTRUCTED)
	    {
		free_object(cop->command_giver, "call_out");
		cop->command_giver = 0;
	    }
	    
	    
	    if (cop->command_giver &&
		cop->command_giver->flags & O_ENABLE_COMMANDS)
		command_giver = cop->command_giver;
	    else if (ob->flags & O_ENABLE_COMMANDS)
		command_giver = ob;
	    else
		command_giver = 0;
	    if (s_flag)
		reset_mudstatus();
	    eval_cost = 0;

	    current_call_out_object = current_object = ob;
	    INCREF(current_call_out_object->ref);
	    
	    thefun = cop->func;
	    INCREF(thefun->ref); /* keep a reference */
	    if (cop->reload <= 0)
		free_call(cop);
	    if (thefun->funobj != ob)
		previous_ob = ob;
	    
	    if (setjmp(exception_frame.e_context)) {
		extern void clear_state(void);

		clear_state();
		debug_message("Error in call out.\n");
		if (current_call_out_id)
		{
		    /*access_program(ob->prog->inherit[inh].prog);*/
		    debug_message("Call out %s turned off in %s.\n", getclosurename(thefun),
				  current_call_out_object->name);
		    delete_call(current_call_out_object, current_call_out_id);
		}
		free_closure(thefun); /* and kill the extra reference */
		free_object(current_call_out_object, "call_out");
	    }
	    else
	    {
		extern void update_alarm_av(void);

		exception = &exception_frame;
		update_alarm_av();
		(void)call_var(0, thefun);
		pop_stack();
		
		if (s_flag)
		{
		    num_done++;
		    sum_eval += eval_cost;
		    sum_time += get_millitime();
		    sum_ptime += get_processtime();
		    (void)sprintf(caodesc,"CAO:%s", getclosurename(thefun));
		    print_mudstatus(caodesc, eval_cost, 
				    get_millitime(), get_processtime());
		}
		free_closure(thefun); /* and kill the extra reference */
		free_object(current_call_out_object, "call_out");
	    }
	    previous_ob = 0;
	}
    }
    exception = exception_frame.e_exception;
    next_call_out(last, tvp);
    if (s_flag && num_done)
    {
	reset_mudstatus();
	(void)sprintf(caodesc,"Call_out (%d)", num_done);
	print_mudstatus(caodesc, sum_eval, sum_time, sum_ptime);
    }
}

#ifdef DEBUG
void
count_ref_from_call_outs()
{
    int i;
    struct object *ob;
    struct call *cop;
    
    for(i = 0; i < NUM_SLOTS; i++)
	for(ob = call_outs[i]; ob; ob = ob->next_call_out)
	    for(cop = ob->call_outs; cop; cop = cop->next)
	    {
#if 0
		switch(cop->v->item->type)
		{
		case T_POINTER:
		    cop->v->item->u.vec->extra_ref++;
		    break;
		case T_OBJECT:
		    cop->v->item->u.ob->extra_ref++;
		    break;
		}
#endif
		if (cop->command_giver)
		    cop->command_giver->extra_ref++;
	    }
}
#endif
