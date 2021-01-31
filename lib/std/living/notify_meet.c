/* 
 * /std/living/notify_meet.c
 */

static int last_met_interactive;  /* Time when we met an interactive */
static string *notify_meet_interactive; /* List of functions to call */

notify_meet_reset()
{
    last_met_interactive = time();
    notify_meet_interactive = ({});
}

notify_meet_init()
{
    int il;

    if (this_player() && interactive(this_player()))
    {
	last_met_interactive = time();
	if (sizeof(notify_meet_interactive))
	    for (il = 0; il < sizeof(notify_meet_interactive); il++)
		process_value(notify_meet_interactive[il], 1);
    }

}

/*
 * Function name: query_last_met_interactive
 * Description:    Gives the time of last meeting with an interactive player
 *                 If an interactive player is in the same environment as
 *	           this living the current time is returned.
 * Returns:        Time is seconds since... oh well you know...
 */
public int
query_last_met_interactive()
{
    object *ob;
    int il;


    if (environment(this_object()))
	ob = all_inventory(environment(this_object()));
    else
	ob = ({});

    for (il = 0; il < sizeof(ob); il++)
	if (objectp(ob[il]) && interactive(ob[il]))
	    return time();

    return last_met_interactive;
}

/*
 * Function name: add_notify_meet_interactive
 * Description:    Adds a function to call when this living meets an
 *                 interactive player.
 * Arguments:	   func: The function, can be on VBFC format
 */
public void
add_notify_meet_interactive(string func)
{
    if (!pointerp(notify_meet_interactive))
	notify_meet_interactive = ({});
    notify_meet_interactive += ({ func });
}

/*
 * Function name: remove_notify_meet_interactive
 * Description:    Removes a function to call when this living meets an
 *                 interactive player.
 * Arguments:	   func: The function, can be on VBFC format
 */
public void
remove_notify_meet_interactive(string func)
{
    int pos;

    if (pos = (member_array(func, notify_meet_interactive) >= 0))
	notify_meet_interactive = 
	    exclude_array(notify_meet_interactive, pos, pos);
}

/*
 * Function name: query_notify_meet_interactive
 * Description:    Give the list of functions to call when this living
 *                  meets an interactive player.
 */
public string *
query_notify_meet_interactive()
{
    return notify_meet_interactive;
}
