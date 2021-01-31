/*
 * /cmd/live/state.c
 *
 * General commands for finding out a livings state.
 * The following commands are:
 *
 * - cechy
 * - czary
 * - email
 * - k(ondycja)
 * - kondycja
 * - opcje
 * - porownaj
 * - powroc (tylko w czasie Apokalipsy)
 * - poziomy
 * - przetrwaja
 * - stan
 * - um(iejetnosci)
 * - umiejetnosci
 */

#pragma no_inherit
#pragma save_binary
#pragma strict_types

inherit "/cmd/std/command_driver";

#include <cmdparse.h>
#include <composite.h>
#include <const.h>
#include <files.h>
#include <filter_funs.h>
#include <language.h>
#include <macros.h>
#include <ss_types.h>
#include <state_desc.h>
#include <std.h>
#include <stdproperties.h>
#include <time.h>
#include <options.h>
#include "/config/login/login.h"

#define SUBLOC_MISCEXTRADESC 	"_subloc_misc_extra"

/*
 * Global constants
 */
private mixed
	stat_strings, denom_strings, brute_fact, panic_state, 
	fatigue_state, intox_state, head_state, stuff_state, 
	soak_state, improve_fact, skillmap, compare_strings, health_state,
	mana_state;

private string *stat_names, *stat_names_bie, *enc_weight;

private mapping lev_map;

/*
 * Prototypes
 */
public varargs string get_proc_text(int num, mixed maindesc,
				    int turnpoint, mixed subdesc);

void
create()
{
    seteuid(getuid(this_object())); 
    
    /* These global arrays are created once for all since they are used
       quite often. They should be considered constant, so do not mess
       with them
    */

    skillmap = 		SS_SKILL_DESC;
    denom_strings = 	SD_STAT_DENOM;

    stat_names = 	SD_STAT_NAMES;
    stat_names_bie = 	SD_STAT_NAMES_BIE;

    stat_strings = 	({ ({ SD_STATLEV_STR("y"), SD_STATLEV_DEX("y"), 
    			      SD_STATLEV_CON("y"), SD_STATLEV_INT("y"), 
    			      SD_STATLEV_WIS("y"), SD_STATLEV_DIS("y"), }),
    			   ({ SD_STATLEV_STR("a"), SD_STATLEV_DEX("a"), 
    			      SD_STATLEV_CON("a"), SD_STATLEV_INT("a"), 
    			      SD_STATLEV_WIS("a"), SD_STATLEV_DIS("a") })
    			});

    compare_strings = 	({
			  ({ SD_COMPARE_STR("y"), SD_COMPARE_DEX("y"), 
			     SD_COMPARE_CON("y") }), ({ SD_COMPARE_STR("a"), 
			     SD_COMPARE_DEX("a"), SD_COMPARE_CON("a") }),
			  ({ SD_COMPARE_STR("e"), SD_COMPARE_DEX("e"), 
			     SD_COMPARE_CON("e") })
			}); 

    brute_fact = 	({ SD_BRUTE_FACT("y"), SD_BRUTE_FACT("a") });
    health_state = 	({ SD_HEALTH("y"), SD_HEALTH("a"), SD_HEALTH("e") });
    mana_state = 	({ SD_MANA("y"), SD_MANA("a") });
    panic_state = 	({ SD_PANIC("y"), SD_PANIC("a") });
    fatigue_state = 	({ SD_FATIGUE("y"), SD_FATIGUE("a") });
    stuff_state = 	({ SD_STUFF("y"), SD_STUFF("a") });
    intox_state = 	({ SD_INTOX("y"), SD_INTOX("a") });

    soak_state = 	SD_SOAK;
    head_state = 	SD_HEADACHE;
    improve_fact = 	SD_IMPROVE;
    enc_weight = 	SD_ENC_WEIGHT;
}

/* **************************************************************************
 * Return a proper name of the soul in order to get a nice printout.
 */
string
get_soul_id()
{
    return "state";
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
	     "cechy":"cechy",
	     "compare":"compare",
	     "czary":"czary",

	     "email":"email",

	     "h":"health",
	     "health":"health",
	     
	     "k":"kondycja",
	     "kondycja":"kondycja",

	     "levels":"levels",

	     "opcje":"opcje",
	     "odmien":"odmien",
	     "options":"options",
	     
	     "porownaj":"porownaj",
	     "powroc":"powroc",
	     "poziomy":"poziomy",
	     "przetrwaja":"przetrwaja",

	     "skills":"show_skills",
	     "spells":"show_spells",
	     "stan":"stan",
	     "stats":"show_stats",
	     
	     "um":"umiejetnosci",
	     "umiejetnosci":"umiejetnosci",

	     "v":"show_vitals",
	     "vitals":"show_vitals",
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
    live->add_subloc(SUBLOC_MISCEXTRADESC, file_name(this_object()));
    live->add_textgiver(file_name(this_object()));
}

