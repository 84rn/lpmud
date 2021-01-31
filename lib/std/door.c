/* 
 * door.c
 *
 * This is the standard door object.
 *
 * User function in this object:
 * -----------------------------
 *
 * Argumenty:	        s = string, i = integer, o = object, m = mixed
 *			*x = tablica podanego typu.
 *			** niezbedna funkcja. 
 *			-* niezbedna w pewnych okolicznosciach.
 *
 * Wszystkie funkcje set_ maja odpowiedniki w query_.
 *
 *
 * create_door()	- Glowna funkcja; w niej powinienes umiescic
 *			  wywolania wszystkich pozostalych funkcji.
 *
** set_other_room(s)	- Wywolaj te funkcje ze sciezka do lokacji
 *			  po drugiej stronie drzwi.
 *
** set_door_id(s)	- Wywolaj te funkcje, jako argument podajac
 *			  unikalny identyfikator drzwi, cos dziwnego,
 *			  cos co nie bedzie identyfikatorem zadnych
 *			  innych drzwi w okolicy. "drzwi" na pewno
 *			  _NIE_ jest najlepszym pomyslem..
 *
 * set_door_desc(s)	- Wywolaj te funkcje podajac dlugi opis drzwi.
 *
 * set_open(i)		- Podaj 1 gdy maja byc otwarte, zas 0 gdy zamkniete.
 *
 * set_open_desc(s)	- Wywolaj te funkcje z opisem otwartych drzwi.
 *
 * set_closed_desc(s)	- Wywolaj te funkcje z opisem zamknietych drzwi.
 *
** set_pass_command(m)	- Wywolaj te funkcje ze stringiem, lub tablica
 *			  stringow. Kazdy jeden, jest komenda powodujaca
 *			  przejscie przez drzwi.
 *
 * set_pass_mess(s)	- Uzyj tej funkcji do ustawienia komunikatu
 *			  pokazujacego sie, gdy gracz wychodzi przez
 *			  drzwi. Np "Alvin wychodzi _przez drzwi_",
 *			  gdzie _przez drzwi_ jest wartoscia ustawiona
 *			  przez set_pass_mess().
 *
 * set_fail_pass(s)	- Wywolaj te funkcje podajac komunikat, jaki 
 *			  zostanie wyswietlony graczowi probujacemu
 *			  przejsc przez zamkniete drzwi.
 *
 * set_open_command(m)	- Wywolaj te funkcje ze stringiem lub tablica
 *			  stringow. Kazdy jeden jest komenda sluzaca
 *			  do otwierania drzwi (ale nie kluczem,
 *			  normalnego otwierania!).
 *
-* set_open_mess(*s)	- Wywolaj te funkcje podajac tablice trzech
 *			  stringow. Pierwszy jest komunikatem 
 *			  jaki widza ludzie w pokoju, gdy jeden z nich
 *			  otwiera drzwi. Cos w formie emote'u, np.
 *			  'xxx otwiera drzwi'. Drugi jest komunikatem
 *			  wyslanym osobom po drugiej stronie drzwi.
 *			  Trzeci jest tekstem, jaki zobaczy
 *			  otwierajacy gracz.
 *
-* set_fail_open(m)	- Wywolaj te funkcje, jako argument podajac
 *			  komunikat, jaki sie pokaze osobie probujacej
 *			  otworzyc juz otwarte drzwi. Jesli twoje
 *			  drzwi maja zamek, powinienes podac tablice
 *			  dwoch stringow. Pierwszym jest juz w/w, zas
 *			  drugim komunikat pokazujacy sie osobie
 *			  ktora probuje otworzyc zamkniete na klucz drzwi.
 *
 * set_close_command(m)	- Wywolaj te funkcje ze stringiem lub tablica
 *			  stringow. Kazdy jeden jest komenda sluzaca
 *			  do zamykania drzwi (ale nie kluczem,
 *			  normalnego zamykania!).
 *
 * set_close_mess(*s)	- Wywolaj te funkcje podajac tablice dwoch
 *			  stringow. Pierwszy jest komunikatem
 *			  jaki widza ludzie w pokoju, gdy jeden z nich
 *			  zamyka drzwi. Cos w formie emote'u, np.
 *			  'xxx zamyka drzwi'. Drugi jest komunikatem
 *			  wyslanym osobom po drugiej stronie drzwi.
 *			  Trzeci jest tekstem, jaki zobaczy
 *			  zamykajacy gracz.
 *
 * set_fail_close(m)	- Wywolaj te funkcje, jako argument podajac
 *			  komunikat, jaki sie pokaze osobie probujacej
 *			  zamknac juz zamkniete drzwi.
 *
 * set_locked(i)	- Podaj 1, gdy maja byc zamkniete na klucz, albo
 *			  0 gdy zamek jest otwarty.
 * 
-* set_lock_name(m)	- Wywolaj te funkcje, podajac nazwe, lub tablice
 *			  nazw zamka (w przypadku gdyby gracz sobie chcial
 *			  go obejrzec). Powinienes to zdefiniowac, jesli
 *			  twoje drzwi maja zamek.
 *
 * set_lock_desc(s)	- Wywolaj te funkcje podajac zamka. (j.w.)
 *
 * set_lock_command(m)	- Wywolaj te funkje podajac komende, lub tablice
 *			  komend sluzacych do zamykania zamka w drzwiach.
 *
-* set_lock_mess(*s)	- Wywolaj te funkcje podajac tablice
 *			  trzech stringow. Pierwszy powinien byc komunikatem
 *			  ktory bedzie pokazywany ludziom w pokoju, gdy
 *			  jeden z nich zamyka zamek. Drugi jest komunikatem
 *			  wysylanym postaciom po drugiej stronie drzwi.
 *			  Trzeci jest tekstem, jaki zobaczy
 *			  gracz zamykajacy drzwi.
 *
-* set_fail_lock(*s)	- Wywolaj te funkcje podajac tablice dwoch
 *			  stringow. Pierwszy powinien byc komunikatem
 *			  wyswietlanym osobie, probujacej zamknac na klucz
 *			  juz zamkniete drzwi. Drugi string powinien
 *			  zawierac komunikat dla osoby probujacej
 *			  zamknac na klucz otwarte na osciez drzwi.
 *
-* set_fail_unlock(s)	- The message given if the door already was unlocked.
 *			  Must be defined if an unlock command is defined.
 *
 * set_unlock_command(m)- Wywolaj te funkje podajac komende, lub tablice
 *			  komend sluzacych do otwierania zamka w drzwiach.
 *
-* set_unlock_mess(*s)	- Wywolaj te funkcje podajac tablice
 *			  trzech stringow. Pierwszy powinien byc komunikatem
 *			  ktory bedzie pokazywany ludziom w pokoju, gdy
 *			  jeden z nich otwiera zamek. Drugi jest komunikatem
 *			  wysylanym postaciom po drugiej stronie drzwi.
 *			  Trzeci jest tekstem, jaki zobaczy
 *			  gracz odkluczajacy drzwi.
 *
-* set_fail_unlock(s)	- Wywolaj te funkcje podajac komunikat wyswietlany
 *			  osobie, probujacej otworzyc otwarty juz 
 *			  zamek w drzwiach.
 *
 * set_key(m)		- Wywolaj te funkcje, podajac unikalny
 *			  identyfikator klucza do tych drzwi.
 *			  (Identyczny musisz ustawic w kluczu)
 *
 * lock_function()	- Mozesz zastapic te funkcje wlasna, jesli
 *			  masz jakies wlasne pomysly zwiazane z 
 *			  otwieraniem zamkow (ukryte pulapki z
 *			  trucizna, etc)
 *
 * set_no_pick()	- Wywolaj te funkcje, jesli chcesz zeby
 *			  miala zamek, ale zeby sie dalo sie wlamac.
 * 
 * set_pick(i)		- Wywolaj te funkcje podajac liczbe, stanowiaca
 *			  stopien trudnosci wlamania sie przez
 *			  te drzwi (tzn otwarcia zamka bez klucza).
 *			  Jesli podana liczba bedzie > 100, to zamek
 *			  bedzie nie do otwarcia bez klucza.
 *
 * set_str(i)           - Ta funkcja podaje sie sile, niezbedna
 *			  do otwarcia/zamkniecia drzwi.
 *
 * Standardowe drzwi nie maja zamkow, waza 60 kilo, i maja objetosc
 * 80 litrow. Standardowa wysokosc drzwi wynosi 2 metry.
 *
 */
