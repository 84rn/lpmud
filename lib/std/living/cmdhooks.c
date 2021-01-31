/*
 * /std/living/cmdhooks.c
 *
 * This is a subpart of /std/living.c
 *
 * All command hooks are handled here, wiz, soul, tool and spell commands.
 */

#include <cmdparse.h>
#include <login.h>
#include <macros.h>
#include <std.h>
#include <options.h>

/*
 * Variables, These are only accessed from routines in this module.
 */
static private mapping com_sounds;
static string 	*wiz_souls,		/* The wizard soul names */
		*soul_souls,		/* The ordinary soul names */
		*tool_souls,		/* The tool soul names */
		say_string,             /* The last message said */
		gSpell,		        /* The last spell cast. */
		gArg;			/* The argument to that spell. */
		
static mixed	ob_zaimkow;

static int	gTime,			/* The time left for the spell to */
					/* get realized. */
                aid,                    /* Alarm id for spells */
                bit_zaimkow,		/* Mapa bitowa na potrzeby zaimkow */
                *r_zaimkow;		/* Tablica z rodzajami zaimkow. */
                

/*
 * Prototypes
 */
public string race_sound();
public void update_hooks();
public varargs int communicate(string str = "");
static int my_commands(string str);
void cmdhooks_do_spell(string spell, string sparg, int dtime, object spellob);
public void set_obiekty_zaimkow(object *ob, object *ob2 = 0);

#define REOPEN_SOUL_ALLOWED ([ "exec_done_editing" : WIZ_CMD_NORMAL, \
			       "pad_done_editing"  : WIZ_CMD_NORMAL, \
			       "do_many_delayed"   : WIZ_CMD_NORMAL, \
                               "tail_input_player" : WIZ_CMD_APPRENTICE ])
#define REOPEN_SOUL_RELOAD  "_reloaded"

/*
 * Function name: cmdhooks_reset
 * Description  : Start the command parsing. The last added action is
 *                evaluated first, so speech is checked first.
 */
static void
cmdhooks_reset()
{
    update_hooks();

    add_action(my_commands, "", 1);
    add_action(communicate, "'", 2);
    add_action(communicate, "powiedz");
    add_action("stary_say", "say");

    /* Get the different race-sounds. */
    if (!mappingp(com_sounds = RACESOUND[query_race()]))
    {
    	com_sounds = ([ ]);
    }
    
    r_zaimkow = allocate(2);
    ob_zaimkow = ({ ({ }), ({ }), ({ }), ({ }), ({ }) });
}

public int
stary_say()
{
    notify_fail("Komenda 'say' zostala wycofana. Zamiast " +
        "niej mozesz uzyc 'powiedz', albo prosciej - samego znaku apostrofu.\n");

    return 0;
}

/*
 * Function name: communicate_to
 * Description  : This function is called whenever the player starts his
 *                say-string with 'to'. This usually indicates that the
 *                player wants to say something to some people in particular.
 *                All others will still hear it though.
 * Arguments    : string str - the text to say (not including 'to').
 * Returns      : int 1/0 - success/failure.
 */
