/*
 * /cmd/live/social.c
 *
 * General commands for 'nonemotive social' behaviour.
 * The following commands are defined:
 *
 * - aggressive		+ skasowane.
 * - ask		+ zapytaj.
 * - assist		+ wesprzyj.
 * - commune		+ przyzwij.
 * - emote		+ bez zmian.
 * - forget		+ zapomnij.
 * - introduce		+ przedstaw sie.
 * - introduced		+ przedstawieni.
 * - invite		+ zapros.
 * - join		+ dolacz do.
 * - kill		+ zabij.
 * - last		+ ostatnio.
 * - leave		+ porzuc.
 * - mwho		+ olac.
 * - present		+ wyrzucone. (odpowiednik introduce).
 * - reply		+ odpowiedz.
 * - remember		+ zapamietaj.
 * - remembered		+ zapamietani.
 * - stop		+ przestan.
 * - team		+ druzyna.
 * - who		+ kto.
 */

#pragma no_clone
#pragma no_inherit
#pragma save_binary
#pragma strict_types

inherit "/cmd/std/command_driver";

#include <cmdparse.h>
#include <composite.h>
#include <const.h>
#include <files.h>
#include <filter_funs.h>
#include <flags.h>
#include <formulas.h>
#include <language.h>
#include <macros.h>
#include <mail.h>
#include <std.h>
#include <stdproperties.h>
#include <time.h>
#include <options.h>

nomask int sort_name(object a, object b);

/*
 * Function name: create
 * Description  : This function is called the moment this object is created
 *                and loaded into memory.
 */
void
create()
{
    seteuid(getuid(this_object())); 
}

/* **************************************************************************
 * Return a proper name of the soul in order to get a nice printout.
 */
string
get_soul_id()
{
    return "social";
}

/* **************************************************************************
 * This is a command soul.
 */
int
query_cmd_soul()
{
    return 1;
}

/* **************************************************************************
 * The list of verbs and functions. Please add new in alfabetical order.
 */
mapping
query_cmdlist()
{
    return ([
	     "ask":"ask",
	     "assist":"assist",

	     "commune":"commune",
	     
	     "dolacz":"dolacz",
	     "druzyna":"druzyna",

	     "emote":"emote",

	     "forget":"forget",

	     "introduce":"intro_live",
	     "introduced":"introduced_list",
	     "invite":"invite",

	     "join":"join",

	     "kill":"kill",
	     "kto":"kto",

	     "last":"last",
	     "leave":"leave",
	     
	     "odpowiedz":"odpowiedz",
	     "ostatnio":"ostatnio",

	     "porzuc":"porzuc",
	     "przedstaw":"przedstaw",
	     "przedstawieni":"przedstawieni",
	     "przestan":"przestan",
	     "przyzwij":"przyzwij",

	     "reply":"reply",
	     "remember":"remember_live",
	     "remembered":"remember_live",

	     "stop":"stop",

	     "team":"team",

	     "wesprzyj":"wesprzyj",
	     "who":"who",
	     
	     "zabij":"zabij",
	     "zapamietaj":"zapamietaj",
	     "zapamietani":"zapamietani",
	     "zapomnij":"zapomnij",
	     "zapros":"zapros",
	     "zapytaj":"zapytaj"
	     ]);
}

/*
 * Function name: using_soul
 * Description:   Called once by the living object using this soul. Adds
 *		  sublocations responsible for extra descriptions of the
 *		  living object.
 */
public void 
using_soul(object live)
{
}

/* **************************************************************************
 * Here follows some support functions. 
 * **************************************************************************/

/* **************************************************************************
 * Here follows the actual functions. Please add new functions in the 
 * same order as in the function name list.
 * **************************************************************************/

/*
 * Ask - Ask someone something.
 */
int
ask(string str)
{
    notify_fail("Komenda 'ask' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'zapytaj'.\n");

    return 0;
}

/*
 * assist - Help a friend to kill someone else
 */
int 
assist(string str)
{
    notify_fail("Komenda 'assist' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'wesprzyj'.\n");

    return 0;
}

/*
 * Commune - talk with the wizards.
 *
 * This is supposed to be used in extreme emergencies only.
 */
int
commune(string str)
{
    notify_fail("Komenda 'commune' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'przyzwij'.\n");

    return 0;
}

/*
 * dolacz - Dolacza do czyjejs druzyny
 */
varargs int 
dolacz(string name)
{
    object leader, *oblist;
    int i;

    if (!name)
    {
	notify_fail("Musisz podac imie gracza, ktory ma przewodzic druzynie.\n");
	return 0;
    }

    if (this_player()->query_leader() || sizeof(this_player()->query_team()))
    {
	write("Juz jestes w druzynie!\n");
	return 1;
    }

    oblist = parse_this(name, "'do' %l:" + PL_DOP);

    if (!(i = sizeof(oblist)))
    {
	notify_fail("Dolacz do kogo?\n");
	return 0;
    }

    if (i > 1)
    {
	write("Nie mozesz byc prowadzon" + this_player()->koncowka("y", "a") +
	    " jednoczesnie przez " + COMPOSITE_LIVE(oblist, PL_BIE) + ".\n");
	return 1;
    }
    
    leader = oblist[0];

    if (member_array(this_player(), leader->query_invited()) < 0)
    {
	write(leader->query_Imie(this_player(), PL_MIA) + " nie zaprosil" +
	    leader->koncowka("", "a") + " cie do swej druzyny.\n");
	return 1;
    }

#if 0    
    if (leader->query_leader())
    {
	write(leader->query_Imie(this_player(), PL_MIA) + " jest juz w " +
	    "innej druzynie.\n");
	return 1;
    }
#endif

    /*
     * Can not have a leader with too low DIS
     */
    if (leader->query_stat(SS_DIS) + 20 < this_player()->query_stat(SS_DIS) &&
	!this_player()->query_wiz_level())
    {
	write("Nie bardzo ufasz w zdolnosci przywodcze " +
	    leader->query_imie(this_player(), PL_DOP) + ".\n");
 	return 1;
    }

    if (!this_player()->query_option(OPT_BRIEF))
    {
	write("W momencie wejscia do druzyny zaczynasz widziec krotkie " +
	     "opisy lokacji.\n");
	this_player()->add_prop(TEMP_BACKUP_BRIEF_OPTION, 1);
	this_player()->set_option(OPT_BRIEF, 1);
    }
    
    write("Dolaczasz do druzyny " + 
	leader->query_imie(this_player(), PL_DOP) + ".\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " dolacza do druzyny " + 
	QIMIE(leader, PL_DOP) + ".\n", ({ leader, this_player() }));
    tell_object(leader, this_player()->query_Imie(leader, PL_MIA) + 
	" dolacza do twojej druzyny.\n");
    
    return 1;
}

/*
 * druzyna - Podaje info o druzynie, ktorej gracz jest czlonkiem.
 */
varargs int
druzyna(string str)
{
    object leader;
    object *members;
    int i;

    if (stringp(str))
    {
	notify_fail("Po prostu 'druzyna'.\n");
	return 0;
    }

    if (leader = (object)this_player()->query_leader()) 
    {
	write("Druzyne prowadzi " + leader->query_imie(this_player(), PL_MIA));
	members = (object *) leader->query_team(); 
	members = members - ({ this_player() });
	if (!(i = sizeof(members)))
	     write(", zas ty jestes jej jedynym czlonkiem.\n");
	else
	if (i == 1)
	    write(" i oprocz ciebie jest w niej jeszcze " +
		members[0]->query_imie(this_player(), PL_MIA) + ".\n");
	else
	    write(" i oprocz ciebie sa w niej jeszcze: " +
		FO_COMPOSITE_LIVE(members, this_player(), PL_MIA) + ".\n");
    }
    else if (sizeof(members = (object *) this_player()->query_team()) > 0)
    {
	write("Przewodzisz druzynie, w ktorej oprocz ciebie jest jeszcze " + 
	    FO_COMPOSITE_LIVE(members, this_player(), PL_MIA) + ".\n");
    }
    else
    {
	notify_fail("Nie jestes w zadnej druzynie.\n");
	return 0;
    }
    
    return 1;
}