#pragma save_binary
#pragma strict_types

inherit "/std/object";
#include <stdproperties.h>
#include <macros.h>
#include <ss_types.h>
#include <cmdparse.h>
#include <language.h>

string	other_room,	/* The name of the other side of the door */
	door_id,	/* A unique (?) id of the door */
    	door_desc,	/* A description of the door */
	lock_desc,	/* A description of the lock */
    	open_desc,	/* The description of the open door */
	closed_desc,	/* The description of the closed door */
	pass_mess,	/* tekst przy wychodzeniu */
	fail_pass,	/* The fail pass message */
	fail_close,	/* The fail close message */
	fail_unlock;	/* The fail unlock message */

string	*open_mess,	/* The open messages */
	*fail_open,	/* The fail open messages */
	*close_mess,	/* The close messages */
	*lock_mess,	/* The lock messages */
	*fail_lock,	/* The fail lock messages */
	*unlock_mess;	/* The unlock messages */

mixed	
//	door_name,	/* The name(s) of the door */
	lock_name,	/* The name(s) of the lock */
    	key;		/* The key for opening locks */

object	klucz,		/* Obiekt klucza, do komunikatow */
	other_door;	/* The door at the other side */

string	*pass_commands,		/* The commands used to enter the door */
	*open_commands,		/* The commands used to open the door */
	*close_commands,	/* The commands used to close the door */
	*lock_commands,		/* The commands used to lock the door */
	*unlock_commands;	/* The commands used to unlock the door */

int	open_status,		/* If the door is open or not */
	lock_status,		/* If the door is locked or not */
	no_pick,		/* If the door is possible to pick */
	pick,			/* How hard is the lock to pick? */
	open_str;		/* Strength needed to open door */

/* 
 * Some prototypes 
 */
void create_door();
void set_lock_desc(string desc);
void set_lock_mess(string *mess);
void set_fail_lock(mixed mess);
void set_unlock_mess(string *mess);
void set_fail_unlock(string mess);
void set_other_room(string name);
void set_door_id(string id);
// void set_door_name(mixed name);
void set_door_desc(string desc);
void set_open_desc(string desc);
void set_closed_desc(string desc);
void set_pass_command(mixed command);
void set_pass_mess(string mess);
void set_fail_pass(string mess);
void set_open_command(mixed command);
void set_open_mess(string *mess);
void set_fail_open(mixed mess);
void set_close_command(mixed command);
void set_close_mess(string *mess);
void set_fail_close(string mess);
void load_other_door();
static void remove_door_info(object dest);
void do_open_door(string mess);
void do_close_door(string mess);
void do_lock_door(string mess);
void do_unlock_door(string mess);
int  lock_procedure(string arg);
void set_open(int i);
void set_locked(int i);
void do_set_key(mixed keyval);

/*
 * Function name: create_object
 * Description:   Initialize object.
 */
