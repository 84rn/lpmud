/*
  cmd/live/things2.c

  General commands for manipulating things.
  
  Ver 2.0

*/
#pragma save_binary

inherit "/cmd/std/command_driver";

#include <std.h>
#include <macros.h>
#include <stdproperties.h>

#include <cmdparse.h>
#include <language.h>
#include <subloc.h>

#define TO this_object()
#define TP this_player()
#define ENV (environment(TP))
#define PREV_LIGHT CAN_SEE_IN_ROOM(TP)

/*
 * Prototypes
 */


/*
 * Global variables
 */

/*
 * create - just set backbone uid
 */
create()
{
    seteuid(getuid(this_object())); 
}

/* **************************************************************************
 * Return a proper name of the soul in order to get a nice printout.
 */
string
get_soul_id()
{
    return "things2";
}

/* **************************************************************************
 * This is a command soul.
 */
int
query_cmd_soul()
{
    return 1;
}

/* **************************************************************************
 * The list of verbs and functions. Please add new in alfabetical order.
 */
mapping
query_cmdlist()
{
    return ([
	     "appraise":"appraise",  	/* Not ready */

	     "drop":"put",		/* Not ready */

	     "examine":"examine",	
	     "exa":"examine",		

	     "get":"get",		/* Not ready */
	     "give":"give",		/* Not ready */

	     "inventory":"inventory",	

	     "look":"look",		

	     "pick":"get",		/* Not ready */
	     "put":"put",		/* Not ready */

	     "search":"search",		/* Not ready */

	     "take":"get",		/* Not ready */

	     ]);
}

/*
 * Function name: using_soul
 * Description:   Called once by the living object using this soul. Adds
 *		  sublocations responsible for extra descriptions of the
 *		  living object.
 *		  We will use this for manipulations that is extended in
 *		  time.
 */
public void 
using_soul(object live)
{
/*
    live->add_subloc(SUBLOC_THINGSEXTRADESC, file_name(this_object()));
    live->add_textgiver(file_name(this_object()));
*/
}

/* **************************************************************************
 * Here follows some support functions. 
 * **************************************************************************/

/*
 * We fail to do something because it is dark
 */
static varargs int
light_fail(string str)
{
    if (!str)
	str = query_verb() + " things";
    notify_fail("It is too dark to " + str + ".\n");
    return 0;
}

/*
 * To remove a property, used by call_out
 */
void 
remove_temp_prop(mixed arr)
{
    if (objectp(arr[0]))
	arr[0]->remove_prop(arr[1]);
}

/*
 * Here are some functions with the looks command.
 */

/*
 * Check to see if a certain sublocation is visible on an object
 */
static int
see_inside_subloc(string subloc, object ob)
{
    return ob->subloc_cont_access(subloc, SUBL_ACS_SEE, this_player());
}

static int
visible(object ob)
{
    return SUBL_CODE->object_visible(ob, this_player());
}

varargs int
visible_here(object ob)     /* Cant be static, used from cmdparse */
{
    object env;

    if (!objectp(ob))
	return 0;

    env = environment(ob);
    if (env == this_player() || env == environment(this_player()))
	return visible(ob);
}

/*
 * Show sublocs as given by name
 */
static string
show_subloc(string name, object ob)
{
    string *slocs;

    /* Examine will give all sublocations 
     */
    slocs = ob->subloc_id(name);
    slocs = filter(slocs, "see_inside_subloc", this_object(), ob);
    if (sizeof(slocs))
	return ob->show_sublocs(this_player(), slocs);
    else 
    {
	return "";
    }
}

/*
 * Returns a subitem at a specific object if that object has the subitem,
 * otherwise 0.
 */
static string
show_subitem(string sit, string prep, object ob)
{
    string *slocs, itl, str;

    /* 
	at and prp_examine are hardcoded to indicate long()
     */
    if (prep == "at" || prep == "prp_examine")
    {
	str = ob->long(sit);

	/* Some objects do not handle parameters to long()
         */
	if (strlen(str) && itl == ob->long())
	    str = "";
    }
    else
	str = "";

    itl = show_subloc(prep + " " + sit, ob);
    if (strlen(itl))
	str += itl;

    if (strlen(str))
	return str;
    else if (prep != "prp_examine")
	return "You see nothing " + prep + " " + sit + ".\n";
    else
	return "You see nothing specific about " + sit + ".\n";
}

/*
 * Returns description of a specific normal item, optionally with inventory.
 * prep - What is to be shown at object. 
 */