/*
 * emote - Put here so NPC:s can emote (  No error messages if they do wrong,
 *	   why waste cpu on NPC:s ? ;-)   )
 */
int
emote(string str)
{
    if (!stringp(str) ||
	!this_player()->query_npc())
    {
	return 0;
    }

    saybb(QCIMIE(this_player(), PL_MIA) + " " + str + "\n");

    return 1;
}

/*
 * forget - Forget someone we have remembered
 */
int
forget(string name)
{
    notify_fail("Komenda 'forget' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'zapomnij'.\n");

    return 0;
}

/*
 * introduce - Present yourself or someone else.
 */
int
intro_live(string str)
{
    notify_fail("Komenda 'introduce' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'przedstaw'.\n");

    return 0;
}

/*
 * introduced - Give a list of livings we have been introduced to.
 */
int
introduced_list(string str)
{
    notify_fail("Komenda 'introduced' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'przedstawieni'.\n");

    return 0;
}

/*
 * invite - Invite someone to join my team
 */
int
invite(string name)
{
    notify_fail("Komenda 'invite' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'zapros'.\n");

    return 0;
}

/*
 * join - Join someones team
 */
varargs int 
join(string name)
{
    notify_fail("Komenda 'join' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'dolacz'.\n");

    return 0;
}

/*
 * kill - Start attacking someone with the purpose to kill
 */
varargs int 
kill(string str)
{
    notify_fail("Komenda 'kill' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'zabij'.\n");

    return 0;
}

/*
 * kto - Pokazuje znane osoby ktore sa aktualnie w grze.
 */

/*
 * Function name: index_arg
 * Description  : This function returns whether a particular letter is
 *                used in the argument the player passed to the function.
 * Arguments    : string str    - the arguments.
 *                string letter - the letter to search for.
 * Returns      : int 1/0 - true if the letter is used.
 */
nomask int
index_arg(string str, string letter)
{
    return (member_array(letter, explode(str, "")) != -1);
}

/*
 * Function name: get_name
 * Description  : This map function will return the name of the player for
 *                the 'who n' command. If the player is invis it will
 *                return the name in brackets and if the living is linkdead,
 *                an asterisk (*) is added.
 * Arguments    : object player - the player to return the name for.
 * Returns      : string - the name to print.
 */
nomask string
get_name(object player)
{
    string name = capitalize(player->query_real_name());

    /* If the player is linkdead, we add an asterisk (*) to the name.
     */
    if (!interactive(player) &&
	!player->query_npc())
    {
	name += "*";
    }

    /* If the living is invis, we put the name between breakets. */
    if (player->query_prop(OBJ_I_INVIS))
    {
    	return ("(" + name + ")");
    }

    return name;
}

/*
 * Function name: print_who
 * Description  : This function actually prints the list of people known.
 * Arguments    : string opts  - the command line arguments.
 *                object *list - the list of livings to display.
 *                int    size  - the number of people logged in.
 * Returns      : int 1 - always.
 */
nomask int
print_who(string opts, object *list, int size)
{
    int i, j;
    int scrw = this_player()->query_option(OPT_SCREEN_WIDTH);
    string to_write = "";
    string *title;
    string tmp;

    scrw = (scrw ? (scrw - 3) : 77);
    list = sort_array(list, &sort_name());

    if (!sizeof(list))
    {
	if (size == 1)
	    to_write += "Przebywasz w swiecie Arkadii sam"
		      + this_player()->koncowka("", "a") + ".\n";
	else if (index_arg(opts, "c"))
	    to_write += "Nie znasz zadnego czarodzieja posrod "
		      + LANG_SNUM(size, PL_DOP, PL_ZENSKI)
		      + " osob przebywajacych obecnie w swiecie Arkadii.\n";
	else if (index_arg(opts, "s"))
	    to_write += "Nie znasz zadnego smiertelnika posrod "
		      + LANG_SNUM(size, PL_DOP, PL_ZENSKI)
		      + " osob przebywajacych obecnie w swiecie Arkadii.\n";
	/* No need to check for mwho here. */
	write(to_write);
	return 1;
    }

    if (size == 1)
	to_write += "Przebywasz w swiecie Arkadii sam"
		  + this_player()->koncowka("", "a") + ":\n";
    else if (index_arg(opts, "c"))
	to_write += "Sposrod " + LANG_SNUM(size, PL_DOP, PL_ZENSKI)
		  + " osob przebywajacych obecnie w swiecie Arkadii, "
		  + "znani tobie czarodzieje to:\n";
    else if (index_arg(opts, "s"))
	to_write += "Sposrod " + LANG_SNUM(size, PL_DOP, PL_ZENSKI)
		  + " osob przebywajacych obecnie w swiecie Arkadii, "
		  + "znani tobie smiertelnicy to:\n";
    else
	to_write += "Sposrod " + LANG_SNUM(size, PL_DOP, PL_ZENSKI)
		  + " osob przebywajacych obecnie w swiecie Arkadii, "
		  + "znane tobie to:\n";

    /* By default we display only the names, unless the argument 'f' for
     * full was given.
     */
    if (index_arg(opts, "k"))
    {
	to_write += (sprintf("%-*#s\n", scrw,
	    implode(map(list, get_name), "\n")));
	/* No need to check for mwho here. */
	write(to_write);
	return 1;
    }

    for(i = 0; i < sizeof(list); i++)
    {
	
	if (list[i]->query_prop(OBJ_I_INVIS))
	{
	    to_write += (interactive(list[i]) || list[i]->query_npc() ? "" :
		"*") + "(" + capitalize(list[i]->query_real_name(PL_MIA))
	      + ")\n";
	}
	else 
	{
	    tmp = list[i]->query_presentation();
	    
	    if (!interactive(list[i]) &&
		!list[i]->query_npc())
	    {
		tmp = ("*" + tmp);
	    }
	    
	    if (strlen(tmp) < scrw)
	    {
		to_write += (tmp + "\n");
	    }
	    else /* Split a too long title in a nice way. */
	    {
		title = explode(break_string(tmp, (scrw - 2)), "\n");
		tmp = sprintf("%-*s\n", scrw, title[0]);

		title = explode(break_string(
		    implode(title[1..(sizeof(title) - 1)], " "),
		    (scrw - 8)), "\n");

		for(j = 0; j < sizeof(title); j++)
		    tmp += (sprintf("      %-*s\n", (scrw - 6), title[j]));

		to_write += (tmp);
	    }
	}
    }

    setuid();
    seteuid(getuid());
    this_player()->more(to_write);

    return 1;
}

/*
 * Function name: sort_name
 * Description  : This sort function sorts on the name of the player. Since
 *                no two players can have the same name, we do not have to
 *                check for that.
 * Arguments    : object a - the playerobject to player a.
 *                object b - the playerobject to player b.
 * Returns      : int -1 - name of player a comes before that of player b.
 *                     1 - name of player b comes before that of player a. 
 */
nomask int
sort_name(object a, object b)
{
    string aname = a->query_real_name();
    string bname = b->query_real_name();

    return ((aname == bname) ? 0 : ((aname < bname) ? -1 : 1));
}

