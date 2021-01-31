/*
  /std/heap.c

  This is a heap object for things like coins, matches and such stuff.

   Defined functions and variables:
*/

#pragma save_binary
#pragma strict_types

inherit "/std/object";

#include <macros.h>
#include <stdproperties.h>
#include <ss_types.h>
#include <language.h>

/*
 * Protoypes.
 */
void count_up(float delay, int amount, int counted);
int count(string str);
int stop(string str);
int heap_volume();
int heap_weight();
int heap_value();
int heap_light();

static int      item_count,	/* Number of items in the heap */
                leave_behind;	/* Number of items to leave behind */
static private int count_alarm,
                   gNo_merge;
static private mapping gOwn_props;

/*
 * Description: The stadard create_object
 */
nomask void
create_object()
{
    add_prop(OBJ_I_WEIGHT, heap_weight);
    add_prop(OBJ_I_VOLUME, heap_volume);
    add_prop(OBJ_I_VALUE,  heap_value);
    add_prop(OBJ_I_LIGHT,  heap_light);
    add_prop(HEAP_I_IS, 1);
    gOwn_props = obj_props;
    this_object()->create_heap();
}

/*
 * Description: The standard reset_object
 */
public nomask void
reset_object()
{
    this_object()->reset_heap();
}

/*
 * Description: This function is called each time the object 'meet' another
 *		object.
 */
void
init()
{
    add_action(count, "policz");
}

/*
 * Description: The weight of the heap, used by OBJ_I_WEIGHT
 */
public int
heap_weight()
{
    return query_prop(HEAP_I_UNIT_WEIGHT) * (item_count - leave_behind);
}

/*
 * Description: The volume of the heap, used by OBJ_I_VOLUME
 */
public int
heap_volume()
{
    return query_prop(HEAP_I_UNIT_VOLUME) * (item_count - leave_behind);
}

/*
 * Description: The value of the heap, used by OBJ_I_VALUE
 */
public int
heap_value()
{
    return query_prop(HEAP_I_UNIT_VALUE) * (item_count - leave_behind);
}

/*
 * Description: The lightvalue of the heap, used by OBJ_I_LIGHT
 */
public int
heap_light()
{
    return query_prop(HEAP_I_UNIT_LIGHT) * (item_count - leave_behind);
}

/*
 * Description: Called to restore the heap to its origional state
 */
public void
restore_heap()
{
    mark_state();
    leave_behind = 0;
    update_state();
}

/*
 * Description: Set the size of the heap to num.
 */
public void
set_heap_size(int num)
{
    mark_state();

    if(num <= 0)
    {
	set_alarm(1.0, 0.0, remove_object); 
	num = 0;
    }
    item_count = num;

    /*
     * We must update the weight and volume of what we reside in
     */
    update_state();
}

/*
 * Description: Returns the size of the heap.
 */
public int
num_heap()
{
    return item_count - leave_behind;
}

/*
 * Description: Called before a pending move of a part of the heap
 *              The heap size / volume is set to the part to be moved
 */
public int
split_heap(int num)
{
    mark_state();
    if (item_count <= num)
    {
	leave_behind = 0;
	/* It is ok to return here without updating the state as the
	   entire heap will be moved.
	 */
	return item_count;
    }
    leave_behind = item_count - num;
    set_alarm(0.1, 0.0, restore_heap);
    update_state();
    return num;
}

/*
 * Description: Get the short description for the heap.
 */
