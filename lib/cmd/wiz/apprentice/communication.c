/*
 * /cmd/wiz/apprentice/communication.c
 *
 * This is a sub-part of /cmd/wiz/apprentice.c
 *
 * This file contains the commands that allow wizards to communicate.
 *
 * Commands currently included:
 * - audience
 * - busy
 * - emote (short ':')
 * - line
 * - linee
 * - lineconfig
 * - next
 * - tell
 * - wiz
 * - wize
 * - wsay
 */

#include <composite.h>
#include <filter_funs.h>
#include <flags.h>
#include <std.h>
#include <stdproperties.h>
#include <options.h>

/* **************************************************************************
 * Prototypes.
 */
nomask varargs int line(string str, int emotion);
static nomask string cat_string(mixed org, string what, object pl, int up);
static nomask mixed mk_item(object ob);
static nomask void tell_them(object ob, mixed arg);

/* **************************************************************************
 * Global variable.
 *
 * channels = ([ (string) lower case channel name :
 *                              ({ (string)   channel name,
 *                                 (string *) users,
 *                                 (string *) hidden users,
 *                                 (int 0/1)  status open/closed,
 *                                 (string)   owner name
 *                               }) ])
 */
static private mapping channels;

#define CHANNEL_OPEN    (0) /* channel is open for all wizard.            */
#define CHANNEL_CLOSED  (1) /* channel is closed unless after invitation. */

#define CHANNEL_NAME    (0) /* the name of the channel.                   */
#define CHANNEL_USERS   (1) /* the users of the channel.                  */
#define CHANNEL_HIDDEN  (2) /* the hidden users of the channel.           */
#define CHANNEL_STATUS  (3) /* the status of the channel (open/closed).   */
#define CHANNEL_OWNER   (4) /* the owner of the channel.                  */

#define CHANNEL_WIZRANK ({ WIZNAME_APPRENTICE, WIZNAME_LORD, WIZNAME_ARCH })

/*
 * Function name: init_line
 * Description  : This function is called when the soul is created. It
 *                makes sure the mapping with the channels is loaded.
 *                Also, every time this object is created, all members
 *                of the channels are checked to see whether they are
 *                still wizards.
 */
private nomask void
init_line()
{
    int size;
    string *lines;
    string *wizards = SECURITY->query_wiz_list(-1);

    /* Get the channels information from SECURITY. */
    channels = SECURITY->query_channels();

    if (!mappingp(channels))
    {
	channels = ([ ]);
    }

    /* Every time this object is loaded into memory we check whether the
     * people in the channels are still real wizards.
     */
    lines = m_indices(channels);
    size = sizeof(lines);
    while(--size >= 0)
    {
	channels[lines[size]][CHANNEL_USERS] &= wizards;
	channels[lines[size]][CHANNEL_HIDDEN] &= wizards;
    }

    /* It is not likely that this information changes each time, so we
     * do not have to save this information every time we process it.
     */
    if (!random(20))
    {
	SECURITY->set_channels(channels);
    }
}

/* **************************************************************************
 * audience - queue somone for talks.
 */
nomask int
audience(string who)
{
    string *queue_list, name;
    int index, size;
    object ob, *obs;

    queue_list = (string *)this_interactive()->query_queue_list();
    if (!stringp(who))
    {
	if (!sizeof(queue_list))
	{
	    write("No one is requesting an audience.\n");
	    return 1;
	}

	size = sizeof(queue_list);
	index = -1;
	while(++index < size)
	{
	    write((index + 1) + ": " + capitalize(queue_list[index]) + "\n");
	}
	return 1;
    }

    if (who == "list")
    {
	obs = filter(users(), objectp);

	size = sizeof(obs);
	index = -1;
	while(++index < size)
	{
	    if (member_array(this_interactive()->query_real_name(), 
		obs[index]->query_queue_list()) == -1)
	    {
		obs[index] = 0;
	    }
	}
	obs -= ({ 0 });
	if (!sizeof(obs))
	{
	    write("You are not requesting an audience with anyone.\n");
	    return 1;
	}
	write("You are requesting an audience with " +
	    COMPOSITE_WORDS(map(obs, capitalize @ &->query_real_name())) +
	    ".\n");
	return 1;
    }

    if (sscanf(who, "cancel %s", name) == 1) 
    {
	name = lower_case(name);
	ob = find_player(name);
	if (ob == this_interactive())
	{
	    write("You are not requesting an audience with yourself.\n");
	    return 1;
	}
	if (!ob)
	{
	    write("Couldn't locate " + name + ".\n");
	    return 1;
	}
	if (ob->pop_queue(this_interactive()->query_real_name()) == "")
	{
	    write("You are not requesting an audience with " +
		capitalize(name) + ".\n");
	    return 1;
	}

	write("You cancel your request for audience with " + 
	      capitalize(name) + ".\n");
	tell_object(ob, this_interactive()->query_cap_name() +
	    " canceled " + this_interactive()->query_possessive() +
	    " request for audience with you.\n");
	return 1;
    }

    who = lower_case(who);
    ob = find_player(who);
    if (ob == this_interactive())
    {
	write("No, you are not availble for consultations to yourself.\n");
	return 1;
    }
    if (!objectp(ob))
    {
	write("Couldn't locate " + who + ".\n");
	return 1;
    }
    index = ob->add_queue(this_interactive()->query_real_name());
    if (!index)
    {
	write("You are already requesting an audience with " +
	    capitalize(who) + ".\n");
    }
    else
    {
	write("You have been queued as number " + index + ".\n");
	tell_object(ob, this_interactive()->query_cap_name() +
	    " is requesting an audience with you.\n");
    }
    return 1;
}

