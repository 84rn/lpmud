/*
  Standard commands

  This object gives you standard handling of commands.


  Available standard forms:

  "verb %i"              Normal access
  "verb %i %s %i"        Access in containers
  "verb %i %s %i"        Both %i normal access


  For details on %i and %s  see /doc/efun/parse_command
*/

#pragma no_clone
#pragma no_inherit
#pragma no_shadow
#pragma save_binary

#include <macros.h>
#include <std.h>
#include <stdproperties.h>
#include <cmdparse.h>
#include <composite.h>

#define PARSE_ENV (objectp(environment(this_player()))	\
		   ? environment(this_player()) : this_player())

/*
 * Prototypes
 */
static mixed *order_num(mixed *arr, int num);
nomask int can_see(object ob, object player);
nomask int first(object ob);
nomask int nacs(object ob);

static object gObj;
int gNum;

int
eq_gObj(object ob)
{
    return (gObj == ob);
}

varargs mixed *
visible_access(mixed *arr, string acsfunc, object acsobj, int normflag,
	       int include_invis)
{
    int num;
    object *a;
    
    if (!pointerp(arr))
	return 0;                   /* It must be an array */
    num = arr[0];                   /* Numeric designation */

    a = COMPOSITE_SORT(arr, "short");

    if (acsfunc)
    {
	a = filter(a, acsfunc, acsobj);
    }
    else
	/* must be in me or in env */
	a = filter(a, nacs);

    if (!include_invis)
        a = filter(a, &can_see(, this_player()));

    if (normflag && a) /* Most often we do not want to return this_player() */
    {
        a -= ({ this_player() });
    }

    if (!sizeof(a))
	return ({ });               /* Not accessible */
    if (num == 0)
	return a;                   /* Select all items */
    else if (num < 0)               
	/* Select a certain item, eg. first or third */
	return order_num(a, -num);
    else
    {
	/* Select a number of items, eg. one or three */
	gNum = num;
	a = filter(a, first);
	/* Get the first so many we want */
	return a;                   /* If fewer, return so many we have */
    }
}

varargs mixed *
normal_access(mixed *arr, string acsfunc, object acsobj, int include_invis)
{
    return visible_access(arr, acsfunc, acsobj, 1, include_invis);
}

nomask int
nacs(object ob)
{
    if (!objectp(ob))
	return 0;
    if (environment(ob) == this_player())
	return 1;
    if (environment(ob) == environment(this_player()))
	return 1;
    return 0;
}

nomask int
can_see(object ob, object player)
{
    return ob->check_seen(player);
}

nomask int 
first(object ob)
{
    if (gNum < 1)
	return 0;
    else if (ob->query_prop(HEAP_I_IS)) 
	gNum -= ob->split_heap(gNum);
    else
	gNum -= 1;
    return 1;
}

static mixed *
order_num(mixed *arr, int num)
{
    int cnt, ix;
    cnt = 1;
    for (ix = 0; ix < sizeof(arr); ix++)
    {
	if (arr[ix]->query_prop(HEAP_I_IS))
	{
	    cnt += arr[ix]->num_heap();
	    if (cnt > num)
	    {
		arr[ix]->split_heap(1);
		return ({ arr[ix] });
	    }
	}
	else
	    cnt++;
	if (cnt > num)
	    return ({ arr[ix] });
    }
    return ({ });
}

/* General command routine for "verb %i"

   Access objects inside player or in players environment.
   Parses command and calls 'single_func' for each object to do the command
   upon. Returns an array of these objects for which 'single_func' returned 1.
*/
varargs object *
do_verb_1obj(string cmd, string single_func, string acsfunc, object cobj,
	     int include_invis)
{
    mixed * itema;
    object call_obj;
    
    if (!cmd)
	return 0;
    if (!cobj)
	call_obj = previous_object();
    else
	call_obj = cobj;
    
    if (!parse_command(cmd, PARSE_ENV, "%i:" + PL_BIE, itema))
	return 0;
    
    itema = normal_access(itema, acsfunc, cobj, include_invis);
    if (!itema)
	return 0;
    itema = filter(itema, single_func, call_obj);
    this_player()->set_obiekty_zaimkow(itema);
    
    return itema;
}

