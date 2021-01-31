/*
 * /std/armour.c
 *
 * Contains all routines relating to armours of any kind.
 *
 */

#pragma save_binary
#pragma strict_types

inherit "/std/object";

#include <wa_types.h>
#include <formulas.h>
#include <stdproperties.h>
#include <macros.h>
#include <cmdparse.h>
#include <composite.h>

/*
 * Prototypes.
 */
string arm_condition_desc();
void update_prop_settings();
public int *query_slots();
nomask int wear_arm(string what);
nomask int remove_arm(string what);
varargs void remove_broken(int silent = 0);
public string wear_how(int location);
varargs mixed wear_me(int cicho);
public nomask int *query_protects();
public void set_slots(int slot, ...);

/*
 * Variables. They are all static which means that they will not be saved.
 */
static int
		old,
/* to_remove */ arm_at,		/* Armour type */

		arm_slots,	/* Sloty bez side_bits */
		slot_side_bits, /* Sloty wyznaczajace strone zakladania */
		worn_on_part,	/* On which part is the armour worn */
		ac_slots,	/* Hitlokacje chronione bez side_bits */
		ac_side_bits,	/* Hitlokacje zalezne od strony zalozenia */
		ac_worn,	/* Hitlokacje chronione gdy zalozona */

/* to remove */ shield_size, 	/* Powierzchnia tarczy */
		parry_bonus,	/* Bonus do parowania tarcza */
		worn,		/* Worn or not */
/* to remove */	arm_shield,     /* Bodypart(s) protected by shield */
		max_value,	/* The value of armour when not worn down */
		hits,		/* No of hits on armour without worse cond */
		condition,	/* How correded/worn down the armour has been */
		likely_break,	/* How likely the armour is to break */
		likely_cond,	/* Likely cond of armour will worsen */
		repair;		/* How much of this has been repaired */
static object   wearer,		/* Who is holding it */
                wear_func;	/* Object that defines extra wear/remove */
static string   gFail;          /* Acc error messages when wearing */
static mapping 	ac_table;	/* Rozlozenie ac na roznych hitlokacjach */
static private int	will_not_recover;	/* True if it won't recover */


private void make_old_type_armour(int ac);


/*
 * Function name: create_armour
 * Description  : In order to create an armour, you should redefine this
 *                function. Typically you use this function to set the
 *                name and description, the armour type, the armour class
 *                etcetera.
 */
void
create_armour() 
{
}

/*
 * Function name: create_object
 * Description  : This function is called to create the armour. It sets
 *                some default (sanity) settings and then the function
 *                create_armour is called. You must redefine that function
 *                as this function is nomasked, ie you cannot redefine it.
 */
public nomask void
create_object()
{
    arm_at = -1 /*F_ARMOUR_DEFAULT_AT*/;
//    arm_mods = allocate(W_NO_DT);
    likely_break = 2;
    likely_cond = 3;
    ac_table = ([]);
    
    ustaw_nazwe( ({ "zbroja", "zbroi", "zbroi", "zbroje", "zbroja",
        "zbroi" }), ({ "zbroje", "zbroi", "zbrojom", "zbroje", 
        "zbrojami", "zbrojach" }), PL_ZENSKI );
    
    worn = 0;
    add_prop(OBJ_I_VALUE, "@@query_value");
    add_prop(OBJ_I_VOLUME, 1000);
    add_prop(OBJ_I_WEIGHT, 500);

    /* If this will be true, the weapon will not recover on reboot and is
     * lost after the reboot.
     */
    will_not_recover = (random(100) < PERCENTAGE_OF_RECOVERY_LOST);

    create_armour();

    if (old)
        make_old_type_armour(old);
    update_prop_settings();
}

/*
 * Function name: reset_armour
 * Description  : At a regular interval this function will be called to
 *                reset the armour. If you define this function, you must
 *                also call enable_reset from your create_armour. If you
 *                fail to do this, the function reset_armour will _not_
 *                be called.
 */
void
reset_armour()
{
}

/*
 * Function name: reset_object
 * Description  : At a regular interval this function will be called. All
 *                it does is call reset_armour. For further details see
 *                that function. As this function is declared nomask, you
 *                may not redefine it.
 */
nomask void
reset_object()
{
    reset_armour();
}

/*
 * Function name: init
 * Description  : This function adds the commands 'wear' and 'remove' to
 *                any living that 'comes close' to the armour, that is
 *                if this armour enters the inventory or environment of
 *                the player. If you redefine this function to add your
 *                own commands to the player you MUST also make a call
 *                to ::init();
 */
void
init()
{
    ::init();

    add_action(wear_arm, "zaloz");
    add_action(remove_arm, "zdejmij");
}

/*
 * Function name: short
 * Description  : If the armour is broken, we add the adjective broken to
 *                the short description. There is a little caveat it the
 *                short description has not been explicitly set. In that
 *                case, the adjective broken may appear twice.
 * Arguments    : object for_obj - the object that wants to know.
 * Returns      : string - the short description.
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
	return oblicz_przym("zniszczony", "zniszczeni", przyp, 
	    this_object()->query_rodzaj(), 0) + " " + 
	    ::short(for_obj, przyp);
    }

    return ::short(for_obj, przyp);
}

/*
 * Function name: plural_short
 * Description  : If the armour is broken, we add the adjective broken
 *                to the plural short description.
 * Arguments    : object for_obj - the object that wants to know.
 * Returns      : string - the plural short description.
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
	return oblicz_przym("zniszczony", "zniszczeni", przyp, 
	    this_object()->query_rodzaj(), 1) + " " + str;

    }

    return str;
}

/*
 * Function names: long
 * Description   : The long description. We add the condition information
 *                 to it. This function only returns the string, it does
 *                 not print it to the looker.
 * Arguments     : string str - long of pseudo item or of the very object ?
 *		   object for_obj - the object that wants to know.
 * Returns       : string - the long description.
 */
public varargs string
long(string str, object for_obj)
{
    return ::long(str, for_obj) + (str ? "" : arm_condition_desc());
}

/*
 * Function name: wear_arm
 * Description  : This function is called when the player wants to wear
 *                something. It is run only once even if the player has
 *                more armours with him because it would be rather wasteful
 *                to check it multiple times.
 * Arguments    : string what - the command line argument.
 * Returns      : int 1/0 - this function will only return 0 if it fails
 *                          to find the objects to wear. If the player just
 *                          fails to wear them, it will return 1.
 */
