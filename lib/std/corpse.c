/*
 * /std/corpse.c
 *
 * This is a decaying corpse. It is created automatically
 * when a player or monster die.
 */

#pragma save_binary
#pragma strict_types

inherit "std/container";

#include <cmdparse.h>
#include <composite.h>
#include <files.h>
#include <filter_funs.h>
#include <language.h>
#include <macros.h>
#include <ss_types.h>
#include <stdproperties.h>
#include <wa_types.h>

/*
 * Prototypes
 */
public int get_leftover(string arg);
static int move_out(object ob);
void decay_fun();

#define DECAY_TIME	10

int             decay;
int             decay_id;
int 		known_flag; /* 1 - neverknown, 2 - alwaysknown */
string          *met_name, *nonmet_name, *nonmet_pname, *state_desc, 
			*pstate_desc;
mixed		leftover_list;
string file;

/*
 * Prototypes
 */
public int get_leftover(string arg);
void decay_fun();

void
init()
{
    ::init();

    add_action(get_leftover, "wyrwij");
    add_action(get_leftover, "wytnij");
}

nomask void
create_container()
{
    add_prop(OBJ_I_SEARCH_TIME, 2);
    add_prop(CONT_I_DONT_SHOW_CONTENTS, 1); /* Sami wypiszemy zawartosc */

    this_object()->create_corpse();

    decay = DECAY_TIME;
}

nomask void
reset_container()
{
    this_object()->reset_corpse();
}

void set_file(string str)
{
    file = str;
}

void
ustaw_imie_denata(string *n)
{
    int i;
    string *temp, *temp2;
    object pob;
    
    pob = previous_object();
    
    if (pob->query_prop(LIVE_I_NEVERKNOWN))
        known_flag = 1;
    else if (pob->query_prop(LIVE_I_ALWAYSKNOWN))
        known_flag = 2;
    else known_flag = 0;
    
    temp = ({ "cialo", "ciala", "cialu", "cialo", "cialem", "ciele", 
        "ciala", "cial", "cialom", "ciala", "cialami", "cialach" });
        
    temp2 = temp + ({});

    state_desc = temp[0..5];
    pstate_desc = temp[6..11];

    ustaw_nazwe(temp[0..5], temp[6..11], PL_NIJAKI_NOS);
    
    nonmet_name = allocate(6);
    nonmet_pname = nonmet_name + ({});
    met_name = nonmet_name + ({});
    
    if (known_flag != 2)
    {
        for (i = 0; i < 6; i++)
        {
            nonmet_name[i] = pob->query_nonmet_name(i);
            nonmet_pname[i] = pob->query_nonmet_pname(i);
        }

        for (i = 0; i < 6; i++)
        {
            temp2[i] = temp2[i] + " " + nonmet_name[PL_DOP];
            temp2[i+6] = temp2[i + 6] + " " + nonmet_pname[PL_DOP];
        }

        dodaj_nazwy( temp2[0..5], temp2[6..11], PL_NIJAKI_NOS);
    }

    if (known_flag != 1)
    {
        for (i = 0; i < 6; i++)
            met_name[i] = lower_case(n[i]);

        for (i = 0; i < 6; i++)
        {
            temp[i] = temp[i] + " " + met_name[PL_DOP];
            temp[i+6] = temp[i + 6] + " " + met_name[PL_DOP];
        }

        dodaj_nazwy( temp[0..5], temp[6..11], PL_NIJAKI_NOS);
    }

    temp = ({ "szczatki", "szczatkow", "szczotkom", "szczatki",
        "szczatkami", "szczatkach" });
    temp2 = temp + ({});

    dodaj_nazwy(temp, PL_MESKI_NOS_NZYW);
    
    if (known_flag != 2)
    {
        for (i = 0; i < 6; i ++)
            temp2[i] = temp2[i] + " " + nonmet_name[PL_DOP];

        dodaj_nazwy(temp2, PL_MESKI_NOS_NZYW);
    }

    if (known_flag != 1)
    {
        for (i = 0; i < 6; i ++)
        {
            temp[i] = temp[i] + " " + met_name[PL_DOP];
        }

        dodaj_nazwy(temp, PL_MESKI_NOS_NZYW);
    }

    ustaw_shorty( ({ "@@short_func|0", "@@short_func|1", "@@short_func|2",
        "@@short_func|3", "@@short_func|4", "@@short_func|5" }), 
        ({"@@pshort_func|0", "@@pshort_func|1", "@@pshort_func|2",
        "@@pshort_func|3", "@@pshort_func|4", "@@pshort_func|5" }), 
        PL_NIJAKI_NOS);
        
    set_long("@@long_func");
    
    change_prop(CONT_I_WEIGHT, 50000);
    change_prop(CONT_I_VOLUME, 50000);
    decay_id = set_alarm(60.0, 60.0, decay_fun);
}

