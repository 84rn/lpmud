/*
 * /std/living/move.c
 *
 * This is a subpart of living.c
 *
 * All movement related routines are coded here.
 */
 
#include <stdproperties.h>
#include <macros.h>
#include <filter_funs.h>
#include <std.h>
#include <options.h>

static object	*following_me;		/* List of objects following me */

/*
 * Function name: move_reset
 * Description  : Reset the move module of the living object.
 */
static nomask void
move_reset()
{
    set_m_in(LD_ALIVE_MSGIN);
    set_m_out(LD_ALIVE_MSGOUT);
 
    set_mm_in(LD_ALIVE_TELEIN);
    set_mm_out(LD_ALIVE_TELEOUT);
    
    following_me = ({});
}
 
/*
 * Function name: move_living
 * Description:   Posts a move command for a living object somewhere. If you
 *                have a special exit like 'climb tree' you might want to
 *                use set_dircmd() and set it to 'tree' in the room to allow
 *                teammembers to follow their leader.
 * Arguments:     how:      The direction of travel, like "north".
 *                          "X" for teleportation
 *                          "M" if you write leave and arrive messages yourself.
 *                to_dest:  Destination
 *                dont_follow: A flag to indicate group shall not follow this
 *                          move if this_object() is leader
 *                no_glance: Don't look after move.
 *
 * Returns:       Result code of move:
 *                      0: Success.
 *
 *                      3: Can't take it out of it's container.
 *                      4: The object can't be inserted into bags etc.
 *                      5: The destination doesn't allow insertions of objects.
 *                      7: Other (Error message printed inside move() func)
 */
public varargs int
move_living(string how, mixed to_dest, int dont_follow, int no_glance)
{
    int    index, size;
    object *team, *drag, env, oldtp;
    string vb, com, msgout, msgin;
    mixed msg;
    
    oldtp = this_player();
    
    if (!objectp(to_dest))
    {
	msg = LOAD_ERR(to_dest);
	to_dest = find_object(to_dest);
    }
    
    if (stringp(msg))
    {
	if (!environment(this_object()))
	{
	    tell_object(this_object(), "PANIC Move error: " + msg);
	    to_dest = this_object()->query_default_start_location();
	    msg = LOAD_ERR(to_dest);
	    to_dest = find_object(to_dest);
	}
	else
	{
	    tell_object(this_object(), msg);
	    SECURITY->log_loaderr(to_dest, environment(this_object()), how,
		previous_object(), msg);
	    return 7;
	}
    }
 
    if (!to_dest->query_prop(ROOM_I_IS))
    {
	return 7;
    }

    if (!how)
    {
	return move(to_dest, 1);
    } 

    if (how == "M") 
    {
	msgin = 0;
	msgout = 0;
    } 
    else if (how == "X") 
    {
	msgin = this_object()->query_mm_in() + "\n";
	msgout = this_object()->query_mm_out() + "\n";
    }
    else if (query_prop(LIVE_I_SNEAK)) 
    {
	msgin = explode(this_object()->query_m_in(), ".")[0] +
	    " skradajac sie. " + "\n";
	msgout = "przemyka sie " + how + ".\n";
    }
    else 
    {
	msgin = this_object()->query_m_in() + " " + "\n";
	msgout = this_object()->query_m_out() + " " + how + ".\n";
    }

    /* Make us this_player() if we aren't already. */
    if (this_object() != this_player())
    {
	set_this_player(this_object());
    }

    if (env = environment(this_object()))
    {
	/* Update the last room settings. */
	add_prop(LIVE_O_LAST_ROOM, env);
	add_prop(LIVE_S_LAST_MOVE, (vb = query_verb()));
 
	/* Update the hunting status */
	this_object()->adjust_combat_on_move(1);

	/* Leave footprints. */
 	if (!env->query_prop(ROOM_I_INSIDE) &&
	    (env->query_prop(ROOM_I_TYPE) == ROOM_NORMAL) &&
	    !query_prop(LIVE_I_NO_FOOTPRINTS))
	{
	    env->add_prop(ROOM_AS_DIR, ({ how, query_rasa(PL_BIE) }) );
	}

	/* Report the departure. */                     
	if (msgout)
	{
	    saybb(QCIMIE(this_player(), PL_MIA) + " " + msgout);
	}
    }    

    if (!query_prop(LIVE_I_SNEAK))
    {
	remove_prop(OBJ_I_HIDE); 
    }
    else
    {
	remove_prop(LIVE_I_SNEAK);
    }

    if (index = move(to_dest)) 
    {
	return index;
    }

    if (msgin)
    {
	saybb(QCIMIE(this_player(), PL_MIA) + " " + msgin);
    }

    /* Take a look at the room you've entered, before the combat adjust.
     * Only interactive players bother to look. Don't waste our precious
     * CPU-time on NPC's.
     */
    if (interactive(this_object()) &&
	!no_glance)
    {
	this_object()->do_glance(this_object()->query_option(OPT_BRIEF));
    }
 
    /* See is people were hunting us or if we were hunting people. */
    this_object()->adjust_combat_on_move(0);
#if 0 
    if (sizeof(drag = query_prop(TEMP_DRAGGED_ENEMIES)))
    {
	index = -1;
	size = sizeof(drag);

	while(++index < size)
	{
	    tell_roombb(environment(drag[index]), QCIMIE(drag[index], PL_MIA) +
		" wychodzi, podazajac za " + QIMIE(this_object(), PL_NAR) + 
		".\n", ({drag[index]}), drag[index]);
	    drag[index]->move_living("M", to_dest);
	    tell_roombb(environment(drag[index]), QCIMIE(drag[index], PL_MIA) +
		" przychodzi, podazajac za " + QIMIE(this_object(), PL_NAR) + 
		".\n", ({drag[index], this_object()}), drag[index]);
	    tell_object(this_object(),
		drag[index]->query_Imie(this_object(), PL_MIA) +
		" przychodzi, podazajac za toba.\n");
	}
	remove_prop(TEMP_DRAGGED_ENEMIES);
    }
#endif
    
    if (!dont_follow &&
	stringp(how))
    {
	/* Command for the followers or team-mates. */
        if (!strlen(vb))
        {
	    if (sizeof(explode(how, " ")) == 1)
	    {
		com = how;
	    }
	    else
	    {
		com = "";
	    }
        }
        else if (com = env->query_dircmd())
	{
	    com = vb + " " + com;
	}
	else
	{
	    com = vb;
	}

	if (size = sizeof(team = query_team()))
	{
	    /* Move the present team members. */
	    index = -1;
	    while(++index < size)
	    {
		if ((environment(team[index]) == env) &&
		    this_object()->check_seen(team[index]))
		{
		    team[index]->follow_leader(com);
		}
	    }
	}
	
	if (size = sizeof(following_me))
	{
	    /* Move the present people following me. */
	    while(--size >= 0)
	    {
	        if (!following_me[size])
	        {
	             following_me = exclude_array(following_me, size, size);
	             continue;
	        }
	        if ((environment(following_me[size]) == env) &&
		    this_object()->check_seen(following_me[size]))
		{
		    following_me[size]->hook_follow_me(com);
		}
		else
		{
		    following_me[size]->hook_follow_you_lost_me();
		}
	    }
	}
    }

    /* Only reset this_player() if we weren't this_player already. */
    if (oldtp != this_object())
    {
	set_this_player(oldtp);
    }

    return 0;
}