int
kto(string opts)
{
    object  *list = users();
    object  npc;
    mapping rem;
    mapping memory = ([ ]);
    string  *names = ({ });
    int     index;
    int     size;
    int     size_list;

    if (!stringp(opts))
    {
	opts = "";
    }

#ifdef STATUE_WHEN_LINKDEAD
#ifdef OWN_STATUE
    /* If there is a room where statues of linkdead people can be found,
     * we add that to the list.
     */
    list += (all_inventory(find_object(OWN_STATUE)) - list);
#endif OWN_STATUE
#endif STATUE_WHEN_LINKDEAD

    /* This filters out players logging in and such. */
    list = filter(list, &operator(==)(LIVING_OBJECT) @
	&function_exists("create_container"));
    size = sizeof(list);
    
    /* Player may indicate to see only wizards or mortals. */
    if (index_arg(opts, "c"))
    {
	list = filter(list, &->query_wiz_level());
    }
    else if (index_arg(opts, "s"))
    {
	list = filter(list, &not() @ &->query_wiz_level());
    }

    /* Wizards won't see the NPC's and wizards are not subject to the
     * met/nonmet system if that is active.
     */
    if (this_player()->query_wiz_level())
    {
	return print_who(opts, list, size);
    }

    if (mappingp(rem = this_player()->query_remembered()))
    {
	memory += rem;
    }
    if (mappingp(rem = this_player()->query_introduced()))
    {
	memory += rem;
    }
    
#ifdef MET_ACTIVE
    index = -1;
    size_list = sizeof(list);
    while(++index < size_list)
    {
	if ((!(memory[list[index]->query_real_name()])) &&
	    (!(list[index]->query_prop(LIVE_I_ALWAYSKNOWN))))
	{
	    list[index] = 0;
	}
    }

    list = list - ({ 0 });
#endif MET_ACTIVE

    /* Don't add NPC's if the player wanted wizards. Here we also add the
     * player himself again, because that is lost during the met-check (when
     * enabled).
     */
    if (!index_arg(opts, "c"))
    {
#ifdef MET_ACTIVE
	list += ({ this_player() });
//	size++;   /* this_player() juz dodany */
#endif MET_ACTIVE
	names = m_indices(memory) - list->query_real_name();

	index = -1;
	size_list = sizeof(names);
	while(++index < size_list)
	{
	    /* We check that the people found this way are NPC's since
	     * we do not want linkdead people to show up this way They
	     * are already in the list.
	     */
	    if (objectp(npc = find_living(names[index])) &&
		npc->query_npc())
	    {
		list += ({ npc });
		size++;
	    }
	}
    }

    /* To mortals 'who' will only display players who are visible to them. */
    list = filter(list,
	&operator(>=)(this_player()->query_prop(LIVE_I_SEE_INVIS)) @
	&->query_prop(OBJ_I_INVIS));

    return print_who(opts, list, size);
}


/*
 * last - display information on when a player was last logged in.
 */
int
last(string str)
{
    notify_fail("Komenda 'last' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'ostatnio'.\n");

    return 0;
}

/*
 * leave - Leave a team or force someone to leave a team
 */
int 
leave(string name)
{
    notify_fail("Komenda 'leave' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'porzuc'.\n");

    return 0;
}

/*
 * odpowiedz - Umozliwia graczom na odpowiedzenie czarodziejom, jesli ci
 *	im cos zatellowali.
 */
int
odpowiedz(string str)
{
    string *names, who;
    object target, *gracze;
    int i;

    /* Access failure. No command line argument. */
    if (!stringp(str))
    {
	notify_fail("Odpowiedz [komu] co?\n");
	return 0;
    }

    /* Wizard may block mortals from replying again. */
    if (this_player()->query_wiz_level() &&
	wildmatch("zablokuj *", str))
    {
	sscanf(lower_case(str), "zablokuj %s", str);

	gracze = users();
	i = sizeof(gracze) - 1;
	
	while ((gracze[i]->query_real_name(PL_CEL) != str) &&
	       (--i >= 0))
	    ;
	    
	if (i == -1)
	{
	    notify_fail("Nie ma takiego gracza.\n");
	    return 0;
	}
	
	target = gracze[i];

	names = target->query_prop(PLAYER_AS_REPLY_WIZARD);
	who = this_player()->query_real_name(PL_CEL);
	if (!pointerp(names) ||
	    (member_array(who, names) == -1))
	{
	    write(target->query_name(PL_MIA) + " i tak nie mogl" +
		target->koncowka("", "a") + " ci odpowiadac.\n");
	    return 1;
	}

	names -= ({ who });
	if (sizeof(names))
	{
	    target->add_prop(PLAYER_AS_REPLY_WIZARD, names);
	}
	else
	{
	    target->remove_prop(PLAYER_AS_REPLY_WIZARD);
	}

	write("Twoje imie zostalo usuniete z listy osob, ktorym " +
	    target->query_name(PL_MIA) + " moze odpowiadac.\n");
	return 1;
    }

    /* See if any wizard has told anything to this player. */
    names = this_player()->query_prop(PLAYER_AS_REPLY_WIZARD);
    if (!pointerp(names) ||
	!sizeof(names))
    {
	notify_fail("Mozesz odpowiedziec tylko wtedy, gdy ktos ci cos " +
	    "powiedzial.\n");
	return 0;
    }

    /* If the mortal wants to reply to someone in particular, see get the
     * name and see whether that is possible, else we take the first wizard
     * from the list.
     */
    
    if (sscanf(str, "%s %s", who, str) == 2)
    {
	if (member_array(lower_case(who), names) == -1)
	{
	    str = who + " " + str;
	    who = names[0];
	}
    }
    else 
       who = names[0];
	
    gracze = users();
    i = sizeof(gracze) -1;
    
    while ((gracze[i]->query_real_name(PL_CEL) != who) &&
	   (--i >= 0))
	    ;

    /* Wizard is no longer logged in. */
    if (i == -1)
    {
	write("Nie mozesz odpowiedziec, gdyz ta osoba nie jest juz obecna.\n");
	return 1;
    }
    
    target = gracze[i];

    /* No point in replying to someone who is linkdead. */
    if (!interactive(target))
    {
	write(target->query_Imie(this_player(), PL_MIA) +
	    " stracil" + target->koncowka("", "a") + " kontakt z " +
	    "rzeczywistoscia.\n");
	return 1;
    }

    tell_object(target, this_player()->query_Imie(target, PL_MIA) + 
	" odpowiada: " + str + "\n");
    if (this_player()->query_option(OPT_ECHO))
    {
	write("Odpowiadasz " + target->query_imie(this_player(), PL_CEL) +
	    ": " + str + "\n");
    }
    else
    {
	write("Odpowiedzial" + this_player()->koncowka("es", "as") + 
	    " " + target->query_imie(this_player(), PL_CEL) + ".\n");
    }
    return 1;
}

/*
 * ostatnio - Wyswietla informacje o graczu
 */