/*
 * Nazwa funkcji : set_decay
 * Opis          : Ustawia poziom rozlozenia ciala. Cialo zaczyna rozkladac
 *		   sie na poziomie 10, zupelnie rozklada sie przy zejsciu na 0.
 * Argumenty     : int - stopien rozlozenia.
 */
void
set_decay(int d)
{
    decay = d;
}

/*
 * Nazwa funkcji : query_decay
 * Opis          : Zwraca stopien rozlozenia sie ciala. Cialo zaczyna rozkladac
 *		   na poziomie 10, zas zupelnie rozklada sie przy zejsciu
 *		   na 0.
 * Funkcja zwraca: int - stopien rozkladu ciala.
 */
int
query_decay()
{
    return decay;
}



varargs string
short_func(string prz)
{
    object pob;
    int przyp = atoi(prz);
    string str;
    
    pob = vbfc_caller();
    
    if (!pob || !query_ip_number(pob) || pob == this_object())
	pob = this_player();

    if (pob && pob->query_real_name() == lower_case(met_name[0]))
    {
        switch(query_rodzaj())
        {
             case PL_NIJAKI_NOS:
                switch (przyp)
		{
		    case PL_MIA:
		    case PL_BIE: str = "twoje"; break;
		    case PL_DOP: str = "twojego"; break;
		    case PL_CEL: str = "twojemu"; break;
		    case PL_NAR:
		    case PL_MIE: str = "twoim"; break;
		}
		break;
	    case PL_ZENSKI:
                switch (przyp)
		{
		    case PL_MIA:
		    case PL_BIE:
		    case PL_NAR: str = "twoja"; break;
		    case PL_DOP:
		    case PL_CEL:
		    case PL_MIE: str = "twojej"; break;
		}
        }
        
        return str + " " + state_desc[przyp];
    }
    else 
        if ( (known_flag != 1) && 
             ((known_flag == 2) || 
              (pob && pob->query_met(met_name[0]))) ) 
	    return state_desc[przyp] + " " + capitalize(met_name[PL_DOP]);
    else
	return state_desc[przyp] + " " + nonmet_name[PL_DOP];
}

string
pshort_func(string prz)
{
    object pob;
    int przyp = atoi(prz);

    pob = vbfc_caller();
    if (!pob || !query_ip_number(pob) || pob == this_object())
	pob = this_player();
    if (pob && pob->query_real_name() == lower_case(met_name[0]))
	return ({ "twoje", "twoich", "twoim", "twoje", "twoimi", 
            "twoich" })[przyp] + " " + pstate_desc[przyp];
    else
        if ( (known_flag != 1) && 
             ((known_flag == 2) || 
              (pob && pob->query_met(met_name[0]))) ) 
	return pstate_desc[przyp] + " " + capitalize(met_name[PL_DOP]);
    else
	return pstate_desc[przyp] + " " + nonmet_pname[PL_DOP];
}

