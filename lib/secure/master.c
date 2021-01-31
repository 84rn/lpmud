/*
 * /secure/master.c
 *
 * This is the LPmud master object, used from version 3.0.
 *
 * It is the first object loaded, save the simul_efun object, but that
 * isn't a real object rather set of simulated efuns that have to be
 * somewhere.
 *
 * Everything written with 'write()' at startup will be printed on
 * stdout.
 *
 * 1. create() will be called first.
 * 2. flag() will be called once for every argument to the flag -f
 * 	supplied to the driver.
 * 3. start_boot() will be called.
 * 4. preload_boot() will be called for each file to preload.
 * 5. final_boot() will be called.
 * 6. The game will enter multiuser mode, and enable log in.
 */

#pragma no_clone
#pragma no_inherit
#pragma save_binary
#pragma strict_types

/*
 * This is the only object in the game in which we have to use absolute path'
 * to the inclusion files. The reason for this is that the function that
 * contains the search-path for inclusion files is defined in this object.
 */
#include "/sys/debug.h"
#include "/sys/filepath.h"
#include "/sys/files.h"
#include "/sys/language.h"
#include "/sys/living_desc.h"
#include "/sys/log.h"
#include "/sys/macros.h"
#include "/sys/mail.h"
#include "/sys/std.h"
#include "/sys/stdproperties.h"
#include "/sys/time.h"
#include "/sys/udp.h"

#define SAVEFILE   ("/syslog/KEEPERSAVE")
#define GAME_START ("/GAME_START")
#define DEBUG_RESTRICTED ({ \
            "dump_objects",	/* informacje o rozmieszczeniu obiektow */ \
         /* "get_variables", */	/* restrykcje wbudowane w do_debug() */	\
            "inhibitcallouts",	/* wylaczenie wykonywania alarmow */	\
            "mudstatus",	/* jeszcze nie wiem dlaczego :) */	\
            "send_udp",		/* podszywanie sie pod intermuda */	\
            "set_swap",		/* grzebanie w pamieci */		\
            "shared_strings",	/* _zdecydowanie_ zbyt duzo informacji */ \
            "shutdown",		/* Apokalipsa bez ostrzezenia */	\
            "swap",		/* grzebanie w pamieci (wylaczone) */	\
            "warnobsolete",	/* za duzo spamu w lplogu */		\
                         })
#define RESET_TIME (900.0) /* 15 minutes */

/* All prototypes have been placed in /secure/master.h */
#include "/secure/master.h"

#include "/secure/master/fob.c"
#include "/secure/master/siteban.c"
#include "/secure/master/spells.c"
#include "/secure/master/language.c"
#include "/secure/master/notify.c"
#include "/secure/master/sanction.c"
#include "/secure/master/guild.c"
#include "/secure/master/mail_admin.c"

/*
 * The global variables that are saved in the SAVEFILE.
 */
private int     game_started;
private string  *def_locations;
private string  *temp_locations;
private mapping known_muds;
private int     runlevel;

/*
 * The global variables that are not saved.
 */
private static int     mem_fail_flag;
private static int     memory_limit;
private static mapping command_substitute;
private static string  udp_manager;
private static mapping admin_teams;
private static int     irregular_uptime;

/*
 * Function name: create
 * Description  : This is the first function called in this object.
 */
void
create() 
{
    /* Using a global variable for this is exactly TWICE as fast as using a
     * defined mapping. At this point we only have the default direction
     * commands here because they are all over the game in add_action()s.
     * The other abbreviations can easily be added to the respective souls.
     */
    command_substitute = ([
			   "n"  : "polnoc",
			   "ne" : "polnocny-wschod",
			   "e"  : "wschod",
			   "se" : "poludniowy-wschod",
			   "s"  : "poludnie",
			   "sw" : "poludniowy-zachod",
			   "w"  : "zachod",
			   "nw" : "polnocny-zachod",
			   "u"  : "gora",
			   "d"  : "dol",
			  ]);

    mem_fail_flag = 0;
#ifdef LARGE_MEMORY_LIMIT
    memory_limit = LARGE_MEMORY_LIMIT;
#else
    memory_limit = 28000000;
#endif
    set_auth(this_object(), "root:root");

    /* We reset the master every RESET time seconds, initially synchronizing
     * it at exactly 1 second after that occurance. I.e. if RESET_TIME is
     * 1 hour, it will be started exactly one second after the top of the
     * hour.
     */
    set_alarm(((RESET_TIME + 1.0) - (itof(time() % ftoi(RESET_TIME)))),
	      RESET_TIME, reset_master);

    /* Compute the uptime for this reboot. */
#ifdef REGULAR_UPTIME
    irregular_uptime = (REGULAR_UPTIME * 3600);
#ifdef UPTIME_VARIATION
    irregular_uptime +=
        (random(UPTIME_VARIATION * 3600) - (UPTIME_VARIATION * 1800));
#endif UPTIME_VARIATION
#endif REGULAR_UPTIME
}

/*
 * Function name: reset_master
 * Description  : This function will be called regularly, each RESET_TIME
 *                seconds. Since we want only one alarm running, from this
 *                function we can make calls to other modules that need it.
 */
static void
reset_master()
{
    /* Check whether there is still enough memory to run the game. The
     * argument 1 means decay the domain experience.
     */
    check_memory(1);

    /* Gather information for the graph command. */
    probe_for_graph();

    /* Save the master. */
    save_master();
}

/*
 * Function name: short
 * Description  : This function returns the short description of this object.
 * Returns      : string - the short description.
 */
string 
short() 
{
    return "the hole of the donut";
}

/*
 * Function name: save_master
 * Description  : This function saves the master object to a file.
 */
static void
save_master()
{
    set_auth(this_object(), "root:root");

    save_object(SAVEFILE); 
}

/*********************************************************************
 *
 * GD - INTERFACE LFUNS
 *
 * The below lfuns are called from the Gamedriver for various reasons.
 */

/*
 * Called on startup of game if '-f' is given on the commandline.
 *
 * To test a new function xx in object yy, do
 * driver "-fcall yy xx arg" "-fshutdown"
 */
static void 
flag(string str) 
{
    string file, arg;

    if (game_started)
	return;

    if (sscanf(str, "for %d", arg) == 1) 
    {
	return;
    }

    if (str == "shutdown") 
    {
	do_debug("shutdown");
	return;
    }

    if (sscanf(str, "echo %s", arg) == 1) 
    {
	write(arg + "\n");
	return;
    }
    
    if (sscanf(str, "call %s %s", file, arg) == 2) 
    {
	arg = (string)call_other(file, arg);
	write("Got " + arg + " back.\n");
	return;
    }
    write("master: Unknown flag " + str + "\n");
}

/*
 * Function name:   get_mud_name
 * Description:     Gives the name of the mud. The name will be #defined in
 *		    all files compiled in the mud. It can not contain spaces.
 * Returns:         Name of the mud.
 *		    We always return a string but we must declare it mixed
 *		    otherwise the type checker gets allergic reactions.
 */
mixed
get_mud_name() 
{
#ifdef MUD_NAME    
    mixed n;

    n = MUD_NAME;
    if (mappingp(n))
    {
	if (stringp(n[debug("mud_port")]))
	    return n[debug("mud_port")];
	else
	    return n[0];
    }
    else if (stringp(MUD_NAME))
    {
	return MUD_NAME;
    }
#endif MUD_NAME
    return "LPmud(" + debug("version") + ":" + MUDLIB_VERSION + ")";
}

/*
 * Function name: get_root_uid
 * Description  : Gives the uid of the root user.
 * Returns      : string - the name of the 'root' user.
 */
string
get_root_uid() 
{
    return ROOT_UID;
}

/*
 * Function name: get_bb_uid
 * Description  : Gives the uid of the backbone user. That is, each user
 *                that does not have an uid of its own. Apart from backbone
 *                all wizard names and domain names are valid uids. And
 *                root naturally.
 * Returns      : string - the name of the 'backbone' user.
 */
string
get_bb_uid()
{
    return BACKBONE_UID;
}

/*
 * Function name: get_vbfc_object
 * Description  : This function returns the objectpointer to the VBFC
 *                object.
 * Returns      : object - the objectpointer to the VBFC object.
 */
object
get_vbfc_object()
{
    return VBFC_OBJECT->ob_pointer();
}

/*
 * Function name: connect
 * Description  : This function is called every time a player connects. We
 *                return a clone of the login object through which the socket
 *                of the new player is connected.
 *                The efun input_to() cannot be called from here.
 * Returns      : object - the login object.
 */
static object
connect() 
{
    write("\n");
    set_auth(this_object(), "root:root");

    return clone_object(LOGIN_OBJECT);
}

/*
 * Function name: valid_set_auth
 * Description  : Whenever the hidden authorization information of an object,
 *                i.e. the uid or the euid, is altered this function is being
 *                called. It checks the format of the new autorization
 *                information and makes sure that the change is valid.
 * Arguments    : object setter      - the object forcing the change.
 *                object getting_set - the object being changed.
 *                string value       - the new value.
 * Returns      : string - the new value.
 */
string
valid_set_auth(object setter, object getting_set, string value)
{
    string *oldauth;
    string *newauth;
    string auth = query_auth(getting_set);

    if (!stringp(value) ||
    	((setter != this_object()) &&
	 (setter != find_object(SIMUL_EFUN))))
    {
	return auth;
    }

    newauth = explode(value, ":");
    if (sizeof(newauth) != 2)
    {
	return auth;
    }
    
    oldauth = (stringp(auth) ? explode(auth, ":") : ({ "0", "0"}) );
    if (newauth[0] == "#")
    {
	newauth[0] = oldauth[0];
    }
    if (newauth[1] == "#")
    {
	newauth[1] = oldauth[1];
    }

    return implode(newauth, ":");
}

/*
 * Function name:   valid_seteuid
 * Description:     Checks if a certain user has the right to set a certain
 *                  objects effective 'userid'. All objects has two 'uid'
 *                  - Owner userid: Wizard who caused the creation.
 *                  - Effective userid: Wizard responsible for the objects
 *                    actions.
 *                  When an object creates a new object, the new objects
 *                  'Owner userid' is set to creating objects 'Effective
 *                  userid'.
 * Arguments:       ob:   Object to set 'effective' user id in.
 *                  str:  The effuserid to be set.
 * Returns:         True if set is allowed.
 * Note:	    Setting of effuserid to userid is allowed in the GD as
 *		    well as setting effuserid to 0.
 */
int
valid_seteuid(object ob, string str) 
{
    string uid = getuid(ob);

    /* Root can be anyone it pleases. */
    if (uid == ROOT_UID)
    {
	return 1;
    }

    /* We can be ourselves. That is... set the euid to our uid. */
    if (uid == str)
    {
	return 1;
    }

    /* Arches and keepers can be anyone they please, except for root and
     * other arches or keepers.
     */
    if (query_wiz_rank(uid) >= WIZ_ARCH)
    {
    	return ((query_wiz_rank(str) < WIZ_ARCH) &&
    		(str != ROOT_UID));
    }

    /* When arches and keepers are in a domain, the lord can't change
     * euid in his objects to the arch.
     */
    if (query_wiz_rank(str) >= WIZ_ARCH)
    {
	return 0;
    }

    /* A lord can be anyone of his subject wizards. */
    if ((uid == query_domain_lord(str)) ||
	(uid == query_domain_lord(query_wiz_dom(str))))
    {
	return 1;
    }

    /* No one else can be anything. */
    return 0;
}

