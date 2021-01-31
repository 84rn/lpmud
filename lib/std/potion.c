/*
 * /std/potion.c
 *
 * This is the standard object used for any form of potion.
 *
 * It works much the same way as herb.c.
 * Potions are quaffed btw ;-)
 */

#pragma save_binary
#pragma strict_types

inherit "/std/object";
inherit "/lib/herb_support";

#include <cmdparse.h>
#include <composite.h>
#include <files.h>
#include <macros.h>
#include <ss_types.h>
#include <stdproperties.h>

int	id_diff,		/* How difficult is it to id the potion? */
        magic_resistance,	/* How strong is the resistance against magic */
        soft_amount,
        alco_amount,
        potion_value,
        identified = 0,
        quaffed_it,
        rodzaj_mikstury;
object	*gFail;
string	*quaff_verb,
        *potion_names,
        id_long,
        unid_long,
        id_smell,
        unid_smell,
        id_taste,
        unid_taste;

/*
 * Function name: do_id_check
 * Description:   This little function is called each time the potion
 *                is referred to by a player, to check if (s)he
 *                identifies it or not.
 * Arguments:	  player - The player
 * Returns: 1 for identification, else 0.
 */
varargs int
do_id_check(object player)
{
    if (!objectp(player))
        player = this_player();

    if (objectp(player) && (
        ((environment(this_object()) == player) && identified) ||
        (id_diff <= player->query_skill(SS_ALCHEMY))
        ))
        return 1;
    else
        return 0;
}

int quaff_it(string str);
int taste_it(string str);
int smell_it(string str);

/*
 * Function name: init
 * Description:   adds the quaff-action to the player
 */
void
init()
{
    ::init(); /* If this isn't added cmd items won't work. */

    add_action(quaff_it, quaff_verb[0]);
    if (quaff_verb[0] != "wypij")
        add_action(quaff_it, "wypij");
    if (quaff_verb[0] != "posmakuj")
        add_action(taste_it, "posmakuj");
    if (quaff_verb[0] != "powachaj")
        add_action(smell_it, "powachaj");
}

void
leave_env(object from, object to)
{
    int ix;
    
    if (from && living(from))
    {
        for (ix = 0; ix < 6; ix++)
            remove_name(potion_names[ix], ix);
        identified = 0;
    }

    ::leave_env(from, to);
}

void
enter_env(object dest, object old)
{
    if (dest && living(dest) && do_id_check(dest))
    {
        dodaj_nazwy(potion_names, allocate(6), rodzaj_mikstury);
        identified = 1;
    }

    ::enter_env(dest, old);
}

/*
 * Function name: long_func
 * Description:   This is an VBFC function for the set_long in the
 *                potion, which tests if the player examining it can
 *                identify it.
 *                Make sure to set_id_long(str) and set_unid_long(str)
 *                from the create_potion() function.
 */
nomask string
long_func()
{
    if (do_id_check(this_player()))
        return id_long;
    return unid_long;
}

/*
 * Nazwa funkcji : set_quaff_verb
 * Opis          : Ustawia czasownik jakim gracz moze wypic dana miksturke.
 *		   Standardowo jest to 'wypij'. 
 * Argumenty     : Tablica czterech form czasownika:
 *		   ({ tryb rozkazujacy, 2 os (np. "wypij"), 
 *		      2 osoba (np. "wypijasz"),
 *		      3 osoba (np. "wypija") 
 *		      bezokolicznik (np. "wypic")
 *		    })
 */
void
set_quaff_verb(string *str)
{
    if (!pointerp(str) || sizeof(str) != 4)
        return ;
        
    if (str[0] != "wypij")
        quaff_verb = str;
}

/*
 * Function name: query_quaff_verb
 * Description:   What verb is required to quaff this potion?
 * Returns:	  The verb;
 */
string *
query_quaff_verb() 
{ 
    return quaff_verb; 
}

/*
 * Nazwa funkcji : ustaw_nazwe_mikstury
 * Opis          : Ustawia odmiane prawdziwej nazwy mikstury.
 * Argumenty	 : string * - tablica z odmiana prawdziwej nazwy mikstury
 *		   int - rodzaj gramatyczny tej nazwy
 */