void
create_object()
{
    pass_commands = ({});
    open_commands = ({});
    close_commands = ({});
    pick = 100;
    add_prop(OBJ_I_WEIGHT, 60000);
    add_prop(OBJ_I_VOLUME, 80000);
    add_prop(DOOR_I_HEIGHT, 200); /* Standardowa wysokosc 2 metry. */
    add_prop(OBJ_I_NO_GET, 1);
    set_no_show();

    /*
     * Default messages.
     */
    set_open_desc("@@standard_open_desc");
    set_closed_desc("@@standard_closed_desc");

    set_fail_pass("@@standard_fail_pass");
    
    set_open_command("otworz");
    set_open_mess(({"@@standard_open_mess1", "@@standard_open_mess2",
        "@@standard_open_mess3" }));
    set_fail_open(({"@@standard_fail_open1", "@@standard_fail_open2" }));
    
    set_close_command("zamknij");
    set_close_mess(({"@@standard_close_mess1", "@@standard_close_mess2",
        "@@standard_close_mess3" }));
    set_fail_close("@@standard_fail_close");
    
    set_door_desc("Wyglada mocno.\n");
    set_lock_desc("Zwykly zamek.\n");
    
    set_lock_mess(({"@@standard_lock_mess1", "@@standard_lock_mess2", 
        "@@standard_lock_mess3" }));
    set_fail_lock(({"@@standard_fail_lock1", "@@standard_fail_lock2"}));
    
    set_unlock_mess(({"@@standard_unlock_mess1", "@@standard_unlock_mess2",
        "@@standard_unlock_mess3" }));
    set_fail_unlock("@@standard_fail_unlock");
    
    set_pass_mess("@@standard_pass_mess");
    set_open(1);
    set_locked(0);

    /* If you want to have a lock on the door, and be able to both lock it
       and unlock it you have to add these functions yourself. Don't do it
       here since all doors would have locks then... */

    /* You have to define the following yourself:
       (see docs in the beginning of this file)

             set_other_room()
             set_door_id()
             set_pass_command()
    */

    create_door();
}

/*
 * Function name: create_door
 * Description:   Sets default names and id
 */
void
create_door()
{
}

/*
 * Function name: reset_door
 * Description:   Reset the door
 */
void
reset_door() 
{
}

/*
 * Function name: reset_object
 * Description:   Reset the object
 */
nomask void
reset_object()
{
    reset_door();
}

int pass_door(string str);
int open_door(string str);
int close_door(string str);
int lock_door(string str);
int unlock_door(string str);
// int pick_lock(string str);

/*
 * Function name: init
 * Description:   Initalize the door actions
 */
void
init()
{
    int i;

    ::init();
    for (i = 0 ; i < sizeof(pass_commands) ; i++)
	add_action(pass_door, check_call(pass_commands[i]));

    for (i = 0 ; i < sizeof(open_commands) ; i++)
	add_action(open_door, check_call(open_commands[i]));

    for (i = 0 ; i < sizeof(close_commands) ; i++)
	add_action(close_door, check_call(close_commands[i]));
	
    for (i = 0 ; i < sizeof(lock_commands) ; i++)
	add_action(lock_door, check_call(lock_commands[i]));

    for (i = 0 ; i < sizeof(unlock_commands) ; i++)
	add_action(unlock_door, check_call(unlock_commands[i]));

/*
    if (!no_pick && sizeof(unlock_commands))
	add_action(pick_lock, "pick");
*/
}

/*
 * Function name: pass_door
 * Description:   Pass the door.
 * Arguments:	  arg - arguments given
 */
int
pass_door(string arg)
{
    int dexh;

    if (!other_door)
	load_other_door();

    /*
	The times higher a player can be and still get through
    */
    dexh = 2 + (this_player()->query_stat(SS_DEX) / 25);

    if (open_status)
    {
	/* Lets say we arbitrarily can bend as dexh indicates.
	   For something else, inherit and redefine.
	 */
	if ((int)this_player()->query_prop(CONT_I_HEIGHT) > 
			query_prop(DOOR_I_HEIGHT) * dexh) 
	{
	    write("Jestes zbyt duz" + 
	        this_player()->koncowka("y", "a", "y") + 
	        " i za malo zreczn" + 
	        this_player()->koncowka("y", "a", "y") +
	        ", zeby sie przecisnac przez " + short(PL_BIE));
	    return 1;
	}
	else if ((int)this_player()->query_prop(CONT_I_HEIGHT) > 
			query_prop(DOOR_I_HEIGHT))
	{
	    write("Z wielkim trudem przeciskasz sie przez " + short(PL_BIE) + 
	        ".\n");
	    saybb(QCIMIE(this_player(), PL_MIA) + " z wielkim trudem "
	        + "przeciska sie przez " + this_object()->short(PL_BIE)
	        + ".\n");
	}
	    
	this_player()->move_living(check_call(pass_mess), other_room);
    }
    else
	write(check_call(fail_pass));

    return 1;
}

/*
 * Function name: open_door
 * Description:   Open the door.
 * Arguments:	  arg - arguments given
 */
int
open_door(string arg)
{
    mixed drzwi_ob;
    
    notify_fail("Co chcesz otworzyc?\n");
    
    if (!stringp(arg))
        return 0;
            
    if (!parse_command(arg, environment(this_object()), "%i:3", drzwi_ob))
        return 0;

    drzwi_ob = CMDPARSE_STD->normal_access(drzwi_ob, 0, this_object(), 1);
    
    if (!drzwi_ob || !sizeof(drzwi_ob) || drzwi_ob[0] != this_object())
        return 0;

    if (!other_door)
	load_other_door();

    if (!open_status)
    {
        if (lock_status)
	    write(check_call(fail_open[1]));
	else if (this_player()->query_stat(SS_STR) < open_str)
//	    write("You lack the strength needed.\n");
            write(check_call(fail_open[1]));
	else
	{
	    write(check_call(open_mess[2]));
	    saybb(QCIMIE(this_player(), PL_MIA) + " "
	        + check_call(open_mess[0]));
	    do_open_door("");
	    other_door->do_open_door(check_call(open_mess[1]));
	}
    }
    else
	write(check_call(fail_open[0]));

    return 1;
}

