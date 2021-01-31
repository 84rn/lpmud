/*
 * /std/player/savevars_sec.c
 *
 * This is a subpart of player_sec.c
 *
 * All the variables that are to be saved to the players save file
 * are defined here.
 */

#include <language.h>
#include <log.h>
#include <macros.h>
#include <ss_types.h>
#include <std.h>
#include <options.h>

/*
 * Non-static global variables. These variables are stored in the players
 * savefile.
 */
private string	name,			/* Name of the player */
                password,        	/* Password of the player */
	        player_file,		/* Name of file used as player file */
                path,			/* Current directory path */
                mailaddr,		/* Email adress of the player */
                default_start_location,	/* Start location of player */
                temp_start_location,	/* Temp start location */
		login_from,		/* Host wherefrom last logged in */
    		options,		/* Options for the player */
    		*imiona,		/* Odmiana imienia gracza */
		*auto_load,             /* Automatically loaded objects */
		*auto_shadow,           /* Our loyal shadows */
		*recover_list,		/* The recover list */
		*seconds;		/* Second characters */

private mixed	saved_props,		/* List of saved properties */
		*przymiotniki = ({ ({}), ({}) });		
					/* The adjectives describing player */

private int	*bit_savelist,		/* Saved bits */
		death_count,		/* Number of deaths */
                tot_value,		/* Accumulated value of inventory */
		age_heart,		/* # of heartbeats (== 2 seconds) */
		notify,			/* Notify status */
#ifdef FORCE_PASSWORD_CHANGE
		password_time,          /* Time passwords was changed */
#endif FORCE_PASSWORD_CHANGE
#ifndef NO_SKILL_DECAY
                decay_time_acc,         /* # of seconds since last decay */
#endif NO_SKILL_DECAY
		login_time;		/* Last time logging in */

private mapping m_remember_name,	/* Names of players we have met */
		m_alias_list,		/* Aliases for the quicktyper */
		m_notified;		/* People we are notified about */

/*
 * Static global variables. These variables are used internally and are not
 * stored in the players savefile.
 */
private static string	cap_name;	/* Capitalized name of the player */
private static int	*bit_wizlist,	/* The runtime bit wizardlist */
			*bit_bitlist,	/* The runtime bit bitlist */
			wiz_level,	/* BOOLEAN indicating wiz or not wiz */
#ifndef NO_SKILL_DECAY
			decay_time,     /* the last time the decay was done */
#endif NO_SKILL_DECAY
			age_time;	/* the last time the age was updated */

/*
 * Prototypes.
 */
nomask public int remove_autoshadow(mixed shadowfile);
public nomask void fixup_screen();
#ifndef NO_SKILL_DECAY
nomask public int query_skill_decay();
#endif NO_SKILL_DECAY

/*
 * Function name: player_save_vars_reset
 * Description  : Reset the private time-related functions.
 */
static nomask void
player_save_vars_reset()
{
    age_time = time();
}

/* 
 * Name of the player.
 * This can only be called from player_sec.
 */
static /* private */ nomask void
set_name(string n)
{
    name = n;
    cap_name = capitalize(n);

    ::set_name(name);
}

/*
 * Nazwa funkcji : ustaw_imiona
 * Opis          : Funkcja ustawia odmianie imienia gracza. Mianownik
 *		   imienia musi byc taki sam, jak prawdziwe imie gracza.
 *		   Odmiana musi byc podana w 6sciu przypadkach,
 *		   wszystkie imiona powinny skladac sie tylko z malych
 *		   liter.
 * Argumenty     : *string  - odmiana imienia gracza, 6 elementow.
 * Funkcja zwraca: 1, jesli podano prawidlowo ilosc przypadkow,
 *		   0, w przeciwnym razie.
 */
public nomask int
ustaw_imiona(string *tab)
{
    if (sizeof(tab) != 6)
        return 0;
        
    if (lower_case(tab[0]) != name)
        return 0;
        
    imiona = tab + ({ });
    name = tab[0];
    cap_name = capitalize(tab[0]);
    
    ::ustaw_imie(imiona, this_object()->query_living_rodzaj());
    
    return 1;
}

/*
 *
 */
public nomask string *
query_imiona()
{
    int c;
    
    if (sizeof(imiona) != 6)
    {
        imiona = allocate(6);
        for (c = 0 ; c < 6 ; c++)
            imiona[c] = name;
    }
    return imiona + ({ });
}