nomask int
wear_arm(string what)
{
    object *a;

    /* This is an exception from the rule that we should do 'notify_fail'
     * and return 0, all the time. If we do then all armours will run
     * this code, which is somewhat wastefull.
     */
    gFail = "";
    a = CMDPARSE_ONE_ITEM(what, "wear_one_thing", "wear_access");
    if (!sizeof(a))
    {
	if (!strlen(gFail))
	{
	    notify_fail(capitalize(query_verb()) + " co?\n");
	    return 0;
	}
	else
	{
	    write(gFail);
	}
    }
    
    gFail = 0;
    return 1;
}

/*
 * Function name: wear_access
 * Description  : This function is a filter to see whether the objects the
 *                player tries to wear are really armours and whether the
 *                player also carries them. You have no business redefining
 *                this function and therefore it is declared nomask.
 * Arguments    : object what - the object to check.
 * Returns      : int 1/0 - true of the object is a true armour that the
 *                          player carries, else 0.
 */
public nomask int
wear_access(object what)
{
    if (function_exists("create_object", what) != "/std/armour")
	return 0;
    if (environment(what) == this_player())
	return 1;
    else
	return 0;
}

/*
 * Function name: wear_one_thing
 * Description  : This is a filter function that is used by wear_arm to
 *                try to actually wear this armour. If the player fails
 *                to wear the armour, a fail message will be added to the
 *                global fail message. You should not redefine this function
 *                and therefore it is declared nomask.
 * Arguments    : object what - the object to wear.
 * Returns      : int 1/0 - true if the armour is worn.
 */
nomask int
wear_one_thing(object what)
{
    string fail;

    if (this_object() != what)
	fail = what->wear_me();
    else
	fail = wear_me();

    if (stringp(fail))
    {
	gFail += fail;
	return 0;
    }
    return 1;
}

/*
 * Wear this armour
 * Return error message, write success message. Return 1 if success.
 */
public varargs mixed
wear_me(int cicho)
{
    int wret, il, ix;
    string wfail, what, how;
    object ob;

    what = short(this_player(), PL_BIE);
    
    if (worn)
	return "Juz masz na sobie " + what + ".\n";
    else if (this_player() != environment(this_object()))
	return "Musisz wpierw wziac " + what + " zeby moc " +
	    (query_rodzaj() == PL_ZENSKI ? "ja" : 
	    (query_rodzaj() >= PL_NIJAKI_OS ? "je" : "go")) + 
	    " zalozyc.\n";
    else if (query_prop(OBJ_I_BROKEN))
	return "Ale " + short(PL_MIA) + " " + (query_tylko_mn() ? "sa" :
	    "jest") + " kompletnie zniszcz" + koncowka("ony", "ona", "one",
	    "eni", "one") + "!\n";

    wearer = this_player();
    
    ix = ((slot_side_bits || ac_side_bits) ? 2 : 1);
    
    while (--ix >= 0)
    {
        worn_on_part = arm_slots;
        worn_on_part |= (slot_side_bits << !ix);

	for (il = TS_TORSO; il <= worn_on_part; il <<= 1)
	    if ((worn_on_part & il) && ((ob = wearer->query_armour(il)) ||
	    				(ob = wearer->query_weapon(il))))
	    {
		break;
	    }
	    else
	    	ob = 0;
	
	if (!ob)
	{
	    ac_worn = (ac_slots | (ac_side_bits << (!ix)));
	    break;
	}
    }

    if (ob)
	return "Zawadza ci " + ob->short(this_player(), PL_MIA) + ".\n";

/*
    if (worn_on_part == A_SHIELD)
    {
	if (wearer->query_armour(W_LEFT) == 0)
	    worn_on_part = W_LEFT;
	else if (wearer->query_armour(W_RIGHT) == 0)
	    worn_on_part = W_RIGHT;
	else
	    return "Twoje rece sa juz zajete.\n";
    }
    else if (worn_on_part == A_ANY_FINGER)
    {
	if (wearer->query_armour(A_L_FINGER) == 0)
	    worn_on_part = A_L_FINGER;
	else if (wearer->query_armour(A_R_FINGER) == 0)
	    worn_on_part = A_R_FINGER;
	else
	    return "Nie ma juz wolnego miejsca na twoich palcach.\n";
    } else
	for (il = TS_TORSO; il <= worn_on_part; il *= 2)
	    if ((worn_on_part & il) && wearer->query_armour(il))
	    	return "Zawadza ci " + 
	    	    wearer->query_armour(il)->short(PL_MIA) + ".\n";
*/

    if (stringp(wfail = wearer->wear_arm(this_object())))
	return wfail;

    wret = 0;

    /*
     * A wear function in another object.
     */
    if ((!cicho) &&
        ((!wear_func) || (!(wret = wear_func->wear(this_object())))))
    {
        if (worn_on_part == A_ROBE)
        {
            write("Otulasz sie " + short(this_player(), PL_NAR) + ".\n");
            saybb(QCIMIE(this_player(), PL_MIA) + " otula sie " + 
               QSHORT(this_object(), PL_NAR) + ".\n");
        }
        else
        {
	    how = wear_how(worn_on_part);
	    write("Zakladasz " + what + how + ".\n");
	    saybb(QCIMIE(this_player(), PL_MIA) + " zaklada " + 
	        QSHORT(this_object(), PL_BIE) + ".\n");
	}
    }

    /*
     * If the wearfunc returned a value < 0 then we can not wear the armour
     */
    if (intp(wret) && wret >= 0)
    {
//	set_adj("worn");
	worn = 1;
	return 1;
    } 
    else
    {
	wearer->remove_arm(this_object());
//	remove_adj("worn");
	worn = 0;
	if (stringp(wret))
	    return wret;
	else
	    return "Nie mozesz zalozyc " + short(this_player(), PL_DOP) + ".\n";
    }
}

/*
 * Function name: wear_how
 * Description:   Generate the string that is shown when the armour is worn
 * Arguments:	  location: the location(s) covered by the armour
 */
