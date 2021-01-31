/*
 * player/cmd_sec.c
 *
 * This is a subpart of player_sec.c
 *
 * Some standard commands that should always exist are defined here.
 * This is also the place for the quicktyper command hook.
 */

#include <files.h>
#include <macros.h>
#include <std.h>
#include <stdproperties.h>

/*
 * Prototypes.
 */
#ifndef NO_SKILL_DECAY
public nomask int query_skill_decay();
static nomask void decay_skills();
#endif NO_SKILL_DECAY
public nomask void save_me(int value_items);
nomask int quit(string str);
public int save_character(string str);
static nomask int change_password(string str);

/*
 * Global variables, they are static and will not be saved.
 */
static int save_alarm;           /* The id of the autosave-alarm */
static private string password2; /* Used when someone changes his password. */

/*
 * Function name: start_autosave
 * Description  : Call this function to start autosaving. Only works for
 *                wizards.
 */
static nomask void
start_autosave()
{
    /* Start autosaving if the player is not a wizard. */
    if (!query_wiz_level())
    {
	remove_alarm(save_alarm);

	save_alarm = set_alarm(300.0, 0.0, &save_me(1));
    }
}

/*
 * Function name: stop_autosave
 * Description  : Call this function to stop autosaving.
 */
static nomask void
stop_autosave()
{
    remove_alarm(save_alarm);
    save_alarm = 0;
}

/*
 * Function name: cmd_sec_reset
 * Description  : When the player logs in, this function is called to link
 *                some essential commands to him.
 */
static nomask void
cmd_sec_reset()
{
    add_action("old_quit",		"quit");
    add_action(quit,			"zakoncz");
    add_action("old_save_character",	"save");
    add_action(save_character,	    	"nagraj");
    add_action("old_change_password",	"password");
    add_action(change_password,		"haslo");

    init_cmdmodify();

    /* Start autosaving. */
    start_autosave();
}

/*
 * Function name:   compute_values
 * Description:     Recursively compute the values of the given list of
 *                  objects. If the objects contain objects with a value
 *                  that value is also taken into account. Only 2/3 of the
 *		    actual value will be stored. Also, coins aren't included
 *		    in the value summed up. Don't figure objects that can
 *		    be recovered into this list.
 * Arguments:       ob_list: The list of objects to sum up.
 */
public nomask int
compute_values(object *ob_list)
{
    int v, i, size, tmp;

    if (!pointerp(ob_list))
	return 0;

    v = 0;
    i = -1;
    size = sizeof(ob_list);
    while(++i < size)
    {
	if (ob_list[i]->query_recover() || ob_list[i]->query_auto_load())
	    continue;
	if (function_exists("create_heap", ob_list[i]) == COINS_OBJECT)
	    continue;
	tmp = ob_list[i]->query_prop(OBJ_I_VALUE);
	if (tmp > 1000)
	    tmp = 1000;
	v += tmp;
    }

    return ((2 * v) / 3);
}

/*
 * Function name: compute_auto_str
 * Description  : Walk through the inventory and check all the objects for
 *                the function query_auto_load(). Constructs an array with
 *                all returned strings. query_auto_load() should return
 *                a string of the form "<file>:<argument>".
 */
static nomask void
compute_auto_str()
{
    object *ob = deep_inventory(this_object());
    string str;
    string *auto = ({ });
    int i = sizeof(ob);

    while(--i >= 0)
    {
	str = ob[i]->query_auto_load();
	if (strlen(str))
	{
	    auto += ({ str });
	}
    }
    set_auto_load(auto);
}

/*
 * Function name: check_recover_loc
 * Description:   This function checks if the player is standing on a spot
 *                where recover may happend
 * Returns:       1 / 0 depending on outcome.
 */
nomask int
check_recover_loc()
{
    object tmp;
    int i;
    string *list, env;

    /* Check for armageddon */
    if (ARMAGEDDON->shutdown_active())
	return 1;

    /* Check for recoverable surroundings */
    if (objectp(tmp = environment(this_object())))
	env = file_name(tmp);
    else
	return 0;

    list = SECURITY->query_list_temp_start() +
	SECURITY->query_list_def_start();

    if (member_array(env, list) != -1)
    {
	return 1;
    }

    return 0;
}

/*
 * Function name: compute_recover_str
 * Description  : Walk through the inventory and check all the objects for
 *                the recover property.
 * Arguments    : int - if true the player manually typed 'save'. It means
 *                      we compute the string even though the player may
 *                      not be in a recover location.
 */