public int
communicate_to(string str)
{
    object *oblist;
    string r_sound;
    string qcomp;
    int ix;

    /* We must parse the lower case of the string 'str' since parse_command
     * does not find capitalized player names, so it would not trigger on
     * "say to Mercade Greetings!" However, since we want to keep the
     * capitals in the said text, we store the original text in the variable
     * 'say_string' and use that later.
     */
    if (!parse_command(lower_case(str), environment(this_player()),
	"'do' %l:" + PL_DOP + " %s", oblist, str))
    {
	return 0;
    }

    oblist = NORMAL_ACCESS(oblist, 0, 0) - ({ this_player(), 0 });
    if (!sizeof(oblist) ||
	!strlen(str))
    {
	return 0;
    }

    /* In order to be able to use QCOMPLIVE later, we have to query the
     * COMPOSITE_LIVE macro now, even though the player may not be using
     * echo.
     */
    qcomp = COMPOSITE_LIVE(oblist, PL_DOP);

    say_string = extract(say_string, -(strlen(str)));
    if (this_player()->query_option(OPT_ECHO))
    {
	write(capitalize(this_object()->actor_race_sound()) + " do " +
	    qcomp + ": " + say_string + "\n");
    }
    else
    {
	write("Ok.\n");
    }

#if 0
    say(QCIMIE(this_object(), PL_MIA) +
	(r_sound = (" @@race_sound:" + file_name(this_object()) + "@@ do ")) +
	QCOMPLIVE(PL_DOP) + ": " + say_string + "\n", 
	(oblist + ({ this_player() }) ));
    oblist->catch_msg(QCIMIE(this_object(), PL_MIA) + r_sound + "ciebie: " +
	say_string + "\n");
#endif

    say(sprintf("@@vbfc_say_to:%s@@: %s\n", file_name(this_object()),
	say_string), (oblist + ({ this_player() }) ));
    ix = sizeof(oblist);
    while (--ix >= 0)
    {
	if (CAN_SEE_IN_ROOM(oblist[ix]) && CAN_SEE(oblist[ix], this_object()))
	    oblist[ix]->catch_msg(sprintf("%s %s do ciebie: %s\n",
		query_Imie(oblist[ix], PL_MIA),
		(com_sounds[oblist[ix]->query_race()] ?: "mowi"),
		say_string));
	else
	    oblist[ix]->catch_msg(sprintf("Glos %s %s: %s",
		(oblist[ix]->query_met(this_object())
		    ? query_met_name(PL_DOP)
		    : query_rasa(PL_DOP)),
		(com_sounds[oblist[ix]->query_race()] ?: "mowi"), say_string));
	oblist[ix]->catch_say_to(say_string);
    }

    set_obiekty_zaimkow(oblist);

    return 1;
}

/*
 * Function name: communicate
 * Description  : This function is called whenever the player wants to say
 *                something. We have to put it here rather than in a soul
 *                because people can use the single quote ' as alias for
 *                say.
 * Arguments    : string str - the command line argument.
 * Returns      : int 1/0 - success/failure.
 */
public varargs int
communicate(string str = "")
{
    mixed tmp;
    
    if (!strlen(str))
    {
        write("W ostatniej chwili decydujesz sie nic nie mowic.\n");
        saybb(QCIMIE(this_player(), PL_MIA) + " wyglada jakby chcial" +
            this_player()->koncowka("", "a", "o") + " cos powiedziec, " +
            "ale w ostatniej chwili sie rozmyslil" + 
            this_player()->koncowka("", "a", "o") + ".\n");
            
        return 1;
    }
    
    if (tmp = query_prop(LIVE_M_MOUTH_BLOCKED))
    {
	write(stringp(tmp) ? tmp : "Nie mozesz z siebie wydobyc glosu.\n");
	return 1;
    }

    /* We do not want people to add too many spaces and use the say command
     * as a way to generate emotions themselves. However, we do not want to
     * waste this on wizards and we also test whether people haven't used
     * too many spaces. You cannot make an emotion with only a few. This
     * wildmatch is 40% faster than the explode/implode stuff, so as long
     * as people don't use 8 spaces more than 40% of the time, this check
     * pays itself back.
     */
#if 0
    if (!query_wiz_level() &&
    	wildmatch("*       *", str))
    {
	str = implode((explode(str, " ") - ({ "" }) ), " ");
    }
#endif

    /* This is a test for the command 'say to'. If it fails, we just default
     * to the normal say.
     */
    say_string = str;
    
/*
    if (wildmatch("do *", str))
    {
*/
	if (communicate_to(str))
	{
	    return 1;
	}
/*
    }
*/

    if (this_player()->query_option(OPT_ECHO))
    {
	write(capitalize(this_object()->actor_race_sound()) + ": " + str + "\n");
    }
    else
    {
	write("Ok.\n");
    }

#if 0
    say(QCIMIE(this_object(), PL_MIA) + " @@race_sound:" + 
        file_name(this_object()) +
	"@@: " + str + "\n");
#endif

    say(sprintf("@@vbfc_say:%s@@: %s\n", file_name(this_object()), str));
    return 1;
}

public string
vbfc_say()
{
    object for_ob = previous_object(-1);
    string ret;
    
    if (CAN_SEE_IN_ROOM(for_ob) && CAN_SEE(for_ob, this_object()))
	ret = query_Imie(for_ob, PL_MIA);
    else
	ret = "Glos " + (for_ob->query_met(this_object())
		? query_met_name(PL_DOP)
		: query_rasa(PL_DOP));

    return ret + " " + this_object()->race_sound();
}