/*
 * Function name: valid_write
 * Description  : Checks whether a certain user has the right to write a
 *                particular file.
 * Arguments    : string path  - the path name of the file to be write.
 *                mixed writer - the name or object of the writer.
 *                string func  - the calling function.
 * Returns      : int 1/0 - allowed/disallowed.
 */
int
valid_write(string file, mixed writer, string func) 
{
    string *dirs;
    string dname;
    string wname;
    string dir;

    if (objectp(writer))
    {
	writer = geteuid(writer);
    }

    /* Root may do as he please. */
    if (writer == ROOT_UID)
    {
        return 1;
    }

    /* Keepers and arches may do as they please. */
    if (query_wiz_rank(writer) >= WIZ_ARCH)
    {
	return 1;
    }

    /* Anonymous objects cannot do anything. */
    if (!stringp(writer) ||
	!strlen(writer))
    {
	return 0;
    }

    dirs = explode(file, "/") - ({ "" });

    /* Everyone may write in /open */
    if (dirs[0] == "open")
    {
	return 1;
    }

    /* Must be /d/Domain/something at least. */
    if (sizeof(dirs) < 3)
    {
	return 0;
    }

    dname = dirs[1];

    /* W /d/common moga zapisywac wylacznie lordowie i administracja.
     * Stewardzi moga zapisywac w /d/common/Domena.
     */
    if (dname == "common")
    {
        if (query_wiz_rank(writer) == WIZ_LORD)
            return 1;
            
        if ((query_wiz_rank(writer) == WIZ_STEWARD) &&
            (dirs[2] == query_wiz_dom(writer)))
            return 1;
            
        return 0;
    }

    /* The domain must be an existing domain. */
    if ((dirs[0] != "d") ||
	(query_domain_number(dname) == -1))
    {
	return 0;
    }

    /* A Lord can write anywhere in the domain, and wizards can naturally
     * write their own directory.
     */
    wname = dirs[2];
    if ((query_domain_lord(dname) == writer) ||
	((wname == writer) &&
	 (query_wiz_dom(wname) == dname)))
    {
	return 1;
    }

    /* Steward can write anywhere, except in the Lord's directory. */
    if ((query_domain_steward(dname) == writer) &&
	(wname != query_domain_lord(dname)))
    {
	return 1;
    }

    /* We have to check for the directory sanctions here because they might
     * disclose the private directories.
     */
    if (recursive_valid_write_path_sanction(writer, dname, dirs[2..]))
    {
	return 1;
    }

    /* Only the Lord of a domain can write in the 'domain' directory. */
    if (wname == "domain")
    {
	return 0;
    }

    /* The private directory of a wizard is closed to all others. */
    dir = ((sizeof(dirs) > 3) ? dirs[3] : "");
    if (dir == "private")
    {
	return 0;
    }

    /* To write something now you need a sanction. If it is a wizard directory
     * it must be a personal sanction or a domain 'write all' sanction. */
    if (query_wiz_dom(wname) == dname)
    {
	return (valid_write_sanction(writer, wname) ||
		valid_write_all_sanction(writer, dname));
    }


    /* Trainees can only write in their own directory and in sanctioned
     * directories. No point in checking further since if you want to give
     * a trainee a global sanction, you'd better remove the traineeship.
     */
    if (m_trainees[writer])
    {
	return 0;
    }

    /* Wizards can write everywhere in the domain and so can the domain itself
     * unless this is the domain for 'lonely' wizards or unless the directory
     * belongs to another wizard.
     */
    if ((query_wiz_dom(wname) != dname) &&
	((query_wiz_dom(writer) == dname) ||
	 (dname == writer)))
    {
	return (dname != WIZARD_DOMAIN);
    }

    /* If the directory is in another place, just a domain write sanction
     * suffices.
     */
    {
	return (valid_write_sanction(writer, dname) ||
		valid_write_all_sanction(writer, dname));
    }

    return 0;
}

/*
 * Function name: valid_read
 * Description  : Checks if a certain user has the right to read a file.
 * Arguments    : string path  - path name of the file to be read.
 *                mixed reader - the object or name of the reader.
 *		  string func  - the calling function.
 * Returns      : int 1/0 - allowed/disallowed.
 */
int
valid_read(string file, mixed reader, string func) 
{
    string *dirs;
    string dname;
    string wname;
    string dir;

    /* Everyone is allowed to see the time or size of a file. */
    if ((func == "file_time") ||
	(func == "file_size"))
    {
	return 1;
    }

    if (objectp(reader))
    {
	reader = geteuid(reader);
    }

    /* Allow read in / */
    if (file == "/")
    {
	return 1;
    }

    /* Root and archwizards and keepers may do as they please. */
    if ((reader == ROOT_UID) ||
	(query_wiz_rank(reader) >= WIZ_ARCH))
    {
	return 1;
    }

    /* Anonymous objects cannot do anything. */
    if (!stringp(reader) ||
	!strlen(reader))
    {
	return 0;
    }

    /* These directories are closed to all save the admin. */
    dirs = explode(file, "/") - ({ "" });
    if ((dirs[0] == "players") ||
	(dirs[0] == "binaries"))
    {
	return 0;
    }

    /* The AoP team and Lords can see various logs. Others cannot see them. */
    if ((dirs[0] == "syslog") &&
        (dirs[1] == "log"))
    {
        if (member_array(dirs[2], AOP_TEAM_LOGS) > -1 &&
            (query_wiz_rank(reader) >= WIZ_LORD ||
             query_admin_team_member(reader, "aop")))
        {
            return 1;
        }

        return 0;
    }

    /* Allow all directories that aren't in a domain. */
    if ((dirs[0] != "d") ||
	(sizeof(dirs) == 1))
    {
	return 1;
    }

    dname = dirs[1];

    /* W /d/common moga czytac wylacznie lordowie i administracja.
     * Stewardzi moga czytac w /d/common/Domena.
     */
    if (dname == "common")
    {
        if (query_wiz_rank(reader) == WIZ_LORD)
            return 1;
            
        if ((query_wiz_rank(reader) == WIZ_STEWARD) &&
            (dirs[2] == query_wiz_dom(reader)))
            return 1;
            
        return 0;
    }
    
    /* The domain must be an existing domain. */
    if (query_domain_number(dname) == -1)
    {
	return 0;
    }

    /* A Lord can read anywhere in the domain, and wizards can naturally
     * read their own directory.
     */
    wname = ((sizeof(dirs) > 2) ? dirs[2] : "");
    if ((query_domain_lord(dname) == reader) ||
	((wname == reader) &&
	 (query_wiz_dom(wname) == dname)))
    {
	return 1;
    }

    /* The open directory of a domain or wizard is free to read for all. */
    dir = ((sizeof(dirs) > 3) ? dirs[3] : "");
    if ((wname == "open") ||
	(dir == "open"))
    {
	return 1;
    }

    /* We have to check for the directory sanctions here because they might
     * disclose the private directories.
     */
    if (strlen(wname) &&
	recursive_valid_read_path_sanction(reader, dname, dirs[2..]))
    {
	return 1;
    }

    /* The private directory of a wizard is closed to all others. */
    if (dir == "private")
    {
	return 0;
    }

    /* Some people have been granted global read rights. Global read includes
     * the private directory of a domain.
     */
    if (m_global_read[reader])
    {
	return 1;
    }

    /* Wizards can read everywhere in the domain and so can the domain itself
     * unless this is the domain for 'lonely' wizards.
     */
    if ((query_wiz_dom(reader) == dname) ||
	(dname == reader))
    {
	return (dname != WIZARD_DOMAIN);
    }

    /* The private directory of a domain is close to all others. */
    if (wname == "private")
    {
	return 0;
    }

    /* To read something now you need a sanction. If it is a wizard directory
     * it must be a personal sanction or a domain 'read all' sanction. If the
     * directory is in another place, just a domain read sanction suffices.
     */
    if (query_wiz_dom(wname) == dname)
    {
	return (valid_read_sanction(reader, wname) ||
		valid_read_all_sanction(reader, dname));
    }
    else
    {
	return (valid_read_sanction(reader, dname) ||
		valid_read_all_sanction(reader, dname));
    }

    /* No show. */
    return 0;
}

#if 0
/*
 * Function name: valid_move
 * Description  : This function is called by the gamedriver to see whether
 *                it is possible to move an object to a destination.
 * Arguments    : object ob   - the object to move.
 *                object dest - the intended destination.
 * Arguments    : int 1/0 - allowed/disallowed.
 */
int
valid_move(object ob, object dest)
{
    return 1;
}

/*
 * Function name: valid_resident
 * Description  : This function is called to see whether the object is allowed
 *                to set the pragma 'resident'. At this point no object is
 *                allowed to do so. That is safest.
 * Arguments    : object ob - the object to test.
 * Returns      : int 1/0 - allowed/disallowed.
 */
int
valid_resident(object ob)
{
    return 0;
}
#endif

/*
 * Function name: valid_debug
 * Description  : This function is called to see whether the object is allowed
 *                to call the efun debug() if the object is anything but this
 *                object SECURITY. Since we don't want other objects to call
 *                debug other than via SECURITY->do_debug(), we disallow it
 *                for all.
 * Arguments    : object ob - the object calling valid_debug
 *                string cmd - the debug command.
 *                mixed arg1 - the argument 1 to debug.
 *                mixed arg2 - the argument 2 to debug.
 *                mixed arg3 - the argument 3 to debug.
 * Returns      : int 1/0 - allowed/ disallowed.
 */
varargs int
valid_debug(object ob, string cmd, mixed arg1, mixed arg2, mixed arg3)
{
    return 0;
}

/*
 * Function name: valid_query_ip_ident
 * Description  : This function is called to check whether the object is
 *                allowed to call the efun query_ip_ident() on a particular
 *                target.
 * Arguments    : object actor  - the object that wants to call the efun.
 *                object target - the object the actor wants to know about.
 * Returns      : int 1/0 - allowed/ disallowed.
 */
varargs int
valid_query_ip_ident(object actor, object target)
{
    string euid = geteuid(actor);
    
    /* Only archwizard and keepers may call this efun. */
    return ((query_wiz_rank(euid) >= WIZ_ARCH) || (euid == "root"));
}

/*
 * Function name: valid_query_ip
 * Description  : This function is called to check whether the actor is
 *                allowed to call the efun query_ip_number() or _name() on
 *                a particular target.
 * Arguments    : mixed actor   - the actor that wants to call the efun.
 *                object target - the object the actor wants to know about.
 * Returns      : int 1/0 - allowed/ disallowed.
 */
varargs int
valid_query_ip(mixed actor, object target)
{
    return 1;
#if 0
    if (objectp(actor))
    {
        actor = geteuid(actor);
    }

    /* Lords and arches can see ip-number. */
    if (query_wiz_rank(actor) >= WIZ_LORD)
    {
        return 1;
    }

    /* Members of the domain arch and player arch team, too. */
    if (query_admin_team_member(actor, "aod") ||
        query_admin_team_member(actor, "aop"))
    {
        return 1;
    }

    return 0;
#endif
}