void
do_open_door(string mess)
{
    object env;

    env = environment(this_object());
    env->change_my_desc(check_call(open_desc));
    if (strlen(mess))
	tell_roombb(env, mess);
    open_status = 1;
}

/*
 * Function name: close_door
 * Description:   Close the door.
 * Arguments:	  arg - arguments given
 */
int
close_door(string arg)
{
    mixed drzwi_ob;
    
    notify_fail("Co chcesz zamknac?\n");
    
    if (!stringp(arg))
        return 0;
            
    if (!parse_command(arg, environment(this_object()), "%i:3", drzwi_ob))
        return 0;

    drzwi_ob = CMDPARSE_STD->normal_access(drzwi_ob, 0, this_object(), 1);
    
    if (!drzwi_ob || !sizeof(drzwi_ob) || drzwi_ob[0] != this_object())
        return 0;

    if (!other_door)
	load_other_door();

    if (open_status)
    {
	if (this_player()->query_stat(SS_STR) < open_str)
	{
            write(capitalize(short(PL_MIA)) + " nie chc" +
                (query_tylko_mn() ? "a" : "e") + " sie ruszyc.\n");
	}
	else
	{
	    write(check_call(close_mess[2]));
	    saybb(QCIMIE(this_player(), PL_MIA) + " " +
		check_call(close_mess[0]));
	    do_close_door("");
	    other_door->do_close_door(check_call(close_mess[1]));
	}
    }
    else
    {
        notify_fail(check_call(fail_close));
        return 0;
    }

    return 1;
}

void
do_close_door(string mess)
{
    object env;

    env = environment(this_object());
    env->change_my_desc(check_call(closed_desc));
    if (strlen(mess))
	tell_roombb(env, mess);
    open_status = 0;
}

/*
 * Function name: lock_door
 * Description:   Lock the door.
 * Arguments:	  arg - arguments given
 */
int
lock_door(string arg)
{
    int ret;
    
    if (!other_door)
	load_other_door();

    if (!lock_status)
    {
	if (open_status)
	{
	    notify_fail(check_call(fail_lock[1]));
	    return 0;
	}
	else
	{
            if (!(ret = lock_procedure(arg)))
                return 0;
        	
            if (ret == 2)
                return 1; /* lock_procedure wyswietla wlasny blad */
                
	    write(check_call(lock_mess[2]));
	    saybb(QCIMIE(this_player(), PL_MIA) + " " +
		check_call(lock_mess[0]));
	    do_lock_door("");
	    other_door->do_lock_door(check_call(lock_mess[1]));
	}
    }
    else
    {
	notify_fail(check_call(fail_lock[0]));
	return 0;
    }

    return 1;
}

void
do_lock_door(string mess)
{
    if (strlen(mess))
	tell_roombb(environment(this_object()), mess);
    lock_status = 1;
}

/*
 * Function name: unlock_door
 * Description:   Unlock the door.
 * Arguments:	  arg - arguments given
 */
int
unlock_door(string arg)
{
    int ret;
    
    if (!other_door)
	load_other_door();

    if (lock_status)
    {
        if (!(ret = lock_procedure(arg)))
    	return 0;
    	
        if (ret == 2)
            return 1; /* lock_procedure wyswietla wlasny blad */
	    
	write(check_call(unlock_mess[2]));
	saybb(QCIMIE(this_player(), PL_MIA) + " "
	    + check_call(unlock_mess[0]));
	do_unlock_door("");
	other_door->do_unlock_door(check_call(unlock_mess[1]));
    }
    else
    {
	notify_fail(check_call(fail_unlock));
	return 0;
    }

    return 1;
}

void
do_unlock_door(string mess)
{
    if (strlen(mess))
	tell_roombb(environment(this_object()), mess);
    lock_status = 0;
}

int
sprawdz_klucz(object ob)
{
    return (environment(ob) == this_player());
}

/*
 * Nazwa funkcji: lock_procedure
 * Opis:          Funkcja jest wywolywana w celu sprawdzenia, czy
 *		  zamek moze byc otwarty/zamkniety.
 *
 *  Istnieja dwa warianty:
 *
 *  1: Nie ma klucza. Drzwi odblokowuje sie "<komenda> <drzwi>".
 *  2: Jest klucz. Otwiera sie przez "<komenda> <drzwi> <kluczem>".
 *
 * Argumenty:  arg - wszystko to, co zostalo napisane przez gracza po komendzie
 * Zwraca:       1 - Ok, 0 - Nie mozna, 2 - Nie mozna, ale lock_procedure
 *		   wyswietla wlasny komunikat bledu
 */