public string
vbfc_say_to()
{
    object for_ob = previous_object(-1);
    
    if (CAN_SEE_IN_ROOM(for_ob) && CAN_SEE(for_ob, this_object()))
	return sprintf("%s %s do %s", query_Imie(for_ob, PL_MIA),
	    this_object()->race_sound(),
	    COMPOSITE_FILE->fo_desc_live(0, for_ob, PL_DOP));
    else
	return sprintf("Glos %s %s",
	    (for_ob->query_met(this_object())
		? query_met_name(PL_DOP)
		: query_rasa(PL_DOP)),
	    this_object()->race_sound());
}

/*
 * Function name: query_say_string 
 * Description  : This function returns the text the player last spoke using
 *                the say command. This can only be queried with this person
 *                being the interactive party for security reasons.
 * Returns      : string - the last string the player said.
 */
public nomask string
query_say_string()
{
    if (this_interactive() != this_object())
    {
	return "";
    }

    return say_string;
}

/*
 * Function name: race_sound
 * Description  : This function returns the VBFC value for the sound a
 *                particular living hears when this player speaks. It
 *                operates on previous_object(-1). Notice that we use
 *                query_race rather than query_race_name since the first
 *                will always return a true and valid race name. The
 *                person speaking is this_player().
 * Returns      : string - the race sound the receiver hears.
 */
public string
race_sound()
{
    string raceto = previous_object(-2)->query_race();

    if (!com_sounds[raceto])
    {
	return "mowi";
    }

    return com_sounds[raceto];
}

/*
 * Function name: actor_race_sound
 * Description  : This function returns the sound this_player() makes when
 *                he or she speaks. By default this is 'say'.
 * Returns      : string - the race sound the receiver hears.
 */
public string
actor_race_sound()
{
    return "mowisz";
}

/*
 * Function name: query_com_sounds
 * Description  : Returns the mapping with the sounds the way people
 *                understand the speech of this player.
 * Returns      : mapping - the mapping.
 */
public mapping
query_com_sounds()
{
    return secure_var(com_sounds);
}

/*
 * Function name:   start_souls
 * Description:     Tell the souls that we are using them, this is used to
 *		    add sublocations for the living object. Also call
 *		    'replace_soul' so that an obsolete soul can rederict
 *		    the usage to another newer soul/souls.
 * Arguments:       souls: an array with all souls that should be started
 */
nomask public string *
start_souls(string *souls)
{
    int il, rflag;
    mixed ob;
    string *replace_souls, *used_souls, *tmp;
    mapping replaced;

    used_souls = ({});
    replaced = ([]);

    do
    {
	rflag = 0;
	for (replace_souls = ({}), il = 0; il < sizeof(souls); il++)
	{
	    ob = souls[il];
	    catch(ob->teleledningsanka());
	    ob = find_object(ob);
	    if (ob)
	    {
		if (replaced[ob]) /* Dont replace twice */
		    continue;
		else
		{
		    tmp = ob->replace_soul();
		    replaced[ob] = 1;
		}

		if (stringp(tmp))
		{
		    replace_souls += ({ tmp });
		    rflag = 1;
		}
		else if (pointerp(tmp))
		{
		    replace_souls += tmp;
		    rflag = 1;
		    if (member_array(souls[il], tmp) >= 0)
			tmp = 0;
		}
		
		if ((tmp == 0) && (member_array(souls[il], used_souls) < 0))
		{
		    ob->using_soul(this_object());
		    used_souls += ({ souls[il] });
		}
	    }
	    else
		used_souls += ({ souls[il] });
	}
	if (rflag)
	    souls = replace_souls + ({});
    } while (rflag);

    return used_souls;
}

/*
 * Function name: query_wizsoul_list
 * Description  : Give back the array with filenames of wizard souls.
 * Returns      : string * - the wizard soul list.
 */
nomask public string *
query_wizsoul_list()
{
    return secure_var(wiz_souls);
}

/*
 * Function name:   load_wiz_souls
 * Description:     Load the wizard souls into the player.
 * Returns:         True if successful.
 */
