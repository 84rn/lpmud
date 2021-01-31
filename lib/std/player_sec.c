/*
 * /std/player_sec.c
 *
 * This file is statically inherited by player_pub to ensure the
 * protection of all lower level routines.
 */

#pragma save_binary
#pragma strict_types

inherit "/std/living";

/* This order is on purpose to limit the number of prototypes necessary. */
#include "/std/player/savevars_sec.c"
#include "/std/player/quicktyper.c"
#include "/std/player/cmd_sec.c"
#include "/std/player/getmsg_sec.c"
#include "/std/player/death_sec.c"
#include "/std/player/querys_sec.c"
#include "/std/player/pcombat.c"
#include "/std/player/more.c"

#include <files.h>
#include <macros.h>
#include <money.h>
#include <language.h>
#ifndef OWN_STATUE
#include <living_desc.h>
#endif
#include <ss_types.h>
#include <std.h>
#include <stdproperties.h>
#include <options.h>
#include <composite.h>
#include <debug.h>

#define LINKDEATH_TIME 180.0 /* three minutes */

#define DEBUG_EVAL_LIMIT	10000

/* 
 * List of properties that are to be saved in the player object. This list
 * is used at both restore and save since the name of the property itself
 * isn't stored, just the value.
 * 
 * WARNING: If you add something, add it at the end of the array. Do
 *          NOT insert anything, or all previously stored arrays will
 *          be restored in an incorrect sequence.
 */
#define SAVE_PROPS ({ CONT_I_WEIGHT, CONT_I_HEIGHT, WIZARD_I_BUSY_LEVEL, \
		      PLAYER_I_MORE_LEN, CONT_I_VOLUME, LIVE_I_LANGUAGE, \
		      PLAYER_I_SCREEN_WIDTH })

/*
 * Global variables. They are not saved.
 */
private static int    ld_alarm;  /* The alarm used when a player linkdies. */
#ifndef NO_SKILL_DECAY
private static int do_skill_decay = 0; /* Flag to control skill decay */
#endif NO_SKILL_DECAY

/*
 * Prototypes
 */
void new_init();
void load_auto_obj(string *load_arr);
int load_recover_list(string pl_name, string *recover_arr);
void load_auto_shadow(string *load_arr);
static void init_saved_props();
#ifndef NO_SKILL_DECAY
static nomask void decay_skills();
#endif NO_SKILL_DECAY

/*
 * Przed zniszczeniem obiektu, sprawdza czy wywolane przez mudlib,
 * a jesli nie, to loguje w stosownym pliku.
 */
public int
remove_object()
{
    object ti;

    ti = this_interactive();

    if (ti && (file_name(ti)[0..6] != "/secure")
           && (getuid(ti) != getuid(this_object()))
           && (IS_CLONE))
        SECURITY->log_syslog("REM_PLAYER", ctime(time()) + ": UID[" +
        getuid(this_interactive()) + "], p_obj[" + 
        file_name(previous_object()) + "] -> " + 
        this_object()->query_name() + (query_ip_number(this_object()) ? "" :
            "[IDLE]") + "\n", 10000);
        
    return ::remove_object();
}


/*
 * Function name: query_def_start
 * Description  : Return the default starting location of the player type.
 *                This function is supposed to be replaced in inheriting
 *                player objects.
 */
public string
query_def_start()
{
    return DEFAULT_START;
}

/*
 * Function name: query_orig_stat
 * Description:   Return the default starting stats of a player
 *                This function is supposed to be replaced in inheriting
 *                player objects.
 */
public int *
query_orig_stat() 
{
    int i, *list;

    list = ({});
    i = -1;
    while(++i < SS_NO_STATS)
    {
	list += ({ 1 });
    }
    return list;
}

/*
 * Function name: query_orig_learn
 * Description:   Return the default starting stats of a player
 *                This function is supposed to be replaced in inheriting
 *                player objects.
 */
