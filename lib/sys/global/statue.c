/*
 * /d/Genesis/obj/statue.c
 *
 * This is the statue room. Here we keep all players who linkdied.
 */

#pragma no_clone
#pragma no_inherit
#pragma no_clone
#pragma save_binary
#pragma strict_types

inherit "/std/room";

#include <macros.h>
#include <std.h>
#include <stdproperties.h>

#define BSN(message) break_string(message, 75) + "\n"
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
void
create_room() 
{
    set_short("This is the statue room");
    set_long(BSN(
	"You are in a part of the Keepers' art collection. " +
	"This is where they keep the statues of some unfortunate " +
	"adventurers. Rumours have it that the Keepers once fought " +
	"a great battle against revolting Archwizards here and " +
	"that the room still contains strong magic from that time. " +
	"Some visitors claim that they have seen statues turn alive " +
	"and disappear!"));
    add_exit("/d/Standard/start/human/town/tower", "south");
    change_prop(ROOM_I_LIGHT, 1000);
    places = ([]);
}

/*
 * Function name: remove_player
 * Description  : Can be called to remove a player from the room if it has
 *                been in here too long.
 * Arguments    : object ob - the player to remove.
 */
void
remove_player(object ob)
{
    if (ob && present(ob))
    {
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
void 
die(object ob) 
{
    int index, size;
    
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

    tell_roombb(environment(ob), QCIMIE(ob, PL_MIA) + "traci kontakt z "
              + "rzeczywistoscia.\n", ({ob}), ob);

    places += ([ ob : environment(ob) ]);

    /* save the props altered in move_living() */
    index = -1;
    size = sizeof(props_to_save);
    while(++index < size)
    {
        ob->add_prop(STATUE_ROOM + props_to_save[index],
	    ob->query_prop_setting(props_to_save[index]));
    }

    ob->move_living("M", this_object(), 1);
    ob->save_me(1);
    set_alarm(1800.0, 0.0, "remove_player", ob); /* 30 minutes */

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
void 
revive(object ob)
{
    int index, size;

/*
    if (member_array(ob->query_race_name(), m_indices(REVIVE_MESSAGE)) < 0)
    {
	tell_room(places[ob], QCTNAME(ob) + " revives from link death.\n",
	    ({ ob }) );
    }
    else
    {
	tell_room(places[ob], QCTNAME(ob) +
	    REVIVE_MESSAGE[ob->query_race_name()] + "\n", ({ ob }) );
    }
*/

    tell_roombb(places[ob], QCIMIE(ob, PL_MIA) + "odzyskuje kontakt z "
              + "rzeczywistoscia.\n", ({ob}), ob);

    ob->move_living("M", places[ob], 1);

    /* restore the props altered in move_living() */
    index = -1;
    size = sizeof(props_to_save);
    while(++index < size)
    {
        ob->add_prop(props_to_save[index],
	    ob->query_prop_setting(STATUE_ROOM + props_to_save[index]));
	ob->remove_prop(STATUE_ROOM + props_to_save[index]);
    }

    places = m_delete(places, ob);
}