static varargs void
add_name(mixed name, int przyp)
{
    ::add_name(name, przyp);
}

/* 
 * Function name:   master_set_name
 * Description:     /secure/master needs to be able to do set_name. This
 *                  function can only be called from that object.
 * Arguments:       n: name to set
 */
nomask void
master_set_name(string n)
{
    if (previous_object() != find_object(SECURITY))
	return;

    name = n;
    cap_name = capitalize(n);

    ::set_name(name);
}

/*
 * Function name:   remove_name
 * Description:     Remove a certain name
 * Arguments:       n: A string or a pointer of strings of names
 */
nomask void
remove_name(mixed n)
{
    ::remove_name(n);
}

/*
 * Nazwa funkcji : dodaj_przym
 * Opis          : Funkcja przechwytujaca na poziomie gracza wywolanie
 *		   standardowego dodaj_przym(). Jej zadaniem jest
 *		   upewnienie sie, ze dodane przymiotniki zostana
 *		   zapamietane do zgrania w pliku postaci. Wywolana
 *		   bez argumentow wczytuje i ustawia przymiotniki
 *		   (wywolywane przy wejsciu do gry).
 * Argumenty     : string lp, lmn - takie jak przy dodaj_przym z /std/object
 *		   0 - przy logowaniu, wczytuje i ustawia przymiotniki
 * Funkcja zwraca: 1 przy udanym ustawieniu,
 *		   0 w przeciwnym wypadku.
 */
public varargs int
dodaj_przym(string lp, string lmn)
{
#ifdef LOG_ADJ
    if (this_player() != this_object())
        SECURITY->log_syslog(LOG_ADJ, ctime(time()) + ": "
          + query_real_name() + " new adj " + lp + ":" + lmn + " by "
          + this_player()->query_real_name() + "\n");
#endif

    if (lp) 
    {
/*
	string *adj, temp;
	int j, len;
	adj = query_przym(1, PL_MIA);

    	if (sizeof(adj) > 1)
    	    return 0;

	for (i = 0; i < sizeof(adj); i++)
	    len += strlen(adj[i]);
	    
	len += strlen(lp);
	    
	if (len > 33)
	    return 0;

*/
	::dodaj_przym(lp, lmn);
	przymiotniki = query_przymiotniki();
	return 1;
    }
    else 
    {
        int i = sizeof(przymiotniki[0]);
        
        while (--i >= 0)
            ::dodaj_przym(przymiotniki[0][i], przymiotniki[1][i]);
    }
    
    return 1;
}

/*
 * Function name:   remove_adj
 * Description:     'go through function' for removing the adjectives.
 * Arguments:       str: The adjective string to remove.
 */
public void
remove_adj(string str)
{
#ifdef LOG_ADJ
    if (this_player() != this_object())
        SECURITY->log_syslog(LOG_ADJ, ctime(time()) + ": "
          + query_real_name() + " removed adj " + str + " by "
          + this_player()->query_real_name() + "\n");
#endif

    ::remove_adj(str);
    przymiotniki = query_przymiotniki();
}

/*
 * Function name:   query_real_name
 * Opis:	    Zwraca prawdziwe imie gracza w podanym przypadku
 * Returns:         Imie gracza w danym przypadku malymi literami
 */
nomask public varargs string
query_real_name(int przyp)
{
    if (!imiona)
       return name;
       
    return imiona[przyp];
}

/*
 * Function name: set_wiz_level
 * Description  : This function makes the player ask SECURITY whether it
 *                is a wizard or a mortal player.
 */
public nomask void
set_wiz_level()
{
    wiz_level = (SECURITY->query_wiz_rank(query_real_name()) > WIZ_MORTAL);
}

/*
 * Function name: query_wiz_level
 * Description  : This function tests whether this player is a wizard of
 *                a mortal player. Note that it only returns 1/0. In order
 *                to figure out the rank of the player, check:
 *
 *                SECURITY->query_wiz_rank(string name);
 *
 *                and for the level within this rank, call:
 *
 *                SECURITY->query_wiz_level(string name);
 *
 * Returns      : int 1/0 - wizard/mortal player.
 */
public int
query_wiz_level()
{
    return wiz_level;
}

/*
 * Password
 */