/* **************************************************************************
 * busy - set/query your busy status
 */

/*
 * Function name: busy_string
 * Description  : Converts an the busy flag into a string.
 * Arguments    : int    - the busy level
 * Returns      : string - its string representation.
 */
public nomask string
busy_string(int busy)
{
    string bstring = "";

    if (busy & BUSY_F)
	return "F";

    if (busy & BUSY_W)
	bstring += "W";
    if (busy & BUSY_T)
	bstring += "T";
    if (busy & BUSY_M)
	bstring += "M";
    if (busy & BUSY_P)
	bstring += "P";
    if (busy & BUSY_S)
	bstring += "S";
    if (busy & BUSY_C)
	bstring += "C";
    if (busy & BUSY_G)
	bstring += "G";
    if (busy & BUSY_I)
	bstring += "I";
    if (busy & BUSY_L)
	bstring += "L";

    return bstring;
}

nomask int
busy(string what)
{
    int index;
    int size;
    int busy;

    CHECK_SO_WIZ;

    busy = this_interactive()->query_prop(WIZARD_I_BUSY_LEVEL);

    if (!stringp(what))
    {
	if (!busy)
	{
	    write("You're in an attentive state of mind.\n");
	    return 1;
	}

	write("Busy: " + busy_string(busy) + "\n");
	return 1;
    }

    if (what == "clear")
    {
	if (!busy)
	{
	    write("You are already in an attentive state of mind.\n");
	    return 1;
	}

	this_interactive()->remove_prop(WIZARD_I_BUSY_LEVEL);
	write("Cleared your busy status.\n");
	return 1;
    }

    size = strlen(what);
    index = -1;

    while(++index < size)
    {
	switch(what[index])
	{

	case 'W':
	    busy ^= BUSY_W;
	    break;

	case 'T':
	    busy ^= BUSY_T;
	    break;

	case 'M':
	    busy ^= BUSY_M;
	    break;

	case 'P':
	    busy ^= BUSY_P;
	    break;

	case 'S':
	    busy ^= BUSY_S;
	    break;

	case 'C':
	    busy ^= BUSY_C;
	    break;

	case 'G':
	    busy ^= BUSY_G;
	    break;

	case 'I':
	    busy ^= BUSY_I;
	    break;

	case 'L':
	    busy ^= BUSY_L;
	    break;

	case 'F':
	    busy = BUSY_ALL;
	    break;

	default:
	    write("Strange busy state: " + extract(what, index, index) + "\n");
	    break;
	}
    }

    this_interactive()->add_prop(WIZARD_I_BUSY_LEVEL, busy);

    write("New busy status: " + busy_string(busy) + ".\n");
    return 1;
}

/* **************************************************************************
 * emote, : - emote something
 */