public string
wear_how(int location)
{
    string *how = ({ });

/* Pozbycie sie nielegalnych bitow.
 */
    location &= ((MAX_TS << 1) - 1);

    if (location & A_TORSO)
    {
        how += ({ "tulow" }); location -= A_TORSO;
    }

    if (location & A_HEAD)
    {
        how += ({ "glowe" }); location -= A_HEAD;
    }

    if (location & A_LEGS)
    {
        how += ({ "nogi" }); location -= A_LEGS;
    }

    if ((location & W_RIGHT) && (location & A_R_FOREARM))
    {
        how += ({ "prawa reke" }); location &= ~(W_RIGHT | A_R_FOREARM);
    }
    
    if ((location & W_LEFT) && (location & A_L_FOREARM))
    {
        how += ({ "lewa reke" }); location &= ~(W_LEFT | A_L_FOREARM);
    }
    
    if ((location & A_ARMS) == A_ARMS)
    {
        how += ({ "ramiona" }); location -= A_ARMS;
    }

    if (location & A_R_ARM)
    {
        how += ({ "prawe ramie" }); location -= A_R_ARM;
    }

    if (location & A_L_ARM)
    {
        how += ({ "lewe ramie" }); location -= A_L_ARM;
    }

    if (location & A_R_FOREARM)
    {
        how += ({ "prawe przedramie" }); location -= A_R_FOREARM;
    }

    if (location & A_L_FOREARM)
    {
        how += ({ "lewe przedramie" }); location -= A_L_FOREARM;
    }

    if (location & A_NECK)
    {
        how += ({ "szyje" }); location -= A_NECK;
    }

    if (location & A_WAIST)
    {
        how += ({ "biodra" }); location -= A_WAIST;
    }

    if (location & A_R_FINGER)
    {
        how += ({ "serdeczny palec prawej reki" }); location -= A_R_FINGER;
    }

    if (location & A_L_FINGER)
    {
        how += ({ "serdeczny palec lewej reki" }); location -= A_L_FINGER;
    }

    if ((location & A_HANDS) == A_HANDS)
    {
        how += ({ "dlonie" }); location -= A_HANDS;
    }

    if (location & (A_R_HAND | W_RIGHT))
    {
        how += ({ "prawa dlon" }); location &= ~(A_R_HAND | W_RIGHT);
    }

    if (location & (A_L_HAND || W_LEFT))
    {
        how += ({ "lewa dlon" }); location &= ~(A_L_HAND | W_LEFT);
    }

    if ((location & A_FEET) == A_FEET)
    {
        how += ({ "stopy" }); location -= A_FEET;
    }

    if (location & A_R_FOOT)
    {
        how += ({ "prawa stope" }); location -= A_R_FOOT;
    }

    if (location & A_L_FOOT)
    {
        how += ({ "lewa stope" }); location -= A_L_FOOT;
    }
	    
    return " na " + COMPOSITE_WORDS(how);
}

/*
 * Function name: remove_arm
 * Description:   Unwear the armour.
 * Arguments:	  what: What to remove.
 */
int
remove_arm(string what)
{
    mixed 	*a;
    string	prep, loc;
    object arm;
    int il;
    
    notify_fail(capitalize(query_verb()) + " co?\n");  /* access failure */
    if (!what)
	return 0;

    /*
     * This is an exception from the rule that we should do 'notify_fail'
     * and return 0, all the time. If we do then all armours will run
     * this code, which is somewhat wastefull.
     */
    if (!parse_command(what, all_inventory(this_player()), "%i:3", a))
	    return 0;
	    
    arm = a[0];
    a = (a & this_player()->query_armour(-1));
    a = CMDPARSE_STD->normal_access( ({ arm }) + a, 0, 0, 1);

    if (!sizeof(a))
    {
	notify_fail("Nie masz nic takiego na sobie.\n");
	return 0;
    }

    for (il = 0; il < sizeof(a); il++)
	a[il]->remove_me();
	
    this_player()->set_obiekty_zaimkow(a);

    return 1;
}

/*
 * Remove the armour, return 0 if failure
 */
nomask int
remove_me()
{
    int wret;

    if (!worn || !wearer)
	return 0;
	
    /*
     * A remove function in another object.
     */
    if ((!wear_func) || (!(wret = wear_func->remove(this_object()))))
    {
	if (check_seen(this_player()))
	    write("Zdejmujesz z siebie " + short(PL_BIE) + ".\n");
	else
	    write("Zdejmujesz cos.\n");
	saybb(QCIMIE(this_player(), PL_MIA) + " zdejmuje z siebie " + 
	    QSHORT(this_object(), PL_BIE) + ".\n");
    }
    
    if (wret >= 0)
    {
	wearer->remove_arm(this_object());
//	remove_adj("worn");
	worn = 0;
	worn_on_part = 0;
	ac_worn = 0;
	return 1;
    }
    else
	return 0;
}

/*
 * Function name: leave_env
 * Description:   The armour is moved from the inventory
 * Arguments:	  from - Where from
 */
void
leave_env(object from, object to)
{
    ::leave_env(from, to);

    if (!worn)
	return;
#if 0
    remove_me();
#endif
    if ((!wear_func || !wear_func->remove(this_object())) && wearer)
	tell_object(wearer, "Zdejmujesz " + short(wearer, PL_BIE) + ".\n");

    wearer->remove_arm(this_object());
//    remove_adj("worn");
    worn = 0;
}

private void
make_old_type_armour(int ac)
{
    int at, il;
    
    at = arm_at;
    
    if (at < 0)
    {
        at *= -1;
        ac_side_bits = slot_side_bits = at;
        at = (at | (at << 1));
    }
    else
        ac_slots = arm_slots = at;
    
    for (il = TS_TORSO; il <= at; il <<= 1)
    {
        if (il & at)
            ac_table[il] = ({ ac, ac, ac });
    }
    
    log_file("OLD_ARMOR", file_name(this_object()) + "\n");
    
    add_prop(OBJ_I_VALUE, "@@query_value");
    max_value = -1;
}

/*
 * Nazwa funkcji : set_ac
 * Opis          : Ustawia klase pancerza zbroji. Przy jej podawaniu
 *		   precyzuje sie klase pancerza osobno dla kazdej
 *		   hitlokacji, ktora zbroja chroni. Klase pancerza
 *		   charakteryzuja trzy liczby - klasy pancerza na
 *		   trzy rodzaje obrazen.
 * Argumenty     : set_ac() przyjmuje dowolna ilosc argumentow, ktora
 *		   jest krotnoscia 4. Kazda czworka opisuje klase
 *		   pancerza na jednej hitlokacji (zdefiniowane w 
 *		   '/sys/wa_types.h'.
 *			int hid - identyfikator hitlokacji (A_HEAD, itp),
 *			int ac_klute - wyparowania na rany klute,
 *			int ac_ciete - wyparowania na rany ciete,
 *			int ac_obuch - wyparowania na rany obuchowe.
 */