/*
 * Function name: set_password
 * Description  : Set the password of a player. This is an internal function
 *                and cannot be called externally.
 * Arguments    : string p - the new password string.
 */
nomask static void     
set_password(string p)
{
    password = p;

#ifdef FORCE_PASSWORD_CHANGE
    password_time = time();
#endif FORCE_PASSWORD_CHANGE
}

/*
 * Function name: query_password
 * Description  : This function will reveal the password of the player,
 *                but _only_ to SECURITY. Otherwise you shall have to
 *                call the function match_password() to see whether the
 *                password matches.
 * Returns      : string - the password.
 */
nomask public string
query_password()
{
    if (previous_object() != find_object(SECURITY))
    {
	return "to tajemnica ;-)";
    }

    return password;
}

/*
 * Function name: match_password
 * Description  : This function matches the password of a player with an
 *                arbitrary string someone claims is the password of this
 *                player. NOTE that if the player has NO password, it always
 *                matches.
 * Arguments    : string p - the password to check.
 * Returns      : int - true/false.
 */
nomask public int
match_password(string p)
{
    if (!strlen(password))
    {
	return 1;
    }

    return (password == crypt(p, password));
}

/*
 * Function name: query_password_time
 * Description  : This function will return the time the password was last
 *                changed/set.
 * Returns      : int - the time.
 */
#ifdef FORCE_PASSWORD_CHANGE
nomask int
query_password_time()
{
    return password_time;
}
#endif FORCE_PASSWORD_CHANGE

/*
 * Function name:   set_player_file
 * Description:     Set the playerfile that is loaded when the player enters
 *                  the game. The lagality of the file is checked against
 *		    legal_player in the 'login_new_player' file defined in
 *		    <config.h>
 * Arguments:       f: The filename-string.
 */
nomask public void
set_player_file(string f)
{
    object ob;
    
    catch(f->teleledningsanka());

    ob = find_object(f);
    if (ob && LOGIN_NEW_PLAYER->legal_player(ob))
    {
	player_file = f;
    }
}

/*
 * Function name:   query_player_file
 * Description:     Gives back the playerfile that is loaded when the player
 *                  enters the game.
 * Returns:         The string with the filename
 */
nomask public string
query_player_file()
{
    return player_file;
}

/*
 * Function name:   query_auto_load
 * Description:     Gives back the array of strings with objects to autoload
 *                  when the player enters the game.
 * Returns:         An array of strings describing what objects to load.
 */
nomask string *
query_auto_load()
{
    return auto_load;
}

/*
 * Function name:   query_recover_list
 * Description:     Gives back the array of strings with objects to recover
 *                  when the player enters the game.
 * Returns:         An array of strings describing what objects to recover,
 *		    along with the 
 */
nomask string *
query_recover_list()
{
    return recover_list;
}

/*
 * Function name:   query_path
 * Description:     Gives back the path of the current directory.
 * Returns:         The path string
 */
public nomask string
query_path()
{
    if (path) return path;
    else if (query_wiz_level())
	path = (string) SECURITY->query_wiz_path(query_real_name());
    return path;
}

/*
 * Function name:   query_mailaddr
 * Description:     Gives back the Email address of a player.
 * Returns:         The Email address string
 */
public nomask string
query_mailaddr()
{
    return mailaddr;
}

/*
 * Function name:   query_cap_name
 * Description:     Gives back the capitalized name of a player.
 * Returns:         The capitalized name string
 */
public nomask string
query_cap_name()
{
    return cap_name;
}

/*
 * Function name:   query_default_start_location
 * Description:     Gives back the default start location of a player when
 *                  she enters the game.
 * Returns:         The string with the filename of the startup-room.
 */
public string
query_default_start_location()
{
    return default_start_location;
}

/*
 * Function name:   query_temp_start_location
 * Description:     Gives back the temporary start location of a player when
 *                  she enters the game.
 * Returns:         The string with the filename of the startup-room.
 */
public nomask string
query_temp_start_location()
{
    return temp_start_location;
}

/*
 * Function name:   query_remember_name
 * Description:     Gives back a mapping with all names that a player has
 *                  remembered.
 * Returns:         The mapping with all names.
 */
public nomask mapping
query_remember_name()
{
    if (mappingp(m_remember_name))
	return ([ ]) + m_remember_name;
    else
	return ([ ]);
}

/*
 * Function name:   query_age
 * Description:     Gives the age of the living in heart_beats.
 * Returns:         The age.
 */