string
long_func()
{
    object pob, *inv;
    string ret;

    pob = vbfc_caller();
    if (!pob || !query_ip_number(pob) || pob == this_object())
	pob = this_player();
    if (pob->query_real_name() == lower_case(met_name[0]))
	ret = "Jest to twoje wlasne cialo.\n";
    else if (pob->query_met(met_name[0]))
	ret = "Jest to martwe cialo " + capitalize(met_name[PL_DOP]) + ".\n";
    else
	ret = "Jest to martwe cialo " + nonmet_name[PL_DOP] + ".\n";

    inv = all_inventory(this_object());
    if (!sizeof(inv))
        return ret;
    return ret + "Zauwazasz przy " + query_zaimek(PL_MIE) + " " +
	FO_COMPOSITE_DEAD(inv, pob, PL_BIE) + ".\n";
}

/*
 * Nazwa funkcji : decay_fun
 * Opis          : Funkcja rozkladajaca cialo o jeden stopien. Przy zejsciu
 *		   na poziom 3 cialo rozklada sie na 'resztki'. Przy zejsciu
 *		   na 0 i te sie rozkladaja.
 */
void
decay_fun()
{
    object *ob, obj;
    string desc;
    int i, j, flag;

    if (--decay == 3)
    {
	ob = filter(all_inventory(this_object()), move_out);
        /* fix this to get singular/plural of 'appear' */
	i = ((sizeof(ob) != 1) ? sizeof(ob) :
             ((ob[0]->query_prop(HEAP_I_IS)) ? (int)ob[0]->num_heap() : 1));

	tell_roombb(environment(this_object()), QCSHORT(this_object(), PL_MIA) + 
            " rozklada sie, pozostawiajac po sobie " +
            (i ? "smetne resztki, w postaci " + COMPOSITE_DEAD(ob, PL_DOP)
               : "tylko smetne resztki") + ".\n", ({}), this_object());
               
	state_desc = ({ "sterta", "sterty", "stercie", "sterte", "sterta",
            "stercie" });

	pstate_desc = ({ "sterty", "stert", "stertom", "sterty",
            "stertami", "stertach" });

        ustaw_nazwe(state_desc, pstate_desc, PL_ZENSKI);
        
        ustaw_shorty( ({ "@@short_func|0", "@@short_func|1", "@@short_func|2",
            "@@short_func|3", "@@short_func|4", "@@short_func|5" }), 
            ({ "@@pshort_func|0", "@@pshort_func|1", "@@pshort_func|2",
            "@@pshort_func|3", "@@pshort_func|4", "@@pshort_func|5" }), 
            PL_ZENSKI);
        for (i = 0; i < 6; i++)
        {
            state_desc[i] = state_desc[i] + " rozkladajacych sie szczatkow";
            pstate_desc[i] = pstate_desc[i] + " rozkladajacych sie szczatkow";
        }
    }
    
    if (decay > 0)
	return;

    for (i = 0; i < sizeof(leftover_list); i++)
    {
	if (leftover_list[i][6])
	{
	    for (j = 0 ; j < leftover_list[i][4] ; j++)
	    {
		obj = clone_object(leftover_list[i][0]);
		obj->leftover_init(leftover_list[i][1], 
		  leftover_list[i][2], leftover_list[i][3],
		  query_prop(CORPSE_M_RACE), query_prop(CORPSE_I_RRACE) - 1,
		  leftover_list[i][6]);
		obj->move(environment(this_object()), 0);
		flag = 1;
	    }
	}
    }

    if (flag)
        tell_roombb(environment(this_object()),
              QCSHORT(this_object(), PL_MIA) +
                 " rozpada sie.\n", ({}), this_object());

    map(all_inventory(this_object()), move_out);
    remove_object();
}

static int
move_out(object ob)
{
    if (ob->move(environment(this_object())))
	return 0;
    else
	return 1;
}

void
remove_object()
{
    map(all_inventory(this_object()), move_out);

    ::remove_object();
}