public int *
query_orig_learn() 
{
    int i, *list;

    list = ({});
    i = -1;
    while(++i < SS_NO_EXP_STATS)
    {
	list += ({ 100 / SS_NO_EXP_STATS });
    }
    return list;
}

#ifndef NO_ALIGN_TITLE
/*
 * Function name: query_new_al_title
 * Description:   Return the default starting title of a player
 *                This function is supposed to be replaced in inheriting
 *                player objects.
 */
public string
query_new_al_title()
{
    return "neutral";
}
#endif NO_ALIGN_TITLE

/*
 * Function name: fixup_screen
 * Description:   Restore the players screen width. Normally called
 *                during login.
 */
public nomask void
fixup_screen()
{
    int width = query_option(OPT_SCREEN_WIDTH);

    if (!width)
	width = 80;
    set_screen_width(width);
}

/*
 * Function name: add_prop_player_i_screen_width
 * Description:   Autodetect when a player's screen width is modified
 *                and notify the game driver. Do not allow VBFC values.
 * Arguments:     val: The new property value
 */
public nomask int
add_prop_player_i_screen_width(mixed val)
{
    if (!intp(val))
	return 1;
    if (val)
	set_screen_width(val);
    else
	set_screen_width(80);
    return 0;
}

#ifndef NO_SKILL_DECAY
/*
 * Function name:   setup_skill_decay()
 * Description:     setup the skill decay flag.
 */
public nomask void
setup_skill_decay()
{
    if (query_wiz_level())
	return;

    do_skill_decay = 1;
    set_alarm(2.0, 0.0, decay_skills);
}

/*
 * Function name:   query_skill_decay
 * Description:     Gives back the skill decay status
 * Returns:         The skill decay status
 */
public nomask int
query_skill_decay()
{
    return do_skill_decay;
}

/*
 * Function name: get_train_max
 * Description:   Return the max value of a skill that a trainer trains.
 * Arguments:     skill - the skill to be examined.
 *                ob - the object defining the skill
 * Returns:       See above.
 */
static nomask int
get_train_max(int skill, mixed ob)
{
    int rval = 0;

    if (catch(rval = ob->sk_query_max(skill)))
	log_file("BAD_TRAIN", ctime(time()) + ": " + ob + "\n");

    return rval;
}

/*
 * Function name: query_decay_skill
 * Description:   Return 1 if a skill should be decayed, 0 if not.
 * Arguments:     list - the list of objects defining the skill train max
 *                skill - the skill to be examined.
 * Returns:       See above.
 */
static nomask int
query_decay_skill(mixed list, int skill)
{
    int *sklist, i, sz, max, sk;

    /* Load all trainers first */
    catch(list->teleledningsanka());

    /* Check the contents */
    sklist = ({ }) + map(list, &get_train_max(skill, ));
        if (sizeof(SS_SKILL_DESC[skill]))
	    sk = SS_SKILL_DESC[skill][5];
        else
	    sk = 0;
    sklist += ({ (sk > 0 ? sk : MIN_SKILL_LEVEL) });

    for (i = 0, max = 0, sz = sizeof(sklist) ; i < sz ; i++)
	        if (max < sklist[i])
		    max = sklist[i];

    return (query_base_skill(skill) > max);
}

/*
 * Function name: decay_skills
 * Description:   Do skill decay in the player
 *                Call this function ONLY if it's necessary, as when
 *                entering the game or entering/leaving a guild as
 *                it's a bit costly.
 */