void
ustaw_nazwe_mikstury(string *odmiana, int rodzaj)
{
    if (!pointerp(odmiana) || sizeof(odmiana) != 6)
        return ;
        
    potion_names = odmiana + ({});
    rodzaj_mikstury = rodzaj;
}

/*
 * Nazwa funkcji : query_nazwa_mikstury
 * Opis		 : Zwraca prawdziwa nazwe mikstury w podanym przypadku
 * Argumenty	 : przyp - przypadek
 * Funkcja zwraca: Nazwe mikstury w podanym przypadku
 */
string
query_nazwa_mikstury(int przyp)
{
    if (!pointerp(potion_names) || sizeof(potion_names) != 6)
        return 0;
        
    return potion_names[przyp];
}

/*
 * Function name: set_id_long
 * Description:   Set the long description you see if you know
 *                what potion it is.
 * Arguments:     str - The description
 */
void
set_id_long(string str) { id_long = str; }

/*
 * Function name: query_id_long
 * Description:   The long description if you can id the potion
 * Returns:       The long description
 */
string
query_id_long() { return id_long; }

/*
 * Function name: set_unid_long
 * Description:   Set the long description you see if you cannot identify the 
 *		  potion.
 * Arguments:     str - The long description
 */
void
set_unid_long(string str) { unid_long = str; }

/*
 * Function name: query_unid_long
 * Description:   Query the long description you get if you cannot identify
 *		  the potion.
 * Returns:   	  The unidentified long description
 */
string
query_unid_long() { return unid_long; }

/*
 * Function name: set_id_taste
 * Description:   Set the message when you taste the identified potion
 * Arguments:     str - The message
 */
void
set_id_taste(string str) { id_taste = str; }

/*
 * Function name: query_id_taste
 * Description:   Query the message you get if you taste and identify
 *                the potion.
 * Returns:   	  The taste of the identified potion
 */
string
query_id_taste() { return id_taste; }

/*
 * Function name: set_id_smell
 * Description:   Set the message when you smell the identified potion
 * Arguments:     str - The message
 */
void
set_id_smell(string str) { id_smell = str; }

/*
 * Function name: query_id_smell
 * Description:   Query the message you get if you smell and identify
 *                the potion.
 * Returns:   	  The smell of the identified potion
 */
string
query_id_smell() { return id_smell; }

/*
 * Function name: set_unid_taste
 * Description:   Set the message when you taste the unidentified potion
 * Arguments:     str - The message
 */
void
set_unid_taste(string str) { unid_taste = str; }

/*
 * Function name: query_unid_taste
 * Description:   Query the message you get if you taste and do not
 *                identify the potion.
 * Returns:   	  The taste of the unidentified potion
 */
string
query_unid_taste() { return unid_taste; }

/*
 * Function name: set_unid_smell
 * Description:   Set the message when you smell the unidentified potion
 * Arguments:     str - The message
 */
void
set_unid_smell(string str) { unid_smell = str; }

/*
 * Function name: query_unid_smell
 * Description:   Query the message you get if you smell and do not
 *                identify the potion.
 * Returns:   	  The smell of the unidentified potion
 */
string
query_unid_smell() { return unid_smell; }

/*
 * Function name: set_id_diff
 * Description:   Set how hard it is to identify a potion
 * Arguments:     i - The skill needed to know the potion
 */
void
set_id_diff(int i) { id_diff = i; }

/*
 * Function name: query_id_diff
 * Description:   How hard is it to identify this potion
 * Returns:  	  The difficulty
 */
int
query_id_diff() { return id_diff; }

/*
/*
 * Function name: set_soft_amount
 * Description:   Set the soft amount of the potion
 * Argument:      int - the soft amount
 */
void 
set_soft_amount(int i)
{
    if (i > 10)
        soft_amount = i;
    else
        soft_amount = 10;

    add_prop(OBJ_I_VOLUME, soft_amount);
    add_prop(OBJ_I_WEIGHT, soft_amount);
}

/*
 * Function name: query_soft_amount
 * Description:   What is the soft amount of the potion
 * Returns:       The soft amount
 */
int 
query_soft_amount(int i) { return soft_amount; }

/*
 * Function name: set_alco_amount
 * Description:   Set the alco amount of the potion
 * Argument:      int - the alco amount
 */
