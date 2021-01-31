/* 
 * /std/living.c
 *
 * Contains all routines relating to living objects of any kind.
 *
 * If you are going to copy this file, in the purpose of changing
 * it a little to your own need, beware:
 *
 * First try one of the following:
 *
 * 1. Do clone_object(), and then configure it. This object is specially
 *    prepared for configuration.
 *
 * 2. If you still is not pleased with that, create a new empty
 *    object, and make an inheritance of this object on the first line.
 *    This will automatically copy all variables and functions from the
 *    original object. Then, add the functions you want to change. The
 *    original function can still be accessed with '::' prepended on the name.
 *
 * The maintainer of this LPmud might become sad with you if you fail
 * to do any of the above. Ask other wizards if you are doubtful.
 *   
 * The reason of this, is that the above saves a lot of memory.
 */
#pragma save_binary
#pragma strict_types

inherit "/std/container";

#include <std.h>
#include <log.h>		/* What shall we log */
#include <macros.h>
#include <stdproperties.h>
#include <formulas.h>		/* All game formulas, ex: combat */
#include <language.h>
#include <living_desc.h>	/* All text constants */

#include "/std/living/living.h"
#include "/std/living/savevars.c"
#include "/std/living/combat.c"
#include "/std/living/gender.c"
#include "/std/living/stats.c"
#include "/std/living/carry.c"
#include "/std/living/heart_beat.c"
#include "/std/living/drink_eat.c"
#include "/std/living/cmdhooks.c"
#include "/std/living/description.c"
#include "/std/living/move.c"
#include "/std/living/tasks.c"
#include "/std/living/wizstat.c"
#include "/std/living/spells.c"
#include "/std/living/possess.c"
#include "/std/living/notify_meet.c"

#include <debug.h>

static int tell_active_flag;      /* Flag to check in catch_msg() */

/*
 * Function name: query_init_master
 * Description:   Should return true if create_living shall be called
 *                in the master object of a living.
 */
public int
query_init_master()
{
    return 0;
}

/*
 * Function name: create_container
 * Description:   Create the living object. (constructor)
 */
nomask void
create_container()
{
    int g;
    
    if (!(IS_CLONE ||
	  query_init_master()))
    {
	return;
    }

    save_vars_reset();
    skill_extra_map_reset();
    notify_meet_reset();
    gender_reset();
    spells_reset();
    ss_reset(); 
    carry_reset();
    drink_eat_reset();
    move_reset(); 

    add_prop(LIVE_I_IS, 1);
    add_prop(CONT_I_ATTACH, 1);
    add_prop(CONT_I_HIDDEN, 1);
    add_prop(CONT_I_REDUCE_WEIGHT, 200);
    add_prop(CONT_I_REDUCE_VOLUME, 200);

    create_living();

    g = this_object()->query_gender();
    if (g < 0 && !interactive(this_object()))
        set_gender(G_MALE);
        
    /* An NPC has full hitpoints, full mana and full fatigue by default. */
    if (this_object()->query_npc())
    {
        refresh_living();
        add_gender_names();
    }

    if (!geteuid(this_object()))   /* Get our own uid if not prepared */
    { 
	setuid(); 
	seteuid(getuid(this_object()));
    }

    combat_reset();

    enable_commands();
    cmdhooks_reset();
}

/*
 * Function name: create_living
 * Description:   Create the living object. (standard)
 */
public void
create_living()
{
    ustaw_nazwe(({"zyjatko", "zyjatka", "zyjatku", "zyjatko", "zyjatkiem",
                  "zyjatku"}),
                ({"zyjatka", "zyjatek", "zyjatkom", "zyjatka", "zyjatkami",
                  "zyjatkach"}), PL_NIJAKI_NOS);
}

/*
 * Function name: reset_container
 * Description:   Reset the living object. 
 */
public nomask void
reset_container() 
{ 
    reset_living(); 
}

/*
 * Function name: reset_living
 * Description:   Reset the living object. (standard)
 */
public void
reset_living() 
{ 
    ::reset_container(); 
}

/*
 * Function name: init
 * Description:   Tells us of new players in our neigbourhood
 */
nomask void
init()
{
    ::init();
    combat_init();
    start_heart();
    notify_meet_init();
    this_object()->init_living();
}

/*
 * Function name: encounter
 * Description:   Called when encountering an object
 */
public void
encounter(object obj)
{
    obj->init();
}

/*
 * Function name:	command
 * Description:		Makes the living object execute a command
 * Arguments:		cmd: String containing the command
 * Returns:		eval_cost or '0' if unsuccessfull
 */
public int
command(string cmd)
{
    return efun::command(cmd);
}

/*
 * Function name: can_see_in_room
 * Description  : This function will return whether this object can see
 *                in the room he/she is in. It is used from filters, among
 *                other things.
 * Returns      : int 1/0 - the result from CAN_SEE_IN_ROOM()
 */
public nomask int
can_see_in_room()
{
    return CAN_SEE_IN_ROOM(this_object());
}

/*
 * Function name: catch_msg
 * Description:   This function is called for every normal message sent
 *                to this living.
 * Arguments:     str:       Message to tell the player
 *                from_player: The object that generated the message
 *			     This is only valid if the message is on the
 *			     form ({ "met message", "unmet message",
 *				     "unseen message" })
 */
public void 
catch_msg(mixed str, object from_player)
{
    if (!query_ip_number(this_object()) &&
	!query_tell_active())
    {
	return;
    }

    if (pointerp(str))
    {
"/d/Wiz/silvathraec/private/say_logger"->log_say("Obsolete syntax - str is an array.");

	if (!from_player)
	{
	    from_player = this_player();
	}
	if ((sizeof(str) > 2) &&
	    (!CAN_SEE_IN_ROOM(this_object()) ||
		!CAN_SEE(this_object(), from_player)))
	{
	    write_socket(str[2]);
	}
	else if (this_object()->query_met(from_player))
	{
	    write_socket(str[0]);
	}
	else
	{
	    write_socket(str[1]);
	}
    }
    else
    {
	write_socket(process_string(str, 1));
    }
}

/*
 * Function name: remove_object
 * Description:   Destruct this object, but check for possessed first
 */
public int
remove_object()
{
    possessed_remove();
    if (query_combat_object())
	catch(query_combat_object()->remove_object());
    return ::remove_object();
}

/*
 * Function name: modify_command
 * Description:	 This is here so that a possessing wizard will get commands.
 *    Technically, it should be part of /std/living/possess.c, but since
 *    the lfun is more general, it is left here.
 */
string
modify_command(string cmd)
{
    return cmd;
}

/*
 * Function name: local_cmd()
 * Description:   Return a list of all add_actioned commands
 */
nomask string *
local_cmd()
{
    return get_localcmd();
}

/*
 * Function name: set_tell_active
 * Description:   Sets the tell_active_flag so that catch_msg() will send
 *                all messages to us.
 * Arguments:     i - a number, 1 or 0, on or off
 */
void set_tell_active(int i) { tell_active_flag = i; }

/*
 * Functione name: query_tell_active
 * Description:    Query the tell_active_flag
 * Returns:        The flag
 */
int query_tell_active() { return tell_active_flag; }