static nomask void
decay_skills()
{
    mixed obs;
    mixed otmp;
    int *skills, i, sz;
    string str, tmp;
    
    /* Only do this on the proper interval, and wizards pass by, of course */
    if ((query_decay_time() < SKILL_DECAY_INTERVAL) ||
	query_wiz_level())
    {
	return;
    }

    set_this_player(this_object());
    
    /* Reset the time for next call. */
    reset_decay_time();

    /* Get the list of trainer objects */
    obs = ({});
    otmp = this_object()->query_guild_trainer_occ();
    obs += pointerp(otmp) ? otmp : ({ otmp });
    otmp = this_object()->query_guild_trainer_race();
    obs += pointerp(otmp) ? otmp : ({ otmp });
    otmp = this_object()->query_guild_trainer_lay();
    obs += pointerp(otmp) ? otmp : ({ otmp });
    obs -= ({ 0 });

    /* Filter all relevant skills */
    skills = filter(query_all_skill_types(), &operator(>)(99999));

    /* Find out what skills need decay */
    skills = filter(skills, &query_decay_skill(obs, ));

    /* Do decay */
    if (sizeof(skills))
    {
	tmp = ((tmp = this_object()->query_guild_name_occ()) ? tmp : "") + ", " +
	    ((tmp = this_object()->query_guild_name_lay()) ? tmp : "") + ", " +
	    ((tmp = this_object()->query_guild_name_race()) ? tmp : "");
	
	str = sprintf("%s\t\t%s\n%s\t\t", this_object()->query_name(), tmp,
		      ctime(time()));
		      
	for (i = 0, sz = sizeof(skills) ; i < sz ; i++)
	{
	    str += sprintf("%i ", skills[i]);
	    set_skill(skills[i], query_base_skill(skills[i]) - 1);
	}
	log_file("DECAY_LOG", str + "\n", 50000);
    }
    else
	do_skill_decay = 0;
}
#endif NO_SKILL_DECAY

/*
 * Function name: enter_game
 * Description  : Enter the player into the game.
 * Arguments    : string pl_name - the name of the player.
 *                string pwd     - the password if it was changed.
 * Returns      : int 1/0 - login succeeded/failed.
 */