public string
show_subloc_size(object on, object for_obj)
{
    string race, res, konc1, konc2;
    int val, rval, *proc, gender;

    race = on->query_race();
    gender = on->query_gender();
    konc1 = on->koncowka("y", "a");
    konc2 = (konc1 == "y" ? "i" : "a");

    if (member_array(race, RACES) >= 0)
    {
	val = on->query_prop(CONT_I_HEIGHT);
	rval = RACEATTR[race][gender][0];
	val = 100 * val / (rval ? rval : val);
	proc = HEIGHTPROC;
	
	rval = sizeof(proc) - 1;
	while (--rval >= 0)
	{
	    if (val >= proc[rval])
		break;
	}
	
	if (rval < 0)
	    rval = 0;
	    
	res = " " + HEIGHTDESC(konc2)[rval] + " i ";

	val = on->query_prop(CONT_I_WEIGHT) / on->query_prop(CONT_I_HEIGHT);
	rval = RACEATTR[race][gender][2];
	val = 100 * val / (rval ? rval : val);
	proc = WEIGHTPROC;

	rval = sizeof(proc) - 1;
	while (--rval >= 0)
	{
	    if (val >= proc[rval])
		break;
	}
	
	if (rval < 0)
	    rval = 0;

	res += WIDTHDESC(konc1)[rval] + " jak na " + 
	    on->query_rasa(PL_BIE) + ".\n";
    }
    else
	res = "";

    return res;
}

public string
show_subloc_fights(object on, object for_obj)
{
    object eob;

    eob = (object)on->query_attack();
    
    return (for_obj == on ? "Walczysz" : "Walczy") + 
	" z " + (eob == for_obj ? "toba" :
	 (string)eob->query_imie(for_obj, PL_NAR)) + ".\n";
}

public string
show_subloc_health(object on, object for_obj)
{
    int hp, mhp;

    hp = on->query_hp();
    mhp = on->query_max_hp();
    if (mhp == 0)
	mhp = 1;
	
    return get_proc_text((hp * 100) / mhp, (on == this_player() ?
	health_state[this_player()->koncowka(0, 1, 2)] : 
	health_state[on->koncowka(0, 1, 2)]), 0, ({}) );
}

/*
 * Function name: show_subloc
 * Description:   Shows the specific sublocation description for a living
 */
public string 
show_subloc(string subloc, object on, object for_obj)
{
    string res, cap_pronoun, cap_pronoun_verb, tmp;

    if (on->query_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS))
	return "";

    if (for_obj == on)
    {
	res = "Jestes";
	cap_pronoun_verb = res;
	cap_pronoun = "Jestes ";
    }
    else
    {
	res = "Jest";
	cap_pronoun_verb = res;
	cap_pronoun = "Zdaje sie byc ";
    }
    
    if (strlen(tmp = show_subloc_size(on, for_obj)))
	res += tmp;
    else
	res = "";

    if (on->query_attack())
	res += show_subloc_fights(on, for_obj);

    res += cap_pronoun + show_subloc_health(on, for_obj) + ".\n";

    return res;
}

/****************************************************************
 *
 * A textgiver must supply these functions
 */

/*
 * Returns true if this textgiver describes the given stat
 */
public int 
desc_stat(int stat)
{
    return (stat >= 0 && stat <= SS_NO_EXP_STATS);
}

/*
 * Returns true if this textgiver describes the given skill
 */
public int 
desc_skill(int skill)
{
    if (sizeof(skillmap[skill]))
	return 1;
    else
	return 0;
}

/*
 * Function name: query_stat_string
 * Description:   Gives text information corresponding to a stat
 * Parameters:    stat: index in stat array
 *                value: stat value (range 1..100)
 *                negative value gives the statstring with index corresponding
 *                to -value
 * Returns:       string corresponding to stat/value
 */
public string
query_stat_string(int stat, int value, int konc = 0)
{
    if (stat < 0 || stat >= SS_NO_EXP_STATS)
	return "";
    if (value < 0)
    {
	if (value < -sizeof(stat_strings[stat]))
	    value = 0;
	return stat_strings[konc][stat][-value];
    }
    /*
     * 17 levels on each stat, 8 descriptions
     */
    if (value > 135)
	value = 135;
    return stat_strings[konc][stat][sizeof(stat_strings[konc][stat]) * value / 136];
}

public varargs string
get_proc_text(int num, mixed maindesc, int turnpoint, mixed subdesc)
{
    int a, b, c, d,j;
    mixed subs;

    if (!sizeof(maindesc))
	return ">normal<";

    if (num < 0)
	num = 0;
    if (num > 99)
	num = 99;

    j = sizeof(maindesc) * num / 100;    
    
    if (!pointerp(subdesc))
	subs = denom_strings;
    else if (sizeof(subdesc))
	subs = subdesc;
    else
	return maindesc[j];

    a = num - (j * 100 / sizeof(maindesc));

    b = (sizeof(subs) * a * sizeof(maindesc)) / 100;

    if (j < turnpoint)
	b = (sizeof(subs) - 1) - b;

    return subs[b] + maindesc[j];
}

/* **************************************************************************
 * Here follows some support functions. 
 * **************************************************************************/

