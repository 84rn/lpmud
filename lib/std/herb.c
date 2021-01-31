/*
 *  The standard herb.
 *
 *  The original made by Elessar Telcontar of Gondor, 
 *		Genesis, April to July 1992.
 */

#pragma save_binary

inherit "/std/object";
inherit "/lib/herb_support";

#include <cmdparse.h>
#include <composite.h>
#include <files.h>
#include <language.h>
#include <macros.h>
#include <ss_types.h>
#include <stdproperties.h>

/*
 * Prototype.
 */
public int ingest_it(string str);
int decay();

/*
 * Variables
 */
int 	find_diff, id_diff, decay_time, herb_value, inited_once;
int     food_amount, dried, dryable, ate_it, decay_left, decay_stopped;
int	rodzaj_nazwy_ziola;
string	id_long, unid_long, *herb_names, *ingest_verb;
object  *gFail;

static private int decay_alarm;

/*
 * Function name: do_id_check
 * Description:   This little function is called each time the herb is referred
 *                to by a player, to check if (s)he identifies it or not.
 * Arguments:	  player - The player
 * Returns: 1 for identification, else 0.
 */
varargs int
do_id_check(object player)
{
    if (!player)
	player = this_player();

    if (player && id_diff <= player->query_skill(SS_HERBALISM))
	return 1;
    else
	return 0;
}

/*
 * Function name: init
 * Description:   adds the ingest-action to the player
 */
void
init()
{
    ::init(); /* If this isn't added cmd items won't work. */

    add_action(ingest_it, ingest_verb[0]);
    if (ingest_verb[0] != "zjedz")
        add_action(ingest_it, "zjedz");
}

void
leave_env(object from, object to)
{
    int ix;
    
    if (from && living(from))
    {
        for (ix = 0; ix < 6; ix++)
            remove_name(herb_names[ix], ix);
    }

    ::leave_env(from, to);
}

/* Start the decay when the herb enters a living,  i.e when it's picked. */

void
enter_env(object dest, object old)
{
    int ix;
    
    if (dest && living(dest) && do_id_check(dest))
	while (--ix >= 0)
	{
	    add_name(herb_names[ix], ix, rodzaj_nazwy_ziola);
	    
	}

    
	dodaj_nazwy(herb_names, allocate(6));
	
    if (dest && living(dest) && !inited_once)
    {
    	if (decay_time)
	    decay_alarm = set_alarm(itof(decay_time), 0.0, decay);
    	inited_once = 1;
    }

    ::enter_env(dest, old);
}

/*
 * Function name: long_func
 * Description:   This is an VBFC function for the set_long in the herb, which
 *                tests if the player examining it can identify it, before
 *                returning the appropriate long-description. To make this
 *                work, you must be sure to do set_id_long(str) and
 *                set_unid_long(str) from the create_herb() function.
 */
nomask string
long_func()
{
    if (do_id_check(this_player()))
	return id_long;
	
    return unid_long;
}

/*
 * Nazwa funkcji : set_ingest_verb
 * Opis          : Ustawia czasownik jakim gracz moze spozyc dane ziolo.
 *		   Chodzi tu o spozycie z efektem, np. 'przezucie', itp.
 *		   Ale moze byc oczywiscie tez po prostu 'zjedzenie'.
 * Argumenty     : Tablica czterech form czasownika:
 *		   ({ tryb rozkazujacy, 2 os. (np. "zjedz"), 
 *		      2 osoba (np. "zjadasz"),
 *		      3 osoba (np. "zjada") 
 *		      bezokolicznik (np "zjesc")
 *		    })
 */
void
set_ingest_verb(string *str) 
{ 
    if (!pointerp(str) || sizeof(str) != 4)
        return; 
        
    ingest_verb = str; 
}

/*
 * Function name: query_ingest_verb
 * Description:   What verb is required to ingest this herb?
 * Returns:	  The verb;
 */
string *
query_ingest_verb() 
{ 
    return ingest_verb; 
}

/*
 * Function name: set_decay_time
 * Description:	  Set how long time it takes for the herb to decay
 * Argumetns:	  i - The time (in seconds)
 */
