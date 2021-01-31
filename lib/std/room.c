/*  
 * /std/room.c
 *
 * This is the room object. It should be inherited by all rooms.
 */

#pragma save_binary
#pragma strict_types

inherit "/std/container";

#include <files.h>
#include <macros.h>
#include <ss_types.h>
#include <std.h>
#include <stdproperties.h>

#include "/std/room/exits.c"
#include "/std/room/description.c"
#include "/std/room/link.c"
#include "/std/room/move.c"

static string	obj_short; /* Przeniesione z /std/object.c */

static object   room_link_cont;	/* Linked container */
static object   *accept_here = ({ }); /* Items created here on roomcreation */

/*
 * Function name: create_room
 * Description  : Constructor. You should redefine this function to create
 *                your own room.
 */
public void
create_room()
{
    remove_prop(ROOM_I_INSIDE);          /* Default room has heaven above */
    add_prop(ROOM_I_TYPE, ROOM_NORMAL);  /* Default is a room */
}

/*
 * Function name: create_container
 * Description  : Constructor. Since you may not redefine this function,
 *                you must define the function create_room() to create your
 *                room.
 */
nomask void
create_container()
{
    add_prop(ROOM_I_IS,    1);
    add_prop(ROOM_I_LIGHT, 1);
    add_prop(ROOM_I_HIDE, 25);
    
    room_link_cont = 0;

    seteuid(creator(this_object()));

    /* As service to the folks, we automatically call the function
     * enable_reset() to start resetting if the function reset_room() has
     * been redefined.
     */
    if (function_exists("reset_room", this_object()) != ROOM_OBJECT)
	enable_reset();

    create_room();

    /* Jesli w lokacji rosna ziola, odpalamy ich reset. */
    if (sizeof(query_herb_files()))
        enable_reset();

    accept_here = all_inventory(this_object());
    if (!sizeof(accept_here))
	accept_here = ({ });
}

/*
 * Function name: reset_room
 * Description  : This function should be redefined to make the room reset
 *                every half our or so. If you redefine it, you do not have
 *                to call enable_reset() since we call it as part of our
 *                service ;-) Note that this service is only valid for rooms.
 */
void
reset_room()
{
    /* Rosna nowe ziolka... */
    this_object()->set_searched(0);
}

/*
 * Function name: reset_container
 * Description  : This function will reset the container. Since you may not
 *                redefine it, you must define the function reset_room() to
 *                make the room reset.
 */
nomask void
reset_container()
{
    reset_room();

    if (!sizeof(accept_here))
	accept_here = ({ });
    else
	accept_here = accept_here - ({ 0 });
}

public string
short()
{
    return obj_short;
}

public void
set_short(string str)
{
    obj_short = str;
}

public string
query_short()
{
    return obj_short;
}

/*
 * Function name: clone_here
 * Description  : The behaviour of this function is exactly the same as the
 *                efun clone_object(). It clones the 'file' and returns the
 *                objectpointer to that object. However, it will also add
 *                the object to a list of items that 'belongs' in this room.
 *                This means that the presence of this object in the room will
 *                not prevent the room from being cleaned with clean_up().
 * Arguments    : string file - the path to the item to clone.
 * Returns      : object - the objectpointer to the clone.
 */
public object
clone_here(string file)
{
    object ob;
    
    ob = clone_object(file);
    accept_here += ({ ob });
    return ob;
}

/*
 * Function name: query_cloned_here
 * Description  : Returns all objects that have been cloned in this room with
 *                clone_here() or registered with add_accepted_here().
 * Returns      : object * - the list of objects.
 */
public object *
query_cloned_here()
{
    return secure_var(accept_here);
}

/*
 * Function name: add_accepted_here
 * Description  : With this function, you can register an object as being
 *                accepted in this room. This means that the object will not
 *                prevent the room from being cleaned up. It will give an
 *                item the same status as when it was cloned with clone_here()
 *                in this room.
 * Arguments    : object ob - the object to register.
 */
void
add_accepted_here(object ob)
{
    accept_here += ({ ob });
}

/*
 * Function name: light
 * Description:   Returns the light status in this room
 *                This function is called from query_prop() only.
 * Returns:	  Light value
 */