int
ostatnio(string str)
{
    object player;
    int duration;
    string konc;
    int npc;

    if (!stringp(str))
	str = this_player()->query_real_name();
    else
    {
	str = lower_case(str);
	if (!(this_player()->query_met(str)))
	{
	    notify_fail("Nie znasz nikogo o tym imieniu.\n");
	    return 0;
	}
    }

    if (SECURITY->query_wiz_rank(str))
    {
	notify_fail("Ta komenda nie dziala na czarodzieji.\n");
	return 0;
    }

    if (!(SECURITY->exist_player(str)))
    {
	npc = 1;
    }

/*
    if (objectp(player = find_living(str)) &&
	player->query_npc())
*/
    if (npc)
    {
	player = find_living(str);
	if (objectp(player))
	    konc = player->koncowka("y", "a", "e");
	else
	    konc = "y";

	write("Ostatnie logowanie : " + ctime(SECURITY->query_start_time() +
	    (random(time() - SECURITY->query_start_time()) / 2)) + "\n");
	write("Aktywnosc          : aktywn" + konc + "\n");
	return 1;
    }

    if (objectp(player = find_player(str)))
    {
	write("Ostatnie logowanie : " + ctime(player->query_login_time()) +
	     "\n");

	if (interactive(player) &&
	    CAN_SEE(this_player(), player))
	{
	    if (query_idle(player) > 60)
	    {
		write("Aktywnosc          : " + 
		    TIME2STR(query_idle(player), 2) + " nieaktywn" + 
		    player->koncowka("y", "a") + "\n");
	    }
	    else
	    {
		write("Aktywnosc          : aktywn" + 
		    player->koncowka("y", "a") +
		    "\n");
	    }
	}
	else
	{
	    write("Aktywnosc          : stracil" + 
		this_player()->koncowka("", "a") + " kontakt z " +
		"rzeczywistoscia.\n");
	}

	return 1;
    }

    player = SECURITY->finger_player(str);
    write("Ostatnie logowanie : " + ctime(player->query_login_time()) + ",\n");
    duration = (file_time(PLAYER_FILE(str) + ".o") -
	player->query_login_time());
    if (duration < 86400)
    {
	write("Spedzony czas      : " + TIME2STR(duration, 3) + "\n");
    }
    else
    {
	write("Spedzony czas: nieznany\n");
    }
    player->remove_object();
    return 1;
}

/*
 * porzuc - Porzuc druzyne lub kogos, kto jest w druzynie.
 */
int 
porzuc(string name)
{
    object ob, *members;
    mixed oblist;
    int i;

    if (!stringp(name))
    {
	notify_fail("Porzuc kogo? Moze druzyne?\n");
	return 0;
    }
    
    if (name == "druzyne")
    {
	if (objectp(ob = this_player()->query_leader()))
	{
	    ob->team_leave(this_player());
	    write("Porzucasz swoja druzyne.\n");
	    tell_object(ob, this_player()->query_Imie(ob, PL_MIA) +
		" porzuca twoja druzyne.\n");
		
	    if (this_player()->query_prop(TEMP_BACKUP_BRIEF_OPTION))
	    {
		write("Od momentu wyjscia z druzyny spowrotem widzisz " +
		    "dlugie opisy lokacji.\n");
		this_player()->remove_prop(TEMP_BACKUP_BRIEF_OPTION);
		this_player()->set_option(OPT_BRIEF, 0);
	    }

	    return 1;
	}

	if (i = sizeof(members = this_player()->query_team()))
	{
	    write("Porzucasz druzyne, ktorej przewodzil" +
		this_player()->koncowka("es", "as") + ".\n");
	    while(--i >= 0)
	    {
		tell_object(members[i], 
		    capitalize(this_player()->query_Imie(members[i], PL_MIA)) +
		    " rozwiazuje druzyne.\n");
		this_player()->team_leave(members[i]);
		if (this_player()->query_prop(TEMP_BACKUP_BRIEF_OPTION))
		{
		    write("Od momentu wyjscia z druzyny spowrotem widzisz " +
			"dlugie opisy lokacji.\n");
		    this_player()->remove_prop(TEMP_BACKUP_BRIEF_OPTION);
		    this_player()->set_option(OPT_BRIEF, 0);
		}
	    }
	    return 1;
	}

	notify_fail("Nie jestes czlonkiem zadnej druzyny.\n");
	return 0;
    }

    if (!(i = sizeof(members = this_player()->query_team())))
    {
	notify_fail("Te druzyne prowadzi kto inny... Mozesz co najwyzej " +
	    this_player()->koncowka("samemu", "sama") + " porzucic druzyne.\n");
	return 0;
    }

    notify_fail("Porzuc kogo?\n");
    if (!parse_command(name, members, "%l:" + PL_BIE, oblist))
	return 0;

//    oblist = NORMAL_ACCESS(oblist, 0, 0);
    i = sizeof(oblist);
    if (i < 2)
	return 0;

    if (i > 2)
    {
	write("Nie mozesz porzucic wiecej niz jedna osobe na raz.\n");
	return 1;
    }

    this_player()->set_obiekty_zaimkow(oblist[1..1]);
    ob = oblist[1];

    this_player()->team_leave(ob);
    this_player()->remove_invited(ob); /* disallow him/her to rejoin. */
    write("Zmuszasz " + ob->query_imie(this_player(), PL_BIE) +
	" do opuszczenia druzyny.\n");
    tell_object(ob, this_player()->query_Imie(ob, PL_MIA) +
	" zmusza cie do opuszczenia druzyny.\n");
    if (this_player()->query_prop(TEMP_BACKUP_BRIEF_OPTION))
    {
	write("Od momentu wyjscia z druzyny spowrotem widzisz " +
	    "dlugie opisy lokacji.\n");
	this_player()->remove_prop(TEMP_BACKUP_BRIEF_OPTION);
	this_player()->set_option(OPT_BRIEF, 0);
    }

    return 1;
}

/*
 * przedstaw - Przedstaw sie albo kogos innego.
 */