/*
 * Function name: check_snoop_validity
 * Description  : Do the actual validity checking.
 * Arguments    : object snooper - the prospective snooper.
 *		  object snopee  - the prospective snoopee.
 *		  int sanction - consider sanctioning or not.
 * Returns      : int 1/0 - allowed/disallowed.
 */
static int
check_snoop_validity(object snooper, object snoopee, int sanction)
{
    int by_type;
    int on_type;
    string by_name;
    string on_name;

    by_name = geteuid(snooper);
    by_type = query_wiz_rank(by_name);
    on_name = geteuid(snoopee);
    on_type = query_wiz_rank(on_name);
 
    /* Only wizards can snoop. */
    if (by_type == WIZ_MORTAL)
    {
	return 0;
    }

    /* Lords and stewards can snoop members everywhere, if the snoopee has a
     * level lower than their own and they can snoop apprentices too.
     */
    if (((by_type == WIZ_LORD) ||
	 (by_type == WIZ_STEWARD)) && 
	(((query_wiz_dom(on_name) == query_wiz_dom(by_name)) &&
	 (on_type < by_type)) ||
	 (on_type == WIZ_APPRENTICE)))
    {
	return 1;
    }

    /* Arch snoops all but arch. */
    if ((by_type >= WIZ_ARCH) &&
	(on_type < WIZ_ARCH))
    {
	return 1;
    }

    /* Mortals are safe in sanctuary for all but arch++. */
    if (environment(snoopee) &&
	(environment(snoopee)->query_prevent_snoop()))
    {
	return 0;
    }

    /* Ordinary wizzes snoops all mortals. */
    if (on_type == WIZ_MORTAL)
    {
	return 1;
    }

    /* A wizard can sanction another wizard to snoop him. */
    if (sanction)
    {
	return valid_snoop_sanction(by_name, on_name);
    }

    return 0;
}

/* This macro will log the snoop-action when the actor is not a member of
 * the administration.
 */
#ifdef LOG_SNOOP
#define ACTION_SNOOP(str) \
if (query_wiz_rank(caller_name) < WIZ_ARCH) \
{ this_object()->log_syslog(LOG_SNOOP, ctime(time()) + " " + (str)); }
#endif LOG_SNOOP

/*
 * Function name: valid_snoop
 * Description  : Checks if a user has the right to snoop another user.
 * Arguments    : object initiator - the actor for the command.
 *                object snooper   - the prospective snooper.
 *		  object snopee    - the prospective snoopee.
 * Returns      : int 1/0 - allowed/disallowed.
 */
public int
valid_snoop(object initiator, object snooper, object snoopee)
{
    string caller_name;
    string actor_name;
    string target_name;
    int result;

    caller_name = geteuid(initiator);
    actor_name  = geteuid(snooper);

    /* Break snoop case. Valid if the breaker has the right to snoop the
     * person currently doing the snooping and naturally people can break
     * their own snoop as well. Do not consider sanctioning in this case.
     */
    if (!snoopee)
    {
	if ((caller_name != actor_name) &&
	    !check_snoop_validity(initiator, snooper, 0))
	{
	    return 0;
	}

#ifdef LOG_SNOOP
	ACTION_SNOOP(sprintf(" %-11s snoop broken by %s\n",
			     capitalize(snooper->query_real_name()),
			     capitalize(caller_name)));
#endif LOG_SNOOP
	return 1;
    }

    /* Prevent accidental breaking of snoop. If the target is already snooped
     * it is not possible to set a new snoop on that target.
     */
    if (efun::query_snoop(snoopee))
    {
	return 0;
    }

    /* Set up snoop case. This way player A forces B to snoop C. This is
     * only valid for archwizards++. We do not consider sanctioning in
     * this case.
     */
    target_name = geteuid(snoopee);
    if (caller_name != actor_name)
    {
	if ((query_wiz_rank(caller_name) < WIZ_ARCH) ||
	    !check_snoop_validity(initiator, snoopee, 0))
	{
	    return 0;
	}

#ifdef LOG_SNOOP
	ACTION_SNOOP(sprintf(" %-11s snoops %-11s forced by %s\n",
			     capitalize(snooper->query_real_name()),
			     capitalize(snoopee->query_real_name()),
			     capitalize(caller_name)));
#endif LOG_SNOOP
	return 1;
    }
    
    /* Ordinary snoop case. A wants to snoop B. Valid if snooper can snoop
     * snoopee. Consider sanctioning in this case.
     */
    if (check_snoop_validity(snooper, snoopee, 1))
    {
#ifdef LOG_SNOOP
	ACTION_SNOOP(sprintf(" %-11s snoops %s\n",
			     capitalize(snooper->query_real_name()),
			     capitalize(snoopee->query_real_name())));
        if (query_wiz_rank(caller_name) < WIZ_ARCH)
        {
            this_object()->log_syslog(LOG_SNOOP, "    " +
                RPATH(file_name(environment(snooper))) + " snoops " +
                RPATH(file_name(environment(snoopee))) + "\n");
        }

#endif LOG_SNOOP
	return 1;
    }

    return 0;
}

/*
 * Function name: creator_file
 * Description  : Gives the name of the creator of a file. This is a
 *                direct function of the file_name(). The creator must be
 *                a domain or wizard name, the root or backbone euid or
 *                it can be 0 if it is invalid.
 * Arguments    : string str - the path to process.
 * Returns      : string - the name of the creator.
 */
string 
creator_file(string str)
{
    string *parts;

    /* Get the parts of the name. */
    parts = explode(str, "/") - ({ "" });

    /* The file is probably in a domain directory. */
    if (parts[0] == "d")
    {
	/* The file is owned by an active wizard. */
	if ((sizeof(parts) > 2) &&
	    (query_wiz_dom(parts[2]) == parts[1]))
	{
	    return parts[2];
	}

	/* The file is owned by an active domain. */
	if (query_domain_number(parts[1]) > -1)
	{
	    return parts[1];
	}
    }

    /* Everything in /secure gets return root uid. */
    if (parts[0] == "secure")
    {
	return ROOT_UID;
    }

    /* No cloning or loading from or /open. */
    if (parts[0] == "open")
    {
	return 0;
    }

    /* All else: return backbone uid. */
    return BACKBONE_UID;
}

string
modify_path(string path, object ob)
{
    return path;
}

#if 0
string
valid_compile_path(string path, string filename, string fun)
{
    return path;
}
#endif

/*
 * Convert a possibly relative path to an absolute path. We can assume
 * that there is a this_player(). This is called from within the editor.
 */
string 
make_path_absolute(string path)
{
    return FTPATH(this_player()->query_path(), path);
}

/*
 * Function name: load_domain_link
 * Description:   Try to load a domain_link file
 * Arguments:     file - The domain_link to load
 * Returns:       True if everything went ok, false otherwise
 */
static int
load_domain_link(string file)
{
    string err, creator;

    if (extract(file, -2) == ".c")
    {
	file = extract(file, 0, -3);
    }

    if (file_size(file + ".c") == -1)
    {
        return 0;
    }

    creator = creator_file(file);
    set_auth(this_object(), "root:" + creator);

    if (err = (string)LOAD_ERR(file))
    {
        write("\tCan not load: " + file + ":\n     " + err + "\n");
        return 0;
    }

    write("\t" + file + ".c (" + creator + ")\n");

    catch(file->preload_link());

    return 1;
}

/*
 * Function name: 	start_boot()
 * Description:		Loads master data, including list of all domains and
 *			wizards. Then make a list of preload stuff
 * Arguments:		load_empty: If true start_boot() does no preloading
 * Return:		List of files to preload
 */
static string *
start_boot(int load_empty)
{
    string *prefiles, *links;
    object simf;
    mixed test;
    int size, ix;

    if (game_started)
	return 0;

    set_auth(this_object(), "root:root");

    /* Fix the userids of the simul_efun object */
    if (objectp(simf = find_object(SIMUL_EFUN)))
    {
	set_auth(simf, "root:root");
    }

    write("Retrieving master data.\n");
    if (!restore_object(SAVEFILE))
    {
	write(SAVEFILE + " nonexistant. Using defaults.\n");
	load_fob_defaults();
	load_guild_defaults();
	reset_graph();
        runlevel = WIZ_MORTAL;
    }

    if (load_empty) 
    {
	write("Not preloading.\n");
	return 0;
    }

    /* Make list of preload stuff: Domain and wizard */
#ifdef PRELOAD_FIRST
    if (stringp(PRELOAD_FIRST))
    {
	prefiles = explode(read_file(PRELOAD_FIRST), "\n");
    }
    else if (pointerp(PRELOAD_FIRST))
    {
	prefiles = PRELOAD_FIRST + ({});
    }
#endif PRELOAD_FIRST

    write("Loading and setting up domain links:\n");
    links = filter(query_domain_links() + query_mage_links(),
               load_domain_link);

    size = sizeof(links);
    ix = -1;

    while (++ix < size)
        prefiles = prefiles + links[ix]->query_preload();

    return prefiles;
}

static void
preload_boot(string file)
{
    string err, creator;

    if (file_size(file + ".c") == -1)
    {
	return;
    }

    creator = creator_file(file);
    set_auth(this_object(), "root:" + creator);

    if (err = (string)LOAD_ERR(file))
    {
	write("\tCan not load: " + file + ":\n     " + err + "\n");
    }
    else
    {
        write("\tPreloading: " + file + ".c  (" + creator + ")\n");
        if (strlen(file = catch(file->teleledningsanka())))
        {
            write("\tError: " + file + ".c\n");
        }
    }
}

/*
 * Function name: final_boot
 * Description  : This function will be called from the gamedriver when the
 *                game is started, after start_boot() and preload_boot() are
 *                called. Note that this function is not called when the
 *                master is updated.
 */
static void
final_boot()
{
    int theport;

    game_started = 1;
    theport = debug("mud_port");
    if (theport)
    {
	set_auth(this_object(), "root:root");
	write_file((GAME_START + "." + theport), ctime(time()) + "\n");
    }

    debug("set_swap",
	  ({
	      SWAP_MEM_MIN,
	      SWAP_MEM_MAX,
	      SWAP_TIME_MIN,
	      SWAP_TIME_MAX
	      }) );

    /* Tell the graph we rebooted. */
    mark_graph_reboot();

#ifdef UDP_MANAGER
    udp_manager = UDP_MANAGER;
    if (catch(udp_manager->teleledningsanka()))
	udp_manager = 0;

    if (stringp(udp_manager))
	udp_manager->send_startup_udp(MUDLIST_SERVER[0], MUDLIST_SERVER[1]);
    else
#endif UDP_MANAGER
	debug("send_udp", MUDLIST_SERVER[0], MUDLIST_SERVER[1],
	      "@@@" + UDP_STARTUP + this_object()->startup_udp() + "@@@\n");
}

/*
 * Function name: start_shutdown
 * Description  : This function is called by the gamedriver to get a list
 *                of all interactive objects that need to be disconnected
 *                from the game when it is shutting down.
 * Returns      : object * - the interactive users in the game.
 */
object *
start_shutdown()
{
    return users();
}

/*
 * Function name: cleanup_shutdown
 * Description  : This function is called for each interactive object when
 *                the game is shutting down. It makes all those players
 *                quit the game.
 * Arguments    : object ob - the object to force to quit.
 */
static void
cleanup_shutdown(object ob)
{
    set_this_player(ob);
    ob->quit();
}