static nomask void
compute_recover_str(int verb)
{
    object *ob;
    string str, *recover;
    int i, loc, size;

    recover = ({ });

    loc = check_recover_loc();

    if (!(loc || verb))
    {
	set_recover_list(recover);
	return;
    }

    ob = deep_inventory(this_object());

    i = -1;
    size = sizeof(ob);
    while(++i < size)
    {
        if (ob[i]->check_recoverable(1) && !ob[i]->may_not_recover())
        {
	    if (!strlen(str = ob[i]->query_recover()))
		continue;
	    recover += ({ str });
        }

#if 0
	if (ob[i]->check_recoverable(1))
	{
	    if (ob[i]->may_not_recover())
	    {
		if (loc)
		    write("The " + ob[i]->short() + " fails to glow.\n");
	    }
	    else
	    {
		if (!strlen(str = ob[i]->query_recover()))
		    continue;
		recover += ({ str });
		
		if (loc)
		    write("The " + ob[i]->short() + " glows briefly.\n");
	    }
	}
#endif
    }

    set_recover_list(recover);
}

/*
 * Function name: save_me
 * Description  : Save all internal variables of a character to disk.
 * Arguments    : int value_items - If 0, set total value of money to 0,
 *                      otherwise calculate the total value of all the
 *                      stuff a character carries. It is true when the
 *                      player typed save and when he is autosaving.
 */
public nomask void
save_me(int value_items)
{
    if (value_items)
	set_tot_value(compute_values(deep_inventory(this_object())));
    else
	set_tot_value(0);

    /* Do some queries to make certain time-dependent 
     * vars are updated properly.
     */
    query_mana();
    query_fatigue();
    query_hp();
    query_stuffed();
    query_soaked();
    query_intoxicated();
    query_headache();
    query_age();
    compute_auto_str();
    compute_recover_str(value_items);
#ifndef NO_SKILL_DECAY
    query_decay_time();
#endif NO_SKILL_DECAY
    seteuid(0);
    SECURITY->save_player();
    seteuid(getuid(this_object()));

    /* If the player is a mortal, we will restart autosave. */
    start_autosave();
    
#ifndef NO_SKILL_DECAY
    if (!query_wiz_level())
    {
	/* Handle decay here as this function is called regularly and often */
	if (query_skill_decay())
	{
	    set_alarm(1.0, 0.0, decay_skills);
	}
    }
#endif NO_SKILL_DECAY
}

public int
old_save_character(string str)
{
    notify_fail("Komenda 'save' zostala wycofana. Zamiast " +
        "niej mozesz uzyc 'nagraj'.\n");

    return 0;
}

/*
 * Function name:   save_character
 * Description:     Saves all internal variables of a character to disk
 * Returns:         Always 1
 */
public int
save_character(string str) 
{
    save_me(1);
    write("Ok.\n");
    return 1;
}

public int
old_quit(string str)
{
    notify_fail("Komenda 'quit' zostala wycofana. Zamiast " +
        "niej mozesz uzyc 'zakoncz'.\n");

    return 0;
}

/*
 * Function name: quit
 * Description:	  The standard routine when quitting. You cannot quit while
 *                you are in direct combat.
 * Returns:	  1 - always.
 */
nomask int
quit(string str)
{
    object *inv, tp;
    int    index, loc, size;
    
    tp = this_player();
    set_this_player(this_object());
    
    if (str == "cierpienia")
    {
        write("W takim razie pomecz sie jeszcze troche!\n");
        return 1;
    }
    
    loc = check_recover_loc();

    /* No way to chicken out of combat like that, but do allow it when
     * the game is being shut down.
     */
    if (!loc)
    {
	inv = all_inventory(environment(this_object()));
	index = sizeof(inv);
	while (--index >= 0)
	    if (living(inv[index]) &&
	        (inv[index]->query_attack() == this_object()))
	        break;
	        
	if (index != -1)
	{
	    write("\nNie mozesz zakonczyc gry w takim pospiechu - jestes w " +
	        "trakcie walki.\n\n");
	    return 1;
	}
    }

    inv = all_inventory(this_object());

    /* Only mortals drop stuff when they quit. */
    if (!query_wiz_level())
    {
	index = -1;
	size = sizeof(inv);
	while(++index < size)
	{
	    /* Objects that are neither recoverable, nor autoloading
 	     * should not be destructed if they are droppable.
	     */
	    if (!((loc && inv[index]->check_recoverable()) ||
		  stringp(inv[index]->query_auto_load()) ))
	    {
		/* However, we only try to drop it if the player is
		 * carrying it on the top level.
		 */
		if (!(inv[index]->query_prop(OBJ_M_NO_DROP)))
		{
		    command("poloz " + inv[index]->parse_command_id_list(PL_BIE)[0]);
		    inv[index] = 0;
		}
	    }
	}
    }

    write("Nagrywam postac.\n");
    say(QCIMIE(this_player(), PL_MIA) + " opuszcza swiat Arkadii.\n");

    save_me(0);

    /* Remove the objects. If there are some persistant objects left,
     * hammer hard and they will go away eventually.
     */
    inv = deep_inventory(this_object());
    size = sizeof(inv);
    index = -1;
    while(++index < size)
    {
	if (objectp(inv[index]))
	{
	    inv[index]->remove_object();
	}
	if (objectp(inv[index]))
	{
	    /* This is the hammer. */
	    SECURITY->do_debug("destroy", inv[index]);
	}
    }
    
    set_this_player(tp);

    this_object()->remove_object();
    return 1;
}