static nomask int
load_wiz_souls()
{
    int rank;

    if (!strlen(geteuid(this_object())))
    {
	write("PANIC! Player has no euid!\n");
	return 0;
    }

    /* Only wizards can have wizard souls. */
    if (rank = SECURITY->query_wiz_rank(geteuid(this_object())))
    {
        wiz_souls = WIZ_SOUL(rank)->get_soul_list();
    }
    else
    {
	wiz_souls = ({ });
	return 1;
    }

    if (!sizeof(wiz_souls))
    {
	write("Error loading wizard soul list. No wizard soul loaded.\n");
	return 0;
    }

    wiz_souls = start_souls(wiz_souls);
    return 1;
}

/*
 * Function name: load_command_souls
 * Description  : Load the command souls into the player.
 * Returns      : int 1/0 - success/failure.
 */
nomask public int
load_command_souls()
{
    soul_souls = query_cmdsoul_list();
    if (!sizeof(soul_souls))
    {
	soul_souls = NPC_SOULS;
    }

    soul_souls = start_souls(soul_souls);
    update_cmdsoul_list(soul_souls);
    return 1;
}

/*
 * Function name:   load_tool_souls
 * Description:     Load the tool souls into the player.
 * Returns:         True upon success.
 */
nomask public int
load_tool_souls()
{
    if ((SECURITY->query_wiz_rank(geteuid()) < WIZ_NORMAL) ||
	!interactive(this_object()))
    {
	tool_souls = ({});
	return 0;
    }

    tool_souls = query_tool_list();
    if (!sizeof(tool_souls))
    {
	/* This must be this_object()-> so don't touch! */
	this_object()->add_toolsoul(TRACER_TOOL_SOUL);
	tool_souls = query_tool_list();
    }

    tool_souls = start_souls(tool_souls);
    update_tool_list(tool_souls);
    return 1;
}

/*
 * Function name:   my_commands
 * Description:     Try to find and perform a command.
 * Arguments:       str - the argument string.
 * Returns:         True if the command was found.
 */
static int
my_commands(string str)
{
    int    i, rv, time, time2;
    object ob;
    mixed  *spell_list;
    string verb = query_verb();
    int    size;

    /* Zamiana ?temat -> ? temat. */
    if (verb[0] == '?' && verb != "?")
    {
        str = str ? verb[1..] + " " + str : verb[1..];
        verb = "?";
    }

    /* Don't waste the wiz-souls and toolsouls on mortals.
     */
    if (query_wiz_level())
    {
	/* This construct with while is faster than any for-loop, so keep
	 * it this way.
	 */
	size = sizeof(wiz_souls);
	i = -1;
	while(++i < size)
	{
	    ob = find_object(wiz_souls[i]);
	    if (!ob)
	    {
		if (catch(wiz_souls[i]->teleledningsanka()))
		    tell_object(this_object(),
			"Yikes, baaad soul: " + wiz_souls[i] + "\n");
		ob = find_object(wiz_souls[i]);
		if (!ob)
		    continue;
	    }
	    if (ob->exist_command(verb))
	    {
		ob->open_soul(0);
		export_uid(ob);
		ob->open_soul(1);
		rv = ob->do_command(verb, str);
		ob->open_soul(0);
		if (rv)
		    return 1;
	    }
	}

	size = sizeof(tool_souls);
	i = -1;
	while(++i < size)
	{
	    ob = find_object(tool_souls[i]);
	    if (!ob)
	    {
		if (catch(tool_souls[i]->teleledningsanka()))
		    tell_object(this_object(),
			"Yikes, baaad soul: " + tool_souls[i] + "\n");
		ob = find_object(tool_souls[i]);
		if (!ob)
		    continue;
	    }
	    if (ob->exist_command(verb))
	    {
		ob->open_soul(0);
		export_uid(ob);
		ob->open_soul(1);
		rv = (int)ob->do_command(verb, str);
		ob->open_soul(0);
		if (rv)
		    return 1;
	    }
	}
    }

    size = sizeof(soul_souls);
    i = -1;
    while(++i < size)
    {
	ob = find_object(soul_souls[i]);
	if (!ob)
	{
	    if (catch(soul_souls[i]->teleledningsanka()))
		tell_object(this_object(),
		    "Yikes, baaad soul: " + soul_souls[i] + "\n");
	    ob = find_object(soul_souls[i]);
	    if (!ob)
		continue;
	}
	if (ob->exist_command(verb))
	{
	    if (ob->do_command(verb, str))
		return 1;
	}
    }

    spell_list = (object *)this_object()->query_spellobjs();
    size = sizeof(spell_list);
    i = -1;
    while(++i < size)
    {
	if (stringp(spell_list[i]))
	    ob = find_object(spell_list[i]);
	else
	    ob = spell_list[i];
	if (!objectp(ob))
	{
	    if (catch(spell_list[i]->teleledningsanka()))
		tell_object(this_object(),
		    "Yikes, baaad spell soul: " + spell_list[i] + "\n");
	    ob = find_object(spell_list[i]);
	    if (!ob)
		continue;
	}
	if (ob->exist_command(verb))
	{
	    notify_fail("Koncentrujesz sie juz nad innym czarem.\n");
    	    if (aid && get_alarm(aid))
	    	return 0;

	    /* A call to 'start_spell_fail' indicates that we are
               preparing for casting the spell. This can be used to
	       deduct mana. If the spell is broken mana will still be lost.
	     */
	    if (ob->start_spell_fail(verb, str))
		return 1;

	    if (!ob->query_spell_mess(verb, str))
	    {
	        write("Zaczynasz koncentrowac sie nad czarem.\n");
	        say(QCIMIE(this_player(), PL_MIA) + " zamyka oczy i "
	          + "koncentruje sie.\n");
	    }

            time = ob->query_spell_time(verb, str);
	    time2 = (time < 1 ? 2 : time + 2) + 
		query_prop(LIVE_I_ATTACK_DELAY);
	    aid = set_alarm(itof(time2), 0.0,
		&cmdhooks_do_spell(verb, str, time, ob));
/*	    add_attack_delay(time, 0); */
	    return 1;
	}
    }

    return 0;
}

