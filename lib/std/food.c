/*
   /std/food.c
   
   This is the standard object used for any form of eatable things

   Typical usage of /std/food.c

	inherit "/std/food";

	void
	create_food()
	{
	    set_amount(amount_in_gram);
	    set_name("name of food");
	    set_short(...);
	    set_long(....);
	}

    If you want any special effect in your food you can define
    special_effect() which is called when eating it.
    The number of items you ate is passed as argument to special_effect();

    Food may recover and recovery is added by default, but only if the
    the food has been coded in a particular object and not cloned from
    this file directly and externally set up.
*/

#pragma save_binary
#pragma strict_types

inherit "/std/heap";

#include <cmdparse.h>
#include <composite.h>
#include <files.h>
#include <macros.h>
#include <stdproperties.h>

/*
 * Global variables. They are not saved.
 */
static	int	food_amount;
static	object	*gFail;
static	int	decay_time, decayed, decay_alarm;
static	string	*decay_adj;

/*
 * Prototypes.
 */
public	int	eat_it(string str);
public	void	decay();
public	void	total_decay();

/*
 * Function name: create_food
 * Description  : This function is the constructor. You should redefine
 *                it to create your own food object.
 */
public void
create_food()
{
    ustaw_nazwe( ({ "posilek", "posilku", "posilkowi", "posilek", "posilkiem",
        "posilku" }), ({ "posilki", "posilkow", "posilkom", "posilki",
        "posilkami", "posilkach" }), PL_MESKI_NOS_NZYW );
}

/*
 * Function name: create_heap
 * Description  : The heap constructor. You should not redefine this
 *                function, but use create_food() instead. It makes the
 *                food object have a default size of 1 item.
 */
public nomask void
create_heap()
{
    set_heap_size(1);
    add_prop(OBJ_M_NO_SELL, "Jedzenie nie moze byc ponownie sprzedawane.\n");
    
    decay_adj = ({ "zepsuty", "zepsuci" });

    create_food();
}

/*
 * Function name: set_long
 * Description  : Set the long description. If no unique identifier is
 *                set, the long description is used.
 * Arguments    : string long - the long description.
 */
public void
set_long(string long)
{
    ::set_long(long);

    if (!query_prop(HEAP_S_UNIQUE_ID))
        add_prop(HEAP_S_UNIQUE_ID, long);
}

/*
 * Function name:	set_amount
 * Description:		sets the amount of food in this food (in grams)
 * Arguments:		a: The amount of food
 */
public void
set_amount(int a) 
{ 
    food_amount = a; 
    add_prop(OBJ_I_VOLUME, a / 10);
    add_prop(OBJ_I_WEIGHT, a);
}

/*
 * Function name:	query_amount
 * Description:		Gives the amount of food in this food
 * Returns:		Amount as int (in grams)
 */
public int
query_amount() { return food_amount; }

public void
set_decay_time(int time)
{
    if (time < 0)
        time = 0;
        
    decay_time = time;
}

public int
query_decay_time()
{
     return decay_time;
}

public void
set_decay_adj(string *adjs)
{
    if (sizeof(adjs) != 2)
        return;
        
    decay_adj = adjs + ({});
}

public void
stop_decay()
{
    if (!decay_alarm || !decay_time || decayed)
        return ;
        
    decay_time = ftoi(get_alarm(decay_alarm)[2] / 60.0);
    remove_alarm(decay_alarm);
    decay_alarm = 0;
}

public void
start_decay()
{
    if (decay_alarm || !decay_time || decayed)
        return ;
        
    decay_alarm = set_alarm(itof(decay_time * 60), 0.0, &decay());

}

public void
decay()
{
    remove_alarm(decay_alarm);
    decay_alarm = 0;
    decayed = 1;
    decay_time = 10;
    
    dodaj_przym(decay_adj[0], decay_adj[1]);
    
    usun_shorty();
}

public int
query_decayed()
{
    return decayed;
}


public void
init()
{
    ::init();

    add_action(eat_it, "zjedz");
}

/*
 * Function name: consume_text
 * Description:   Here the eat message is written. You may redefine it if
 *		  you wish.
 * Arguments:     arr - Objects being consumed
 *		  vb  - The verb player used to consume them
 */
