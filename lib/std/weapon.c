/*
 * /std/weapon.c
 *
 * Contains all routines relating to weapons of any kind.
 * Defined functions and variables:
 *
 * set_hit(int)		Sets the weapon "to hit" value.
 *
 * set_pen(int)		Sets the weapon penetration value.
 *
 * set_pm(int *)	Sets some modifiers for the penetration on the weapon.
 *			only useful if weapon is of multiple damage type.
 *
 * set_wt(type)		Set the weapon type.
 *			If not defined, set to default.
 *
 * set_dt(type)		Set the damage type.
 *			If not defined, set to default.
 *
 * set_hands(which)	Set the hand(s) used to wield the weapon.
 *			If not defined, set to default.
 *
 * set_wf(obj)		Sets the name of the object that contains the function
 *			to call for extra defined wield() and unwield()
 *			functions.
 *
 * query_wielded()      Returns the wielder if this object is wielded
 *
 * Weapons recover by default.
 */

#pragma save_binary
#pragma strict_types

inherit "/std/object";

#include <cmdparse.h>
#include <files.h>
#include <formulas.h>
#include <language.h>
#include <macros.h>
#include <ss_types.h>
#include <stdproperties.h>
#include <wa_types.h>

static int      wep_hit,	/* Weapon to hit */
                wep_pen,	/* Weapon penetration */
                wep_wt,		/* Weapon type */
                wep_dt,		/* Damage type */
                wep_hands,	/* How many hands the weapon takes */
                wielded,	/* Wielded or not */
		wielded_in_hand,/* Wielded in which hand */
		*m_pen,		/* Modifiers for the wep_pen */
		hits,		/* No of hits the weapon have made */
		dull,		/* How dull the weapon has become */
		corroded,	/* Corrotion on the weapon */
		repair_dull,	/* How much dullness we have repaired */
		repair_corr,	/* How much corrosion we have repaired */
		likely_corr,	/* How likely will this weapon corrode */
		likely_break,	/* How likely will this weapon break? */
		likely_dull,	/* How likely will it be dulled by battle? */
		max_value;	/* Value of weapon at prime condition */
static object   wielder,	/* Who is holding it */
                wield_func;	/* Object that defines extra wield/unwield */
static string   gFail;          /* Acc error messages when wielding */
static private int	will_not_recover;	/* True if it won't recover */

/*
 * Prototypes
 */
string	wep_condition_desc();
void	update_prop_settings();
public nomask int wield_wep(string what);
public nomask int unwield_wep(string what);
varargs void remove_broken(int silent = 0);
static int find_wep(object wep, string ploc);

/*
 * Function name: create_weapon
 * Description  : Create the weapon. You must define this function to
 *                construct the weapon.
 */
public void
create_weapon()
{
}

/*
 * Function name: create
 * Description  : Create the weapon. As this function is declared nomask
 *                you must use the function create_weapon to actually
 *                construct it. This function does some basic initializing.
 */
public nomask void
create_object()
{
    wep_wt = F_WEAPON_DEFAULT_WT;
    wep_dt = F_WEAPON_DEFAULT_DT;
    likely_dull = 10;
    likely_corr = 10;
    likely_break = 10;
    wep_hands = F_WEAPON_DEFAULT_HANDS;
    ustaw_nazwe( ({ "bron", "broni", "broni", "bron", "bronia", "broni" }),
        ({ "bronie", "broni", "broniom", "bronie", "bronmi", "broniach" }),
        PL_ZENSKI);
    wielded = 0;
    add_prop(OBJ_I_VALUE, "@@query_value");
    add_prop(OBJ_I_VOLUME, 500);
    add_prop(OBJ_I_WEIGHT, 200);
    m_pen = ({ 0, 0, 0 });

    /* If this will be true, the weapon will not recover on reboot and is
     * lost after the reboot.
     */
    will_not_recover = (random(100) < PERCENTAGE_OF_RECOVERY_LOST);

    create_weapon();
    
    update_prop_settings();
}

/*
 * Function name: reset_weapon
 * Description  : This function can be used to reset the weapon at a regular
 *                interval. However, if you do so, you must also call the
 *                function enable_reset() from your create_weapon() function
 *                in order to start up the reset proces.
 */
public void
reset_weapon()
{
}

/*
 * Function name: reset_object
 * Description  : Reset the weapon. If you want to make the weapon reset
 *                at a certain interval you must use reset_weapon as this
 *                function is nomasked.
 */
public nomask void
reset_object()
{
    reset_weapon();
}

/*
 * Function name: init
 * Description  : Initialize some weapon related actions. If you want to
 *                mask this function to add some actions for yourself, you
 *                _must_ make a call to ::init();
 */
public void
init()
{
    ::init();

    add_action(wield_wep, "dobadz");
    add_action(unwield_wep, "opusc");
    add_action("old_wield_wep", "wield");
    add_action("old_unwield_wep", "unwield");
}

/*
 * Function name: short
 * Description  : The short description. We modify it when the weapon is
 *                broken. There is a little caveat if the wizard has not
 *                set a short description since it will double the
 *                adjective 'broken'.
 * Arguments    : object for_obj - the object that wants to know.
 * Returns      : string - the description.
 */
public varargs string
short(mixed for_obj, mixed przyp)
{
    if (!objectp(for_obj))
    {
        if (intp(for_obj))
            przyp = for_obj;
        else if (stringp(for_obj))
            przyp = atoi(for_obj);
        
        for_obj = previous_object();
    }
    else
        if (stringp(przyp))
            przyp = atoi(przyp);

    if (query_prop(OBJ_I_BROKEN))
    {
	return oblicz_przym("zlamany", "zlamani", przyp, 
	    this_object()->query_rodzaj(), 0) + " " + 
	    ::short(for_obj, przyp);
    }

    return ::short(for_obj, przyp);
}