/*
 * Function name: reopen_soul
 * Description  : This function allows for the euid of this player to be
 *                re-exported in only a very limited number of cases.
 */
nomask public void
reopen_soul()
{
    object ob  = previous_object();
    string fun = calling_function();

    /* Check carefully. */
    if ((!strlen(REOPEN_SOUL_ALLOWED[fun])) ||
	(file_name(ob) != REOPEN_SOUL_ALLOWED[fun]) ||
	(!interactive(this_object())))
    {
	return;
    }

    ob->open_soul(0);
    export_uid(ob);
    ob->open_soul(1);
    call_other(ob, (fun + REOPEN_SOUL_RELOAD));
    ob->open_soul(0);
}

/*
 * Function name: update_hooks
 * Description  : This function loads and initializes all wizards souls,
 *                tool souls and command souls the player can have.
 */
nomask public void
update_hooks()
{
    load_wiz_souls();
    load_tool_souls();
    load_command_souls();
}

/*
 * Function name:   cmdhooks_do_spell
 * Description:     Execute a spell
 */
void
cmdhooks_do_spell(string spell, string sparg, int dtime, object spellob)
{
    mixed fail;
    string fail_str;
    int i;

    fail_str = 0;

    remove_prop(LIVE_I_ATTACK_DELAY);
    set_this_player(this_object());

    if (objectp(spellob))
    {
	if (spellob->exist_command(spell))
	{
	    if (stringp(fail = spellob->do_command(spell, sparg)))
		fail_str = fail;
	    if (intp(fail) && fail == 1)
		return;
	}
    }

    if (strlen(fail_str))
	write(fail_str);
    else
	write(LD_SPELL_FAIL);
    return;

}

/*
 * Function name:   cmdhooks_break_spell
 * Description:     Break the preparation for a spell. Note that the caster
 *		    still suffers the attack delay.
 * Returns:	    True if a spell was being prepared
 */
public int
cmdhooks_break_spell(string msg)
{
    if (aid && get_alarm(aid))
    {
	if (!strlen(msg))
	    tell_object(this_object(), LD_SPELL_CONC_BROKEN);
	else
	    this_object()->catch_msg(msg);
	remove_alarm(aid);
	this_object()->remove_prop(LIVE_I_CONCENTRATE);
	aid = 0;
	return 1;
    }
    return 0;
}

#define R_MESKI		0
#define R_ZENSKI	1
#define R_NIJAKI	2
#define R_MESKOOS	3
#define R_NIEMESKOOS	4