/*
 * Function name: change_password_new
 * Description  : This function is called by change_password_old to catch the
 *                new password entered by the player. Calls itself again to
 *                verify the new entered password and makes sure the new
 *                password is somewhat secure.
 * Arguments    : string str - the new password.
 */
static nomask void
change_password_new(string str)
{
    write("\n");
    if (!strlen(str))
    {
	write("Nie podal" + koncowka("es", "as") + " zadnego hasla, wiec "+
	    "nie zostalo ono zmienione.\n");
	return;
    }

    /* The first time the player types the new password. */
    if (password2 == 0)
    {
	if (strlen(str) < 6)
	{
	    write("Nowe haslo musi miec minimum 6 znakow.\n");
	    return;
	}

	if (!(SECURITY->proper_password(str)))
	{
	    write("Nowe haslo musi spelniac podstawowe zasady " +
	        "bezpieczenstwa.\n");
	    return;
	}

	password2 = str;
	input_to(change_password_new, 1);
	write("Wpisz nowe haslo ponownie, w celu potwierdzenia go.\n");
	write("Nowe haslo (ponownie): ");
	return;
    }

    /* Second password doesn't match the first one. */
    if (password2 != str)
    {
	write("Nowe hasla nie zgadzaja sie. Haslo nie zostalo zmienione.\n");
	return;
    }

    set_password(crypt(password2, 0));	/* Generate new seed */
    password2 = 0;
    write("Haslo zmienione.\n");
}

/*
 * Function name: change_password_old
 * Description  : Takes and checks the old password.
 * Arguments    : string str - the given (old) password.
 */
static nomask void 
change_password_old(string str)
{
    write("\n");
    if (!strlen(str) ||
	!match_password(str)) 
    {
	write("Zle stare haslo.\n");
	return;
    }

    password2 = 0;
    input_to(change_password_new, 1);
    write("Zeby twoje haslo bylo bezpieczniejsze, uwazamy, iz powinno\n" +
          "spelniac podstawowe kryteria:\n" +
          "- haslo musi miec minimum 6 znakow;\n" +
          "- haslo musi zawierac przynajmniej jeden znak nie bedacy " +
            "litera;\n" +
          "- haslo musi zaczynac sie i konczyc litera.\n\n" +
          "Nowe haslo: ");
          
    
    
#if 0    
    write("To prevent people from breaking your password, we feel the need\n" +
	  "require your password to match certain criteria:\n" +
	  "- the password must be at least 6 characters long;\n" +
	  "- the password must at least contain one 'special character';\n" +
	  "- a 'special character' is anything other than a-z and A-Z;\n" +
	  "- the 'special character' may not be the first or the last\n" +
	  "  letter in the password, that is somewhere before and after a\n" +
	  "  'special character' there must be a normal letter.\n\n" +
	  "New password: ");
#endif
}

public int
old_change_password(string str)
{
    notify_fail("Komenda 'password' zostala wycofana. Zamiast " +
        "niej mozesz uzyc 'haslo'.\n");

    return 0;
}

/*
 * Function name: change_password
 * Description  : Allow a player to change his old password into a new one.
 * Arguments    : string str - the command line argument.
 * Returns:     : int 1 - always.
 */
static nomask int
change_password(string str)
{
    write("Stare haslo: ");
    input_to(change_password_old, 1);
    return 1;
}