nomask int
emote(string arg)
{
    object	pl, *pls;
    string	*args, *nms, oarg;
    int		i, sz, up;
    mapping	emap;

    CHECK_SO_WIZ;

    pls = FILTER_LIVE(all_inventory(environment(this_player()))) - ({ this_player() });
    emap = mkmapping(pls, map(pls, mk_item));

    if (!stringp(arg))
	return notify_fail("Usage: : <emote string>, emote string can contain names of present people enclosed in '|' characters, which then will be identified by their met/nonmet names to observers. e.g. ': smile fondly at |madmartigan|'\n");

    oarg = arg;
    args = explode(arg, "|");

    emap = map(emap, &cat_string(, args[0], 0, 0));
    if ((sz = sizeof(args)) > 1)
    {
	for (i = 1 ; i < sz ; i ++)
	{
	    nms = explode(args[i], " ");
	    up = nms[0] == lower_case(nms[0]) ? 0 : 1;
	    nms[0] = lower_case(nms[0]);
	    if (objectp((pl = present(nms[0], environment(this_player())))))
	    {
		emap = map(emap, &cat_string(, "", pl, up));
		if (sizeof(nms) > 1)
		{
		    arg = implode(nms[1..], " ");
		    emap = map(emap, &cat_string(, arg, 0, up));
		}
	    }
	    else if (i % 2)
		return notify_fail("You can`t see " + nms[0] + " here.\n");
	    else
		emap = map(emap, &cat_string(, args[i], 0, up));
	}
    }

    map(emap, &tell_them(this_player(), ));

    if (this_player()->query_option(OPT_ECHO))
	write("You emote: |" + capitalize(this_player()->query_real_name()) + "| " + oarg + "\n");
    else
	write("Ok.\n");
    return 1;
}

static nomask string
cat_string(mixed org, string what, object pl, int up)
{
    if (objectp(pl))
	org[1] += (pl == org[0] ? (up == 1 ? "You" : "you") : (up == 1 ? QCTNAME(pl) : QTNAME(pl)));
    else
	org[1] += what;
    
    return org;
}

static nomask mixed
mk_item(object ob)
{
    return ({ ob, "" });
}

static nomask void
tell_them(object ob, mixed arg)
{
    arg[0]->catch_msg(QCTNAME(ob) + " " + arg[1] + "\n");
}

/* **************************************************************************
 * line  - speak on any of the lines.
 * linee - emote on any of the lines.
 */

/*
 * Function name: line_list
 * Description  : Display the people listening to a certain line. At this
 *                point we are sure that the line in question exists.
 * Arguments    : string line - the list to display.
 * Returns      : int 1/0 - true if the line was displayed to the player.
 */
static nomask int
line_list(string line)
{
    string name = this_interactive()->query_real_name();
    int    access = (SECURITY->query_wiz_rank(name) >= WIZ_ARCH);
    /*    || (channels[line][CHANNEL_OWNER] == name); */
    string *names;
    string *nonpresent;
    string str;

    /* Closed channel that the wizard is not entitled to list. */
    if ((channels[line][CHANNEL_STATUS] == CHANNEL_CLOSED) &&
    	(!access) &&
    	(member_array(name, channels[line][CHANNEL_USERS]) == -1) &&
    	(member_array(name, channels[line][CHANNEL_HIDDEN]) == -1))
    {
    	return 0;
    }

    names = channels[line][CHANNEL_USERS] & users()->query_real_name();
    nonpresent = (string *)channels[line][CHANNEL_USERS] - names;

    str = sprintf("%-8s: %-1s:", channels[line][CHANNEL_NAME],
    	(channels[line][CHANNEL_STATUS] ? "C" : "O"));
    str += (sizeof(names) ? (" " +
    	implode(map(sort_array(names), capitalize), ", ")) : "");
    str += (sizeof(nonpresent) ? (" NONPRESENT " +
    	implode(map(sort_array(nonpresent), capitalize), ", ")) : "");

    /* Test for hidden listeners if the person is entitled to do so. */
    if (access)
    {
	names = channels[line][CHANNEL_HIDDEN] & users()->query_real_name();
	nonpresent = (string *)channels[line][CHANNEL_HIDDEN] - names;

	if (sizeof(names) ||
	    sizeof(nonpresent))
	{
	    str += " HIDDEN";
	    str += (sizeof(names) ? (" " +
		implode(map(sort_array(names), capitalize), ", ")) : "");
	    str += (sizeof(nonpresent) ? (" NONPRESENT " +
		implode(map(sort_array(nonpresent), capitalize), ", ")) : "");
	}
    }
    /* Else see whether the player is hidden himself. */
    else if (member_array(name, channels[line][CHANNEL_HIDDEN]) >= 0)
    {
    	str += " HIDDEN " + capitalize(name);
    }

    str += " OWNER " + capitalize(channels[line][CHANNEL_OWNER]);

    if (strlen(str) <= 77)
    {
	write(str + "\n");
    }
    else
    {
	names = explode(break_string(str, 77), "\n");
	write(implode( ({ names[0] }) +
	    (explode(break_string(implode(names[1..], " "), 62), "\n")),
	    ("\n             ")) + "\n");
    }

    return 1;
}