public varargs string
short(mixed for_object, mixed przyp)
{
    string str, sh;
    int rodzaj, num;
    
    if (!objectp(for_object))
    {
        if (intp(for_object))
            przyp = for_object;
        else if (stringp(for_object))
            przyp = atoi(for_object);
        
        for_object = this_player();
    }
    else
        if (stringp(przyp))
            przyp = atoi(przyp);

    sh = ::short(for_object, przyp);

    if (!strlen(query_prop(HEAP_S_UNIQUE_ID)))
    {
	set_alarm(0.1, 0.0, remove_object);
	return "ghost " + sh;
    }

    num = num_heap();

    if (num < 1) return 0;
    if (num == 1) return sh;

    rodzaj = this_object()->query_rodzaj();

    if (for_object->query_stat(SS_INT) / 2 > num)
    {
	str = this_object()->plural_short(for_object,
	    LANG_PRZYP(num, przyp, query_rodzaj()));

	if (num <= 10)
 	    return LANG_SNUM(num, przyp, rodzaj) + " " + str;
	else
	    return num + " " + str;
    }
    
    str = this_object()->plural_short(for_object,
	LANG_PRZYP(num, PL_DOP, query_rodzaj()));

    if (num < 1000)
	switch(przyp)
	{
	    case PL_MIA: return "wiele " + str; break;
	    case PL_DOP: return "wielu " + str; break;
	    case PL_CEL: return "wielu " + str; break;
	    case PL_BIE: return "wiele " + str; break;
	    case PL_NAR: return "wieloma " + str; break;
	    case PL_MIE: return "wielu " + str; break;
	}
    else
	switch(przyp)
	{
	    case PL_MIA: return "ogromny stos " + str; break;
	    case PL_DOP: return "ogromnego stosu " + str; break;
	    case PL_CEL: return "ogromnemu stosowi " + str; break;
	    case PL_BIE: return "ogromny stos " + str; break;
	    case PL_NAR: return "ogromnym stosem " + str; break;
	    case PL_MIE: return "ogromnym stosie " + str; break;
	}
}

/*
 * Description: Called when heap leaves it's environment
 */
public void
leave_env(object env, object dest)
{
    object          ob;

    if (!leave_behind)
	return;

    if (item_count <= leave_behind)
	return;

    if (!geteuid(this_object()))
	seteuid(getuid(this_object()));

    ob = CLONE_COPY;
    ob->config_split(leave_behind, this_object());
    ob->move(env, 1);
    item_count -= leave_behind;
    leave_behind = 0;
}

/*
 * Description: Called when heap enters and environment
 */
public void
enter_env(mixed env, object old)
{
    object *ob;
    int i;

    if (!objectp(env))
	env = find_object(env);
    ob = filter(all_inventory(env) - ({ this_object() }),
		&->query_prop(HEAP_I_IS));
    
    for (i = 0; i < sizeof(ob); i++)
    {
	if (!gNo_merge && !ob[i]->query_prop(OBJ_I_HIDE) &&
	    !ob[i]->query_prop(OBJ_I_INVIS) &&
	    !ob[i]->query_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT) &&
	    query_prop(HEAP_S_UNIQUE_ID) == ob[i]->query_prop(HEAP_S_UNIQUE_ID))
	{
	    ob[i]->set_heap_size(item_count + ob[i]->num_heap());
	    
	    /* item_count = 0; No do, we need the short desc.
	       
	       As we need the shortdesc to be correct for the duration
	       of this command, ie 'Blabla drops four something' we
	       must keep this object around intact.
	       */
	    
	    leave_behind = 0;
	    move(ob[i], 1); /* Better place to move it? void? */
	    set_alarm(0.1, 0.0, remove_object);
	    return;
	}
    }
}

/*
 * Function name: set_no_merge
 * Description:   Make sure that the heap wont merge
 */
public void
set_no_merge(int i)
{
    if (i && !gNo_merge)
	set_alarm(0.1, 0.0, &set_no_merge(0));

    gNo_merge = i;
}

public void
force_heap_merge()
{
    object *ob;
    int i;

    ob = filter(all_inventory(environment(this_object())) -
		({ this_object() }), &->query_prop(HEAP_I_IS));
    
    for (i = 0; i < sizeof(ob); i++)
    {
	if (!ob[i]->query_prop(OBJ_I_HIDE) &&
	    !ob[i]->query_prop(OBJ_I_INVIS) &&
	    !ob[i]->query_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT) &&
	    query_prop(HEAP_S_UNIQUE_ID) == ob[i]->query_prop(HEAP_S_UNIQUE_ID))
	{
	    ob[i]->set_heap_size(item_count + ob[i]->num_heap());
	    
	    remove_object();
	}
    }
}

/*
 * Function name: query_prop_map
 * Description:   Returns mapping containg all props and their values.
 * Returns:       The obj_props mapping.
 */
public nomask mapping
query_prop_map()
{
    return secure_var(obj_props);
}

/*
 * Description: This is called before inserting this heap into the game
 */