public nomask int
enter_game(string pl_name, string pwd)
{
    string      fname;
    string	*souls;
    int 	il, size;
    int		lost_money;
    object 	ob;

    if ((MASTER_OB(previous_object()) != LOGIN_OBJECT) &&
	(MASTER_OB(previous_object()) != LOGIN_NEW_PLAYER) &&
	(MASTER_OB(previous_object()) != WIZ_CMD_ARCH))
    {
	write("Bad login object: " + file_name(previous_object()) + "\n");
	return 0;
    }

    set_name(pl_name);
    set_wiz_level();

    new_init();             	/* All variables to default condition */

    seteuid(0);
    
    if (!SECURITY->load_player())
    {
	return 0;
    }
    
    ustaw_imiona(query_imiona());
    seteuid(query_wiz_level() ? pl_name : getuid(this_object()));
    dodaj_przym(0);			/* Set the adjectives as loaded */
    fixup_screen();

//  add_name(query_race_name());

    ustaw_odmiane_rasy(0);	/* Laduje odmiane rasy */
//    set_gender(query_gender());
    add_gender_names();
    if (query_wiz_level())
        dodaj_nazwy( ({ "czarodziej", "czarodzieja", "czarodziejowi",
          "czarodzieja", "czarodziejem", "czarodzieju" }),
          ({ "czarodzieje", "czarodzieji", "czarodziejom", 
          "czarodzieji", "czarodziejami", "czarodziejach" }), PL_MESKI_OS);

    /* Make some sanity things to guard against old and patched .o files */
    set_m_out(query_m_out());
    set_m_in(query_m_in());
    set_mm_out(query_mm_out());
    set_mm_in(query_mm_in());
    set_learn_pref(query_learn_pref(-1));

    /* People might have inconsistent acc_exp in guildstats, we must guard
     *  against it.
     */
    il = query_exp();
    this_object()->add_exp(1, 1);
    if (query_exp() < il)
    {
	il -= query_exp();
	this_object()->add_exp(il);
    }
    else
    {
	this_object()->add_exp(-1, 1);
    }

    set_living_name(pl_name);
    cmd_sec_reset();
    player_save_vars_reset();
    
    acc_exp_to_stats();   /* Setup our current stats */

    if (query_temp_start_location() &&
	SECURITY->check_temp_start_loc(query_temp_start_location()) >= 0)
    {
	catch(move_living(0, query_temp_start_location()));
	set_temp_start_location(0);
    }
    
    if (!environment())
    {
	if (!query_default_start_location() ||
	    ((!query_wiz_level() && !wildmatch("*jr", pl_name)) &&
	     SECURITY->check_def_start_loc(query_default_start_location()) < 0))
	    set_default_start_location(query_def_start());
	catch(move_living(0, query_default_start_location()));
    }

    /* Let players start even if their start location is bad */
    if (!environment())
    {
	if (catch(move_living(0, query_def_start())))
	{
	    /* If this start location is corrupt too, destruct the player */
	    write("ALARM, twoja startowa lokacja jest zepsuta!!\n");
	    destruct();
    	}
    }

    /* Restore the bits */
    unpack_bits();

/*
    if (query_wiz_level() &&
	objectp(environment()))
    {
	write("Starting in:" + file_name(environment()) + "\n");
    }
*/

    /* Restore the saved properties and add a default one. */
    add_prop(PLAYER_I_MORE_LEN, 20);
    init_saved_props();

    /* Non wizards should not have a lot of command souls */
    if (!query_wiz_level())
    {
	souls = query_cmdsoul_list();
	if (sizeof(souls))
	{
	    il = -1;
	    size = sizeof(souls);
	    while(++il < size)
	    {
		remove_cmdsoul(souls[il]);
	    }
	}
    }

    if (!m_alias_list)
    {
	m_alias_list = ([]);
    }

    if (query_autoshadow_list())
	load_auto_shadow(query_autoshadow_list());
    if (query_auto_load())
	load_auto_obj(query_auto_load());
    if (query_recover_list())
	if (!load_recover_list(pl_name, query_recover_list()))
	    set_recover_list( ({ }) );

    /* Set up skill decay now that the guild shadows are loaded
     * Do a first decay as well, making it a bit more frequent for
     * people who log on/off/on all the time
     */
#ifndef NO_SKILL_DECAY
    decay_time = time();
    setup_skill_decay();
#endif NO_SKILL_DECAY
     
    /* Tell the player when he was last logged in and from which site. */
    write("Ostatnie logowanie: " + ctime(query_login_time(), 1) +
        "\n           z hosta: " + query_login_from() + "\n");

    /* Start him up */
    this_object()->start_player();
    query_combat_object()->cb_configure();

    /* Do this after startup, so we can use the address and time at startup. */
    set_login_time();
    set_login_from();

    /* Non wizards should not have a lot of tool souls */
    if (!query_wiz_level())
    {
	souls = query_tool_list();
	if (sizeof(souls))
	{
	    il = -1;
	    size = sizeof(souls);
	    while(++il < size)
	    {
		this_object()->remove_toolsoul(souls[il]);
	    }
	}
    }

    if ((lost_money = query_tot_value()) > 0)
    {
	tell_object(this_object(), "Odnajdujesz " + lost_money +
		    " " + (lost_money == 1 ? "miedziak" : "miedziaki") +
		    ", ktore wczesniej gdzies zapodzial" + 
		    koncowka("es", "as") + ".\n");
    	MONEY_ADD(this_object(), lost_money);
    	if (lost_money > 3000)
	{
	    SECURITY->log_syslog("BIG_TOT_VALUE", sprintf("%s: %s %d\n",
		ctime(time()), query_real_name(), lost_money));
    	}
    }

    add_prop(PLAYER_I_LASTXP, query_exp());

    /* If a password was changed, set it. */
    if (strlen(pwd))
    {
	set_password(pwd);

	save_me(1);
    }
    
    return 1;
}

/*
 * Function name: init_saved_props
 * Description  : Add the saved properties to the player.
 */
