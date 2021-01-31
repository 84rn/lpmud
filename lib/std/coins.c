/* 
 * /std/coins.c
 *
 * This is the heap object for coins.
 */

#pragma save_binary
#pragma strict_types
#pragma no_inherit

inherit "/std/heap";

#include <macros.h>
#include <money.h>
#include <language.h>
#include <stdproperties.h>
#include <ss_types.h>
#include <std.h>
#include <debug.h>

/*
 * Global variable. It holds the coin type.
 */
static string coin_type;

/*
 * Prototype.
 */
void set_coin_type(string str);

/*
 * Function name: create_coins
 * Description  : Called at creation of the coins. To create your own coins
 *                you must define this function.
 */
public void
create_coins()
{
    ustaw_nazwe( ({ "moneta", "monety", "monecie", "monete", "moneta",
        "monecie" }), ({ "monety", "monet", "monetom", "monety", "monetami",
        "monetach" }), PL_ZENSKI );
        
    set_heap_size(1);
    set_coin_type(MONEY_TYPES[0]);
}

/*
 * Function name: create_heap
 * Description  : Constructor. This will create the heap and set some stuff
 *                that we want. You may not mask this function. You have to
 *                use create_coins() to create your own coins.
 */
public nomask void
create_heap()
{
    add_prop(OBJ_M_NO_SELL, 1);

    create_coins();
    
    if (!query_prop(HEAP_S_UNIQUE_ID))
    {
	set_coin_type(MONEY_TYPES[0]);
    }
}

/*
 * Function name: reset_coins
 * Description  : In order to have some code executed when this heap of
 *                coins resets, mask this function. Notice that in order
 *                to make them reset, call enable_reset() from the function
 *                create_coins().
 */
public void
reset_coins()
{
}

/*
 * Function name: reset_heap
 * Description  : Called to make this heap reset. You may not mask this
 *                function, so use reset_coins() instead.
 */
public nomask void
reset_heap()
{
    reset_coins();
}

/*
 * Function name: query_auto_load
 * Description  : Coins are autoloading. This function is called to find
 *                out whether they are. It returns the coin type and the
 *                number of coins in this heap.
 * Returns      : string - the auto-load string.
 */
public string
query_auto_load()
{
#if 0
    /* If the player quits, we do not want him/her to drop the coins, so
     * we set the OBJ_M_NO_DROP property when the player saves. However,
     * if the player stays in the game, we remove the property immediately
     * again.
     */
    add_prop(OBJ_M_NO_DROP, 1);
    set_alarm(0.1, 0.0, &remove_prop(OBJ_M_NO_DROP));
#endif

    return (MASTER + ":" + num_heap() + "," + coin_type);
}

/* 
 * Function name: init_arg
 * Description  : Called when autoloading. It will set the type of coins
 *                and the number of coins in the heap.
 * Arguments    : string arg - the auto-load argument.
 */
void
init_arg(string arg)
{
    int sum;
    string ct;

    if (sscanf(arg, "%d,%s", sum, ct) == 2)
    {
	set_heap_size(sum);
	set_coin_type(ct);
    }
}

/*
 * Function name: short
 * Description  : This function is called to get the short description of
 *                these coins. We make it dependant on the intelligence of
 *                the onlooker and have special cases for different numbers
 *                of coins.
 * Arguments    : object for_object - the object that wants to know.
 * Returns      : string - the short string.
 */
