/*
 * /cmd/std/command_driver.c
 *
 * These routines handles the actual finding and execution of commands.
 * It is called from cmdhooks.c in the living object that performs the
 * command.
 */

#pragma no_clone
#pragma save_binary
#pragma strict_types

inherit "/std/callout";

#include <adverbs.h>
#include <cmdparse.h>
#include <composite.h>
#include <filter_funs.h>
#include <macros.h>
#include <std.h>
#include <stdproperties.h>

/* Dodaje spacje przed stringiem, o ile nie zaczyna sie on od przecinka. */
#define ADD_SPACE_TO_STRING(str) (str[0] == ',' ? str : " " + str)

/*
 * Prototype.
 */
public mapping query_cmdlist();

/*
 * Global variable. It contains the list of verbs and functions.
 */
static mapping cmdlist = query_cmdlist();

/*
 * Function name: query_cmdlist
 * Description  : You must redefine this function in your soul. This
 *                function returns a mapping with the verbs and functions
 *                that are defined in the soul. The format of the mapping
 *                should be ([ "verb" : "function" ]).
 * Returns      : mapping - the verb & function - mapping.
 */
public mapping
query_cmdlist()
{
    return ([]);
}

/*
 * Function name: update_commands
 * Description  : This function is called from the spell object when a
 *                new spell is added.
 */
public void
update_commands()
{
    cmdlist = query_cmdlist();
}

/* 
 * Function name: do_command
 * Description  : Perform the given command, if present.
 * Arguments    : string verb - the verb the player executes.
 *                string arg  - the command line argument.
 * Returns      : int - 1/0 depending on success.
 */
public int
do_command(string verb, string arg)
{
    return call_other(this_object(), cmdlist[verb], arg);
}

/* 
 * Function name: exist_command
 * Description  : Check if a command exists.
 * Returns      : int - 1/0 depending on success.
 */
public nomask int
exist_command(string verb)
{
    return stringp(cmdlist[verb]);
}

/*
 * Function name: open_soul
 * Description  : Set the euid of the soul to 0.
 */
public nomask void
open_soul(int state)
{
    if (state)
        seteuid(getuid(this_object()));
    else
        SECURITY->remote_setuid();
}

/*
 * Function name: teleledningsanka
 * Description  : This function is used to load a soul and in command
 *                souls also to set the uid, euid to "backbone". If you
 *                don't speak Swedish, rest assured that the name of this
 *                function isn't real Swedish either.
 */
public nomask void
teleledningsanka()
{
    SECURITY->remote_setuid();
}

/*
 * Function name: using_soul
 * Description  : this function is called from /std/living.c when it first
 *                starts to use the soul and can be used for initialization.
 * Arguments    : object live - the living that is going to use this soul.
 */
public void
using_soul(object live)
{
}

/*
 * Kilka funkcji ulatwiajacych tworzenie emotow.
 */

/*
 * Nazwa funkcji : actor
 * Opis          : Wypisuje wykonawcy emota wiadomosc, o ile istnieje cel
 *                 przedsiebranej przez niego akcji. Standardowym zakonczeniem
 *                 wiadomosci jest kropka. Zawsze dodawany jest znak konca
 *                 linii. Zwrocmy uwage na dobor spacji (patrz przyklady).
 * Przyklad      : actor("Usmiechasz sie do", oblist, PL_DOP);
 *
 *                 Usmiechasz sie do kogos.
 *                 Usmiechasz sie do Alvina.
 *                 Usmiechasz sie do bursztynookiego elfa czarodzieja.
 *
 *                 actor("Lapiesz", oblist, PL_BIE, " za jezyk.");
 *
 *                 Lapiesz kogos za jezyk.
 *                 Lapiesz Lewego za jezyk.
 *                 Lapiesz czlowieka czarodzieja za jezyk.
 *
 * Argumenty     : string str     - poczatek wiadomosci do wyswietlenia.
 *                 object *cele   - cele emota.
 *                 int    przyp   - przypadek, w jakim maja byc cele.
 *                 string str1    - opcjonalnie, zakonczenie wiadomosci.
 */
public void
actor(string str, object *cele, int przyp, string str1 = ".")
{
    write(str + " " + COMPOSITE_LIVE(cele, przyp) + str1 + "\n");
}

/*
 * Nazwa funkcji : target
 * Opis          : Wypisuje celowi emota wiadomosc. Zawsze dodawany jest znak
 *                 konca linii.
 * Przyklad      : target("cie sciska.", oblist);
 *
 *                 Ktos cie sciska.
 *                 Alvin cie sciska.
 *                 Bursztynooki elf czarodzie cie sciska.
 *
 * Argumenty     : string str     - wiadomosc do wyswietlenia.
 *                 object *cele   - adresaci wiadomosci.
 *                 string adverb  - opcjonalnie - przyslowek (dla npcow).
 */