nomask int
light()
{
    int li;
    
    li = query_prop(ROOM_I_LIGHT);
    if (objectp(room_link_cont))
    {
	if ((environment(room_link_cont)) &&
	    (room_link_cont->query_prop(CONT_I_TRANSP) ||
	     room_link_cont->query_prop(CONT_I_ATTACH) ||
	    !room_link_cont->query_prop(CONT_I_CLOSED)))
	{
	    li += (environment(room_link_cont))->query_prop(OBJ_I_LIGHT);
	}
    }
    return query_internal_light() + li;
}

/*
 * Function name: set_container
 * Description:   Sets the container for which the room represents the inside
 * Arguments:	  ob: The container object
 */
public void
set_container(object ob)
{
    room_link_cont = ob;
}

/*
 * Function name: set_room
 * Description  : This function is a mask for the function set_room() in
 *                /std/container.c. That function is not valid for rooms,
 *                so we block it here.
 * Arguments    : the arguments described in /std/container.c
 */
public nomask void
set_room(mixed room)
{
}

/* 
 * Function name: update_internal
 * Description:   Updates the light, weight and volume of things inside
 *                also updates a possible connected container.
 * Arguments:     l: Light diff.
 *		  w: Weight diff. (Ignored)
 *		  v: Volume diff. (Ignored)
 */
public void
update_internal(int l, int w, int v)
{
    ::update_internal(l, w, v);
    
    if (l)
	all_inventory()->notify_light_change(l);
    
    if (room_link_cont)
	room_link_cont->update_internal(l, w, v);
}

/*
 * Function name: clean_up
 * Description  : This function destruct the room if there is nothing in it.
 *                If you have special variables stored in a room you should
 *		  define your own clean_up(). Also if you on startup of the
 *		  room clone some objects and put inside it, please define
 *		  your own clean_up() to destruct the room. This saves a
 *		  lot of memory in the game.
 * Returns      : int 1/0 - call me again/ don't bother me again.
 */
public int
clean_up()
{
    /* Do not destroy the room object. */
    if (MASTER == ROOM_OBJECT)
    {
	return 0;
    }

    if (!query_prop(ROOM_I_NO_CLEANUP) &&
	!sizeof(all_inventory(this_object()) - accept_here))
    {
	remove_object();
    }

    return 1;
}

/*
 * Function name: room_add_object
 * Description:   Clone and move an object into the room
 * Arguments:	  file - What file it is we want to clone
 *		  num  - How many clones we want to have, if not set 1 clone
 *		  mess - Message to be written when cloned
 */
varargs void
room_add_object(string file, int num, string mess)
{
    int i;
    object ob;

    if (num < 1)
	num = 1;

    seteuid(getuid());
    for (i = 0; i < num; i++)
    {
	ob = clone_object(file);
	if (stringp(mess))
	{
	    ob->move(this_object(), 1);
	    tell_roombb(this_object(), mess, ({}), ob);
 	}
	else if (living(ob))
	    ob->move_living("xx", this_object());
	else
	    ob->move(this_object(), 1);
    }
}

/*
 * Function name: stat_object
 * Description:   Called when someone tries to stat the room
 * Returns:	  A string to write
 */
string
stat_object()
{
    string str;
    int type;

    str = ::stat_object();

    if (query_prop(ROOM_I_INSIDE))
	str += "wewnatrz\t";
    else
	str += "zewnatrz\t";

    type = query_prop(ROOM_I_TYPE);
    str += " ";
    switch (type)
    {
    case 0: str += "zwykly"; break;
    case 1: str += "w wodzie"; break;
    case 2: str += "pod woda"; break;
    case 3: str += "w powietrzu"; break;
    case 4: str += "plaza"; break;
    default: str += "nieznany typ"; break;
    }
    str += "\t";

    return str + "\n";
}

/*
 * Function name: query_domain
 * Description  : This function will return the name of the domain this
 *                room is in.
 * Returns      : string - the domain name.
 */
nomask string
query_domain()
{
    /* Normal room. */
    if (wildmatch("/d/*", file_name(this_object())))
    {
	return explode(file_name(this_object()), "/")[2];
    }

    /* Link-room. */
    /* Dodalem test query_link_master(), bo jak 0 wchodzilo do wildmatch
     * to byl runtime...
     */
    if (query_link_master() && wildmatch("/d/*", query_link_master()))
    {
	return explode(query_link_master(), "/")[2];
    }

    /* This shouldn't happen. */
    return BACKBONE_UID;
}