int
lock_procedure(string arg)
{
    int use_key;
    mixed drzwi_ob, klucz_ob;
    
    notify_fail(capitalize(query_verb()) + " co?\n");

    if (!strlen(arg))
        return 0;

    use_key = query_prop(DOOR_I_KEY);
    
    if (parse_command(arg, environment(this_object()), 
        "%i:" + PL_BIE + " %i:" + PL_NAR, drzwi_ob, klucz_ob))
    {
        notify_fail(capitalize(query_verb()) + " co? Albo moze czyms?\n");
        
        drzwi_ob = CMDPARSE_STD->normal_access(drzwi_ob, 0, this_object(), 1);
        if (!drzwi_ob || !sizeof(drzwi_ob) || drzwi_ob[0] != this_object())
            return 0;
            
        klucz_ob = CMDPARSE_STD->normal_access(klucz_ob, "sprawdz_klucz", 
            this_object());
            
        if (!klucz_ob || !sizeof(klucz_ob))
        {
            write(capitalize(query_verb()) + " " + 
                drzwi_ob[0]->short(PL_BIE) + " czym?\n");
            return 2;
        }

        if (!use_key)
        {
            write("W " + short(PL_MIE) + " nie widzisz dziurki od klucza.\n");
            return 2;
        }
        
        if ((mixed)klucz_ob[0]->query_key() == key)
        {
            klucz = klucz_ob[0];
            return 1;
        }
               
        write(capitalize(klucz_ob[0]->short(PL_MIA)) + " nie pasuje.\n");
        return 2;
    }
    
    if (use_key)
        return 0;

    if (!parse_command(arg, environment(this_object()), "%i:3", drzwi_ob))
        return 0;

    drzwi_ob = CMDPARSE_STD->normal_access(drzwi_ob, 0, this_object(), 1);

    if (!drzwi_ob || !sizeof(drzwi_ob) || drzwi_ob[0] != this_object())
    {
        notify_fail(capitalize(query_verb()) + " co?\n");
        return 0;
    }
    
    return 1;
}

#if 0
/*
 * Function name: do_pick_lock
 * Description:   Here we pick the lock, redefine this function if you want
 *		  something to happen, like a trap or something.
 * Arguments:	  skill - randomized picking skill of player
 *		  pick  - how difficult to pick the lock
 */
void
do_pick_lock(int skill, int pick)
{
    if (skill > pick)
    {
	write("Udalo sie! Od zamka dochodzi cie satysfakcjonujace " + 
	   "klikniecie.\n");
	say("Slyszysz ciche klikniecie dochodzace od zamka.\n");
	do_unlock_door("");
	other_door->do_unlock_door(check_call(unlock_mess[1]));
    } 
    else 
    if (skill < (pick - 50))
	write("You failed to pick the lock. It seems unpickable to you.\n");
    else
	write("You failed to pick the lock.\n");
}

/*
 * Function name: pick_lock
 * Description:   Pick the lock of the door.
 * Arguments:	  str - arguments given
 */
int
pick_lock(string str)
{
    int skill;
    string arg;

    notify_fail("Pick lock on what?\n");
    if (!str)
	return 0;
    if (sscanf(str, "lock on %s", arg) != 1)
	arg = str;
    notify_fail("No " + arg + " here.\n");
    if (member_array(arg, door_name) < 0)
	return 0;

    if (!lock_status)
    {
	write("Much to your surprise, you find it unlocked already.\n");
	return 1;
    }

    if (this_player()->query_mana() < 10)
    {
        write("You can't concetrate enough to pick the lock.\n");
	return 1;
    }

    this_player()->add_mana(-10); /* Cost 10 mana to try to pick a lock.*/
    write("You try to pick the lock.\n");
    say(QCTNAME(this_player()) + " tries to pick the lock.\n");
    skill = random(this_player()->query_skill(SS_OPEN_LOCK));

    if (!other_door)
	load_other_door();

    do_pick_lock(skill, pick);

    return 1;      
}
#endif 0

/*
 * Function name: set_str
 * Description:   Set the strength needed to open/close the door
 * Arguments:     str - strength needed
 */
void
set_str(int str) { open_str = str; }

/*
 * Function name: set_no_pick
 * Description:   Make sure the lock of the door is not pickable
 */
void
set_no_pick() { no_pick = 1; }

/*
 * Function name: query_no_pick
 * Description:	  Return 1 if door not pickable
 */
int
query_no_pick()	{ return no_pick; }

/*
 * Function name: set_pick
 * Description:   Set the difficulty to pick the lock, 100 impossible, 10 easy
 */
void
set_pick(int i)	{ pick = i; }

/*
 * Function name: query_pick
 * Description:	  Returns how easy it is to pick the lock on a door
 */
int
query_pick() { return pick; }

/*
 * Function name: set_open
 * Description:   Set the open staus of the door
 */
void
set_open(int i)	{ open_status = i; }

/*
 * Function name: query_open
 * Description:   Query the open status of the door.
 */
int
query_open() { return open_status; }

#if 0
/*
 * Function name: set_door_name
 * Description:	  Set the name of the door
 */
void
set_door_name(mixed name) 
{ 
  if (pointerp(name))
      door_name = name;
  else 
      door_name = ({ name });
}
#endif

/*
 * Function name: set_door_desc
 * Description:   Set the long description of the door
 */
void
set_door_desc(string desc) { door_desc = desc; }

/*
 * Function name: query_door_desc
 * Description:   Query the long description of the door
 */
string
query_door_desc() { return door_desc; }

/*
 * Function name: set_open_desc
 * Description:   Set the description of the door when open
 */
void
set_open_desc(string desc) { open_desc = desc; }

/*
 * Function name: query_open_desc
 * Description:   Query the open description of the door
 */
string
query_open_desc() { return open_desc; }

/*
 * Function name: set_closed_desc
 * Description:   Set the description of the door when closed
 */
void
set_closed_desc(string desc) { closed_desc = desc; }

/*
 * Function name: query_closed_desc
 * Description:   Query the description of the door when closed
 */
string
query_closed_desc() { return closed_desc; }

/*
 * Function name: set_pass_command
 * Description:   Set which command is needed to pass the door
 */
void
set_pass_command(mixed command)
{
    if (pointerp(command))
	pass_commands = command;
    else
	pass_commands = ({ command });
}

void
set_pass_mess(string mess)
{
    pass_mess = mess;
}

/*
 * Function name: query_pass_command
 * Description:   Query what command lets you pass the door
 */
string *
query_pass_command() { return pass_commands; }

/*
 * Function name: set_fail_pass
 * Description:   Set messaged when failing to pass the door.
 */
void
set_fail_pass(string mess) { fail_pass = mess; }