/*
 * Function name: final_shutdown
 * Description  : When all mortals are kicked out of the game, this function
 *                is called last by the gamedriver before the game closes.
 */
static void
final_shutdown()
{
    string *links;
    int index;
    int size;

    if (stringp(udp_manager))
    {
	udp_manager->send_shutdown_udp(MUDLIST_SERVER[0], MUDLIST_SERVER[1]);
	udp_manager->update_masters_list();
    }

    /* Process the graph data even if this isn't the top of the hour. */
    graph_process_data();

    /* Save the master. */
    save_master();

    /* Call the domain links. */
    links = query_domain_links();
    size = sizeof(links);
    index = -1;
    while(++index < size)
    {
	if (file_size(links[index] + ".c") != -1)
	{
	    catch(links[index]->armageddon());
	}
    }
}

/*
 * Function name: log_error
 * Description  : This function is called from the game driver if there is
 *                an error while compiling an object.
 * Arguments    : string path  - the path of the object having the error.
 *                string error - the error message.
 */
static void 
log_error(string path, string error)
{
    set_auth(this_object(), "root:root");

    path = query_wiz_path(creator_file(path)) + "/log";
    if (file_size(path) != -2) 
    {
	mkdir(path);
    }

    path += "/errors";
    if (file_size(path) == -1)
    {
	write_file(path, "*\n* This is your error log file\n*\n\n");
    }
    write_file(path, error);
}

/*
 * This function is called from GD when rooms are destructed so that master
 * can move players to safety.
 */
void
destruct_environment_of(object ob)
{
    if (environment(ob))
	catch(ob->move(environment(ob)));

    if (!query_ip_number(ob))
	return;
    ob->move_living("X", ob->query_default_start_location());
}

/*
 * Function name: define_include_dirs
 * Description  : Define  where the '#include' statement is supposed to
 *                search for files. "." will automatically be searched
 *                first, followed in order as given below. The path should
 *                contain a '%s', which will be replaced by the file
 *                searched for.
 * Returns      : string * - the array of path to search.
 */
string *
define_include_dirs()
{
    return ({ "/sys/%s" });
}

/*
 * Function name: get_ed_buffer_save_file_name
 * Description  : When a wizard is in ed and goes linkdead, this function
 *                is called to get the name of the save-file. If the
 *                wizard does not have the necessary rights, the function
 *                will disallow save.
 * Arguments    : string - the name of the file to save
 * Returns      : string - the name of the secured save-file
 *                0      - the player has no write-rights.
 */
string
get_ed_buffer_save_file_name(string file) 
{
    string *path;

    if (!objectp(this_player()))
	return 0;

    if (!valid_write(file, this_player(), "ED"))
	return 0;

    path = explode(file, "/");
    path[sizeof(path) - 1] = "dead_ed_" + path[sizeof(path) - 1];

    return implode(path, "/");
}

/* save_ed_setup and restore_ed_setup are called by the ed to maintain
   individual options settings. These functions are located in the master
   object so that the local gods can decide what strategy they want to use.

/*
 * The wizard object 'who' wants to save his ed setup. It is saved in the
 * file ~wiz_name/.edrc . A test should be added to make sure it is
 * a call from a wizard.
 *
 * Don't care to prevent unauthorized access of this file. Only make sure
 * that a number is given as argument.
 */
int 
save_ed_setup(object who, int code)
{
    string file;

    if (!intp(code))
	return 0;
    file = query_wiz_path((string)who->query_real_name()) + "/.edrc";
    rm(file);
    return write_file(file, code + "");
}

/*
 * Retrieve the ed setup. No meaning to defend this file read from
 * unauthorized access.
 */
int 
retrieve_ed_setup(object who)
{
    string file;
    int code;

    file = query_wiz_path(who->query_real_name()) + "/.edrc";
    if (file_size(file) <= 0)
	return 0;
    sscanf(read_file(file), "%d", code);
    return code;
}

/*
 * Function name: query_allow_shadow
 * Description  : This function is called from the game driver to find out
 *                whether it is allowed to shadow a particular object. The
 *                object that wants to shadow is previous_object(). To
 *                prevent shadowing, the target object will have to define
 *                the function query_prevent_shadow() to return 1.
 * Arguments    : object target - the object targeted for shadowing.
 * Returns      : int 1/0 - allowed/disallowed.
 */
int 
query_allow_shadow(object target)
{
    return !(target->query_prevent_shadow(previous_object()));
}

/*
 * Function name:   valid_exec
 * Description:     Checks if a certain 'program' has the right to use exec()
 * Arguments:       name: Name of the 'program' that attempts to use exec()
 *                        Note that this is different from file_name(),
 *                        The program name is what calling_program returns.
 *                  to:   destination of socket
 *                  from: target of the socket
 * Returns:         True if exec() is allowed.
 */
int
valid_exec(string name, object to, object from)
{
    name = "/" + name;
    if ((name == (LOGIN_OBJECT + ".c")) || 
	(name == (POSSESSION_OBJECT + ".c")) ||
	(name == (LOGIN_NEW_PLAYER + ".c")) ||
	(name == (WIZ_CMD_ARCH + ".c")))
    {
	return 1;
    }

    return 0;
}

/*
 * Function name: simul_efun_reload
 * Description  : This function sets the authorisation variables for the
 *                simul_efun object.
 */
void 
simul_efun_reload()
{
    set_auth(find_object(SIMUL_EFUN), "root:root");
}

/*
 * Function name: loaded_object
 * Description  : This function is called when an object is loaded into
 *                memory by another object. It tests whether it was valid
 *                to load the object and sets the authorisation variables
 *                in the loaded object. If the load was not valid, throw()
 *                will terminate the execution.
 * Arguments    : object lob - the loading object.
 *                object ob  - the loaded object.
 */
void
loaded_object(object lob, object ob)
{
    string creator = creator_object(ob); 
    string *auth = explode(query_auth(lob), ":");

    if (!strlen(creator))
    {
	do_debug("destroy", ob);
	throw("Loading a bad object from: " + file_name(lob) + ".\n");
	return;
    }

    if (auth[1] == "0")
    {
	creator = file_name(ob);
	do_debug("destroy", ob);
	throw("Unauthorized load: " + creator + " by: " +
	    file_name(lob) + ".\n");
	return;
    }

    if ((creator == BACKBONE_UID) ||
	(creator == auth[0]))
    {
	set_auth(ob, (auth[1] + ":" + auth[1]));
	return;
    }

    set_auth(ob, (creator + ":0"));
}

/*
 * Function name: cloned_object
 * Description  : This function is called when an object is cloned. It
 *                tests whether the clone was valid. It also sets the
 *                authorisation variable in the cloned object. If the
 *                clone was not valid, throw() will terminate the
 *                execution.
 * Arguments    : object cob - the cloning object.
 *                object ob  - the cloned object.
 */
void
cloned_object(object cob, object ob)
{
    string creator = creator_object(ob); 
    string *auth = explode(query_auth(cob), ":");

    if (!strlen(creator))
    {
	creator = file_name(ob);
	do_debug("destroy", ob);
	throw("Unauthorized clone: " + creator + " by: " +
	    file_name(cob) + ".\n");
	return;
    }

    if (auth[1] == "0")
    {
	creator = file_name(ob);
	do_debug("destroy", ob);
	throw("Cloning without privilege: " + creator + " by: " +
	    file_name(cob) + ".\n");
	return;
    }

    if ((creator == BACKBONE_UID) ||
	(creator == auth[0]))
    {
	set_auth(ob, (auth[1] + ":" + auth[1]));
	return;
    }

    set_auth(ob, (creator + ":0"));
}

/*
 * Function name: modify_command
 * Description  : Modify a command given by a certain living object. This can
 *                be used for many quicktyper-like functions. There are also
 *                some master.c defined substitutions. Commands that start
 *                with a dollar ($) are not substututed.
 * Arguments    : string cmd - the command to modify.
 *                object ob - the object for which to modify the command.
 * Returns      : string - the modified command to execute.
 */
string
modify_command(string cmd, object ob)
{
    string str;
    string domain;
    int no_subst;

    while(wildmatch("$*", cmd))
    {
        cmd = extract(cmd, 1);
        no_subst = 1;
    }

    while(wildmatch(" *", cmd))
    {
        cmd = extract(cmd, 1);
    }

    if (strlen(str = command_substitute[cmd]))
    {
        if (query_ip_number(ob) &&
            !ob->query_wiz_level() &&
            pointerp(m_domains[domain = environment(ob)->query_domain()]))
        {
            m_domains[domain][FOB_DOM_CMNDS]++;
        }
        return str;
    }

    /* We can not allow any handwritten VBFC */
    if (wildmatch("*@@*", cmd))
    {
        cmd = implode(explode(cmd, "@@"), "#");
    }

    /* No modification for NPC's */
    if (!query_ip_number(ob))
    {
        return cmd;
    }

    /* Count commands for ranking list */
    if (environment(ob) &&
        !ob->query_wiz_level() &&
        pointerp(m_domains[domain = environment(ob)->query_domain()]))
    {
        m_domains[domain][FOB_DOM_CMNDS]++;
    }

    /* No modification if it starts with a "$". */
    if (no_subst)
    {
        return cmd;
    }

    return ob->modify_command(readable_string(cmd));
}

/*
 * Function name: query_memory_percentage
 * Description  : This function will return the percentage of memory usage
 *                of the game so far. When the counter reaches 100, it is
 *                time to reboot.
 * Returns      : int - the relative memory usage.
 */
nomask public int
query_memory_percentage()
{
    int    f;
    int    cval;
    string mc;
    string foobar;

    mc = debug("malloc");
    sscanf(mc, "%ssbrk requests: %d %d (a) %s", foobar, f, cval, foobar);

    return (cval / (memory_limit / 100));
}

/*
 * Function name:   memory_failure
 * Description:     This function is called when the gamedriver considers
 *                  itself in trouble and need the game shut down in a
 *		    graceful manner. This function _must_ be called
 *                  via a call_other. It may only be called by root itself
 *                  or by a member of the administration.
 */
static void
memory_failure()
{
    if (!mem_fail_flag)
    {
	mem_fail_flag = 1;

	set_auth(this_object(), "root:root");
	ARMAGEDDON->start_shutdown("Caly swiat zostal opanowany przez "
	                         + "Ciemnosc.", 10, ROOT_UID);
    }
}

/*
 * Function name:   memory_reconfigure
 * Description:	    This function is called when the gamedriver receives
 *		    an external signal, denoting that the memory status
 *		    has changed.
 * Arguments:       mem: Memory size, 0 small, 1 large.
 */
static void
memory_reconfigure(int mem)
{
    string mess = "a different";
    object *list;
    int    index;
    int    size;

#ifdef LARGE_MEMORY_LIMIT
    if (((mem == 0) &&
	 (memory_limit == SMALL_MEMORY_LIMIT)) ||
	((mem == 1) &&
	 (memory_limit == LARGE_MEMORY_LIMIT)))
    {
	return;
    }

    if (mem == 0)
    {
	memory_limit = SMALL_MEMORY_LIMIT;
	mess = "small";
    }
    else
    {
	memory_limit = LARGE_MEMORY_LIMIT;
	mess = "large";
    }
    check_memory(0);
#endif LARGE_MEMORY_LIMIT

    list = filter(users(), &->query_wiz_level());
    size = sizeof(list);
    index = -1;
    while(++index < size)
    {
	tell_object(list[index], "@ Armageddon: I have switched to " + mess +
	    " memory mode.\n");
    }
}