public void
target(string str, object *cele, string adverb = 0, string info = 0)
{
    int i = -1;
    int ilu;
    object *npce;

    str = ADD_SPACE_TO_STRING(str);

    npce = cele - FILTER_PLAYERS(cele);
    if (sizeof(npce))
    {
        npce->emote_hook(query_verb(), this_player(), adverb, info);
        cele -= npce;
    }
    ilu = sizeof(cele);
    while (++i < ilu)
        cele[i]->catch_msg(this_player()->query_Imie(cele[i], PL_MIA) + str
                         + "\n");
}

/*
 * Nazwa funkcji : targetbb
 * Opis          : Identyczna z target(), z ta roznica ze wiadomosc jest
 *                 wyswietlana jedynie widzacym wykonawce emota.
 * Przyklad      : targetbb("usmiecha sie do ciebie radosnie.", oblist);
 * Argumenty     : patrz target().
 */
public void
targetbb(string str, object *cele, string adverb = 0, string info = 0)
{
    int i = -1;
    int ilu;
    object *npce;

    str = ADD_SPACE_TO_STRING(str);

    cele = FILTER_CAN_SEE_IN_ROOM(cele);
    cele = FILTER_IS_SEEN(this_player(), cele);
    npce = cele - FILTER_PLAYERS(cele);
    if (sizeof(npce))
    {
        npce->emote_hook(query_verb(), this_player(), adverb, info);
        cele -= npce;
    }
    ilu = sizeof(cele);
    while (++i < ilu)
        cele[i]->catch_msg(this_player()->query_Imie(cele[i], PL_MIA) + str
                         + "\n");
}

/*
 * Nazwa funkcji : all
 * Opis          : Wyswietla wiadomosc swiadkom emota, jesli nie angazuje on
 *                 zadnych osob, poza wykonawca. Zawsze dodawany jest znak
 *                 konca linii.
 * Przyklad      : all("glosno krzyczy.");
 *
 *                 Ktos glosno krzyczy.
 *                 Alvin glosno krzyczy.
 *                 Bursztynooki elf czarodziej glosno krzyczy.
 *
 * Argumenty     : string str    - wiadomosc do wyswietlenia.
 *                 string adverb - opcjonalnie - przyslowek (dla npcow).
 */
public void
all(string str, string adverb = 0, string info = 0)
{
    int i = -1;
    int ilu;
    object *cele;
    object *npce;

    str = ADD_SPACE_TO_STRING(str);

    cele = FILTER_OTHER_LIVE(all_inventory(environment(this_player())));
    npce = cele - FILTER_PLAYERS(cele);
    if (sizeof(npce))
    {
        npce->emote_hook_onlookers(query_verb(), this_player(), 0, adverb,
                                   info);
        cele -= npce;
    }
/*
    say(QCIMIE(this_player(), PL_MIA) + " " + str + "\n", npce);
*/
    ilu = sizeof(cele);
    while (++i < ilu)
        cele[i]->catch_msg(this_player()->query_Imie(cele[i], PL_MIA) + str
                         + "\n");
}

/*
 * Nazwa funkcji : allbb
 * Opis          : Identyczna z all(), z ta roznica ze wiadomosc jest
 *                 wyswietlana jedynie widzacym wykonawce emota.
 * Przyklad      : allbb("usmiecha sie radosnie.");
 * Argumenty     : patrz all().
 */
public void
allbb(string str, string adverb = 0, string info = 0)
{
    int i = -1;
    int ilu;
    object *cele;
    object *npce;

    str = ADD_SPACE_TO_STRING(str);

    /* can_see_in_room() jest zdefiniowane tylko dla livingow, wiec
     * FILTER_CAN_SEE_IN_ROOM odfiltruje nam wylacznie livingi.
     */
    cele = FILTER_CAN_SEE_IN_ROOM(all_inventory(environment(this_player())) -
                                  ({this_player()}));
    cele = FILTER_IS_SEEN(this_player(), cele);
    npce = cele - FILTER_PLAYERS(cele);
    if (sizeof(npce))
    {
        npce->emote_hook_onlookers(query_verb(), this_player(), 0, adverb,
                                   info);
        cele -= npce;
    }
    ilu = sizeof(cele);
    while (++i < ilu)
        cele[i]->catch_msg(this_player()->query_Imie(cele[i], PL_MIA) + str
                         + "\n");
}