/*
 * Function name: compare_living
 * Description:   Support function to compare
 */
mixed
compare_living(int stat, object player1, object player2)
{
    int a, b, c, skill, seed1, seed2, swap;
    string konc;

    a  = player1->query_stat(stat);
    b  = player2->query_stat(stat);

    if (player1 != player2 )
    {
	skill = this_player()->query_skill(SS_APPR_MON);
	sscanf(OB_NUM(player1), "%d", seed1);
	sscanf(OB_NUM(player2), "%d", seed2);
	skill = 1000 / (skill + 1);
#if 0
	a += random(skill, seed1 + seed2 + 27 + stat);
	/* 27 is an arbitrarily selected constant */
	b += random(skill, seed1 + seed2 + stat);
#endif
	a += random(skill, seed1 + stat);
	b += random(skill, seed2 + stat);
    }

    if (a > b)
    {
	c = 100 - (80 * b) / a;
	konc = player1->koncowka(0, 1, 2);
    }
    else
    {
	c = 100 - (80 * a) / b;
	swap = 1;
	konc = player2->koncowka(0, 1, 2);
    }

    c = (c * sizeof(compare_strings[konc][stat]) / 100);
    if (c > 3)
	c = 3;
    return ({swap, compare_strings[konc][stat][c]});
}

/* **************************************************************************
 * Here follows the actual functions. Please add new functions in the 
 * same order as in the function name list.
 * **************************************************************************/

/*
 * cechy - Pokazuje cechy gracza, oraz jego brutalnosc.
 */
varargs int
cechy(string str)
{
    int a, i, j, konc = this_player()->koncowka(0, 1);
    float c, d;
    object ob;
    string s, s2, t;
    int *learn;

    if (!str)
    {
	ob = this_player();
	s = "Jestes ";

	a = ob->query_prop(PLAYER_I_LASTXP);
	j = ob->query_exp() - a;
        if (a <= 0)
        {
/*
            write("Your progress indicator was not working properly " +
                "and is now reset.\n");
 */
            ob->add_prop(PLAYER_I_LASTXP, ob->query_exp());
        }
        else if (j > 0)
	{
	    /* The progress is measured relatively to your current exp. If
	     * you gained more than 10% of the exp you had when you logged
	     * in, you get the maximum progress measure. There are a minimum
	     * and a maximum, though.
	     */
	    a /= 10;
	    if (a > SD_IMPROVE_MAX)
		a = SD_IMPROVE_MAX;
	    else if (a < SD_IMPROVE_MIN)
		a = SD_IMPROVE_MIN;

	    j /= (a / 100);
	    write("Poczynil" + (konc ? "as" : "es") +
		" " + get_proc_text(j, improve_fact, 0, ({ }) ) +
		" postepy, od momentu kiedy " + 
		(konc ? "weszlas" : "wszedles") + " do gry.\n");
	}
	else
	{
	    write("Nie poczynil" + (konc ? "as" : "es") +
		" zadnych postepow, od kiedy " +
		(konc ? "weszlas" : "wszedles") + " do gry.\n");
	}
    }
    else
    {
	if(!((ob = find_player(str)) && this_player()->query_wiz_level()))
	{
	    notify_fail("Ciekawscy jestesmy.\n");
	    return 0;
	}
	s = capitalize(str) + " jest ";
    }
    
    for (t = s, i = 0; i < SS_NO_EXP_STATS; i++)
    {
	s2 = query_stat_string(i, ob->query_stat(i), konc);

	if (i < SS_NO_EXP_STATS - 2) 
	    s2 += ", ";
	else if (i < SS_NO_EXP_STATS - 1)
	    s2 += " i ";
	else
	    s2 += ".";
	t += s2;
    }
    write(t + "\n");

    /* brutalfactor  
     */
    c = itof(ob->query_brute_exp());
    d = itof(ob->query_exp() - ob->query_exp_combat());
    j = 100 - ftoi(((100.0 * d) / ( c != 0 ? c : d)));
    write("Sadzac po " + (!str ? "twoim" : ob->query_zaimek(PL_DOP, 1)) +
	" dotychczasowym zachowaniu, " + (str ? "jest" : "jestes") + " " +
	get_proc_text(j, brute_fact[konc], 0, ({})) + ".\n");

    return 1;
}

/*
 * Function name: compare
 * Description:   compare stats of 2 living objects
 */
int
compare(string str)
{
    notify_fail("Komenda 'compare' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'porownaj'.\n");

    return 0;
}

/*
 * czary - Pokazuje znane czary
 */
int
czary()
{
    int i;
    object *ob_list;

    ob_list = (object *)this_player()->query_spellobjs();

    for (i = 0 ; i < sizeof(ob_list) ; i++)
	ob_list[i]->list_spells();

    return 1;
}

/*
 * email - Pokaz/zmien swojego emaila.
 */