/* General command routine for "verb %i %s %i" (typical : get from )

   Access objects located inside containers.
   Access containers inside player or in players environment.
   Calls 'prep_func' for acknowledge of word returned by %w
   
   Parses command and calls 'single_func' for each object to do the command
   upon. Returns array of these objects for which 'single_func' returned 1,

*/
static mixed *gContainers;

varargs object *
do_verb_inside(string cmd, string prep_func, string single_func,
	       string afunc, mixed cobj)
{
    mixed *itema1, *itema2;
    object call_obj;
    string str2;
    
    if (!cmd)
	return 0;
    if (!cobj)
	call_obj = previous_object();
    else
	call_obj = cobj;
    
    if (!parse_command(cmd, PARSE_ENV,
		       "%i %w %i", itema1, str2, itema2))
	return 0;
    
    if (!call_other(call_obj, prep_func, str2))
	return 0;
    
    itema2 = normal_access(itema2, afunc, cobj);  
    if (!itema2)
	return 0;
    
    gContainers = itema2;
    
    itema1 = normal_access(itema1, "in_containers", this_object());
    if (!itema1)
	return 0;
    
    itema1 = filter(itema1, single_func, call_obj);
    this_player()->set_obiekty_zaimkow(itema1, itema2);
    return itema1;
}

/*
 * Function name: in_containers
 * Description:   test if object is in one of a set of containers
 * Arguments:	  ob: object
 * Returns:       1: is in one of the conatiners
                  0: not in any of the containers
 * globals        gObj: the container of ob
                  gContainers: the array of containers
 *
 */

int
in_containers(object ob) 
{
    int in;
    
    if (!objectp(ob))
	return 0;
    if (!environment(ob))
	return 0;
    gObj = environment(ob);
    if (filter(gContainers, eq_gObj))
	return 1;
    else
	return 0;
}

/* General command routine for "verb %i %w %i" (typical hit xx with yy )

   Access objects inside player or in players environment.
   Access a single object inside player or in players environment.
   Calls 'check_func' for acknowledge of word returned by %w and second %i
   
   Parses command and calls 'single_func' for each object to do the command
   upon. Returns an array of these objects for which 'single_func' returned 1.
*/
/* static int gContainers; */


do_verb_with(string cmd, string check_func, string single_func,
	     string acsfunc1, string acsfunc2, mixed cobj)
{
    mixed *itema1, *itema2;
    object call_obj;
    string str2;
    
    if (!cmd)
	return 0;
    if (!cobj)
	call_obj = previous_object();
    else
	call_obj = cobj;
    
    if (!parse_command(cmd, PARSE_ENV,
		       "%i %w %i", itema1, str2, itema2))
	return 0;
    
    itema2 = normal_access(itema2, acsfunc2, cobj);  
    if (!itema2)
	return 0;
    
    gContainers = itema2;
    
    itema1 = normal_access(itema1, acsfunc1, cobj);
    if (!itema1)
	return 0;
    
    if (!call_other(call_obj, check_func, str2, itema2))
	return 0;
    
    itema1 = filter(itema1, single_func, call_obj);
    this_player()->set_obiekty_zaimkow(itema1, itema2);
    return itema1;
}

/*
 * Function name: find_str_in_arr
 * Description:   Will return the array of objects corresponding to the str 
 *		  that this_player can see, picked from the given array.
 * Arguments:	  str - the string to search for
 *		  ob  - An array with objects to look through
 * Returns:	  the object array.
 */
