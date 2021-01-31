/*
 * /secure/master/fob.c
 *
 * Subpart of /secure/master.c
 *
 * Handles all domain, wizard and application administration in the game.
 */

#include "/sys/composite.h"
#include "/sys/const.h"

/*
 * These global variables are stored in the KEEPERSAVE.
 */
private int	dom_count;	/* The next domain number to be added */
private mapping m_domains;	/* The domain mapping */
private mapping m_wizards;	/* The wizard mapping */
private mapping m_applications;	/* The applications mapping */
private mapping m_trainees;	/* The list of trainees */
private mapping m_global_read;	/* The global read mapping */

/**************************************************************************
 *
 * The 'm_domains' mapping holds the domain name as index and an array with
 * the following items as value:
 *
 * [ 0] - The domain number. It is stored as tag in the player along with
 *        the quest bit set by the domain objects.
 *
 * [ 1] - The short name of the domain. This should be a unique abbreviation
 *        of exactly three characters.
 *
 * [ 2] - The name of the domain lord.
 *
 * [ 3] - The name of the steward of the domain, if any.
 *
 * [ 4] - An array holding the names of all members, including the lord and 
 *        the steward.
 *
 * [ 5] - The name of the domain madwand, if any.
 *
 * [ 6] - The max number of members the domain wants to accept.
 *
 * [ 7] - The number of quest xp given to mortals
 *
 * [ 8] - The number of combat xp given to mortals
 *
 * [ 9] - The number of commands executed in the domain.
 *
 **************************************************************************
 *
 * The 'm_wizards' mapping holds the wizard name as index and an array with
 * the following items as value:
 *
 * [ 0] - The rank of the wizard.
 *
 * [ 1]	- The level of the wizard.
 *
 * [ 2]	- The name of the wizard who last changed the level/rank.
 *
 * [ 3] - The time the level/rank was last changed.
 *
 * [ 4]	- The domain the wizard is a member of.
 *
 * [ 5]	- The name of the wizard who last changed the domain.
 *
 * [ 6] - The time the domain was last changed.
 *
 **************************************************************************
 *
 * The 'm_applications' mapping has the domain-names as index and the values
 * are arrays with the names of the wizards applying for membership.
 *
 **************************************************************************
 *
 * The 'm_global_read' mapping contains all wizards who have global read
 * access. Their names are the incides. The values is an array with the
 * following information:
 *
 * [ 0] - The wizards who granted the access.
 *
 * [ 1] - A short string describing the reason the wizard has global read.
 *
 **************************************************************************
 *
 * The 'm_trainees' mapping contains all wizards who have been marked as
 * trainee. Their names are the indices. The value is '1'. This mapping is
 * a mapping since that is quicker to test than membership of an array.
 *
 */

/* The maximum number of members a Lord may allow to his domain. */
#define DOMAIN_MAX	 (9)

/* These are the indices to the arrays in the domain-mapping. */
#define FOB_DOM_NUM	 (0)
#define FOB_DOM_SHORT    (1)
#define FOB_DOM_LORD	 (2)
#define FOB_DOM_STEWARD  (3)
#define FOB_DOM_MEMBERS	 (4)
#define FOB_DOM_MADWAND  (5)
#define FOB_DOM_MAXSIZE	 (6)
#define FOB_DOM_QXP	 (7)
#define FOB_DOM_CXP	 (8)
#define FOB_DOM_CMNDS    (9)

/* These are the indices to the arrays in the wizard-mapping. */
#define FOB_WIZ_RANK	 (0)
#define FOB_WIZ_LEVEL	 (1)
#define FOB_WIZ_CHLEVEL	 (2)
#ifdef FOB_KEEP_CHANGE_TIME
#define FOB_WIZ_CHLTIME  (3)
#define FOB_WIZ_DOM	 (4)
#define FOB_WIZ_CHDOM	 (5)
#define FOB_WIZ_CHDTIME  (6)
#else
#define FOB_WIZ_DOM	 (3)
#define FOB_WIZ_CHDOM	 (4)
#endif FOB_KEEP_CHANGE_TIME

/*
 * Function name: load_fob_defaults
 * Description  : This function is called from master.c when the KEEPERAVE
 *                file cannot be found. The defined values can be found in
 *                config.h.
 */
static void
load_fob_defaults()
{
    m_wizards = DEFAULT_WIZARDS;
    m_domains = DEFAULT_DOMAINS;
    dom_count = m_sizeof(m_domains);
    m_applications = ([ ]);
    m_global_read = ([ ]);
    m_trainees = ([ ]);
}

/*
 * Function name: getwho
 * Description  : This function gets the name of the interactive command
 *                giver. The euid of the interactive player and the euid
 *                of the previous object must be equal in order to function.
 * Returns      : string - the name or "" in case of inconsistencies.
 */
static string
getwho()
{
    string euid;

    euid = geteuid(this_interactive());
    return ((euid == geteuid(previous_object())) ? euid : "");
}

/************************************************************************
 *
 * The domain administration code.
 *
 */

/*
 * Function name: query_domain_name
 * Description  : Find the domain name from the number.
 * Arguments    : int number - the domain number.
 * Returns      : string - the domain name if found, else 0.
 */
string
query_domain_name(int number)
{
    string *domains;
    int index;

    domains = m_indices(m_domains);
    index = sizeof(domains);

    while(--index >= 0)
    {
        if (m_domains[domains[index]][FOB_DOM_NUM] == number)
        {
            return domains[index];
        }
    }

    return 0;
}

/*
 * Function name: query_domain_number
 * Description  : Find the number of a domain.
 * Arguments    : string dname - the name of the domain.
 * Returns      : int - the number if found, -1 otherwise.
 */
int
query_domain_number(string dname)
{
    dname = capitalize(dname);

    if (!sizeof(m_domains[dname]))
    {
	return -1;
    }

    return m_domains[dname][FOB_DOM_NUM];
}

/*
 * Function name: query_domain_short
 * Description  : Find the short name of the domain.
 * Arguments    : string dname - the name of the domain.
 * Returns      : string - the short name if found, "" otherwise.
 */
string
query_domain_short(string dname)
{
    dname = capitalize(dname);

    if (!sizeof(m_domains[dname]))
    {
	return "";
    }

    return m_domains[dname][FOB_DOM_SHORT];
}

/*
 * Function name: set_domain_short
 * Description  : Set the short name of a domain. The short name is an
 *                appropriate abbreviation of exactly three characters of
 *                the domain-name. This function may only be called from
 *                the Lord soul, so we do not have to make additional
 *                checks.
 * Arguments    : string dname - the name of the domain (capitalized).
 *                string sname - the short name of the domain (lower case).
 * Returns      : int 1/0 - success/failure.
 */
int
set_domain_short(string dname, string sname)
{
    /* This function may only be called from the Lord soul. */
    if (!CALL_BY(WIZ_CMD_LORD))
    {
	return 0;
    }

    m_domains[dname][FOB_DOM_SHORT] = sname;
    save_master();
    return 1;
}

/*
 * Function name: query_domain_lord
 * Description  : Find the lord of a domain.
 * Arguments    : string dname - the name of the domain.
 * Returns      : string the domain lord if found, "" otherwise.
 */
string
query_domain_lord(string dname)
{
    dname = capitalize(dname);

    if (!sizeof(m_domains[dname]))
    {
	return "";
    }

    return m_domains[dname][FOB_DOM_LORD];
}

/*
 * Function name: query_domain_steward
 * Description  : Find the steward of a domain.
 * Arguments    : string dname - the name of the domain.
 * Returns      : string the steward if the domain exists, "" otherwise.
 */
string
query_domain_steward(string dname)
{
    dname = capitalize(dname);

    if (!sizeof(m_domains[dname]))
    {
	return "";
    }

    return m_domains[dname][FOB_DOM_STEWARD];
}

/*
 * Function name: query_domain_members
 * Description  : Find and return the member array of a given domain.
 * Arguments    : string dname - the domain.
 * Returns      : string * - the array if found, ({ }) otherwise.
 */
string *
query_domain_members(string dname)
{
    dname = capitalize(dname);

    if (!sizeof(m_domains[dname]))
    {
	return ({ });
    }

    return secure_var(m_domains[dname][FOB_DOM_MEMBERS]);
}

/*
 * Function name: query_domain_madwand
 * Description  : Find and return the name of the madwand of the domain.
 *                When there is no madwand support, always returns "".
 * Arguments    : string dname - the domain.
 * Returns      : string - the name of the madwand if found, "" otherwise.
 */
string
query_domain_madwand(string dname)
{
#ifdef FOB_MADWAND_SUPPORT
    dname = capitalize(dname);

    if (!sizeof(m_domains[dname]))
    {
	return "";
    }

    return m_domains[dname][FOB_DOM_MADWAND];
#else
    return "";
#endif FOB_MADWAND_SUPPORT
}

/*
 * Function name: query_domain_list
 * Description  : Return a list of all domains.
 * Returns      : string * - the list.
 */
string *
query_domain_list()
{
    return m_indices(m_domains);
}

/*
 * Function name: set_domain_max
 * Description  : Set maximum number of wizards wanted. This function may
 *                only be called from the Lord soul so we do not have to
 *                make any additional checks.
 * Arguments    : string dname - the capitalized domain name.
 *                int    max   - the new maximum.
 * Returns      : int 1/0 - success/failure.
 */
int
set_domain_max(string dname, int max)
{
    /* May only be called from the Lord soul. */
    if (!CALL_BY(WIZ_CMD_LORD))
    {
	return 0;
    }

    m_domains[dname][FOB_DOM_MAXSIZE] = max;
    save_master();
    return 1;
}

/*
 * Function name: query_domain_max
 * Description  : Find and return the maximum number of wizards wanted. 
 * Arguments    : string dname - the name of the domain.
 * Returns      : int - the max number if found, -1 otherwise.
 */
int
query_domain_max(string dname)
{
    dname = capitalize(dname);

    if (!sizeof(m_domains[dname]))
    {
	return -1;
    }

    return m_domains[dname][FOB_DOM_MAXSIZE];
}

/*
 * Function name: query_default_domain_max
 * Description  : Get the default maximum number of members in a domain.
 * Returns      : int - the value defined in DOMAIN_MAX.
 */
int
query_default_domain_max()
{
    return DOMAIN_MAX;
}

/*
 * Function name: make_domain
 * Description  : Create a new domain.
 * Arguments    : string dname - the domain name.
 *                string sname - the short domain name.
 *                string wname - the name of the domain lord.
 * Returns      : int 1/0 - success/failure.
 */
int
make_domain(string dname, string sname, string wname)
{
    string cmder;

    /* Only accept calls from the arch command soul. */
    if (!CALL_BY(WIZ_CMD_ARCH))
    {
	return 0;
    }

    cmder = getwho();
    if (sizeof(m_domains[dname]))
    {
	notify_fail("The domain " + dname + " already exists.\n");
	return 0;
    }

    if ((strlen(dname) > 11) ||
	(strlen(sname) != 3))
    {
	notify_fail("The domain name must be 11 characters long at most and " +
		    "the short name must be exactly three characters long.\n");
	return 0;
    }

    if (sizeof(filter(m_indices(m_domains),
		      &operator(==)(sname) @ query_domain_short)))
    {
	notify_fail("The short domain name " + sname +
		    " is already in use.\n");
    }

    if ((!pointerp(m_wizards[wname])) ||
	(m_wizards[wname][FOB_WIZ_RANK] != WIZ_APPRENTICE))
    {
	notify_fail("The player " + capitalize(wname) +
		    " is not an apprentice.\n");
	return 0;
    }

    set_auth(this_object(), "root:root");

    switch(file_size("/d/" + dname))
    {
    case -1:
	/* Check for old discarded instances of the domain. If there is one,
	 * we must use it or else you shall have to rename or remove it
	 * first.
	 */
	if (file_size(DISCARD_DOMAIN_DIR + "/" + dname) == -2)
	{
	    if (rename((DISCARD_DOMAIN_DIR + "/" + dname), ("/d/" + dname)))
	    {
		write("Revived the old discarded " + dname + ".\n");
	    }
	    else
	    {
		write("Failed to discard old instance of " + dname +
		      ". Do this manually or rename it.\n");
		return 1;
	    }
	}
	else
	{
	    /* Create the domain subdirectory. */
	    if (!mkdir("/d/" + dname))
	    {
		write("Can not create the domain subdir!\n");
		return 1;
	    }

	    write("Created directory for new domain " + dname + ".\n");
	}
	break;

    case -2:
	write("Directory for " + dname + " already exists.\n");
	break;

    default:
	write("There is a file named /d/" + dname +
	      ", making it impossible to create the domain.\n");
	return 1;
    }

    /* Add the domain entry to the domain mepping. */
    m_domains[dname] =
	({ dom_count++, sname, wname, "", ({ }), "", DOMAIN_MAX, 0, 0, 0 });

    /* Make the apprentice a lord. This will also save the domain mapping. */
    add_wizard_to_domain(dname, wname, cmder);
    do_change_rank(wname, WIZ_LORD, cmder);

    return 1;
}