/*
 * Function name: plural_short
 * Description  : The plural short description. When the weapon is broken,
 *                we alter it. See 'short' for details.
 * Arguments    : object for_obj - the object that wants to know.
 * Returns      : string - the description.
 */
public varargs string
plural_short(mixed for_obj, int przyp)
{
    string str;

    if (intp(for_obj)) 
    {
	przyp = for_obj;
	for_obj = this_player();
    }
    /* nie sprawdzamy czy przyp to int, bo nie ma makr do tej funkcji */
    
    str = ::plural_short(for_obj, przyp);


    /* We make this additional check for stringp(str) because if no plural
     * short has been set, we shoudln't alter it. The plural short will
     * be generated if no plural short has been set.
     */
    if (query_prop(OBJ_I_BROKEN) && stringp(str))
    {
	return oblicz_przym("zlamany", "zlamani", przyp, 
	    this_object()->query_rodzaj(), 1) + " " + str;
    }

    return str;
}

/*
 * Function name: long
 * Description  : The long description. We add the information about the
 *                condition to it.
 * Arguments    : string str - long of pseudo item or of the very object ?
 *		  object for_obj - the object that wants to know.
 * Returns      : string - the long description.
 */
public varargs string
long(string str, object for_obj)
{
    return ::long(str, for_obj) + (str ? "" : wep_condition_desc());
}

void
old_wield_wep()
{
    notify_fail("Komenda 'wield' zostala wycofana. Zamiast " +
        "niej mozesz uzyc 'dobadz'.\n");

    return 0;

}

/*
 * Function name: wield_wep
 * Description  : This function is called when the player executes the
 *                wield command.
 * Arguments    : string what - the command line argument.
 */
public nomask int
wield_wep(string str)
{
    object 	*a;
    
    notify_fail(capitalize(query_verb()) + " czego?\n");  /* access failure */
    	
    /*
     * This is an exception from the rule that we should do 'notify_fail'
     * and return 0, all the time. If we do then all weapons will run
     * this code, which is somewhat wastefull.
     */
    gFail = "";
    
    if (!str)
        str = "";
        
// ten warunek jest troche bez sensu... jesli parse_command srpawdzil
// to po co reszta... popatrzec na to kiedys.
    if (!parse_command(str, this_player(), "%i:" + PL_DOP, a) ||
        !(a = CMDPARSE_STD->normal_access(a, "wield_access", this_object(), 0))
        || !(a = filter(a, "wield_one_thing", this_object()))
        || !sizeof(a))
    {
	if (!strlen(gFail))
	    return 0;
	else
	    write(gFail);
    }
    
    this_player()->set_obiekty_zaimkow(a);
        
    gFail = 0;
    return 1;
}

/*
 * Function name: wield_access
 * Description  : This function is a check ensures that what we try to get
 *                is a weapon and that we carry it.
 * Arguments    : object what - the weapon to test.
 * Returns      : int 1/0 - true if the weapon is a true weapon that we
 *                          carry.
 */
nomask int
wield_access(object what)
{
    if (function_exists("create_object", what) != WEAPON_FILE)
	return 0;
    if (environment(what) == this_player())
	return 1;
    else
	return 0;
}

/*
 * Function name: wield_one_thing
 * Description  : This function is called for each weapon the player tries
 *                to wield to call the wield function in it. If it fails, a
 *                message is added to the fail-string.
 * Arguments    : object what - the object to wield
 * Returns      : int 1/0 - true if the weapon was wielded.
 */    
nomask int
wield_one_thing(object what)
{
    string fail;

    fail = what->wield_me();

    if (stringp(fail))
    {
	gFail += fail;
	return 0;
    }
    return 1;
}

/*
 * Function name: wield_me
 * Description  : When the player tries to wield this weapon, this function
 *                is called. If the player managed to wield the weapon,
 *                the message is printed.
 * Argumenty	: int cicho - gdy 1, nie wyswietla zadnych komunikatow.
 * Returns      : int 1  - success
 *                string - a fail message (nothing is printed).
 */