/*
 * Function name: follow_leader
 * Description  : If the leader of the team moved, follow him/her.
 * Arguments    : string com - the command to use to follow the leader.
 *
 * WARNING      : This function makes the person command him/herself. This
 *                means that when a wizard is in a team, the team leader can
 *                force the wizard to perform non-protected commands. Wizard
 *                commands cannot be forced as they are protected.
 */
public void
follow_leader(string com) 
{
    /* Only accept this call if we are called from our team-leader. */
    if (previous_object() != query_leader())
    {
	return;
    }

    set_this_player(this_object());

    /* We use a call_other since you are always allowed to force yourself.
     * That way, we will always be able to follow our leader.
     */
    this_object()->command("$" + com);
}

/*
 * Function name: reveal_me
 * Description  : Reveal me unintentionally.
 * Arguments    : int tellme - true if we should tell the player.
 * Returns      : int - 1 : He was hidden, 0: He was already visible.
 */
public nomask int
reveal_me(int tellme)
{
    object *list, *list2;
    int index, size;

    if (!query_prop(OBJ_I_HIDE))
    {
	return 0;
    }

    if (tellme)
    {
	this_object()->catch_msg("Nie jestes juz schowan" +
	    koncowka("y", "a") + ".\n");
    }

    list = FILTER_LIVE(all_inventory(environment()) - ({ this_object() }) );
    list2 = FILTER_IS_SEEN(this_object(), list);
    list -= list2;

    remove_prop(OBJ_I_HIDE);

    list = FILTER_IS_SEEN(this_object(), list);

    index = -1;
    size = sizeof(list);
    while(++index < size)
	tell_object(list[index], "Ku twojemu zdumieniu, " +
	    this_object()->query_imie(list[index], PL_MIA) + " pojawil" +
	    koncowka("", "a") + " sie nagle tuz obok ciebie!\n");

    index = -1;
    size = sizeof(list2);
    while(++index < size)
	tell_object(list2[index], this_object()->query_Imie(list2[index],
	    PL_MIA) + " wychodzi z ukrycia.\n");

    return 1;
}

public void
add_following(object ob)
{
    if (ob)
        following_me = (following_me - ({ ob })) + ({ ob });
}

public void
remove_following(object ob)
{
    following_me -= ({ ob });
}

public object *
query_following()
{
    return following_me + ({});
}
