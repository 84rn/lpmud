/*
 * leftover.c 
 *
 * Podstawowy kod leftover'ow
 *
 * Obiekt dziala na bardzo podobnej zasadzie, jak /std/food.c. Podstawowa
 * roznica, jest set_decay_time(), determinujacy jak dlugo obiekt
 * moze zostac w pomieszczeniu, zanim sie nie rozlozy - tak jak cialo.
 * Jesli obiekt zostanie umieszczony w jakims pojemniku, innym
 * niz obiekt pokoju (np obiekt gracza), to nie bedzie ulegal rozkladowi.
 * Rozklad bedzie przywrocony natychmiast jak znajdzie sie spowrotem
 * w jakims pomieszczeniu. Czas rozkladu jest mierzony w minutach.
 */

#pragma save_binary
#pragma strict_types

inherit "/std/food";

#include <cmdparse.h>
#include <composite.h>
#include <files.h>
#include <language.h>
#include <macros.h>
#include <stdproperties.h>

int decay_time;		/* Czas jaki uplynie zanim sie nie rozlozy */
int simple_names;	/* Setup of simple names or not */
static private int decay_alarm;
#if 0
string *l_porgan; /* Nazwa organu */
int l_r_rasy;		/* Rodzaj gramatyczny nazwy rasy */
#endif
string l_rasa, *l_organ;		/* Rasa poprzedniego wlasciciela organu */
int l_hard;		/* organ twardy czy mietki ;) */

public void decay_fun();

public void
create_leftover()
{
    simple_names = 1;
    dodaj_nazwy( ({ "szczatek", "szczatka", "szczatkowi", "szczatek",
        "szczatkiem", "szczatku" }), ({ "szczatki", "szczatkow", 
        "szczatkom", "szczatki", "szczatkami", "szczatkach" }),
        PL_MESKI_NOS_NZYW);
    set_amount(10);
}

public void
leftover_init(string *lp, string *lmn, int rodzaj, string *rasa,
	      int rodzaj_rasy = PL_MESKI_NOS_ZYW, int hard = 0)
{
    l_organ = lp;
#if 0
    l_porgan = lmn;
    l_r_rasy = rodzaj_rasy;
#endif
    l_rasa = rasa[PL_MIA];
    l_hard = hard;

    if (simple_names)
    {
	ustaw_nazwe(lp, lmn, rodzaj);
	set_long("Krwaw" + koncowka("y", "a", "e") + " " + lp[PL_MIA] + 
	   ", najprawdopodobniej jakie" + (rodzaj_rasy == PL_ZENSKI ? "js"
	   : "gos") + " " + rasa[PL_DOP] + ".\n");
    }
}

public nomask void
create_food() 
{ 
    simple_names = 0;
    decay_time = 10;
    create_leftover();
}

public void
set_decay_time(int time)
{
    decay_time = time;
}

public int
query_decay_time()
{
    return decay_time;
}

public string
query_rasa()
{
    return l_rasa;
}

#if 0
public varargs string
query_organ(int przyp)
{
    return l_organ[przyp];
}

public varargs string
query_porgan(int przyp)
{
    return l_porgan[przyp];
}
#endif

public int
query_hard()
{
   return l_hard;
}

public void 
enter_env(object dest, object old) 
{
    ::enter_env(dest, old);

    remove_alarm(decay_alarm);
    if (function_exists("create_container", dest) == ROOM_OBJECT)
	decay_alarm = set_alarm(1.0, 0.0, decay_fun);
}

public void
decay_fun()
{
    int num;
    
    if (--decay_time)
	decay_alarm = set_alarm(60.0, 0.0, decay_fun);
    else
    {
        if (query_rodzaj() == PL_NIJAKI_OS)
            num = 0;
        else
        {
            num = num_heap();
            if ((num < 10 || num > 20) && (num = num % 10) < 5 && num > 1)
                num = 1;
            else num = 0;
        }
        
	tell_roombb(environment(this_object()),
	            QCSHORT(this_object(), PL_MIA) + " obraca"
	          + (num ? "ja" : "") + " sie w proch.\n", ({}),
	            this_object());
	remove_object();
    }
}

/*
 * Function name:	consume_them 
 * Description:		Consumes a number of food item. This function
 *			shadows the ordinary /std/food function.
 *			All this to detect cannibals...
 * Arguments:		arr: Array of food objects.
 */
public void
consume_them(object *arr)
{
    int il;
    
    for (il = 0; il < sizeof(arr); il++)
    {
        if (arr[il]->query_rasa() == this_player()->query_rasa())
	{
	    write("Czyniac to, czujesz sie troche nieswojo, ale...\n");
	    this_player()->add_prop("cannibal", 1);
	}
	arr[il]->delay_destruct();
    }
}

int
eat_one_thing(object ob)
{
    if (ob->query_hard())
    {
        write("Chyba nie chcesz zjesc " + ob->short(PL_DOP) + "??\n");
        return 0;
    }
    return ::eat_one_thing(ob);
}

void
config_split(int new_num, mixed orig)
{
    ::config_split(new_num, orig);
    l_hard = orig->query_hard();
    l_rasa = orig->query_rasa();
}