static void
init_saved_props()
{
    int index = -1;
    int size = ((sizeof(SAVE_PROPS) < sizeof(saved_props)) ?
	sizeof(SAVE_PROPS) : sizeof(saved_props));

    while(++index < size)
    {
	if (saved_props[index])
	{
	    add_prop(SAVE_PROPS[index], saved_props[index]);
	}
	else
	{
	    remove_prop(SAVE_PROPS[index]);
	}
    }

    saved_props = 0;
}

/*
 * Function name: open_player
 * Description  : This function may only be called by SECURITY or by the
 *                login object to reset the euid of this object.
 */
public nomask void
open_player()
{
    if ((previous_object() == find_object(SECURITY)) ||
	(MASTER_OB(previous_object()) == LOGIN_OBJECT))
    {
	seteuid(0);
    }
}

/*
 * Function name: fix_saveprops_list
 * Description  : Before the player is saved, this function is called to
 *                store several properties into an array that will be
 *                saved in the player file.
 */
nomask public int
fix_saveprop_list()
{
    int i, size;

    /* Fix the saved_props list before save */
    saved_props = ({ });
    i = -1;
    size = sizeof(SAVE_PROPS);
    while(++i < size)
    {
	saved_props += ({ query_prop(SAVE_PROPS[i]) });
    }
}

/*
 * Function name: save_player
 * Description  : This function actually saves the player object.
 * Arguments    : string pl_name - the name of the player
 * Returns      : int 1/0 - success/failure.
 */
nomask public int
save_player(string pl_name)
{
    if (!pl_name)
	return 0;

    pack_bits();
    seteuid(getuid(this_object()));
    save_object(PLAYER_FILE(pl_name));
    seteuid(getuid(this_object()));

    /* Discard the props again */
    saved_props = 0;
    return 1;
}

/*
 * Function name: load_player
 * Description  : This function actually loads the player file into the
 *                player object.
 * Arguments    : string pl_name - the name of the player.
 * Returns      : int 1/0 - success/failure.
 */
nomask public int
load_player(string pl_name)
{
    int ret;
   
    if (!pl_name)
	return 0;

    seteuid(getuid(this_object()));
    ret = restore_object(PLAYER_FILE(pl_name));
    seteuid(0);
    return ret;
}

/*
 * Function name: actual_linkdeath
 * Description  : This function is called when the player actually linkdies.
 *                If the player is in combat, this will be delayed, or else
 *                it is called directly.
 */
static nomask void
actual_linkdeath()
{
#ifdef STATUE_WHEN_LINKDEAD
#ifdef OWN_STATUE
    OWN_STATUE->die(this_object());
#else   
    tell_roombb(environment(), LD_STATUE_TURN(this_object()), ({}),
                this_object());
#endif OWN_STATUE
#endif STATUE_WHEN_LINKDEAD

    /* People should not autosave while they are linkdead. */
    stop_autosave();


    remove_alarm(ld_alarm);
    ld_alarm = 0;
    set_linkdead(1);
}

/*
 * Function name: linkdie
 * Description  : When a player linkdies, this function is called.
 */
nomask public void
linkdie()
{
    if (previous_object() != find_object(SECURITY))
    {
	return;
    }

    if (objectp(query_attack()))
    {
	tell_room(environment(), QCIMIE(this_object(), PL_MIA) +
	    " traci kontakt z rzeczywistoscia.\n", ({ this_object() }) );

	ld_alarm = set_alarm(LINKDEATH_TIME, 0.0, actual_linkdeath);
    }
    else
    {
	actual_linkdeath();
    }
}

/*
 * Function name: query_linkdead_in_combat
 * Description  : This function returns true if the player is linkdead,
 *                but still in combat.
 * Returns      : int 1/0 - in combat while linkdead or not.
 */
nomask public int
query_linkdead_in_combat()
{
    return (ld_alarm != 0);
}

/*
 * Function name: revive
 * Description  : When a player revives from linkdeath, this function is
 *                called.
 */