int
przedstaw(string str)
{
    string  kogo;
    string  komu = "";
    string  mianownik, dopelniacz;
    object  przedstawiany;
    object *przedstawiani;
    object *odbiorcy;
    object *osoby;
    object *inni;
    int     wszyscy = 0;
    int     sam_sie = 0;
    int     i;
    int     size;

    notify_fail(capitalize(query_verb()) + " kogo [komu]?\n");
    if (!stringp(str))
    {
	return 0;
    }
    
    kogo = explode(str, " ")[0];
    
    if (kogo == "sie" || kogo == "siebie")
    {
	przedstawiany = this_player();
	sscanf(str, kogo + " %s", komu);
    }
    else
    {
	if (parse_command(str, environment(this_player()), 
		 "%l:" + PL_BIE + " %s", przedstawiani, komu))
	    przedstawiani = NORMAL_ACCESS(przedstawiani, 0, 0);
	    
	if (!sizeof(przedstawiani))
	{
	    notify_fail("Kogo chcesz przedstawic?\n");
	    return 0;
	}
	
	if (sizeof(przedstawiani) > 1)
	{
	    write("Mozesz przedstawic naraz tylko jedna osobe.\n");
	    return 1;
	}
	
	przedstawiany = przedstawiani[0];
    }
    
    
    sam_sie = ((this_player() == przedstawiany) ? 1 : 0);
    
    if (!sam_sie && (!this_player()->query_met(przedstawiany)))
    {
	write("Ha! Najpierw sam" + this_player()->koncowka("", "a") +
	    " " + przedstawiany->koncowka("go", "ja", "to", "ich", "je") + 
	    " poznaj.\n");
	return 1;
    }


    if (strlen(komu))
    {
	odbiorcy = parse_this(komu, "%l:" + PL_CEL) - ({ przedstawiany });
    }
    else
    {
	odbiorcy = FILTER_LIVE(all_inventory(environment(this_player()))) -
	     ({ this_player(), przedstawiany });
	     
	wszyscy = 1;
    }
    
    osoby = FILTER_CAN_SEE(odbiorcy, this_player());
    
    if (!wszyscy)
    {
	if (!sizeof(osoby))
	{
	    write("Komu chcesz sie przedstawic?\n");
	    return 1;
	}
	i = -1;
	size = sizeof(osoby);
	
	while(++i < size)
	    osoby[i]->reveal_me(1);
    }
    
    this_player()->reveal_me(1); przedstawiany->reveal_me(1);
    
    str = przedstawiany->query_presentation();
    size = sizeof(odbiorcy);

    if (!sam_sie)
    {
	if (wszyscy)
	    this_player()->set_obiekty_zaimkow(przedstawiani);
	else
	    this_player()->set_obiekty_zaimkow(przedstawiani, odbiorcy);

	write("Przedstawiasz " + przedstawiany->query_imie(this_player(), 
	    PL_BIE) + " " + (wszyscy ? "wszystkim" : 
	    FO_COMPOSITE_LIVE(odbiorcy, this_player(), PL_CEL)) + ".\n");

	przedstawiany->catch_msg(this_player()->query_Imie(przedstawiany, 
	    PL_MIA) + " przedstawia cie " + (wszyscy ? "wszystkim" 
	    : FO_COMPOSITE_LIVE(FILTER_CAN_SEE(odbiorcy, przedstawiany), 
	    przedstawiany, PL_CEL)) + ".\n");
	    
	i = -1;
	while(++i < size)
	{
	    tell_object(odbiorcy[i],
    	        this_player()->query_Imie(odbiorcy[i], PL_MIA) + 
    	        " przedstawia " + przedstawiany->query_imie(odbiorcy[i], 
    	        PL_BIE) + " slowami:\nOto " + str + ".\n");
	}

	inni = FILTER_LIVE(all_inventory(environment(this_player()))) -
    	     ({ this_player(), przedstawiany }) - odbiorcy;

	size = sizeof(inni);
	i = -1;
	while(++i < size)
	{
	    tell_object(inni[i], this_player()->query_Imie(inni[i], PL_MIA) +
		" przedstawia " + przedstawiany->query_imie(inni[i], PL_BIE) +
		" " + FO_COMPOSITE_LIVE(FILTER_CAN_SEE(odbiorcy, inni[i]), 
		inni[i], PL_CEL) + ".\n");
	}
    }
    else
    {
	i = -1;
	while(++i < size)
	{
	    tell_object(odbiorcy[i],
    	        this_player()->query_Imie(odbiorcy[i], PL_MIA) + 
    	        " przedstawia sie jako:\n" + str + ".\n");
	}

	if (!wszyscy)
	    this_player()->set_obiekty_zaimkow(odbiorcy);
	
	if (this_player()->query_option(OPT_ECHO))
	    write("Przedstawiasz sie " + (wszyscy ? "wszystkim" 
		: FO_COMPOSITE_LIVE(osoby, przedstawiany, PL_CEL)) + ".\n");
	else
	    write("Ok.\n");


	inni = FILTER_LIVE(all_inventory(environment(this_player()))) -
    	     ({ this_player() }) - odbiorcy;

	size = sizeof(inni);
	i = -1;
	while(++i < size)
	{
	    tell_object(inni[i], this_player()->query_Imie(inni[i], PL_MIA) +
		" przedstawia sie " + 
		FO_COMPOSITE_LIVE(FILTER_CAN_SEE(odbiorcy, inni[i]), 
		inni[i], PL_CEL) + ".\n");
	}
    }
    
    if (!sizeof(osoby))
	write("Nie widzisz w tym co prawda najmniejszego sensu, gdyz " +
	    "nie ma nikogo, kto moglby cie uslyszec.\n");

    size = sizeof(odbiorcy);
    i = -1;
    
    mianownik = przedstawiany->query_real_name(PL_MIA);
    dopelniacz = przedstawiany->query_real_name(PL_BIE);

    while (++i < size)
	odbiorcy[i]->add_introduced(mianownik, dopelniacz);
	    
    return 1;
}


/*
 * przedstawieni - Zwraca liste osob, ktore sie nam przedstawily.
 */
int
przedstawieni(string str)
{
    object ob;
    mapping tmp;
    
    tmp = this_player()->query_introduced();
    if (m_sizeof(tmp))
    {
	write("Przypominasz sobie " + 
	    COMPOSITE_WORDS(map(sort_array(m_values(tmp)), capitalize)) + 
	    ".\n");

	return 1;
    }
    else
    {
	write("Nie przypominasz sobie nikogo, kto by ci sie ostatnio "+
	    "przedstawil, a kogo bys nie pamietal" + 
	    this_player()->koncowka("", "a") + ".\n");
	return 1;
    }
}

/*
 * przestan - Przestan walczyc
 */
int
przestan(string str)
{
    string a,b;
    object *e, *pe;
    int walcz_z, i;
    
    notify_fail("Czego chcesz zaprzestac?\n");
    
    if (!stringp(str))
	return 0;

    pe = this_player()->query_enemy(-1);

    if (!sscanf(str, "%s %s", a, b))
       a = str;
    
    if (a != "walczyc" && a != "atakowac" && a != "gonic")
	return 0;
	
    if (!pe ||
	(sizeof(pe) == 0))
    {
	notify_fail("Nie atakujesz nikogo.\n");
	return 0;
    }
	
    if (!strlen(b))
	e = ({ this_player()->query_enemy() });
    else
    {
	if (a == "walczyc")
	{
	    if (strlen(b) < 5 || b[0..1] != "z ")
	    {
		notify_fail("Przestan walczyc Z kim?\n");
		return 0;
	    }
		
	    b = b[2..];
	    
	    walcz_z = 1;
	}
	else walcz_z = 0;
	
	if (!parse_command(b, pe, "%l:" + (walcz_z ? PL_NAR : PL_BIE), e))
	{
	    notify_fail("Nie atakujesz nikogo takiego.\n");
	    return 0;
	}

	e = NORMAL_ACCESS(e, 0, 0);
	this_player()->set_obiekty_zaimkow(e);
    }
    
    i = sizeof(e);
    
    while (--i >= 0)
    {
	write("Decydujesz sie nie atakowac " + 
	    e[i]->query_imie(this_player(), PL_DOP) + ".\n");
	this_player()->stop_fight(e[i]);
    }
	
    return 1;
}

/*
 * przyzwij - Komenda dla graczy do komunikacji z czarodziejami.
 *
 * Powinna byc uzywana tylko w ekstremalnych przypadkach.
 */