/*
 * Function name: remove_domain
 * Description  : Remove a domain, demote the wizards, move the code.
 * Arguments    : string dname - the domain to remove.
 * Returns      : int 1/0 - success/failure.
 */
int
remove_domain(string dname)
{
    int    index;
    int    size;
    string cmder;
    string *members;

    /* May only be called from the arch soul. */
    if (!CALL_BY(WIZ_CMD_ARCH))
    {
	return 0;
    }

    cmder = getwho();
    dname = capitalize(dname);
    if (!sizeof(m_domains[dname]))
    {
	notify_fail("No such domain '" + dname + "'.\n");
	return 0;
    }

    if (dname == WIZARD_DOMAIN)
    {
	write("The domain " + WIZARD_DOMAIN + " cannot be removed.\n");
	return 1;
    }

    if (file_size(DISCARD_DOMAIN_DIR + "/" + dname) != -1)
    {
	notify_fail("There already is something called " + DISCARD_DOMAIN_DIR +
		    "/" + domain + ". Rename or remove this first.\n");
	return 0;
    }

    members = m_domains[dname][FOB_DOM_MEMBERS];
    index = -1;
    size = sizeof(members);
    while(++index < size)
    {
	/* Don't demote mages, arches and keepers. */
	switch(m_wizards[members[index]][FOB_WIZ_RANK])
	{
	case WIZ_MAGE:
	case WIZ_ARCH:
	case WIZ_KEEPER:
	    break;

	default:
	    do_change_rank(members[index], WIZ_APPRENTICE, cmder);
	    write("Demoted " + capitalize(members[index]) +
		  " to apprentice.\n");
	}

	/* Though expel everyone. */
	add_wizard_to_domain("", members[index], cmder);
	write("Expelled " + capitalize(members[index]) + ".\n");
    }

    /* Remove all sanctions given out to or received by the domain. */
    remove_all_sanctions(dname);

    /* Move the domain directory to the discarded domain directory. */
    mkdir(DISCARD_DOMAIN_DIR);
    if (!rename("/d/" + dname, DISCARD_DOMAIN_DIR + "/" + dname))
    {
	write("Unable to move the domain directory to " + DISCARD_DOMAIN_DIR +
	      ".\n");
	return 1;
    }

    /* Delete the domain from the domain mapping. */
    m_domains = m_delete(m_domains, dname);
    save_master();

    write("You have just obliterated " + dname + ".\n");
    return 1;
}

/*
 * Function name: tell_domain
 * Description  : Tell something to all in the domain save one, who gets a
 *                special message.
 * Arguments    : string dname - the domain.
 *		  string wname - special message to this wiz.
 *		  string wmess - wizard message.
 *		  string dmess - domain message.
 */
static void 
tell_domain(string dname, string wname, string wmess, string dmess)
{
    object wiz;
    string *wlist;
    int    size;

    if (objectp(wiz = find_player(wname)))
    {
	tell_object(wiz, wmess);
    }

    wlist = (string *)m_domains[dname][FOB_DOM_MEMBERS] - ({ wname });
    size = sizeof(wlist);
    while(--size >= 0)
    {
	if (objectp(wiz = find_player(wlist[size])))
	{
	    tell_object(wiz, dmess);
	}
    }
}

/************************************************************************
 *
 * The madwand administration code.
 *
 */

/*
 * Function name: transform_mortal_into_wizard
 * Description  : This function gives the mortal the apprentice scroll and
 *                alters the start location of the player, both if they
 *                player is logged in or now.
 * Arguments    : string wname - the name of the mortal.
 *                string cmder - the name of the wizard doing the honour.
 */
static void
transform_mortal_into_wizard(string wname, string cmder)
{
    object  wizard;
    object  scroll;
    object *players;
    mapping playerfile;
    int     fingered;

    /* Update the wizard-mapping. This just adds an empty slot for the
     * player. He isn't apprentice yet.
     */
    m_wizards[wname] = ({ WIZ_MORTAL, WIZ_RANK_START_LEVEL(WIZ_MORTAL),
			  cmder, "", cmder });
    save_master();

    if (objectp(wizard = find_player(wname)))
    {
	if (catch(scroll = clone_object(APPRENTICE_SCROLL_FILE)))
	{
	    write("Error cloning the apprentice scroll.\n");
	    tell_object(wizard, "Error cloning the apprentice scroll.\n");
	}
	else
	{
	    write("Apprentice scroll moved to " + capitalize(wname) + ".\n");
 	    tell_object(wizard, "\n\nDostal" + wizard->koncowka("es", "as") +
 	        " kartke zawierajaca bardzo cenne informacje. Koniecznie " +
 		"ja przeczytaj!\n");
	    scroll->move(wizard, 1);
	}
	wizard->set_wiz_level();
	wizard->set_default_start_location(WIZ_ROOM);
	wizard->save_me(1);	
    }
    else
    {
	playerfile = restore_map(PLAYER_FILE(wname));

	if (!pointerp(playerfile["auto_load"]))
	{
	    playerfile["auto_load"] = ({ });
	}

	playerfile["auto_load"] += ({ APPRENTICE_SCROLL_FILE });
	playerfile["default_start_location"] = WIZ_ROOM;

	save_map(playerfile, PLAYER_FILE(wname));

	fingered = 1;
	wizard = SECURITY->finger_player(wname);
    }

    players = users() - ({ this_interactive(), wizard });
    players -= QUEUE->queue_list(0);

    players->catch_tell("Niebo rozswietla szkarlatny blask rozdartej " +
        "osnowy magii. Zdajesz sobie sprawe, ze " + capitalize(wname) +
        " wstapil" + (wizard->query_gender() == G_MALE ? "" : "a") + 
        " do Krainy Niesmiertelnych.\n");

    SECURITY->log_public("NEW_WIZ",
        sprintf("%s   %-11s   Avg = %3d; Age = %3d\n", ctime(time()),
        capitalize(wname), wizard->query_average_stat(),
        (wizard->query_age() / 43200)), -1);

    /* Clean up after ourselves. */
    if (fingered)
    {
	do_debug("destroy", wizard);
    }
}

#ifdef FOB_MADWAND_SUPPORT
/*
 * Function name: draft_madwand
 * Description  : This function is used to draft a madwand to a domain.
 *                A madwand is a mortal player who is changed into a
 *                wizard, linked to the domain (s)he is added to. When
 *                expelled from the domain, the madwand reverts to mortal
 *                again with the previous status.
 * Arguments    : string dname - the domain to draft the madwand to.
 *                string wname - the mortal to make wizard.
 *                string cmder - who does it.
 * Returns      : int 1/0 - success/failure.
 */
static int
draft_madwand(string dname, string wname, string cmder)
{
    object player;

    /* Only mortals can be made into madwand. */
    if (m_wizards[wname])
    {
	notify_fail("Only mortals can be made into a madwand and " +
		    capitalize(wname) + " is a wizard.\n");
	return 0;
    }

    /* No such domain. */
    if (!m_domains[dname])
    {
	notify_fail("There is no domain named " + dname + ".\n");
	return 0;
    }

    /* Domain already has a madwand. */
    if (strlen(m_domains[dname][FOB_DOM_MADWAND]))
    {
	notify_fail("The domain " + dname + " already has a madwand, " +
		    capitalize(m_domains[dname][FOB_DOM_MADWAND]) + ".\n");
	return 0;
    }

    /* Domain is full. */
    if (sizeof(m_domains[dname][FOB_DOM_MEMBERS]) >=
	m_domains[dname][FOB_DOM_MAXSIZE])
    {
	notify_fail("The domain " + dname + " is already full with " +
		    sizeof(m_domains[dname][FOB_DOM_MEMBERS]) + " members.\n");
	return 0;
    }

    if (!objectp(player = find_player(wname)))
    {
	write("In order to make " + capitalize(wname) +
	      " a madwand, (s)he must be logged in.\n");
	return 1;
    }

    /* First save the mortals playerfile in a name.madwand */
    set_auth(this_object(), "root:root");
    rename(PLAYER_FILE(wname) + ".o", MADWAND_FILE(wname));

    /* Then make the mortal into a wizard. Note that this alone does not
     * make the mortal into apprentice, but that isn't necessary. We can
     * put him into the domain just like that.
     */
    transform_mortal_into_wizard(wname, cmder);

    /* Mark the wizard as madwand and make him a trainee. */
    m_domains[dname][FOB_DOM_MADWAND] = wname;
    
    /* Then draft the mortal wizard into the domain. First we make him/her
     * into a normal wizard though ;-)
     */
    do_change_rank(wname, WIZ_NORMAL, cmder);
    add_wizard_to_domain(dname, wname, cmder);

    log_file("MADWAND", ctime(time()) + " " + capitalize(wname) +
	     " drafted to " + dname + " by " + capitalize(cmder) + ".\n", -1); 
    return 1;
}

/*
 * Function name: expel_madwand
 * Description  : Expel a madwand from the domain (s)he is a member of.
 *                This will turn the wizard back into the mortal (s)he
 *                was before joining a domain as wizard.
 * Arguments    : string wname - the madwand to turn into mortal again.
 *                string cmder - the command giver.
 * Returns      : int 1/0 - success/failure.
 */
static int
expel_madwand(string wname, string cmder)
{
    string dname;
    object wiz;

    /* Remove the wizard from the domain. */
    dname = m_wizards[wname][FOB_WIZ_DOM];
    add_wizard_to_domain("", wname, cmder);

    /* Unmark the madwand as being the madwand of the domain. Then
     * remove him/her from the wizard-list.
     */
    m_domains[dname][FOB_DOM_MADWAND] = "";
    m_wizards = m_delete(m_wizards, wname);
    save_master();

    /* Remove all possible sanctions by the soon to be former madwand. */
    remove_all_sanctions(wname);

    /* Give the player the notice and then boot him/her. */
    if (objectp(wiz = find_player(wname)))
    {
	tell_object(wiz, "As you are expelled from " + dname +
		    " by " + capitalize(cmder) + ", you are changed back " +
		    "into mortal again. You shall regain your previous " +
		    "status if the old file is present. Else you shall " +
		    "have to create a new character.\n");
	wiz->quit();

	if (objectp(wiz))
	{
	    do_debug("destroy", wiz);
	}
    }

    /* Try to restore the mortal playerfile. */
    if (file_size(MADWAND_FILE(wname)) > 0)
    {
	rename(MADWAND_FILE(wname), PLAYER_FILE(wname) + ".o");
    }

    log_file("MADWAND", ctime(time()) + " " + capitalize(wname) +
	     " expelled from " + dname + " by " + capitalize(cmder) + "\n",
	     -1); 
    return 1;
}