/*
 * Nazwa funkcji : all2act
 * Opis          : Wyswietla wiadomosc swiadkom emota, jesli oprocz wykonawcy
 *                 angazuje on takze inne osoby. Standardowym zakonczeniem
 *                 wiadomosci jest kropka. Zawsze dodawany jest znak konca
 *                 linii. Zwrocmy uwage na dobor spacji.
 * Przyklad      : all2act("policzkuje glosno", oblist);
 *
 *                 Ktos policzkuje glosno Alvina.
 *                 Lewy policzkuje glosno bursztynookiego elfa czarodzieja.
 *                 Lewy policzkuje glosno kogos.
 *                 Czlowiek czarodziej policzkuje glosno Alvina.
 *                 (etc. analogicznie)
 *
 *                 all2act("policzkuje", oblist, " z rozmachem.");
 *
 *                 Lewy policzkuje kogos z rozmachem.
 *                 (et caetera...)
 *
 * Argumenty     : string str     - pierwsza czesc wiadomosci do wyswietlenia.
 *                 object *cele   - cele emota (NIE mylic z obserwatorami).
 *                 int    przyp   - przypadek, w jakim maja byc cele.
 *                 string str1    - opcjonalnie, zakonczenie wiadomosci.
 *                 string adverb  - opcjonalnie, przyslowek (dla npcow).
 */
public void
all2act(string str, object *cele, int przyp, string str1 = ".",
        string adverb = 0, string info = 0)
{
    int i = -1;
    int ilu;
    object *widzowie;
    object *npce;

    str = ADD_SPACE_TO_STRING(str);

    widzowie = FILTER_OTHER_LIVE(all_inventory(environment(this_player())) -
                                 cele);
    npce = widzowie - FILTER_PLAYERS(widzowie);
    if (sizeof(npce))
    {
        npce->emote_hook_onlooker(query_verb(), this_player(), cele, adverb,
                                  info);
        widzowie -= npce;
    }
/*
    say(QCIMIE(this_player(), PL_MIA) + " " + str + " " + QCOMPLIVE(przyp)
      + str1 + "\n", cele + npce);
*/
    ilu = sizeof(widzowie);
    while (++i < ilu)
        widzowie[i]->catch_msg(this_player()->query_Imie(widzowie[i], PL_MIA)
                             + str + " "
                             + FO_COMPOSITE_LIVE(cele, widzowie[i], przyp)
                             + str1 + "\n");
}

/*
 * Nazwa funkcji : all2actbb
 * Opis          : Identyczna z all2act, z ta roznica, ze wiadomosc jest
 *                 wyswietlana jedynie widzacym wykonawce emota.
 * Argumenty     : Patrz all2act().
 */
public void
all2actbb(string str, object *cele, int przyp, string str1 = ".",
          string adverb = 0, string info = 0)
{
    int i = -1;
    int ilu;
    object *widzowie;
    object *npce;

    str = ADD_SPACE_TO_STRING(str);

    widzowie =
        FILTER_CAN_SEE_IN_ROOM(all_inventory(environment(this_player())) -
                               cele - ({this_player()}));
    widzowie = FILTER_IS_SEEN(this_player(), widzowie);
    npce = widzowie - FILTER_PLAYERS(widzowie);
    if (sizeof(npce))
    {
        npce->emote_hook_onlooker(query_verb(), this_player(), cele, adverb,
                                  info);
        widzowie -= npce;
    }
    ilu = sizeof(widzowie);
    while (++i < ilu)
        widzowie[i]->catch_msg(this_player()->query_Imie(widzowie[i], PL_MIA)
                             + str + " "
                             + FO_COMPOSITE_LIVE(cele, widzowie[i], przyp)
                             + str1 + "\n");
}

/*
 * Function name: parse_this
 * Description  : This is a parser with some extra functions and checks,
 *                specially designed for the soul.
 * Arguments    : string str - the string to parse.
 *                string form - the parse-pattern.
 * Returns      : object * - an array of matching objects.
 */
public object *
parse_this(string str, string form)
{
    object *oblist;

    /* Sanity checks. Player must be able to see in the room
       Use NORMAL_ACCESS to for instance get the 'second dwarf'. */ 

    if (str && CAN_SEE_IN_ROOM(this_player()) &&
        parse_command(str, environment(this_player()), form, oblist) &&
        sizeof(oblist = NORMAL_ACCESS(oblist, 0, 0)))
    {
        this_player()->set_obiekty_zaimkow(oblist);
        return oblist;
    }

    return ({});
}