int
przyzwij(string str)
{
    object *us;
    object spec;
    int il;
    int flag = 0;
    int size;
    string *arg;
    string *alias;
    string wiz;
    string mess;

    if (!stringp(query_ip_number(this_player())))
    {
	notify_fail("Tylko prawdziwi gracze moga przyzywac bogow.\n");
	return 0;
    }

    if (this_player() != this_interactive())
    {
	tell_object(this_interactive(),
	    "Przyzywanie to czynnosc jaka gracze musza wykonac " +
		"samodzielnie.\n");
	notify_fail("Niedopuszczone wezwanie, jako ze musisz " +
	    "wykonac je samodzielnie.\n");
	return 0;
    }

    if (this_player()->query_wiz_level())
    {
	notify_fail("Wzywanie jest czyms zarezerwowanym dla graczy, ty "+
	    "zas byc moze chcesz skorzystac z komendy 'audience'.\n");
	return 0;
    }

    if (!stringp(str))
    {
	write("Sprawdz pomoc do komendy przyzwij ('?przyzwij'), zeby sie " +
	    "dowiedziec, jak dziala ta komenda. Ale uwazaj! Smiertelnicy " +
	    "sa narazeni na potworny gniew bostw w przypadku " +
	    "nieuzasadnionego uzycia.\n");
	return 1;
    }

    arg = explode(str, " ");
    if (sizeof(arg) < 2)
    {
	notify_fail("Sprawdz pomoc do komendy przyzwij ('?przyzwij'), " +
	    "zeby sie dowiedziec, jak dziala ta komenda.\n");
	return 0;
    }
    
    wiz = lower_case(arg[0]);
    str = capitalize(this_interactive()->query_real_name());
    mess = implode(arg[1..], " ") + "\n";

    switch(wiz)
    {
    case "wszystkich":
	us = filter(users(), &->query_wiz_level());
	il = -1;
	size = sizeof(us);

	while(++il < size)
	{
	    if (query_ip_number(us[il]) &&
		!(us[il]->query_prop(WIZARD_I_BUSY_LEVEL) & BUSY_C))
	    {
		tell_object(us[il],
		    "PRZYZWANIE kogokolwiek od '" + str + "': " + mess);
		flag = 1;
	    }
	}
	break;

    case "miejscowych":
	if (!environment(this_player()))
	{
	    notify_fail("Jestes w Pustce - tu nie ma Bostw.\n");
	    return 0;
	}

	wiz = explode(file_name(environment(this_player())), "/")[2];
	if (!sizeof(arg = (string *)SECURITY->query_domain_members(wiz)))
	{
	    notify_fail("Przykro mi, nie mozesz przyzywac miejscowych "+
		"czarodzieji z tej lokacji.\n");
	    return 0;
	}

	il = -1;
	size = sizeof(arg);

	while(++il < size)
	{
	    spec = find_player(arg[il]);

	    if (objectp(spec) &&
		query_ip_number(spec) &&
		!(spec->query_prop(WIZARD_I_BUSY_LEVEL) & BUSY_C))
	    {
		tell_object(spec,
		    "PRZYZWANIE " + wiz + " od '" + str + "': " + mess);
		flag = 1;
	    }
	}
	break;

    default:

        setuid();
        seteuid(getuid());

        if ((IS_MAIL_ALIAS(wiz) &&
            sizeof(alias = EXPAND_MAIL_ALIAS(wiz))) ||
            ((SECURITY->query_domain_number(capitalize(wiz)) > -1) &&
            sizeof(alias = SECURITY->query_domain_members(capitalize(wiz)))))
        {
            wiz = capitalize(wiz);

            for (il = 0; il < sizeof(alias); il++)
            {
                if (!(spec = find_player(alias[il])))
                {
                    continue;
                }

                if (!spec->query_wiz_level() ||
                    !query_ip_number(spec) ||
                    (spec->query_prop(WIZARD_I_BUSY_LEVEL) & BUSY_C))
                {
                    continue;
                }

		tell_object(spec, "PRZYZWANIE " + wiz + " od '" + str +
		    "': " + mess);

                if (!spec->query_invis())
                {
                    flag = 1;
                }
            }

            break;
        }


	us = users();
	il = sizeof(users()) - 1;
	while ((us[il]->query_real_name(PL_BIE) != wiz) &&
	       (--il >= 0))
	    ;
	    
	wiz = capitalize(wiz);

	if (il == -1 ||
	    !(spec = us[il])->query_wiz_level() ||
	    !query_ip_number(spec) ||
	    (spec->query_prop(WIZARD_I_BUSY_LEVEL) & BUSY_C) ||
	    spec->query_prop(OBJ_I_INVIS) > 0)
	{
	    break;
	}

	if (this_player()->query_mana() >=
	    this_player()->query_max_mana() / 10)
	{
	    this_player()->add_mana(-(this_player()->query_max_mana() / 10));
	}
	else
	{
	    write("Jestes zbyt wyczerpan" + 
	       this_player()->koncowka("y", "a") + " na to.\n");
	    return 1;
	}

	tell_object(spec, "PRZYZWANIE ciebie od '" + str + "': " + mess);
	flag = 1;
	break;
    }

    /* Log the commune message in a public log. */
    SECURITY->commune_log(wiz + ": " + mess);

    if (flag)
	write("Masz wrazenie, iz modlitwa od ciebie zostala wysluchana.\n");
    else
	write("Masz wrazenie, iz twa modlitwa nie zostala wysluchana.\n");
    
    return 1;
}

/*
 * reply - Allow mortals to reply when something is told to them.
 */
int
reply(string str)
{
    notify_fail("Komenda 'reply' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'odpowiedz'.\n");
}

/*
 * remember - Remember one of the livings introduced to us
 */
int
remember_live(string str)
{
    if (query_verb() == "remembered")
    {
	notify_fail("Komenda 'remembered' zostala wycofana. Zamiast " +
	    "niej mozesz uzyc 'zapamietani'.\n");
    }
    else
    {
	notify_fail("Komenda 'remember' zostala wycofana. Zamiast " +
	    "niej mozesz uzyc 'zapamietaj'.\n");
    }
    
    return 0;
}

/*
 * stop - Stop hunting or fighting
 */
int
stop(string str)
{
    notify_fail("Komenda 'stop' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'przestan'.\n");

    return 0;
}

/*
 * team - Tell me what team I am a member 
 */
varargs int
team(string str)
{
    notify_fail("Komenda 'team' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'druzyna'.\n");

    return 0;
}

/*
 * who - Tell what players are logged in and who we know
 */

int
who(string opts)
{
    notify_fail("Komenda 'who' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'kto'.\n");

    return 0;
}

/*
 * wesprzyj - Wspomoz przyjeciela w walce.
 */