int
email(string str)
{
    if (!stringp(str))
    {
	write("Twoj aktualny adres email wyglada nastepujaco:\n" +
	    "\t" + this_player()->query_mailaddr() + "\n");
	return 1;
    }

    this_player()->set_mailaddr(str);
    write("Adres email zmieniony.\n");
    return 1;
}

/*
 * health - Display your health or that of someone else.
 */
int
health(string str)
{
    notify_fail("Komenda 'health' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'kondycja', lub prosciej - 'k'.\n");

    return 0;
}

/*
 * kondycja - Wyswietla kondycje wlasna lub innej osoby.
 */
int
kondycja(string str)
{
    object enemy;
    object *oblist;
    int index;
    int size;
    int display_self = 0;

    switch (str ?: "")
    {
	case "":
	    display_self = 1;
	    oblist = ({});
	    break;

	case "przeciwnika":
	    if (!(enemy = this_player()->query_attack()))
	    {
		notify_fail("Z nikim nie walczysz.\n");
		return 0;
	    }
	    else if (!CAN_SEE_IN_ROOM(this_player()) ||
		     !CAN_SEE(this_player(), enemy))
	    {
		notify_fail("Nie widzac swojego przeciwnika nie mozesz "
		          + "ocenic jego kondycji.\n");
		return 0;
	    }

	    oblist = ({enemy});
	    break;

	case "druzyny":
	    if (!sizeof(oblist = this_player()->query_team_others()))
		write("Nie jestes w zadnej druzynie.\n");
	    else if (!CAN_SEE_IN_ROOM(this_player()))
	    {
		write("Nie widzisz przy sobie zadnego z czlonkow swojej "
		    + "druzyny.\n");
		oblist = ({});
	    }
	    else if (!sizeof(oblist =
		FILTER_CAN_SEE(FILTER_PRESENT_LIVE(oblist), this_player())))
		write("Nie widzisz przy sobie zadnego z czlonkow swojej "
		    + "druzyny.\n");

	    display_self = 1;
	    break;

	case "wszystkich":
	    display_self = 1;
	    /* Intentionally no "break". We need to catch "default" too. */

	default:
	    oblist = parse_this(str, "%l:" + PL_DOP);
	    if (!sizeof(oblist) && !display_self)
	    {
		notify_fail("Czyja kondycje chcesz poznac?\n");
		return 0;
	    }
    }

    if (display_self)
	write("Jestes " + show_subloc_health(this_player(), this_player())
	    + ".\n");

    index = -1;
    size = sizeof(oblist);

    while(++index < size)
	write(oblist[index]->query_Imie(this_player(), PL_MIA) + " jest "
	    + show_subloc_health(oblist[index], this_player()) + ".\n");

    return 1;
}

/*
 * levels - Print a summary of different levels of different things
 */
public int
levels(string str)
{
    notify_fail("Komenda 'levels' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'poziomy'.\n");

    return 0;
}

/* **************************************************************************
 * opcje - Wlacz / wylacz / obejrzyj jakas opcje
 */
