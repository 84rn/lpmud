/*
 * /std/act/asking.c
 *
 * A simple support for helping answering questions
 * Idea from Padermud, buy coded by Nick
 */

#pragma save_binary
#pragma strict_types

#include <macros.h>
#include <stdproperties.h>

static	mixed	ask_arr;		/* The triggers and answers */
static	mixed	default_answer;		/* A default answer if you want. */
static	string	posed_question;		/* The exact question the player posed. */
static	string	not_here_func;	 	/* A function to call if player not here */
static	int	dont_answer_unseen = 1;	/* Flag if not to answer unseen */

/*
 * Nazwa funkcji : set_dont_answer_unseen
 * Opis          : Jesli znacznik jest wlaczony, NPC bedzie udawal Greka,
 *		   gdy zapyta sie go o cos ktos, kogo on nie widzi.
 * Argumenty     : int - znacznik.
 */
public void
set_dont_answer_unseen(int flag)
{
    dont_answer_unseen = flag;
}

/*
 * Nazwa funkcji : query_dont_answer_unseen
 * Opis          : Zwraca stan znacznika dont_answer_unseen
 * Funkcja zwraca: int - znacznik
 */
public int
query_dont_answer_unseen()
{
    return dont_answer_unseen;
}

/*
 * Nazwa funkcji : ask_id
 * Opis          : Sprawdza, czy npc reaguje na podane pytanie.
 * Argumenty     : string - badane pytanie.
 * Funkcja zwraca: int - prawda lub falsz.
 */
public int
ask_id(string str)
{
    int i;

    if (!ask_arr) return 0;

    for (i = 0; i < sizeof(ask_arr); i++)
        if (member_array(str, ask_arr[i][0]) >= 0) return 1;
}

/*
 * Nazwa funkcji : add_ask
 * Opis          : Dodaje pytanie do listy pytan, na ktore ten NPC reaguje.
 *		   Pierwszy argument powinien zawierac pojedynczy string
 *		   lub tablice stringow - pytania na ktore ma reagowac w
 *		   okreslony sposob. Drugi argument powinna stanowic
 *		   odpowiedz na podane pytania. add_ask() moze byc
 *		   wielokrotnie wywolywane w celu dodania nowych pytan
 *		   i nowych odpowiedzi.
 * Argumenty     : mixed pytania    - pytania, na ktore jest dana odpowiedz,
 *		   string odpowiedz - odpowiedz na pytanie,
 *		   int komenda	    - czy odpowiedz jest komenda.
 * Funkcja zwraca: Prawde lub falsz.
 */
public varargs int
add_ask(mixed pytania, string odpowiedz, int komenda)
{
    if (!pointerp(pytania))
	pytania = ({ pytania });
    if (ask_arr)
	ask_arr = ask_arr + ({ ({ pytania, odpowiedz, komenda }) });
    else
	ask_arr = ({ ({ pytania, odpowiedz, komenda }) });
	
    return 1;
}

/*
 * Nazwa funkcji : query_ask
 * Opis          : Zwraca tablice z zapytaniami na jakie NPC reaguje.
 * Funkcja zwraca: Ponizsza tablice z zapytaniami:

  [0] = array
     [0] ({ "nazwa1 zapytania1", "nazwa2 zapytania1",... })
     [1] "Oto odpowiedz na zapytanie1."
     [2] komenda (1 lub 0)
  [1] = array
     [0] ({ "nazwa1 zapytania2", "nazwa2 zapytania2",... })
     [1] "Oto odpowiedz na zapytanie2."
     [2] komenda (1 lub 0)
 */
public mixed
query_ask()
{
    return ask_arr;
}

/*
 * Nazwa funkcji : remove_ask
 * Opis          : Usuwa odpowiedz na podane pytanie.
 * Argumenty     : string question - pytanie, na ktore odpowiedz ma zostac
 *				     usunieta.
 * Funkcja zwraca: prawda - gdy pytanie i odpowiedz usuniete, 
 *		   falsz - gdy nie ma takiego pytania.
 */
public int
remove_ask(string question)
{
    int i;

    if (!pointerp(ask_arr))
        return 0;

    for (i = 0; i < sizeof(ask_arr); i++)
        if (member_array(question, ask_arr[i][0]) >= 0 )
        {
            ask_arr = exclude_array(ask_arr, i, i);
            return 1;
        }
    return 0;
}

/*
 * Nazwa funkcji : set_default_answer
 * Opis          : Ustawia standardowa odpowiedz na pytanie, ktorego NPC
 *		   nie jest w stanie zidentyfikowac. ZAUWAZ, ze npc wogole
 *		   nie zareaguje, gdy standardowa odpowiedz nie bedzie
 *		   ustawiona, a ktos zada nieobslugiwane pytanie.
 *		   W przypadku ustawienia VBFC jako reakcji na pytanie,
 *		   w podanej funkcji zwracaj "".
 * Argumenty     : mixed answer - standardowa odpowiedz (dopuszczalne VBFC).
 */