public nomask int
query_age()
{
    age_heart += ((time() - age_time) / 2);
    age_time = time();

    return age_heart;
}

#ifndef NO_SKILL_DECAY
/*
 * Function name:   query_decay_time
 * Description:     Gives the time since last decay of skills
 * Returns:         The time.
 */
public nomask int
query_decay_time()
{
    /* Due to a bug decay_time_acc has become negative in some players.
       This will fix that. */
    if (decay_time_acc < 0)
    {
	decay_time_acc = 10000;
    }
    
    /* Only increment if we're actually decaying skills */
    if (!query_skill_decay())
	return decay_time_acc;

    decay_time_acc += (time() - decay_time);
    decay_time = time();

    return decay_time_acc;
}

/*
 * Function name:   reset_decay_time
 * Description:     Reset the decay time to the remainder of time
 *                  after a full interval.
 */
static nomask void
reset_decay_time()
{
    /*
     * Use a loop since it the clock might have ticked untended during
     * the time the player wasn't decaying skills at all. Leave only a
     * reminder <= the total interval.
     */
    if (decay_time_acc < 0)
    {
	decay_time_acc = 10000;
    }
    
    if (decay_time_acc > 432000)
    {
	decay_time_acc = 432200;
    }
    
    while (decay_time_acc >= SKILL_DECAY_INTERVAL)
	decay_time_acc -= SKILL_DECAY_INTERVAL;
}
#endif NO_SKILL_DECAY

/*
 * Function name:   query_tot_value
 * Description:     Gives back the total value of all that is carried
 * Returns:         the total carried value
 */
public nomask int
query_tot_value()
{
    return tot_value;
}

/*
 * Function name:   set_auto_load
 * Description:     Sets the array with autoload strings. Auytoload strings
 *                  look like "<file>:<arg>".
 * Arguments:       arr: An array with autoload strings.
 */
nomask void
set_auto_load(string *arr)
{
    auto_load = arr;
}

/*
 * Function name:   set_recover_list
 * Description:     Sets the array with recover strings. Recover strings
 *                  look like "<file>:<arg>".
 * Arguments:       arr: An array with autoload strings.
 */
nomask void
set_recover_list(string *arr)
{
    recover_list = arr;
}

/*
 * Function name:   set_path
 * Description:     Sets the current path of a player.
 * Arguments:       str: Pathstring
 */
public nomask void
set_path(string str)
{
    path = str;
}

/*
 * Function name:   set_mailaddr
 * Description:     Sets the Email address of a player
 * Arguments:       addr: The Email address string
 */
public nomask void
set_mailaddr(string addr)
{
    string *parts = explode(addr, "\n");

    if (sizeof(parts) > 1)
    {
	write("Email nie powinien zawierac zadnych spacji. Pobierana jest " +
 	    "wylacznie czesc sprzed pierwszej spacji.\n");
	addr = parts[0];
    }

    if (strlen(addr) > 65)
    {
	write("Zbyt dlugi adres. Ucinam.\n");
	addr = extract(addr, 0, 64);
    }

    write("Ustawiam na: " + addr + "\n");
    mailaddr = addr;
}

/*
 * Function name:   set_cap_name
 * Description:     Sets the capitalized name of a player. This is derived
 *                  from query_real_name(), which returns a lowercase name.
 */
public nomask void
set_cap_name()
{
    cap_name = capitalize(query_real_name()); 
}

/*
 * Function name:   set_default_start_location
 * Description:     Sets a new default startup location for a player. The
 *                  default startup-location must have been approved of by
 *                  an archwizard or keeper.
 * Arguments:       str: the startup room's filename string
 * Returns:         0 if the string was not an accepted location,
 *                  1 when set.
 */
public nomask int
set_default_start_location(string str)
{
    if (file_name(previous_object()) != SECURITY &&
        !query_wiz_level() && 
        !wildmatch("*jr", query_real_name()) &&
	SECURITY->check_def_start_loc(str) < 0)
	return 0;
	
    if (catch(str->teleledningsanka()) || !find_object(str))
	return 0;

    default_start_location = str;
    return 1;
}