int 
wesprzyj(string str)
{
    object *obs;
    object friend;
    object victim;
    int    index;
    mixed  tmp;

    if (!CAN_SEE_IN_ROOM(this_player()))
    {
	notify_fail("Nie widzisz tu nic.\n");
	return 0;
    }

    if (this_player()->query_ghost())
    {
	notify_fail("O wlasnie, zabit" + this_player()->koncowka("y", "a") +
	    ". Oto, jak" + this_player()->koncowka("i", "a") + 
	    " jestes.\n");
	return 0;
    }

    if (!stringp(str))
    {
	if (!sizeof(obs = this_player()->query_team_others()))
	{
	    notify_fail("Wesprzyj kogo? Nie jestes w zadnej druzynie.\n");
	    return 0;
	}

	obs = ({ this_player()->query_leader() }) - ({ 0 }) + obs;

	for (index = 0; index < sizeof(obs); index++)
	{
	    if ((environment(this_player()) == environment(obs[index])) &&
		(objectp(victim = obs[index]->query_attack())))
	    {
		friend = obs[index];
		break;
	    }
	}

	if (!objectp(friend))
	{
	    notify_fail("Nikt z twojej druzyny nie jest zaangazowany " +
		"w walke.\n");
	    return 0;
	}
    }
    else
    {
	obs = parse_this(str, "%l:" + PL_BIE);

	this_player()->set_obiekty_zaimkow(obs);

	if (sizeof(obs) > 1)
	{
	    notify_fail("Zdecyduj sie, nie mozesz wesprzec jednoczesnie " +
		COMPOSITE_LIVE(obs, PL_BIE) + ".\n");
	    return 0;
	}
	else if (sizeof(obs) == 0)
	{
	    notify_fail("Wesprzyj kogo?\n");
	    return 0;
	}

	friend = obs[0];
    }

    if (friend == this_player())
    {
	write("Jasne! Wesprzyj sie sam" + this_player()->koncowka("", "a") +
	    "!\n");
	return 1;
    }

    if (member_array(friend, this_player()->query_enemy(-1)) != -1)
    {
	write("Pomoc " + friend->query_imie(this_player(), PL_CEL) +
	    " w zabiciu siebie? Sa chyba prostsze sposoby na popelnienie "+
	    "samobojstwa...\n");
	return 1;
    }

    victim = friend->query_attack();
    if (!objectp(victim))
    {
	write(friend->query_Imie(this_player(), PL_MIA) +
	    " nie walczy z nikim.\n");
	return 1;
    }

    if (member_array(victim, this_player()->query_team_others()) != -1)
    {
	notify_fail("Ale jestescie razem w druzynie z " + 
	    victim->query_imie(this_player(), PL_NAR) + ".\n");
	return 0;
    }

    if (this_player()->query_attack() == victim)
    {
	write("Juz walczysz z " + victim->query_imie(this_player(), PL_NAR) + 
	    ".\n");
	return 1;
    }

    if (tmp = environment(this_player())->query_prop(ROOM_M_NO_ATTACK))
    {
	if (stringp(tmp))
	    write(tmp);
	else
	    write("Jakas boska sila nie pozwala ci zaatakowac.\n");
	return 1;
    }

    if (tmp = victim->query_prop(OBJ_M_NO_ATTACK))
    {
	if (stringp(tmp))
	    write(tmp);
	else
	    write("To istnienie jest chronione przez jakas boska sile - " +
		"nie wazysz sie nan podniesc reki.\n");
	return 1;
    }

    if ((!this_player()->query_npc()) &&
	(this_player()->query_met(victim)) &&
	(this_player()->query_prop(LIVE_O_LAST_KILL) != victim))
    {
	write("Zaatakowac " + victim->query_imie(this_player(), PL_BIE) +
	    "?!? Potwierdz to przez ponowne zaatakowanie.\n");
	this_player()->add_prop(LIVE_O_LAST_KILL, victim);
	return 1;
    }

    /*
     * Check if we dare!
     */
    if (!F_DARE_ATTACK(this_player(), victim))
    {
	write("Uhh.. Nie, nie odwazysz sie na to!\n");
	return 1;
    }

    this_player()->reveal_me(1);

    say(QCIMIE(this_player(), PL_MIA) + " wspiera " + QIMIE(friend, PL_BIE) +
	" w walce z " + QIMIE(victim, PL_NAR) + ".\n",
	({ this_player(), friend, victim }) );
    tell_object(victim, this_player()->query_Imie(victim, PL_MIA) +
	" atakuje cie!\n");
    tell_object(friend, this_player()->query_Imie(friend) +
	" wspiera cie w walce z " + victim->query_imie(friend, PL_NAR) + 
	".\n");

    this_player()->attack_object(victim);
    this_player()->add_prop(LIVE_O_LAST_KILL, victim);

    write("Ok.\n");
    return 1;
}


/*
 * zabij - Zaatakuj kogos, w celu zabicia go.
 */
varargs int 
zabij(string str)
{
    object ob, *oblist;
    string str2;
    mixed  tmp;

    if (!CAN_SEE_IN_ROOM(this_player()))
    {
	notify_fail("Nie widzisz tu nic.\n");
	return 0;
    }

    if (this_player()->query_ghost())
    {
	notify_fail("O wlasnie, zabit" + this_player()->koncowka("y", "a") +
	    ". Oto, jak" + this_player()->koncowka("i", "a") + 
	    " jestes.\n");
	return 0;
    }
    if (!stringp(str))
    {
	notify_fail("Zabij kogo?\n");
	return 0;
    }

    if (!parse_command(str, all_inventory(environment(this_player())),
       "%i:" + PL_BIE, oblist) || !sizeof(oblist = NORMAL_ACCESS(oblist, 0, 0)))
    {
	notify_fail("Nie widzisz zadnej takiej osoby.\n");
	return 0;
    }

    if (sizeof(oblist) > 1)
    {
	notify_fail("Troche konkretniej, nie mozesz zabic jednoczesnie " + 
	    COMPOSITE_LIVE(oblist, PL_DOP) + ".\n");
	return 0;
    }

    this_player()->set_obiekty_zaimkow(oblist);

    ob = oblist[0];

    if (!objectp(ob))
    {
	notify_fail(capitalize(str) + " juz tu nie ma!\n");
	return 0;
    }

    if (!living(ob))
    {
	write("To nie jest istota zywa!\n");
	saybb(QCIMIE(this_player(), PL_MIA) + " probuje zabic " + str + ".\n");
	return 1;
    }

    if (ob == this_player())
    {
	write("Co takiego? Siebie chcesz zaatakowac?\n");
	return 1;
    }

    if (this_player()->query_attack() == ob)
    {
	write("Tak, tak.\n");
	return 1;
    }

    if (tmp = environment(this_player())->query_prop(ROOM_M_NO_ATTACK))
    {
	if (stringp(tmp))
	    write(tmp);
	else
	    write("Jakas boska sila nie pozwala ci zaatakowac.\n");
	return 1;
    }

    if (tmp = ob->query_prop(OBJ_M_NO_ATTACK))
    {
	if (stringp(tmp))
	    write(tmp);
	else
	    write("To istnienie jest chronione przez jakas boska sile - " +
		"nie wazysz sie nan podniesc reki.\n");
	return 1;
    }

    if (!this_player()->query_npc() &&
	this_player()->query_met(ob) &&
	(this_player()->query_prop(LIVE_O_LAST_KILL) != ob))
    {
	write("Zaatakowac " + ob->query_imie(this_player(), PL_BIE) +
	    "?!? Potwierdz to przez ponowne zaatakowanie.\n");
	this_player()->add_prop(LIVE_O_LAST_KILL, ob);
	return 1;
    }

    /* Check if we dare! */
    if (!F_DARE_ATTACK(this_player(), ob))
    {
	write("Uhh.. Nie, nie odwazysz sie na to!\n");
	return 1;
    }

    if (this_player()->reveal_me(0))
	write("Atakujac " + ob->query_imie(this_player(), PL_BIE) +
	    " wychodzisz z ukrycia.\n");

    say(QCIMIE(this_player(), PL_MIA) + " atakuje " + QIMIE(ob, PL_BIE) + ".\n",
	({ this_player(), ob }) );
    tell_object(ob, this_player()->query_Imie(ob, PL_MIA) + " atakuje cie!\n");

    this_player()->attack_object(ob);
    this_player()->add_prop(LIVE_O_LAST_KILL, ob);

    write("Ok.\n");
    return 1;
}


/*
 * zapamietaj - Zapamietuje osobe, ktora sie nam przedstawila.
 */
int
zapamietaj(string str)
{
    object ob;
    mapping tmp;
    int num;
    string *imie;

    if (!strlen(str))
	return notify_fail("Zapamietaj kogo?\n");

    str = lower_case(str);    
    
    if (this_player()->query_real_name(PL_BIE) == str)
    {
	write("Kogo jak kogo, ale siebie znasz wystarczajaco dobrze.\n");
	return 1;
    }

    if (this_player()->query_wiz_level())
    {
	write("Alez niesmiertelni znaja kazdego!\n");
	return 1;
    }
    
    switch (this_player()->add_remembered(str))
    {
    case -1:
	write("Nie wepchniesz juz niczego wiecej do swej biednej glowy.\n");
	return 1;
    case 1:
	write("Ok.\n");
 	return 1;
    case 2:
	write("Wspominasz " + capitalize(str) + ".\n");
	return 1;
    default:
	notify_fail("Nie przypominasz sobie, zebys ostatnio spotkal" +
	    this_player()->koncowka("", "a") + " osobe o tym imieniu.\n");
	return 0;
    }
}

/*
 * zapamietani - Lista osob, ktore mozemy jeszcze zapamietac
 */