void
config_split(int new_num, object orig)
{
    int 	index, ix;
    string 	*shorty = allocate(6), 
    		*pshorty = allocate(6),
    		ob_name;
    mixed	org_przym;
    
    item_count = new_num;

    index = -1;
    while(index++ < 5)
    {
	set_name(orig->parse_command_id_list(index), index,
		 orig->query_real_rodzaj(1, index));
	set_pname(orig->parse_command_plural_id_list(index), index,
		 orig->query_real_prodzaj(1, index));

	shorty[index] = orig->short(orig, index);
	pshorty[index] = orig->plural_short(orig, index);
    }

    org_przym = orig->query_przymiotniki();

    index = sizeof(org_przym[0]);

    while(--index >= 0)
        dodaj_przym(org_przym[0][index], org_przym[1][index]);
    
    ob_name = OB_NAME(orig);
    for (ix = 0; ix < 6; ix++)
	remove_name(ob_name, ix);

    ustaw_shorty(shorty, pshorty, orig->query_rodzaj());

    set_long(orig->query_long());

    obj_props = orig->query_prop_map() + gOwn_props;
}

public varargs string 
koncowka(string meski = "", string zenski = "", string nijaki = "", 
    string mos = "", string mnos = "")
{
    int rodzaj = this_object()->query_real_rodzaj();
    int num;
    
    if (rodzaj < 0)
    {
	if (rodzaj < 0)
	    rodzaj = -rodzaj;

	if (rodzaj == (PL_MESKI_OS - 1))
	    return mos;
	else
	    return mnos;
    }

    rodzaj -= 1;
    num = num_heap();

    if (num > 1)
    {
	if ((num % 10 > 1) && (num % 10 < 5) && (num / 10 != 1) &&
	   (num > 10 || (rodzaj != PL_NIJAKI_OS && rodzaj != PL_NIJAKI_NOS)))
	{
	    if (rodzaj == PL_MESKI_OS)
		return mos;
	    else
		return mnos;
	}
    }
    switch (this_object()->query_rodzaj())
    {
	case PL_MESKI_OS:
	case PL_MESKI_NOS_ZYW:
	case PL_MESKI_NOS_NZYW:
	    return meski;
	case PL_ZENSKI:
	    return zenski;
	default:
	    if (!strlen(nijaki))
	return meski;
	    else
		return nijaki;
    }
}

/*
 * Description: Function called when player gives 'count' command
 */
public int
count(string str)
{
    string *tmp;
    float delay;
    int intg;

    if ( (!check_seen(this_player())) || (!CAN_SEE_IN_ROOM(this_player())) )
    {
        return 0;
    }

    if (this_player()->query_attack())
    {
	notify_fail("Jestes zbyt zajet" + this_player()->koncowka("y", "a") +
	    " walka, zeby moc liczyc!\n");
	return 0;
    }

    if (!stringp(str) ||
	!parse_command(str, ({ this_object() }), "%i:3", tmp))
    {
	notify_fail("Policz co?\n");
        return 0;
    }

    intg = this_player()->query_stat(SS_INT);
    delay = 60.0 / itof(intg);
    /* count_arg contains interval, coins per count and total so far */
    count_alarm = set_alarm(delay, 0.0, &count_up(delay, 5 * (intg / 10 + 1), 0));
    add_action(stop, "", 1);
    return 1;
}

/*
 * Description: Stop counting the items in the heap.
 */
varargs int
stop(string str)
{
    if (query_verb() == "przestan")
    {
        update_actions();
	remove_alarm(count_alarm);
        write("Przestajesz liczyc.\n");
        return 1;
    }
    else if (str == "done")
    {
        update_actions();
        return 1;
    }
    else
    {
        write("Jestes zajet" + this_player()->koncowka("y", "a") +
           " liczeniem. Musisz przestac, jesli chcesz zrobic cos innego.\n");
        return 1;
    }
}

/*
 * Description: Count some more, how much depends on intelligence of player
 */
void
count_up(float delay, int amount, int counted)
{
    counted += amount;
    if (counted < num_heap())
    {
        write(counted + "\n");
	count_alarm = set_alarm(delay, 0.0, &count_up(delay, amount, counted));
    }
    else
    {
        write("Doliczyl" + this_player()->koncowka("es", "as") + 
            " sie " + num_heap() + " " + 
            (num_heap() > 1 ? plural_short(PL_DOP) : short(PL_DOP)) + ".\n");
        stop("done"); /* 'done' wkodowane w funkcje stop() */
	count_alarm = 0;
    }
}