/*
 * Function name:   set_temp_start_location
 * Description:     Sets a new temporary startup location for a player. The
 *                  next time the player logs in, she will enter the game at
 *                  the set temporary location, which is then discarded. The
 *                  temporary startup-location must have been approved of by
 *                  an archwizard or keeper.
 * Arguments:       str: the startup room's filename string
 * Returns:         0 if the string was not an accepted location,
 *                  1 when set.
 */
public nomask int
set_temp_start_location(string str)
{
    if (strlen(str) && (SECURITY->check_temp_start_loc(str) < 0))
	return 0;
    temp_start_location = str;
    return 1;
}

/*
 * Function name:   set_remember_name
 * Description:     Sets the people who are remembered by a player.
 * Arguments:       nlist: A mapping with names of remembered players
 */
public nomask void
set_remember_name(mapping nlist)
{
    m_remember_name = ([ ]) + nlist;
}

/*
 * Function name:   set_notify
 * Description:     Set notify status
 * Arguments:	    flag - The current status
 */
public nomask void
set_notify(int flag)
{
    notify = flag;
}

/*
 * Function name:    query_notify
 * Description:	     Query the notify status
 * Returns:	     The setting
 */
public nomask int
query_notify()
{
    return notify;
}

public nomask int
add_notified(string name)
{
    if (m_sizeof(m_notified) > 10)
	return 0;
	
    if (!mappingp(m_notified))
	m_notified = ([]);

    m_notified[name] = 1;
    return 1;
}

public nomask int
remove_notified(string name)
{
    if (!mappingp(m_notified))
	return 0;

    m_notified = m_delete(m_notified, name);
    return 1;
}

public nomask mixed
query_notified(string arg)
{
    if (!mappingp(m_notified))
    {
	if (!arg)
	    return ({});
	else
	    return 0;
    }

    if (!arg)
	return m_indices(m_notified);
    return m_notified[arg];
}

/*
 * Function name:   set_tot_value
 * Description:     Sets the total value carried by a player
 * Arguments:       val: The carried value
 */
static nomask void
set_tot_value(int val)
{
    tot_value = val;
}

static nomask void
inc_death_count()
{
    death_count++;
}

public nomask int
query_death_count()
{
    return death_count;
}

/*
 * Function name:   set_login_time
 * Description:     sets the time of the login
 */
static nomask void
set_login_time() 
{ 
    login_time = time(); 
}

/*
 * Function name:   query_login_time
 * Description:     Gives back the login-time.
 * Returns:         The login-time.
 */
public nomask int
query_login_time() 
{ 
    return login_time; 
}

/*
 * Function name:   set_login_from
 * Description:     Sets from which site the player is logged in.
 */
public nomask void
set_login_from() 
{ 
    login_from = query_ip_name(this_object()); 
}

/*
 * Function name:   query_login_from
 * Description:     shows from which site the player is logged in
 * Returns:         A string with the site name.
 */
public nomask string
query_login_from() 
{ 
    return login_from; 
}

/*************************************************************************
 * 
 * Auto shadow routines.
 *
 */

/*
 * Function name:   add_autoshadow
 * Description:	    Add a shadow to the shadow list.
 * Arguments:       shadowfile: the shadow-object or the filename of the
 *                              shadow-object.
 * Returns:         1 if the adding was successfull,
 *                  0 otherwise.
 */
nomask public int
add_autoshadow(mixed shadowfile)
{
    string *sh;

    if (query_wiz_level())
    {
	if (geteuid(previous_object()) != geteuid(this_object()) &&
	    geteuid(previous_object()) != ROOT_UID)
	    return 0;
    }

    if (objectp(shadowfile))
	shadowfile = MASTER_OB(shadowfile) + ":";

    sh = explode(shadowfile, ":");
    if (sizeof(sh) > 1 && strlen(sh[1]) > 80)
	return 0;

    /* There can only be one. */
    while(remove_autoshadow(shadowfile));

    if (!sizeof(auto_shadow))
	auto_shadow = ({ shadowfile });
    else
	auto_shadow += ({ shadowfile });
    return 1;
}

/*
 * Function name:   remove_autoshadow
 * Description:	    Remove a shadow from the shadow list
 * Arguments:       shadowfile: The shadow-object or the filename of the
 *                              shadow_object.
 * Returns:         1 if the removing was successfull,
 *                  0 otherwise
 */
