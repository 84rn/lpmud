/*
 * /d/Genesis/obj/statue.c
 *
 * This is the statue room. Here we keep all players who linkdied.
 */

#pragma no_clone
#pragma no_inherit
#pragma save_binary
#pragma strict_types

inherit "/std/room";

#include <macros.h>
#include <std.h>
#include <stdproperties.h>

#define BSN(message) (break_string(message, 75) + "\n")
#define STATUE_ROOM  "_statue_room"
#define EDITOR_ID    "_editor_"

/*
 * These properties are used in move_living and therefore we save them
 * when a player goes linkdeath.
 */
static string *props_to_save = ({ LIVE_O_LAST_ROOM, LIVE_S_LAST_MOVE });

mapping places;

#define DEATH_MESSAGE ([                                    \
       "human"  : " passes into limbo.\n",                  \
       "hobbit" : " enters the world of dreams.\n",         \
       "elf"    : " travels to the land of bliss.\n",       \
       "dwarf"  : " returns to the bowels of the earth.\n", \
       "gnome"  : " is lost in the shadows.\n",             \
       "goblin" : " is cast into the void.\n" ])

#define REVIVE_MESSAGE ([                                   \
       "human"  : " returns from limbo.",                   \
       "elf"    : " returns from the land of bliss.",       \
       "hobbit" : " awakens from the world of dreams.",     \
       "dwarf"  : " ascends from the depths of the earth.", \
       "gnome"  : " finds the way back from the shadows.",  \
       "goblin" : " returns from the void." ])

/*
 * Function name: create_room
 * Description  : Called to create the room.
 */
public nomask void
create_room() 
{
    set_short("This is the statue room");
    set_long("You are in a part of the Keepers' art collection. " +
	"This is where they keep the statues of some unfortunate " +
	"adventurers. Rumours have it that the Keepers once fought " +
	"a great battle against revolting Archwizards here and " +
	"that the room still contains strong magic from that time. " +
	"Some visitors claim that they have seen statues turn alive " +
	"and disappear!");

    add_exit("/d/Standard/start/church", "poludnie", "@@may_leave@@");

    change_prop(ROOM_I_LIGHT, 100);
    places = ([]);
}

/*
 * Function name: may_leave
 * Description  : To prevent linkdead players from leaving this room, we
 *                check that in the exit.
 * Returns      : int 1/0 - false if the player may leave.
 */
nomask public int
may_leave()
{
    if (!interactive(this_player()))
    {
	write("Only interactive players may use this exit.\n");
	return 1;
    }

    return 0;
}

/*
 * Function name: remove_player
 * Description  : Can be called to remove a player from the room if it has
 *                been in here too long.
 * Arguments    : object ob - the player to remove.
 */
static nomask void
remove_player(object ob)
{
    if (ob && present(ob))
    {
	places = m_delete(places, ob);
        ob->remove_object();
    }
}

/*
 * Function name: die
 * Description  : This function is called from SECURITY when a player
 *                linkdies. We more him to the statue room and do some
 *                additional stuff.
 * Arguments    : object ob - the object that linkdies.
 */