public nomask int
opcje(string arg)
{
    string	*args, rest;
    int		wi, plec, opt;
    
    plec = ((this_player()->query_gender() != G_MALE) ? 1 : 0);

    if (!stringp(arg))
    {
	opcje("wysokosc_ekranu");
	opcje("szerokosc_ekranu");
	opcje("krotkie_opisy");
	opcje("echo");
	opcje("uciekaj_przy");
	opcje("opisuj_walki");
//        opcje("walka_bez_broni");
	return 1;
    }
    
    args = explode(arg, " ");

    if (sizeof(args) == 1)
    {
	switch(arg)
	{
	case "wysokosc":
	case "wysokosc_ekranu":
	    write("Wysokosc ekranu:\t\t" + 
		this_player()->query_option(OPT_MORE_LEN) + "\n");
	    break;

	case "szerokosc":
	case "szerokosc_ekranu":
	    write("Szerokosc ekranu:\t\t" + 
		 this_player()->query_option(OPT_SCREEN_WIDTH) + "\n");
	    break;

	case "krotkie":
	case "krotkie_opisy":
	    write("Krotkie opisy lokacji:\t\t" + 
		(this_player()->query_option(OPT_BRIEF) ? "Wlaczone" : 
		    "Wylaczone") + "\n");
	    break;	

	case "echo":
	    write("Echo komend:\t\t\t" + 
		(this_player()->query_option(OPT_ECHO) ? "Wlaczone" : 
		    "Wylaczone") + "\n");
	    break;

	case "uciekaj_przy":
	case "uciekaj":
//	    wi = this_player()->query_option(OPT_WHIMPY);
	    wi = this_player()->query_whimpy();
	    write("Uciekaj przy:\t\t\t'");
	    if (wi)
	    {
		wi = wi * sizeof(health_state[plec]) / 100;
		write(capitalize(health_state[plec][wi]) + "'\n");
	    }
	    else
		write("Nigdy'\n");
	    break;
	    
	case "opisuj":
	case "opisywanie":
	case "opisywanie_walki_innych":
	case "opisuj_walki":
	    write("Opisywanie walki innych:\t" + 
		(this_player()->query_option(OPT_BLOOD) ? "Wylaczone" : 
		    "Wlaczone") + "\n");
	    break;

/*        case "walka":
	case "walka_bez_broni":
	    write("Walka bez broni:\t\t" + 
		(this_player()->query_option(OPT_UNARMED_OFF) ? 
		"Wylaczona" : "Wlaczona") + "\n");
	    break; */

	default:
	    return notify_fail("Nie ma takiej opcji.\n");
	    break;
	}
	return 1;
    }

    rest = implode(args[1..], " ");
    
    switch(args[0])
    {
    case "wysokosc":
    case "wysokosc_ekranu":
	if (!this_player()->set_option(OPT_MORE_LEN, atoi(args[1])))
	    return notify_fail("Blad skladniowy: " +
		"Wysokosc ekranu powinna miescic sie w przedziale 1 - 150.\n");
	opcje("wysokosc_ekranu");
	break;

    case "szerokosc":
    case "szerokosc_ekranu":
	if (!this_player()->set_option(OPT_SCREEN_WIDTH, atoi(args[1])))
	    return notify_fail("Blad skladniowy: " +
		"Szerokosc ekranu powinna miescic sie w przedziale 10 - 200.\n");
	opcje("szerokosc_ekranu");
	break;
	
    case "krotkie":
    case "krotkie_opisy":
	if (args[1] == "wlacz" || args[1] == "+")
	    this_player()->set_option(OPT_BRIEF, 1);
	else if (args[1] == "wylacz" || args[1] == "-")
	    this_player()->set_option(OPT_BRIEF, 0);
	else
	{
	    write("Nie rozumiem. Mam wlaczyc czy wylaczyc ?\n");
	    break;
	}
	
	opcje("krotkie_opisy");
	break;

    case "echo":
	if (args[1] == "wlacz" || args[1] == "+")
	    this_player()->set_option(OPT_ECHO, 1);
	else if (args[1] == "wylacz" || args[1] == "-")
	    this_player()->set_option(OPT_ECHO, 0);
	else
	{
	    write("Nie rozumiem. Mam wlaczyc czy wylaczyc ?\n");
	    break;
	}
	
	opcje("echo");
	break;

    case "uciekaj_przy":
    case "uciekaj":
	if (args[1] == "nigdy")
	{
//	    this_player()->set_option(OPT_WHIMPY, 0);
	    this_player()->set_whimpy(0);
	}	    
	else if (args[1] == "?")
	    write("nigdy, " + implode(health_state[plec], ", ") + "\n");
	else
	{
	    wi = member_array(rest, health_state[plec]);
	
	    if (wi < 0)
	    {
		notify_fail("Nie ma takiego poziomu kondycji. Dostepne to:\n" +
		    break_string(COMPOSITE_WORDS(health_state[plec]) + ".", 
		    70, 3) + "\n");
		return 0;
	    }

	    wi = (100 * (wi + 1)) / sizeof(health_state[plec]);
	    if (wi > 99)
		wi = 99;

//	    this_player()->set_option(OPT_WHIMPY, wi);
	    this_player()->set_whimpy(wi);
	}
	opcje("uciekaj");
	break;

    case "opisywanie":
    case "opisywanie_walki_innych":
    case "opisuj":
    case "opisuj_walki":
	if (args[1] == "wlacz" || args[1] == "+")
	    this_player()->set_option(OPT_BLOOD, 0);
	else if (args[1] == "wylacz" || args[1] == "-")
	    this_player()->set_option(OPT_BLOOD, 1);
	else
	{
	    write("Nie rozumiem. Mam wlaczyc czy wylaczyc ?\n");
	    break;
	}
	
	opcje("opisuj_walki");
	break;

/*
    case "walka":
    case "walka_bez_broni":
	if (args[1] == "wlacz" || args[1] == "+")
	    this_player()->set_option(OPT_UNARMED_OFF, 0);
	else if (args[1] == "wylacz" || args[1] == "-")
	    this_player()->set_option(OPT_UNARMED_OFF, 1);
	else
	{
	    write("Nie rozumiem. Mam wlaczyc czy wylaczyc ?\n");
	    break;
	}

	this_player()->update_procuse();

	opcje("walka_bez_broni");
	break;
*/

    default:
	return notify_fail("Nie ma takiej opcji.\n");
	break;
    }
    return 1;
}

/* **************************************************************************
 * odmien - Sprawdz odmiane jakiegos obiektu / jakiegos gracza.
 */