nomask varargs mixed
wield_me(int cicho)
{
    int wret, skill, pen;
    string penuse, wfail;

    if (wielded)
	return "Juz dzierzysz " + short(this_player(), PL_BIE) + ".\n";
    else if (this_player() != environment(this_object()))
	return "Musisz miec przy sobie " + short(this_player(), PL_BIE) + 
	  ", zeby moc " + koncowka("go", "ja", "je", "ich", "je") + 
	  " dobyc...\n";
    else if (query_prop(OBJ_I_BROKEN))
	return capitalize(short(this_player(), PL_MIA)) + " " + 
	    (query_tylko_mn() ? "sa" : "jest") + " zlaman" + 
	    koncowka("y", "a", "e", "i", "e") + 
	    " i nie nadaje sie juz do niczego.\n";

    wielder = this_player();

    /* 
     * Check for a hand to wield the weapon in.
     */
    wielded_in_hand = wep_hands;
    if (wep_hands != W_ANYH)
    {
	/* 
         * Anything in both hands
         */
	if (wielder->query_weapon(W_BOTH) && wep_hands < W_FOOTR)
	    return "Twoje rece sa juz zajete czym innym.\n";

	/* 
         * Anything in the specified hand
         */
	if (wielder->query_weapon(wep_hands))
	    return "Juz dzierzysz " + 
		   wielder->query_weapon(wep_hands)->short(wielder, PL_BIE) +
		   ".\n";
    
	if (wep_hands == W_BOTH &&
	    (wielder->query_slot(W_RIGHT) || 
	     wielder->query_slot(W_LEFT)))
	    return "Potrzebujesz obu rak, zeby moc " + 
	      koncowka("go", "ja", "go", "ich", "je") + " dobyc.\n";
    }
    else if (!wielder->query_slot(W_RIGHT))
	wielded_in_hand = W_RIGHT;

    else if (!wielder->query_slot(W_LEFT))
	wielded_in_hand = W_LEFT;

    else
	return "Nie masz wolnej reki, w ktora mog" + wielder->koncowka("lbys",
	    "labys") + " dobyc " + short(wielder, PL_DOP) + ".\n";

    if (stringp(wfail = wielder->wield(this_object())))
	return wfail;

    wret = 0;

    /*
     * A wield function in another object.
     */
    if ((!cicho) && 
        ((!wield_func) || (!(wret=wield_func->wield(this_object())))))
    {
	if (wielded_in_hand == W_BOTH)
	    write("Dobywasz oburacz " + short(wielder, PL_DOP) + ".\n");
	else if (wielded_in_hand < W_FOOTR)
	    write("Dobywasz " + (wielded_in_hand == W_RIGHT ? "prawa" : 
	          "lewa") + " reka " + short(wielder, PL_DOP) + ".\n");
	else
	    write("Dobywasz " + short(wielder, PL_DOP) + " " +
		  (wielded_in_hand == W_FOOTR ? "prawa" : "lewa") + 
		  " stopa.\n");

	saybb(QCIMIE(this_player(), PL_MIA) + " dobywa " + 
	      QSHORT(this_object(), PL_DOP) + ".\n");
    }
    if (intp(wret) && wret >= 0)
    {
	wielded = 1;
/*	set_adj("wielded"); */
	return 1;
    }

    if (stringp(wfail = wielder->unwield(this_object())))
    {
	/* This is serious and almost fatal, please panic! */
	wielded = 1;
/*	set_adj("wielded"); */
	return 1;
    }


    /*
     * If the wieldfunc returned a value <0 the we can not wield
     * likewise if it returned a string, but then we use that string
     * as error message.
     */
    if (stringp(wret))
	return wret;
    else 
	return "Nie mozesz dobyc " + short(wielder, PL_DOP) + "\n";
}

void
old_unwield_wep()
{
    notify_fail("Komenda 'unwield' zostala wycofana. Zamiast " +
        "niej mozesz uzyc 'opusc'.\n");

    return 0;
}

/*
 * Function name: unwield_wep
 * Description  : When the player uses the unwield command, this function
 *                is called.
 * Arguments    : string what - the command line argument.
 */
public nomask int
unwield_wep(string what)
{
    object *a;
    object wep;
    int    index = -1;
    int    size;
    
    notify_fail(capitalize(query_verb()) + " co?\n");  /* access failure */
    if (!what)
    {
	return 0;
    }

    /*
     * This is an exception from the rule that we should do 'notify_fail'
     * and return 0, all the time. If we do then all weapons will run
     * this code, which is somewhat wastefull.
     */
    if (!parse_command(what, all_inventory(this_player()), "%i:3", a))
    {
	return 0;
    }

    wep = a[0];
    a = (a & this_player()->query_weapon(-1));
    a = CMDPARSE_STD->normal_access( ({ wep }) + a, 0, 0, 1);

    if (!sizeof(a))
    {
	notify_fail("Nie dobywasz zadnej takowej broni.\n");
	return 0;
    }

    size = sizeof(a);
    while(++index < size)
    {
	a[index]->unwield_me();
    }
    
    this_player()->set_obiekty_zaimkow(a);

    return 1;
}

/*
 * Function name: unwield_me
 * Description  : When the player tries to unwield this weapon, this function
 *                is called. If the player managed to unwield the weapon,
 *                the message is printed.
 * Returns      : int 1  - success
 *                string - a fail message (nothing is printed).
 */
nomask int
unwield_me()
{
    mixed wret;

    if (!wielded || !wielder)
	return 0;

    wret = 0;

    /*
     * A unwield function in another object.
     */
    if ((!wield_func) || (!(wret = wield_func->unwield(this_object()))))
    {
	if (check_seen(this_player()))
	    write("Opuszczasz " + short(this_player(), PL_BIE) + ".\n");
	else
	    write("Opuszczasz cos.\n");
	saybb(QCIMIE(this_player(), PL_MIA) + " opuszcza " + 
	      QSHORT(this_object(), PL_BIE) + ".\n");
    }
    /*
     * If the wieldfunc returned a value < 0 then we can not unwield
     */
    if (intp(wret) && (wret >= 0))
    {
	wielder->unwield(this_object());
	wielded = 0;
/*	remove_adj("wielded"); */
	wielded_in_hand = wep_hands;
	return 1;
    }
    else if (stringp(wret))
    {
	write(wret);
	return 0;
    }
    {
	return 0;
    }
}

/*
 * Function name: find_wep
 * Description  : This filter function is used to find any weapon wielded
 *                in a particular location.
 * Arguments    : object wep  - the weapon to test.
 *                string ploc - the location to test for.
 * Returns      : int 1/0 - true if the weapon is wielded on that location.
 */
static int
find_wep(object wep, string ploc)
{
    int where;
    string a, b;

    ploc = "d " + ploc + " d";
    where = wep->query_attack_id();

    if (sscanf(ploc, "%slew%s", a, b) &&
	((where == W_RIGHT) ||
	 (where == W_FOOTR)))
	return 0;
    if (sscanf(ploc, "%spraw%s", a, b) &&
	((where == W_LEFT) ||
	 (where == W_FOOTL)))
	return 0;
    if (sscanf(ploc, "%srek%s", a, b) &&
	((where == W_FOOTL) ||
	 (where == W_FOOTR)))
	return 0;
    if (sscanf(ploc, "%sstop%s", a, b) &&
	((where == W_ANYH) ||
	 (where == W_LEFT) ||
	 (where == W_RIGHT) ||
	 (where == W_BOTH)))
	return 0;
    return 1;
}