public object *
parse_command_obiekty_zaimka(int rodzaj)
{
    ob_zaimkow[rodzaj] -= ({ 0 });
    return ob_zaimkow[rodzaj];
}

public void
parse_command_set_rodzaj(int rodzaj, int plur_flag)
{
    r_zaimkow[1] = r_zaimkow[0];

    if (rodzaj == -1)
    {
        r_zaimkow[0] = -1;
        return ;
    }
    
    if (plur_flag)
    {
        if (rodzaj == PL_MESKI_OS)
            r_zaimkow[0] = R_MESKOOS;
        else
            r_zaimkow[0] = R_NIEMESKOOS;
    }
    else
    {
	if (rodzaj < PL_ZENSKI)
	    r_zaimkow[0] = R_MESKI;
	else if (rodzaj == PL_ZENSKI)
	    r_zaimkow[0] = R_ZENSKI;
	else r_zaimkow[0] = R_NIJAKI;
    }
}

private void
update_bit_zaimkow(int r)
{
    switch(r)
    {
        case R_MESKI:		bit_zaimkow |= 1;
            break;
        case R_NIJAKI:		bit_zaimkow &= (~1);
        			bit_zaimkow |= 4;
            break;
        case R_NIEMESKOOS:	bit_zaimkow |= 2;
        			bit_zaimkow &= (~4);
            break;
        case R_MESKOOS: 	bit_zaimkow &= (~2);
            break;
    }
}

public int
parse_command_bit_zaimkow()
{
    return bit_zaimkow;
}

/*
 * Nazwa funkcji : set_obiekty_zaimkow
 * Opis          : Jest to funkcja wspomagajaca system zaimkow Arkadii.
 *		   Ustawia w livingu liste obiektow, ktore beda pozniej mogly
 *		   byc uzyte, jesli living uzyje w komendzie zaimka zamiast
 *		   nazwy. Kazdy rodzaj ma swoja liste obiektow. W tej funkcji
 *		   sie go nie podaje, gdyz driver w czasie rozpatrywania
 *		   parse_command() ustawil w livingu rodzaj uzytej nazwy.
 *		   Dlatego tez funkcja ta powinna byc wywolywana wylacznie
 *		   bezposrednio po udanym uzyciu parse_command(). Funkcja
 *		   przyjmuje jeden albo dwa argumenty - tablice obiektow.
 *		   Jeden w przypadku komend z jednym obiektowym argumentem
 *		   (np 'zabij <kogo>'), dwa dla komend z dwoma argumentami
 *		   (np 'daj <co> <komu>'). Tablice obiektow podaje sie w tej
 *		   kolejnosci, w jakiej zostaly uzyte w parse_command().
 * Argumenty     : object *   - tablica z lista obiektow do zapamietania
 *				do pozniejszego wykorzystania przez zaimki.
 *		   object *   - to co powyzej, dla komend z dwoma argumentami
 *				obiektowymi. [opcjonalny]
 */
public void
set_obiekty_zaimkow(object *ob, object *ob2 = 0)
{
    int r, rodzaj_shorta, ix;
    object *obs;
    
    if (!sizeof(ob))
        return ;
        
    if (sizeof(ob2))
        ix = 2;
    else ix = 1;
        
    while (--ix >= 0)
    {
        if (ix)
            obs = ob2;
        else
            obs = ob;

        r = r_zaimkow[ix];
        if (r == -1)
            continue;
        ob_zaimkow[r] = obs;
        
        rodzaj_shorta = obs[0]->query_rodzaj();
        
        if (r >= R_MESKOOS)
        {
            if (rodzaj_shorta == PL_MESKI_OS)
                rodzaj_shorta = R_MESKOOS;
            else
                rodzaj_shorta = R_NIEMESKOOS;
        }
        else
        {
    	if (rodzaj_shorta < PL_ZENSKI)
    	    rodzaj_shorta = R_MESKI;
    	else if (rodzaj_shorta == PL_ZENSKI)
    	    rodzaj_shorta = R_ZENSKI;
    	else rodzaj_shorta = R_NIJAKI;
        }

        if (r != rodzaj_shorta)
        {
            ob_zaimkow[rodzaj_shorta] = obs;
            update_bit_zaimkow(rodzaj_shorta);
        }
        
        update_bit_zaimkow(r);
    }
}