void
set_decay_time(int i) 
{ 
    decay_time = i; 
}

/*
 * Function name: query_decay_time
 * Description:   How long time does it take for the herb to decay?
 * Returns:	  The time in seconds
 */
int
query_decay_time() 
{ 
    return decay_time; 
}

/*
 * Function name: set_dryable
 * Description:   Calling this function makes the herb dryable
 */
void
set_dryable() 
{ 
    if (!dried) dryable = 1; 
}

/*
 * Function name: query_dryable
 * Description:   Is the function dryable?
 * Returns:       1 if it is dryable
 */
int
query_dryable() 
{ 
    return dryable; 
}

/*
 * Function name: dry
 * Description:   This function is called when a herb dries, to allow
 *                different effects for dried herbs.
 */
void
dry() 
{
}

/*
 * Function name: force_dry
 * Description:   Call this function if you want to make the herb
 *                dry after creation. E.g. if you have some tool that
 *                makes a herb dry. If you want to set the herb 
 *                to dried at creation, use set_dried.
 */
void
force_dry()
{
    string *short_temp, *pshort_temp;
    int x;

    if (dried || !dryable) return;

    dryable = 0;
    dried = 1;
    
    this_object()->dry();
    
    short_temp = allocate(6);
    pshort_temp = allocate(6);
    
    for (x = 0; x < 6; x++)
    {
        short_temp[x] = oblicz_przym("ususzony", "ususzeni", x, 
            this_object()->query_rodzaj(), 0) + " " + short(x);
            
        pshort_temp[x] = oblicz_przym("ususzony", "ususzeni", x,
            this_object()->query_rodzaj(), 1) + " " + plural_short(x);
    }

    ustaw_shorty(short_temp, pshort_temp, this_object()->query_rodzaj());

    dodaj_przym("ususzony", "ususzeni");

    if (decay_stopped)
    {
	decay_left = decay_time * 5 + random(2 * decay_time);
    }
    else
    {
	remove_alarm(decay_alarm);
	decay_alarm = set_alarm(itof(decay_time * 5) +
	    rnd() * (2.0 + itof(decay_time)), 0.0, decay);
    }
    inited_once = 1;
}
 
/*
 * Function name: set_dried
 * Description:   Set the herb to dried. Use this function in
 *                create_herb. If you want to make the herb dry
 *                after creation, use force_dry.
 */
void
set_dried()
{
    if (!dried)
    {
	dried = 1;
	dryable = 0;
	remove_alarm(decay_alarm);
	decay_alarm = set_alarm(itof(decay_time * 5) + rnd() * (2.0 + itof(decay_time)), 0.0, decay);
	inited_once = 1;
    }
}

/*
 * Function name: query_dried
 * Description:   Is the herb dried?
 * Returns:	  1 if dried
 */
int
query_dried() 
{
    return dried; 
}

/*
 * Nazwa funkcji : ustaw_nazwe_ziola
 * Opis          : Ustawia odmiane prawdziwej nazwy ziola.
 * Argumenty	 : string * - tablica z odmiana prawdziwej nazwy ziola
 *		   int - rodzaj tej nazwy
 */
void
ustaw_nazwe_ziola(string *odmiana, int rodzaj)
{
    if (!pointerp(odmiana) || sizeof(odmiana) != 6)
        return ;
        
    herb_names = odmiana;
    rodzaj_nazwy_ziola = rodzaj;
//    dodaj_rodzaj(rodzaj);
}

/*
 * Nazwa funkcji : query_nazwa_ziola
 * Opis		 : Zwraca prawdziwa nazwe ziola w podanym przypadku
 * Argumenty	 : przyp - przypadek
 * Funkcja zwraca: Nazwe ziola w podanym przypadku
 */
string
query_nazwa_ziola(int przyp)
{
    if (!pointerp(herb_names) || sizeof(herb_names) != 6)
        return 0;
        
    return herb_names[przyp];
}

/*
 * Function name: set_id_long
 * Description:   Set the long description you see if you know what herb it
 *		  is.
 * Arguments:     str - The description
 */