/*
 * Function name: query_fail_pass
 * Description:   Query message when failing to pass the door
 */
string
query_fail_pass()		{ return fail_pass; }

/*
 * Function name: set_open_command
 * Description:   Set command to open the door
 */
void
set_open_command(mixed command)
{
    if (pointerp(command))
	open_commands = command;
    else
	open_commands = ({ command });
}

/*
 * Function name: query_open_command
 * Description:   Query what command opens the door
 */
string *
query_open_command() { return open_commands; }

/*
 * Function name: set_open_mess
 * Description:   Set the message to appear when door opens
 */
void
set_open_mess(string *mess) { open_mess = mess; }

/*
 * Function name: query_open_mess
 * Description:   Query what messages we get when dor is opened
 */
string *query_open_mess() { return open_mess; }

/*
 * Function name: set_fail_open
 * Description:   Set the message when we fail to open door
 */
void	
set_fail_open(mixed mess)	
{ 
    if (pointerp(mess))
	fail_open = mess;
    else
	fail_open = ({ mess });
}

/*
 * Function name: query_fail_open
 * Description:   Query message when open fails
 */
string	*
query_fail_open() { return fail_open; }

/*
 * Function name: set_close_command
 * Description:   Set what command closes the door
 */
void
set_close_command(mixed command)
{
    if (pointerp(command))
	close_commands = command;
    else
	close_commands = ({ command });
}

/*
 * Function name: query_close_command
 * Description:   Query what command closes the door
 */
string *
query_close_command() { return close_commands; }

/*
 * Function name: set_close_mess
 * Description:   Set the message to appear when we close the door
 */
void
set_close_mess(string *mess) { close_mess = mess; }

/*
 * Function name: query_close_mess
 * Description:   Query message when we close the door
 */
string *query_close_mess()		{ return close_mess; }

/*
 * Function name: set_fail_close
 * Description:   Set message when we fail to close the door
 */
void
set_fail_close(string mess) { fail_close = mess; }

/*
 * Function name: query_fail_close
 * Description:   Query message when we fail to close the door
 */
string
query_fail_close() { return fail_close; }

/*
 * Function name: set_locked
 * Description:   Set lock status
 */
void
set_locked(int i) { lock_status = i; }

/*
 * Function name: query_locked
 * Description:   Query lock status
 */
int
query_locked() { return lock_status; }

/*
 * Function name: set_other_room
 * Description:   Set which rooms is on the other side
 */
void
set_other_room(string name) { other_room = name; }

/*
 * Function name: query_other_room
 * Description:   Query what room is on the other side
 */
string
query_other_room() { return other_room; }

/*
 * Function name: set_lock_name 
 * Description:   Set name of the lock
 */
void
set_lock_name(mixed name) { lock_name = name; }

/*
 * Function name: query_lock_name
 * Description:   Query the name of the lock
 */
mixed
query_lock_name() { return lock_name; }

/*
 * Function name: set_lock_desc
 * Description:   Set the description of the lock
 */
void
set_lock_desc(string desc) { lock_desc = desc; }

/*
 * Function name: query_lockdesc
 * Description:   Query the description of the lock
 */
string
query_lock_desc() { return lock_desc; }

/*
 * Function name: set_locl_command
 * Description:   Set which command locks the door
 */
void
set_lock_command(mixed command)
{
    if (pointerp(command))
	lock_commands = command;
    else
	lock_commands = ({ command });
}

/*
 * Function name: query_lock_command
 * Description:   Query what command locks the door
 */
string *
query_lock_command() { return lock_commands; }

/*
 * Function name: set_lock_mess
 * Description:   Set message when locking door
 */
void
set_lock_mess(string *mess) { lock_mess = mess; }

/*
 * Function name: query_lock_mess
 * Description:   Query the message when locking door
 */
string *
query_lock_mess() { return lock_mess; }

/*
 * Function name: set_fail_lock
 * Description:   Set message when fail to lock the door
 */
void	
set_fail_lock(mixed mess)	
{ 
    if (pointerp(mess))
	fail_lock = mess; 
    else
    	fail_lock = ({ mess });
}

/*
 * Function name: query_fail_lock
 * Description:   Query message when lock fails
 */
string	*
query_fail_lock() { return fail_lock; }

/*
 * Function name: set_unlock_command
 * Description:   Set what command unlocks the door
 */
void
set_unlock_command(mixed command)
{
    if (pointerp(command))
	unlock_commands = command;
    else
	unlock_commands = ({ command });
}

/*
 * Function name: query_unlock_command
 * Description:   Query what command unlocks the door
 */
string *
query_unlock_command() { return unlock_commands; }

/*
 * Function name: set_unlock_mess
 * Description:   Set message when unlocking door
 */
void
set_unlock_mess(string *mess) { unlock_mess = mess; }

/*
 * Function name: query_unlock_mess
 * Description:   Query message when unlocking door
 */
string *
query_unlock_mess() { return unlock_mess; }

/*
 * Function name: set_fail_unlock
 * Description:   Set fail message when unlocking
 */
void
set_fail_unlock(string mess) { fail_unlock = mess; }

/*
 * Function name: query_fail_unlock
 * Description:   Query message when failing to unlock door
 */
string
query_fail_unlock() { return fail_unlock; }

/*
 * Function name: set_key
 * Description:   Set the number of the key that fits
 */
void	
set_key(mixed keyval)		
{ 
    do_set_key(keyval);
/*
 * These lines cause trouble when you do set_key() in the create_door(),
 * because by that time none of the doors has an environment yet...
 * You'll just have to set the same key value in the other door manually

    if (!other_door)
	load_other_door();
    other_door->do_set_key(keyval);
 */
}

/*
 * Function name: do_set_key
 * Description:   Called from the other side
 */