nomask varargs int
line(string str, int emotion = 0, int busy_level = 0)
{
    string *members, *receivers;
    string line;
    int    size;
    int    rank;
    object wizard;

    CHECK_SO_WIZ;

    if (!stringp(str))
    {
        notify_fail(capitalize(query_verb()) + " what?\n");
        return 0;
    }

    if (sscanf(str, "%s %s", line, str) != 2)
    {
    	line = str;
    	str = "";
    }

    /* Channel is a wizline-channel. */
    if ((rank = member_array(line, CHANNEL_WIZRANK)) >= 0)
    {
	rank = WIZ_R[member_array(line, WIZ_N)];
	members = map(users() - ({ this_player() }), geteuid);
	members = filter(members, &operator( >= )(, rank) @
	    SECURITY->query_wiz_rank);
	line = ((line == WIZNAME_APPRENTICE) ? "Wizline" : capitalize(line));
    }
    /* Channel is domain-channel. */
    else if (SECURITY->query_domain_number(line) > -1)
    {
	rank = 1;
	line = capitalize(line);
	members = SECURITY->query_domain_members(line);
	members &= map(users() - ({ this_player() }), geteuid);
    }
    /* Channel is normal type of channel, well, you know what I mean. */
    else if (pointerp(channels[lower_case(line)]))
    {
	line = lower_case(line);
	members = channels[line][CHANNEL_USERS] +
	    channels[line][CHANNEL_HIDDEN];
	members &= map(users() - ({ this_player() }), geteuid);
    	line = channels[line][CHANNEL_NAME];
    }
    else
    {
	notify_fail("There is no channel called '" + line + "'.\n");
	return 0;
    }

    /* No message means list the particular line. */
    if (!strlen(str))
    {
	if (rank != -1)
	{
	    notify_fail("You cannot list domain or wizard lines this way.\n");
	    return 0;
	}
    
    	if (line_list(lower_case(line)))
    	{
    	    return 1;
    	}

    	notify_fail("There is no channel named '" + line + "' for you.\n");
    	return 0;
    }

    receivers = filter(members, not @ &operator(&)(busy_level | BUSY_F) @
		       &->query_prop(WIZARD_I_BUSY_LEVEL) @ find_player);
    
    if (!(size = sizeof(receivers)))
    {
	notify_fail("There is no one listening to the channel '" + line +
	    "' at this moment, so your message is not heard.\n");
	return 0;
    }

    str = (line == "Wizline" ? "@ " : "<" + line + "> ") +
	   capitalize(this_player()->query_real_name() +
	   ((emotion ? emotion : (query_verb() == "linee")) ? " " : ": ") +
	   str + "\n");

    while(size--)
    {
	tell_object(find_player(receivers[size]), str);
    }

    if (this_player()->query_option(OPT_ECHO))
    {
        write(str);
    }
    else
    {
        write("Ok.\n");
    }

    return 1;
}

/* **************************************************************************
 * lineconfig - configurate any of the lines.
 */