nomask public int
remove_autoshadow(mixed shadowfile)
{
    string *sh;
    int i;

    if (objectp(shadowfile))
	shadowfile = MASTER_OB(shadowfile);

    sh = explode(shadowfile, ":");

    for (i = 0 ; i < sizeof(auto_shadow) ; i++)
    {
	if (sh[0] == explode(auto_shadow[i], ":")[0])
	{
	    auto_shadow = exclude_array(auto_shadow, i, i);
	    return 1;
	}
    }
    return 0;
}

/*
 * Function name:   query_autoshadow_list
 * Description:	    Gives back the list of autoshadow object filenames.
 * Returns:         The autoshadow list
 */
nomask public string *
query_autoshadow_list()
{
    return secure_var(auto_shadow);
}


/*************************************************************************
 * 
 * Bit handling routines.
 *
 */

/*
 * Function name:   set_bit
 * Description:     Set a given bit in a given group. The effective userid of
 *                  the object calling this function is used to find which
 *                  domain is setting the bits.
 * Arguments:       group: An integer 0-4
 *                  bit:   An integer 0-19
 * Returns:         1 if the bit was successfully set, 0 otherwise.
 */
public int
set_bit(int group, int bit)
{
    int index, num, domain, domain_flag;
    string euid, czas, str;
    object pobj;

    if (group < 0 || group > 4 || bit < 0 || bit > 19)
	return 0;

    pobj = previous_object();
    euid = geteuid(pobj);
	
    domain = (int)SECURITY->query_domain_number(euid); /* If euid == domain */
    if (domain < 0)
	domain = SECURITY->query_domain_number(SECURITY->query_wiz_dom(euid));
    else
	domain_flag = 1;
    if (domain < 0)
	return 0;

    num = domain * 5 + group;
    index = member_array(num, bit_wizlist);
    if (index < 0)
    {
	bit_wizlist += ({ num });
	bit_bitlist += ({ 0 });
	index = sizeof(bit_wizlist) - 1;
    }
    bit_bitlist[index] |= (1 << bit);
    
    if (file_name(pobj) == WIZ_CMD_APPRENTICE)
	pobj = previous_object(-2);
	
    czas = ctime(time())[4..];
    str = sprintf("%s:   set %s euid: %s B:%d, G:%d, D:%d w %s; ",
	czas[14..],
	interactive(pobj) ? upper_case(pobj->query_real_name()) : file_name(pobj),
	(domain_flag ? euid : upper_case(euid)), bit, group, domain,
	capitalize(query_real_name()));

    czas = czas[0..11];
    index = -1;
    while (czas[++index] == ' ')
        ;
    str += czas[index..] + "\n";

    SECURITY->log_syslog("BIT", str);

    return 1;
}

/*
 * Function name:   clear_bit
 * Description:     Clear a given bit in a given group. The effective userid
 *                  of the object calling this function is used to find which
 *                  domain is clearing the bits.
 * Arguments:       group: An integer 0-4
 *                  bit:   An integer 0-19
 * Returns:         1 if the bit was successfully cleared, 0 otherwise.
 */
public int
clear_bit(int group, int bit)
{
    int index, num, domain, domain_flag;
    string euid, czas, str;
    object pobj;

    if (group < 0 || group > 4 || bit < 0 || bit > 19)
	return 0;
	
    pobj = previous_object();
    euid = geteuid(pobj);

    domain = (int)SECURITY->query_domain_number(euid); /* If euid == domain */
    if (domain < 0)
	domain = SECURITY->query_domain_number(SECURITY->query_wiz_dom(euid));
    else
	domain_flag = 1;
    if (domain < 0)
	return 0;

    num = domain * 5 + group;
    index = member_array(num, bit_wizlist);
    if (index < 0)
	return 1;

    bit_bitlist[index] &= (0xFFFFFFFF - (1 << bit));

    if (file_name(pobj) == WIZ_CMD_APPRENTICE)
	pobj = previous_object(-2);

    czas = ctime(time())[4..];
    str = sprintf("%s: clear %s euid: %s B:%d, G:%d, D:%d w %s; ",
	czas[14..],
	interactive(pobj) ? upper_case(pobj->query_real_name()) : file_name(pobj),
	(domain_flag ? euid : upper_case(euid)), bit, group, domain,
	capitalize(query_real_name()));

    czas = czas[0..11];
    index = -1;
    while (czas[++index] == ' ')
        ;
    str += czas[index..] + "\n";

    SECURITY->log_syslog("BIT", str);

    return 1;
}