/*
 * Function name: madwand_to_wizard
 * Description  : With this function a madwand can be changed into a normal
 *                wizard when he or she has proven worthy. This way the
 *                madwand flag can be removed without having to demote,
 *                expel, and squeeze the wizard.
 * Arguments    : string wname - the name of the wizard.
 * Returns      : int 1/0 - success/failure.
 */
int
madwand_to_wizard(string wname)
{
    string cmder;
    string dname;

    /* May only be called from the arch soul. */
    if (!CALL_BY(WIZ_CMD_ARCH))
    {
	return 0;
    }

    cmder = getwho();
    wname = lower_case(wname);
    if (!m_wizards[wname])
    {
	notify_fail("No wizard named " + capitalize(wname) + ".\n");
	return 0;
    }

    dname = m_wizards[wname][FOB_WIZ_DOM];
    if (m_domains[dname][FOB_DOM_MADWAND] != wname)
    {
	write(capitalize(wname) + " is not the madwand of the domain " +
	      dname + ".\n");
	return 1;
    }

    /* Remove the old madwand save-file if there is one. */
    if (file_size(MADWAND_FILE(wname)) >= 0)
    {
	rm(MADWAND_FILE(wname));
    }

    /* Unmark the wizard as madwand. */
    m_domains[dname][FOB_DOM_MADWAND] = "";
    save_master();
    log_file("MADWAND", ctime(time()) + " " + FORMAT_NAME(capitalize(wname)) +
	     " made into a real wizard by " + capitalize(cmder) + ".\n", -1);
    return 1;
}
#endif FOB_MADWAND_SUPPORT

/************************************************************************
 *
 * The wizard administration code.
 *
 */

/*
 * Function name: add_wizard_to_domain
 * Description  : Add a wizards to a domain and removes the wizard from his/
 *                her pervious domain. If you add a wizard to the domain "",
 *                it removes the wizard from his current domain.
 *                This is an interal function only that just adds or removes
 *                the people without additional checks. Those must have been
 *                made earlier. Note that this function does not alter the
 *                rank of the wizard. This must have been altered before.
 * Arguments    : string dname - the domain to add the wizard to. If "",
 *                               the wizard is removed from his/her domain.
 *		  string wname - the wizard to add/remove.
 *		  string cmder - who does it.
 * Returns      : int 1/0 - success/failure.
 */
static int
add_wizard_to_domain(string dname, string wname, string cmder)
{
    string old_domain;
    string old_dir;
    string new_dir;

    dname = capitalize(dname);
    wname = lower_case(wname);
    old_domain = m_wizards[wname][FOB_WIZ_DOM];

    /* Mages, arches and keepers cannot be without a domain. They will be
     * moved to WIZARD_DOMAIN if you try to remove them from their domain.
     */
    if ((!strlen(dname)) &&
	((m_wizards[wname][FOB_WIZ_RANK] == WIZ_MAGE) ||
	 (m_wizards[wname][FOB_WIZ_RANK] >= WIZ_ARCH)))
    {
	dname = WIZARD_DOMAIN;
    }

    m_wizards[wname][FOB_WIZ_DOM] = dname;
    m_wizards[wname][FOB_WIZ_CHDOM] = cmder;
#ifdef FOB_KEEP_CHANGE_TIME
    m_wizards[wname][FOB_WIZ_CHDTIME] = time();
#endif FOB_KEEP_CHANGE_TIME

    /* If the person leaves an old domain, update the membership and tell
     * the people.
     */
    if (strlen(old_domain))
    {
	m_domains[old_domain][FOB_DOM_MEMBERS] -= ({ wname });
	m_trainees = m_delete(m_trainees, wname);

        tell_domain(old_domain, wname, ("You are no longer a member of " +
            old_domain + ".\n"), (capitalize(wname) +
            " is no longer a member of " + old_domain + ".\n"));
    }
    else
    {
	old_domain = WIZARD_DOMAIN;
    }

    /* Leaving the domain and not joining another domain. */
    if (!strlen(dname))
    {
	save_master();

	return 1;
    }

    /* Joining a new domain means no more applications. */
    remove_all_applications(wname);

    /* Add him/her to the domain-list. */
    m_domains[dname][FOB_DOM_MEMBERS] += ({ wname });

    /* Mark him as trainee if necessary. */
    if ((dname != WIZARD_DOMAIN) &&
	(m_wizards[wname][FOB_WIZ_RANK] == WIZ_NORMAL))
    {
	m_trainees[wname] = 1;
    }

    tell_domain(dname, wname, ("You are now a member of " + dname +".\n"),
		capitalize(wname) + " is now a member of " + dname + ".\n");

    new_dir = "/d/" + dname + "/" + wname;
    old_dir = "/d/" + old_domain + "/" + wname;

    set_auth(this_object(), "root:root");

    /* If there isn't a directory yet, create one. */
    if (file_size(new_dir) != -2)
    {
	/* This isn't nice, but the player still is a new member of the
	 * domain. That hasn't changed.
	 */
	if (file_size(new_dir) != -1)
	{
	    write("Failed to create " + new_dir +
		  " since there is already a file with that name.\n");
	    return 1;
	}

	/* If there is a directory in the old domain, move it if it is in
	 * WIZARD_DOMAIN. We don't want to move domain-directories from
	 * normal domains.
	 */
	if ((file_size(old_dir) == -2) &&
	    (old_domain == WIZARD_DOMAIN))
	{
	    rename(old_dir, new_dir);
	}
	else
	{
	    mkdir(new_dir);
	}
    }
    /* This one is also joining a domain, but apperently he was a member
     * before because there is already a directory with his/her name. If
     * possible, we move the private directory.
     */
    else if ((file_size(old_dir + "/private") == -2) &&
	     (file_size(new_dir + "/private") == -1))
    {
	rename(old_dir + "/private", new_dir);
    }

    return 1;
}

/*
 * Function name: draft_wizard_to_domain
 * Description  : With this function an archwizard or keeper can draft a
 *                wizard to a domain. This command can also be used to draft
 *                madwands.
 * Arguments    : string dname - the domain to draft the wizard to.
 *                string wname - the wizard to draft.
 * Returns      : int 1/0 - success/failure.
 */
int
draft_wizard_to_domain(string dname, string wname)
{
    string cmder;

    /* May only be called from the arch soul. */
    if (!CALL_BY(WIZ_CMD_ARCH))
    {
	return 0;
    }

    cmder = getwho();
    dname = capitalize(dname);
    wname = lower_case(wname);

    if (!m_domains[dname])
    {
        notify_fail("Nie ma domeny o nazwie '" + dname + "'.\n");
        return 0;
    }

    /* No vacancies in the domain. */
    if (sizeof(m_domains[dname][FOB_DOM_MEMBERS]) >=
	m_domains[dname][FOB_DOM_MAXSIZE])
    {
	notify_fail("There are no vacancies in " + dname + ".\n");
	return 0;
    }

    /* Drafting a madwand */
    if (!sizeof(m_wizards[wname]))
    {
#ifdef FOB_MADWAND_SUPPORT
	return draft_madwand(dname, wname, cmder);
#else
	notify_fail("It is not possible to draft madwands in this mud.\n");
	return 0;
#endif FOB_MADWAND_SUPPORT
    }

    /* Don't draft people who are in another domain. */
    if (strlen(m_wizards[wname][FOB_WIZ_DOM]) &&
	(m_wizards[wname][FOB_WIZ_DOM] != WIZARD_DOMAIN))
    {
	notify_fail(capitalize(wname) + " already is a member of " + 
		    m_wizards[wname][FOB_WIZ_DOM] + ".\n");
	return 0;
    }

    /* Only draft apprentices. */
    if ((m_wizards[wname][FOB_WIZ_RANK] != WIZ_APPRENTICE) &&
	(m_wizards[wname][FOB_WIZ_RANK] < WIZ_NORMAL))
    {
	notify_fail(capitalize(wname) + " is not an apprentice.\n");
	return 0;
    }

    /* Apprentices should be made full wizard. */
    if (m_wizards[wname][FOB_WIZ_RANK] == WIZ_APPRENTICE)
    {
	do_change_rank(wname, WIZ_NORMAL, cmder);
    }

    write("Drafting " + capitalize(wname) + " to " + dname + ".\n");
    return add_wizard_to_domain(dname, wname, cmder);
}

/*
 * Function name: expel_wizard_from_domain
 * Description  : Expel a wizard from a domain.
 * Arguments    : string wname - the wizard to expel.
 * Returns      : int 1/0 - success/failure.
 */
int
expel_wizard_from_domain(string wname)
{
    string cmder;

    /* May only be called from the Lord soul. */
    if (!CALL_BY(WIZ_CMD_LORD))
    {
	return 0;
    }

    wname = lower_case(wname);
    if (!strlen(wname))
    {
	notify_fail("Expel whom?\n");
	return 0;
    }

    if (!sizeof(m_wizards[wname]))
    {
	notify_fail(capitalize(wname) + " is a mortal player!\n");
	return 0;
    }

    if (!strlen(m_wizards[wname][FOB_WIZ_DOM]))
    {
	notify_fail(capitalize(wname) + " is not a member of any domain.\n");
	return 0;
    }

    /* Lords may only boot their own domain members. Use <= WIZ_LORD to
     * also include expelling stewards.
     */
    cmder = getwho();
    if ((m_wizards[cmder][FOB_WIZ_RANK] <= WIZ_LORD) &&
	(m_wizards[wname][FOB_WIZ_DOM] != m_wizards[cmder][FOB_WIZ_DOM]))
    {
	write(capitalize(wname) + " is not a member of your domain " +
	      m_wizards[wname][FOB_WIZ_DOM] + ".\n");
	return 1;
    }

    /* Try to add mages, arches and keepers to WIZARD_DOMAIN. */
    if ((m_wizards[wname][FOB_WIZ_RANK] == WIZ_MAGE) ||
	(m_wizards[wname][FOB_WIZ_RANK] >= WIZ_ARCH))
    {
	if (m_wizards[wname][FOB_WIZ_DOM] == WIZARD_DOMAIN)
	{
	    write(capitalize(wname) + " cannot leave the domain " +
		  WIZARD_DOMAIN + ".\n" );
	    return 1;
	}

	return add_wizard_to_domain(WIZARD_DOMAIN, wname, cmder);
    }

    /* If the player was a madwand, kick him out via another funciton.
     * When madwand support is turned off, just unmark him as madwand
     * and expel him the normal way.
     */
    if (m_domains[m_wizards[wname][FOB_WIZ_DOM]][FOB_DOM_MADWAND] == wname)
    {
#ifdef FOB_MADWAND_SUPPORT
	return expel_madwand(wname, cmder);
#else
	m_domains[m_wizards[wname][FOB_WIZ_DOM]][FOB_DOM_MADWAND] = 0;
#endif FOB_MADWAND_SUPPORT
    }

    /* Else, demote them and kick them out of the domain. */
    do_change_rank(wname, WIZ_APPRENTICE, cmder);
    return add_wizard_to_domain("", wname, cmder);
}

/*
 * Function name: leave_domain
 * Description  : Someone wants to leave his/her domain.
 * Returns      : int 1/0 - success/failure.
 */