varargs mixed
query_race(int przyp)
{
    return query_prop(CORPSE_M_RACE)[przyp];
}

/*
 * Function name: add_leftover
 * Description:   Add leftovers (at death) to the body.
 * Arguments:	  leftover - The leftover list to use.
 */
varargs public void
add_leftover(mixed list)
{
    leftover_list = list;
}

/*
 * Function name: query_leftover
 * Description:   Return the leftover list. If an organ is specified, that
 *		  actual entry is looked for, otherwise, return the entire
 *		  list.
 *		  The returned list contains the following entries:
 *		  ({ objpath, organ, nitems, vbfc, hard, cut})
 * Arguments:	  organ - The organ to search for W BIERNIKU.
 */
varargs public mixed
query_leftover(string organ)
{
    int i;

    if (!sizeof(leftover_list))
	return ({ });

    if (!strlen(organ))
	return leftover_list;

    for (i = 0 ; i < sizeof(leftover_list) ; i++)
	if (leftover_list[i][1][PL_BIE] == organ)
	    return leftover_list[i];
}

/*
 * Function name: remove_leftover
 * Description:   Remove a leftover entry from a body.
 * Arguments:	  organ - Which entry to remove W BIERNIKU.
 * Returns:       1 - Ok, removed, 0 - Not found.
 */
public int
remove_leftover(string organ)
{
    int i;

    if (!sizeof(leftover_list))
	return 0;

    for (i = 0 ; i < sizeof(leftover_list) ; i++)
	if (leftover_list[i][1][PL_BIE] == organ)
	    leftover_list = leftover_list[0..(i - 1)] +
		leftover_list[(i + 1)..(sizeof(leftover_list))];
}

/*
 * Function name: get_leftover
 * Description:   Get leftovers from the body.
 */