/*
 * Function name: parse_adverb
 * Destription  : This function is designed to separate the adverb a player
 *                uses from the rest of the command line.
 * Arguments    : str     - the command line string. This may not be "", so
 *                          should either be 0 or a string containing at
 *                          least one character.
 *                def_adv - the default adverb for this emotion
 *                trail   - 1 if the adverb comes behind the target
 * Returns      : array of string: ({ cmd_str, adv_str })
 *                cmd_str - the rest of the line
 *                adv_str - the adverb
 *
 * Examples:
 *     parse_adverb("happ", "sadly", 0/1) returns
 *          ({ "", "happily" })
 *     parse_adverb("hupp", "sadly", 0/1) returns
 *          ({ "hupp", "sadly" })
 *     parse_adverb("happ at the dwarf wizard", "sadly", 0)
 *          ({ "at the wizard", "happily" })
 *     parse_adverb("Mercade", "sadly", 0)
 *          ({ "Mercade", "sadly" })
 *     parse_adverb("Mercade merri", "gracefully", 1)
 *          ({ "Mercade", "merrily" })
 *     parse_adverb("merri Mercade", "gracefully", 1)
 *          ({ "merri Mercade", "gracefully" })
 */
public string *
parse_adverb(string str, string def_adv, int trail)
{
    string *words;
    string adverb;
    int size;

    /* Argument zerowy - zwracamy standardowy przyslowek. */
    if (!str)
        return ({0, def_adv});

    words = explode(str, " ");

    switch(size = sizeof(words))        /* Liczba slow. */
    {
        case 1:
            if (strlen(adverb = FULL_ADVERB(str)))
                return ({0, adverb});   /* Znaleziono pasujacy przyslowek. */
            return ({str, def_adv});    /* Kicha. Nie znaleziono. */

        case 2:
            if (!trail && strlen(adverb = FULL_ADVERB(words[0])))
                return ({words[1], adverb});    /* Na poczatku, 1-wyrazowy. */
            if (strlen(adverb = FULL_ADVERB(str)))
                return ({0, adverb});           /* Dwuwyrazowy. */
            if (trail && strlen(adverb = FULL_ADVERB(words[1])))
                return ({words[0], adverb});    /* Na koncu, jednowyrazowy. */
            return ({str, def_adv});            /* Kicha. */

        default:
            if (trail)          /* Wiecej slow, przyslowek na koncu. */
            {
                /* Dwuwyrazowy. */
                if (strlen(adverb = FULL_ADVERB(words[size - 2] + " "
                                              + words[size - 1])))
                    return ({implode(words[..-3], " "), adverb});
                /* Jednowyrazowy. */
                if (strlen(adverb = FULL_ADVERB(words[size - 1])))
                    return ({implode(words[..-2], " "), adverb});
                /* Kicha. */
                return ({str, def_adv});
            }

            /* Wiecej slow, przyslowek na poczatku. */

            /* Jednowyrazowy. */
            if (strlen(adverb = FULL_ADVERB(words[0])))
                return ({implode(words[1..], " "), adverb});
             /* Dwuwyrazowy. */
            if (strlen(adverb = FULL_ADVERB(words[0] + " " + words[1])))
                return ({implode(words[2..], " "), adverb});
            return ({str, def_adv});
    }
}

/*
 * Function name: parse_adverb_with_space
 * Description  : This function returns the adverb from parse_adverb with a
 *                preceding space if it is an adverb and it returns an empty
 *                string if the adverb was the special adverb that means that
 *                the player does not want an adverb.
 * Arguments    : see parse_adverb
 * Returns      : see parse_adverb, the description and /sys/adverbs.h
 */
public string *
parse_adverb_with_space(string str, string def_adv, int trail)
{
    string *pa = parse_adverb(str, def_adv, trail);

    return ({pa[0], ADD_SPACE_TO_ADVERB(pa[1])});
}

/*
 * Function name: check_adverb
 * Description  : Returns the full adverb the player meant. If he did not
 *                specify an an adverb, return the default adverb.
 * Arguments    : str     - the pattern to match
 *                def_adv - the default adverb
 * Returns      : string  - the full adverb or NO_ADVERB
 */
public string
check_adverb(string str, string def_adv)
{
    return str ? FULL_ADVERB(str) : def_adv;
}

/*
 * Function name: check_adverb_with_space
 * Description  : This function returns the adverb from check_adverb with a
 *                preceding space if it is an adverb and it returns an empty
 *                string if the adverb was the special adverb that means that
 *                the player does not want an adverb.
 * Arguments    : see check_adverb
 * Returns        see check_adverb, the description and /sys/adverbs.h
 */
public string
check_adverb_with_space(string str, string def_adv)
{
    return ADD_SPACE_TO_ADVERB(check_adverb(str, def_adv));
}

/*
 * Nazwa funkcji: query_prevent_shadow
 * Opis         : Nie uwazamy za wlasciwej mozliwosci shadowowania souli.
 *                Ta funkcja zabezpiecza je przed ta ewentualnoscia -
 *                '#pragma no_shadow' nie dziala na obiekty dziedziczace.
 */
public nomask int
query_prevent_shadow()
{
    return 1;
}