nomask int
odmien(string str)
{
    object *cele = parse_this(str, "%i:" + PL_MIA);
    object ob;
    string *odmiana = ({});
    int i = -1;

    if (!sizeof(cele))
    {
	if (str == this_player()->query_real_name())
	    cele = ({ this_player() });
	else
	{
	    notify_fail("Odmien <kto/co>?\n");
	    return 0;
	}
    }
    else if (sizeof(cele) > 1)
    {
	notify_fail("Nie mozesz odmieniac jednoczesnie "
		  + COMPOSITE_DEAD(cele, PL_DOP) + "\n");
	return 0;
    }

    ob = cele[0];

    if (!living(ob))
	while (++i <= PL_MIE)
	    odmiana += ({ ob->query_nazwa(i) });
    else if (this_player()->query_met(ob))
	while (++i <= PL_MIE)
	    odmiana += ({ ob->query_met_name(i) });
    else
	while (++i <= PL_MIE)
	    odmiana += ({ ob->query_nonmet_name(i) });

    write(capitalize(ob->short(this_player(), PL_MIA)) + " odmienia" + 
	(ob->query_tylko_mn() ? "ja" : "") + " sie nastepujaco:\n" +
	"  Mianownik: " + odmiana[0] + ",\n" +
	" Dopelniacz: " + odmiana[1] + ",\n" +
	"   Celownik: " + odmiana[2] + ",\n" +
	"    Biernik: " + odmiana[3] + ",\n" +
	"  Narzednik: " + odmiana[4] + ",\n" +
	"Miejscownik: " + odmiana[5] + ".\n");

    return 1;
}

/* **************************************************************************
 * options - Change/view the options
 */
nomask int
options(string arg)
{
    notify_fail("Komenda 'options' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'opcje'.\n");

    return 0;
}


/*
 * Function name: porownaj
 * Description:   porownuje cechy dwoch istot zyjacych
 */
int
porownaj(string str)
{
    int i, me;
    string stat, name1, name2, *slowa;
    mixed p1, p2;
    object pl1, pl2, *ob_list;
    mixed *cstr;

    notify_fail("Prawidlowa skladnia:\n" +
    		"\t'porownaj <ceche> z <osoba>' lub tez\n" +
    		"\t'porownaj <ceche> <osoby I> i <osoby II>'\n");
    if (!str)
	return 0;

    if (!CAN_SEE_IN_ROOM(this_player()))
    {
	notify_fail("Ty przeciez nic nie widzisz!\n");
	return 0;
    }

    slowa = explode(str, " ");
    if (sizeof(slowa) < 3)
	return 0;

    ob_list = FILTER_LIVE(all_inventory(environment(this_player())));
    ob_list = FILTER_CAN_SEE(ob_list, this_player());

    stat = slowa[0];
    
    for (i = 0;;i++)
    {
	if (i > 5)
	{
	    notify_fail("Nie ma takiej cechy. Masz do wyboru: " +
		COMPOSITE_WORDS(stat_names_bie) + ".\n");
	    return 0;
	}
	if ((stat == lower_case(stat_names[i])) ||
	    (stat == stat_names_bie[i]))
	   break;
    }
    
    if (i == SS_INT || i == SS_WIS || i == SS_DIS)
    {
	write("Wiesz, to troche ciezko okreslic na pierwszy rzut "+
	    "oka.\n");
	return 1;
    }
    
    if (slowa[1] == "z" || slowa[1] == "ze")
    {
	str = implode(slowa[2..], " ");
	
	if (!parse_command(str, ob_list, "%l:" + PL_NAR, p2))
	    return 0;
	p2 = NORMAL_ACCESS(p2, 0, 0);
	
	if (!sizeof(p2))
	    return 0;
	    
	if (sizeof(p2) > 1)
	{
	    notify_fail("Nie mozesz porownywac cech z wiecej, niz jedna "+
		"osoba na raz.\n");
	    return 0;
	}
	
	this_player()->set_obiekty_zaimkow(p2);
	p2 = p2[0];
	p1 = this_player();
    }
    else
    {
	str = implode(slowa[1..], " ");
	
	if (!parse_command(str, ob_list, "%l:" + PL_DOP + " 'i' %l:" + PL_DOP,
		p1, p2))
	    return 0;

	p1 = NORMAL_ACCESS(p1, 0, 0);
	p2 = NORMAL_ACCESS(p2, 0, 0);

	if (!sizeof(p2) || !sizeof(p1))
	    return 0;
	    
	if ((sizeof(p1) > 1) || (sizeof(p2) > 1))
	{
	    notify_fail("Mozesz porownywac cechy tylko jednej osoby z " +
		"druga.\n");
	    return 0;
	}
	
	if (p1 == p2)
	{
	    notify_fail("Chyba nie chcesz porownywac cech tej samej osoby?\n");
	    return 0;
	}

	this_player()->set_obiekty_zaimkow(p1, p2);
	p1 = p1[0];
	p2 = p2[0];
    }

    cstr = compare_living(i, p1, p2);
    
/*
 * He he he, czadowo to napisalem, nie? Juz po 5 minutach nie wiem o
 * co chodzi :-)) TO jest przyklad, jak kod _nie_ powinien wygladac.
 * /Alvin
 */
    write((random(2) ? "Wydaje ci sie, ze " : "Masz wrazenie, ze ") + 
	(p1 == this_player() ? (!cstr[0] ? "jestes " : "jest ") : 
	(!cstr[0] ? p1->query_imie(this_player(), PL_MIA) :
	p2->query_imie(this_player(), PL_MIA)) + " jest ") +
	cstr[1] + " " + (cstr[0] ? (p1 == this_player() ? "ty" :
	p1->query_imie(this_player(), PL_MIA)) : p2->query_imie(this_player(), 
	PL_MIA)) + ".\n");

    return 1;
}