public int
get_leftover(string arg)
{
    mixed	corpses, leftovers;
    object 	*found, *weapons, theorgan;
    int		i, slash;
    string	organ, vb, fail;
    
    if (this_player()->query_prop(TEMP_STDCORPSE_CHECKED))
	return 0;

    vb = query_verb();
    
    notify_fail(capitalize(vb) + " co z czego?\n");  /* access failure */
    if (!arg)
	return 0;

    if (!parse_command(arg, environment(this_player()), "%s 'z' %i:" + PL_DOP,
		organ, corpses))
	return 0;

    if (!strlen(organ))
        return 0;

    if (this_player()->query_attack())
    {
        write("Jestes w trakcie walki - lepiej skoncentruj sie na niej, " +
            "bo inaczej bedziesz wygladac jak " + short(PL_MIA) + "!\n");
        return 1;
    }

    found = VISIBLE_ACCESS(corpses, "find_corpse", this_object());
    
    if (sizeof(found) == 1)
    {
	switch(vb)
	{
	case "wytnij":
	    slash = 0;
	    if (!sizeof(weapons = this_player()->query_weapon(-1)))
	    {
        	notify_fail("@@get_fail:" + file_name(this_object()) +
        	    "|Warto by trzymac w rece cos, czym mozna by wyciac, " +
        	    "nie uwazasz?\n@@");
        	return 0;
	        
	    }
	    
	    for (i = 0 ; i < sizeof(weapons) ; i++)
		if (weapons[i]->query_dt() & W_SLASH)
		    slash = 1;
		    
	    if (slash == 0)
            {
        	notify_fail("@@get_fail:" + file_name(this_object()) +
        	    "|Jesli chcesz cos wyciac, poszukaj lepiej czegos " +
        	    "ostrzejszego.\n@@");
        	return 0;
            }

	    break;

	case "wyrwij":
	default:
	    slash = 0;
	    break;
	}

	if (vb == "wytnij" && slash == 0)
	{
	    notify_fail("@@get_fail:" + file_name(this_object()) +
		"|Jesli chcesz cos wyciac, poszukaj lepiej czegos " +
		"ostrzejszego.\n@@");
	    return 0;
	}
	
	leftovers = found[0]->query_leftover(organ);
	if (!sizeof(leftovers) || leftovers[4] == 0)
	{
	    notify_fail("@@get_fail:" + file_name(this_object()) +
		"|W " + found[0]->short(PL_MIE) + 
		" nie ma niczego takiego.\n@@");
	    return 0;
	}

	if (leftovers[7] && slash == 0)
	{
	    notify_fail("Nie mozesz tego tak po prostu wyrwac. Uzyj noza, " +
	       "albo czegos w tym stylu...\n");
	    return 0;
	}

	if (strlen(leftovers[5]))
	    fail = check_call(leftovers[5]);

	if (strlen(fail))
	{
	    notify_fail("@@get_fail:" + file_name(this_object()) + "|" +
		fail + ".\n@@");
	    return 0;
	}

	if(leftovers[4]-- == 0)
	    remove_leftover(leftovers[1][PL_BIE]);

	theorgan = clone_object(leftovers[0]);
	theorgan->leftover_init(leftovers[1], leftovers[2], leftovers[3], 
	    found[0]->query_prop(CORPSE_M_RACE),
	    found[0]->query_prop(CORPSE_I_RRACE) - 1,
	    leftovers[6]);
	if (theorgan->move(this_player(), 0))
	    theorgan->move(environment(this_player()));
	
	this_player()->set_obiekty_zaimkow(({ theorgan }), found);
	
	vb = (vb == "wytnij" ? "wycina" : "wyrywa");
	
	saybb(QCIMIE(this_player(), PL_MIA) + " " + vb + " " + organ +
	    " z " + QSHORT(found[0], PL_DOP) + ".\n");
	write(capitalize(vb) + "sz " + organ + " z " + found[0]->short(PL_DOP) 
	    + ".\n");
	return 1;
    }

    set_alarm(0.5, 0.0, &(this_player())->remove_prop(TEMP_STDCORPSE_CHECKED));
    this_player()->add_prop(TEMP_STDCORPSE_CHECKED, 1);
    if (sizeof(found))
	notify_fail("@@get_fail:" + file_name(this_object()) +
	    "|Sprobuj bardziej sprecyzowac, o ktore cialo ci chodzi.\n@@");
    else
	notify_fail("@@get_fail:" + file_name(this_object()) +
	    "|Nie widzisz nigdzie w okolicy takiego ciala.\n@@");
    return 0;
}

string
get_fail(string fault)
{
    return fault;
}

public int
find_corpse(object ob)
{
    if ((function_exists("create_container") == CORPSE_OBJECT) &&
	((environment(ob) == this_player()) ||
	 (environment(ob) == environment(this_player()))))
    {
	return 1;
    }

    return 0;
}


/*
 * Function name: search_fun
 * Description:   This function is called when someone searches the corpse
 *		  as set up in create_container()
 * Arguments:	  player - The player searching
 *		  str    - If the player specifically said what to search for
 * Returns:       A string describing what, if anything the player found.
 */
string
search_fun(string str, int trail)
{
    mixed left;
    string *found;
    int i;

    left = query_leftover();
    found = ({});

    for (i = 0; i < sizeof(left); i++)
    {
	if (left[i][4] == 1)
	    found += ({ left[i][1][PL_BIE] });
	else if (left[i][4] > 1)
	    found += ({ LANG_SNUM(left[i][4], PL_BIE, left[i][3]) + " " + 
	    left[i][2][LANG_PRZYP(left[i][4], PL_BIE, left[i][3])] });
    }

    if (sizeof(found) > 0)
	return "Po dokladnym przeszukaniu ciala stwierdzasz, ze mogl" +
	    this_player()->koncowka("bys", "abys") + " wyrwac lub wyciac " +
	    COMPOSITE_WORDS(found) + ".\n";

    return ::search_fun(str, trail);
}