void
do_set_key(mixed keyval)
{
    key = keyval;
    add_prop(DOOR_I_KEY, 1);
}

/*
 * Function name: query_key
 * Description:   Query what key that fits.
 */
mixed
query_key() { return key; }

/*
 * Nazwa funkcji : query_used_key
 * Opis          : Zwraca obiekt klucza, ktorym gracz sie wlasnie posluzyl
 *		   do otwarcia/zamkniecia drzwi na klucz. Stworzone
 *		   z mysla o komunikatach otwierania/zamykania.
 * Funkcja zwraca: Wskaznik obiektu klucza.
 */
object
query_used_key()
{
    return klucz;
}

/*
 * Function name: set_door_id
 * Description:   Set the id of the door
 */
void	
set_door_id(string id) 		
{ 
    id = "--" + id + "--";
    door_id = id; 
//    ustaw_shorty(({ id, id, id, id, id, id }) , PL_ZENSKI);
}

/*
 * Function name: query_door_id
 * Description:   Query the id of the door
 */
string
query_door_id()	{ return door_id; }

/*
 * Function name: query_other_door
 * Description:   Get the other door object pointer. The other
 *		  door will be loaded if neccesary. If that proovs
 *		  impossible, this door will autodestruct.
 */
object
query_other_door()
{
    if (!other_door)
	load_other_door();

    return other_door;
}

/*
 * Function name: load_other_door
 * Description:   Try to load the door in the other room. If this
 *		  fails, autodestruct.
 */
void
load_other_door()
{
    string *door_ids;
    object *doors;
    int pos;

    seteuid(getuid());

    /*
     * No other side or already loaded.
     */
    if (!strlen(other_room) || other_door)
	return;

    /*
     * Try to load the other side.
     */
    if (!find_object(other_room))
    {
	other_room->teleledningsanka();
	if (!find_object(other_room))
	{
	    write("Blad w ladowaniu drugiej strony drzwi: " + other_room + ".\n");
	    remove_door_info(environment(this_object()));
	    remove_object();
	    return;
	}
    }

    door_ids = (string *)other_room->query_prop(ROOM_AS_DOORID);
    doors = (object *)other_room->query_prop(ROOM_AO_DOOROB);
    pos = member_array(door_id, door_ids);
    if (pos < 0)
    {
	write("Po drugiej stronie, po zaladowaniu pokoju nie ma drzwi: " + 
		other_room + ".\n");
	remove_door_info(environment(this_object()));
	remove_object();
	return;
    }

    other_door = doors[pos];
}

/*
 * Function name: add_door_info
 * Description:   Add information about this door to the room it
 *		  stands in. If this door already exists, autodestruct.
 * Arguments:	  dest - The room that contains the door.
 */
static void
add_door_info(object dest)
{
    string *door_ids;
    object *doors;

    door_ids = (string *)dest->query_prop(ROOM_AS_DOORID);
    doors = (object *)dest->query_prop(ROOM_AO_DOOROB);
    if (!pointerp(door_ids))
    {
	door_ids = ({});
	doors = ({});
    }
    
    /* 
     * One door of the same type is enough.
     */
    if (member_array(door_id, door_ids) >= 0)
    {
	write("Jedne drzwi wystarcza.\n");
	remove_object();
	return;
    }

    door_ids += ({ door_id });
    doors += ({ this_object() });

    dest->add_prop(ROOM_AS_DOORID, door_ids);
    dest->add_prop(ROOM_AO_DOOROB, doors);
}

/*
 * Function name: remove_door_info
 * Description:   Remove information about this door from the room it
 *		  stands in.
 * Arguments:	  dest - The room that contains the door.
 */
static void
remove_door_info(object dest)
{
    string *door_ids;
    object *doors;
    int pos;

    door_ids = (string *)dest->query_prop(ROOM_AS_DOORID);
    doors = (object *)dest->query_prop(ROOM_AO_DOOROB);
    if (!sizeof(door_ids))
	return;

    pos = member_array(door_id, door_ids);
    if (pos < 0)
	return;

    door_ids = exclude_array(door_ids, pos, pos);
    doors = exclude_array(doors, pos, pos);

    dest->add_prop(ROOM_AS_DOORID, door_ids);
    dest->add_prop(ROOM_AO_DOOROB, doors);
}

/*
 * Function name: enter_env
 * Description:   The door enters a room, activate it.
 * Arguments:	  dest - The destination room,
 * 		  old - Where it came from
 */
void
enter_env(object dest, object old)
{
    string *itemy;
    int x;
    
    add_door_info(dest); 
    if (open_status)
        dest->change_my_desc(check_call(open_desc), this_object());
    else
        dest->change_my_desc(check_call(closed_desc), this_object());
    if (strlen(door_desc))
    {
        itemy = ({ query_nazwa(PL_BIE) });
        
        
        for (x = 0; x < sizeof(query_przym(1, PL_BIE)); x++)
        {
            itemy = itemy + ({ query_przym(1, PL_BIE)[x] + " " +
                query_nazwa(PL_BIE) });
        }
	dest->add_item(itemy, door_desc);
    }
    if (strlen(lock_desc) && lock_name) 
	dest->add_item(lock_name, lock_desc);
}

/*
 * Function name: leave_env
 * Description:   The door leaves a room, remove it.
 * Arguments:     old - Where it came from,
 * 		  dest - The destination room
 */
void
leave_env(object old, object dest)
{
    if (!old)
	return;

    old->remove_my_desc(this_object());
    if (strlen(door_desc))
	old->remove_item(query_nazwa(PL_BIE), door_desc);
    if (strlen(lock_desc) && lock_name) 
    {
	if (pointerp(lock_name))
	{
	    if (sizeof(lock_name))
		old->remove_item(lock_name[0], lock_desc);
	}
	else
	    old->remove_item(lock_name, lock_desc);
    }	
    remove_door_info(old);
}

