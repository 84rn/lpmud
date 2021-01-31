/*
 *  Plik zawierajacy funkcje umozliwiajace graczom trenowanie umiejetnosci.
 *
 *  /lib/skill_raise.c
 *
 *  Koszt wytrenowania umiejetnosci od zera na dany poziom wynosi:
 *
 *      cost = (level ^ 3 * skill_cost_factor) / 100
 */

#pragma no_clone
#pragma save_binary
#pragma strict_types

#include <macros.h>
#include <money.h>
#include <ss_types.h>
#include <state_desc.h>
#include <stdproperties.h>
#include <tasks.h>

#define CONV_TS2SS(ts)	(-(ts) - 1)	/* Konwersja TS_xxx -> SS_xxx. */

/* Znane umiejetnosci */
private static mapping sk_skills = SS_SKILL_DESC;

/* Maksymalne poziomy umiejetnosci, do ktorych mozna sie tu szkolic. */
private static mapping sk_trains = ([]);

/* Nazwy poziomow bieglosci w umiejetnosciach */
private static string *desc = SD_SKILLEV;

/* Prototypy */
int sk_improve(string str);

/*
 * Nazwa funkcji : sk_query_levels
 * Opis          : Zwraca tablice opisow poziomow bieglosci.
 */
public nomask string *
sk_query_levels()
{
    return secure_var(desc);
}

/*
 * Nazwa funkcji : sk_add_skills
 * Opis          : Dodaje rozpoznawane umiejetnosci do standardowych.
 * Argumenty     : skill_desc: Mapping z umiejetnosciami domenowymi w
 *                             formacie SS_SKILL_DESC.
 */
public nomask void
sk_add_skills(mapping skill_desc)
{
    sk_skills += skill_desc;
}

/*
 * Nazwa funkcji : sk_add_train
 * Opis          : Dodaje umiejetnosc, ktora mozna tu trenowac.
 * Argumenty     : snum: Numer umiejetnosci.
 *                 smax: Maksymalny dostepny tu poziom.
 * Uwaga         : Jesli dodawana umiejetnosc nie jest standardowa, musi byc
 *                 uprzednio dodana przez funkcje sk_add_skills.
 */
public nomask void
sk_add_train(int snum, int smax)
{
    sk_trains[snum] = smax;
}

/*
 * Nazwa funkcji : sk_query_name
 * Opis          : Zwraca nazwe danej umiejetnosci.
 * Argumenty     : snum: Numer umiejetnosci.
 *                 b: Jesli 0, funkcja zwroci mianownik nazwy,
 *                    jesli 1, biernik,
 *                    jesli 2, dlugi opis w miejscowniku.
 */
public nomask string
sk_query_name(int snum, int b)
{
    mixed *skval = sk_skills[snum];

    return skval ? skval[0..2][b] : 0;
}

/*
 * Nazwa funkcji : sk_query_cost_factor
 * Opis          : Zwraca wspolczynnik kosztu umiejetnosci (0..100).
 * Argumenty     : snum: Numer umiejetnosci.
 */
public nomask int
sk_query_cost_factor(int snum)
{
    mixed *skval = sk_skills[snum];

    return skval ? skval[3] : 0;
}

/*
 * Nazwa funkcji : sk_query_stat_limits
 * Opis          : Zwraca limity z cech i umiejetnosci.
 * Argumenty     : snum: Numer umiejetnosci.
 */
public nomask mapping
sk_query_stat_limits(int snum)
{
    mixed *skval = sk_skills[snum];

    return skval ? secure_var(skval[4]) : 0;
}

/*
 * Nazwa funkcji : sk_query_max
 * Opis          : Zwraca maksymalny dostepny tu poziom umiejetnosci.
 * Argumenty     : snum: Numer umiejetnosci.
 */
public int 
sk_query_max(int snum)
{
    return sk_trains[snum];
}

/*
 * Nazwa funkcji  : sk_query_limit
 * Opis           : Zwraca maksymalny dostepny dla gracza poziom umiejetnosci,
 *                  narzucany przez limitujaca ceche lub umiejetnosc.
 * Argumenty      : stat: Limitujaca cecha lub umiejetnosc w formacie
 *                        <tasks.h>.
 *                  lower: Poziom limitujacej cechy/umiejetnosci konieczny
 *                         do rozpoczecia szkolenia (0).
 *                  upper: Poziom limitujacej cechy/umiejetnosci konieczny
 *                         do wyszkolenia sie do maksimum (100).
 */
public nomask int
sk_query_limit(int stat, int lower, int upper)
{
    stat = (stat < 0 ? this_player()->query_stat(CONV_TS2SS(stat)) :
                       this_player()->query_skill(stat));

    return 100 * (stat - lower) / (upper - lower);
}