int
leave_domain()
{
    string cmder;

    /* May only be called from the normal wizards soul. */
    if (!CALL_BY(WIZ_CMD_NORMAL))
    {
	return 0;
    }

    /* Mages, arches and keepers cannot leave WIZARD_DOMAIN. */
    cmder = getwho();
    if ((m_wizards[cmder][FOB_WIZ_DOM] == WIZARD_DOMAIN) &&
	((m_wizards[cmder][FOB_WIZ_RANK] >= WIZ_ARCH) ||
	 (m_wizards[cmder][FOB_WIZ_RANK] == WIZ_MAGE)))
    {
	write("You cannot leave " + WIZARD_DOMAIN + " for good.\n");
	return 1;
    }

    /* Domain-member or not? */
    if (!strlen(m_wizards[cmder][FOB_WIZ_DOM]))
    {
	write("You are not a member of any domain.\n");
	return 1;
    }

    /* Madwands have their own expelling function. When madwand support is
     * not turned on, just unmark the madwand and expel him the normal way.
     * This turns the madwand into a normal wizard.
     */
    if (m_domains[m_wizards[cmder][FOB_WIZ_DOM]][FOB_DOM_MADWAND] == cmder)
    {
#ifdef FOB_MADWAND_SUPPORT
	return expel_madwand(cmder, cmder);
#else
	m_domains[m_wizards[cmder][FOB_WIZ_DOM]][FOB_DOM_MADWAND] = "";
#endif FOB_MADWAND_SUPPORT
    }

    /* Demote the wizard if he isn't a mage, arch or keeper. */
    if ((m_wizards[cmder][FOB_WIZ_RANK] != WIZ_MAGE) &&
	(m_wizards[cmder][FOB_WIZ_RANK] < WIZ_ARCH))
    {
	do_change_rank(cmder, WIZ_APPRENTICE, cmder);
    }

    /* Add the wizard to the empty domain, i.e leave the domain. */
    return add_wizard_to_domain("", cmder, cmder);
}

/*
 * Function name: rename_wizard
 * Description  : A service function to update the name of the wizard in
 *                case a wizard changes his/her name. It makes no validity
 *                checks.
 * Arguments    : string oldname - the old name of the wizard.
 *                string newname - the new name of the wizard.
 */
static int
rename_wizard(string oldname, string newname)
{
    string dname = m_wizards[oldname][FOB_WIZ_DOM];

    /* Rename the wizard in the wizard mapping. */
    write("Wizard status copied to " + capitalize(newname) + ".\n");
    m_wizards[newname] = secure_var(m_wizards[oldname]);
    m_wizards = m_delete(m_wizards, oldname);

    /* Update trainee status. */
    if (m_trainees[oldname])
    {
        write("Trainee status copied to " + capitalize(newname) + ".\n");
        m_trainees[newname] = 1;
        m_trainees = m_delete(m_trainees, oldname);
    }

    /* Update global read. */
    if (m_global_read[oldname])
    {
        write("Global read copied to " + capitalize(newname) + ".\n");
        m_global_read[newname] = secure_var(m_global_read[oldname]);
        m_global_read = m_delete(m_global_read, oldname);
    }

    /* Update domain information. */
    if (strlen(dname))
    {
        write("Domain membership of " + capitalize(newname) + " updated.\n");
        m_domains[dname][FOB_DOM_MEMBERS] -= ({ oldname });
        m_domains[dname][FOB_DOM_MEMBERS] += ({ newname });

        switch(m_wizards[newname][FOB_WIZ_RANK])
        {
        case WIZ_STEWARD:
            m_domains[dname][FOB_DOM_STEWARD] = newname;
            break;

        case WIZ_LORD:
            m_domains[dname][FOB_DOM_LORD] = newname;
            break;
        }

        if (rename(("/d/" + dname + "/" + oldname),
               ("/d/" + dname + "/" + newname)))
        {
            write("Home directory successfully renamed.\n");
        }
        else
        {
            write("Failed to rename home directory.\n");
        }
    }

    save_master();
    log_file("LEVEL",
        sprintf("%s %-11s: renamed to %-11s by %s.\n",
        ctime(time()),
        capitalize(oldname),
        capitalize(newname),
        capitalize(this_interactive()->query_real_name())));
}

/************************************************************************
 *
 * The applications administration code.
 *
 */

/*
 * Function name: apply_to_domain
 * Description  : Apply to a domain.
 * Arguments    : string dname - domain the application goes to.
 * Returns      : int 1/0 - success/failure.
 */
int
apply_to_domain(string dname)
{
    string wname;
    string *appl;
    object wiz;

    /* May only be called from the apprentice soul. */
    if (!CALL_BY(WIZ_CMD_APPRENTICE))
    {
	return 0;
    }

    if (!strlen(dname))
    {
	notify_fail("Apply to what domain?\n");
	return 0;
    }

    dname = capitalize(dname);
    wname = getwho();
    
    switch(m_wizards[wname][FOB_WIZ_RANK])
    {
    case WIZ_MAGE:
    case WIZ_ARCH:
    case WIZ_KEEPER:
	if (m_wizards[wname][FOB_WIZ_DOM] != WIZARD_DOMAIN)
	{
	    notify_fail("You are already a member of the domain " + 
			m_wizards[wname][FOB_WIZ_DOM] + ".");
	    return 0;
	}
	break;

    case WIZ_PILGRIM:
	notify_fail("Pilgrims are doomed to wander the world; " +
		    "they may not settle down.\n");
	return 0;
	break;

    default:
	if (m_wizards[wname][FOB_WIZ_DOM] != "")
	{
	    notify_fail("You are already a member of the domain " + 
			m_wizards[wname][FOB_WIZ_DOM] + ".\n");
	    return 0;
	}
	break;
    }

    if (!sizeof(m_domains[dname]))
    {
	notify_fail("There is no domain named '" + dname + "'.\n");
	return 0;
    }

    /* See if there is an array of people for that domain already. */
    if (!pointerp(m_applications[dname]))
    {
	m_applications[dname] = ({ });
    }
    /* else see if the player already applied. */
    else if (member_array(wname, m_applications[dname]) != -1)
    {
	write("You already filed an application to " + dname + "!\n");
	return 1;
    }

    /* See whether there is room in the domain. This does not terminate
     * the application process if it fails.
     */
    if (sizeof(m_domains[dname][FOB_DOM_MEMBERS]) >=
	m_domains[dname][FOB_DOM_MAXSIZE])
    {
	write("The domain " + dname + " is already full, but your " +
	      "application will still be registered.\n");
    }

    /* Add the person to the list of people who applied. */
    m_applications[dname] += ({ wname });
    save_master();

    /* Tell the wizard and the members in the domain. */
    tell_domain(dname, wname, ("You applied to " + dname + ".\n"),
		(capitalize(wname) + " just applied for membership to " +
		 dname + ".\n"));

    return 1;
}

/*
 * Function name: accept_application
 * Description  : A lord accepts an application. This function can also
 *                be used by the Liege to draft a madwand into the domain.
 * Arguments    : string wname - the wizard to accept.
 * Returns      : int 1/0 - success/failure.
 */
int
accept_application(string wname)
{
    string dname;
    string cmder;

    /* May only be called from the Lord soul. */
    if (!CALL_BY(WIZ_CMD_LORD))
    {
	return 0;
    }

    if (!strlen(wname))
    {
	notify_fail("Accept which wizard?\n");
	return 0;
    }

    cmder = getwho();
    if (m_wizards[cmder][FOB_WIZ_RANK] > WIZ_LORD)
    {
	write("You are not the liege or steward of any domain!\n");
	return 1;
    }

    wname = lower_case(wname);
    dname = m_wizards[cmder][FOB_WIZ_DOM];

    if (sizeof(m_domains[dname][FOB_DOM_MEMBERS]) >=
	m_domains[dname][FOB_DOM_MAXSIZE])
    {
	notify_fail("There are no vacancies in " + dname + ".\n");
	return 0;
    }

    /* When the draftee isn't a wizard, this means the Lord wants to draft
     * the mortal as a madwand.
     */
    if (!m_wizards[wname])
    {
#ifdef FOB_MADWAND_SUPPORT
	return draft_madwand(dname, wname, cmder);
#else
	notify_fail("It is not possible to draft madwands in this mud.\n");
	return 0;
#endif FOB_MADWAND_SUPPORT
    }

    if (!pointerp(m_applications[dname]))
    {
	write("Nobody has asked to join your domain.\n");
	return 1;
    }
    else if (member_array(wname, m_applications[dname]) == -1)
    {
	write(capitalize(wname) + " has not asked to join your domain.\n");
	return 1;
    }

    if ((m_wizards[wname][FOB_WIZ_RANK] != WIZ_APPRENTICE) &&
	 (m_wizards[wname][FOB_WIZ_DOM] != WIZARD_DOMAIN))
    {
	notify_fail(capitalize(wname) +
		    " is neither an apprentice, nor a member of the domain " +
		    WIZARD_DOMAIN + ".\n");
	return 0;
    }

    /* Joining means no more applications. */
    remove_all_applications(wname);

    /* People who aren't a wizard already should become full wizard. */
    if (m_wizards[wname][FOB_WIZ_RANK] == WIZ_APPRENTICE)
    {
	do_change_rank(wname, WIZ_NORMAL, cmder);
    }

    return add_wizard_to_domain(dname, wname, cmder);
}

/*
 * Function name: deny_application
 * Description  : The lord denies an application of a wizard.
 * Arguments    : string wname - the wizard name.
 * Returns      : int 1/0 - success/failure.
 */
int
deny_application(string wname)
{
    string dname;
    string cmder;
    object wiz;

    /* May only be called from the Lord soul. */
    if (!CALL_BY(WIZ_CMD_LORD))
    {
	return 0;
    }
    
    cmder = getwho();
    if (m_wizards[cmder][FOB_WIZ_RANK] > WIZ_LORD)
    {
	notify_fail("You are not the liege or steward of any domain!\n");
	return 0;
    }

    wname = lower_case(wname);
    dname = m_wizards[cmder][FOB_WIZ_DOM];

    if (!pointerp(m_applications[dname]))
    {
	write("Nobody has asked to join your domain.\n");
	return 1;
    }
    else if (member_array(wname, m_applications[dname]) == -1)
    {
	write(capitalize(wname) + " has not asked to join " + dname + ".\n");
	return 1;
    }

    /* Remove the application. */
    m_applications[dname] -= ({ wname });

    /* Remove the domain-entry if this was the last application. */
    if (!sizeof(m_applications[dname]))
    {
        m_applications = m_delete(m_applications, dname);
    }

    if (objectp(wiz = find_player(wname)))
    {
	tell_object(wiz, "Your application to the domain '" + dname +
		    "' was denied.\n");
    }

    write("You denied the application to " + dname + " by " +
	  capitalize(wname) + ".\n");
    return 1;
}

/*
 * Function name: regret_application
 * Description  : A wizard regrets an application to a domain.
 * Arguments    : string dname - the domain name.
 * Returns      : int 1/0 - success/failure.
 */
int
regret_application(string dname)
{
    string wname;

    /* May only be called from the apprentice soul. */
    if (!CALL_BY(WIZ_CMD_APPRENTICE))
    {
	return 0;
    }

    if (!strlen(dname))
    {
	notify_fail("Regret your application to which domain?");
	return 0;
    }

    wname = getwho();
    dname = capitalize(dname);

    /* See if the wizard applied to the domain. */
    if (member_array(wname, m_applications[dname]) == -1)
    {
	notify_fail("You have no application pending to the domain " +
		    dname + ".\n");
	return 0;
    }

    /* Remove the application from the list. */
    m_applications[dname] -= ({ wname });

    /* If there are no other applications, remove the empty array. */
    if (!sizeof(m_applications[dname]))
    {
	m_applications = m_delete(m_applications, dname);
    }

    save_master();

    tell_domain(dname, wname, ("Regretted your application to " + dname +
        ".\n"), (capitalize(wname) + " just regretting applying to " +
        dname + ".\n"));
    return 1;
}

/*
 * Function name: remove_all_applications
 * Description  : Remove all application by a wizard from the application list.
 * Arguments    : string wname - the name of the wizard in lower case.
 */
static void
remove_all_applications(string wname)
{
    string *domains;
    int    index;
    int    size;

    domains = m_indices(m_applications);

    index = -1;
    size  = sizeof(domains);
    while(++index < size)
    {
	/* Remove the application from the wizard if it exists. */
        if (pointerp(m_applications[domains[index]]))
        {
            write("Removing application from " + capitalize(wname) + " to " +
                domains[index] + ".\n");
  	    m_applications[domains[index]] -= ({ wname });

  	    /* If this was the last wizard, remove the domain from the list. */
	    if (!sizeof(m_applications[domains[index]]))
 	    {
	        m_applications = m_delete(m_applications, domains[index]);
 	    }
        }
    }

    /* Save the master object. */
    save_master();
}