public varargs string
short(mixed for_object, mixed przyp)
{
    int typ = member_array(coin_type, MONEY_TYPES),
	num = num_heap();
    string str;

    /* No elements in the heap == no show. */
    if (num < 1)
    {
	return 0;
    }

    /* No identifier: BAD coins. Remove them. */
    if (!strlen(query_prop(HEAP_S_UNIQUE_ID)))
    {
	set_alarm(0.1, 0.0, remove_object);
	
	return "ghost coins";
    }
    
    /* No onlooker, default to this_player(). */
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
        
    /* One coin, singular, not really a heap. */
    if (num < 2)
    {
	return oblicz_przym(MONEY_ODM[typ][0], MONEY_ODM[typ][1], przyp, 
	    PL_ZENSKI, 0) + " " + query_nazwa(przyp);
    }

    /* Less than a dozen, we see the number as a word. */
    if (num < 12)
    {
	return LANG_SNUM(num, przyp, PL_ZENSKI) + " " +
	    oblicz_przym(MONEY_ODM[typ][0], MONEY_ODM[typ][1], 
	    (przyp = LANG_PRZYP(num, przyp, PL_ZENSKI)), PL_ZENSKI,
	    1) + " " + query_pnazwa(przyp);
    }

    /* If we are smart enough, we can see the number of coins. */
    if (for_object->query_stat(SS_INT) / 2 > num)
    {
	return num + " " + oblicz_przym(MONEY_ODM[typ][0], 
	    MONEY_ODM[typ][1], (przyp = LANG_PRZYP(num, przyp, 
	    PL_ZENSKI)), PL_ZENSKI, 1) + " " + query_pnazwa(przyp);
    }

    /* Else, default to 'many' or to a 'huge heap'. */
    if (num < 1000)
    {
	switch(przyp)
	{
	    case PL_MIA: str = "wiele "; break;
	    case PL_DOP: str = "wielu "; break;
	    case PL_CEL: str = "wielu "; break;
	    case PL_BIE: str = "wiele "; break;
	    case PL_NAR: str = "wieloma "; break;
	    case PL_MIE: str = "wielu "; break;
	}

	return str + oblicz_przym(MONEY_ODM[typ][0],
	    MONEY_ODM[typ][1], PL_DOP, PL_ZENSKI, 1) + " " +
	    query_pnazwa(PL_DOP);
    }
    else
    {
	switch(przyp)
	{
	    case PL_MIA: str = "ogromny stos "; break;
	    case PL_DOP: str = "ogromnego stosu "; break;
	    case PL_CEL: str = "ogromnemu stosowi "; break;
	    case PL_BIE: str = "ogromny stos "; break;
	    case PL_NAR: str = "ogromnym stosem "; break;
	    case PL_MIE: str = "ogromnym stosie "; break;
	}
	
	return str + oblicz_przym(MONEY_ODM[typ][0], MONEY_ODM[typ][1], PL_DOP,
	    PL_ZENSKI, 1) + " " + query_pnazwa(PL_DOP);
    }
    
    return 0;
}

/*
 * Function name: long
 * Description  : This function will slip the short description into the
 *                long description. Money will always look like good
 *                money, but don't try to fool the shopkeepers with wooden
 *                coins ;-)
 * Returns      : string - the long description.
 */
varargs public mixed
long()
{
    int typ;
    
    if (num_heap() < 2)
    {
	return "Jest to " + short(PL_MIA) + " i wyglada na prawdziwa.\n";
    }
    else
    {
        typ = member_array(coin_type, MONEY_TYPES);
    
	return "Sa to " + oblicz_przym(MONEY_ODM[typ][0], MONEY_ODM[typ][1],
	    PL_MIA, PL_ZENSKI, 1) + " " + query_pnazwa(PL_MIA) + 
	    ". Wygladaja na prawdziwe.\n";
    }
}

/*
 * Function name: set_coin_type
 * Description  : Set the type of coins we have here. Update all necessary
 *                properties with respect to the coins.
 * Arguments    : string str - the coin type to set.
 */