/*
 * Function name: standard_open_desc
 */
string
standard_open_desc() 
{
  string temp_desc;
  
  temp_desc = "Widzisz otwart" + koncowka("ego", "a", "e", "ych", "e") + 
      " " + short(PL_BIE);
  if (strlen(pass_commands[0]) <= 2)
    temp_desc = temp_desc + " prowadzace na " + pass_commands[1] + ".\n";
  else
    temp_desc = temp_desc + ".\n";
  return temp_desc;
}


/*
 * Function name: standard_closed_desc
 */
string
standard_closed_desc()
{
  string temp_desc;
  temp_desc = "Widzisz zamkniet" + koncowka("ego", "a", "e", "ych", "e") + " "
            + short(PL_BIE);
  if (strlen(pass_commands[0]) <= 2)
   temp_desc = temp_desc + " prowadzace na " + pass_commands[1] + ".\n";
  else
    temp_desc = temp_desc + ".\n";
  return temp_desc;
}

/*
 * Function name: standard_open_mess1
 */
string
standard_open_mess1()
{
    return "otwiera " + short(PL_BIE) + ".\n";
}

/*
 * Function name: standard_open_mess2
 */
string
standard_open_mess2()
{
    return capitalize(short(PL_MIA)) + " otwiera" +
        (query_tylko_mn() ? "ja" : "") + " sie.\n";
}

/*
 * Function name: standard_open_mess3
 */
string
standard_open_mess3()
{
    return "Otwierasz " + short(this_player(), PL_BIE) + ".\n";
}

/*
 * Function name: standard_fail_open1
 */
string
standard_fail_open1()
{
    return capitalize(short(PL_MIA)) + 
        (query_tylko_mn() ? " sa" : " jest") + " juz otwar" +
        koncowka("ty", "ta", "te", "ci", "te") + ".\n";
}

/*
 * Function name: standard_fail_open2
 */
string
standard_fail_open2()
{
    return capitalize(short(PL_MIA)) + " nie chc" +
        (query_tylko_mn() ? "a" : "e") + " sie ruszyc.\n";
}

/*
 * Function name: standard_close_mess1
 */
string
standard_close_mess1()
{
    return "zamyka " + short(PL_BIE) + ".\n";
}

/*
 * Function name: standard_close_mess2
 */
string
standard_close_mess2()
{ 
    return capitalize(short(PL_MIA)) + " zamyka" +
        (query_tylko_mn() ? "ja" : "") + " sie.\n";
}

/*
 * Function name: standard_close_mess3
 */
string
standard_close_mess3()
{
    return "Zamykasz " + short(this_player(), PL_BIE) + ".\n";
}

/*
 * Function name: standard_lock_mess1
 */
string
standard_lock_mess1()
{
    return "zamyka " + short(PL_BIE) + " " + klucz->short(PL_NAR) + ".\n";
}

/*
 * Function name: standard_lock_mess2
 */
string
standard_lock_mess2()
{
    return "Slyszysz ciche klikniecie, dochodzace od " + short(PL_DOP) + ".\n";
}

/*
 * Function name: standard_lock_mess3
 */
string
standard_lock_mess3()
{
    return "Zamykasz " + short(this_player(), PL_BIE) + " " +
        klucz->short(PL_NAR) + ".\n";
}


/*
 * Function name: standard_fail_lock1
 */
string
standard_fail_lock1()
{
    return capitalize(short(PL_MIA)) + " juz " + 
        (query_tylko_mn ? "sa" : "jest") + " zamknie" +
        koncowka("ty", "ta", "te", "ci", "te") + ".\n";
}

/*
 * Function name: standard_fail_lock2
 */
string
standard_fail_lock2()
{
     return "Wpierw musisz zwyczajnie zamknac " + short(PL_BIE) + ".\n";
}

/*
 * Function name: standard_unlock_mess1
 */
string
standard_unlock_mess1()
{
     return "otwiera " + short(PL_BIE) + " " + klucz->short(PL_NAR) + ".\n";
}

/*
 * Function name: standard_unlock_mess2
 */
string
standard_unlock_mess2()
{
    return "Slyszysz ciche klikniecie, dochodzace od " + short(PL_DOP) + ".\n";
}

/*
 * Function name: standard_unlock_mess3
 */
string
standard_unlock_mess3()
{
    return "Otwierasz " + short(this_player(), PL_BIE) + " " + 
        klucz->short(PL_NAR) + ".\n";
}


/*
 * Function name: standard_door_desc
 */
string
standard_door_desc()
{
    return "Mocno wygladajac" + koncowka("y", "a", "e", "y", "e") + 
        short(PL_MIA) + ".\n";
}

/*
 * Function name: standard_fail_pass
 */
string
standard_fail_pass()
{
    return capitalize(short(PL_MIA)) + " " + 
        (query_tylko_mn() ? "sa" : "jest") + " zamknie" + 
        koncowka("ty", "ta", "te", "ci", "te") + ".\n";
}

/*
 * Function name: standard_fail_close
 */
string
standard_fail_close()
{
     return capitalize(short(PL_MIA)) + " juz " +
         (query_tylko_mn() ? "sa" : "jest") + " zamknie" +
         koncowka("ty", "ta", "te", "ci", "te") + ".\n";
}

/*
 * Function name: standard_fail_unlock
 */
string
standard_fail_unlock()
{
    return capitalize(short(PL_MIA)) + " juz " + 
        (query_tylko_mn() ? "sa" : "jest") + " otwar" +
        koncowka("ty", "ta", "te", "ci", "te") + ".\n";
}

string
standard_pass_mess()
{
    return query_verb();
}

/*
 * Function name: 
 * Description:   
 * Arguments:	  
 * Returns:       
 */