int
powroc(string str)
{
    if (!ARMAGEDDON->shutdown_active())
	return 0; // Udajemy, ze nas nie ma.

    if (str == "do domu")
    {
	ARMAGEDDON->send_me_home();
	return 1;
    }

    notify_fail("Jesli chcesz powrocic w miejsce, z ktorego startujesz, "
	      + "napisz po prostu 'powroc do domu'.\n");
    return 0;
}

/*
 * poziomy - Wypisuje poziomy roznych rzeczy
 */
public int
poziomy(string str)
{
    string *ix, *levs;

    if (!mappingp(lev_map))
	lev_map = SD_LEVEL_MAP;

    ix = m_indexes(lev_map);

    if (!str)
    {
	notify_fail("Dostepne listy poziomow:\n" + 
		    break_string(COMPOSITE_WORDS(sort_array(ix)) + ".", 70, 3)
		  + "\n");
	return 0;
    }
    levs = lev_map[str];
    if (!sizeof(levs))
    {
	notify_fail("Nie ma takiej listy poziomow. Oto dostepne:\n" + 
		    break_string(COMPOSITE_WORDS(sort_array(ix)) + ".", 70, 3)
		  + "\n");
	return 0;
    }	
    write("Dostepne poziomy " + str + ":\n" +
	  break_string(COMPOSITE_WORDS(levs) + ".", 70, 3) + "\n");
    return 1;
}

/*
 * przetrwaja - Pokazuje, ktore przedmioty przetrwaja apokalipse
 */
public int
przetrwaja(string str)
{
    object *ob;
    int i, size;
    
    if (strlen(str))
    {
	notify_fail("Komenda 'przetrwaja' nie przyjmuje zadnych " +
	    "argumentow.\n");
	return 0;
    }
    
    ob = deep_inventory(this_player());

    i = -1;
    size = sizeof(ob);
    while(++i < size)
    {
	if (!ob[i]->check_seen(this_player()))
	    continue;
	    
	if (ob[i]->check_recoverable(1))
	{
	    if (ob[i]->may_not_recover())
		    write(capitalize(ob[i]->short(this_player(), PL_MIA)) + 
		        " nie przetrwa" +
		        (ob[i]->query_tylko_mn() ? "ja" : "") + 
		        " Apokalipsy.\n");
	    else
	    {
		if (!strlen(ob[i]->query_recover()))
		    continue;
		write(capitalize(ob[i]->short(this_player(), PL_MIA)) + 
		    " przetrwa" + (ob[i]->query_tylko_mn() ? "ja" : "") + 
		    " Apokalipse.\n");
	    }
	}
    }

    return 1;
}

/*
 * spells - Show what spells we know
 */
/*
 * Function name: show_spells
 * Description:   List the active spells.
 */
int
show_spells()
{
    notify_fail("Komenda 'spells' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'czary'.\n");

    return 0;
}

/*
 * vitals - Give vital state information about the living
 */
/*
 * Function name: show_vitals
 * Description:   Gives information on misc items
 */
varargs int
show_vitals(string str)
{
    notify_fail("Komenda 'vitals' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'stan'.\n");

    return 0;
}

/*
 * stan - Podaje informacje o stanie gracza
 */