nomask int
lineconfig(string str)
{
    string cmd;
    string name = this_player()->query_real_name();
    int    rank = SECURITY->query_wiz_rank(name);
    string line;
    string *lines;
    string target;
    int    hide = 0;
    int    index;
    int    size;

    if (!stringp(str))
    {
	notify_fail("Which subcommand for lineconfig do you wish to use?\n");
	return 0;
    }

    if (sscanf(str, "%s %s", cmd, str) != 2)
    {
	cmd = str;
	str = "";
    }

    switch(cmd)
    {
    case "hadd":
	if (rank < WIZ_ARCH)
	{
	    notify_fail("You may not make someone a hidden member to a " +
		"line.\n");
	    return 0;
	}
	hide = 1;
	/* Intentionally no break or return. Fall into the next case: add */

    case "add":
	if (!strlen(str) ||
	    (sscanf(str, "%s to %s", target, line) != 2))
	{
	    notify_fail("Syntax: lineconfig add <name> to <channel>\n");
	    return 0;
	}
	if (!pointerp(channels[line = lower_case(line)]))
	{
	    notify_fail("No line called '" + line + "'.\n");
	    return 0;
	}
	if ((rank < WIZ_ARCH) &&
	    (channels[line][CHANNEL_OWNER] != name))
	{
	     notify_fail("You are not the owner of '" + line + "'.\n");
	     return 0;
	}
	if (!(SECURITY->query_wiz_rank(target = lower_case(target))))
	{
	     notify_fail("No wizard name '" + capitalize(target) + "'.\n");
	     return 0;
	}

	channels[line][(hide ? CHANNEL_HIDDEN : CHANNEL_USERS)] +=
	    ({ target });
	SECURITY->set_channels(channels);

	write("Added " + capitalize(target) + " as " +
	    (hide ? "hidden " : "") + "member to channel " +
	    channels[line][CHANNEL_NAME]  + ".\n");
	return 1;

    case "create":
	if (rank < WIZ_NORMAL)
	{
	    notify_fail("Only full wizards and higher may create lines.\n");
	    return 0;
	}
	if (!strlen(str) ||
	    (sscanf(str, "%s %s", target, str) != 2))
	{
	    notify_fail("Syntax: lineconfig create open/closed <channel>\n");
	    return 0;
	}
	if ((strlen(str) < 2) ||
	    (strlen(str) > 8))
	{
	    notify_fail("The channel-name must be at least 2 and at most " +
		"8 characters long.\n");
	    return 0;
	}
	if (pointerp(channels[line = lower_case(str)]))
	{
	    notify_fail("The channel '" + line + "' already exists.\n");
	    return 0;
	}
	if (SECURITY->query_domain_number(line) > -1)
	{
	    notify_fail("There already is a domain by the name '" +
		capitalize(line) + "'.\n");
	    return 0;
	}
	if (member_array(line, WIZ_N) >= 0)
	{
	    notify_fail("The name " + capitalize(line) +
		" is a wizard-type.\n");
	    return 0;
	}
	hide = ((target == "closed") ? CHANNEL_CLOSED : CHANNEL_OPEN);

	channels[line] = ({ str, ({ name }), ({ }), hide, name });
	SECURITY->set_channels(channels);

	write("Created " + (hide ? "closed" : "open") + " channel named " +
	    str + ".\n");
	return 1;

    case "expel":
	if (!strlen(str) ||
	    (sscanf(str, "%s from %s", target, line) != 2))
	{
	    notify_fail("Syntax: lineconfig expel <name> from <channel>\n");
	    return 0;
	}
	if (!pointerp(channels[line = lower_case(line)]))
	{
	    notify_fail("No line called '" + line + "'.\n");
	    return 0;
	}
	if ((rank < WIZ_ARCH) &&
	    (channels[line][CHANNEL_OWNER] != name))
	{
	     notify_fail("You are not the owner of '" + line + "'.\n");
	     return 0;
	}
	if (member_array(target, channels[line][CHANNEL_USERS]) >= 0)
	{
	    hide = 0;
	}
	else if ((rank >= WIZ_ARCH) &&
		 (member_array(target, channels[line][CHANNEL_HIDDEN]) >= 0))
	{
	    hide = 1;
	}
	else
	{
	    notify_fail(capitalize(target) +
		" is not a member of the channel " +
		channels[line][CHANNEL_NAME] + ".\n");
	    return 0;
	}

	channels[line][(hide ? CHANNEL_HIDDEN: CHANNEL_USERS)] -= ({ target });
	SECURITY->set_channels(channels);

	write("Expelled " + (hide ? "hidden " : "") + "member " +
	    capitalize(target) + " from channel " +
	    channels[line][CHANNEL_NAME]  + ".\n");
	return 1;

    case "hjoin":
	if (rank < WIZ_ARCH)
	{
	    notify_fail("You may not become a hidden member of a line.\n");
	    return 0;
	}
	hide = 1;
	/* Intentionally no break or return. Use the next case: join */

    case "join":
	if (!pointerp(channels[line = lower_case(str)]))
	{
	    notify_fail("There is no channel named '" + line + "'.\n");
	    return 0;
	}
	if (member_array(name, channels[line][CHANNEL_USERS]) >= 0)
	{
	    notify_fail("You are already a member of the channel " +
		channels[line][CHANNEL_NAME] + ".\n");
	    return 0;
	}
	if (member_array(name, channels[line][CHANNEL_HIDDEN]) >= 0)
	{
	    notify_fail("You are already a hidden member of the channel " +
		channels[line][CHANNEL_NAME] + ".\n");
	    return 0;
	}
	if ((channels[line][CHANNEL_STATUS] == CHANNEL_CLOSED) &&
	    (rank < WIZ_ARCH) &&
	    (channels[line][CHANNEL_OWNER] != name))
	{
	    notify_fail("The channel " + channels[line][CHANNEL_NAME] +
		" is closed. Its owner, " +
		capitalize(channels[line][CHANNEL_OWNER]) +
		", can add you.\n");
	    return 0;
	}

	channels[line][(hide ? CHANNEL_HIDDEN : CHANNEL_USERS)] += ({ name });
	SECURITY->set_channels(channels);

	write("Joined channel " + channels[line][CHANNEL_NAME] +
	    (hide ? " on the hidden list" : "") + ".\n");
	return 1;

    case "leave":
	if (!pointerp(channels[line = lower_case(str)]))
	{
	    notify_fail("There is no channel named '" + line + "'.\n");
	    return 0;
	}
	if (member_array(name, channels[line][CHANNEL_USERS]) >= 0)
	{
	    hide = 0;
	}
	else if (member_array(name, channels[line][CHANNEL_HIDDEN]) >= 0)
	{
	    hide = 1;
	}
	else
	{
	    notify_fail("You are not a member of the channel " +
		channels[line][CHANNEL_NAME] + ".\n");
	    return 0;
	}

	channels[line][(hide ? CHANNEL_HIDDEN : CHANNEL_USERS)] -= ({ name });
	SECURITY->set_channels(channels);

	write("You just left your " + (hide ? "hidden " : "") +
	    "membership of channel " + channels[line][CHANNEL_NAME] + ".\n");
	return 1;

    case "list":
	if (!strlen(str))
	{
	    size = m_sizeof(channels);
	    if (!size)
	    {
	    	notify_fail("There are no channels.\n");
	    	return 0;
	    }

	    lines = m_indices(channels);
	    index = -1;
	    hide = 0;
	    while(++index < size)
	    {
	        if (line_list(lines[index]))
	        {
	            hide = 1;
	        }
	    }

	    if (hide)
	    {
	    	return 1;
	    }

	    notify_fail("There are no channels open to you.\n");
	    return 0;
	}

	if (!pointerp(channels[line = lower_case(str)]))
	{
	    notify_fail("There is no channel named '" + line + "'.\n");
	    return 0;
	}

	if (line_list(line))
	{
	    return 1;
	}

	notify_fail("There is no channel named '" + line + "' for you.\n");
	return 0;

    case "remove":
	if (!pointerp(channels[line = lower_case(str)]))
	{
	    notify_fail("There is no channel named '" + line + "'.\n");
	    return 0;
	}
	if ((rank < WIZ_ARCH) &&
	    (channels[line][CHANNEL_OWNER] != name))
	{
	    notify_fail("You are not allowed to remove channel " +
		channels[line][CHANNEL_NAME] + ".\n");
	    return 0;
	}

	write("Channel " + channels[line][CHANNEL_NAME] + " removed.\n");

	channels = m_delete(channels, line);
	SECURITY->set_channels(channels);

	return 1;

    default:
	notify_fail("No lineconfig subcommand '" + cmd + "'.\n");
	return 0;
    }

    write("Fatal error in lineconfig(). Please report this.\n");
    return 1;
}