public nomask void 
die(object ob) 
{
    int index, size, id_alarm;
    string env;

    /* If Armageddon is active when the player looses his/her link, we
     * make him quit instantly.
     */
    if (ARMAGEDDON->shutdown_active())
    {
	set_this_player(ob);
	ob->quit();

	return;
    }

    /* Just to be sure */
    if (!objectp(environment(ob)))
    {
	ob->quit();
	return;
    }

/*
    if (member_array(ob->query_race_name(), m_indices(DEATH_MESSAGE)) < 0)
    {
	tell_room(environment(ob), QCTNAME(ob) + " goes link dead.\n",
	    ({ ob }) );
    }
    else
    {
    	tell_room(environment(ob),
	    QCTNAME(ob) + DEATH_MESSAGE[ob->query_race_name()], ({ ob }) );
    }
*/

    tell_roombb(environment(ob), QCIMIE(ob, PL_MIA) + 
        " traci kontakt z rzeczywistoscia.\n", ({ ob }));


    env = file_name(environment(ob));

    /* save the props altered in move_living() */
    index = -1;
    size = sizeof(props_to_save);
    while(++index < size)
    {
        ob->add_prop(STATUE_ROOM + props_to_save[index],
	    ob->query_prop_setting(props_to_save[index]));
    }

    tell_room(this_object(), "Ze chmury dymu wylania sie statua " +
	QIMIE(ob, PL_DOP) + ".\n ");

    ob->move_living("M", this_object(), 1);
    ob->save_me(1);
    id_alarm = set_alarm(1800.0, 0.0, "remove_player", ob); /* 30 minutes */

    places += ([ ob : ({ env, id_alarm }) ]);
    
    if (objectp(ob = present(EDITOR_ID, ob)))
    {
	ob->linkdie();
    }
}

/*
 * Function name: revive
 * Description  : This function is called from the login object if a player
 *                revives from linkdeath. The player object is moved into
 *                his original room and some additional stuff is done.
 * Arguments    : object ob - the object that revives from linkdeath
 */
public nomask void 
revive(object ob)
{
    int index, size, aid;
    object roomob;
    mixed room, *alarms, *args;
    
    /* Check it just in case. We do _not_ want a runtime error here */
    if (pointerp(places[ob]))
    {
	room = places[ob][0];
	aid = places[ob][1];
    }
    else
    {
	room = 0;
	aid = 0;
    }
    
    if (objectp(room))
	roomob = room;
    else if(stringp(room))
    {
	roomob = find_object(room);
	if (!objectp(roomob))
	{
	    catch(room->baba());
	    roomob = find_object(room);
	}
    }
    if (!objectp(roomob))
    {
	ob->catch_msg("Couldn't locate the location where you link died.\n"+
		      "Moving you to your start location.\n\n");
	roomob = ob->query_default_start_location();
    }
/*
    if (member_array(ob->query_race_name(), m_indices(REVIVE_MESSAGE)) < 0)
    {
	tell_room(roomob, QCTNAME(ob) + " revives from link death.\n",
		  ({ ob }) );
    }
    else
    {
	tell_room(roomob, QCTNAME(ob) +
		  REVIVE_MESSAGE[ob->query_race_name()] + "\n", ({ ob }) );
    }
*/

    tell_roombb(roomob, QCIMIE(ob, PL_MIA) + 
        " odzyskuje kontakt z rzeczywistoscia.\n", ({ ob }));

    ob->move_living("M", roomob, 1);

    tell_roombb(this_object(), "Statua " + QIMIE(ob, PL_DOP) +
	" zamienia sie w chmure dymu i znika.\n");

    /* restore the props altered in move_living() */
    index = -1;
    size = sizeof(props_to_save);
    while(++index < size)
    {
        ob->add_prop(props_to_save[index],
	    ob->query_prop_setting(STATUE_ROOM + props_to_save[index]));
	ob->remove_prop(STATUE_ROOM + props_to_save[index]);
    }

    remove_alarm(aid);
    places = m_delete(places, ob);    
}

/*
 * Function name: shutdown_activated
 * Description  : When shutdown is activated, this function is called to
 *                deal with the people who are linkdead. They are
 *                destructed.
 */
public nomask void
shutdown_activated()
{
    object *players;
    int     index;
    int     size;

    /* It may only be called from ARMAGEDDON. */
    if (!CALL_BY(ARMAGEDDON))
    {
	return;
    }

    players = m_indices(places);
    index = -1;
    size = sizeof(players);

    /* We force all players that are linkdead to quit. */
    while(++index < size)
    {
	if (objectp(players[index]) && present(players[index]))
	{
	    set_this_player(players[index]);
	    players[index]->quit();
	}
    }
}