/*
 * Nazwa funkcji  : sk_query_cost
 * Opis           : Zwraca koszt treningu umiejetnosci.
 * Argumenty      : snum: Numer umiejetnosci.
 *                  from: Z jakiego poziomu.
 *                  to: Na jaki.
 * Uwaga          : Funkcja nie sprawdza, czy umiejetnosc istnieje, ani czy
 *                  podane poziomy maja sens.
 */
public nomask int
sk_query_cost(int snum, int from, int to)
{
    int cost = (to * to * to - from * from * from) *
               sk_query_cost_factor(snum) / 100;

    return cost ? cost : 1;
}

/*
 * Function name: sk_rank
 * Description:   Give the textual level of a skill
 * Arguments:     lev: The skill level
 * Returns:       The skill rank descriptions
 */
public nomask string
sk_rank(int level)
{
    if (level <= 0)
        return "brak umiejetnosci";

    if (--level > 99)
        level = 99;

    return desc[sizeof(desc) * level / 100];
}

/*
 * Nazwa funkcji : sk_query_train
 * Opis          : Zwraca tablice umiejetnosci, jakie mozna tu trenowac.
 */
public nomask int *
sk_query_train()
{
    return m_indices(sk_trains);
}

/*
 * Nazwa funkcji  : sk_find_skill
 * Opis           : Znajduje numer umiejetnosci na podstawie jej nazwy.
 * Argumenty      : skill: Biernik nazwy.
 * Funkcja zwraca : Numer odpowiadjacej umiejetnosci lub -1.
 */
public nomask int
sk_find_skill(string skill)
{
    int *skind = m_indices(sk_skills);
    int i = -1;
    int size = sizeof(skind);

    while (++i < size)
	if (sk_query_name(skind[i], 1) == skill)
	    return skind[i];

    return -1;
}

/*
 * Nazwa funkcji : init_skill_raise
 * Opis          : Dodaje w obiekcie komende sluzaca do trenowania
 *                 umiejetnosci. Funkcja powinna byc wywolywana w init().
 */
public nomask void
init_skill_raise()
{
    add_action(sk_improve, "trenuj");
}

/*
 * Nazwa funkcji : sk_fix_cost
 * Opis          : Zwraca odpowiednio sformatowana linijke z listy
 *                 dostepnych umiejetnosci.
 * Argumenty     : snum: Numer umiejetnosci.
 */
public string
sk_fix_cost(int snum)
{
    int this_level = this_player()->query_base_skill(snum);
    int cost;
    string money_unit;

    if (this_level < sk_query_max(snum))
    {
        cost = sk_query_cost(snum, this_level, this_level + 1);
        switch (cost)
        {
            case 1:
                money_unit = "miedziak";
                break;
            case 12..14:
                money_unit = "miedziakow";
                break;
            default:
                switch (cost % 10)
                {
                    case 2..4:
                        money_unit = "miedziaki";
                        break;
                    default:
                        money_unit = "miedziakow";
                }
        }
        return sprintf("  %-30s %13i %s\n", sk_query_name(snum, 0), cost,
                       money_unit);
    }

    return sprintf("  %-30s %13s\n", sk_query_name(snum, 0), "---");
}

/*
 * Nazwa funkcji  : sk_answer
 * Opis           : Funkcja formatuje odpowiedz, jaka ma byc udzielona
 *                  graczowi, aby gdy trenerem jest npc, zostala ona
 *                  wypowiedziana przez niego.
 * Argumenty      : Odpowiedz, jaka ma byc udzielona.
 */
public string
sk_answer(string answer)
{
    return living(this_object()) ? this_object()->query_Imie(this_player(),
           PL_MIA) + " odpowiada: " + answer + "\n" : answer + "\n";
}

/*
 * Nazwa funkcji  : sk_hook_unknown_skill
 * Opis           : Gracz usiluje trenowac nieznana umiejetnosc.
 * Argumenty      : skill: Umiejetnosc, jaka usilowal trenowac.
 * Funkcja zwraca : 0 lub 1.
 */
public int
sk_hook_unknown_skill(string skill)
{
    notify_fail(sk_answer("Obawiam sie, ze nie znam takiej umiejetnosci."));
    return 0;
}

/*
 * Nazwa funkcji  : sk_hook_untrained_skill
 * Opis           : Gracz usiluje trenowac nie nauczana tu umiejetnosc.
 * Argumenty      : snum: Numer umiejetnosci.
 * Funkcja zwraca : 0 lub 1.
 */
public int
sk_hook_untrained_skill(int snum)
{
    notify_fail(sk_answer("Obawiam sie, ze nie prowadze szkolen w tej "
                        + "dziedzinie."));
    return 0;
}

/*
 * Nazwa funkcji  : sk_hook_improved_max
 * Opis           : Gracz nie moze juz trenowac jakiejs umiejetnosci.
 * Argumenty      : snum: Numer umiejetnosci.
 *                  level: Obecny poziom gracza w tej umiejetnosci.
 * Funkcja zwraca : 0 lub 1.
 */