/* **************************************************************************
 * next - auidence next person in the list
 */
nomask int
next(string cmd)
{
    string name;
    object ob;

    name = this_interactive()->pop_queue();
    if (!stringp(name))
    {
	write("Your audience list is empty.\n");
	return 1;
    }

    if (!objectp(ob = find_player(name)))
    {
	write("Couldn't locate " + capitalize(name) + ".\n");
	return 1;
    }

    if ((cmd == "refuse") ||
	(cmd == "deny") ||
	(cmd == "cancel"))
    {
	tell_object(ob, this_interactive()->query_cap_name() +
	    " has refused your request for an audience.\n");
	write("You have refused " + capitalize(name) +
	    "'s request for an audience.\n");
    }
    else
    {
	tell_object(ob, this_interactive()->query_cap_name() +
	    " has granted you an audience.\n");
	write("You have granted " + capitalize(name) + " an audience.\n");
    }
    return 1;
}

/* **************************************************************************
 * tell - tell something to someone
 */
nomask int
tell(string str)
{
    int idle;
    object ob;
    string who;
    string msg;
    string *names;

    CHECK_SO_WIZ;

    if (!stringp(str) ||
	(sscanf(str, "%s %s", who, msg) != 2))
    {
	notify_fail("Tell whom what?\n");
	return 0;
    }

    who = capitalize(who);
    ob = find_living(lower_case(who));
    if (!objectp(ob))
    {
	notify_fail("No player named " + who + " logged in.\n");
	return 0;
    }

    if (!query_ip_number(ob))
    {
	notify_fail(who + " is link dead right now.\n");
	return 0;
    }

    if (ob->query_prop(WIZARD_I_BUSY_LEVEL) & (BUSY_S | BUSY_P))
    {
	write(who + " seems to be busy at the moment. If you want to be " +
	    "contacted when " + ob->query_pronoun() +
	    " is available again, use the 'audience' command.\n");
	return 1;
    }

    if ((idle = query_idle(ob)) > 300)
    {
        write(capitalize(ob->query_real_name()) + " is idle for " +
            CONVTIME(idle) + " and may not react instantly.\n");
    }

    if (ob->query_wiz_level())
    {
	ob->catch_msg(capitalize(this_player()->query_real_name()) +
	    " tells you: " + msg + "\n", this_player());
    }
    else if ((environment(ob) != environment(this_player())) ||
	     (!CAN_SEE(ob, this_player())))
    {
	ob->catch_msg("Nagle, tuz przed toba, z chmury dymu wylania sie " +
	    "wizerunek " + (ob->query_met(this_player()) ? 
	        this_player()->query_met_name(PL_DOP) : 
	        this_player()->query_nonmet_name(PL_DOP)) + ".\n" +
	    "W twym umysle dzwieczy " + this_player()->koncowka("jego", 
	    "jej") + " glos: " + msg + "\n" +
	    "Dym ponownie osnuwa postac i po chwili rozwiewa sie, " +
	    "pozostawiajac tylko wspomnienie po tej dziwnej wizycie.\n\n");
    }
    else
    {
	ob->catch_msg(this_player()->query_Imie(ob, PL_MIA) + 
	    " przekazuje ci: " + msg + "\n");
    }

    if (this_player()->query_option(OPT_ECHO))
    {
	write("You tell " + capitalize(who) + ": " + msg + "\n");
    }
    else
    {
        write("Ok.\n");
    }

    names = ob->query_prop(PLAYER_AS_REPLY_WIZARD);
    who = this_player()->query_real_name(PL_CEL);
    if (pointerp(names))
    {
	names = ({ who }) + (names - ({ who }) );
    }
    else
    {
	names = ({ who });
    }
    ob->add_prop(PLAYER_AS_REPLY_WIZARD, names);

    return 1;
}