void
set_id_long(string str) 
{
    id_long = str; 
}

/*
 * Function name: query_id_long
 * Description:   The long description if you can id the herb
 * Returns:       The long description
 */
string query_id_long() 
{ 
    return id_long; 
}

/*
 * Function name: set_unid_long
 * Description:   Set the long description you see if you cannot identify the 
 *		  herb.
 * Arguments:     str - The long description
 */
void
set_unid_long(string str) 
{
     unid_long = str; 
}

/*
 * Function name: query_unid_long
 * Description:   Query the long description you get if you cannot identify
 *		  the herb.
 * Returns:   	  The unidentified long description
 */
string
query_unid_long() 
{ 
    return unid_long; 
}

/*
 * Function name: set_id_diff
 * Description:   Set how hard it is to identify a herb
 * Arguments:     i - The skill needed to know the herb
 */
void
set_id_diff(int i) 
{ 
    id_diff = i; 
}

/*
 * Function name: query_id_diff
 * Description:   How hard is it to identify this herb
 * Returns:  	  The difficulty
 */
int
query_id_diff() 
{ 
    return id_diff;
}

/*
 * Function name: set_find_diff
 * Description:   Set how hard it is to find the herb
 * Arguments:     i - Difficulty (suggested range is 0 - 10)
 */
void
set_find_diff(int i) 
{ 
    find_diff = i; 
}

/*
 * Function name: query_find_diff
 * Description:   How hard is it to find this herb
 * Returns: 	  The difficulty to find the herb in the nature
 */
int
query_find_diff() 
{
    return find_diff; 
}

/*
 * Function name: set_herb_value
 * Description:   Set the value of the herb when dealing with a herb specialist
 * Arguments:     i - The value
 */
void
set_herb_value(int i) 
{
    herb_value = i; 
}

/*
 * Function name: query_herb_value
 * Description:   The value of the herb when dealing with a specialist
 * Returns:	  The value
 */
int
query_herb_value() 
{
    return herb_value; 
}

/*
 * Function name:       set_amount
 * Description:         sets the amount of food in this herb (in grams)
 * Arguments:           a: The amount of food
 */
public void
set_amount(int a) 
{ 
    food_amount = a; 
    add_prop(OBJ_I_VOLUME, a / 10);
    add_prop(OBJ_I_WEIGHT, a);
}

/*
 * Function name:       query_amount
 * Description:         Gives the amount of food in this herb
 * Returns:             Amount as int (in grams)
 */
public int
query_amount() 
{
    return food_amount; 
}

/*
 * Function name:       set_ate_it
 * Description:         This is called if the herb is eaten and the 
 *                      ingest_verb != "eat"
 */
void
set_ate_it() 
{
    ate_it = 1; 
}

/*
 * Function name: ate_non_eat_herb
 * Description:   This function is called in stead of do_herb_effect
 *                if you eat a herb that isn't supposed to be eaten
 */
void
ate_non_eat_herb()
{
    write("Nie czujesz zadnego efektu.\n");
}

/* 
 * Function name: decay
 * Description:   This function is called with an alarm, and simulates the
 *                somewhat rapid decay of herbs - herbs lose their effect
 *                soon after being picked, if they don't dry and last longer.
 *                To make a herb dry, do set_dryable() from create_herb().
 *                set_dried() can be used to dry the herb immediately.
 */
int
decay()
{
    object env;
    string mess;

    decay_alarm = 0;
    if (dryable)
    {
        force_dry();
    	return 1;
    }

    if (env = environment())
    {
	mess = capitalize(short(PL_MIA) + " " + (query_tylko_mn() ? "sa"
	    : "jest") + " tak su" + 
	    koncowka("chy", "cha", "che", "si", "che") +
	    ", ze obraca" + koncowka("", "", "", "ja", "ja") + " sie w " +
	    "proch.\n");
	if (living(env))
	    tell_object(env, mess);
	else if (env->query_prop(ROOM_I_IS))
	    tell_roombb(env, mess, ({}), this_object());
    }
	    
    remove_object();
    return 1;
}