void
set_ac(int hid, int ac_klute = -1, int ac_ciete = 0, int ac_obuch = 0, ...)
{
    int size, ix, il, ok;
    
    /* All changes may have been locked out. */
    if (query_lock())
    {
	return;
    }
    
if (ac_klute == -1)
{
    old = hid;
    return;
}

    size = sizeof(argv);
    if ((size % 4) != 0)
    {
         throw("/std/armour.c - zla liczba argumentow do set_ac.\n");
         return;
    }
    
    argv = ({ hid, ac_klute, ac_ciete, ac_obuch }) + argv;
    ok = (!arm_slots && !slot_side_bits);
    
    ix = -4;
    while ((ix += 4) <= size)
    {
        if (argv[ix] < 0)
        {
            argv[ix] *= -1;
	    ac_side_bits |= argv[ix];
	    if (ok) slot_side_bits |= argv[ix];
            argv[ix] = (argv[ix] | (argv[ix] << 1));
        }
        else
        {
            ac_slots |= argv[ix];
            if (ok) arm_slots |= argv[ix];
        }
        
        for (il = TS_TORSO; il <= argv[ix]; il <<= 1)
        {
            if (il & argv[ix])
                ac_table[il] = ({ argv[ix + 1], argv[ix + 2], argv[ix + 3] });
        }
    }
}

/*
 * Nazwa funkcji : query_ac
 * Opis          : Zwraca wartosc klasy pancerza chroniacego dana
 *		   hitlokacje. W zaleznosci od obecnosci drugiego
 *		   argumentu zwraca tablice z wartosciami pancerza
 *		   na rozne typy obrazen lub klase pancerza na podany
 *		   typ obrazen.
 * Argumenty     : int hid   - hitlokacja, o ktorej pancerz pytamy,
 *		   int dtype - jesli podany, typ obrazen. (W_IMPALE, W_SLASH,
 *			       W_BLUDGEON - zdefiniowane w /sys/wa_types.h).
 * Funkcja zwraca: Jesli podano drugi argument - int,
 *		   jesli nie - tablice w postaci ({ ac_na_klute,
 *			       ac_na_ciete, ac_na_obuchowe }).
 */
public mixed
query_ac(int hid = 0, int dtype = -1)
{
    int prot;
    
    prot = (ac_worn ?: (ac_slots | ac_side_bits | (ac_side_bits << 1)));
    
    if (dtype == -1)
    {
        return ((hid & prot)
	    ? ({ ac_table[hid][0] - condition + repair,
		 ac_table[hid][1] - condition + repair,
		 ac_table[hid][2] - condition + repair })
	    : ({ 0, 0, 0 }));
    }
    
    switch(dtype)
    {
        case W_IMPALE: dtype = 0; break;
        case W_SLASH: dtype = 1; break;
        case W_BLUDGEON: dtype = 2; break;
        default: dtype = 0;
    }

    return ((hid & prot)
	    ? ac_table[hid][dtype] - condition + repair
	    : 0);
}

/*
 * Function name: set_at
 * Description:   Set armour type

 *	  *UWAGA* Funkcja ulega likwidacji - nie uzywac!

 */
void
set_at(int type)
{
    /* All changes may have been locked out. */
    if (query_lock())
    {
	return;
    }

    arm_at = type;

/*    
    if (arm_at == A_SHIELD && !shield_size)
        shield_size = F_ARMOUR_DEFAULT_SHIELD_SIZE; */
}

/*
 * Description: Give the armour type for this armour.
 *		Armour type is a combination of the tool slots that this
 *		armour takes up and possibly the magic flag.
 *		(see /sys/wa_types.h)
 * Arguments:   hid: Hitlocation id

 *	  *UWAGA* Funkcja ulega likwidacji - nie uzywac!

 */
int
query_at()
{
    return arm_at;
}

public void
setup_shield(int typ, int ac)
{
    int cena;
    
    if (typ == SH_PUKLERZ)
    {
        ustaw_nazwe( ({ "puklerz", "puklerza", "puklerzowi", "puklerz",
            "puklerzem", "puklerzu" }), ({ "puklerze", "puklerzy",
            "puklerzom", "puklerze", "puklerzami", "puklerzach" }),
            PL_MESKI_NOS_NZYW);
            
        dodaj_nazwy( ({ "tarcza", "tarczy", "tarczy", "tarcze", "tarcza",
            "tarczy" }), ({ "tarcze", "tarcz", "tarczom", "tarcze",
            "tarczami", "tarczach" }), PL_ZENSKI);
    }
    else
    {
        ustaw_nazwe( ({ "tarcza", "tarczy", "tarczy", "tarcze", "tarcza",
            "tarczy" }), ({ "tarcze", "tarcz", "tarczom", "tarcze",
            "tarczami", "tarczach" }), PL_ZENSKI);
        likely_break = 25;
        likely_cond = 20;
    }

    switch(typ)
    {
	case SH_PUKLERZ:
	    set_slots(A_BUCKLER);
	    parry_bonus = 90;
	    cena = 100 + random(50);
	    add_prop(OBJ_I_WEIGHT, 1000 + random(300));
	    add_prop(OBJ_I_VOLUME, 400 + random(100));
	    likely_break = 30;
	    likely_cond = 35;
	    break;
	case SH_TARCZA:
	    set_slots(A_SHIELD);
	    parry_bonus = 180;
	    cena = 190 + random(50) + 100 * (ac - 10) / 10;
	    set_ac( -A_R_ARM, ac, ac, ac);
	    add_prop(OBJ_I_WEIGHT, 4000 + 2000 * (ac - 10) / 10);
	    add_prop(OBJ_I_VOLUME, 2000 + random(500));
	    break;
	case SH_RYCERSKA:
	    set_slots(A_SHIELD);
	    parry_bonus = 70;
	    cena = 300 + random(50) + 100 * (ac - 20) / 10;
	    set_ac( -A_R_ARM, ac, ac, ac,
		     A_BODY, ac/3, ac/3, ac/3);
	    add_prop(OBJ_I_WEIGHT, 6000 + 1500 * (ac - 20) / 10);
	    add_prop(OBJ_I_VOLUME, 2800 + random(500));
	    break;
	case SH_DLUGA_RYCERSKA:
	    set_slots(A_SHIELD);
	    parry_bonus = 0;
	    cena = 380 + random(70) + 100 * (ac - 25) / 10;
	    set_ac( -A_R_ARM, ac, ac, ac,
	    	     A_BODY, ac/2, ac/2, ac/2,
	    	     A_LEGS, ac/4, ac/4, ac/4);
	    add_prop(OBJ_I_WEIGHT, 7500 + 1500 * (ac - 25) / 10);
	    add_prop(OBJ_I_VOLUME, 3300 + random(500));
	    break;
	case SH_PELNA_PIECHOTY:
	    set_slots(A_SHIELD);
	    parry_bonus = 0;
	    cena = 450 + random(80) + 150 * (ac - 28) / 10;
	    set_ac( -A_R_ARM, ac, ac, ac,
		     A_BODY, ac/2, ac/2, ac/2,
		     A_LEGS, ac/2, ac/2, ac/2);
		     
	    add_prop(OBJ_I_WEIGHT, 9000 + 2500 * (ac - 28) / 10);
	    add_prop(OBJ_I_VOLUME, 4000 + random(500));
	    break;
	case SH_LEKKA:
	default:
	    set_slots(A_SHIELD);
	    parry_bonus = 130;
	    cena = 130 + random(50);
	    add_prop(OBJ_I_VOLUME, 800 + random(200));
	    add_prop(OBJ_I_WEIGHT, 2000 + random(1000));
	    break;
    }
    
    if (max_value == -1)
        max_value = cena;
}