/*
 * Function name: leave_env
 * Description  : When the weapon leaves a certain inventory this function
 *                is called. If the weapon is wielded, we unwield. If you
 *                mask this function you _must_ make a call to ::leave_env
 * Arguments    : object from - the object the weapon leaves.
 *                object desc - the destination of the weapon.
 */
void
leave_env(object from, object dest)
{
    ::leave_env(from, dest);

    if (!wielded)
	return;

    if ((!wield_func ||
    	 !wield_func->unwield(this_object())) &&
    	wielder)
    {
	tell_object(wielder, "Opuszczasz " + short(wielder, PL_BIE) + ".\n");
    }

    wielder->unwield(this_object());
/*  remove_adj("wielded"); */
    wielded = 0;
}

/*
 * Function name: set_hit
 * Description  : Set the to hit value in the weapon. This can only be done
 *                if the lock has not been set.
 * Arguments    : int class - the new weapon class.
 */
void
set_hit(int class)
{
    if (query_lock())
    {
	return;
    }

    wep_hit = class;
}

/*
 * Function name: query_hit
 * Description:   Query the to hit value in the weapon
 */
int query_hit() { return wep_hit; }

/*
 * Function name: set_pen
 * Description:   Set penetration of the weapon
 */
void
set_pen(int class)
{
    if (query_lock())
	return;			/* All changes has been locked out */

    wep_pen = class;
}

/*
 * Function name: query_pen
 * Description:   Query penetration of the weapon
 */
int query_pen() { return wep_pen; }

/*
 * Function name: set_pm
 * Description:   Set the modifiers for damage types.
 * Arguments:	  list, an array of modifiers like ({ impale, slash, bludgeon })
 *		  The sum of the modifiers should be 0 and a modifier for
 *		  a damage type that is not used should also be 0.
 */
void
set_pm(int *list)
{
    if (query_lock())
	return;

    if (F_LEGAL_AM(list))
	m_pen = list;
}

/*
 * Function name: query_pm
 * Description:   Query the modifiers of damage type
 */
int *query_pm() { return m_pen + ({}); }

/*
 * Function name; query_modified_pen
 * Description:   Query for pen modified for different damage types
 */
int *
query_modified_pen()
{
    int *m_pent, i, du, co, pen;

    du = dull - repair_dull;
    co = corroded - repair_corr;
    pen = this_object()->query_pen();

    m_pent = allocate(W_NO_DT);

    for (i = 0; i < W_NO_DT; i++)
    {
	if (!pointerp(m_pen))
	    m_pent[i] = pen;
	else if (i >= sizeof(m_pen))
	    m_pent[i] = pen + (i ? m_pen[0] : 0);
	else
	    m_pent[i] = pen + m_pen[i];
    }

    return ({ m_pent[0] - 2 * (du + co) / 3, m_pent[1] - du - co,
		m_pent[2] - (du + co) / 3 });
}

/*
 * Function name: set_wt
 * Description:   Set the weapon type
 */
void
set_wt(int type)
{
    int *likely;

    if (query_lock())
	return;			/* All changes has been locked out */

    if (type >= 0 && type < W_NO_T)
    {
	wep_wt = type;
	likely = W_DRAWBACKS[type];
	likely_dull = likely[0];
	likely_corr = likely[1];
	likely_break = likely[2];
    }
}

/*
 * Function name: query_wt
 * Description:   Query the weapon type
 */
int query_wt() { return wep_wt; }

/*
 * Function name: set_dt
 * Description:   Set the damage type of the weapon
 */
void
set_dt(int type)
{
    if (query_lock())
	return;			/* All changes has been locked out */

    if (F_LEGAL_DT(type))
	wep_dt = type;
}

/*
 * Function name: query_dt
 * Description:   Query damage type of weapon
 */
int query_dt() { return wep_dt; }

/*
 * Function name: set_hands
 * Description:   Set how the weapon should be wielded
 */
void
set_hands(int which)
{
    if (query_lock())
	return;			/* All changes has been locked out */

    if (F_LEGAL_HANDS(which))
	wep_hands = which;
}

/*
 * Description: The hands that we can use for this weapon
 */
int query_hands() { return wep_hands; }

/*
 * Description: This is the attack that it supports
 */
int query_attack_id() { return wielded_in_hand; }

/*
 * Description: This is the tool slot that the weapon occupies now
 */
int *
query_slots()
{
    int abit, *hids;

    for (hids = ({}), abit = 2; abit <= wielded_in_hand; abit <<= 1)
    {
        if (wielded_in_hand & abit)
        {
            hids = hids + ({ abit });
        }
    }
    return hids;
}

#if 0
int *
query_protects()
{
/* Uznalem, ze jest to uwzgledniane przez parowanie. Dodawanie tego
 * do AC danego miejsca jest bez sensu. Alvin.
 */

    switch (wielded_in_hand)
    {
	case W_LEFT:
	    return ({ A_L_ARM });
	case W_RIGHT:
	    return ({ A_R_ARM });
	case W_BOTH:
	    return ({ A_L_ARM, A_R_ARM });
	case W_FOOTR:
	    return ({ A_R_FOOT });
	case W_FOOTL:
	    return ({ A_L_FOOT });
        default:
	    return ({ });
    }
}
#endif