nomask public void
revive()
{
    object default_death;

    if (MASTER_OB(previous_object()) != LOGIN_OBJECT)
    {
	return;
    }

    /* If the player is not in combat, revive him. Else, just give a
     * a message about the fact that the player reconnected.
     */
    if (!ld_alarm)
    {
	set_linkdead(0);

#ifdef OWN_STATUE
	OWN_STATUE->revive(this_object());
#else	
	tell_roombb(environment(), QCIMIE(this_object(), PL_MIA) + " " +
	    STATUE_TURNS_ALIVE + ".\n", ({this_object()}), this_object());
#endif OWN_STATUE

	/* We reset these variables so the player does not gain mana or
	 * hitpoints while in LD.
	 */
	player_save_vars_reset();
	save_vars_reset();

	/* Start autosaving again. */
	start_autosave();
	
	/* Jesli gracz zginal w momencie straty polaczenia,
	 * przepuszczamy go przez sekwencje smierci.
	 */
	if (query_ghost())
	{
	    default_death = present("_default_death_", this_object());
	    if (!default_death)
		this_object()->death_sequence();
	    else
		default_death->init();
	}
    }
    else
    {
	tell_roombb(environment(), QCIMIE(this_object(), PL_MIA) +
	    " odzyskuje kontakt z rzeczywiscoscia.\n", ({this_object()}),
	    this_object());

	remove_alarm(ld_alarm);
	ld_alarm = 0;
    }
}

/*
 * Function name: load_auto_obj
 * Description  : Loads all autoloaded objects
 * Arguments    : string *load_arr - the array of objects to load.
 */
nomask static void
load_auto_obj(string *load_arr)
{
    string file, argument, *parts;
    object ob;
    int	   il, size;

    if (!pointerp(load_arr) ||
	!sizeof(load_arr))
    {
	return;
    }

    il = -1;
    size = sizeof(load_arr);
    while(++il < size)
    {
	if (!stringp(load_arr[il]))
	{
	    write("Autoload array element " + il + " corrupt.\n");
	    continue;
	}

	if (sscanf(load_arr[il], "%s:%s", file, argument) != 2)
	{
	    file = load_arr[il];
	    argument = 0;
	}
	if (!stringp(file))
	{
	    write("Auto load string corrupt: " + load_arr[il] + "\n");
	    continue;
	}
	if (LOAD_ERR(file))
	    continue;
	if (!objectp(find_object(file)))
	    continue;
	catch(ob = clone_object(file));

	/* Note that we don't check for strlen() since we also want to call
	 * init_arg() if the format is 'filename:'.
	 */
	if (stringp(argument))
	{
	    if (catch(ob->init_arg(argument)))
	    {
		catch(ob->remove_object());
		continue;
	    }
	}
	catch(ob->move(this_object(), 1));
    }
}

/*
 * Function name: load_recover_list
 * Description:   Loads all recoverable objects.
 * Arguments:	  pl_name - the player name,
 *		  recover_arr - The array of objects to recover
 */
nomask static int
load_recover_list(string pl_name, string *recover_arr)
{
    string          file,
                    argument,
                    *parts;
    object          ob;
    int		    il, size;

    if (!pointerp(recover_arr) ||
	!sizeof(recover_arr))
    {
	return 0;
    }

    /* Do not restore after 2 hours */
    if (time() - file_time(PLAYER_FILE(pl_name) + ".o") > 7200)
    {
	return 0;
    }

    il = -1;
    size = sizeof(recover_arr);
    while(++il < size)
    {
	if (sscanf(recover_arr[il], "%s:%s", file, argument) != 2)
	{
	    file = recover_arr[il];
	    argument = 0;
	}
	if (!stringp(file))
	{
	    write("Recover string corrupt: " + recover_arr[il] + "\n");
	    continue;
	}
	if (LOAD_ERR(file))
	    continue;
	if (!objectp(find_object(file)))
	    continue;
	catch(ob = clone_object(file));

	/* Note that we don't check for strlen() since we also want to call
	 * init_recover() if the format is 'filename:'.
	 */
	if (stringp(argument))
	{
	    if (catch(ob->init_recover(argument)))
	    {
		catch(ob->remove_object());
		continue;
	    }
	}
        write("Masz nadal przy sobie " + ob->short(this_object(), PL_BIE) + 
            ".\n");
	catch(ob->move(this_object(), 1));
    }
}