/*
 * This function is called if the driver gets sent a signal that it catches.
 */
static void
external_signal(string sig_name)
{
    write("Received " + sig_name + " signal.\n");
    switch (sig_name)
    {
    case "INT":
	ARMAGEDDON->start_shutdown("Someone halted the game!", 0, ROOT_UID);
	break;
    case "HUP":
    case "KILL":
    case "QUIT":
    case "TERM":
        debug("shutdown");
        break;
    case "USR1":
	memory_reconfigure(0);
	break;
    case "USR2":
	memory_reconfigure(1);
	break;
    case "TSTP":
    case "CONT":
	break;
    case "UNKNOWN":
    default:
	write("Unknown signal received!\n");
    }
}

/*
 * Function name: query_memory_limit
 * Description:   This function returns the current memory limit.
 */
public int
query_memory_limit()
{
    return memory_limit;
}

/*
 * Function name: query_memory_failure
 * Description:   This function returns 1 if memory failure is detected.
 */
public int
query_memory_failure()
{
    return mem_fail_flag;
}

/*
 * Function name: log_incoming_service
 * Description  : This function will make a log of the incomming service.
 * Arguments    : string request - the request to log.
 *                string wname   - the wizard name.
 *                string path    - the path to log.
 */
#ifdef LOG_FTP
static string
log_incoming_service(string request, string wname, string path)
{
    string *parts;
    string fname;

    parts = explode(path, "/") - ({ "" });
    wname = lower_case(wname);

    if ((sizeof(parts) > 1) && (query_domain_number(parts[1]) >= 0))
	fname = parts[1];
        
    this_object()->log_syslog(("ftplog/" + (stringp(fname) ? fname : "Lib")),
        sprintf("%s %-5s %-11s %s\n", ctime(time()), request, wname, path),
			      5000000);
    this_object()->log_syslog(("ftplog/wizards/" + extract(wname, 0, 0) +
        "/" + wname), sprintf("%s %-5s %s\n", ctime(time()), request, path),
        1000000);
}
#endif LOG_FTP
    
/*
 * Function name: incoming_service
 * Description  : Handle incoming request from other programs. This function
 *                may only be called from the gamedriver.
 * Arguments    : string request - the request.
 * Returns      : string - the answer to the request.
 */
static string
incoming_service(string request)
{
    string *tmp;
    string str;
    string path;
    object ob;

    /* There must be a request, or we cannot answer it ;-) */
    if (!strlen(request))
    {
	return "ERROR Bad request\n";
    }
    

    /* The request may be separated by three different characters, \n,
     * \r or the space.
     */
    tmp = explode(request, "\n");
    if (sizeof(tmp))
    {
	request = tmp[0];
    }
    tmp = explode(request, "\r");
    if (sizeof(tmp))
    {
	request = tmp[0];
    }
    tmp = explode(request, " ");

    /* Switch on the request command. */
    switch (lower_case(tmp[0]))
    {
    case "user":
	if (sizeof(tmp) != 2)
	{
	    return "ERROR Wrong number of parameters\n";
	}

	tmp[1] = lower_case(tmp[1]);
	ob = find_player(tmp[1]);
	if (objectp(ob))
	{
	    if (query_wiz_rank(tmp[1]) >= WIZ_APPRENTICE)
	    {
		path = query_wiz_path(tmp[1]);
#ifdef LOG_FTP
		log_incoming_service("AUTH", tmp[1], path);
#endif LOG_FTP
		return ob->query_password() + ":" +
		    query_wiz_level(tmp[1]) + ":" +
		    path + "\n";
	    }
	}
	else
	{
	    ob = finger_player(tmp[1]);
	    if (objectp(ob))
	    {
		path = query_wiz_path(tmp[1]);
		str = ob->query_password() + ":" +
		    query_wiz_level(tmp[1]) + ":" +
		    path + "\n";
		ob->remove_object();
		if (query_wiz_rank(tmp[1]) >= WIZ_APPRENTICE)
		{
#ifdef LOG_FTP
		    log_incoming_service("AUTH", tmp[1], path);
#endif LOG_FTP
		    return str;
		}
	    }

	    /* This is done to ensure that no one will ever log in with the
	     * name of a domain. The "*" as password can never be matched,
	     * though it allows to type "cd ~Domain".
	     */
	    if (query_domain_number(capitalize(tmp[1])) >= 0)
	    {
		return "*:0:/d/" + capitalize(tmp[1]) + "\n";
	    }
	}
	return "ERROR No such user\n";

    case "read":
	if (sizeof(tmp) != 3)
	{
	    return "ERROR Wrong number of parameters\n";
	}
	if (valid_read(tmp[2], lower_case(tmp[1]), "FTP"))
	{
#ifdef LOG_FTP
	    log_incoming_service("READ", tmp[1], tmp[2]);
#endif LOG_FTP
	    return "READ access\n";
	}
	return "ERROR No access\n";

    case "write":
	if (sizeof(tmp) != 3)
	{
	    return "ERROR Wrong number of parameters\n";
	}
	if (valid_write(tmp[2], lower_case(tmp[1]), "FTP"))
	{
#ifdef LOG_FTP
	    log_incoming_service("WRITE", tmp[1], tmp[2]);
#endif LOG_FTP
	    return "WRITE access\n";
	}
	return "ERROR No access\n";

    default:
	return "ERROR Unknown request\n";
    }
}

#if 0
/*
 * Function name:   valid_save_binary
 * Description:     This function is called when a file has ordered the GD
 *                  to save a binary image of the program. This might not
 *		    be allowed by any and every file so master is asked.
 * Arguments:       file: Filename of the object.
 */
int 
valid_save_binary(string filename)
{
    return 1;
}

/* ob wishing to inherit inherit_file */
int
valid_inherit(object ob, string inherit_file)
{
    return 1;
}

/* ob trying to load file */
int 
valid_load(object ob, string file)
{
    return 1;
}
#endif

/*
 * Function name:   master_reload
 * Description:     Called from GD after a reload of the master object
 */
void
master_reload()
{
}

void
recreate(object old_master)
{
    create();
    game_started = 0;
    start_boot(1); /* This does what we want */
    game_started = 1;
#ifdef UDP_MANAGER
    udp_manager = UDP_MANAGER;
#endif
}

/*
 * Function name:   incoming_udp
 * Description:     Called from GD if a udp message has been received. This
 *		    can only happen if CATCH_UDP_PORT has been defined in
 *		    the GD's config.h file.
 * Arguments:	    from_host: The IP number of the sending host
 *		    message:   The message sent.
 */
void
incoming_udp(string from_host, string message)
{
//    log_syslog("UDP_IO", sprintf("%15s < %s\n", from_host,
//               implode(explode(message, "\n"), "\\n")));

    if (stringp(udp_manager))
	udp_manager->incoming_udp(from_host, message);
    else
    {
	set_auth(this_object(), "#:root");
	log_file("LOST_UDP", "(" + from_host + ") " + message + "\n", -1);
    }
}

void
mark_quit(object player)
{
    string text;
    int index = 0;
    object prev;

    if ((query_idle(player) > 1200) ||
        (player->query_linkdead()) ||
        ((player == this_interactive()) &&
         (query_verb() == "zakoncz")))
    {
        return;
    }
    
    text = "Current time: " + ctime(time()) + "\nDestructed  : " +
        capitalize(player->query_real_name()) + ".\n";
    if (objectp(this_interactive()))
    {
        text += "Interactive : " +
            capitalize(this_interactive()->query_real_name()) + ".\n";
    }
    if (strlen(query_verb()))
    {
        text += "Queried verb: " + query_verb() + "\n";
    }
    
    while(objectp(prev = calling_object(index)))
    {
        text += "    call: " + file_name(prev) + "  " +
            calling_function(index) + "()";
        if (interactive(prev))
        {
            text += "  [" + capitalize(prev->query_real_name()) + "]";
        }
        text += "\n";
        
        if ((MASTER_OB(prev) + ".c") != ("/" + calling_program(index)))
        {
            text += "    file: /" + calling_program(index) + "\n";
        }
        
        index--;
    }
    
    set_auth(this_object(), "root:root");
    write_file("/syslog/log/DESTRUCTED", text + "\n");
}
 
/*
 * Function name: remove_interactive
 * Description  : Called from GD if a player logs out or goes linkdead. If
 *                the player quit the game, we don't do anything.
 * Arguments    : object ob    - the player that leaves the game.
 *                int linkdead - true if the player linkdied.
 */
static void 
remove_interactive(object ob, int linkdied)
{
    string master_ob;

    QUEUE->dequeue(ob);

    /* If someone who is logging in linkdies, we just dispose of it. Also,
     * people who are trying to create a character, will have to start
     * over again.
     */
    master_ob = MASTER_OB(ob);
    if ((master_ob == LOGIN_OBJECT) ||
	(master_ob == LOGIN_NEW_PLAYER) ||
	(master_ob == GAMEINFO_OBJECT))
    {
	ob->remove_object();
	return;
    }

    /* Player left the game. */
    if (!linkdied)
    {
	/* Notify the wizards of the fact that the player quit. */
	notify(ob, 1);
   // mark_quit(ob);
        return;
    }

    /* Notify the wizards of the linkdeath. */
    notify(ob, 2);

#ifdef STATUE_WHEN_LINKDEAD
    ob->linkdie();
#endif STATUE_WHEN_LINKDEAD
}

/*
 * Function name: gamedriver_message
 * Description  : This function may (only) be called by the gamedriver to
 *                give a message to all players if that is necessary.
 * Arguments    : string str - the message to tell the people
 */
static void
gamedriver_message(string str)
{
    users()->catch_tell(str);
}

/*
 * Function name: runtime_error
 * Description  : In case a runtime error occurs, we tell the message to
 *                the people who need to hear it.
 * Arguments    : string error   - the error message.
 *                object ob      - the object that has the error.
 *                string program - the program name of the error.
 *                string file    - the filename of the error.
 */
static void
runtime_error(string error, object ob, string prog, string file)
{
    string fmt_error, cr, path;
    
    if (this_interactive())
    {
	if (this_interactive()->query_wiz_level() ||
	    this_interactive()->query_prop(PLAYER_I_SEE_ERRORS))
	{
	    fmt_error =
		"\n\nRuntime error: " + error +
		"       Object: " + (ob ? file_name(ob) : "<???>") + 
		"\n      Program: " + prog +
		"\n         File: " + file + "\n\n";
	    this_interactive()->catch_tell(fmt_error);
	}
	else
	{
/*
	    this_interactive()->catch_tell("Your sensitive mind notices " +
		"a wrongness in the fabric of space.\n");
*/
	    this_interactive()->catch_tell("Wystapil powazny blad. " +
	        "Zglos go, opisujac okolicznosci w jakich do niego doszlo.\n");
	}
    }
    else
    {
	path = "";
	if (objectp(ob))
	{
	    cr = creator_object(ob);
	    path = query_wiz_path(cr);
	}
	path += "/log/runtime";
	fmt_error = ctime(time()) +
	    "\nRuntime error: " + error +
	    "       Object: " + (ob ? file_name(ob) : "<???>") + 
	    "\n      Program: " + prog +
	    "\n         File: " + file + "\n\n";
	
	write_file(path, fmt_error);
    }
}