/*
 * Sets the object to call wield/unwield in when this occurs.
 * Those functions can return:
 *		0 - No affect the weapon can be wielded / unwielded
 *		1 - It can be wielded / unwielded but no text should be printed
 *		    (it was done in the function)
 *		-1  It can not be wielded / unwielded default failmsg will be 
 *		    written
 *             string  It can not be wielded / unwielded 'string' is the 
 *		       fail message to print
 */
void
set_wf(object obj)
{
    if (query_lock())
	return;			/* All changes has been locked out */

    wield_func = obj;
}

#if 0
/* 
 * Function name: wield
 * Description  : This function might be called when someone tries to wield
 *                this weapon. To have this function called, use the function
 *                set_wf().
 * Arguments    : object obj - the weapon someone tried to wield.
 * Returns      : int  0 - wield this weapon normally.
 *                     1 - wield the weapon, but print no messages.
 *                    -1 - do not wield the weapon, use default messages.
 *                string - do not wield the weapon, use this fail message.
 */
public mixed
wield(object obj)
{
    return 0;
}

/*
 * Function name: unwield
 * Description  : This function might be called when someone tries to unwield
 *                this weapon. To have this function called, use the function
 *                set_wf().
 * Arguments    : object obj - the weapon to stop wielding.
 * Returns      : int  0 - the weapon can be unwielded normally.
 *                     1 - unwield the weapon, but print no messages.
 *                    -1 - do not unwield the weapon, print default messages.
 *                string - do not unwield the weapon, use this fail message.
 */
public mixed
unwield(object obj)
{
    return 0;
}
#endif

/*
 * Function name: set_corroded
 * Description:   Use this to increases the corroded status on weapons.
 * Arguments:     cond - The new condition we want (can only be raised)
 * Returns:       1 if new condition accepted, 0 if no corrosion
 */
int
set_corroded(int corr)
{
    if (corr > corroded)
    {
        corroded = corr;
        if (F_WEAPON_BREAK(dull - repair_dull, corroded - repair_corr,
			likely_break))
            set_alarm(0.1, 0.0, &remove_broken(0));
        if (wielded && wielder)
            wielder->update_weapon(this_object());
        return 1;
    }
    return 0;
}

/*
 * Function name: query_corroded
 * Description:   Returns how many times this weapon has become corroded
 * Returns:	  The number of times
 */
int query_corroded()
{
    return corroded;
}

/*
 * Function name: set_likely_corr
 * Description:   Set how likely it is this weapon will corrode when in acid 
 *		  or something like that. 0 means it won|t corrode at all.
 * Arguments:	  i - how likely it will corrode, probably corrode if random(i)
 *		      [0, 20] recommended
 */
void set_likely_corr(int i) { likely_corr = i; }

/*
 * Function name: query_likely_corr
 * Description:   Query how likely the weapon will corrode, use it to test
 * 		  if weapon survived your rustmonster or acid pool :)
 */
int query_likely_corr() { return likely_corr; }

/*
 * Function name: set_dull
 * Description:   Use this to increases the dull status on weapons.
 * Arguments:     cond - The new condition we want (can only be raised)
 * Returns:       1 if new condition accepted
 */
int
set_dull(int du)
{
    if (du > dull)
    {
        dull = du;
        if (F_WEAPON_BREAK(dull - repair_dull, corroded - repair_corr,
			likely_break))
            set_alarm(0.1, 0.0, &remove_broken(0));
        if (wielded && wielder)
            wielder->update_weapon(this_object());
        return 1;
    }
    return 0;
}

/* 
 * Function name: query_dull
 * Description:   Returns how many times this weapon has become duller 
 * Returns:	  The number of times
 */
int query_dull() { return dull; }

/*
 * Function name: set_likely_dull
 * Description:   Set how likely the weapon will get dull or in case of a club
 *		  or mace, wear down in combat Mainly used from did_hit().
 * Arguments:     i - how likely [0, 30] recommended
 */
void set_likely_dull(int i) { likely_dull = i; }

/*
 * Function name: query_likely_dull
 * Description:   How likely it is this weapon will become duller when used
 * Returns:       How likely it is
 */
int query_likely_dull() { return likely_dull; }

/*
 * Function name: set_likely_break
 * Description:   Set how likely the weapon is to break if you use it.
 * Argument:	  i - How likely, [0, 20] recommended
 */
void set_likely_break(int i) { likely_break = i; }

/*
 * Function name: query_likely_break
 * Description:   How likely is it this weapon will break with use
 * Returns:	  How likely it is
 */
int query_likely_break() { return likely_break; }

/*
 * Function name: remove_broken
 * Description  : The weapon got broken so the player has to stop
 *                wielding it.
 * Arguments    : int silent - true if no message is to be genereated
 *                             about the fact that the weapon broke.
 */
varargs void
remove_broken(int silent = 0)
{
    dodaj_przym("zlamany", "zlamani");
    /* If the weapon is not wielded, we only adjust the broken information
     * by adding the adjective and the property. We do this within the
     * if-statement since we do not want to screw the message that may
     * be displayed later. When the property is added, the adjective is
     * added automatically.
     */
    if (!wielded || !wielder)
    {
	add_prop(OBJ_I_BROKEN, 1);
        return;
    }

    if (objectp(wield_func))
    {
	wield_func->unwield(this_object());
    }

    /* If the wizard so chooses, these messages may be suppressed. */
    if (!silent)
    {
	tell_object(wielder, capitalize(short(wielder, PL_MIA)) + " peka!!!\n");
	tell_room(environment(wielder), QCSHORT(this_object(), PL_MIA) +
	    " dzierz" + koncowka("ony", "ona", "one", "eni", "one") +
	    " przez " + QCIMIE(wielder, PL_DOP) + " peka!!!\n", ({wielder}));
    }

    /* Force the player to unwield the weapon and then adjust the broken
     * information by adding the property and the adjective.
     */
    wielder->unwield(this_object());
/*  remove_adj("wielded"); */
    add_prop(OBJ_I_BROKEN, 1);
    wielded = 0;
}