/*
 * Function name: filter_applications
 * Description  : This function checks whether a particular wizard is in the
 *                list of people who have applied to a domain. 
 * Arguments    : string *wizards - the people who have applied to a domain.
 *                string wname    - the wizard to check.
 * Returns      : int 1/0 - true if the wizard applied to the domain.
 */
static int
filter_applications(string *wizards, string wname)
{
    return (member_array(wname, wizards) != -1);
}

/*
 * Function name: list_applications_by_wizard
 * Description  : This function lists all applications made by a wizard.
 * Arguments    : string wname  - the name of the wizard to list.
 *                int list_self - true if someone inquires about himself.
 * Returns      : int 1/0 - success/failure.
 */
static int
list_applications_by_wizard(string wname, int list_self)
{
    mapping domains;

    wname = lower_case(wname);
    domains = filter(m_applications, &filter_applications(, wname));

    wname = (list_self ? "You have" : (capitalize(wname) + " has"));

    if (!m_sizeof(domains))
    {
	write(wname + " no pending applications to any domain.\n");
	return 1;
    }

    write(wname + " applications pending to " +
	  COMPOSITE_WORDS(m_indices(domains)) + ".\n");
    return 1;
}

/*
 * Function name: list_applications
 * Description  : List applications. Domain-members may see the people that
 *                have applied to the domain. Non-domain members may see
 *                only their own applications. Arches and keepers may see
 *                all applications.
 * Arguments    : string str - the command line argument.
 * Returns      : int 1/0 - success/failure.
 */
int
list_applications(string str)
{
    string cmder;
    string dname;
    string *words;
    int    index;
    int    size;
    int    rank;

    cmder = getwho();
    if (!strlen(cmder))
    {
	notify_fail("You are not allowed to do this.\n");
	return 0;
    }

    rank = m_wizards[cmder][FOB_WIZ_RANK];

    /* Arches or keepers. */
    if (rank >= WIZ_ARCH)
    {
	/* Basic operation: list all applications. */
	if (!strlen(str))
	{
	    if (!m_sizeof(m_applications))
	    {
		write("No applications have been filed.\n");
		return 1;
	    }

	    words = m_indices(m_applications);
	    index = -1;
	    size = sizeof(words);
	    write("Domain      Applications\n----------- ------------\n");

	    while(++index < size)
	    {
                write(FORMAT_NAME(words[index]) + " " +
                    COMPOSITE_WORDS(sort_array(map(m_applications[words[index]],
                    capitalize))) + "\n");
	    }
	    return 1;
	}

	words = explode(str, " ");
        if ((sizeof(words) != 2) ||
	    (words[0] != "clear"))
	{
	    notify_fail("Syntax: applications clear <domain> / <wizard>\n");
	    return 0;
	}

	/* Remove all applications to a certain domain. */
        if (sizeof(m_domains[capitalize(words[1])]))
        {
            dname = capitalize(words[1]);
	    if (!sizeof(m_applications[dname]))
	    {
                write("No apprentices applied to " + dname + ".\n");
		return 1;
	    }

	    m_applications = m_delete(m_applications, dname);
	    save_master();
	    write("Removed all applications to " + dname + ".\n");
	    return 1;
	}

	/* Remove all applications by a certain player. */
        write("Checking applications for " + capitalize(words[1]) + ".\n");
        remove_all_applications(lower_case(words[1]));
	return 1;
    }

    /* People in a domain may list the applications to their domain. */
    if ((rank >= WIZ_NORMAL) &&
	(m_wizards[cmder][FOB_WIZ_DOM] != WIZARD_DOMAIN))
    {
	if (strlen(str) &&
	    (rank == WIZ_LORD))
	{
	    return list_applications_by_wizard(str, 0);
	}

	if (!sizeof(m_applications[m_wizards[cmder][FOB_WIZ_DOM]]))
	{
	    write("No wizards have applied to your domain.\n");
	    return 1;
	}	    

        write("The following people have applied to your domain: " +
            COMPOSITE_WORDS(sort_array(map(m_applications[m_wizards[cmder][FOB_WIZ_DOM]],
            capitalize))) + ".\n");
	return 1;
    }

    /* List your own applications. */
    return list_applications_by_wizard(cmder, 1);
}

/************************************************************************
 *
 * The experience administration code.
 *
 */

/*
 * Function name: bookkeep_exp
 * Description  : Note the xp domains give to the mortals and what kind.
 *                This is not saved each time to KEEPERSAVE, we trust it
 *		  to be saved at one time or other. Exact bookkeeping is not
 *		  absolutely crucial and it would take a lot of time.
 * Arguments    : int q_xp: Amount of quest xp. Can only be 0 or positive.
 *		  int c_xp: Amount of combat xp. Can be both pos. and neg.
 */
public void
bookkeep_exp(int q_xp, int c_xp)
{
    int    cobj = 0;
    string dname = "";
    object pobj = previous_object();
    object giver = previous_object(-1);

    /* It should be a mortal player, not an NPC and it should not be fixup
     * of accumulated stats, not should it be a 'jr' wizhelper character.
     */
    if (!interactive(pobj) ||
	(geteuid(pobj) != BACKBONE_UID) ||
	((pobj == giver) &&
	 (q_xp == 0) &&
         (ABS(c_xp) < 2)) ||
	wildmatch("*jr", pobj->query_real_name()))
    {
	return;
    }

    /* If it is combat XP, we want to get the living, not the combat
     * object, otherwise tracing is real hard.
     */
    if (c_xp &&
	objectp(giver->qme()))
    {
	giver = giver->qme();
	cobj = 1;
    }

#ifdef LOG_BOOKKEEP_ERR
    /* BAD wiz! Won't be wiz much longer. */
    if (objectp(this_interactive()))
    {
        if (SECURITY->query_wiz_level(dname =
				      this_interactive()->query_real_name()))
        {
	    set_auth(this_object(), "root:root");
            log_file(LOG_BOOKKEEP_ERR,
		     sprintf("%s %-12s %-1s(%8d) %-12s\n",
			     ctime(time()),
			     capitalize(pobj->query_real_name()),
			     ((q_xp == 0) ? "C" : "Q"),
			     ((q_xp == 0) ? c_xp : q_xp),
			     capitalize(dname)), 10000);
        }
    }
    /* This indicates that it wasn't a combat object giving the combat
     * experience.
     */
    else if ((c_xp > 0) &&
	     (!living(giver) ||
	      !cobj))
    {
	set_auth(this_object(), "root:root");
	log_file(LOG_BOOKKEEP_ERR,
		 sprintf("%s %-12s C(%8d) %s\n",
			 ctime(time()),
			 capitalize(pobj->query_real_name()),
			 c_xp,
			 file_name(giver)), 10000);
    }
#endif LOG_BOOKKEEP_ERR

    /* Get the euid of the experience giving object. */
    dname = geteuid(giver);
    if (sizeof(m_wizards[dname]))
    {
	dname = m_wizards[dname][FOB_WIZ_DOM];
    }

    /* Experience can only be given by a domain. This object had a bad
     * euid. Nonexistant or apprentice.
     */
    if (!sizeof(m_domains[dname]))
    {
#ifdef LOG_BOOKKEEP_ERR
	/* If this is the case, it is a playerkill and we do not want to
	 * log that.
	 */
	if (dname != BACKBONE_UID)
	{
	    set_auth(this_object(), "root:root");
	    log_file(LOG_BOOKKEEP_ERR,
		     sprintf("%s %-12s %-1s(%8d) %-1s\n",
			     ctime(time()),
			     capitalize(pobj->query_real_name()),
			     ((q_xp == 0) ? "C" : "Q"),
			     ((q_xp == 0) ? c_xp : q_xp),
			     file_name(giver)), 10000);
	}
#endif LOG_BOOKKEEP_ERR
	return;
    }

#ifdef LOG_BOOKKEEP
    /* This is the log all normal combat experience and quest experience
     * ends up. It is only logged if it exceeds a certain level.
     */
    if (( q_xp > LOG_BOOKKEEP_LIMIT_Q) ||
	( c_xp > LOG_BOOKKEEP_LIMIT_C) ||
	(-q_xp > LOG_BOOKKEEP_LIMIT_Q) ||
	(-c_xp > LOG_BOOKKEEP_LIMIT_C))
    {
	set_auth(this_object(), "root:root");
	log_file(LOG_BOOKKEEP, sprintf("%s %-12s %-1s(%8d) %-1s\n",
				       ctime(time()),
				       capitalize(pobj->query_real_name()),
				       ((q_xp == 0) ? "C" : "Q"),
				       ((q_xp == 0) ? c_xp : q_xp),
				       file_name(giver)), 1000000);
    }
#endif LOG_BOOKKEEP

    /* Give the experience to the domain. Not necessary to save it, that
     * will happen next time something needs to be saved.
     */
    m_domains[dname][FOB_DOM_QXP] += q_xp;
    if (c_xp > 0)
    {
        m_domains[dname][FOB_DOM_CXP] += c_xp;
    }
}

/*
 * Function name: do_decay
 * Description  : The mapping m_domains is mapped over this function to
 *                do the actual decay.
 * Arguments    : mixed *darr - the array of the individual domains.
 * Returns      : mixed * - the modified array of the individual domains.
 */
static mixed *
do_decay(mixed *darr)
{
    int decay;
#ifdef DECAY_XP
    decay = DECAY_XP;
#else
    decay = 100;
#endif
    if (!decay)
    {
	return darr;
    }

    if (darr[FOB_DOM_QXP] >= decay || (-darr[FOB_DOM_QXP]) >= decay)
	darr[FOB_DOM_QXP] -= darr[FOB_DOM_QXP] / decay;
    else if (darr[FOB_DOM_QXP] < 0)
	darr[FOB_DOM_QXP] += 1;
    else if (darr[FOB_DOM_QXP] > 0)
	darr[FOB_DOM_QXP] -= 1;

    if (darr[FOB_DOM_CXP] >= decay || (-darr[FOB_DOM_CXP]) >= decay)
	darr[FOB_DOM_CXP] -= darr[FOB_DOM_CXP] / decay;
    else if (darr[FOB_DOM_CXP] < 0)
	darr[FOB_DOM_CXP] += 1;
    else if (darr[FOB_DOM_CXP] > 0)
	darr[FOB_DOM_CXP] -= 1;
/*
 * Troche za duzo komend zabierane. /Alvin
    darr[FOB_DOM_CMNDS] = darr[FOB_DOM_CMNDS] / DECAY_XP;
 */
    
    darr[FOB_DOM_CMNDS] = darr[FOB_DOM_CMNDS] / 2;

    return darr;
}

/*
 * Function name: do_decay_cmd
 * Description  : This decay function will take 1% off the argument value.
 * Arguments    : int count - the value to decay.
 * Returns      : int - the same value minus one per cent.
 */
int
do_decay_cmd(int count)
{
    return count - count / 100;
}

/*
 * Function name: decay_exp
 * Description:   Let the accumulated xp / domain decay over time. This
 *                is called from check_memory which is called at regular
 *		  intervalls. check_memory also saves to KEEPERSAVE.
 */
static void
decay_exp()
{
    m_domains = map(m_domains, do_decay);
}

/*
 * Function name: query_domain_commands
 * Description  : Gives the total number of commands executed by mortal
 *                players in a domain.
 * Arguments    : string dname - the domain name.
 * Returns      : int - the number of commands.
 */
int
query_domain_commands(string dname)
{
    dname = capitalize(dname);

    if (!sizeof(m_domains[dname]))
    {
	return 0;
    }

    return m_domains[dname][FOB_DOM_CMNDS];
}

/*
 * Function name: query_domain_qexp
 * Description  : Gives the quest experience gathered by a domain.
 * Arguments    : string dname - the domain name.
 * Returns      : int - the accumulated experience.
 */