/*
 * Function name:   test_bit
 * Description:     Test a given bit in a given group for a given domain. 
 * Arguments:       dom:   Domain which bits are to be tested
 *                  group: An integer 0-4
 *                  bit:   An integer 0-19
 * Returns:         1 if the bit was set, 0 if unset or failed to test.
 */
public int
test_bit(string dom, int group, int bit)
{
    int index, num;

    if (group < 0 || group > 4 || bit < 0 || bit > 19)
	return 0;
    num = (int)SECURITY->query_domain_number(dom);
    if (num < 0)
	return 0;
    num = num * 5 + group;
    index = member_array(num, bit_wizlist);
    if (index < 0)
	return 0;

    if(bit_bitlist[index] & (1 << bit))
	return 1;
    else
	return 0;
}

/*
 * Function name: pack_bits
 * Description  : This function packs the domain bits a player has set into
 *                a more efficient array to save memory.
 */
public nomask void
pack_bits()
{
    int index = -1;
    int size = sizeof(bit_wizlist);

    bit_savelist = ({ });
    while(++index < size)
    {
    	if (bit_bitlist[index])
    	{
	    bit_savelist +=
	    	({ bit_wizlist[index] | (bit_bitlist[index] << 12) });
	}
    }
}

/*
 * Function name: unpack_bits
 * Description  : This function gets the stuffed bits out of their array
 *                and puts them in the correct arrays.
 */
public nomask void
unpack_bits()
{
    if (!sizeof(bit_savelist))
    {
	bit_savelist = ({ });
	bit_wizlist = ({ });
	bit_bitlist = ({ });
    }
    else
    {
    	bit_wizlist = map(bit_savelist, &operator(&)(, 0xFFF));
    	bit_bitlist = map(bit_savelist, &operator(>>)(, 12));
    }
}

/*
 * Function name:   set_bit_array
 * Description:     Try to set the bit array. Only the ghost_player-file is
 *                  allowed to call this function.
 */
public void
set_bit_array(int *arr)
{
    if (file_name(previous_object()) != LOGIN_NEW_PLAYER)
    {
	return;
    }
    bit_savelist = arr;
    unpack_bits();
}

/*
 * Function name: query_bit_array
 * Description:   Gives back the global bit array of a player
 * Returns:       An array with packed bitstrings.
 */
public int *
query_bit_array()
{
    pack_bits();
    return bit_savelist + ({ });
}

/*
 * Function name: query_aliases
 * Description  : This returns the list of aliases of a player.
 * Arguments    : mapping - the indices are the alias-names and the
 *                          values the replacements.
 */
public mapping
query_aliases()
{
    if (mappingp(m_alias_list))
	return ([ ]) + m_alias_list;
    else
	return ([ ]);
}

/*
 * Function name: add_second
 * Description  : Add a second to the player list.
 * Arguments    : string second - the name of the second to add.
 * Returns      : int 1/0 - success/failure.
 */
public nomask int
add_second(string second)
{
    if ((this_interactive() != this_player()) &&
    	(WIZ_CHECK < WIZ_ARCH))
    {
    	return 0;
    }

    if (!strlen(second))
    {
	return 0;
    }

    if (!sizeof(seconds))
    {
	seconds = ({});
    }

    second = lower_case(second);
    if (!(SECURITY->exist_player(second)) ||
	SECURITY->query_wiz_rank(second))
    {
	return 0;
    }

    if (member_array(second, seconds) < 0)
    {
	seconds += ({ second });
    }
    
    SECURITY->log_syslog("SECONDS", ctime(time()) + ": Dodany second '" +
        second + "' dla '" + query_real_name() + "' przez '" + 
        this_interactive()->query_real_name() + "'.\n");

    return 1;
}

/*
 * Function name: remove_second
 * Description  : Remove a second from the player list.
 * Arguments    : string second - the name of the second to remove.
 * Returns      : int 1/0 - success/failure.
 */
public nomask int
remove_second(string second)
{
    if ((this_interactive() != this_object()) &&
    	(WIZ_CHECK < WIZ_ARCH))
    {
    	return 0;
    }

    if (!strlen(second))
    {
	return 0;
    }

    if (!sizeof(seconds))
    {
	return 0;
    }

    second = lower_case(second);
    if (member_array(second, seconds) < 0)
    {
	return 0;
    }

    seconds -= ({ second });

    SECURITY->log_syslog("SECONDS", ctime(time()) + ": Usuniety second '" +
        second + "' od '" + query_real_name() + "' przez '" + 
        this_interactive()->query_real_name() + "'.\n");


    return 1;
}