/*
 * Function name: set_repair_dull
 * Description:   When trying to repair the weapon, call this function. Repairs
 *                can only increase the repair factor. (This is sharpening)
 * Arguments:     rep - The new repair number
 * Returns:       1 if new repair status accepted
 */
int
set_repair_dull(int rep)
{
    if (rep > repair_dull && F_LEGAL_WEAPON_REPAIR_DULL(rep, dull))
    {
        repair_dull = rep;
        if (wielded && wielder)
            wielder->update_weapon(this_object());
        return 1;
    }
    return 0;
}

/*
 * Function name: query_repair_dull
 * Description:   How many times has this weapon been sharpened
 * Returns:	  The number of times
 */
int query_repair_dull() { return repair_dull; }

/*
 * Function name: set_repair_corr
 * Description:   When trying to repair the weapon, call this function. Repairs
 *                can only increase the repair factor. This repairs corroded.
 * Arguments:     rep - The new repair number
 * Returns:       1 if new repair status accepted
 */
int
set_repair_corr(int rep)
{
    if (rep > repair_corr && F_LEGAL_WEAPON_REPAIR_CORR(rep, corroded))
    {
        repair_corr = rep;
        if (wielded && wielder)
            wielder->update_weapon(this_object());
        return 1;
    }
    return 0;
}

/*
 * Function name: query_repair_corr
 * Description:	  How many times this weapon has been repaired from corrosion
 * Returns:	  How many times
 */
int query_repair_corr() { return repair_corr; }

/*
 * Function name: set_weapon_hits
 * Description:   By setting the hits counter you will have influence over how
 *                likely the weapon is to get in a worse condition. The hits
 *                variable keeps track of how many times this piece of weapon
 *                has hit something.
 * Argument:      new_hits - integer
 */
public void
set_weapon_hits(int new_hits) { hits = new_hits; }

/*
 * Function name: query_weapon_hits
 * Description:   hits variable keeps track of how many times this weapon has
 *		  hit something.
 * Returns:	  The number of times
 */
public int
query_weapon_hits() { return hits; }

/*
 * Function name: add_prop_obj_i_value
 * Description:   Someone is adding the value prop to this object.
 * Arguments:     val - The new value (mixed)
 * Returns:       1 if not to let the val variable through to the prop
 */
int
add_prop_obj_i_value(mixed val)
{
    if (!max_value)
    {
        max_value = -1;
        return 0;
    }

    if (intp(val) && val)
        max_value = val;

    return 1;
}

/*
 * Function name: query_value
 * Description:   Qhat's the value of this armour
 */
int
query_value()
{
    if (query_prop(OBJ_I_BROKEN))
        return 0;

    return max_value *
	F_WEAPON_VALUE_REDUCE(dull - repair_dull, corroded - repair_corr) / 100;
}

/*
 * Function name: query_wf
 * Description:   Query if/what object defines wield/unwield functions
 */
object query_wf() { return wield_func; }

/*
 * Function name: try_hit
 * Description:   Called from living when weapon used.
 * Arguments:     target - Who I intend to hit.
 * Returns:       False if weapon miss. If true it might hit.
 */
int try_hit(object target) { return 1; }

/*
 * Nazwa funkcji : did_parry
 * Opis          : Mowi broni, ze wlasnie zostala uzyta do parowania.
 */
public void
did_parry()
{
    hits++;

    if (F_WEAPON_CONDITION_DULL(hits, wep_pen, likely_dull))
    {
	hits = 0;
	set_dull(query_dull() + 1);
    }
}

/*
 * Function name: did_hit
 * Description:   Tells us that we hit something. Should produce combat
 *                messages to all relevant parties. If the weapon
 *                chooses not to handle combat messages then a default
 *                message is generated.
 * Arguments:     aid:    The attack id
 *                hdesc:  The hitlocation description.
 *                phurt:  The %hurt made on the enemy
 *                enemy:  The enemy who got hit
 *		  dt:	  The current damagetype
 *                phit:   The %success that we made with our weapon
 *		  dam:    The actual damage caused by this weapon in hit points
 *		  tohit:  How well did we hit the enemy
 *		  def_ob: Obj that defended or how we defended (if miss)
 *		  armour: Armor on the hit hitlocation
 * Returns:       True if it handled combat messages, returning a 0 will let
 *		  the normal routines take over
 */
public varargs int
did_hit(int aid, string hdesc, int phurt, object enemy, int dt,
		int phit, int dam, int tohit, mixed def_ob, object armour)
{
    hits++;

    if (F_WEAPON_CONDITION_DULL(hits, wep_pen, likely_dull))
    {
	hits = 0;
	set_dull(query_dull() + 1);
    }

    return 0;
}

/*
 * Function name: check_weapon
 * Description  : Check file for security.
 * Returns      : int 1 - always.
 */
nomask int
check_weapon()
{
    return 1;
}

/*
 * Function name: set_default_weapon
 * Description:   Configures the weapon
 * Arguments:
 * Returns:
 */
varargs void
set_default_weapon(int hit, int pen, int wt, int dt, int hands, object obj)
{
    /* Sets the weapon "to hit" value.
    */
    if (hit) set_hit(hit);
    else set_hit(5);

    /* Sets the weapon penetration value.
    */
    if (pen) set_pen(pen);
    else set_pen(10);

    /* Set the weapon type.
    */
    if (hit) set_wt(wt);
    else set_wt(W_FIRST);

    /* Set the damage type.
    */
    if (hit) set_dt(dt);
    else set_dt(W_IMPALE | W_SLASH);

    /* Set the hand(s) used to wield the weapon.
    */
    if (hands) set_hands(hands);
    else set_hands(W_NONE);
    
    /* Sets the name of the object that contains the function
       to call for extra defined wield() and unwield()
       functions.
    */
    if (obj) set_wf(obj);
}