int
query_domain_qexp(string dname)
{
    dname = capitalize(dname);

    if (!sizeof(m_domains[dname]))
    {
	return 0;
    }

    return m_domains[dname][FOB_DOM_QXP];
}

/*
 * Function name: query_domain_cexp
 * Description  : Gives the combat experience gathered by a domain.
 * Arguments    : string dname - the domain name. 
 * Returns      : int - the accumulated experience.
 */
int
query_domain_cexp(string dname)	
{
    dname = capitalize(dname);

    if (!sizeof(m_domains[dname]))
    {
	return 0;
    }

    return m_domains[dname][FOB_DOM_CXP];
}

/*
 * Function name: domain_clear_xp
 * Description  : With this function archwizards and keepers may remove the
 *                experience gathered by a domain. This is for debugging
 *                purposes and when errors have been made and the experience
 *                is not correct any more
 * Arguments    : string dname - the domain-name.
 * Returns      : int 1/0 - success/failure.
 */
int
domain_clear_xp(string dname)
{
    string wname;

    /* May only be called from the arch soul. */
    if (!CALL_BY(WIZ_CMD_ARCH))
    {
	return 0;
    }

    /* Check whether such a domain indeed exists. */
    dname = capitalize(dname);
    if (!sizeof(m_domains[dname]))
    {
	notify_fail("No domain " + dname + ".\n");
	return 0;
    }

    /* Wipe the experience, save the master, log possible and tell the
     * wizard.
     */
    m_domains[dname][FOB_DOM_CXP] = 0;
    m_domains[dname][FOB_DOM_QXP] = 0;

    save_master();

#ifdef LOG_BOOKKEEP
    wname = getwho();
    log_file(LOG_BOOKKEEP, sprintf("%s (%s) Cleared by: %s\n",
        ctime(time()), dname, capitalize(wname)), -1);
#endif LOG_BOOKKEEP

    write("Cleared the experience of the domain " + dname + ".\n");
    return 1;
}

/************************************************************************
 *
 * The wizard administration code.
 *
 */

/*
 * Function name: query_wiz_level
 * Description  : Return the level of a wizard.
 * Arguments    : string wname - the wizard.
 * Returns      : int - the level.
 */
int
query_wiz_level(string wname)
{
    wname = lower_case(wname);

    if (!sizeof(m_wizards[wname]))
    {
	return 0;
    }

    return m_wizards[wname][FOB_WIZ_LEVEL];
}

/*
 * Function name: query_wiz_rank
 * Desciption   : Return the rank of the wizard.
 * Arguments    : string wname - the name of the wizard.
 * Returns      : int - the rank.
 */
int
query_wiz_rank(string wname)
{
    wname = lower_case(wname);

    if (!sizeof(m_wizards[wname]))
    {
	return WIZ_MORTAL;
    }

    return m_wizards[wname][FOB_WIZ_RANK];
}

/*
 * Function name: query_wiz_chl
 * Description  : Return the name of the level changer.
 * Arguments    : string wname - the wizard.
 * Returns      : string - the changer.
 */
string
query_wiz_chl(string wname)
{
    wname = lower_case(wname);

    if (!sizeof(m_wizards[wname]))
    {
	return "";
    }

    return m_wizards[wname][FOB_WIZ_CHLEVEL];
}

/*
 * Function name: query_wiz_chl_time
 * Description  : Return the time the level was last changed.
 * Arguments    : string wname - the wizard.
 * Returns      : int - the time.
 */
#ifdef FOB_KEEP_CHANGE_TIME
int
query_wiz_chl_time(string wname)
{
    wname = lower_case(wname);

    if (!sizeof(m_wizards[wname]))
    {
	return 0;
    }

    return m_wizards[wname][FOB_WIZ_CHLTIME];
}
#endif FOB_KEEP_CHANGE_TIME

/*
 * Function name: query_wiz_dom
 * Description  : Return the domain of a wizard.
 * Arguments    : string wname - the wizard.
 * Returns      : string - the domain, else "".
 */
string
query_wiz_dom(string wname)
{
    wname = lower_case(wname);

    if (!sizeof(m_wizards[wname]))
    {
	return "";
    }

    return m_wizards[wname][FOB_WIZ_DOM];
}

/*
 * Function name: query_wiz_chd
 * Description  : Return the name of the domain changer.
 * Arguments    : string wname - the wizard.
 * Returns      : string - the changer.
 */
string
query_wiz_chd(string wname)
{
    wname = lower_case(wname);

    if (!sizeof(m_wizards[wname]))
    {
	return "";
    }

    return m_wizards[wname][FOB_WIZ_CHDOM];
}

/*
 * Function name: query_wiz_chd_time
 * Description  : Return the time the domain was last changed.
 * Arguments    : string wname - the wizard.
 * Returns      : string - the time.
 */
#ifdef FOB_KEEP_CHANGE_TIME
int
query_wiz_chd_time(string wname)
{
    wname = lower_case(wname);

    if (!sizeof(m_wizards[wname]))
    {
	return 0;
    }

    return m_wizards[wname][FOB_WIZ_CHDTIME];
}
#endif FOB_KEEP_CHANGE_TIME

/*
 * Function name: query_wiz_list
 * Description  : Return a list of all wizards of a given rank.
 * Arguments    : int - the rank to get; -1 = all.
 * Returns      : string * - the names of the wizards with that name.
 */
public string *
query_wiz_list(int rank)
{
    if (rank == -1)
    {
	return m_indices(m_wizards);
    }

    return filter(m_indices(m_wizards), &operator(==)(rank) @ query_wiz_rank);
}

/*
 * Function name: reset_wiz_uid
 * Description:   Set up a wizard's uid.
 * Arguments:     object wiz - the wizard.
 */
public nomask void
reset_wiz_uid(object wiz)
{
    if (!query_wiz_level(wiz->query_real_name()))
    {
        return;
    }

    set_auth(wiz, wiz->query_real_name() + ":#");
}

/*
 * Function name: do_change_rank
 * Description  : This function actually changes the rank of the person.
 *                It is an internal function that does not make any
 *                additional checks. They should have been made earlier.
 *                When someone is made liege, the previous liege is made
 *                into normal wizard.
 * Arguments    : string wname - the wizard who rank is changed.
 *                int rank     - the new rank of the wizard.
 *                string cmder - who forces the change.
 */
static void
do_change_rank(string wname, int rank, string cmder)
{
    string dname;
    object wizard;

    /* Log the change of the rank. */
    log_file("LEVEL",
	     sprintf("%s %-11s: %-4s (%2d) -> %-4s (%2d) by %s.\n",
		     ctime(time()),
		     capitalize(wname),
		     WIZ_RANK_SHORT_NAME(m_wizards[wname][FOB_WIZ_RANK]),
		     m_wizards[wname][FOB_WIZ_LEVEL],
		     WIZ_RANK_SHORT_NAME(rank),
		     WIZ_RANK_START_LEVEL(rank),
		     capitalize(cmder)));

    /* If the person was Liege, inform the domain. */
    if (m_wizards[wname][FOB_WIZ_RANK] == WIZ_LORD)
    {
	tell_domain(m_wizards[wname][FOB_WIZ_DOM],
		    m_domains[m_wizards[wname][FOB_WIZ_DOM]][FOB_DOM_LORD],
		    ("You are no longer the liege of " +
		     m_wizards[wname][FOB_WIZ_DOM] + ".\n"),
		    (capitalize(wname) + " no longer is the liege of " +
		     m_wizards[wname][FOB_WIZ_DOM] + ".\n"));

	m_domains[m_wizards[wname][FOB_WIZ_DOM]][FOB_DOM_LORD] = "";
    }

    /* If the person was steward, inform the domain. */
    if (m_wizards[wname][FOB_WIZ_RANK] == WIZ_STEWARD)
    {
	tell_domain(m_wizards[wname][FOB_WIZ_DOM],
		    m_domains[m_wizards[wname][FOB_WIZ_DOM]][FOB_DOM_STEWARD],
		    ("You are no longer the steward of " +
		     m_wizards[wname][FOB_WIZ_DOM] + ".\n"),
		    (capitalize(wname) + " no longer is the steward of " +
		     m_wizards[wname][FOB_WIZ_DOM] + ".\n"));

	m_domains[m_wizards[wname][FOB_WIZ_DOM]][FOB_DOM_STEWARD] = "";
    }

    /* If the person becomes Liege, inform the domain and demote the old
     * Liege, if there was one. Similar for the steward.
     */
    if (rank == WIZ_LORD)
    {
	/* If there already is a Lord, demote the old one. This must be
	 * done before the new one is marked as such.
	 */
	dname = m_wizards[wname][FOB_WIZ_DOM];
	if (strlen(m_domains[dname][FOB_DOM_LORD]))
	{
	    do_change_rank(m_domains[dname][FOB_DOM_LORD], WIZ_NORMAL, cmder);
	}

	m_domains[dname][FOB_DOM_LORD] = wname;

        tell_domain(dname, wname,
            ("You are now the liege of " + dname + ".\n"),
            (capitalize(wname) + " is made the liege of " + dname + ".\n"));
    }

    if (rank == WIZ_STEWARD)
    {
	dname = m_wizards[wname][FOB_WIZ_DOM];
	if (strlen(m_domains[dname][FOB_DOM_STEWARD]))
	{
	    do_change_rank(m_domains[dname][FOB_DOM_STEWARD], WIZ_NORMAL,
		cmder);
	}

	m_domains[dname][FOB_DOM_STEWARD] = wname;

        tell_domain(dname, wname,
            ("You are now the steward of " + dname + ".\n"),
            (capitalize(wname) + " is made the steward of " + dname + ".\n"));
    }

    if (rank == WIZ_RETIRED)
    {
        m_trainees[wname] = 1;
    }
    else

    /* Unmark the person as trainee if no longer 'normal wizard'. */
    if ((rank != WIZ_NORMAL) &&
	m_trainees[wname])
    {
	m_trainees = m_delete(m_trainees, wname);
    }

    /* Tell the player about the change in rank. */
    if (objectp(wizard = find_player(wname)))
    {
	tell_object(wizard, "You have been " +
		    ((rank > m_wizards[wname][FOB_WIZ_RANK]) ? "promoted" :
		     "demoted") + " to " + WIZ_RANK_NAME(rank) + " (" +
		    WIZ_RANK_START_LEVEL(rank) + ") by " + capitalize(cmder) +
		    ".\n");
    }

    /* Make the actual change. */
    m_wizards[wname][FOB_WIZ_RANK] = rank;
    m_wizards[wname][FOB_WIZ_LEVEL] = WIZ_RANK_START_LEVEL(rank);
    m_wizards[wname][FOB_WIZ_CHLEVEL] = cmder;
#ifdef FOB_KEEP_CHANGE_TIME
    m_wizards[wname][FOB_WIZ_CHLTIME] = time();
#endif FOB_KEEP_CHANGE_TIME

    save_master();

    if (objectp(wizard))
    {
        wizard->reset_userids();
        /* Update the command hooks. */
        wizard->update_hooks();
    }
}

/*
 * Function name: wizard_change_rank
 * Description  : Using this function, a liege, arch or keeper can try to
 *                alter the rank of a player.
 * Arguments    : string wname - the wizard whose rank is to be changed.
 *                int rank     - the new rank.
 * Returns      : int 1/0 - success/failure.
 */