public void
set_parry_bonus(int bonus)
{
    parry_bonus = bonus;
}

public int
query_parry_bonus()
{
    return parry_bonus;
}

void
set_shield_size(int size)
{
    if (query_lock())
        return ;
        
    shield_size = size;
}

int
query_shield_size()
{
    return shield_size;
}

/*
 * Function name: set_am
 * Description:   Set the modifyers for different attacks
 *		    that is: ({ impale, slash, bludgeon })

 *	  *UWAGA* Funkcja ulega likwidacji - nie uzywac!

 */
void
set_am(int *list)
{
#if 0
    if (query_lock())
	return;			/* All changes have been locked out */

    if (F_LEGAL_AM(list))
	arm_mods = list;
#endif
}

/*
 * Description: Give the ac modifier for a specific hitlocation
 * Arguments:   hid: Hitlocation id

 *	  *UWAGA* Funkcja ulega likwidacji - nie uzywac!

 */
int *
query_am(int hid)
{
   return allocate(W_NO_DT);
//    return arm_mods + ({});
}

/*
 * Function name: set_condition
 * Description:   Use this to increases the corroded status on armour. If the
 *		  armour gets dented or anything that turns it into worse
 *		  condition, call this function.
 * Arguments:     cond - The new condition we want (can only be raised)
 * Returns:       1 if new condition accepted
 */
int
set_condition(int cond)
{
    if (cond > condition)
    {
	condition = cond;
	if (F_ARMOUR_BREAK(condition - repair, likely_break))
	    set_alarm(0.1, 0.0, remove_broken);
	if (worn && wearer)
	    wearer->update_armour(this_object());
	return 1;
    }
    return 0;
}

/*
 * Function name: query_condition
 * Description:   Return the general condition modifier. It indicates how
 *		  many times the condition of this armour has been worsened.
 *		  The tru condition of the armour is:
 *		   	base_ac - condition + repairs
 * Returns:	  The general condition modifier
 */
int
query_condition()
{
    return condition;
}

/*
 * Function name: set_likely_cond
 * Description:   Set how likely the armour will get worn down if hit
 * Arguments:     i - how likely [0, 30] recommended
 */
void
set_likely_cond(int i)
{
    likely_cond = i;
}

/*
 * Function name: query_likely_cond
 * Description:   How likely is this armour to wear down with use (a relative
 *		  number)
 * Returns:	  The likliness
 */
int
query_likely_cond()
{
    return likely_cond;
}

/*
 * Function name: set_likely_break
 * Description:   Set how likely the armour is to break if you use it.
 * Argument:      i - How likely, [0, 20] recommended
 */
void
set_likely_break(int i)
{
    likely_break = i;
}

/*
 * Function name: query_likely_break
 * Description:   How likely this piece of armour is to break (a relative number)
 * Returns:       How liely it is
 */
int
query_likely_break()
{
    return likely_break;
}

/*
 * Function name: remove_broken
 * Description  : The armour got broken so we remove it from the
 *                player.
 * Arguments    : int silent - true if no message should be generated
 *                             about the fact that the armour breaks.
 */
varargs void
remove_broken(int silent = 0)
{
    /* If the armour is not worn, we only adjust the broken-information
     * by adding the adjective and the property. We do this inside the
     * if-statement since we do not want to screw the message that may
     * be displayed later. Note that the property automatically adds the
     * adjective broken.
     */
    if (!worn || !wearer)
    {
	add_prop(OBJ_I_BROKEN, 1);
	return;
    }

    /* A broken armour will always be removed, so we do not have to
     * dereference the result.
     */
    if (objectp(wear_func))
    {
	wear_func->remove(this_object());
    }

    /* If the wizard so chooses, this message may be omitted. */
    if (!silent)
    {
	tell_object(wearer, capitalize(short(wearer, PL_MIA)) + 
	    " rozpada" + (query_tylko_mn() ? "ja" : "") + " sie!\n");
	tell_room(environment(wearer), QCSHORT(this_object(), PL_MIA) + " " +
	    QIMIE(wearer, PL_DOP) + " rozpada" + 
	    (query_tylko_mn() ? "ja" : "") + " sie!\n", ({ wearer }));
    }

    /* Force the player to remove the armour and adjust the broken
     * information by adding the property and the adjective.
     */
    wearer->remove_arm(this_object());
    add_prop(OBJ_I_BROKEN, 1);
//    remove_adj("worn");
    worn = 0;
}

/*
 * Function name: set_repair
 * Description:   When trying to repair the armour, call this function. Repairs
 *		  can only increase the repair factor.
 * Arguments:	  rep - The new repair number
 * Returns:	  1 if new repair status accepted
 */
int
set_repair(int rep)
{
    if (rep > repair && F_LEGAL_ARMOUR_REPAIR(rep, condition))
    {
	repair = rep;
	if (worn && wearer)
	    wearer->update_armour(this_object());
	return 1;
    }
    return 0;
}

/*
 * Function name: query_repair
 * Description:   How many times this weapon has been repaired. Actual ac is:
 *			base_ac - condition + repairs
 * Returns:	  How many times...
 */
int
query_repair()
{
    return repair;
}

/*
 * Function name: set_armour_hits
 * Description:   By setting the hits counter you will have influence over how
 *		  likely the armour is to get in a worse condition. The hits 
 *		  variable keeps track of how many times this piece of armour
 *		  has been hit.
 * Argument:	  new_hits - integer
 */
public void
set_armour_hits(int new_hits)
{
    hits = new_hits;
}

/*
 * Function name: query_armour_hits
 * Description:   This function returns how many times this armour has been hit
 * 		  since last time it got degenerated. The lower this number is
 *		  the less likely the armour will degenerate (strange isn't it?)
 * Returns:	  The hits variable
 */