/*
 * Function name: stop_decay
 * Description:   Temporarily stop the decay process of the herb.
 *                Restart it again with restart_decay.
 */
void
stop_decay()
{
    if (!inited_once || decay_stopped)
	return;
    decay_left = ftoi(get_alarm(decay_alarm)[2]);
    remove_alarm(decay_alarm);
    decay_alarm = 0;
    decay_stopped = 1;
} 

/*
 * Function name: restart_decay
 * Description:   Restart the decay process of a herb that was stopped
 *                by stop_decay.
 */
void
restart_decay()
{
  if (!decay_stopped) return;
  decay_alarm = set_alarm(itof(decay_left), 0.0, decay);
  decay_stopped = 0;
}

/*
 * Function name: start_decay
 * Description:   Start the decay process of a herb
 */
void
start_decay()
{
    if (!inited_once)
    {
    	if (decay_time)
	    decay_alarm = set_alarm(itof(decay_time), 0.0, decay);
    	inited_once = 1;
    }
}

/*
 * Function name: query_decay_stopped
 * Description:   Check if the decay process has been stopped.
 */
int query_decay_stopped() 
{
    return decay_stopped; 
}

/*
 * Function name: create_herb
 * Description:   This is the create-function of the herb, which you should
 *                redefine and setup the herb from.
 */
void
create_herb() 
{
    dodaj_przym("nieznany", "nieznani");
}

nomask void
create_object()
{
    ustaw_nazwe( ({ "ziolo", "ziola", "ziolu", "ziolo", "ziolem", "ziele" }),
        ({ "ziola", "ziol", "ziolom", "ziola", "ziolami", "ziolach" }),
        PL_NIJAKI_NOS );
        
    set_id_long("To ziolo nie zostalo opisane przez jego stworce.\n");
    set_unid_long("Jest to nieopisane, nieznane ziolo.\n");
    ustaw_nazwe_ziola( ({ "xziolo", "xziola", "xziolu", "xziolo", 
        "xziolem", "xziole" }), PL_NIJAKI_NOS);
    set_amount(2);
    set_decay_time(300);
    set_id_diff(20);
    set_find_diff(5);
    set_herb_value(10);
    set_ingest_verb( ({ "zjedz", "zjadasz", "zjada", "zjesc" }) );
    add_prop(OBJ_I_VALUE, 0);
    add_prop(OBJ_I_WEIGHT, 25);
    add_prop(OBJ_I_VOLUME, 25);
    create_herb();
    set_long("@@long_func");
}

nomask void
reset_object() 
{
    this_object()->reset_herb(); 
}

void
consume_text(object *arr, string *vb)
{
    string str;

    write(capitalize(vb[1]) + " " + (str = COMPOSITE_DEAD(arr, PL_BIE)) + ".\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " " + vb[2] + " " + str + ".\n");
}

/*
 * Function name:	ingest_it
 * Description:		Ingests the objects described as parameter to the
 *                      ingest_verb. It uses command parsing to find which
 *                      objects to ingest.
 * Arguments:		str: The trailing command after 'ingest_verb ...'
 * Returns:		True if command successfull
 */
public int
ingest_it(string str)
{
    object 	*a, *foods;
    int		il;
    string str2, *vb;
    
    if (this_player()->query_prop(TEMP_STDHERB_CHECKED) ||
	query_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT))
    {
	return 0;
    }

    gFail = ({ });
    
    if (query_verb() == "zjedz")
        vb = ({ "zjedz", "zjadasz", "zjada" });
    else
        vb = query_ingest_verb();
    
/* If you type "eat all" and the command comes to a herb first
 * it would cause all eatable herbs to be eaten but no food.
 * This is to prevent this. (How often do you want to eat all herbs?)
 * This also stops "smoke all" "chew all" and so on. 
 */
    if (str == "wszystko")
    {
	notify_fail("Musisz sprecyzowac, o ktore ziolo ci chodzi.\n");
	return 0;
    }

    notify_fail(capitalize(vb[0]) + " co?\n");  /* access failure */
    if (!stringp(str))
	return 0;
    
    a = CMDPARSE_ONE_ITEM(str, "ingest_one_thing", "ingest_access");
    if (sizeof(a) > 0)
    {
	consume_text(a, vb);
	for (il = 0; il < sizeof(a); il++)
	    a[il]->destruct_object();
	return 1;
    }
    else
    {
	set_alarm(1.0, 0.0,
	    &(this_player())->remove_prop(TEMP_STDHERB_CHECKED));
	this_player()->add_prop(TEMP_STDHERB_CHECKED, 1);
	if (sizeof(gFail))
	    notify_fail("@@ingest_fail:" + file_name(this_object()));
	return 0;
    }
}