public void  
set_coin_type(string str)
{
    int x, ix, old_ix;
    string log, nome, czas, *wywolania;
    object env = environment();

    /* If this is one of the default coin types, set the weight, volume
     * and value correctly.
     */
    ix = member_array(str, MONEY_TYPES);
    
    if (ix < 0)
    {
        str = MONEY_TYPES[ix = 0];
    }
        
    mark_state();
    add_prop(HEAP_I_UNIT_VALUE, MONEY_VALUES[ix]);
    add_prop(HEAP_I_UNIT_WEIGHT, MONEY_WEIGHT[ix]);
    add_prop(HEAP_I_UNIT_VOLUME, MONEY_VOLUME[ix]);
    update_state();

    /* If there is a coin-type, remove that coin type as adjective. */
    if (coin_type)
    {
        old_ix = member_array(coin_type, MONEY_TYPES);
	remove_adj(MONEY_ODM[old_ix][0]);
	remove_name("_" + coin_type + " moneta_");
    }

    if (env &&
        (!interactive(env) || 
         !SECURITY->query_wiz_rank(env->query_real_name())))
    {
	czas = ctime(time())[4..];
	log = sprintf("%s   sct %4d %s->%s w ", czas[14..], num_heap(),
	    coin_type, str);

	if (interactive(env))
	    log += capitalize(env->query_real_name());
	else
	    log += file_name(env);
    
	wywolania = DEBUG_POBJS(0);
	x = -1;
	while (++x < sizeof(wywolania))
	{
	    wywolania = wywolania[0..x] + (wywolania[(x+1)..] - wywolania[x..x]);
	}

	log += " (";
	if (this_interactive())
	    log += getuid(this_interactive()) + "; ";
	log += implode(wywolania, "->") + "); ";
	czas = czas[0..11];
	x = -1;
	while (czas[++x] == ' ')
	    ;
	czas = czas[x..];
	log += czas;

	SECURITY->log_syslog("MONEY_LOG", (log + "\n"), MONEY_LOG_SIZE);
    }

    /* Set the new coin type and set it as an adjective. Also, we update
     * our identifier.
     */
    coin_type = str;
    add_prop(HEAP_S_UNIQUE_ID, MONEY_UNIQUE_NAME(coin_type));
    add_name("_" + str + " moneta_");
    dodaj_przym(MONEY_ODM[ix][0], MONEY_ODM[ix][1]);
}

public void
set_heap_size(int num)
{
    string str, typ, czas, *wywolania;
    object env = environment(this_object());
    int x, old_size = num_heap();
    
    ::set_heap_size(num);

    if (!env ||
	((num - old_size) < MONEY_LOG_LIMIT[coin_type]) ||
	(MASTER_OB(previous_object()) == COINS_OBJECT) ||
	(interactive(env) && SECURITY->query_wiz_rank(env->query_real_name())))
	return;

    czas = ctime(time())[4..];
    switch(coin_type)
    {
	case "srebro":  typ = "sr"; break;
	case "zloto":   typ = "zl"; break;
	case "mithryl": typ = "MT"; break;
	default:	    typ = "md";
    }

    str = sprintf("%s   shs %4d->%-4d %2s w ", czas[14..], old_size, num, typ);

    if (interactive(env))
	str += capitalize(env->query_real_name());
    else
	str += file_name(env);
    
    wywolania = DEBUG_POBJS(0);
    x = -1;
    while (++x < sizeof(wywolania))
    {
	wywolania = wywolania[0..x] + (wywolania[(x+1)..] - wywolania[x..x]);
    }

    str += " (";
    if (this_interactive())
        str += getuid(this_interactive()) + "; ";
    
    str += implode(wywolania, "->") + "); ";

    czas = czas[0..11];
    x = -1;
    while (czas[++x] == ' ')
	;
    czas = czas[x..];
    str += czas;

    SECURITY->log_syslog("MONEY_LOG", (str + "\n"), MONEY_LOG_SIZE);
}

/*
 * Function name: query_coin_type
 * Description  : Return what type of coins we have.
 * Returns      : string - the coin type.
 */
public string
query_coin_type()
{
    return coin_type;
}

/*
 * Function name: config_split
 * Description  : When a part of this heap is split, we make sure the new
 *                heap is made into the correct type of coins as well by
 *                setting the coin type to the coin type of the heap we are
 *                being split from.
 * Arguments    : int new_num - the number of coins in this new heap.
 *                object orig - the heap we are split from.
 */
public void
config_split(int new_num, object orig)
{
    ::config_split(new_num, orig);

    set_coin_type(orig->query_coin_type());
}