public void
set_default_answer(mixed answer)
{
    default_answer = answer;
}

/*
 * Nazwa funkcji : query_default_answer
 * Opis          : Zwraca standardowa odpowiedz
 * Funkcja zwraca: string - odpowiedz
 */
public string
query_default_answer()
{
    return default_answer;
}

/*
 * Nazwa funkcji : set_not_here_func
 * Opis          : Ustawia nazwe funkcji, ktora zostanie wywolana, gdy
 *		   gracz, ktory zadal pytanie opusci lokacje zanim
 *		   npc udzieli odpowiedzi.
 * Argumenty     : string - nazwa funkcji
 */
public void
set_not_here_func(string func)
{
    not_here_func = func;
}

/*
 * Nazwa funkcji : query_not_here_func
 * Opis          : Zwraca co jest ustawione jako not-here-function.
 * Funkcja zwraca: string - ustawiona funkcja
 */
public string
query_not_here_func() { return not_here_func; }

/*
 * Nazwa funkcji : query_question
 * Opis          : Zwraca prawdziwa postac pytania, jakie gracz nam zadal
 *		   (malymi literami, bez . i ? na koncu). Moze byc
 *		   wykorzystywane z VBFC np, by wykonac jakies testy
 *		   odnoszac sie do prawdziwej postaci pytania.
 * Funkcja zwraca: string - pytanie.
 */
public string
query_question()
{
    return posed_question;
}

/*
 * Nazwa funkcji : unseen_hook
 * Opis          : Funkcja ta zostaje wywolana za kazdym razem, gdy npc
 *		   nie moze dostrzec gracza, ktory zadal pytanie i ma
 *		   ustawiony znacznik, by nie odpowiadac w takich
 *		   sytuacjach. (patrz set_dont_answer_unseen()).
 * Argumenty     : object gracz - wkaznik na obiekt gracza, ktorego nie widzimy
 */
public void
unseen_hook(object gracz)
{
    switch (random(4))
    {
        case 0:
            command("rozejrzyj sie");
            break;
        case 1:
            command("podrap sie");
            break;
        case 2:
            command("wzrusz ramionami");
            break;
        case 3:
            command("powiedz Chyba sie przeslyszal"
                  + this_object()->koncowka("em", "am") + "...");
            break;
        default:
            throw("Unexpected value in switch.\n");
    }
}

/*
 * Nazwa funkcji : answer_question
 * Opis          : Funkcja ta zostaje wywolana po krotkim odstepie czasu
 *		   od zadania pytania, by npc mogl zareagowac na nie.
 * Argumenty     : mixed msg 	- reakcja na pytanie
 *		   int cmd	- czy powyzsza reakcja jest komenda
 *		   object gracz - wskaznik na obiekt gracza, ktory zadal
 *				  pytanie.
 */
void
answer_question(mixed msg, int cmd, object gracz)
{
    object env;

    if ((env = environment(this_object())) == environment(gracz) || 
	(env == gracz) || (not_here_func && call_other(this_object(), 
			 not_here_func, gracz)))
    {
	msg = this_object()->check_call(msg, gracz);

	if (cmd)
	    command(msg);
	else
	    gracz->catch_msg(msg);
    }
}

/*
 * Nazwa funkcji : catch_question
 * Opis          : Ta funkcja zostaje wywolana w NPCu za kazdym razem, gdy
 *		   ktos go o cos pyta.
 * Argumenty     : string question - zadane pytanie
 */
public void
catch_question(string question)
{
    int i;

    if (dont_answer_unseen && (!this_player()->check_seen(this_object()) ||
		!CAN_SEE_IN_ROOM(this_object())))
    {
	set_alarm(rnd() * 3.0 + 1.0, 0.0, &unseen_hook(this_player()));
	return;
    }

    i = strlen(question);
    if (question[i - 1] == "."[0] || question[i - 1] == "?"[0])
	question = extract(question, 0, i - 2);

    posed_question = lower_case(question);

    for (i = 0; i < sizeof(ask_arr); i++)
	if (member_array(posed_question, ask_arr[i][0]) >= 0)
	{
	    set_alarm(rnd() * 4.0, 0.0, &answer_question(ask_arr[i][1],
		ask_arr[i][2], this_player()));
	    return ;
	}

    if (default_answer)
	set_alarm(rnd() * 4.0, 0.0, &answer_question(default_answer, 0,
	    this_player()));
}