void 
set_alco_amount(int i) { alco_amount = i; }

/*
 * Function name: query_alco_amount
 * Description:   What is the alco amount of the potion
 * Returns:       The alco amount
 */
int 
query_alco_amount(int i) { return alco_amount; }

/*
 * Function name: set_potion_value
 * Description:   Set the value of the potion when dealing with
 *                a potion specialist
 * Arguments:     i - The value
 */
void
set_potion_value(int i) { potion_value = i; }

/*
 * Function name: query_potion_value
 * Description:   The value of the potion when dealing with a specialist
 * Returns:	  The value
 */
int
query_potion_value() { return potion_value; }

/*
 * Function name:       set_quaffed_it
 * Description:         This is called if the potion is quaffed and the 
 *                      quaff_verb != "quaff
 */
void
set_quaffed_it() { quaffed_it = 1; }

/*
 * Function name: quaffed_non_quaff_potion
 * Description:   This function is called instead of do_herb_effects
 *                if you quaff a potion that isn't supposed to be
 *                quaffed.
 */
void
quaffed_non_quaff_potion()
{
    write("Nie czujesz zadnego efektu.\n");
}

/*
 * Function name: query_identified
 * Description:   Did you identify the potion?
 * Returns:       True if identified
 */
int
query_identified() { return identified; }

varargs void
set_identified(int i = 1) { identified = i; }

public void
create_potion()
{
    ::create_object();
    dodaj_przym("nieznany", "nieznani");
}

public nomask void
create_object()
{
    ustaw_nazwe( ({ "mikstura", "mikstury", "miksturze", "miksture",
        "mikstura", "miksturze" }), ({ "mikstury", "mikstur", "miksturom",
        "mikstury", "miksturami", "miksturach" }), PL_ZENSKI);

    set_id_long("Ta mikstura nie zostala opisana przez jej stworce.\n");
    set_unid_long("Jest to nieznana, nieopisana mikstura.\n");
    set_unid_smell("Wydaje ci sie, ze mikstura ta nie ma zapachu.\n");
    set_id_smell("Mikstura ta nie ma zapachu.\n");
    set_unid_taste("Wydaje ci sie, ze mikstura ta jest bez smaku.\n");
    set_id_taste("Mikstura ta jest bez smaku.\n");
    ustaw_nazwe_mikstury( ({ "xmikstura", "xmikstury", "xmiksturze",
        "xmiksture", "xmikstura", "xmiksturze" }), PL_ZENSKI);
    set_soft_amount(50);
    set_alco_amount(0);
    set_id_diff(20);
    set_potion_value(10);
    quaff_verb = ({ "wypij", "wypijasz", "wypija", "wypic" });

    add_prop(OBJ_I_VALUE, 0);

    create_potion();
    set_long("@@long_func");
}

public nomask void
reset_object() 
{ 
    this_object()->reset_potion();
}

void
consume_text(object *arr, string *vb)
{
    string str;

    write(capitalize(vb[1]) + " " + (str = COMPOSITE_DEAD(arr, PL_BIE)) + ".\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " " + vb[2] + " " + str + ".\n");
}

/*
 * Function name:	quaff_it
 * Description:		Quaffs the objects described as parameter to the
 *                      quaff_verb. It uses command parsing to find which
 *                      objects to quaff.
 * Arguments:		str: The trailing command after 'quaff_verb ...'
 * Returns:		True if command successful
 */
public int
quaff_it(string str)
{
    int	    il;
    object *a,
           *potions;
    string  str2,
            *vb;

    if (this_player()->query_prop(TEMP_STDPOTION_CHECKED) ||
	query_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT))
    {
        return 0;
    }

    gFail = ({ });
    
    notify_fail(capitalize(query_verb()) + " co?\n");  /* access failure */
    
    if (query_verb() == "wypij")
         vb = ({ "wypij", "wypijasz", "wpija", "wypic" });
    else
        vb = query_quaff_verb(); 

    if (!stringp(str))
        return 0;

/* It's impossible to quaff all potions at once.
 */
    if (str == "wszystko")
    {
        notify_fail("Musisz sprecyzowac, o ktora miksture ci chodzi.\n");
        return 0;
    }

    a = CMDPARSE_ONE_ITEM(str, "quaff_one_thing", "quaff_access");
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
	    &(this_player())->remove_prop(TEMP_STDPOTION_CHECKED));
        this_player()->add_prop(TEMP_STDPOTION_CHECKED, 1);
        if (sizeof(gFail))
            notify_fail("@@quaff_fail:" + file_name(this_object()));
        return 0;
    }
}