public int
sk_hook_improved_max(int snum, int level)
{
    notify_fail(sk_answer("Obawiam sie, ze w tej dziedzinie nie naucze cie "
                        + "juz nic nowego. " + (level == 100 ? "Osiagn"
                        + this_player()->koncowka("ales", "elas") + " juz "
                        + "absolutne mistrzostwo." : "Moze jednak uda ci sie "
                        + "znalezc innego mistrza...")));
    return 0;
}

/*
 * Nazwa funkcji : sk_add_in_prefix
 * Opis          : Dodaje przed argumentem odpowiedni dla niego przyimek:
 *                 'w' lub 'we', a przed nim jeszcze spacje.
 * Argumenty     : str: Rzeczownik w miejscowniku.
 */
public string
sk_add_in_prefix(string str)
{
    if (str[0] == 'w')
        switch (str[1])
        {
            case 'a':
            case 'e':
            case 'i':
            case 'o':
            case 'u':
            case 'y':
                return " w " + str;
            default:
                return " we " + str;
        }

    return " w " + str;
}

/*
 * Nazwa funkcji  : sk_hook_stat_limit_exceed
 * Opis           : Gracz usiluje trenowac umiejetnosc ponad limit z danej
 *                  cechy lub umiejetnosci.
 * Argumenty      : snum: Numer umiejetnosci.
 *                  level: Obecny poziom gracza w tej umiejetnosci.
 *                  stat: Limitujaca cecha lub umiejetnosc w formacie
 *                        <tasks.h>.
 * Funkcja zwraca : 0 lub 1.
 */
public int
sk_hook_stat_limit_exceed(int snum, int level, int stat)
{
    string statstr;

    switch (stat)
    {
        case TS_STR:
            statstr = "silniejsz" + this_player()->koncowka("y", "a");
            break;
        case TS_DEX:
            statstr = "zreczniejsz" + this_player()->koncowka("y", "a");
            break;
        case TS_CON:
            statstr = "wytrzymalsz" + this_player()->koncowka("y", "a");
            break;
        case TS_INT:
            statstr = "inteligentniejsz" + this_player()->koncowka("y", "a");
            break;
        case TS_WIS:
            statstr = "madrzejsz" + this_player()->koncowka("y", "a");
            break;
        case TS_DIS:
            statstr = "odwazniejsz" + this_player()->koncowka("y", "a");
            break;
        default:
            if (statstr = sk_query_name(stat, 2))
                statstr = "lepsz" + this_player()->koncowka("y", "a")
                        + sk_add_in_prefix(statstr);
            else
                throw("Illegal limiting stat: " + stat + ".\n");
    }

    notify_fail(sk_answer("Musisz byc nieco " + statstr + ", zanim bedziesz "
                        + "mogl" + this_player()->koncowka("", "a") + " sie "
                        + (level ? "dalej szkolic" : "szkolic") + " w tej "
                        + "dziedzinie."));
    return 0;
}

/*
 * Nazwa funkcji  : sk_hook_cant_pay
 * Opis           : Gracz ma za malo pieniedzy aby zaplacic za trening.
 * Argumenty      : snum: Numer umiejetnosci.
 *                  level: Obecny poziom gracza w tej umiejetnosci.
 *                  cost: Cena treningu na nastepny poziom.
 * Funkcja zwraca : 0 lub 1.
 */
public int
sk_hook_cant_pay(int snum, int level, int cost)
{
    notify_fail(sk_answer("Obawiam sie, ze masz za malo pieniedzy, by "
                        + "zaplacic za trening tej umiejetnosci."));
    return 0;
}

/*
 * Nazwa funkcji  : sk_hook_raise_rank
 * Opis           : Gracz wytrenowal umiejetnosc na wyzszy poziom.
 * Argumenty      : snum: Numer umiejetnosci.
 *                  from: Jaki poziom gracz mial poprzednio.
 *                  steps: Ile poziomow wytrenowal.
 *                  cost: Cena, jaka zaplacil.
 * Funkcja zwraca : 1.
 */
public int
sk_hook_raise_rank(int snum, int from, int steps, int cost)
{
    string skill = sk_add_in_prefix(sk_query_name(snum, 2));

    if (living(this_object()))
    {
        string str = (steps == 1 ? "" : " intensywnie");

        write(this_object()->query_Imie(this_player(), PL_MIA) + " szkoli cie"
            + str + skill + ".\n");
        saybb(QCIMIE(this_object(), PL_MIA) + " szkoli" + str + " "
            + QIMIE(this_player(), PL_BIE) + skill + ".\n");
    }
    else
    {
        string str = (steps == 1 ? "" : " intensywne");

        write("Przechodzisz" + str + " szkolenie" + skill + ".\n");
        saybb(QCIMIE(this_player(), PL_MIA) + " przechodzi" + str
            + " szkolenie" + skill + ".\n");
    }
    return 1;
}