int
wizard_change_rank(string wname, int rank)
{
    string cmder;
    int    old_rank;
    string dname;
    object wizard;

    /* May only be called from the Lord soul. */
    if (!CALL_BY(WIZ_CMD_LORD))
    {
	return 0;
    }

    cmder = getwho();
    wname = lower_case(wname);
    if (sizeof(m_wizards[wname]))
    {
	old_rank = m_wizards[wname][FOB_WIZ_RANK];
	dname = m_wizards[wname][FOB_WIZ_DOM];
    }
    else
    {
	old_rank = WIZ_MORTAL;
	dname = "";
    }

    /* See whether the wizard is allowed to alter the rank. Lords may only
     * promote or demote 'normal' wizards to and from steward and vice versa.
     * Stewards may not meddle in ranks.
     */
    if (m_wizards[cmder][FOB_WIZ_RANK] == WIZ_STEWARD)
    {
	if (m_wizards[cmder][FOB_WIZ_DOM] != m_wizards[wname][FOB_WIZ_DOM])
	{
	    write("You may only handle people in your own domain.\n");
	    return 1;
	}

        if ((old_rank == WIZ_RETIRED) && (rank != WIZ_NORMAL) ||
            (old_rank == WIZ_NORMAL) && (rank != WIZ_RETIRED))
        {
            write("Mozesz wylacznie retireda promotowac na pelnego wiza, "+
                "albo wiza demotowac na retireda.\n");
                
            return 1;
        }
    }
    else if (m_wizards[cmder][FOB_WIZ_RANK] == WIZ_LORD)
    {
	if (m_wizards[cmder][FOB_WIZ_DOM] != m_wizards[wname][FOB_WIZ_DOM])
	{
	    write("You may only handle people in your own domain.\n");
	    return 1;
	}

	if ( (old_rank == WIZ_PILGRIM) && (rank != WIZ_NORMAL) )
	{
	    write("Pilgrimow mozesz tylko przywrocic do pelnego wiza, " +
	        "albo wyrzucic z domeny komenda expel.\n");
	        
	    return 1;
	}
	
	if ((old_rank == WIZ_NORMAL) && (rank != WIZ_RETIRED))
	{
	    write("Pelnego wizarda mozesz uczynic jedynie retiredem.\n");
	    
	    return 1;
	}
	
	if ((old_rank == WIZ_RETIRED) && (rank != WIZ_NORMAL))
	{
	    write("Retireda mozesz tylko przywrocic do pelnego wizarda, "+
	        "lub wyrzucic go z domeny przy pomocy 'expel'.\n");
	        
	    return 1;
	}
	
	if ((old_rank != WIZ_NORMAL) && (old_rank != WIZ_RETIRED) &&
	    ((old_rank != WIZ_STEWARD) || (rank != WIZ_NORMAL)))
	{
	    
	    write("You are only allowed to make a steward into a normal " +
                "wizard.\n");
	    return 1;
	}
    }
    /* Arches may not meddle with other arches or keepers. However, they may
     * demote themselves to mage.
     */
    else if (old_rank >= m_wizards[cmder][FOB_WIZ_RANK])
    {
        if (cmder != wname)
        {
            write("You are not allowed to alter the rank of " +
                LANG_ADDART(WIZ_RANK_NAME(old_rank)) + ".\n");
            return 1;
        }
        if (rank != WIZ_MAGE)
        {
            write("You may only demote yourself to mage.\n");
            return 1;
        }
    }

    /* See whether the transition is legal. */
    if (member_array(rank, WIZ_RANK_POSSIBLE_CHANGE(old_rank)) == -1)
    {
	write("It is not possible to " +
	      ((rank > old_rank) ? "promote" : "demote") + " someone from " +
	      WIZ_RANK_NAME(old_rank) + " to " + WIZ_RANK_NAME(rank) + ".\n");
	return 1;
    }

    /* Promote a mortal into wizardhood. */
    if (old_rank == WIZ_MORTAL)
    {
	transform_mortal_into_wizard(wname, cmder);
    }

    /* Do it. */
    do_change_rank(wname, rank, cmder);

    /* For some ranks, we need to do something after the actual change. */
    switch(rank)
    {
    case WIZ_MORTAL:
	/* If the wizard is in a domain, kick him out. */
	if (strlen(dname))
	{
	    add_wizard_to_domain("", wname, cmder);
	}

	/* Burry all evidence of his/her existing. */
	m_wizards = m_delete(m_wizards, wname);
	remove_all_sanctions(wname);

	/* Tell him/her the bad news and boot him. */
	if (objectp(wizard = find_player(wname)))
	{
	    tell_object(wizard, "You are being reverted back to mortal by " +
			capitalize(cmder) + ". This means that you have to " +
			"create a new character again to continu playing.\n");

	    /* ... and ... POOF! ;-) */
	    wizard->quit();
	    if (objectp(wizard))
	    {
		do_debug("destroy", wizard);
	    }
	}

	/* Rename the file after the booting. We might change our mind ;-) */
	if (file_size(PLAYER_FILE(wname) + ".o") >= 0)
	    rename(PLAYER_FILE(wname) + ".o", PLAYER_FILE(wname) + ".o.wizard");
	break;

    case WIZ_APPRENTICE:
	if ((old_rank == WIZ_MORTAL) &&
	    (objectp(wizard = find_player(wname))))
	{
	    wizard->set_wiz_level();
	    wizard->set_default_start_location(WIZ_ROOM);
	    wizard->save_me(1);	
	    wizard->update_hooks();
	}
    case WIZ_PILGRIM:
/*
    case WIZ_RETIRED:
 */
	if (strlen(dname))
	{
	    add_wizard_to_domain("", wname, cmder);
	}
	break;
    }

    write(((rank > old_rank) ? "Promoted " : "Demoted ") +
	  capitalize(wname) + " to " + WIZ_RANK_NAME(rank) + " (" +
	  WIZ_RANK_START_LEVEL(rank) + ").\n");
    return 1;
}

/*
 * Function name: wizard_change_level
 * Description  : Using this function, a liege, arch or keeper can try to
 *                alter the level of a player.
 * Arguments    : string wname - the wizard whose level is to be changed.
 *                int rank     - the new level.
 * Returns      : int 1/0 - success/failure.
 */
int
wizard_change_level(string wname, int level)
{
    string cmder;
    object wizard;

    /* May only be called from the Lord soul. */
    if (!CALL_BY(WIZ_CMD_LORD))
    {
	return 0;
    }

    cmder = getwho();
    wname = lower_case(wname);
    if (!sizeof(m_wizards[wname]))
    {
	write(capitalize(wname) + " is not a wizard. To promote " +
	      "him/her to wizard, use 'promote " + wname + " <rank>'.\n");
	return 1;
    }

    if ((m_wizards[cmder][FOB_WIZ_RANK] <= m_wizards[wname][FOB_WIZ_RANK]) ||
	(((m_wizards[cmder][FOB_WIZ_RANK] == WIZ_LORD) ||
	  (m_wizards[cmder][FOB_WIZ_RANK] == WIZ_STEWARD)) &&
	 (m_wizards[cmder][FOB_WIZ_DOM] != m_wizards[wname][FOB_WIZ_DOM])))
    {
	write("You have no control over the level of " + capitalize(wname) +
	      ".\n");
	return 1;
    }

    if (m_wizards[wname][FOB_WIZ_LEVEL] == level)
    {
	write(capitalize(wname) + " already has level " + level + ".\n");
	return 1;
    }

    if (level < WIZ_RANK_MIN_LEVEL(m_wizards[wname][FOB_WIZ_RANK]))
    {
	write("The minimum level for " +
	      LANG_ADDART(WIZ_RANK_NAME(m_wizards[wname][FOB_WIZ_RANK])) +
	      " is " + WIZ_RANK_MIN_LEVEL(m_wizards[wname][FOB_WIZ_RANK]) +
	      ".\n");
	return 1;
    }

    if (level > WIZ_RANK_MAX_LEVEL(m_wizards[wname][FOB_WIZ_RANK]))
    {
	write("The maximum level for " +
	      LANG_ADDART(WIZ_RANK_NAME(m_wizards[wname][FOB_WIZ_RANK])) +
	      " is " + WIZ_RANK_MAX_LEVEL(m_wizards[wname][FOB_WIZ_RANK]) +
	      ".\n");
	return 1;
    }

    log_file("LEVEL",
	     sprintf("%s %-11s: %-4s (%2d) -> %-4s (%2d) by %s.\n",
		     ctime(time()),
		     capitalize(wname),
		     WIZ_RANK_SHORT_NAME(m_wizards[wname][FOB_WIZ_RANK]),
		     m_wizards[wname][FOB_WIZ_LEVEL],
		     WIZ_RANK_SHORT_NAME(m_wizards[wname][FOB_WIZ_RANK]),
		     level,
		     capitalize(cmder)));

    if (objectp(wizard = find_player(wname)))
    {
	tell_object(wizard, "You been " +
		    ((level > m_wizards[wname][FOB_WIZ_LEVEL]) ? "promoted" :
		     "demoted") + " to level " + level + " by " +
		    capitalize(cmder) + ".\n");
    }

    write(((level > m_wizards[wname][FOB_WIZ_LEVEL]) ? "Promoted " :
	   "Demoted ") + capitalize(wname) + " to level " + level + ".\n");

    m_wizards[wname][FOB_WIZ_LEVEL] = level;
    m_wizards[wname][FOB_WIZ_CHLEVEL] = cmder;
#ifdef FOB_KEEP_CHANGE_TIME
    m_wizards[wname][FOB_WIZ_CHLTIME] = time();
#endif FOB_KEEP_CHANGE_TIME

    save_master();

    return 1;
}

/*
 * Function name: set_keeper
 * Description  : We have a special command to make keepers. Only keepers
 *                may use it to stress its special importance, but hacking
 *                the master save file should not be necessary.
 * Arguments    : string wname - the arch to make keeper (or vice versa)
 *                int promote  - 1 = make keeper, 0 = make arch.
 * Returns      : int 1/0 - success/ failure.
 */
int
set_keeper(string wname, int promote)
{
    /* May only be called from the keeper soul. */
    if (!CALL_BY(WIZ_CMD_KEEPER))
    {
	return 0;
    }

    do_change_rank(wname, (promote ? WIZ_KEEPER : WIZ_ARCH), getwho());
    return 1;
}

/*
 * Function name: create_wizard
 * Description  : Certain objects in the game are allowed to promote a mortal
 *                into wizardhood. This function should be called to do the
 *                transformation. The mortal is then made into apprentice.
 * Arguments    : string name - the name of the mortal to be promoted.
 */
void
create_wizard(string name)
{
    if (member_array(function_exists("make_wiz", previous_object()),
		     WIZ_MAKER) == -1)
    {
	return;
    }

    transform_mortal_into_wizard(name, ROOT_UID);
    do_change_rank(name, WIZ_APPRENTICE, ROOT_UID);
}

/*
 * Function name: query_wiz_pretitle
 * Description  : Gives a pretitle for a specific wizard type.
 * Arguments    : mixed wiz - the wizard object or name.
 * Returns      : string - the pretitle for the wizard.
 */
string
query_wiz_pretitle(mixed wiz)
{
    string name;
    int gender;

    /* Knowing the name, find the objectpointer for the gender. */
    if (stringp(wiz))
    {
	name = wiz;
	wiz = find_player(wiz);
    }

    /* If there is no such object, clone the finger-player and clean up
     * after ourselves. Else, pick up the info from the wizard object.
     */
    if (!objectp(wiz))
    {
	set_auth(this_object(), "root:root");
	wiz = finger_player(name);

	if (!objectp(wiz))
	{
	    return "";
	}

	gender = wiz->query_gender();
	wiz->remove_object();
    }
    else
    {
        gender = wiz->query_gender();
	name = wiz->query_real_name();
    }

    switch(query_wiz_rank(name))
    {
	case WIZ_MORTAL:
	    return "";
	    
        case WIZ_APPRENTICE:
	    return "Rekrut";

        case WIZ_PILGRIM:
            return "Konsul";
            
        case WIZ_RETIRED:
            return ({ "Brat", "Siostra", "Brat" })[gender];

        case WIZ_NORMAL:
            if (query_wiz_level(wiz->query_real_name()) == 
                WIZ_RANK_START_LEVEL(WIZ_NORMAL))
                
                return ({ "Praktykant", "Praktykantka", "Praktykant" })[gender];
    	    return ({ "Szlachetny", "Szlachetna", "Szlachetne" })[gender];

        case WIZ_MAGE:
            return ({ "Czcigodny", "Czcigodna", "Czcigodne" })[gender];

        case WIZ_STEWARD:
            return ({ "Senior", "Seniorita", "Seniorito" })[gender];

        case WIZ_LORD:
    	    return ({ "Lord", "Lady", "Lord" })[gender];

        case WIZ_ARCH:
            return ({ "Mistrz", "Mistrzyni", "Mistrzunio" })[gender];

        case WIZ_KEEPER:
            if (wiz->query_real_name() == "lewy")
                return "Prezes";

            return ({ "Mistrz", "Mistrzyni", "Mistrzunio" })[gender];
    }

    /* Should never end up here. */
    return "";
}