varargs int
stan(string str)
{
    int a, b, i, j, konc;
    object ob;
    string s, s2, s3, s4, t;

    if (!str)
    {
	ob = this_player();
    }
    else
    {
	if(!((ob = find_player(str)) && this_player()->query_wiz_level()))
	{
	    notify_fail("Ciekawscy jestesmy.\n");
	    return 0;
	}
    }

    konc = ob->koncowka(0, 1);

    /* height, width, race
     */

    /* Hitpoints and mana
     */
    a = (ob->query_hp() * 100) / ob->query_max_hp();
    t = get_proc_text(a, health_state[konc], 0, ({}));
    b = (ob->query_mana() * 100) / ob->query_max_mana();

    write("Jestes fizycznie " + t + ", zas mentalnie " +
	  get_proc_text(b, mana_state[konc], 0, ({})) + ".\n");

    /* panic
     */
    a = (10 + (int)ob->query_stat(SS_DIS) * 3);
    b = ob->query_panic();
    a = 100 * b / (a != 0 ? a : b);
    t = get_proc_text(a, panic_state[konc], 2) + " i jestes ";

    /* fatigue
     */
    a = ob->query_max_fatigue();
    b = a - ob->query_fatigue();
    a = 100 * b / (a != 0 ? a : b);
    t += get_proc_text(a, fatigue_state[konc], 0, ({})) + ".";

    write("Czujesz sie " + t + "\n");
    
    /* soaked
     */
    a = ob->query_prop(LIVE_I_MAX_DRINK);
    b = ob->query_soaked();
    a = 100 * b / (a != 0 ? a : b);
    t = get_proc_text(a, soak_state, 0, ({})) + " i ";
    
    /* stuffed
     */
    a = ob->query_prop(LIVE_I_MAX_EAT);
    b = ob->query_stuffed();
    a = 100 * b / (a != 0 ? a : b);
    t += "jestes " + get_proc_text(a, stuff_state[konc], 1) + ".";


    write(capitalize(t) + "\n");

    /* intox
     */
    a = ob->query_prop(LIVE_I_MAX_INTOX);
    b = ob->query_intoxicated();
    a = 100 * b / (a != 0 ? a : b);
    if (b)
	write("Jestes " + get_proc_text(a, intox_state[konc], 0, ({})) + ".\n");
    else
    {
	/* headache
	 */
	a = ob->query_prop(LIVE_I_MAX_INTOX);
	b = ob->query_headache();
	a = 100 * b / (a != 0 ? a : b);
	if (b)
	{
	    if (catch(s2 = get_proc_text(a, head_state, 1)))
		SECURITY->log_error("zero_headache.", "GPT: " +
		    a + ", " + implode(head_state, ";") + ".\n");
	    write("Jestes trzezw" + (konc ? "a" : "y") +
		", ale masz " + s2 + 
		" kaca.\n");
	}
	else
	    write("Jestes trzezw" + (konc ? "a" : "y") + ".\n");
    }

#if 0
    /*
     * Alignment
     */
    write(s + ob->query_align_text() + ".\n");
#endif

    /*
     * Carry
     */
    a = ob->query_encumberance_weight();
    if (a >= 20)
    {
	write(capitalize(get_proc_text((a - 20) * 100 / 75, enc_weight, 0, 
	    ({ }) )) + ".\n");
    }

    /*
     * Age
     */
    write("Wiek: " + CONVTIME(ob->query_age() * 2) + ".\n");

    return 1;
}

/*
 * Function name: show_stats
 * Description:   Gives information on the stats
 */
varargs int
show_stats(string str)
{
    notify_fail("Komenda 'stats' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'cechy'.\n");

    return 0;
}

/*
 * skills - Give information on a livings skills
 */
/*
 * Function name: show_skills
 * Description:   Gives information on the stats
 */
varargs int
show_skills(string str)
{
    notify_fail("Komenda 'skills' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'umiejetnosci' lub prosciej 'um'.\n");

    return 0;
}

/*
 * [um]iejetnosci - Zwraca informacje o umiejetnosciach gracza.
 */
varargs int
umiejetnosci(string str)
{
    int il, wr, i1, i2, num;
    object ob;
    int *sk, ski;
    mapping skdesc;
    string s, s1, s2 = "";

    wr = 1;
    skdesc = SS_SKILL_DESC;

    if (!stringp(str))
    {
	ob = this_player();
    }
    else
    {
	if (sscanf(str, "%s %s", s1, s2) != 2)
	{
	    if (find_player(str))
	    {
		s1 = str;
		s2 = "";
	    } else {
		s1 = this_player()->query_real_name();
		s2 = str;
	    }
	}

	if (!(ob = find_player(s1)))
	{
	    notify_fail("Nie ma takiego gracza.\n");
	    return 0;
	}

	if (!this_player()->query_wiz_level() && ob != this_player())
	{
	    notify_fail("Ciekawscy jestesmy.\n");
	    return 0;
	}
    }
    sk = ob->query_all_skill_types();

    SKILL_LIBRARY->sk_init();

    i1 = 0;
    i2 = 9999999;
    
    switch (s2)
    {
    	case "ogolne": i1 = 100; i2 = 200; break;
    	case "bojowe": i1 = 0; i2 = 29; break;
    	case "magiczne": i1 = 30; i2 = 49; break;
    	case "zlodziejskie": i1 = 50; i2 = 69; break;
	case "": i1 = 0; i2 = 9999999; break;
    	default:
	    notify_fail("Nieznana kategoria. Masz do wyboru: ogolne, " +
		"bojowe, magiczne, zlodziejskie.\n");
	    return 0;
    }

    for (il = 0; il < sizeof(sk); il++)
    {
	ski = sk[il];
	if (ski < i1 || ski > i2)
	    continue;

	if (!(num = ob->query_skill(sk[il])))
	{
	   ob->remove_skill(sk[il]);
	    continue;
	}
	if (pointerp(skdesc[sk[il]]))
	    str = skdesc[sk[il]][0];
	else {
	    str = ob->query_skill_name(sk[il]);
	    if (!strlen(str))
		continue;
	}
	if (++wr % 2)
	  {
	    write(sprintf("%-18s %s", str + ":",
		SKILL_LIBRARY->sk_rank(num)));
	    write("\n");
	  }
	else
	  {
	    s1 = sprintf("%-18s %s ", str + ":",
		SKILL_LIBRARY->sk_rank(num)); 
	    write(sprintf("%-40s ", s1));
	  }
    }
    if (wr)
	write("\n");
    else
	write("Nie posiadasz zadnych umiejetnosci.\n");

    return 1;
}