string
quaff_fail()
{
    string str;

    str = "Probujesz " + quaff_verb[3] + " " + COMPOSITE_DEAD(gFail, PL_BIE) +
        ", ale chyba nie dasz rady.\n";
    saybb(QCIMIE(this_player(), PL_MIA) + " probuje " + quaff_verb[3] + " " +
        QCOMPDEAD(PL_BIE) + ", ale chyba nie daje rady.\n");
    this_player()->remove_prop(TEMP_STDPOTION_CHECKED);
    return str;
}

int
quaff_access(object ob)
{ 
    string vb = query_verb();

    if ((environment(ob) == this_player()) &&
        (function_exists("create_object", ob) == POTION_OBJECT) &&
        (vb == ob->query_quaff_verb()[0] || vb == "wypij") &&
        !ob->query_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT))
        return 1;
    else
        return 0;
}

int
quaff_one_thing(object ob)
{
    int     soft = ob->query_soft_amount(),
            alco = ob->query_alco_amount();
    string  vb = query_verb();

    if (!(this_player()->drink_soft(soft)))
    {
        write(capitalize(ob->short(PL_MIA)) + " to troche za duzo "+
            "dla ciebie.\n");
        gFail += ({ ob });
        return 0;
    }
    if (!(this_player()->drink_alco(alco)))
    {
        this_player()->drink_soft(-soft);
	write(capitalize(ob->short(PL_MIA)) + " jest nieco za mocn" +
	    ob->koncowka("y", "a", "e") + " dla ciebie.\n");
        gFail += ({ ob });
        return 0;
    }
    /* Test if you quaffed a non quaff potion */
    if ((vb == "wypij") && (vb != ob->query_quaff_verb()[0]))
            ob->set_quaffed_it();
    
    return 1;
}

/*
 * Function name: destruct_object
 * Description:   Call do_herb_effects or quaffed_non_quaff_potion
 *                Clone an empty vial and remove potion
 */
void
destruct_object()
{
    object  vial;

    if (quaffed_it && quaff_verb[0] != "wypij")
    {
        quaffed_non_quaff_potion();
    }
    else
    {
        do_herb_effects();
    }

    seteuid(getuid());
    vial = clone_object("/std/container");
    vial->ustaw_nazwe( ({ "flakonik", "flakonika", "flakonikowi", "flakonik",
        "flakonikiem", "flakoniku" }), ({ "flakoniki", "flakonikow",
        "flakonikom", "flakoniki", "flakonikami", "flakonikach" }), 
        PL_MESKI_NOS_NZYW);
    vial->add_name(({"_std_potion_vial"}));
    vial->ustaw_przym("pusty", "pusci");
    vial->set_long("Pusty flakonik. Mozesz go wypelnic jakas mikstura.\n");
    vial->add_prop(CONT_I_MAX_VOLUME, 1100);
    vial->add_prop(CONT_I_MAX_WEIGHT, 1250);
    vial->add_prop(CONT_I_VOLUME, 100);
    vial->add_prop(CONT_I_WEIGHT, 250);
    if (vial->move(environment(this_object())))
        vial->move(environment(this_object()), 1);

    add_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT, 1);
    set_alarm(1.0, 0.0, remove_object);
}

/*
 * Function name: set_magic_res
 * Description:   Set how resistance this potion is agains magic / how easy it
 *		  is to dispel it.
 * Arguments:     resistance - the resistance
 */
void set_magic_res(int resistance) { magic_resistance = resistance; }

/*
 * Function name: query_magic_resistance
 * Description:   Query the magic resistance
 * Returns:       How resistive the potion is
 */
int
query_magic_res(string prop)
{ 
    if (prop == MAGIC_I_RES_MAGIC)
        return magic_resistance;
    else
        return 0;
}