public int
query_armour_hits()
{
    return hits;
}

/*
 * Function name: add_prop_obj_i_value
 * Description:   Someone is adding the value prop to this object.
 * Arguments:	  val - The new value (mixed)
 * Returns:	  1 if not to let the val variable through to the prop
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

    return (max_value * F_ARMOUR_VALUE_REDUCE(condition - repair) / 100);
}

/*
 * Sets the object to call wear/remove in when this occurs.
 * Those functions can return:
 *		0 - No affect the armour can be worn / removed
 *		1 - It can be worn / removed but no text should be printed
 *		    (it was done in the function)
 *		-1  It can not be worn / removed default failmsg will be 
 *		    written
 *             string  It can not be worn / removed 'string' is the 
 *		       fail message to print
 */
void
set_af(object obj)
{
    if (query_lock())
	return;			/* All changes have been locked out */

    wear_func = obj;
}

object
query_af()
{
    return wear_func;
}

#if 0
/*
 * Function name: wear
 * Description  : This function might be called when someone tries to wear
 *                this armour. To have it called, use set_af().
 * Arguments    : object obj - The armour we want to wear.
 * Returns      : int  0 - The armour can be worn normally.
 *                     1 - The armour can be worn, but print no messages.
 *                    -1 - The armour can't be worn, use default messages.
 *                string - The armour can't be worn, use this message.
 */
public mixed
wear(object obj)
{
    return 0;
}

/*
 * Function name: remove
 * Description  : This function might be called when someone tries to remove
 *                 this armour. To have it called, use set_af().
 * Arguments    : object obj - The armour to remove.
 * Returns      : int  0 - Remove the armour normally.
 *                     1 - Remove the armour, but print no messages.
 *                    -1 - Do not remove the armour, print default message.
 *                string - Do not remove the armour, use this message.
 */
mixed
remove(object obj)
{
    return 0;
}
#endif

/*
 * Nazwa funkcji : did_parry
 * Opis          : Mowi zbroji, ze wlasnie zostala uzyta do parowania.
 *		   (powinno byc uzywane tylko przez tarcze)
 */
public void
did_parry()
{
    hits++;
    if (F_ARMOUR_CONDITION_WORSE(hits, arm_ac, likely_cond))
    {
	hits = 0;
	set_condition(query_condition() + 1);
    }
}

/*
 * Function name: got_hit
 * Description:   Notes that the defender has been hit. It can be used
 *		  to reduce the ac for this hitlocation for each hit.
 * Arguments:     hid:   The hitloc id, ie the bodypart hit.
 *                ph:    The %hurt
 *                att:   Attacker
 *		  aid:   The attack id
 *                dt:    The damagetype
 *		  dam:   The damage done to us in hit points
 */
varargs int
got_hit(int hid, int ph, object att, int dt, int dam)
{
    if (dam <= 0)
	return 0;

    hits++;
    if (F_ARMOUR_CONDITION_WORSE(hits, arm_ac, likely_cond))
    {
	hits = 0;
	set_condition(query_condition() + 1);
    }

    return 0;
}

/*
 * Function name: check_armour
 * Description:   Check file for security.
 */
nomask int
check_armour()
{
    return 1;
}

/*
 * Function name: set_default_armour
 * Description: *UWAGA* FUNKCJA ULEGA LIKWIDACJI - NIE UZYWAC!
 * Arguments:
 * Returns:
 */
public varargs void
set_default_armour(int ac, int at, int *am, object af)
{
    /* Sets the armour class.
    */
    if (ac) set_ac(ac); 
    else set_ac(1);

    /* Set the armour type.
    */
    if (at) set_at(at);
    else set_at(A_BODY);
    
    /* Set armour modifier vs weapon damage type.
    */
    if (am) set_am(am);
    else set_am(A_NAKED_MOD);

    
    /* Sets the name of the object that contains the function
       to call for extra defined wear_arm() and remove_arm()
       functions.
    */
    if (af) set_af(af);
}
    

/*
 * Function name: set_shield_slot
 * Description:   Set the hitlocation(s) protected by the shield or magic
 *		  armour
 * Arguments:     hids: Hitlocation(s)
 *			UWAGA - FUNKCJA ULEGA LIKWIDACJI - 
 *				 NIE UZYWAC!
 */
public void
set_shield_slot(mixed hids)
{
    arm_shield = hids;
}


/*
 * Function name: query_shield_slots
 * Description:   Give a bodypart protected by a shield or magic armour
 *			UWAGA - FUNKCJA ULEGA LIKWIDACJI!!
 *			  NIE UZYWAC!!
 */
public int *
query_shield_slots()
{
    return 0;
}

/*
 * Nazwa funkcji : set_slots
 * Opis          : Ustawia, ktore sloty zajmuje zalozona zbroja. Na jednym
 *		   slocie moze byc tylko jedna zbroja. Ustawienie ma sens
 *		   tylko w przypadku tych nielicznych zbroi, ktore zajmuja
 *		   inne miejsca, niz chronia (np. plaszcze i tarcze).
 *		   W przeciwnym wypadku set_ac() ustawia za nas sloty.
 *		   Mozna podac, ze zbroja zajmuje slot dowolnej wolnej
 *		   np. reki, jako wartosc slota podajac ujemny identyfikator
 *		   prawej strony tej czesci ciala - np. (-A_L_ARM).
 *		   Przyklady: /doc/examples/zbroje/.
 * Argumenty     : Funkcja przyjmuje dowolna ilosc argumentow typu int.
 *		   Kazdy z nich reprezentuje jeden, lub sume binarna kilku
 *		   slotow, ktore zajmuje zalozona zbroja (sloty zdefiniowane
 *		   w /sys/wa_types.h). 
 *		   UWAGA - sloty ujemne nalezy podawac osobno.
 */
public void
set_slots(int slot, ...)
{
    int *slots, ix;
    
    arm_slots = 0;
    slot_side_bits = 0;

    if (slot)
	slots = ({ slot });

    if (sizeof(argv))
	slots += argv;

    ix = sizeof(slots);
    while (--ix >= 0)
    {
	if (slots[ix] < 0)
	{
	    slots[ix] = -slots[ix];
	    slot_side_bits |= slots[ix];
	    slots[ix] = (slots[ix] | (slots[ix] << 1));
	}
	else
	    arm_slots |= slots[ix];
    }
}