/*
 * Function name: query_wiz_path
 * Description  : Gives a default path for a wizard
 * Arguments    : string wname - the wizard name.
 * Returns      : string - the default path for the wizard.
 */
string
query_wiz_path(string wname)
{
    string dname;

    /* A domains path. */
    dname = capitalize(wname);
    if (sizeof(m_domains[dname]))
    {
	return "/d/" + dname;
    }

    /* Root. */
    wname = lower_case(wname);
    if (wname == ROOT_UID)
    {
	return "/syslog";
    }

    /* Not a wizard, ie no path. */
    if (!sizeof(m_wizards[wname]))
    {
	return "";
    }

    /* A wizard who is a domain member. */
    dname = m_wizards[wname][FOB_WIZ_DOM];
    if (strlen(dname))
    {
	return "/d/" + dname + "/" + wname;
    }

    /* Non-domain members, ie apprentices, pilgrims and retired people. */
    return "/doc";
}

/*
 * Function name: query_mage_links
 * Description  : Finds all WIZARD_DOMAIN mages and makes a list of all links.
 * Returns      : string * - the list.
 */
public string *
query_mage_links()
{
    string *links;
    int index;
    int size;

    index = -1;
    links = sort_array( ({ }) + m_domains[WIZARD_DOMAIN][FOB_DOM_MEMBERS]);
    size  = sizeof(links);
    while(++index < size)
    {
	links[index] = "/d/" + WIZARD_DOMAIN + "/" + links[index] + "/" +
	    WIZARD_LINK;
    }

    return links;
}

/*
 * Function name: query_domain_links
 * Description  : Make a list of domainfiles to link.
 * Returns      : string * - the list.
 */
public string *
query_domain_links()
{
    int index;
    int size;
    string *links;

    index = -1;
    links = sort_array(m_indices(m_domains));
    size = sizeof(links);
    while(++index < size)
    {
	links[index] = "/d/" + links[index] + "/" + DOMAIN_LINK;
    }

    return links;
}

/*
 * Function name: retire_wizard
 * Description  : Wizards may retire from active coding as they please.
 *                They loose their coding rights and are placed in the
 *                special retired wizards rank.
 * Returns      : int 1/0 - success/failure.
 */
int
retire_wizard()
{
    string cmder;
    int    rank;

    /* May only be called from the 'normal' wizard soul. */
    if (!CALL_BY(WIZ_CMD_NORMAL))
    {
	return 0;
    }

    cmder = getwho();
    rank  = m_wizards[cmder][FOB_WIZ_RANK];

    /* People who aren't full wizards cannot retire. */
    if (rank < WIZ_NORMAL)
    {
	notify_fail("You may not retire. If you want to be retired, " +
		    "ask a member of the administration for permission.\n");
	return 0;
    }

    if (rank >= WIZ_ARCH)
    {
	notify_fail("No way. Archwizards and keepers cannot retire.\n");
	return 0;
    }

    /* Do it. */
    do_change_rank(cmder, WIZ_RETIRED, cmder);
//    add_wizard_to_domain("", cmder, cmder);

    write("You just retired from coding.\n");
    return 1;
}

/************************************************************************
 *
 * The global read administration code.
 *
 */

/*
 * Function name: add_global_read
 * Description  : Add a wizard to the global read list.
 * Arguments    : string wname   - the name of the wiz to be added.
 *		  string comment - a suitable comment to store along with
 *                                 the name telling why the player has it.
 * Returns      : int 1/0 - success/failure.
 */
int
add_global_read(string wname, string comment)
{
    string cmder;
    object wiz;

    /* May only be called from the arch soul. */
    if (!CALL_BY(WIZ_CMD_ARCH))
    {
	return 0;
    }

    cmder = getwho();

    /* Only full wizards can have global read access. */
    wname = lower_case(wname);
    if (m_wizards[wname][FOB_WIZ_RANK] < WIZ_NORMAL)
    {
	notify_fail("Not a full wizard: " + capitalize(wname) + ".\n");
	return 0;
    }

    if (!strlen(comment))
    {
	notify_fail("No comment added to the command line.\n");
	return 0;
    }

    /* Only add global access if the player does not have it already. */
    if (m_global_read[wname])
    {
	notify_fail("The wizard " + capitalize(wname) +
		    " already has global access.\n");
	return 0;
    }

    /* Add the stuff, save the master and tell the caller. */
    m_global_read[wname] = ({ cmder, comment });
    save_master();

    if (objectp(wiz = find_player(wname)))
    {
	tell_object(wiz, "Global read access has been granted to you by " +
		    capitalize(cmder) + ".\n");
    }

    write("Added " + capitalize(wname) + " to have global read access.\n");
    return 1;
}

/*
 * Function name: remove_global_read
 * Description  : Remove a wizard from the global access list.
 * Arguments    : string wname - the wizard to remove.
 * Returns      : int 1/0 - true if successful.
 */
int
remove_global_read(string wname)
{
    string cmder;
    object wiz;

    /* May only be called from the arch soul. */
    if (!CALL_BY(WIZ_CMD_ARCH))
    {
	return 0;
    }

    cmder = getwho();

    /* See whether the person indeed has global read access. */
    wname = lower_case(wname);
    if (!m_global_read[wname])
    {
	notify_fail("Wizard " + capitalize(wname) + " not registered.\n");
	return 0;
    }

    /* Remove the entry, save the master and notify the caller. */
    m_global_read = m_delete(m_global_read, wname);
    save_master();

    if (objectp(wiz = find_player(wname)))
    {
	tell_object(wiz, "Your global read access has been revoked by " +
		    capitalize(cmder) + ".\n");
    }

    write("Removed global read access from " + capitalize(wname) + ".\n");
    return 1;
}

/*
 * Function name: query_global_read
 * Description  : This function returns a mapping with the names of the
 *                people that have global read access. For the format of
 *                the file, see the header of this file.
 * Returns      : mapping - the global read list.
 */
mapping 
query_global_read()
{
    return secure_var(m_global_read);
}

/************************************************************************
 *
 * The trainee administration code.
 *
 */

/*
 * Function name: add_trainee
 * Description  : Add a wizard to the list of trainees. This may only be
 *                called from the Lord soul.
 * Arguments    : string wname - the name of the wizard to mark as trainee.
 * Returns      : int 1/0 - success/failure.
 */
int
add_trainee(string wname)
{
    string cmder;
    object wiz;

    /* May only be called from the Lord soul. */
    if (!CALL_BY(WIZ_CMD_LORD))
    {
	return 0;
    }

    cmder = getwho();

    /* Only full wizards can be trainees. */
    wname = lower_case(wname);
    
    if (!m_wizards[wname])
    {
        notify_fail("Nie ma czarodzieja o imieniu '" + capitalize(wname) + 
            "'.\n");
        return 0;
    }
    
    if (m_wizards[wname][FOB_WIZ_RANK] != WIZ_NORMAL)
    {
	notify_fail(capitalize(wname) + " is not a full wizard.\n");
	return 0;
    }

    /* Lords and stewards may only meddle in their own domain. */
    if ((m_wizards[cmder][FOB_WIZ_RANK] <= WIZ_LORD) &&
        (m_wizards[cmder][FOB_WIZ_DOM] != m_wizards[wname][FOB_WIZ_DOM]))
    {
	notify_fail(capitalize(wname) + " is not a member of " +
		    m_wizards[cmder][FOB_WIZ_DOM] + ".\n");
	return 0;
    }

    /* Only add traineeship if the wizard is not one already. */
    if (m_trainees[wname])
    {
	notify_fail("The wizard " + capitalize(wname) +
		    " already is a trainee.\n");
	return 0;
    }

    /* Add the stuff, save the master and tell the caller. */
    m_trainees[wname] = 1;
    save_master();

    if (objectp(wiz = find_player(wname)))
    {
	tell_object(wiz, "You have been made a trainee domain member by " +
		    capitalize(cmder) + ".\n");
    }

    write("Made " + capitalize(wname) + " a trainee.\n");
    return 1;
}

/*
 * Function name: remove_trainee
 * Description  : Remove a wizard from the list of trainees. This may only
 *                be called from the Lord soul.
 * Arguments    : string wname - the wizard to remove.
 * Returns      : int 1/0 - true if successful.
 */
int
remove_trainee(string wname)
{
    string cmder;
    object wiz;

    /* May only be called from the Lord soul. */
    if (!CALL_BY(WIZ_CMD_LORD))
    {
	return 0;
    }

    cmder = getwho();
    wname = lower_case(wname);

    /* Lords and stewards may only meddle in their own domain. */
    if ((m_wizards[cmder][FOB_WIZ_RANK] <= WIZ_LORD) &&
        (m_wizards[cmder][FOB_WIZ_DOM] != m_wizards[wname][FOB_WIZ_DOM]))
    {
	notify_fail(capitalize(wname) + " is not a member of " +
		    m_wizards[cmder][FOB_WIZ_DOM] + ".\n");
	return 0;
    }

    /* Only remove traineeship if the wizard is a trainee. */
    if (!m_trainees[wname])
    {
	notify_fail(capitalize(wname) + " is not a trainee.\n");
	return 0;
    }

    /* Remove the stuff, save the master and tell the caller. */
    m_trainees = m_delete(m_trainees, wname);
    save_master();

    if (objectp(wiz = find_player(wname)))
    {
	tell_object(wiz, "Your traineeship has been lifted by " +
		    capitalize(cmder) + ".\n");
    }

    write("Lifted traineeship from " + capitalize(wname) + ".\n");
    return 1;
}

/*
 * Function name: query_trainee
 * Description  : This function returns true if the person is a trainee.
 * Arguments    : string wname - the wizard to test.
 * Returns      : int 1/0 - trainee/not a trainee.
 */
int
query_trainee(string wname)
{
    return m_trainees[lower_case(wname)];
}

/*
 * Function name: query_trainees
 * Description  : This function returns an array of the people who are
 *                marked trainee in their domain.
 * Returns      : string * - the list of trainees.
 */
mapping
query_trainees()
{
    return m_indices(m_trainees);
}

/************************************************************************
 *
 * The code securing the channels.
 *
 */

/*
 * Function name: set_channels
 * Description  : This function is used by the apprentice commandsoul to
 *                store the mapping with the multi-wizline channels in a
 *                safe place.
 * Arguments    : mapping channels - the mapping with the channels.
 */
int
set_channels(mapping channels)
{
    /* May only be called from the apprentice soul. */
    if (!CALL_BY(WIZ_CMD_APPRENTICE))
    {
	return 0;
    }

    set_auth(this_object(), "root:root");
    save_map(channels, CHANNELS_SAVE);
    return 1;
}

/*
 * Function name: query_channels
 * Description  : This function is used by the apprentice commandsoul to
 *                retrieve the channels from disk. This is done since they
 *                are stored in a safe place.
 * Returns      : mapping - the mapping with the channels.
 */
mapping
query_channels()
{
    if (!CALL_BY(WIZ_CMD_APPRENTICE))
    {
	return 0;
    }

    set_auth(this_object(), "root:root");
    return restore_map(CHANNELS_SAVE);
}