public string
appraise_value(int num)
{
    int value, skill, seed;
    value = cut_sig_fig(appraise_number(num) * query_prop(HEAP_I_UNIT_VALUE),
	2);

    return value + " miedziak" + (value == 1 ? "a" :
	(value%10 <= 4 && value%10 >= 2 && value%100 != 1 ? "i" : "ow"));
}

public int
appraise_number(int num)
{
    int value = num_heap();
    
    if (value < max(12, this_player()->query_stat(SS_INT) / 2))
	return value;
    else
	return ::appraise_number(num);
}

/*
 * Function name: stat_object
 * Description  : When a wizard stats this heap of coins, we add the coin
 *                type to the information.
 * Returns      : string - the stat-description.
 */
public string
stat_object()
{
    return ::stat_object() + "Typ monety: " + coin_type + "\n";
}

/*
 * Function name: move
 * Description  : Make sure moving of money is logged if the amount is
 *                larger than a certain amount.
 * Arguments    : see move in /std/object.c
 * Returns      : see move in /std/object.c
 */
varargs nomask public int
move(mixed dest, mixed subloc)
{
    string str, str2, czas, importance, typ, *wywolania;
    object env = environment(),
           prev = previous_object();
    int rval = ::move(dest, subloc);
    int x;

    /* If there was an error moving or if the limit is not high enough, do
     * not log.
     */
    if (rval ||
    	((num_heap() < MONEY_LOG_LIMIT[coin_type] &&
    	  !SECURITY->query_wiz_rank(env->query_real_name()))))
    {
	return rval;
    }

    if (MASTER_OB(prev) == COINS_OBJECT) // split_heap(), itp.
    {
        return 0;
    }

    if (stringp(dest))
    {
	dest = find_object(dest);
    }

    /* No destination means coins are destructed. That is not interesting.
     * If the coins enter a wizard that is not interesting either.
     */
    if (!objectp(dest) ||
	(interactive(dest) && 
	 SECURITY->query_wiz_rank(dest->query_real_name())))
    {
	return 0;
    }

/*
    if (!interactive(this_object()) && 
        (previous_object() == dest)) 
        return rval;
*/

    czas = ctime(time())[4..];
    switch(coin_type)
    {
        case "srebro":  x = 12; typ = "sr"; break;
        case "zloto":   x = 240; typ = "zl"; break;
        case "mithryl": x = 24000; typ = "MT"; break;
        default: x = 1; typ = "md";
    }
    x *= num_heap();
    
    if (x >= 24000)
	importance = "****";
    else if (x >= 3600)
	importance = " ++ ";
    else
	importance = "    ";
	
    str = czas[14..];
    str2 = sprintf("%4d %2s %4s ", num_heap(), typ, importance);
        
    if ((interactive(dest)) && 
        (calling_function() == "load_auto_obj") && 
        (previous_object() == dest))
    {
        str += " login " + str2 + capitalize(dest->query_real_name());
        str2 = "";
    }
    else
    {
        str += "  move " + str2;
        if (objectp(env))
        {
            if (interactive(env))
            {
                if (SECURITY->query_wiz_rank(env->query_real_name()))
                    str += upper_case(env->query_real_name());
                else
                    str += capitalize(env->query_real_name());
            }
            else
                str += file_name(env);
            
	    str2 = "";
        }
        else
        {
	    str += "void";
	    wywolania = DEBUG_POBJS(0);
	    x = -1;
	    while (++x < sizeof(wywolania))
	    {
	        wywolania = wywolania[0..x] +
	            (wywolania[(x+1)..] - wywolania[x..x]);
	    }
	    
            str2 = " (" + implode(wywolania, "->") + ")";
	}
	
        str += " -> " + (interactive(dest) ? 
            capitalize(dest->query_real_name()) : file_name(dest));
    }

    czas = czas[0..11];
    x = -1;
    while (czas[++x] == ' ')
        ;
        
    czas = czas[x..];

    /* Log the transation. */
    SECURITY->log_syslog("MONEY_LOG", (str + str2 + "; " + czas + "\n"),
	MONEY_LOG_SIZE);

    return 0;
}