string
ingest_fail()
{
    string str;

    str = "Probujesz " + ingest_verb[3] + " " + 
        COMPOSITE_DEAD(gFail, PL_BIE) + ", ale juz nie mozesz.\n";
    saybb(QCIMIE(this_player(), PL_MIA) + " probuje " + ingest_verb[3] + " " + 
        QCOMPDEAD(PL_BIE) + ", ale juz nie moze.\n");
    this_player()->remove_prop(TEMP_STDHERB_CHECKED);
    return str;
}

int
ingest_access(object ob)
{ 
    string vb;

    vb = query_verb();

    if ((environment(ob) == this_player()) &&
	(function_exists("create_object", ob) == HERB_OBJECT) &&
	(vb == ob->query_ingest_verb()[0] || vb == "zjedz") &&
        !ob->query_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT))
	return 1;
    else
	return 0;
}

int
ingest_one_thing(object ob)
{
    int am;
    string vb;

    vb = query_verb();
    am = (int) ob->query_amount();
    
    if (vb == "zjedz")
    {
        if (!this_player()->eat_food(am))
	{
	    write(capitalize(ob->short()) +
		  " to troche za duzo dla twojego zoladka.\n");
	    gFail += ({ ob });
	    return 0;
	}
	/* Test if you ate a non_eat herb */
	if (vb != ob->query_ingest_verb()[0])
	    ob->set_ate_it();
	    
	return 1;
    }
    
    return 1;
}

void
destruct_object()
{
    if (ate_it && ingest_verb[0] != "zjedz")
    {
	ate_non_eat_herb();
    } 
    else 
    {
	do_herb_effects();
    }

    add_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT, 1);
    set_alarm(1.0, 0.0, remove_object);
}

/*
 * Function name: query_herb_recover
 * Description:   Return the recover strings for changing herb variables.
 */
string
query_herb_recover()
{
    int dec;

    if (decay_stopped || !decay_alarm)
	dec = decay_left;
    else
	dec = ftoi(get_alarm(decay_alarm)[2]);
    return "#h_i#" + inited_once +"#h_s#"+ decay_stopped + "#h_d#" + dec +
	   "#h_dr#" + dried + "#";
}

/*
 * Function name: init_herb_recover
 * Description:   Initialize the herb variables at recover.
 */
void
init_herb_recover(string arg)
{
    string foobar;
    int i,d,dr,s;

    sscanf(arg, "%s#h_i#%d#%s", foobar, i, foobar);
    sscanf(arg, "%s#h_s#%d#%s", foobar, s, foobar);
    sscanf(arg, "%s#h_d#%d#%s", foobar, d, foobar);
    sscanf(arg, "%s#h_dr#%d#%s", foobar, dr, foobar);
    if (i == 1) 
    {
	inited_once = 1;
	remove_alarm(decay_alarm);
	decay_alarm = set_alarm(itof(d), 0.0, decay);
    }
    if (dr && !dried)
	force_dry();
    if (s)
    {
	remove_alarm(decay_alarm);
	decay_stopped = 1;
	decay_left = d;
    }
}

/*
 * Function name: query_recover
 * Description:   A default query_recover() for herbs.
 * Returns:	  A default recovery string.
 */
string
query_recover()
{
    return MASTER + ":" + query_herb_recover();
}

/*
 * Function name: init_recover
 * Description:   A default init_recover() for herbs.
 * Arguments:	  arg - String with variables to recover.
 */
void
init_recover(string arg)
{
    init_herb_recover(arg);
}