/* 
 *    ----------------------------------------------------------------
 *    The code below this divisor is never called from the gamedriver.
 *    ----------------------------------------------------------------
 */

/*
 * Function name: remove_playerfile
 * Description:   This function moves a playerfile from /players/<?>/
 *                to /players/deleted/<?>/
 *                It also adds some text to the log DELETED.
 * Arguments:     player - The player to remove
 *                reason - The reason to why the file is removed
 * Returns:       True if everything went ok, false other wise
 */
public int
remove_playerfile(string player, string reason)
{
    string file, deleted;
    int number = 1;

    /* May only be called from the Arch soul. */
    if (!CALL_BY(WIZ_CMD_ARCH))
    {
	return 0;
    }

    file = PLAYER_FILE(player) + ".o";
    deleted = DELETED_FILE(player) + ".o";

    /* If there is a file, move it to the deleted dir. */
    if (file_size(file) < 0)
    {
	notify_fail("Brak pliku postaci: " + file + ".\n");
	return 0;
    }

    if (file_size(deleted) != -1)
    {
	while (file_size(deleted + "." + number) != -1)
	    number++;

	deleted += "." + number;
    }
    else
	number = 0;

    /* Move the file */
    if (!rename(file, deleted))
	return 0;

    /* Usuniecie poczty delikwenta */
    file = sprintf("%s%s/%s.o", MAIL_DIR, player[0..0], player);
    if (file_size(file) >= 0)
	rm(file);

    /* Usuniecie depozytow bankowych */
    BANK_CENTRAL->usun_depozyty(player, number);

    /* Log the action */
    log_file("DELETED", ctime(time()) + " " + capitalize(player) +
        " by " + capitalize(this_interactive()->query_real_name()) +
        " (" + reason + ")\n", -1);

    return 1;
}

/*
 * Function name: rename_playerfile
 * Description  : This renames a player.
 * Arguments    : string oldname - the old name of the player.
 *                string newname - the new name of the player.
 * Returns      : int 1/0 - success/failure.
 */
public int
rename_playerfile(string oldname, string newname)
{
    mapping playerfile;

    /* May only be called from the Arch soul. */
    if (!CALL_BY(WIZ_CMD_ARCH))
    {
        return 0;
    }

    /* Rename the playerfile itself. */
    if (!rename(PLAYER_FILE(oldname) + ".o",
        PLAYER_FILE(newname) + ".o"))
    {
        notify_fail("Renaming playerfile failed!\n");
        return 0;
    }

    /* The name should also be updated in the playerfile. */
    playerfile = restore_map(PLAYER_FILE(newname));
    playerfile["name"] = newname;
    save_map(playerfile, PLAYER_FILE(newname));
    write("Player " + capitalize(oldname) + " succesfully renamed to " +
        capitalize(newname) + ".\n");

    /* Rename the mail folder if there is one. */
    if (file_size(FILE_NAME_MAIL(oldname) + ".o") > 0)
    {
        if (rename(FILE_NAME_MAIL(oldname) + ".o",
            FILE_NAME_MAIL(newname) + ".o"))
        {
            write("Mail folder found and renamed.\n");
        }
        else
        {
            write("Mail folder found, but renaming failed.\n");
        }
    }

    /* Update a wizard. */
    if (query_wiz_rank(oldname))
    {
        rename_wizard(oldname, newname);
    }

    log_file("DELETED", ctime(time()) + " " + capitalize(oldname) +
        " -> " + capitalize(newname) + ", renamed by " +
        capitalize(this_interactive()->query_real_name()) + ".\n", -1);
    return 1;
}

/*
 * Function name: proper_password
 * Description  : This function can be used to check whether a certain
 *                password is less likely to be broken using a general
 *                cracker. Therefore the following is checked:
 *                - the password must at least be 6 characters long;
 *                - the password must contain at least one 'non-letter';
 *                - this letter may not be the first or the last letter;
 * Arguments    : string str - the password to check.
 * Returns      : int 1/0 - proper/bad.
 */
int
proper_password(string str)
{
    int index = -1;
    int size;
    int normal;
    int stage = 0;

    /* Length of the password must be at least 6 characters. */
    size = strlen(str);
    if (size < 6)
    {
	return 0;
    }

    /* This may seem a little strange, but it actually is quite simple. As
     * stated before, there should at least be one non-letter and that
     * should be between normal letters. Therefore, starting at stage 0, we
     * wait until we find a letter. Have we found that, more to
     * stage 1 and wait for a non-letter. Then, at stage 2 we again wait for
     * a normal letter. If all checks out, we should be at stage 3 at the
     * end.
     */
    str = lower_case(str);
    while(++index < size)
    {
	normal = ((str[index] >= 'a') && (str[index] <= 'z'));

	switch(stage)
	{
	case 0:
	    stage = (normal ? 1 : 0);
	    break;

	case 1:
	    stage = (normal ? 1 : 2);
	    break;

	case 2:
	    stage = (normal ? 3 : 2);
	    break;
	}
    }

    /* If the stage isn't 3, the password isn't good. */
    return (stage == 3);
}

/*
 * Function name: generate_password
 * Description  : This function will generate a six character password that
 *                consists of letters, numbers and or special characters.
 *                Passwords generated with this function are considered to
 *                be safe enough against cracking programs.
 * Returns      : string - the password.
 */
public string
generate_password()
{
    string tmp = "";
    int    size = 8;
    int    index;

    while(--size >= 0)
    {
	switch(random(5))
	{
	case 0:
	    /* With 20% change, add a single digit */
	    tmp += ("" + random(10));
	    break;

	case 1:
	    /* With 20% chance, add a "special" character */
	    index = random(24);
	    tmp += "@?%^&*()[]{};:<>,.-_=+"[index..index];
	    break;

	case 2..4:
	default:
	     /* With 60% chance, add a letter, capitalized with 50% chance */
	    index = random(26);
	    tmp += (random(2) ? ALPHABET[index..index] :
		capitalize(ALPHABET[index..index]));
	    break;
	}
    }

    return tmp;
}

/*
 * Function name: remote_setuid
 * Description  : With this function the COMMAND_DRIVER can request that
 *                its authorisation information is reset in order to allow
 *                for a new uid/euid when another player uses the same soul.
 */
void
remote_setuid()
{
    if (function_exists("open_soul", previous_object()) ==
	COMMAND_DRIVER)
    {
	set_auth(previous_object(), "0:0");
    }
}

/*
 * Function name: set_helper_soul_euid
 * Description  : The helper soul is intended for wizards to get some commands
 *                for abilities they would not normally enjoy. Thus, the soul
 *                requires a higher access level than normally. Make sure that
 *                only the right object qualifies, though.
 */
public void
set_helper_soul_euid()
{
    if (file_name(previous_object()) == WIZ_CMD_HELPER)
    {
        set_auth(previous_object(), "root:root");
    }
}

/*
 * Function name: creator_object
 * Description  : Gives the name of the creator of an object. This is a
 *                direct function of the file_name() of the object.
 * Arguments    : object obj - the object to get the creator from.
 * Returns      : string - the name of the domain or wizard who created
 *                         the object.
 */
string
creator_object(object obj)
{
    if (!objectp(obj))
    {
	return 0;
    }

    return creator_file(file_name(obj));
}

/*
 * Function name:   domain_object
 * Description:     Gives the name of the domain of an object. This is a
 *                  direct function of the file_name() of the object.
 * Returns:         Name of the domain
 */
string
domain_object(object obj)
{
    string str,dom,wiz,name;

    if (!obj) return 0;
    
    str = file_name(obj);
    if (str[1] == 'd')
	sscanf(str,"/d/%s/%s/%s",dom,wiz,name);
    else
	dom = 0;
    return dom;
}

/*
 * Function name: load_player
 * Descripton:    This function is called from /std/player_sec 
 *		  when the player object is loaded initially. 
 *                It sets the euid of the player to root for 
 *                the duration of the load.
 */
int
load_player()
{
    int res;
    object pobj;

    pobj = previous_object();
    
    if (function_exists("load_player", pobj) != PLAYER_SEC_OBJECT ||
	!LOGIN_NEW_PLAYER->legal_player(pobj))
	return 0;
    else
    {
	set_auth(this_object(), "#:root");
	export_uid(pobj);
	res = (int)pobj->load_player(pobj->query_real_name());
	set_auth(this_object(), "#:" + (pobj->query_wiz_level() ?
			  pobj->query_real_name() : BACKBONE_UID));
	export_uid(pobj);
	set_auth(this_object(), "#:root");
	return res;
    }
}

/*
 * Function name:   save_player
 * Description:     Saves a player object.
 */
int
save_player()
{
    int res;
    object pobj;

    pobj = previous_object();
    
    if ((function_exists("save_player", pobj) != PLAYER_SEC_OBJECT) ||
	!LOGIN_NEW_PLAYER->legal_player(pobj))
    {
	return 0;
    }
    else
    {
	set_auth(this_object(), "#:backbone");
        pobj->fix_saveprop_list();
	set_auth(this_object(), "#:root");
	export_uid(pobj);
	res = (int)pobj->save_player(pobj->query_real_name());
	pobj->open_player();
	set_auth(this_object(), "#:" + (pobj->query_wiz_level() ?
					pobj->query_real_name() :
					BACKBONE_UID));
	export_uid(pobj);
	set_auth(this_object(), "#:root");
	return res;
    }
}

int
rem_def_start_loc(string str)
{
    if (!def_locations)
        def_locations = STARTING_PLACES;

    if (query_wiz_rank(this_interactive()->query_real_name()) < WIZ_ARCH)
    {
        write("Only arches or keepers may remove starting locations.\n");
        return 1;
    }

    set_auth(this_object(), "#:root");
    while (member_array(str, def_locations) >= 0) /* Delete  */
        def_locations = def_locations - ({ str });

    save_master();
}

int
add_def_start_loc(string str)
{
    if (!def_locations)
	def_locations = STARTING_PLACES;
    
    if (query_wiz_rank(this_interactive()->query_real_name()) < WIZ_ARCH)
    {
	write("Only arches or keepers may add starting locations.\n");
	return 1;
    }
    
    if (file_size(str + ".c") < 0)
    {
	write("No such file: " + str + "\n");
	return 1;
    }
    while (member_array(str, def_locations) >= 0) /* Delete copies */
	def_locations = def_locations - ({ str });

    def_locations = def_locations + ({ str });
    save_master();
}

int
rem_temp_start_loc(string str)
{
    if (!temp_locations)
        temp_locations = TEMP_STARTING_PLACES;

    if (query_wiz_rank(this_interactive()->query_real_name()) < WIZ_ARCH)
    {
        write("Only arches or keepers may remove starting locations.\n");
        return 1;
    }
    while (member_array(str, temp_locations) >= 0) /* Delete copies */
        temp_locations = temp_locations - ({ str });

    save_master();
}

int
add_temp_start_loc(string str)
{
    if (!CALL_BY(WIZ_CMD_ARCH))
    {
	write("Only arches or keepers may add starting locations.\n");
	return 1;
    }
    if (file_size(str + ".c") <= 0)
    {
	write("No such file: " + str + "\n");
	return 1;
    }
    if (!sizeof(temp_locations))
    {
	temp_locations = TEMP_STARTING_PLACES;
    }
    
    temp_locations = (temp_locations - ({ str }) ) + ({ str });
    save_master();
}