/*
 * Nazwa funkcji : query_slots
 * Opis          : Zwraca tablice ze slotami, jakie zajmuje zalozona
 *		   zbroja. Funkcja dziala tylko, gdy zbroja jest
 *		   zalozona.
 * Funkcja zwraca: int *  sloty zajmowane przez zalozona zbroje.
 */
public int *
query_slots() 
{
    int abit, *hids;

    for (hids = ({}), abit = 2; abit <= worn_on_part; abit <<= 1)
    {
	if (worn_on_part & abit)
	{
	    hids = hids + ({ abit });
	}
    }
    return hids;
}

/*
 * Function name: query_protects
 * Description:   Give an array of the bodyparts protected by this armour.
 */
public nomask int *
query_protects()
{
    int prot, abit, *hids;
    
    prot = (ac_worn ?: (ac_slots | ac_side_bits | (ac_side_bits << 1) ));
    
    for (hids = ({}), abit = 2; abit <= prot; abit <<= 1)
    {
	if (prot & abit)
	{
	    hids = hids + ({ abit });
	}
    }
    return hids;

#if 0
    int abit, *hids, *chid;
    /*
       Table of armour slots <-> Hitlocations

             TORSO HEAD LEGS R_ARM L_ARM ROBE SHIELD

    TORSO      X      
    HEAD	    X
    LEGS	         X
    R_ARM	               X
    L_ARM	                     X
    ROBE       X         X                
    SHIELD                     X     X                          
    MAGIC      X    X    X     X     X   	

	Max ac is 100 for all hitlocations. If the sum of the ac for a
	given hitlocation is > 100 then it is set to 100.
    */
    for (hids = ({}), abit = 1; abit <= worn_on_part; abit *= 2)
    {
	if (worn_on_part & abit)
	{
	    switch (abit)
	    {
	    case A_TORSO:
	    case A_HEAD:	
	    case A_LEGS:
	    case A_R_ARM:
	    case A_L_ARM:
		chid = ({ abit });
		break;
	    case A_ROBE:  
		chid = ({ A_TORSO, A_LEGS });
		break;
	    case W_LEFT:
	    case W_RIGHT:
		chid = query_shield_slots();
		break;
	    default:
		chid = ({});
	    }
	    hids = (hids - chid) + chid;
	}
    }
    return hids;
#endif
}

/*
 * Nazwa funkcji : query_default_value
 * Opis          : Zwraca standardowa wartosc zbroji, wyliczona na podstawie
 *		   jej klas pacenrza i hitlokacji (czesci ciala), ktore
 *		   chroni.
 * Funkcja zwraca: int - srednia wartosc dla danego typu zbroji.
 */
public int
query_default_value()
{
    int val, *slots, ix, ac, double;

/* Nie wazne ktora strona ac_bits, liczy sie sama ilosc. Sloty parzyste
 * maja te sama wartosc.
 */
    val = (ac_slots | ac_side_bits); 
    for (ix = TS_TORSO, slots = ({ }); ix <= val; ix <<= 1)
         if (ix & val)
             slots += ({ ix });
             
    val = 0;
    ix = sizeof(slots);
    while (--ix >= 0)
    {
        ac = (ac_table[slots[ix]][0] + ac_table[slots[ix]][1] +
              ac_table[slots[ix]][2]) / 3;

        ac = (20 + ac * (ac + 21) / 3);
        switch(slots[ix])
        {
            case A_BODY:	val += 4 * ac / 5; break;
            case A_LEGS:	val += 3 * ac / 5; break;
            case A_HEAD:	val += ac; break;
            case A_R_ARM:	val += ac / 3; break;
            case A_L_ARM:	val += ac / 3; break;
            case A_R_FOOT:	val += ac / 4; break;
            case A_L_FOOT:	val += ac / 4; break;
            default:		val += 4 * ac / 5; break;
        }
    }
    
    return val;
}

/*
 * Nazwa funkcji : query_default_weight
 * Opis          : Zwraca standardowa wage zbroji, wyliczona na podstawie
 *		   jej klas pacenrza i hitlokacji (czesci ciala), ktore
 *		   chroni.
 * Funkcja zwraca: int - srednia waga dla danego typu zbroji.
 */
public int
query_default_weight()
{
    int val, *slots, ix, ac;
    
/* Nie wazne ktora strona ac_bits, liczy sie sama ilosc. Sloty parzyste
 * maja te sama wage.
 */
    val = (ac_slots | ac_side_bits); 
    for (ix = TS_TORSO, slots = ({ }); ix <= val; ix <<= 1)
         if (ix & val)
             slots += ({ ix });
             
    val = 0;
    ix = sizeof(slots);
    while (--ix >= 0)
    {
        ac = (ac_table[slots[ix]][0] + ac_table[slots[ix]][1] +
              ac_table[slots[ix]][2]) / 3;
/*              
        ac = 428 * ((ac > 1) ? ac - 1 : 1) + ((ac > 14) ? 10000 : 0);
 */
	ac = ((ac > 19) ? 875 : 500) * ac + 150;
        
        switch(slots[ix])
        {
            case A_BODY:	val += 5 * ac / 10; break;
            case A_LEGS:	val += 2 * ac / 10; break;
            case A_HEAD:	val += ac / 10; break;
            case A_R_ARM:	val += ac / 10; break;
            case A_L_ARM:	val += ac / 10; break;
            case A_R_FOOT:	val += ac / 6; break;
            case A_L_FOOT:	val += ac / 6; break;
            default:		val += ac / 20; break;
        }
    }
    
    return val;
}
 
/*
 * Function name: update_prop_settings
 * Description:   Will uppdate weight and value of this object to be legal
 */
nomask void
update_prop_settings()
{
    int waga, *slots, ix;
    if (max_value == -1)
	max_value = query_default_value();
 
    if (F_WEIGHT_FAULT_ARMOUR(query_prop(OBJ_I_WEIGHT), query_default_weight()) &&
			!query_prop(OBJ_I_IS_MAGIC_ARMOUR))
	add_prop(OBJ_I_WEIGHT, query_default_weight());
}

/*
 * Function name: query_worn
 * Description:   If this object is worn or not
 * Returns:       The object who wears this object if this object is worn
 */
object
query_worn()
{
    if (worn)
    {
	return wearer;
    }
}