void
consume_text(object *arr, string vb)
{
    string str;

    write("Zjadasz " + (str = COMPOSITE_DEAD(arr, PL_BIE)) + ".\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " zjada " + str + ".\n");
}

/*
 * Function name:	eat_it
 * Description:		Eats the objects described as parameter to 'eat'. It
 *			uses command parsing to find which objects to eat.
 * Arguments:		str: The trailing command after 'eat ...'
 * Returns:		True if command successfull
 */
public int
eat_it(string str)
{
    object 	*a, *foods;
    int		il;
    string str2, vb;

    notify_fail("Co chcesz zjesc?\n");

    /* This food has already been eaten or already been tried, so we won't
     * have to test again.
     */
    if (query_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT) ||
	this_player()->query_prop(TEMP_STDFOOD_CHECKED))
    {
	return 0;
    }

    gFail = ({ });
    vb = query_verb(); 
    
    a = CMDPARSE_ONE_ITEM(str, "eat_one_thing", "eat_access");
    if (sizeof(a) > 0)
    {
	consume_text(a, vb);
	for (il = 0; il < sizeof(a); il++)
	{
	    a[il]->special_effect(a[il]->num_heap());
	    a[il]->destruct_object();
	}
	return 1;
    }
    else
    {
	this_player()->add_prop(TEMP_STDFOOD_CHECKED, 1);
	set_alarm(1.0, 0.0,
	    &(this_player())->remove_prop(TEMP_STDFOOD_CHECKED));
	if (sizeof(gFail))
	    notify_fail("@@eat_fail:" + file_name(this_object()));
	return 0;
    }
}

string
eat_fail()
{
    string str = "";

/*
    str = "Probujesz zjesc " + COMPOSITE_DEAD(gFail, PL_BIE) + ", ale "+
        "juz nie mozesz.\n";
    saybb(QCIMIE(this_player(), PL_MIA) + " probuje zjesc " + QCOMPDEAD(PL_BIE) +
	    ", ale juz nie moze.\n");
*/	    
    this_player()->remove_prop(TEMP_STDFOOD_CHECKED);
    return str;
}

int
eat_access(object ob)
{ 
    if ((environment(ob) == this_player()) &&
	(function_exists("create_heap", ob) == FOOD_OBJECT) &&
	(ob->query_short()))
	return 1;
    else
	return 0;
}

int
eat_one_thing(object ob)
{
    int am, num, i;

    am = (int) ob->query_amount();
    num = (int) ob->num_heap();

    for (i = 1; i <= num; i++)
    {
        if (ob->query_decayed())
        {
	    if (i == 1)
	    {
		ob->split_heap(1);
		write("Nie mozesz zjesc " + capitalize(ob->short(PL_DOP)) + 
		    ", gdyz jest " + oblicz_przym(decay_adj[0], 
		    decay_adj[1], PL_MIA, ob->query_rodzaj(), 0) + ".\n");
		gFail += ({ ob });
	    	return 0;
	    }
	    ob->split_heap(i - 1);
	    return 1;
        }

        if (!this_player()->eat_food(am))
        {
	    if (i == 1)
	    {
		ob->split_heap(1);
/*
		write(capitalize(ob->short()) + " to troche za duzo dla "+
		    "twojego zoladka.\n");
*/
		write("Chyba nie dasz rady zjesc " + ob->short(PL_DOP) + 
		    ".\n");
		gFail += ({ ob });
	    	return 0;
	    }
	    ob->split_heap(i - 1);
	    return 1;
	}
    }

    return 1;
}

void
config_split(int new_num,mixed orig)
{
    ::config_split(new_num, orig);
    set_amount(orig->query_amount());
}

public void
enter_env(object dest, object from)
{
    ::enter_env(dest, from);
    
    if (!decayed)
        return ;
        
    if (function_exists("create_container", dest) == ROOM_OBJECT)
    {
        if (decay_alarm)
            return ;
            
        decay_alarm = set_alarm(itof(decay_time), 0.0, &total_decay());
    }
    else if (decay_alarm)
    {
        decay_time = ftoi(get_alarm(decay_alarm)[2]);
        remove_alarm(decay_alarm);
        decay_alarm = 0;
    }
}

public void
total_decay()
{
    tell_roombb(environment(this_object()), capitalize(short()) + " " +
        "rozklada" + (num_heap() > 1 ? "ja" : "") + " sie zupelnie.\n",
        ({}), this_object());
        
    remove_object();
}

void
destruct_object()
{
    if (leave_behind > 0)
    {
	set_heap_size(leave_behind);
    }
    else
    {
	add_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT, 1);
	set_alarm(1.0, 0.0, remove_object);
    }
}

/*
 * Function name: stat_object
 * Description:   This function is called when a wizard wants to get more
 *                information about an object.
 * Returns:       str - The string to write..
 */
string
stat_object()
{
    return ::stat_object() + "Jedzenie: " + food_amount + " (gramow).\n";
}

/*
 * Function name: query_recover
 * Description  : This function is called to see whether this object may
 *                recover. It will only function for food that has a
 *                real file rather than being cloned from /std/food.c
 *                since only the amount of food on the heap is saved.
 */
public string
query_recover()
{
    string file = MASTER;

    /* Don't recover bare /std/drinks since we only recover the amount of
     * drinks and no descriptions.
     */
    if (file == FOOD_OBJECT)
    {
	return 0;
    }

    return file + ":heap#" + num_heap() + "#";
}

/*
 * Function name: init_recover
 * Description  : This function is called when the food recovers.
 * Arguments    : string str - the recover argument.
 */
public void
init_recover(string str)
{
    string foobar;
    int    num;

    if (sscanf(str, "%sheap#%i#%s", foobar, num, foobar) == 3)
    {
	set_heap_size(num);
    }
}