/*
 * Function name: query_wield_desc
 * Description:   Describe this weapon as wielded by a something.
 * Argumensts:    p: Possessive description of wielder
 * Returns:       Description string.
 */
public nomask string 
query_wield_desc()
{
    string str;

    str = short(this_player(), PL_BIE);

    switch (wielded_in_hand)
    {
	case W_RIGHT:return str + " w prawej rece";
	case W_LEFT: return str + " w lewej rece";
	case W_BOTH: return "oburacz " + str;
	case W_FOOTR:return str + " w prawej stopie";
	case W_FOOTL:return str + " w lewej stopie";
    }
    return str;
}

/*
 * Nazwa funkcji : query_default_value
 * Opis          : Zwraca standardowa wartosc broni, wyliczona na podstawie
 *		   jej wartosci hit i pen. Mozna je podac opcjonalnie jako
 *		   argumenty. W przypadku wywolania funkcji bez argumentow,
 *		   zostana wykorzystane wartosci hit/pen juz zdefiniowane
 *		   danej broni.
 * Argumenty     : Opcjonalne:
 *			int hit - wartosc 'to hit' broni,
 *			int pen - wartosc 'penetration' broni.
 * Funkcja zwraca: int - srednia cene dla danego typu broni.
 */
public int
query_default_value(int hit = query_hit(), int pen = query_pen())
{
    int cena;
    
    cena = 50 + hit * pen;
    
    if (wep_wt == W_KNIFE)
        cena /= 2;
        
    return cena;
}

/*
 * Function name: update_prop_settings
 * Description:   Will uppdate weight and value of this object to be legal
 */
nomask void
update_prop_settings()
{
    if (max_value == -1)
        max_value = query_default_value();

/*
 * Ten warunek jest bez sensu. Powyzszy jest zamiast niego.
 *
    if (query_prop(OBJ_I_VALUE) < F_VALUE_WEAPON(wep_hit, wep_pen) &&
	    !query_prop(OBJ_I_IS_MAGIC_WEAPON))
        add_prop(OBJ_I_VALUE, F_VALUE_WEAPON(wep_hit, wep_pen));
*/
 
    if (F_WEIGHT_FAULT_WEAPON(query_prop(OBJ_I_WEIGHT), wep_pen, wep_wt) &&
	    !query_prop(OBJ_I_IS_MAGIC_WEAPON))
        add_prop(OBJ_I_WEIGHT, F_WEIGHT_DEFAULT_WEAPON(wep_pen, wep_wt));
}

/*
 * Function name: query_wielded
 * Description:   If this object is wielded or not
 * Returns:       The object who wields this object if this object is wielded
 */
object
query_wielded()
{
    if (wielded) return wielder;
}

/*
 * Function name: query_am
 * Description:   Called when wielding the weapon, to check for the parry.
 * Returns:       The armour modifier
 */
public nomask int *
query_am() { return ({ -3, 2, 1}); }

/*
 * Function name: query_ac
 * Description:   Called when wielding the weapon, to check for parry.
 * Returns: 	  The ac the weapon contributes with.
 */
public nomask int
query_ac()
{
    if (wielder)
    	return wielder->query_skill(SS_PARRY) * wep_hit / 500 +
		wep_hit / 2;
    else
	return wep_hit / 2;
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
    string str;

    str = ::stat_object();

    str += "Trafienie(hit): " + wep_hit + "\t\tUszkodzenia(pen): " + 
        wep_pen + "\n";

    str += "Typ broni: " + wep_wt + ", ";
    switch(wep_wt)
    {
        case W_SWORD: str += "Miecz"; break;
        case W_POLEARM: str += "Bron drzewcowa"; break;
        case W_AXE: str += "Topor"; break;
        case W_KNIFE: str += "Sztylet"; break;
        case W_CLUB: str += "Maczuga"; break;
        case W_WARHAMMER: str+= "Mlot"; break;
        case W_MISSILE: str += "Bron strzelecka"; break;
        case W_JAVELIN: str += "Bron miotana"; break;
    }
    
    str += "\nRece: " + wep_hands + "\n";
    str += "Typ zadawanych ran: " + wep_dt + "\n";

    return str;
}

/*
 * Function name: wep_condition_desc
 * Description:   Returns the description of the condition of the weapon
 */