/* **************************************************************************
 * wiz  - gossip on the wizline
 * wize - emote on the wizline
 */
nomask int
wiz(string str)
{
    int busy;

    if (!stringp(str))
    {
	notify_fail(capitalize(query_verb()) + " what?\n");
	return 0;
    }

    busy = this_interactive()->query_prop(WIZARD_I_BUSY_LEVEL);
    if (busy & BUSY_F)
    {
	write("WARNING: You are currently 'busy F'.\n");
    }
    else if (busy & BUSY_W)
    {
	write("WARNING: Your 'busy W' state is automatically switched off.\n");
	this_interactive()->add_prop(WIZARD_I_BUSY_LEVEL, (busy ^ BUSY_W));
    }

    return line((WIZNAME_APPRENTICE + " " + str), (query_verb() == "wize"),
		BUSY_W);
}

int
wsay(string str)
{
    object *wizards;
    
    if (!strlen(str))
    {
        notify_fail("Say what in the way of the wizards?\n");
        return 0;
    }
    
    wizards = filter(all_inventory(environment(this_player())),
        &->query_wiz_level()) - ({ this_player() });
    if (!sizeof(wizards))
    {
        notify_fail("There are no wizards present to hear your message.\n");
        return 0;
    }
    
    wizards->catch_tell(capitalize(this_player()->query_real_name()) +
        " wizard-speaks: " + str + "\n");
    
    if (this_player()->query_option(OPT_ECHO))
    {
        write("You wizard-speak: " + str + "\n");
    }
    else
    {
        write("Ok.\n");
    }
    return 1;  
}