/*
 * Nazwa funkcji : sk_hook_list_skills
 * Opis          : Wypisuje naglowek listy mozliwych to trenowania komend.
 */
public void
sk_hook_list_skills()
{
    write(sk_answer("Oto umiejetnosci, w jakich moge cie szkolic:"));
}

/*
 * Nazwa funkcji : sk_hook_write_header
 * Opis          : Wypisuje naglowek listy mozliwych to trenowania komend.
 */
public void
sk_hook_write_header()
{
    write("  Umiejetnosc:                    Koszt sesji treningowej:\n");
    write("------------------------------------------------------------\n");
}

/*
 * Nazwa funkcji  : sk_hook_no_more_improve
 * Opis           : Nie ma juz wiecej umiejetnosci do trenowania.
 * Funkcja zwraca : 0 lub 1.
 */
public int
sk_hook_no_more_improve()
{
    notify_fail(sk_answer("Obawiam sie, ze nie naucze cie juz nic nowego. "
                        + "Moze jednak uda ci sie znalezc innego "
                        + "mistrza..."));
    return 0;
}

/*
 * Nazwa funkcji : sk_filter_improve
 * Opis          : Czy gracz moze szkolic sie w danej umiejetnosci.
 * Argumenty     : snum: Numer umiejetnosci.
 */
public int
sk_filter_improve(int snum)
{
    return this_player()->query_base_skill(snum) < sk_query_max(snum);
}

/*
 * Nazwa funkcji  : sk_list
 * Opis           : Podaje graczowi liste dostepnych umiejetnosci.
 * Funkcja zwraca : 1
 */
int
sk_list()
{
    int *guild_sk = filter(sk_query_train(), &sk_filter_improve());
    int i = -1;
    int size = sizeof(guild_sk);

    if (!size)
        return sk_hook_no_more_improve();

    sk_hook_list_skills();
    sk_hook_write_header();

    while (++i < size)
        write(sk_fix_cost(guild_sk[i]));

    return 1;
}

/*
 * Nazwa funkcji : sk_improve
 * Opis          : Wywolywana, gdy gracz uzyje komendy 'trenuj'.
 * Argumenty     : str: Argument uzytej przez gracza komendy.
 */
public int
sk_improve(string str)
{
    int snum, level, steps, smax, cost;
    string skill;

#ifdef STAT_LIMITED_SKILLS
    mapping slimits;
    int *lstats, i, size, stat;
    mixed limit;
#endif

    if (environment(this_player()) != this_object() &&
        !CAN_SEE(this_player(), this_object()))
        return 0;

    if (!CAN_SEE_IN_ROOM(this_player()))
    {
        string prop = environment(this_player())->query_prop(ROOM_S_DARK_MSG);

        notify_fail((prop ? prop : "Jest zbyt ciemno")
                  + ", by moc trenowac umiejetnosci.\n");
        return 0;
    }

    if (living(this_object()) && (!CAN_SEE_IN_ROOM(this_object()) ||
        !CAN_SEE(this_object(), this_player())))
    {
        this_object()->unseen_hook();
        return 0;
    }

    if (!str || str == "intensywnie")
        return sk_list();

    if (sscanf(str, "intensywnie %s", skill))
        steps = 6 + random(5);	/* Intensywny trening: 6-10 poziomow */
    else
    {
        skill = str;
        steps = 1;
    }

    if ((snum = sk_find_skill(skill)) == -1)
        return sk_hook_unknown_skill(skill);

    if (!(smax = sk_query_max(snum)))
        return sk_hook_untrained_skill(snum);

    if ((level = this_player()->query_base_skill(snum)) >= smax)
        return sk_hook_improved_max(snum, level);
    else if (level + steps > smax)
        steps = smax - level;

#ifdef STAT_LIMITED_SKILLS
    slimits = sk_query_stat_limits(snum);
    lstats = m_indices(slimits);
    i = -1;
    size = sizeof(lstats);

    while (++i < size)
    {
        smax = (pointerp(limit = slimits[stat = lstats[i]]) ?
                sk_query_limit(stat, limit[0], limit[1]) :
                sk_query_limit(stat, 0, limit));

        if (level >= smax)
            return sk_hook_stat_limit_exceed(snum, level, stat);
        else if (level + steps > smax)
            steps = smax - level;
    }
#endif

    while (!MONEY_ADD(this_player(),
                     -(cost = sk_query_cost(snum, level, level + steps))))
        if (!--steps)
            return sk_hook_cant_pay(snum, level, cost);

    this_player()->set_skill(snum, level + steps);

    return sk_hook_raise_rank(snum, level, steps, cost);
}