/*
 * Function name: dispel_magic
 * Description:   Function called by a dispel spell
 * Argument:	  magic - How strong the dispel is
 * Returns:	  0 - No dispelling, 1 - Object dispelled
 */
int
dispel_magic(int magic)
{
    object env = environment(this_object());

    if (magic < query_magic_res(MAGIC_I_RES_MAGIC))
        return 0;

    if (living(env))
    {
        tell_room(environment(env), QCSHORT(this_object(), PL_MIA) +
            ", ktor" + koncowka("y", "a", "e") + " " + QIMIE(env, PL_MIA) + 
            " ma przy sobie, nagle zaczyna swieciec na bialo i eksploduje!\n", 
            env);
        env->catch_msg(QCSHORT(this_object(), PL_MIA) + ", ktor" + 
            koncowka("y", "a", "e") + " masz przy sobie nagle zaczyna "+
            "swiecic na bialo i eksploduje!\n");
    }
    else if (env->query_prop(ROOM_I_IS))
        tell_room(env, QCSHORT(this_object(), PL_MIA) + " nagle zaczyna "+
            "swiecic na bialo i exploduje!\n"); 

    return 1;
}

int
smell_access(object ob)
{ 
    string vb = query_verb();

    if ((environment(ob) == this_player()) &&
        (function_exists("create_object", ob) == POTION_OBJECT) &&
        (vb == "powachaj" || vb == "posmakuj"))
        return 1;
    else
        return 0;
}

int
smell_one_thing(object ob)
{
    object  pl = this_player();
    string  vb = query_verb();

    if (ob->query_identified() ||
        (ob->query_id_diff() <= (pl->query_skill(SS_ALCHEMY) * 150 +
                     pl->query_skill(SS_HERBALISM) * 50) / 100) )
    {
        ob->dodaj_nazwy(potion_names, allocate(6), query_rodzaj());
        ob->set_identified(1);
        if (vb == "posmakuj")
            write(ob->query_id_taste());
        else if (vb == "powachaj")
            write(ob->query_id_smell());
    }
    else
    {
        if (vb == "posmakuj")
            write(ob->query_unid_taste());
        else if (vb == "powachaj")
            write(ob->query_unid_smell());
    }
    
    return 1;
}

/*
 * Function name: smell_it
 * Description:   Smell the potion, but do not quaff it
 *                If smelling should call do_herb_effects set
 *                quaff_verb to "smell"
 * Arguments:     str - the argument of the command
 * Returns:       True is successful
 */
int
smell_it(string str)
{
    int     il;
    object  *a;
    string  *vb;

    if (this_player()->query_prop(TEMP_STDPOTION_CHECKED) ||
	query_prop(TEMP_OBJ_ABOUT_TO_DESTRUCT))
    {
        return 0;
    }

    if (query_verb() == "powachaj")
         vb = ({ "powachaj", "wachasz", "wacha", "powachac" });
    else if (query_verb() == "posmakuj")
         vb = ({ "posmakuj", "smakujesz", "smakuje", "posmakowac" });
    else
        vb = query_quaff_verb(); 

/* It's impossible to smell all potions at once.
 */
    if (str == "wszystko")
    {
        notify_fail("Musisz zdecydowac sie na konkretna miksture.\n");
        return 0;
    }

    notify_fail(capitalize(vb[0]) + " co?\n");  /* access failure */
    if (!stringp(str))
	return 0;

    a = CMDPARSE_ONE_ITEM(str, "smell_one_thing", "smell_access");
    if (sizeof(a) > 0)
    {
        saybb(QCIMIE(this_player(), PL_MIA) + " " + vb[2] + " " +
            COMPOSITE_DEAD(a, PL_BIE) + ".\n");
        return 1;
    }
    else
    {
	set_alarm(1.0, 0.0,
	    &(this_player())->remove_prop(TEMP_STDPOTION_CHECKED));
        this_player()->add_prop(TEMP_STDPOTION_CHECKED, 1);
        return 0;
    }
}

/*
 * Function name: taste_it
 * Description:   Taste the potion, but do not quaff it
 *                If tasting should call do_herb_effects set
 *                quaff_verb to "taste"
 * Arguments:     str - the argument of the command
 * Returns:       True is successful
 */
int
taste_it(string str)
{
    return smell_it(str);
}