/*
 * Function name: load_auto_shadow
 * Description  : Startup all the shadows that should shadow this player.
 * Arguments    : string *load_arr - the array of shadows to add.
 */
nomask static void
load_auto_shadow(string *load_arr)
{
    string file, argument;
    object ob;
    int	   il, size;

    if (!load_arr || !sizeof(load_arr))
	return;

    il = -1;
    size = sizeof(load_arr);
    while(++il < size)
    {
	if (sscanf(load_arr[il], "%s:%s", file, argument) != 2)
	{
	    write("Shadow load string corrupt: " + load_arr[il] + "\n");
	    continue;
	}
	if (LOAD_ERR(file))
	    continue;
	ob = find_object(file);
	if (!ob)
	    continue;
	ob = clone_object(file);
	if (argument)
	    ob->autoload_shadow(argument);
	else
	    ob->autoload_shadow(0);
    }
}

/*
 * Function name: new_save
 * Description  : This function is called to save the player initially.
 *                It is only called when new a player enters the game. It
 *                makes it possible to initialize variables using the
 *                standard set_ calls.
 * Arguments    : string pl_name - the name of the player.
 *                string pwd     - the (encrypted) password of the player.
 *                string pfile   - the player save file.
 * Returns      : int 1/0 - success/failure.
 */
public nomask int
new_save(string pl_name, string pwd, string pfile)
{
    if (!CALL_BY(LOGIN_NEW_PLAYER))
    {
	return 0;
    }

    write("Tworzymy nowa postac: " + pl_name + "\n");
    seteuid(getuid(this_object()));
    set_name(pl_name);
    set_password(pwd);
    set_player_file(pfile);
    new_init();			/* Initialize all variables on startup */

    save_object(PLAYER_FILE(pl_name));
    return 1;
}

/*
 * Function:     new_init
 * Description:  Initialises all variables to default conditions.
 */
static nomask void
new_init()
{
    int i;
    int *ostat;

    ostat = query_orig_stat();

    i = -1;
    while(++i < SS_NO_EXP_STATS)
    {
	set_base_stat(i, ostat[i]);
    }

    stats_to_acc_exp();

    set_learn_pref(query_orig_learn());

#ifndef NO_ALIGN_TITLE
    set_al_title(query_new_al_title());
#endif NO_ALIGN_TITLE
}

/*
 * Function name: create_living
 * Description  : Called to create the player. It initializes some variables.
 */
public nomask void
create_living()
{
    player_save_vars_reset();
}

/*
 * Function name: reset_living
 * Description  : We don't want people to mask this function.
 */
public nomask void
reset_living()
{
    return;
}

/*
 * Function name:	command
 * Description:		Makes the player execute a command, If the player is a
 *			wizard then the object must have the same effective
 *			userid as the wizard being forced.
 * Arguments:		cmd: String containing the command
 * Returns:		eval_cost or '0' if unsuccessful
 */
public nomask int
command(string cmd)
{
    if (query_wiz_level() &&
	objectp(previous_object()))
    {
        if (!SECURITY->wiz_force_check(geteuid(previous_object()),
	    geteuid()))
	{
	    return 0;
	}
    }

    return ::command(cmd);
}

/*
 * Function name: id
 * Description  : Returns whether this object can be identified by a certain
 *                name. That isn't the case if the player hasn't met you
 *                while the real name is used.
 * Arguments    : string str - the name to test
 * Returns      : int 1/0 - true if the name is valid.
 */
public int
id(string str)
{
    if ((str == query_real_name()) &&
	notmet_me(previous_object()))
    {
	return 0;
    }

    return ::id(str);
}