/*
 * Function name: appraise_number
 * Description:   This function is called when someon tries to appraise number 
 *                of pieces in heap of this object.
 * Arguments:     num - use this number instead of skill if given.
 */
public int
appraise_number(int num)
{
    int value = num_heap(),
	skill, seed;
    
    /* Poprawka na to co short() pokazuje. */
    if (this_player()->query_stat(SS_INT) / 2 > value)
	return value;

    if (!num)
	skill = this_player()->query_skill(SS_APPR_OBJ);
    else
	skill = num;

/*
 * To nie mialo wiekszego sensu... /Alvin.
 *    skill = 1000 / (skill + 1);
 */
    skill = ((100 - skill) * 6 / 10) + 1;
    value = num_heap();
    sscanf(OB_NUM(this_object()), "%d", seed);
    skill = random(skill, seed);
    value += ((skill % 2 ?: -1) * skill * value / 100);
    return cut_sig_fig(value, ((value > 50 && value < 100) ? 1 : 2));
}

public string
appraise_value(int num)
{
    int value, skill, seed;

    if (!num)
	skill = this_player()->query_skill(SS_APPR_VAL);
    else
	skill = num;

    skill = ((100 - skill) * 6 / 10) + 1;
    value = query_prop(HEAP_I_UNIT_VALUE);
    sscanf(OB_NUM(this_object()), "%d", seed);
    skill = random(skill, seed);
    value = max(cut_sig_fig(value + ((skill % 2 ?: -1) * skill * value / 100),
	2), 1);
	
    value = cut_sig_fig(appraise_number(num) * value, 2);

    return value + " miedziak" + (value == 1 ? "a" :
	(value%10 <= 4 && value%10 >= 2 && value%100 != 1 ? "i" : "ow"));
}

/*
 * Description: Called when player tries to appraise the heap.
 */
public void
appraise_object(int num)
{
    string str;
    int ile;

    write(this_object()->long(0, this_player()) + "\n");
    write("Oceniasz, ze " + short(PL_MIA) + " waz" +
	koncowka("y", "y", "y", "a", "a") + " " + appraise_weight(num) +
	", zas " + query_zaimek(PL_DOP, 0, (num_heap() > 1)) +
	" objetosc wynosi " + appraise_volume(num) + ".\n");

    write("Wydaje ci sie, ze jest " + (ile = appraise_number(num)) + 
        (ile > 1 ? " sztuk wartych " : " sztuka warta " ) + 
        appraise_value(num) + ".\n");
}

/*
 * Function name: stat_object
 * Description:   This function is called when a wizard wants to get more
 *                information about an object.
 * Returns:       str - The string to write..
 */
public string
stat_object()
{
    string str;

    str = ::stat_object();

    str += "Liczba sztuk: " + num_heap() + ".\n";

    return str;
}

/*
 * Function name: add_prop_obj_i_value
 * Description:   Hook to avoid wrong settings of OBJ_I_VALUE in a heap
 * Arguments:     val - The value OBJ_I_VALUE is intended to be set to
 * Returns:	  1 - If OBJ_I_VALUE shouldn't get this new setting.
 */
public int
add_prop_obj_i_value(mixed val)
{
    if (!functionp(val))
    {
	add_prop(HEAP_I_UNIT_VALUE, val);
	return 1;
    }

    return 0;
}

/*
 * Function name: add_prop_obj_i_volume
 * Description:   Hook to avoid wrong settings of OBJ_I_VOLUME in a heap
 * Arguments:     val - The value OBJ_I_VOLUME is intended to be set to
 * Returns:       1 - If OBJ_I_VOLUME shouldn't get this new setting.
 */
public int
add_prop_obj_i_volume(mixed val)
{
    if (!functionp(val))
    {
	add_prop(HEAP_I_UNIT_VOLUME, val);
	return 1;
    }

    return 0;
}

/*
 * Function name: add_prop_obj_i_weight
 * Description:   Hook to avoid wrong settings of OBJ_I_WEIGHT in a heap
 * Arguments:     val - The value OBJ_I_WEIGHT is intended to be set to
 * Returns:       1 - If OBJ_I_WEIGHT shouldn't get this new setting.
 */
public int
add_prop_obj_i_weight(mixed val)
{
    if (!functionp(val))
    {
	add_prop(HEAP_I_UNIT_WEIGHT, val);
	return 1;
    }

    return 0;
}