public string *
query_list_def_start()
{
    return secure_var(def_locations);
}

public string *
query_list_temp_start()
{
    return secure_var(temp_locations);
}

int
check_temp_start_loc(string str)
{
    return member_array(str, temp_locations);
}

int
check_def_start_loc(string str)
{
    if (!def_locations)
	def_locations = STARTING_PLACES;
    return member_array(str, def_locations);
}

public varargs void
log_syslog(string file, string text, int length = 0)
{
    string fname;

    fname = calling_program();

    if ((fname[0..5] != "secure") &&
	(fname[0..3] != "cmd/") &&
	(fname[0..3] != "std/") &&
	(fname[0..15] != "d/Standard/login/") &&
	(fname != "d/Standard/obj/bank_central.c") &&
	(fname != "d/Standard/obj/odmieniacz.c"))
    {
	return;
    }

    set_auth(this_object(), "#:root");
    log_file(file, text, length);
}

void
log_public(string file, string text)
{
    int fsize, msize;
    string fname;

    fname = calling_program();

    file = OPEN_LOG_DIR + "/" + file;
    
    if(fname[0..5] != "secure" &&
       fname[0..9] != "std/living")
	return;

#ifdef CYCLIC_LOG_SIZE
    fsize = file_size(file);
    msize = CYCLIC_LOG_SIZE["root"];

    if (msize > 0 && fsize > msize)
	rename(file, file + ".old");
#endif /* CYCLIC_LOG_SIZE */

    set_auth(this_object(), "#:root");
    write_file(file, text);
}

/*
 * Function name: query_player_file_time()
 * Description  : This function will return the file-time the playerfile
 *                of a player was last saved.
 * Arguments    : string pl_name - the name of the player.
 * Returns      : int - the file-time of the players save-file.
 */
int
query_player_file_time(string pl_name)
{
    if (!strlen(pl_name))
    {
	return 0;
    }

    pl_name = lower_case(pl_name);
    set_auth(this_object(), "root:root");
    return file_time(PLAYER_FILE(pl_name) + ".o");
}

/*
 * Function name: exist_player
 * Description  : Checks whether a player exist or not.
 * Arguments    : string pl_name: the name of the player to check.
 * Returns      : int 1/0 - true if the player exists.
 */
int
exist_player(string pl_name)
{
    if (!strlen(pl_name))
    {
	return 0;
    }

    pl_name = lower_case(pl_name);
    return (file_size(PLAYER_FILE(pl_name) + ".o") > 0);
}

/*
 * Function name:   finger_player
 * Description:     Returns a player object restored with the values from
 *		    the players save file.
 * Arguments:	    pl_name: Name of player
 *		    file: (optional) Name of player_file
 * Returns:         The restored player object or 
 */
varargs object
finger_player(string pl_name, string file)
{
    object ob;
    int ret, lev;
    string f, pobj;

    if (strlen(pl_name) == 0)
	return 0;

    if (!file)
	file = FINGER_PLAYER;

    set_auth(this_object(), "#:backbone");
    ob = clone_object(file);

    f = function_exists("load_player", ob);
    if (f != PLAYER_SEC_OBJECT && f != FINGER_PLAYER)
    {
	do_debug("destroy", ob);
	return 0;
    }

    ob->master_set_name(pl_name);
    ob->open_player();               /* sets euid == 0 */
    set_auth(this_object(), "#:root");
    export_uid(ob);
    ret = ob->load_player(pl_name);

    set_auth(this_object(), "#:backbone");
    export_uid(ob);			/* Make the object powerless */
    ob->set_trusted(1);
    set_auth(this_object(), "#:root");
    ob->dodaj_przym(0);			/* Set the adjectives correctly */
    
    pobj = (previous_object() ? MASTER_OB(previous_object()) : 0);
    if (pobj &&
	(pobj != LOGIN_OBJECT) &&
	(pobj != CMD_LIVE_SOCIAL) &&
	(pobj != WIZ_CMD_APPRENTICE) &&
	(pobj != WIZ_CMD_NORMAL))
    {
	log_syslog("FINGER_OB", sprintf("%s: %s%s\n%s\n", ctime(time()),
	    (this_interactive() ? "(" + this_interactive()->query_real_name() +
				   ") "
				: ""), pl_name, DEBUG_FMT_CALLS(0)));
    }

    if (ret)
	return ob;
    else
    {
	ob->remove_object();
	return 0;
    }
}

/*
 * Function name: note_something
 * Description  : This function is called from the info.c commandsoul when
 *                someone made a report. It distuishes between sys-reports
 *                and room-related reports and writes them to the correct
 *                directory.
 * Arguments:     string str - the message to log.
 *                int id     - the id (type) of the log.
 *                object env - the environment of this_player().
 */
void
note_something(string str, int id, object env)
{
    string file;
    string text = extract(ctime(time(), 1), 3, -11);

    if (extract(text, 0, 0) == " ")
        text = extract(text, 1);

    /* If there is a SYS-related log, write it to the proper log file in the
     * OPEN_LOG_DIR.
     */
    if (id >= LOG_SYSBUG_ID)
    {
	set_auth(this_object(), "#:root");
	write_file((OPEN_LOG_DIR + "/SYS" + LOG_PATH(id)), (text + " " +
	    file_name(env) + " (" +
	    capitalize(this_player()->query_real_name()) + ")\n" + str +
	    "\n"));
	return;
    }

    file = creator_object(env);

    set_auth(this_object(), "#:root");

    file = query_wiz_path(file) + "/log";
    if (file_size(file) != -2)
    {
	mkdir(file);
    }

    file += LOG_PATH(id);
    
    write_file(file, text + " " + file_name(env) + " (" +
        capitalize(this_player()->query_real_name()) + ")\n" + str + "\n");
}

/*
 * Function name:   query_snoop
 * Description:     Tells caller if a user is snooped.
 * Arguments:	    snoopee: pointer to a user object
 * Returns:         0 if not snooped, 1 if snooped and caller is lord or lower
 *                  object pointer to snooper if caller is arch or higher
 */
mixed
query_snoop(object snoopee)
{
    object snooper;
    int type = query_wiz_rank(geteuid(previous_object()));

    if (type >= WIZ_ARCH)
	return efun::query_snoop(snoopee);
    else if (check_snoop_validity(previous_object(), snoopee, 1) &&
		(snooper = (efun::query_snoop(snoopee))) &&
		(type >= query_wiz_rank(snooper->query_real_name())))
	return 1;
    else
	return 0;
}

/*
 * Function name: query_start_time
 * Description  : Return the time when the game started.
 * Returns      : int - the time.
 */
public int
query_start_time()
{
    int theport;
    string game_start;

    theport = debug("mud_port");
    if (theport != 0)
    {
	game_start = GAME_START + "." + theport;
	if (file_size(game_start) > 0)
	{
	    return file_time(game_start);
	}
    }

    /* This value will be wrong if the master has been updated. */
    return object_time(this_object());
}

/*
 * Function name: query_irregular_uptime
 * Description  : The (irregular) uptime after which this game is being
 *                rebooted. This uptime is counted from the start of the
 *                game.
 * Returns      : int - the (irregular) uptime, or 0.
 */
public int
query_irregular_uptime()
{
    return irregular_uptime;
}

/*
 * Function name: commune_log
 * Description  : Logs a commune from a mortal.
 * Arguments    : string str  - the message.
 */
public void
commune_log(string str)
{
    log_public("COMMUNE", sprintf("%s: %-11s: %s", ctime(time()),
	this_interactive()->query_real_name(), str));
}

/*
 * Function name: set_runlevel
 * Description  : Set the runlevel. That is the lowest rank of player that
 *                is allowed into the game. Set to WIZ_MORTAL (0) when the
 *                game is open to all players and wizards. This function
 *                may only be called from the normal wizard soul, i.e. from
 *                the 'shutdown' command.
 * Arguments    : int - the runlevel. The argument is not checked for
 *                    validity. That must have been done in the command.
 */
public void
set_runlevel(int level)
{
    if (previous_object() != find_object(WIZ_CMD_NORMAL))
    {
        write("Illegal call to set_runlevel(). Call only from wizard soul.\n");
        return;
    }

#ifdef LOG_SHUTDOWN
    log_file(LOG_SHUTDOWN, ctime(time()) + " Runlevel " +
        WIZ_RANK_NAME(level) + " set by " + this_interactive()->query_name() +
        ".\n", -1);
#endif LOG_SHUTDOWN

    runlevel = level;

    save_master();
}

/*
 * Function name: query_runlevel
 * Description  : Returns the runlevel. This is the lowest rank of player
 *                that is allowed into the game. It returns WIZ_MORTAL (0)
 *                when the game is open to all players and wizards.
 * Returns      : int - the runlevel.
 */
public int
query_runlevel()
{
    return runlevel;
}
/*
 * Function name: master_shutdown
 * Description  : Perform the final shutdown. This function may only be
 *                called from the armageddon object.
 * Returns      : 1 - Ok, 0 - No shutdown performed.
 */
public int
master_shutdown(string reason)
{
    if (MASTER_OB(previous_object()) != ARMAGEDDON)
    {
	return 0;
    }

#ifdef LOG_SHUTDOWN
    log_file(LOG_SHUTDOWN, ctime(time()) + " " + reason, -1);
#endif LOG_SHUTDOWN
    
    /* This MUST be a this_object()->
     * If it is removed the game wont go down, so hands off!
     */
    this_object()->do_debug("shutdown");
    return 1;
}

/*
 * Function name: request_shutdown
 * Description  : When a wizard wants to shut down the game, this
 *                function is called to invoke Armageddon. The function
 *                should be called from the shutdown command in
 *                WIZ_CMD_NORMAL.
 * Arguments    : string reason - the reason to shut down the game.
 *                int    delay  - the delay in minutes.
 */
public void
request_shutdown(string reason, int delay)
{
    string euid    = getwho();
    string shutter = ARMAGEDDON->query_shutter();

    if (strlen(shutter))
    {
	write("Game is already being shut down by " + shutter + ".\n");
	return;
    }

    if (query_wiz_rank(euid) < WIZ_NORMAL)
    {
	write("Illegal euid. Shutdown rejected.\n");
	return;
    }

    if (!strlen(reason))
    {
	write("No reason for shutdown. Shutdown rejected.\n");
	return;
    }

    if (reason == "memory_failure")
    {
	/* Arches and keepers can force a memory failure every time. Other
	 * wizards can only do so if the memory usage is >= 99%.
	 */
	if ((query_wiz_rank(euid) < WIZ_ARCH) &&
	    (query_memory_percentage() < 99))
	{
	    write("There is no reason to ask for a memory_failure yet.\n");
	    return;
	}

	memory_failure();
	return;
    }

    ARMAGEDDON->start_shutdown(reason, delay, euid);
}

/*
 * Function name: calcel_shutdown
 * Description  : When the wizard has second thoughts and does not want
 *                to shut the game down after all, this function is
 *                called. The function should be called from
 *                WIZ_CMD_NORMAL.
 */