string
wep_condition_desc()
{
    string hand, hand2;
    int mn;

    if (query_prop(OBJ_I_BROKEN))
	return (query_tylko_mn() ? "Sa" : "Jest") + " peknie" + 
	    koncowka("ty", "ta", "te", "ci", "te") + ".\n";

    mn = query_tylko_mn();

    switch (corroded - repair_corr)
    {
	case 0:
	    hand = ""; break;
	case 1:
	    hand = "Spostrzegasz na " + koncowka("nim", "niej", "nim", "nich",
	        "nich") + " slady rdzy!\n"; break;
	case 2:
	    hand = "Spostrzegasz na " + koncowka("nim", "niej", "nim", "nich",
	        "nich") + " liczne slady rdzy!\n"; break;
	case 3:
	    hand = capitalize(short(PL_MIA)) + " " + (mn ? "sa" : "jest") + 
	        " cal" + koncowka("y pokryty", "a pokryta", 
	        "e pokryte", "i pokryci", "e pokryte") + " rdza!\n"; break;
	case 4:
	case 5:
	    hand = "Wyglada jak po kapieli w kwasie!\n"; break;
 	default:
	    hand = "Jest tak skorodowan" + koncowka("y", "a", "e") +
	        ", ze moze sie rozpasc w kazdej chwili.\n"; break;
    }

    switch (dull - repair_dull)
    {
	case 0:
	    hand2 = (mn ? "sa" : "jest") + " w znakomitym stanie"; break;
	case 1:
	    hand2 = (mn ? "sa" : "jest") + " w dobrym stanie"; break;
	case 2:
	    hand2 = "liczne walki wyryly na " + koncowka("nim", "niej",
	        "nim", "nich", "nich") + " swoje pietno"; break;
	case 3:
	    hand2 = (mn ? "sa" : "jest") + " w zlym stanie"; break;
	case 4:
	case 5:
	    hand2 = (mn ? "sa" : "jest") + " w bardzo zlym stanie"; break;
	case 6:
	case 7:
	case 8:
	    hand2 = "wymaga" + (mn ? "ja" : "") + " natychmiastowej " +
	        "konswerwacji"; break;
	default:
	    hand2 = "mo" + (mn ? "ga" : "ze") + " peknac w kazdej chwili"; break;
    }

    return hand + "Wyglada na to, ze " + hand2 + ".\n";
}

/*
 * Function name: weapon_type
 * Description:   This function should return the type of the weapon in
 * 		  text.
 * Returns:	  The type
 */
string
weapon_type()
{
    if (wep_wt >= W_NO_T)
    {
	return "bron";
    }

    return W_NAMES[wep_wt];
}

/*
 * Function name: wep_usage_desc
 * Description  : This function returns the usage of this weapon. It is
 *                usually printed from the appraise function. The string
 *                includes the type of the weapon and the location where it
 *                should be wielded.
 * Returns      : string - the description.
 */
string
wep_usage_desc()
{
    string hand;
    int typ;

    switch (wep_hands)
    {
        case W_RIGHT:
            hand = "do chwytania w prawej rece"; break;
        case W_LEFT:
            hand = "do chwytania w lewej rece"; break;
        case W_BOTH:
            hand = "do chwytania oburacz"; break;
        case W_FOOTR:
            hand = "do chwytania w prawej stopie"; break;
        case W_FOOTL:
            hand = "do chwytania w lewej stopie"; break;
        case W_ANYH:
            hand = "do chwytania w dowolnej rece"; break;
        default:
            hand = "dla jakiejs dziwnej istoty"; break;
    }
    
    switch (wep_wt)
    {
         case W_SWORD:		typ = 0; break;
         case W_POLEARM:	typ = 1; break;
         case W_AXE:		typ = 0; break;
         case W_KNIFE:		typ = 0; break;
         case W_CLUB:		typ = 1; break;
         case W_WARHAMMER:	typ = 0; break;
         case W_MISSILE:	typ = 1; break;
         case W_JAVELIN:	typ = 0; break;
         default: 		typ = 1; break;
    }

    return "\nZauwazasz, iz " + (typ ? "ta" : "ten") + " " + weapon_type() +
        " jest przystosowan" + (typ ? "a" : "y") + " " + hand + ".\n";
}

/*
 * Function name: appraise_object
 * Description  : Someone tries to appraise the object. We add information
 *                about the way you should use this weapon.
 */
void
appraise_object(int num)
{
    ::appraise_object(num);

    write(wep_usage_desc());
}

/*
 * Function name: may_not_recover
 * Description  : This function will be true if the weapon may not recover.
 * Returns      : 1 - no recovery, 0 - recovery.
 */
nomask int
may_not_recover()
{
    return will_not_recover;
}

/*
 * Function name: may_recover
 * Description  : In some situations it is undesirable to have a weapon
 *                not recover. This function may then be used to force the
 *                weapon to be recoverable.
 *
 *                This function may _only_ be called when a craftsman sells
 *                a weapon he created to a player! It may expressly not be
 *                called in weapons that are to be looted from NPC's!
 */
nomask void
may_recover()
{
    will_not_recover = 0;
}

/*
 * Function name: query_wep_recover
 * Description:   Return the recover strings for changing weapon variables.
 * Returns:	  A recover string
 */
string
query_wep_recover()
{
    return ("#WEP#" + hits + "#" + dull + "#" + corroded + "#" +
	repair_dull + "#" + repair_corr + "#" + query_prop(OBJ_I_BROKEN) +
	"#");
}

/*
 * Function name: init_wep_recover
 * Description:   Initialize the weapon variables at recover.
 * Arguments:	  arg - String with variables to recover
 */
void
init_wep_recover(string arg)
{
    string foobar;
    int    broken;

    sscanf(arg, "%s#WEP#%d#%d#%d#%d#%d#%d#%s", foobar,
	hits, dull, corroded, repair_dull, repair_corr, broken, foobar);

    if (broken != 0)
    {
	add_prop(OBJ_I_BROKEN, broken);
    }
}

/*
 * Function name: query_recover
 * Description  : Called to check whether this weapon is recoverable.
 *                If you have variables you want to recover yourself,
 *                you have to redefine this function, keeping at least
 *                the filename and the weapon recovery variables, like
 *                they are queried below.
 *                If, for some reason, you do not want your weapon to
 *                recover, you should define the function and return 0.
 * Returns      : string - the default recovery string.
 */
public string
query_recover()
{
    return MASTER + ":" + query_wep_recover();
}

/*
 * Function name: init_recover
 * Description  : When the object recovers, this function is called
 *                to set the necessary variables. If you redefine the
 *                function, you must add a call to init_wep_recover
 *                with the string that you got after querying
 *                query_wep_recover.
 * Arguments    : string argument - the arguments to parse
 */
public void
init_recover(string arg)
{
    init_wep_recover(arg);
}

