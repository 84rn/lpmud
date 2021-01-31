/*
 * /lib/guild_support.c
 *
 * Some support for meditating code.
 */
#pragma strict_types
#pragma save_binary

#include <macros.h>
#include <language.h>
#include <ss_types.h>
#include <stdproperties.h>
#include <composite.h>
#include <state_desc.h>

/*
 * These properties are internal to this module. No need to define them
 * elsewhere.
 */
#define GUILD_I_COUNT	  "_guild_i_count"
#define GUILD_AI_PREFS	  "_guild_ai_prefs"
#define GUILD_I_ZERO_USED "_guild_i_zero_used"

#define KONC this_player()->koncowka

/*
 * Global variable. It is not saved.
 */
static string *stat_names, *stat_names_mia, *stat_names_bie;

/*
 * Prototypes.
 */
int gs_catch_all(string arg);
int gs_meditate(string str);

/*
 * Function name: get_pref
 * Description:   This is the input_to function that is used by set_prefs
 *                You should never call it directly
 */
void
get_pref(string str)
{
    int i, count, zero, *prefs;

    count = this_player()->query_prop(GUILD_I_COUNT);
    prefs = this_player()->query_prop(GUILD_AI_PREFS);
    zero = this_player()->query_prop(GUILD_I_ZERO_USED);
    
    if (!pointerp(prefs))
    {
	prefs = allocate(SS_NO_EXP_STATS);
    }

    if (!stringp(str) ||
	!sscanf(str, "%d", i) ||
	(i < 5 && zero) ||
	(i > 25))
    {
	if (str == "~q")
	{
	    write("Wspolczynniki nie ulegly zmianie.\n");
	    this_player()->remove_prop(GUILD_I_COUNT);
	    this_player()->remove_prop(GUILD_AI_PREFS);
	    this_player()->remove_prop(GUILD_I_ZERO_USED);
	    return;
	}
	write("Zla wartosc - sprobuj ponownie. (~q zeby zaniechac)\n");
        write(stat_names[count] + ": ");
        input_to(get_pref);
        return;
    }

    if (i < 5)
    {
	zero = 1;
    }

    prefs[count++] = (i >= 0 ? i : 0);
    if (count == SS_NO_EXP_STATS)
    {
        this_player()->set_learn_pref(prefs);
        write("W ten oto sposob rozlozyl" + 
            KONC("es", "as") + " swoje wspolczynniki "+
            "przyrostu: \n");
        prefs = this_player()->query_learn_pref(-1);
        for (i = 0; i < SS_NO_EXP_STATS; i++)
	{
            write(stat_names[i] + ": " + prefs[i] +
                  (i < SS_NO_EXP_STATS -1 ? ", " : "\n"));
	}
	this_player()->remove_prop(GUILD_I_COUNT);
	this_player()->remove_prop(GUILD_AI_PREFS);
	this_player()->remove_prop(GUILD_I_ZERO_USED);
        return;
    }

    write(stat_names[count] + ": ");
    this_player()->add_prop(GUILD_I_COUNT, count);
    this_player()->add_prop(GUILD_AI_PREFS, prefs);
    this_player()->add_prop(GUILD_I_ZERO_USED, zero);
    input_to(get_pref);
    return;
}

/*
 * Function name: create_guild_support
 * Description:   Set up the stat_names variable
 */
void
create_guild_support()
{
    stat_names = SD_STAT_NAMES;
    stat_names_mia = SD_STAT_NAMES_MIA;
    stat_names_bie = SD_STAT_NAMES_BIE;
}

/*
 * Function name: set_prefs
 * Description:   An interactive sequence that allows you to set your
 *                learning preferences
 * Returns:       1
 */
int
set_prefs()
{
    int i, *orig_prefs;

    write("Twe wspolczynniki przyrostu teraz tak wygladaja: \n");
    orig_prefs = this_player()->query_learn_pref(-1);
    for (i = 0; i < SS_NO_EXP_STATS; i++)
    {
        write(stat_names[i] + ": " + orig_prefs[i] +
              (i < SS_NO_EXP_STATS -1 ? ", " : "\n"));
    }

    write("Mozesz przypisac kazdemu wspolczynnikowi wartosc, oznaczajaca " +
        "jak bardzo chcesz sie koncentrowac na jego rozwoju. Dozwolone " +
        "sa liczby z zakresu 5 - 25. Kazda wartosc jest rozpatrywana " +
        "wzgledem pozostalych i bedzie przeliczona tak, by suma wyniosla " +
        "100 minus podatek gildiowy. Aby zaniechac wpisywania, wpisz ~q.\n");
        
    write(stat_names[0] + ": ");
    input_to(get_pref);

    return 1;
}

/*
 * Function name: assess
 * Description:   Allows a player to estimate how far he is from the next
 *                stat description.
 * Parameters:    stat - one of "STR", "DEX", "CON", "INT", "WIS", "DIS"
 *                lower case is ok as well
 * Returns:       1 - success, 0 - stat not found
 */