public void
cancel_shutdown()
{
    string euid    = getwho();
    string shutter = ARMAGEDDON->query_shutter();
    int    rank    = query_wiz_rank(euid);

    if (mem_fail_flag)
    {
	write("Game is shutting down due to insufficient memory. Cancel " +
	    "is not possible.\n");
	return;
    }

    if (rank <= WIZ_RETIRED)
    {
	write("Illegal euid. Cancel shutdown rejected.\n");
	return;
    }

    if ((euid != shutter) &&
	(rank <= WIZ_LORD))
    {
	write("You are not allowed to cancel a shutdown by " +
	    shutter + ".\n");
	return;
    }

    ARMAGEDDON->cancel_shutdown(euid);
}
/*
 * Function name:  wiz_home
 * Description:    Gives a default 'home' for a wizard, domain or a player
 * Arguments:      wiz: The wizard name.
 * Returns:        A filename for the 'home' room.
 */
string
wiz_home(string wiz)
{
    string path;

    if (query_wiz_rank(wiz) == WIZ_MORTAL)
	if (query_domain_number(wiz) < 0)	/* Not even a domain */
	    return "";

    path = query_wiz_path(wiz) + "/workroom.c";
    set_auth(this_object(), "#:root");
    if (file_size(path) <= 0)
	write_file(path, "inherit \"/std/workroom\";\n\n" +
		   "void\ncreate_workroom()\n{\n  ::create_workroom();\n}\n");

    return path;
}

/*
 * Function name:  wiz_force_check
 * Description:    Checks if one wizard is allowed to force another
 * Arguments:      forcer: Name of wizard trying to force
 *		   forced: Name of wizard being forced
 * Returns:        True if ok
 */
int
wiz_force_check(string forcer, string forced)
{
    int             rlev,
                    dlev;
    if (forcer == forced)
	return 1;

    rlev = query_wiz_rank(forcer);
    if ((rlev == WIZ_KEEPER) || (forcer == ROOT_UID))
	return 1;

    dlev = query_wiz_rank(forced);

    if ((rlev >= WIZ_ARCH) && (dlev <= WIZ_LORD))
	return 1;

    return 0;
}

/*
 * Function name: set_sanctioned
 * Description:   Set the 'sanctioned' field in the player.
 * Arguments:     wizname - The wizard to set in
 *		  map - The 'sanctioned' map to set.
 */
public int
set_sanctioned(string wizname, mapping map)
{
    string wname;
    object wiz;

    wname = geteuid(previous_object());

    if ((wname != wizname) && (query_wiz_rank(wname) < WIZ_ARCH))
	return 0;

    wiz = find_player(wizname);
    if (!wiz)
	return 0;

    wiz->set_sanctioned(map);
    return 1;
}

/*
 * Function name: query_banished
 * Description  : Find out whether a name has been banished for use by
 *                new players.
 * Arguments    : string name - the name to check for banishment.
 * Returns      : int 1/0     - true when the name has been banished.
 */
public int
query_banished(string name)
{
    if (!strlen(name))
    {
	return 0;
    }

    /* No need to set the euid. Everyone can see file sizes. */
    name = lower_case(name);
    return (file_size(BANISH_FILE(name)) > 0);
}

/*
 * Function name: banish
 * Description:   Banish a name, info about name, remove a banishment.
 * Arguments:     name - the name to perform banish operation on.
 *		  what - what to do.
 * Returns:       A list with information.
 */
mixed *
banish(string name, int what)
{
    string file;
    mixed *info;

    info = allocate(2);

    if ((previous_object() != find_object(WIZ_CMD_NORMAL)) &&
        (previous_object() != find_object(WIZ_CMD_APPRENTICE)))
    {
	return ({});
    }

    name = lower_case(name);
    file = BANISH_FILE(name);
    
    if (file_size(file) > -1)
    {
	info[0] = read_file(file);
	info[1] = file_time(file);
    }

    switch (what)
    {
    case 0: /* Information */
	if (file_size(file) > -1)
	    return info;
	break;

    case 1: /* Remove */
	if (file_size(file) > -1)
	{
	    rm(file);

            /* Log the action */
            log_file("BANISHED", ctime(time()) + " " + capitalize(name)
                   + ": removed by " + capitalize(geteuid(previous_object()))
                   + ".\n", -1);

	    return info;
	}
	break;

    case 2: /* Add */
	if (file_size(file) > -1)
	    return info;
	write_file(file, geteuid(previous_object()));

        /* Log the action */
        log_file("BANISHED", ctime(time()) + " " + capitalize(name)
               + ": added by " + capitalize(geteuid(previous_object()))
               + ".\n", -1);

	break;

    default: /* Strange */
	break;
    }

    return ({});
}

/*
 * Function name: do_debug
 * Description  : This function is a front for the efun debug(). You are
 *                only allowed to call debug() through this object because we
 *                need to make some security checks.
 * Arguments    : string icmd - the debug command.
 *                mixed a1    - a possible argument to debug().
 *                mixed a2    - a possible argument to debug().
 *                mixed a3    - a possible argument to debug().
 * Returns      : mixed - the relevant return value for the particular
 *                        debug command.
 */
varargs mixed
do_debug(string icmd, mixed a1, mixed a2, mixed a3)
{
    string euid = geteuid(previous_object());

    /* Some debug() commands are not meant to be called by just anybody. Only
     * 'root' and the administration may call them.
     */
    if (member_array(icmd, DEBUG_RESTRICTED) != -1)
    {
	if (euid != ROOT_UID &&
	    previous_object() != this_object() &&
	    previous_object() != find_object(SIMUL_EFUN) &&
	    query_wiz_rank(euid) < WIZ_ARCH &&
	    !query_admin_team_member(euid, "aog") &&
	    !query_admin_team_member(euid, "aom"))
        {
	    return 0;
	}
    }

    /* In order to get the variables of a certain object, you need to have
     * the proper euid. If you are allowed to write to the file, you are
     * also allowed to view its variables.
     */
    if ((icmd == "get_variables") &&
	(!valid_write(file_name(a1), euid, "get_variables")))
    {
	return 0;
    }

    /* Since debug() returns arrays and mappings by reference, we need to
     * process the value to make it secure, so people cannot alter it.
     */
    return secure_var(debug(icmd, a1, a2, a3));
}

/*
 * Function name:   send_udp_message
 * Description:     Sends a udp message to arbitrary host, port
 * Arguments:	    to_host: Hostname or IP-number
 *		    to_port: Portnumber
 *		    msg:     Message to send
 * Returns:	    True if the message is sent. There is of course no
 *		    guarantee it will be received.
 */
int
send_udp_message(string to_host, int to_port, string msg)
{
//    log_syslog("UDP_IO", sprintf("%15s > %s\n", to_host,
//               implode(explode(msg, "\n"), "\\n")));

    if (stringp(udp_manager) && 
	previous_object() == find_object(udp_manager))
	return debug("send_udp", to_host, to_port, msg);
    else
	return 0;
}

/*
 * Function name:  check_memory
 * Description:    Checks with 'debug malloc' if it is time to reboot
 * Arguments:      dodecay - decay xp or nto.
 *		   The limit can be defined in config.h as MEMORY_LIMIT
 */
public void
check_memory(int dodecay)
{
    int uptime;

    /* Is the game too big? */
    if (query_memory_percentage() >= 100)
    {
	memory_failure();
    }

    uptime = time() - query_start_time();
#ifdef REGULAR_REBOOT
    /* See whether it is time to reboot the game. The game must have been
     * up at least an hour to reboot, though.
     */
    if (uptime > 3600)
    {
	/* We have to add this 3600 because time() starts counting at 01:00. */
	if ((((time() + 3600) % 86400) / 3600) == REGULAR_REBOOT) 
	{
	    set_auth(this_object(), "root:root");
	    ARMAGEDDON->start_shutdown("Juz po " + REGULAR_REBOOT +
		":00, co oznacza, ze nadeszla pora na codzienna Apokalipse!",
		10, ROOT_UID);
	}
    }
#endif REGULAR_REBOOT

#ifdef REGULAR_UPTIME
    if (uptime > irregular_uptime)
    {
        set_auth(this_object(), "root:root");
/*
        ARMAGEDDON->start_shutdown("The game has been up " + CONVTIME(uptime) +
            ", time for a reboot!", 10, ROOT_UID);
*/
	ARMAGEDDON->start_shutdown(CONVTIME(uptime) + " od ostatniej " +
	    "Apokalipsy - najwyzsza pora na nastepna!", 10, ROOT_UID);
    }
#endif REGULAR UPTIME

    /* We should add a decay here for the xp stored for each domain. */
    if (dodecay == 1)
    {
	decay_exp();
    }
}

/*
 * Function name:  startup_udp
 * Description:    Give the contents of the package to send as startup
 *		   message to the mudlist server. This is default if we
 *		   have no UDP_MANAGER.
 * Returns:        The message.
 */
string 
startup_udp()
{
    return 
	"||NAME:" + get_mud_name() +
	"||VERSION:" + do_debug("version") +
	"||MUDLIB:" + MUDLIB_VERSION +
	"||HOST:" + query_host_name() +
	"||PORT:" + debug("mud_port") +
	"||PORTUDP:" + debug("udp_port") +
	"||TIME:" + efun::ctime(time());
}

/*
 * Function name:  set_known_muds
 * Description:    The UDP manager can set what muds are known.
 * Returns:	   True if set.
 */
int
set_known_muds(mapping m)
{
    if (stringp(udp_manager) &&
	previous_object() == find_object(udp_manager))
    {
	known_muds = m;
	save_master();
	return 1;
    }
    return 0;
}

/*
 * Function name:  query_known_muds
 * Description:    Give the currently known muds
 * Returns:        A mapping of information indexed with mudnames
 */
mapping
query_known_muds()
{
    if (mappingp(known_muds))
    {
	return secure_var(known_muds);
    }

    return 0;
}

/*
 * Function name: load_admin_teams
 * Description  : At boot-time, this function loads all admin teams.
 */
void
load_admin_teams()
{
    string *files;
    int    index;

    admin_teams = ([ ]);
    files = get_dir(ALIAS_DIR + "admin") + get_dir(ALIAS_DIR + "ao?");
    index = sizeof(files);
    while(--index >= 0)
    {
        admin_teams[files[index]] =
            explode(read_file(ALIAS_DIR + files[index]), "\n");
    }
}

/*
 * Function name: query_admin_team_members
 * Description  : Returns which wizards are member of an admin team.
 * Arguments    : string team - the team to check.
 * Returns      : string * - array of team members, or 0.
 */
string *
query_admin_team_members(string team)
{
    if (!pointerp(admin_teams[team]))
    {
        return 0;
    }

    return ({ }) + admin_teams[team];
}

/*
 * Function name: query_admin_team_member
 * Description  : Returns which a certain wizard is a member of a certain
 *                admin team.
 * Arguments    : string name - the member to check.
 *                string team - the team to check.
 * Returns      : int 1/0 - member or not.
 */
int
query_admin_team_member(string name, string team)
{
    return (pointerp(admin_teams[team]) &&
        (member_array(name, admin_teams[team]) > -1));
}

/* 
 * Function name: query_prevent_shadow
 * Description  : This function will prevent shadowing of this object.
 * Returns      : int 1 - always.
 */
nomask int
query_prevent_shadow()
{
    return 1;
}