static string
show_item(object it, string prep)
{
    object *invobs;
    string *slocs, itl, p, pron, defprep, str;
    int i;

    if (prep == "at" || prep == "prp_examine")
	str = it->long();
    else
	str = "";

    pron = it->query_objective();

    if (!strlen(pron))
	pron = "it";

    itl = show_subloc(prep, it);
    if (strlen(itl))
	str += itl;

    if (strlen(str))
	return str;
    else if (prep != "prp_examine")
	return "You see nothing " + prep + " " + pron + ".\n";
    else
	return "You see nothing specific about " + pron + ".\n";
}

/*
 * Decide what item(s) are being referenced.
 *
 * It is here all checks of visible objects and insides of containers are
 * done.
 * Arguments are arrays of the same size
 *
 * Return those in an array with each element:  prep, object 
 */
static mixed *
decide_item(string *preps, mixed *itemlists)
{
    object 	*ob;
    mapping	env;
    mixed 	*next;
    string 	sublocid, *slocs;
    int 	i, i2, num;

    notify_fail("No such thing found.\n");

    if (sizeof(preps) == 1 && !stringp(itemlists[0]))
    {
	notify_fail(capitalize(preps[0]) + " what?\n");
	ob = VISIBLE_ACCESS(itemlists[0], "visible_here", this_object());
	if (sizeof(ob))
	    return ({ preps[0], ob });
	else
	    return 0;
    }
    else if (sizeof(preps) > 1)	
    {
	/* 
	    Is the next item a sublocation?
	 */
	if (stringp(itemlists[1]))
	{
	    next = decide_item(preps[2..sizeof(preps)],
			 itemlists[2..sizeof(itemlists)]);
	    notify_fail(capitalize(preps[1]) + " " + itemlists[1] + 
			" of what?\n");
	}
	else
	{
	    next = decide_item(preps[1..sizeof(preps)],
			 itemlists[1..sizeof(itemlists)]);
	    notify_fail(capitalize(preps[1]) + " what?\n");
	}

	if (!sizeof(next))
	    return 0;

	if (stringp(itemlists[1]))
	    sublocid = preps[1] + " " + itemlists[1];
	else
	    sublocid = next[0];  /* Preposition */

	for (env = ([]), i = 1; i < sizeof(itemlists[0]); i++)
	    env[environment(itemlists[0][i])] = 1;

	/* 
	    Find what objects are in that/those sublocation(s)
         */
	for (ob = ({}), i = 0; i < sizeof(next[1]); i++)
	{
	    if (!env[next[1][i]])
		continue;
	    slocs = next[1][i]->subloc_id(sublocid);
	    if (!sizeof(slocs))
		continue;
	    for (i2 = 0; i2 < sizeof(slocs); i2++)
	    {
		if (!next[1][i]->subloc_cont_access(slocs[i2], SUBL_ACS_SEE,
						    TP))
		    continue;
		ob += next[1][i]->subinventory(slocs[i2]);
	    }
	}
	
	/* 
	    Numerical designator
	 */
	num = itemlists[0][0];

	/* 
	    Which of those are common with the parsed ones
         */
	ob = ob & itemlists[0][1..sizeof(itemlists[0])];

	if (!sizeof(ob))
	    return 0;

	notify_fail("Not so many available.\n");

	ob = VISIBLE_ACCESS(({ num }) + ob, "ret1", this_object());

	if (sizeof(ob))
	    return ({ preps[0], ob });
	else
	    return 0;
    }
    else 
    {
	notify_fail("Sorry, that request was very confusing.\n");
	return 0;
    }

    return 0; /* Should not be reached */
}

public int ret1() { return 1; } /* Patch to use visible_access */

/* 
 * Look ended here.
 */

/* **************************************************************************
 * Here follows the actual functions. Please add new functions in the 
 * same order as in the function name list.
 * **************************************************************************/

/*
 * examine - Examine something
 */
/*
 * Function name: examine
 * Description:   Look with a special 'preposition' to see all sublocations
 *		  instead of only one or default when normal prepositions.
 * Arguments:	  string str: tail of examine command or exa command
 * Returns:       1: found something to look at
 *                0: failed to found object
 * Ex:            examine("knife")
 *
*/
int examine(string str)
{
    return this_object()->look("prp_examine " + str);
}

/*
 * inventory - List things in my inventory
 */
int
inventory()
{
    object *obarr;
    string str;

    if (PREV_LIGHT <= 0)
	return light_fail("see");

    TP->add_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS, 1);
    call_out("remove_temp_prop", 1, ({ TP, TEMP_SUBLOC_SHOW_ONLY_THINGS }));
    write(TP->show_sublocs(TP));

    TP->remove_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS);
    return 1;
}