/*
 * Function name: query_seconds
 * Description  : Return the seconds array.
 * Returns      : string * - the seconds if you are allowed or 0.
 */
public nomask string *
query_seconds()
{
    if ((this_interactive() != this_object()) &&
    	(WIZ_CHECK < WIZ_ARCH) &&
    	(this_interactive()->query_real_name() !=
    	    SECURITY->query_domain_lord(SECURITY->query_wiz_dom(name))))
    {
    	return 0;
    }

    if (!sizeof(seconds))
    {
	seconds = ({ });
    }

    return secure_var(seconds);
}

/*
 * Function name: set_option
 * Description:	  Set a wizard option
 * Arguments:	  opt - the option
 *		  val - the value
 * Returns:	  1 - success, 0 - fail
 */
public nomask int
set_option(int opt, mixed val)
{
    if (this_interactive() != this_object())
	return 0;

    if (!stringp(options))
	options = " 20 80 0";
    
    switch (opt)
    {
    case OPT_MORE_LEN:
	if (val > 150 || val < 1)
	    return 0;
	options = sprintf("%3d", val) + options[3..];
	break;

    case OPT_SCREEN_WIDTH:
	if (val > 200 || val < 10)
	    return 0;
	options = options[..2] + sprintf("%3d", val) + options[6..];
	fixup_screen();
	break;

    case OPT_BRIEF:
	if (val)
	    options = efun::set_bit(options, OPT_BASE + 0);
	else
	    options = efun::clear_bit(options, OPT_BASE + 0);
	break;

    case OPT_ECHO:
	if (val)
	    options = efun::set_bit(options, OPT_BASE + 1);
	else
	    options = efun::clear_bit(options, OPT_BASE + 1);
	break;

    case OPT_WHIMPY:
	if (val > 99 || val < 0)
	    return 0;
	options = options[..5] + sprintf("%2d", val) + options[8..];
	break;

    case OPT_BLOOD:
	if (val)
	    options = efun::set_bit(options, OPT_BASE + 2);
	else
	    options = efun::clear_bit(options, OPT_BASE + 2);
	break;
	
    case OPT_UNARMED_OFF:
        if (val)
  	    options = efun::set_bit(options, OPT_BASE + 3);
        else
            options = efun::clear_bit(options, OPT_BASE + 3);
        break;
        
    case OPT_AUTO_PWD:
	if (val)
	    options = efun::set_bit(options, OPT_BASE + 18);
	else
	    options = efun::clear_bit(options, OPT_BASE + 18);
	break;

    default:
	return 0;
	break;
    }
    
    return 1;
}

/*
 * Function name: query_option
 * Description:	  Return a wizard option
 * Arguments:	  opt - the option
 * Returns:	  The option value
 */
public nomask int
query_option(int opt)
{
    if (!stringp(options))
	options = " 20 80";

    switch (opt)
    {
    case OPT_MORE_LEN:
	return atoi(options[..2]);
	break;

    case OPT_SCREEN_WIDTH:
	return atoi(options[3..5]);
	break;

    case OPT_BRIEF:
	return efun::test_bit(options, OPT_BASE + 0);
	break;

    case OPT_ECHO:
	return efun::test_bit(options, OPT_BASE + 1);
	break;

    case OPT_WHIMPY:
	return atoi(options[6..7]);
	break;

    case OPT_BLOOD:
	return efun::test_bit(options, OPT_BASE + 2);
	break;

    case OPT_UNARMED_OFF:
        return efun::test_bit(options, OPT_BASE + 3);
        break;

    case OPT_AUTO_PWD:
	return efun::test_bit(options, OPT_BASE + 18);
	break;

    default:
	return -1;
	break;
    }

    return 1;
}

#ifdef LOG_TITLE
public void
set_title(string t)
{
    if (this_player() != this_object())
        SECURITY->log_syslog(LOG_TITLE, ctime(time()) + ": "
          + query_real_name() + " new title " + t + " by "
          + this_player()->query_real_name() + "\n");

    ::set_title(t);
}
#endif

/*
 * Function name: 
 * Description:	  
 * Arguments:	  
 * Returns:	  
 */