int
zapamietani(string str)
{
    mapping tmp;
    int num;
    
    if (this_player()->query_wiz_level())
    {
	notify_fail("Niesmiertelni, tacy jak ty, znaja kazdego.\n");
	return 0;
    }
    
    tmp = this_player()->query_remembered();
    if (mappingp(tmp))
    {
	if (num = m_sizeof(tmp))
	{
	    num = F_MAX_REMEMBERED(this_player()->query_stat(SS_INT),
		this_player()->query_stat(SS_WIS)) -
		num;
	    if (num < 0)
		num = 0;

	    write("Pamietasz ");
	    write(COMPOSITE_WORDS(map(sort_array(m_values(tmp)), capitalize)) + 
		    ".\n");
	    if (num)
		write("Wydaje ci sie, ze twoj umysl jest w stanie pomiescic " +
		    "jeszcze " + LANG_SNUM(num, PL_BIE, PL_NIJAKI_NOS) + " " + 
		    (num > 1 ? ({ "imie", "imion", "", 
		    "imiona" })[LANG_PRZYP(num, PL_BIE, PL_NIJAKI_NOS)] : 
		    "imie") + ".\n");
	    else
		write("Twoj biedny umysl chyba nie pomiesci juz nikogo " +
		    "wiecej.\n");
			
	    return 1;
	}
    }
    
    write("Nikogo nie znasz.\n");
    
    return 1;
}


/*
 * zapomnij - zapomina kogos uprzednio zapamietanego
 */
int
zapomnij(string name)
{
    string *tab;
    
    if (!stringp(name))
    {
	notify_fail("Zapomnij kogo?\n");
	return 0;
    }

    if (tab = this_player()->remove_remembered(name))
    {
	this_player()->add_introduced(tab[0], tab[1]);
	write("Ok.\n");
	return 1;
    }
    else
    {
	notify_fail("Nie znasz nikogo o tym imieniu.\n");
	return 0;
    }
}

/*
 * zapros - Zapros kogos do druzyny
 */
int
zapros(string name)
{
    object *oblist, member;
    int num, i;

    if (!name)
    {
	oblist = (object *) this_player()->query_invited();
	if (!oblist || !sizeof(oblist))
	    write("Nie zaprosil" + this_player()->koncowka("es", "as") +
		" nikogo do druzyny.\n");
	else
	{
	    if (sizeof(oblist) == 1)
		write("Zaprosil" + this_player()->koncowka("es", "as") + 
		     " " + oblist[0]->query_imie(this_player(), PL_BIE) + 
		     ".\n");
	    else 
	    {
		name = COMPOSITE_LIVE(oblist, PL_BIE);
		write("Zaprosil" + this_player()->koncowka("es", "as") + " " +
		     LANG_SNUM(num = sizeof(oblist), PL_BIE, PL_ZENSKI) +
		     " osob" + ({ "e", "", "", "y" })[LANG_PRZYP(num, 
		     PL_BIE, PL_ZENSKI)] + ":\n" + name + ".\n");
	    }
	}
	return 1;
    }
    
    if (this_player()->query_leader())
    {
	notify_fail("Nie mozesz zapraszac ludzi do druzyny, jesli jej nie "+
	    "przewodzisz.\n");
	return 0;
    }
    
    if (!parse_command(name, all_inventory(environment(this_player())),
	"%l:" + PL_BIE, oblist))
    {
	notify_fail("Zapros kogo?\n");
	return 0;
    }

    oblist = NORMAL_ACCESS(oblist, 0, 0);

    if (!(i = sizeof(oblist)))
    {
	notify_fail("Zapros kogo?\n");
	return 0;
    }

    this_player()->set_obiekty_zaimkow(oblist);

    if (i > 1)
    {
	write("Nie mozesz zaprosic jednoczesnie " + COMPOSITE_LIVE(oblist,
	    PL_DOP) + ".\n");
	return 1;
    }
    
    member = oblist[0];

    if (member == this_player())
    {
	notify_fail("Nie musisz sie zapraszac do wlasnej druzyny!\n");
	return 0;
    }
    
    if (!CAN_SEE(member, this_player()))
    {
	write("Ok.\n");
	tell_object(member, "Ktos probuje cie zaprosic do jakiejs " +
	    "druzyny. Niestety nie widzisz kto to moze byc.\n");
	return 1;
    }

    this_player()->team_invite(member);
    tell_object(member, this_player()->query_Imie(member, PL_MIA) +
	" zaprasza cie do swojej druzyny.\n");
    this_player()->reveal_me(1);
    member->reveal_me(1);
		
    write("Ok.\n");
    return 1;
}

/*
 * Zapytaj - zapytaj kogos o cos.
 */
int
zapytaj(string str)
{
    object *oblist;
    object *wizards;
    string msg;
    mixed tmp;
    int ix;

    if (!CAN_SEE_IN_ROOM(this_player()))
    {
	notify_fail("Nic tu nie widzisz.\n");
	return 0;
    }

    if (!stringp(str))
    {
	notify_fail("Zapytaj kogo o co?\n");
	return 0;
    }

    if (!parse_command(str, environment(this_player()), 
		      "%l:" + PL_BIE + " 'o' %s", 
		      oblist, msg))
    {
	if (!sizeof(oblist))
	{
	    notify_fail("Nie ma tu nikogo takiego.\n");
	    return 0;
	}
    
	if (!msg)
	{
	    notify_fail("Chyba chcesz sie zapytac _o_ cos?\n");
	    return 0;
	}
    }
	
    oblist = NORMAL_ACCESS(oblist, 0, 0);
	
    if (!sizeof(oblist))
    {
	notify_fail("Nie ma tu nikogo takiego.\n");
	return 0;
    }

    if (sizeof(oblist) > 1)
    {
	notify_fail("Pytanie jednej osoby naraz calkowicie wystarczy.\n");
	return 0;
    }

    if (oblist[0] == this_player())
    {
	notify_fail("Siebie chcesz o to spytac?\n");
	return 0;
    }

    if (!strlen(msg))
    {
	write("O co chcesz sie zapytac "
	    + oblist[0]->query_imie(this_player(), PL_BIE) + "?\n");
	return 1;
    }

    if (tmp = this_player()->query_prop(LIVE_M_MOUTH_BLOCKED))
    {
	write(stringp(tmp) ? tmp : "Nie mozesz wydobyc z siebie glosu.\n");
	return 1;
    }

    this_player()->set_obiekty_zaimkow(oblist);

    if (this_player()->query_option(OPT_ECHO))
    {
	write("Pytasz " + oblist[0]->query_imie(this_player(), PL_BIE)
	    + " o " + msg + ".\n");
    }
    else
    {
	write("Ok.\n");
    }

    /* If the actor is a mortal, give the message to all wizards. */
    if (!this_player()->query_wiz_level())
    {
	wizards = FILTER_IS_WIZARD(all_inventory(environment(this_player()))) -
	     oblist;
	ix = sizeof(wizards);
	while (--ix >= 0)
	{
	    wizards[ix]->catch_tell(this_player()->query_Imie(wizards[ix], PL_MIA) +
		" pyta " + oblist[0]->query_imie(wizards[ix], PL_BIE) +
		" o " + msg + ".\n");
	}
    }
    else
    {
	wizards = ({ });
    }

    this_player()->reveal_me(1);
    saybb(QCIMIE(this_player(), PL_MIA) + " pyta " + QIMIE(oblist[0], PL_BIE) +
	" o cos.\n", wizards + oblist + ({ this_player() }) );

    tell_object(oblist[0], this_player()->query_Imie(oblist[0], PL_MIA) + 
    	    " pyta sie ciebie o " + msg + ".\n");
    oblist[0]->catch_question(msg);
    oblist[0]->reveal_me(1);

    return 1;
}