/*
 * Function name: look
 * Description:   glances around, examines objects, or looks at pseudo items
 * Arguments:	  str: tail of look command
 * Returns:       1: found something to look at
 *                0: failed to find object
 * Notify_fail:   several
 * 
 * Globals:       gItem:  
 *                gHolder:
 * Ex:            look, look at orcs, look inside chest
 *
   Documentation of look
   
   The look at (in, inside) command has the following features.

   look	
   	Shows long description of the room and short description of
   	all visible items in the environment (see do_glance() ).

   look prep item prep item ....
        Look at some specific item. It can be a normal object,
	a pseudo item or a sublocation. 
 */
int 
look(string str)
{
    string    	*preps, desc, pron, px;
    mixed	*itemlists, *workitems, *res;
    object	ob;
    int         i, last_sub, only_sub, ok_sub;

    if (PREV_LIGHT <= 0)
	return light_fail("see");

    if (!strlen(str))
	return TP->do_glance(0);
    
    res = CMDPARSE_ITEMLIST(lower_case(str));

    if (!sizeof(res))
	return 0;

    dump_array(res);

    preps = 	res[0];		/* The prepositions */
    itemlists = res[1];		/* The items */
    last_sub = 	res[2];		/* True if last was not a normal object */
    only_sub =	res[3];		/* True if no normal objects */

    /* Subitem or sublocation in the room
     *
     * There is no support for subitems at specific sublocations so
     * we simply skip any specifications and print an assumption message.
     *
     * If not found in the room, visible objects in the room and in players
     * inventory are checked. Assumption message is printed.
     */
    if (only_sub)
    {
	notify_fail("No such thing: " + str + "\n");
	if (preps[0] != "prp_examine")
	    px = preps[0] + " ";
	else
	    px = "";

	desc = show_subitem(itemlists[0], preps[0], environment(TP));
	if (strlen(desc))
	{
	    if (sizeof(itemlists) > 1)
		write("(" + px + "the " + itemlists[0] + " here.)\n");
	    write(desc);
	    return 1;
	}
	else
	{
	    workitems = filter(all_inventory(environment(TP)) +
			       all_inventory(TP),
			       "visible", this_object());

	    for (i = 0; i < sizeof(workitems); i++)
	    {
		desc = show_subitem(itemlists[0], preps[0], workitems[i]);
		if (strlen(desc))
		{
		    if (workitems[i] != TP)
			write("(" + px + "the " + itemlists[0] + " of " + 
			      workitems[i]->short(TP) +
			      (environment(workitems[i]) == TP ?
			       " carried by you" : "") +
			      ".)\n");
		    else
			write("(" + px + "your " + itemlists[0] + ".)\n");

		    write(desc);
		    return 1;
		}
	    }
	    notify_fail("You find no(thing) '" + px + itemlists[0] +
			"' here.\n");
	    return 0;
	}
    }
    
    /* Subitem or sublocations at specific normal item(s)
     */
    if (stringp(itemlists[0]))
    {    
	/* We could check for allowed prepositions here if we want to */

	workitems = decide_item(preps[1..sizeof(preps)],
			 itemlists[1..sizeof(itemlists)]);

	if (!sizeof(workitems))
	    return 0;
	else if (sizeof(workitems[1]) > 1)
	    pron = "any of them";
	else
	    pron = workitems[1][0]->query_objective();
	
	if (!strlen(pron))
	    pron = "it";

	for (ok_sub = 0, i = 0; i < sizeof(workitems[1]); i++)
	{
	    desc = show_subitem(itemlists[0], workitems[0], workitems[1][i]);
	    if (strlen(desc))
	    {
		if (sizeof(workitems[1]) > 1)
		    write("(" + itemlists[0] + " " + workitems[0] + " " +
			  workitems[1][i]->short() + "):\n");
		write(desc);
		ok_sub = 1;
	    }
	}
	if (ok_sub)
	    return 1;
	else
	{
	    if (workitems[0] == "of")
		notify_fail("You can't find any " + itemlists[0] +
			" at " + pron + ".\n");
	    else
		notify_fail("You can't find any " + itemlists[0] + " " +
			workitems[0] + " " + pron + ".\n");
	    return 0;
	}
    }

    /* 
	Normal items
     */
    workitems = decide_item(preps, itemlists);

    if (!sizeof(workitems))
	return 0;

    if (workitems[0] == "of")
    {
	notify_fail("Sorry, item or location needed before 'of'.\n");
	return 0;
    }

    else if (sizeof(workitems[1]) == 1)
	write(show_item(workitems[1][0], workitems[0]));

    else
    {
	for (i = 0; i < sizeof(workitems[1]); i++)
	{
	    if (workitems[0] != "prp_examine")
		write("(" + capitalize(workitems[0]) + " " +
		      workitems[1][i]->short() + ")\n");
	    else
		write("(Examine " + workitems[1][i]->short() + ")\n");
	    write(show_item(workitems[1][i], workitems[0]));
	}
    }
    return 1;
}