object *
find_str_in_arr(string str, object *ob, int przyp)
{
    int num;
    object *a;
    mixed *arr;

    if (!str)
	return ({});
    a = ({});
    
    if (parse_command(str, ob, "%i:" + przyp, arr))
    {
	num = arr[0];                   /* Numeric designation */
        a = COMPOSITE_SORT(arr, "short");
	a -= ({ 0 });
 	a = filter(a, &can_see(, this_player()));

	if (sizeof(a))
	{
	    if (num == 0)
		return a;                   /* Select all items */
	    else if (num < 0)               
	    {
	        a = order_num(a, -num);
		/* Select a certain item, eg. first or third */
	        this_player()->set_obiekty_zaimkow(a);
		return a;
	    }
	    else
	    {
		/* Select a number of items, eg. one or three */
		gNum = num;
		a = filter(a, first);
		/* Get the first so many we want */
	        this_player()->set_obiekty_zaimkow(a);
		return a;            /* If fewer, return so many we have */
	    }
	}
    }

    return ({});
}

/*
 * Function name: find_str_in_object
 * Description:   Will search through an object and try to find objects matching
 *		  the given string.
 * Arguments:	  str - The string to look for
 *		  ob  - The object to look in
 * Returns:	  An array with corresponding objects, empty if no match
 */
object *
find_str_in_object(string str, object ob, int przyp)
{
    object *arr, obj;

    arr = find_str_in_arr(str, all_inventory(ob), przyp);
    
    if (!sizeof(arr) && (obj = present(str, ob)) && CAN_SEE(this_player(), obj))
	arr = ({ obj });
    return arr - ({ 0 });
}

/*
 * Parses a string on the form:
 *
 *    <prep> <item> <prep> <item> <prep> <item> ....
 *
 * item can be a subitem, sublocation or a normal object.
 *
 * Returns an array with four elements:
 *
 * ret[0]		 The prepositions 
 * ret[1]		 The items, a normal parse_command %i return value 
 *			 or a string (subitem/sublocation)
 * ret[2]		 True if last was not a normal object 
 * ret[3]		 True if no normal objects 
 *
 */
mixed *
parse_itemlist(string str)
{
    string    	*preps, *trypreps, itemstr, nrest, rest;
    mixed	*itemlists, *itemlist;
    int		last_sub, only_sub, bef;

    preps = ({});
    itemlists = ({});
    last_sub = 0;
    only_sub = 1;
    rest = str;

    trypreps = SECURITY->parse_command_prepos_list();

    trypreps += ({ "at", "of", "prp_examine" });

    while (strlen(rest))
    {
	notify_fail(break_string("Sorry, In '" + str + "'. The ending: '" + 
				 rest + "', not understood.\n", 76));
	if (!parse_command(rest, ({}), "%p %s", trypreps, nrest))
	    return 0;
	if (!strlen(nrest))
	    return 0;
	
	preps += ({ trypreps[0] });
	
	if (!parse_command(nrest, PARSE_ENV,
			   "%i %s", itemlist, rest))
	{
	    if (!parse_command(nrest, ({}), "%s %p %s", itemstr, 
			       trypreps, rest)) 
	    {
		/* Subitem or sublocation in the room 
		 */
		itemlists += ({ nrest });
		last_sub = 1;
		break;
	    }
	    else /* subitem or sublocation with preposition after */
	    {
		itemlists += ({ itemstr });
		rest = trypreps[0] + (strlen(rest) ? " " + rest : "");
		last_sub = 1;
	    }
	    bef = sizeof(itemlists) - 2;
	    
	    /* Two following subitem/sublocations are combined to one
	     */
	    if (bef >= 0 && stringp(itemlists[bef]))
	    {
		itemlists[bef] += " " + preps[bef + 1] + " " +
		    itemlists[bef + 1];
		preps = preps[0..bef];
		itemlists = itemlists[0..bef];
	    }
	}
	else /* Normal item */
	{
	    itemlists += ({ itemlist });
	    last_sub = 0;
	    only_sub = 0;
	}
    }
    return ({ preps, itemlists, last_sub, only_sub });
}