public string
describe_slots()
{
    string str, *prot;
    int ix, typ;
    
    prot = ({});
    
    typ = arm_slots | slot_side_bits;
    for (ix = TS_TORSO; ix <= typ; ix <<= 1)
    {
        if (!(ix & typ))
            continue;

        switch (ix)
        {
            case A_BODY: prot += ({ "tulow" }); break;
            case A_HEAD: prot += ({ "glowe" }); break;
            case A_NECK: prot += ({ "szyje" }); break;
            case A_WAIST: prot += ({ "pas" }); break;
            case A_LEGS: prot += ({ "nogi" }); break;
            case A_R_ARM: prot += ({ ((ix & slot_side_bits) ? "dowolne" :
            	"prawe") + " ramie" }); break;
            case A_L_ARM: prot += ({ "lewe ramie" }); break;
            case A_R_FOREARM: prot += ({ ((ix & slot_side_bits) ? "dowolne" :
            	"prawe") + " przedramie" }); break;
            case A_L_FOREARM: prot += ({ "lewe przedramie" }); break;
            case A_R_HAND: prot += ({ ((ix & slot_side_bits) ? "dowolna" :
            	"prawa") + " dlon" }); break;
            case A_L_HAND: prot += ({ "lewa dlon" }); break;
            case A_R_FINGER: prot += ({ ((ix & slot_side_bits) ? "dowolny" :
            	"prawy") + " palec" }); break;
            case A_L_FINGER: prot += ({ "lewy palec" }); break;
            case A_R_FOOT: prot += ({ ((ix & slot_side_bits) ? "dowolna" :
            	"prawa") + " stope" }); break;
            case A_L_FOOT: prot += ({ "lewa stope" }); break;
            case A_ROBE: prot += ({ "barki jako plaszcz" }); break;
            default: prot += ({ "'" + ix + "'" }); break;
        }
    }
    
    str = COMPOSITE_WORDS(prot) + ".\n";
    
    return str;
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
    string str, tmp;
    int typ, *slots, *side_bits, ix;

    str = ::stat_object();

    str += capitalize(short(PL_BIE)) + " zaklada sie na " +
        describe_slots();

    
    for (side_bits = ({}), ix = TS_TORSO; ix <= ac_side_bits; ix <<= 1)
        if (ix & ac_side_bits)
            side_bits += ({ ix });
    slots = (int *)m_indices(ac_table) - side_bits;
    ix = sizeof(slots);
    if (ix)
    {
        typ = ac_side_bits << 1;
        str += "Chroni:               klute  ciete  obuchowe\n";
        while (--ix >= 0)
        {
            switch(slots[ix])
            {
                case A_BODY: tmp = "tulow"; break;
                case A_HEAD: tmp = "glowe"; break;
                case A_LEGS: tmp = "nogi"; break;
                case A_R_ARM: tmp = "prawe ramie"; break;
                case A_L_ARM: tmp = "lewe ramie"; break;
                default: tmp = "'" + slots[ix] + "'"; break;
            }
            
            str += sprintf("%20s:%5d, %5d, %5d;\n",
                capitalize((slots[ix] & typ ? "prawe lub " : "") + tmp),
                ac_table[slots[ix]][0], ac_table[slots[ix]][1],
                ac_table[slots[ix]][2] );
        }
    }
    else
        str += "Nie chroni zadnej hitlokacji.\n";
        
    if (!query_worn())
	wearer = this_player();
/*
    else if (arm_at == A_ANY_FINGER)
	str += "Jest to pierscien.\n";
	
    else
    	str += "Moze byc zalozon" + wear_how(arm_at) + ".\n";
*/

    str += "Trafienia: " + hits + "  Stan: " + condition + "  Naprawy: " + repair + "\n";
    if (parry_bonus)
        str += "Bonus do parowania: " + shield_size + "\n";

    return str + "\n";
}

/*
 * Function name: arm_condition_desc
 * Description:   Returns the description of this armour
 */
string
arm_condition_desc()
{
    string str;

    if (query_prop(OBJ_I_BROKEN))
	return (query_tylko_mn() ? "Sa" : "Jest") + " zupelnie zniszcz" +
	    koncowka("ony", "ona", "one", "eni", "one") + ".\n";

    switch(condition - repair)
    {
	case 0:
	    str = "w znakomitym stanie";
	    break;
	case 1:
	case 2:
	    str = "lekko podniszcz" + koncowka("ony", "ona", "one", "eni",
	         "one");
	    break;
	case 3:
	case 4:
	    str = "w kiepskim stanie";
	    break;
	case 5:
	case 6:
	case 7:
	    str = "w oplakanym stanie";
	    break;
	default:
	    str = "gotow" + koncowka("y", "a", "e", "i", "e") + 
	        " sie rozpasc w kazdej chwili";
	    break;
    }

    return "Wyglada na to, ze " + (query_tylko_mn() ? "sa" : "jest") + 
         " " + str + ".\n";
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
 * Description  : In some situations it is undesirable to have an armour 
 *                not recover. This function may then be used to force the
 *                armour to be recoverable.
 *
 *                This function may _only_ be called when a craftsman sells
 *                in armour he created to a player! It may expressly not be
 *                called in armours that are to be looted from NPC's!
 */
nomask void
may_recover()
{
    will_not_recover = 0;
}

/*
 * Function name: query_arm_recover
 * Description:   Return the recover strings for changing armour variables.
 * Returns:	  Part of the recoder string
 */
string
query_arm_recover()
{
    return "#ARM#" + hits + "#" + condition + "#" + repair + "#" +
	query_prop(OBJ_I_BROKEN) + "#";
}

/*
 * Function name: init_arm_recover
 * Description:   Initialize the armour variables at recover.
 * Arguments:	  arg - The recover string as recieved from query_arm_recover()
 */
void
init_arm_recover(string arg)
{
    string foobar;
    int    broken;

    sscanf(arg, "%s#ARM#%d#%d#%d#%d#%s", foobar,
	hits, condition, repair, broken, foobar);

    if (broken != 0)
    {
	add_prop(OBJ_I_BROKEN, 1);
    }
}

/*
 * Function name: query_recover
 * Description  : Called to check whether this armour is recoverable.
 *                If you have variables you want to recover yourself,
 *                you have to redefine this function, keeping at least
 *                the filename and the armour recovery variables, like
 *                they are queried below.
 *                If, for some reason, you do not want your armour to
 *                recover, you should define the function and return 0.
 * Returns      : string - the default recovery string.
 */
public string
query_recover()
{
    return MASTER + ":" + query_arm_recover();
}

/*
 * Function name: init_recover
 * Description  : When the object recovers, this function is called to set
 *                the necessary variables. If you redefine the function,
 *                you must add a call to init_arm_recover or a call to
 *                ::init_recover with the string that you got after querying
 *                query_arm_recover.
 * Arguments    : string argument - the arguments to parse
 */
public void
init_recover(string arg)
{
    init_arm_recover(arg);
}