int
assess(string stat)
{
    int i, j, k, n, n2, residue;
    string *a_strings;
    mixed stat_strings;
    object tgive;

    if (!stat)
    {
        write("Skladnia: " + query_verb() + " <wspolczynnik>\n");
        return 1;
    }
        
    stat = lower_case(stat);
    
    for (i = 0; ; i++)
    {
        if (i >= sizeof(stat_names))
        {
            write("Masz do wyboru: " + COMPOSITE_WORDS(stat_names_bie) + ".\n");
            return 1;
        }
        if (stat == stat_names_bie[i] || stat == lower_case(stat_names[i]))
            break;
    }

    j = this_player()->query_stat(i);
    if (j >= 160)
    {
        write("Twa " + stat_names_mia[i] + " osiagnela nadludzki poziom.\n");
        return 1;
    }

    if (j >= 136)
    {
        write("Masz epicka " + stat_names_bie[i] + ".\n");
        return 1;
    }

/*
    if (j >= 100)
    {
	write("If all your stats were this good you would be able " +
	      "to try the wizquest.\n");
    }
*/
    
    /* 8 descriptions / 17 steps each */
/*
    a_strings = ({ "very far from", "far from", "halfway to",
                   "close to", "very close to" });
 */
    a_strings = ({ "bardzo duzo", "duzo", "troche", "niewiele", 
        "bardzo niewiele" });

    for (n = 7, n2 = 7 * 17; ; n--, n2 -= 17)
    {
	if (j >= n2)
	{
	    residue = j - n2;
	    break;
	}
    }

    write("Wydaje ci sie, ze " + a_strings[(residue * sizeof(a_strings)) / 17] +
        " ci brakuje, zebys mogl" + KONC("", "a") +
	" wyzej ocenic swa " + stat_names_bie[i] + ".\n");
	
    return 1;
}

/*
 * Function name: gs_leave_inv
 * Description:   Should be called if someone leaves the room. if that person
 *		  was meditating, better do something. You should call
 *		  this function from leave_inv() in your room.
 */
void
gs_leave_inv(object ob, object to)
{
    if (ob->query_prop(LIVE_I_MEDITATES))
    {
	ob->remove_prop(LIVE_I_MEDITATES);
	ob->remove_prop(GUILD_I_COUNT);
	ob->remove_prop(GUILD_AI_PREFS);
	ob->remove_prop(LIVE_S_EXTRA_SHORT);
    }
}

/*
 * Function name: init_guild_support
 * Description:   Add the meditate command to the player
 */
void
init_guild_support()
{
    if (!stat_names)
    {
	create_guild_support();
    }

    add_action(gs_meditate, "medytuj");
}

/*
 * Function name: gs_hook_already_meditate
 * Description:	  Called when player is already meditating
 * Returns:	  Always 1
 */
int
gs_hook_already_meditate()
{
    write("Juz jestes pograzon" + KONC("y", "a") + " w transie. Jesli "+
        "chcesz sie zen wydostac, 'powstan'.\n");
    return 1;
}

/*
 * Function name: gs_hook_start_meditate
 * Description:   Called when player starts to meditate
 */
void
gs_hook_start_meditate()
{
    write("Spokojnie klekasz na ziemi, zamykasz oczy i starasz wprowadzic "+
        "sie w gleboki trans. Po jakims nieokreslonym czasie stwierdzasz, " +
        "iz nie docieraja do ciebie zadne bodzce z zewnetrz, zas ty sam" + 
        KONC("", "a") + " jestes w stanie skoncentrowac sie na esencji " +
        "wlasnego jestestwa. Potrafisz 'ocenic' wlasne cechy, jak rowniez " +
        "'ustawic' na rozwoju ktorych cech chcesz sie skoncentrowac. Zeby "+
        "sprobowac wyrwac sie z transu, 'powstan'.\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " kleka na ziemi i zaczyna " +
          "medytowac.\n");
}

/*
 * Function name: gs_hook_rise
 * Description:	  Called when player rises from the meditation
 */
void
gs_hook_rise()
{
    write("Wynurzajac sie z otchlani wlasnego umyslu, odzyskujesz powoli "+
        "zmysly. Czujesz sie bardzo wypoczet" + KONC("y", "a") + " i " +
        "zrelaksowan" + KONC("y", "a") +
        ", gdy w koncu powstajesz.\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " powstaje.\n");
}

/*
 * Function name: gs_hook_catch_error
 * Description:   Called player tried to do something strange while meditating
 *		  like examin things or leave the room.
 * Arguments:	  str - Argument the player tried to send to his command
 * Returns:	  1 normally.
 */
int
gs_hook_catch_error(string str)
{
    write("Nie mozesz tego robic w czasie medytacji.\n");
    return 1;
}

/*
 * Function name: gs_meditate
 * Description:   Player wants to meditate
 */
int
gs_meditate(string str)
{
    string long_str;

    this_player()->add_prop(LIVE_S_EXTRA_SHORT, " medytuje");
    if (this_player()->query_prop(LIVE_I_MEDITATES))
	return gs_hook_already_meditate();

    this_player()->add_prop(LIVE_I_MEDITATES, 1);

    gs_hook_start_meditate();

    add_action(gs_catch_all, "", 1);
    return 1;
}

/*
 * Function name: gs_rise
 * Description:   Player rises
 */
int
gs_rise()
{
    gs_hook_rise();
    this_player()->remove_prop(LIVE_I_MEDITATES);
    return 1;
}

/*
 * Function name: gs_catch_all
 * Description:	  Catch all commands the player makes while meditating
 */
int
gs_catch_all(string arg)
{
    string action;

    if (!this_player()->query_prop(LIVE_I_MEDITATES))
	return 0;
    action = query_verb();

    switch (action)
    {
    case "ocen":
	return assess(arg);
    case "medytuj":
	return gs_meditate("");
    case "ustaw":
	set_prefs();
	return 1;
    case "powstan":
	this_player()->remove_prop(LIVE_S_EXTRA_SHORT);
	gs_rise();
	return 1;
    case "cechy":
    case "przyzwij":
    case "zglos":
    case "odpowiedz":
	return 0;
    default:
	return gs_hook_catch_error(arg);
    }
}

