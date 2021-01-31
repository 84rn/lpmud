/*
 * /cmd/std/tracer_tool.c
 *
 * Below are the functions from the object tracer. This is a general purpose
 * tool. It can be used to find objects, list info about them, and walk up
 * and down the inventory lists.
 *
 * The following commands are supported:
 *
 * - At
 * - Call
 * - Cat
 * - Clean
 * - Dump
 * - Destruct
 * - Ed
 * - Goto
 * - I
 * - In
 * - Info
 * - Inventory
 * - Items
 * - Light
 * - More
 * - Move
 * - Set
 */

#pragma no_clone
#pragma save_binary
#pragma strict_types

inherit "/cmd/std/tracer_tool_base";

#include <filepath.h>
#include <macros.h>
#include <std.h>
#include <stdproperties.h>

/*
 * Prototype.
 */
int In(string str);

#define CHECK_SO_WIZ 	if (WIZ_CHECK < WIZ_NORMAL) return 0; \
			if (this_interactive() != this_player()) return 0
#define TRACER_STORES	"_tracer_stores"
#define TRACER_VARS	"_tracer_vars"
#define SPACES		("                                            ")

/*
 * Function name: get_soul_id
 * Description  : Returns the proper name in order to get a nice printout.
 *                On the other hand the name is simple so people can type
 *                it if they only want to know which commands this soul
 *                supports.
 * Returns      : string - the name.
 */
string
get_soul_id()
{
    return "tracer";
}

/*
 * Function name: query_tool_soul
 * Description  : Identify this as a tool soul.
 * Returns      : int 1 - always.
 */
nomask public int
query_tool_soul()
{
    return 1;
}

/*
 * Function name: query_cmdlist
 * Description  : This function returns mapping with the commands this soul
 *                supports and the functions to call. Please put new in
 *                alphabetical order.
 * Returns      : mapping - the commands and functions.
 */
mapping
query_cmdlist()
{
    return ([
	     "At"       : "At",

	     "Call"     : "Call",
	     "Cat"      : "Cat",
	     "Clean"    : "Clean",
	     
	     "Dump"     : "Dump",
	     "Destruct" : "Destruct",
	     
	     "Ed"       : "Ed",
	     
	     "Goto"     : "Goto",
	     
	     "I"        : "Inventory",
	     "In"       : "In",
	     "Info"     : "Info",
	     "Inventory": "Inventory",
	     "Items"    : "Items",
	     
	     "Light"    : "Light",
	     
	     "More"     : "More",
	     "Move"     : "Move",
	     
	     "Set"      : "Set"
	     ]);
}

/* **************************************************************************
 * Here follows the actual functions. Please add new functions in the 
 * same order as in the function name list.
 * **************************************************************************/

/*
 * At - do someting in someones environment.
 *
 * Syntax   : At <person> <command>
 * Arguments: <person>  - the name of the person.
 *            <command> - the command to execute.
 */
int
At(string str)
{
    string name;
    string cmd;
    object obj;

    if (!stringp(str) ||
	(sscanf(str, "%s %s", name, cmd) != 2))
    {
	notify_fail("Syntax: At <name> <command>\n");
	return 0;
    }

    if (!objectp(obj = find_player(name)))
    {
	notify_fail("Player \"" + name + "\" not found.\n");
	return 0;
    }

    if (!objectp(obj = environment(obj)))
    {
	notify_fail(capitalize(name) + " does not have an environment.\n");
	return 0;
    }

    return In(file_name(obj) + " " + cmd);
}

/*
 * Call - call a function in an object.
 *
 * Syntax   : Call <object> <function> [<arg1>[%%<arg2>...]]
 * Arguments: <object>   - the object to call a function in.
 *            <function> - the function to call in the object.
 *            <arg1> ... - possible arguments to call the function with.
 */
int
Call(string str)
{
    string *args;
    mixed  ret;
    object obj;

    CHECK_SO_WIZ;

    if (!strlen(str))
    {
	notify_fail("Syntax: Call <object> <function> [<arguments>]\n");
	return 0;
    }

    str = extract(implode(explode((str + " "), "\\n"), "\n"), 0, -2);
    args = explode(str, " ");
    if (sizeof(args) == 1)
    {
	notify_fail("Syntax: Call <object> <function> [<arguments>]\n");
	return 0;
    }

    obj = parse_list(args[0]);
    if (!objectp(obj))
    {
	write("Object '" + args[0] + "' not found.\n");
	return 0;
    }

    if (!function_exists(args[1], obj))
    {
	write("Function '" + args[1] + "' not defined by '" + args[0] +
	    "' (" + file_name(obj) + ")\n");
	write("It might exist in a shadow though, let's try.\n");
    }

    /* This message should be done before the actual call in order not to
     * get an error if the object gets destructed for then the VBFC will
     * fail.
     */
    say("@@call_message:" + file_name(this_object()) + "|" +
 	this_player()->query_real_name() + "|" + args[1] + "|" +
	file_name(obj) + "@@");

    /* If the total number of argements is two, this means there is no
     * argument to the function, else, we compute the arguments and
     * make the call via call_otherv().
     */
    if (sizeof(args) == 2)
    {
	ret = call_other(obj, args[1]);
    }
    else
    {
	ret = call_otherv(obj, args[1], parse_arg(implode(args[2..], " ")));
    }

    print_value(ret);

    assign("ret", ret);

    return 1;
}

/*
 * Function name: call_message
 * Description  : Give a different message to wizards and mortals when
 *                you call a function in a certain object.
 * Arguments    : string name    - the person patching.
 *                string command - the function called.
 *                string patched - the patched object.
 * Returns      : string - the straight message.
 */
string
call_message(string name, string command, string patched)
{
    object caller = find_player(name);
    object pobj   = previous_object(-1);
    object in_ob  = find_object(patched);
    int    wiz    = pobj->query_wiz_level();
    string str;

    if (!wiz &&
	(!CAN_SEE(pobj, caller) ||
	 !CAN_SEE_IN_ROOM(pobj)))
    {
	return "";
    }
    
    if (!wiz)
    {
	if (extract(command, 0, 5) == "query_")
	{
	    if (pobj == in_ob)
		str = " przypatruje ci sie w skupieniu.\n";
	    else if (in_ob == caller)
		return "";
	    else if (living(in_ob))
		str = " przypatruje sie w skupieniu " +
		    in_ob->query_imie(pobj, PL_CEL) + ".\n";
	    else if (strlen(str = in_ob->short(pobj, PL_CEL)))
		str = " przypatruje sie w skupieniu " + str + ".\n";
	    else return "";
	}
	else
	{
	    if (pobj == in_ob)
		str = "ciebie";
	    else if (in_ob == caller)
		str = "siebie";
	    else if (living(in_ob))
		str = in_ob->query_imie(pobj, PL_BIE);
	    else if (!strlen(str = in_ob->short(pobj, PL_CEL)))
	        return "";

	    str = " rzuca na " + str + " jakies zaklecie.\n";
	}
    }
    else
    {
	str = " wywoluje na ";
	if (pobj == in_ob)
	    str += "tobie";
	else if (in_ob == caller)
	    str += "sobie";
	else if (living(in_ob))
	    str += in_ob->query_imie(pobj, PL_MIE);
	else
	    str += in_ob->short(pobj, PL_MIE);

	str += " funkcje '" + command + "'.\n";
    }

    return caller->query_Imie(pobj) + str;
}

/*
 * Cat - cat a file.
 *
 * Syntax   : Cat [<object>]
 * Arguments: <object> - the object to cat.
 * Default  : 'here'
 */
int
Cat(string str)
{
    object ob;

    if (!strlen(str))
    {
	str = "here";
	ob = environment(this_interactive());
	write(file_name(ob) + "\n");
    }
    else
    {
	ob = parse_list(str);
    }

    if (!objectp(ob))
    {
	notify_fail("Object '" + str + "' not found.\n");
	return 0;
    }

    str = MASTER_OB(ob) + ".c";
    if (!cat(str))
    {
	notify_fail("No read access to: " + str + "\n");
	return 0;
    }

    return 1;
}

/*
 * Clean - destruct all non-interactive objects in somethings inventory.
 *
 * Syntax   : Clean [<object>]
 * Arguments: <object> - the object to clean.
 * Default  : 'here'
 */
int
Clean(string str)
{
    object ob, *ob_list;
    string tmp;
    int    index;
    int    size;

    CHECK_SO_WIZ;

    if (!strlen(str))
    {
	ob = environment(this_interactive());
	write(file_name(ob) + "\n");
    }
    else
    {
	ob = parse_list(str);
    }

    if (!objectp(ob))
    {
	notify_fail("Clean what object?\n");
	return 0;
    }

    ob_list = all_inventory(ob);

    index = -1;
    size = sizeof(ob_list);

    if (!size)
    {
	write("No objects to destruct.\n");
	return 1;
    }

    while(++index < size)
    {
	if (query_ip_number(ob_list[index]))
	    continue;

	write("Destructed: " +
	    (stringp(tmp = ob_list[index]->short(this_interactive())) ?
	    capitalize(tmp) : file_name(ob_list[index])) + "\n");

	/* Try to remove it the easy way if possible. */
	ob_list[index]->remove_object();

	/* Destruct if the hard way if it still exists. */
	if (objectp(ob_list[index]))
	{
	    SECURITY->do_debug("destroy", ob_list[index]);
	}
    }

    say(QCIMIE(this_interactive(), PL_MIA) +
	" oczyszcza lokacje swa magia.\n");
    return 1;
}

/*
 * Destruct - destruct a certain object.
 *
 * Syntax   : Destruct [-D] <object>
 * Arguments: -D       - destruct the object with force.
 *            <object> - the object to destruct.
 */
int
Destruct(string str)
{
    object ob;
    int    dflag;
    string a;

    CHECK_SO_WIZ;

    if (!strlen(str))
    {
	notify_fail("Destruct what?\n");
	return 0;
    }

    if (sscanf(str, "-D %s", a) == 1)
    {
	dflag = 1;
	str = a;
    }

    ob = parse_list(str);
    if (!objectp(ob))
    {
	notify_fail("Object '" + str + "' not found.\n");
	return 0;
    }

    write("Trying to destruct: " +
	(strlen(str = ob->short(this_interactive())) ? str : "---") + "\n");
	
    if (interactive(ob))
            SECURITY->log_syslog("DESTRUCT", ctime(time()) + 
               ":  UID [" + getuid(this_interactive()) + "] EUID [" + 
               geteuid(this_interactive()) + "] -> " + 
               ob->query_name() + ".\n", 10000);
 
    if (dflag)
	SECURITY->do_debug("destroy", ob);

    if (objectp(ob))
	ob->remove_object();

    if (objectp(ob))
	SECURITY->do_debug("destroy", ob);

    write("Ok.\n");
    return 1;
}

/*
 * Dump - print information on an object.
 *
 * Syntax   : Dump <object> <flag>
 * Arguments: <object> - the object to dump.
 *            <flag>   - what to dump, this can take many forms.
 */
int
Dump(string str)
{
    int     i, query_list, ret;
    object  ob, *ob_list;
    string  flag, *props, path, tmp;
    mixed  *stores, *vars, *data;

    CHECK_SO_WIZ;

    vars = this_interactive()->query_prop(TRACER_VARS);
    stores = this_interactive()->query_prop(TRACER_STORES);

    if (!strlen(str))
    {
	write("All variables:\n");
	while (i < sizeof(vars))
	{
	    if (vars[i])
	    {
		write(vars[i] + ":\t");
		write(stores[i]);
		write("\n");
	    }
	    i += 1;
	}
	return 1;
    }

    if (sscanf(str, "%s %s", path, flag) != 2)
	path = str;
    ob = parse_list(path);
    if (!objectp(ob))
    {
	notify_fail("Object '" + path + "' not found.\n");
	return 0;
    }

    write(file_name(ob) + ":\n" + ob->short(this_interactive()));

    if (living(ob))
	write("(living) ");

    if (tmp = query_ip_name(ob))
	write("(interactive) '" + tmp + "' ");

    write("\n");

    if (stringp(tmp = creator(ob)))
	write("Creator:\t\t" + tmp + "\n");

    data = SECURITY->do_debug("get_variables", ob);
    vars = m_indices(data);
    
    if (member_array(flag, vars) > 0)
    {
	write(flag + " : ");
	print_value(data[flag]);
	write("\n");
    }

    if (flag == "list" || flag == "list!")
    {
	ob_list = all_inventory(ob);
	for (i = 0; i < sizeof(ob_list); i++)
	    write((i + 1) + ":\t" + RPATH(file_name(ob_list[i])) + "\t" +
		(string)ob_list[i]->short() + "\n");

	write("\n");
	return 1;
    }

    if (flag == "shadows" || flag == "shadows!")
    {
	while (ob = shadow(ob,0))
	    write(file_name(ob) + "\n");
	write("\n");
	return 1;
    }

    if (flag == "vars" || flag == "variables" || 
	flag == "vars!" || flag == "variables!")
    {
	for (i = 0; i < sizeof(vars); i++)
	{
	    write(sprintf(" %-30s : ", vars[i]));
	    print_value(data[vars[i]]);
	}
	write("\n");
	return 1;
    }

    if (flag == "inherit" || flag == "inherits" ||
	flag == "inherit!" || flag == "inherits!")
    {
	dump_array(SECURITY->do_debug("inherit_list", ob));

	write("\n");
	return 1;
    }

    if (flag == "props" || flag == "properties" || 
	flag == "props!" || flag == "properties!")
    {
	props = ob->query_props();
	for (i = 0; i < sizeof(props); i++)
	{
	    write(sprintf(" %-30s : ", props[i]));
	    print_value(ob->query_prop_setting(props[i]));
	}
	write("\n");
	return 1;
    }

    if (flag == "flags" || flag == "flags!")
    {
	write(SECURITY->do_debug("object_info", 0, ob));
	write("\n");
	return 1;
    }

    if (flag == "profile" || flag == "profile!")
    {
	write(implode(SECURITY->do_debug("getprofile", ob), "\n"));
	write("\n");
	return 1;
    }
	
    if (flag == "info" || flag == "info!")
    {
	write(SECURITY->do_debug("object_info", 1, ob));
	write("\n");
	return 1;
    }
	
    if (flag == "cpu" || flag == "cpu!")
    {
	write("Object CPU usage: " + SECURITY->do_debug("object_cpu", ob));
	write("\n");
	return 1;
    }
	
    if (flag == "functions" || flag == "functions!")
    {
	data = SECURITY->do_debug("functionlist", ob);
	for (i = 0; i < sizeof(data); i++)
	    write("    " + data[i] + "\n");
	write("\n");
	return 1;
    }

    if (flag == "alarms" || flag == "alarms!")
    {
        data = ob->query_alarms();
        for (i = 0; i < sizeof(data);  i++)
	{
	    write("Id        : " + data[i][0] + "\n"); 
	    write("Function  : " + data[i][1] + "\n");
	    write("Time Left : " + ftoa(data[i][2]) + "\n");
	    write("Repeat    : " + ftoa(data[i][3]) + "\n");
	    write("Arguments :\n");
	    dump_array(data[i][4]);
	    write("\n\n");
	}
	return 1;
    }
	
    return 1;
}

/*
 * Ed - ed a file.
 *
 * Syntax   : Ed [<object>]
 * Arguments: <object> - the object to ed.
 * Default  : 'here'
 */
int
Ed(string str)
{
    object ob;

    if (!strlen(str))
    {
	str = "here";
	ob = environment(this_interactive());
	write(file_name(ob) + "\n");
    }
    else
    {
	ob = parse_list(str);
    }

    if (!objectp(ob))
    {
	notify_fail("Object '" + str + "' not found.\n");
	return 0;
    }

    ed(MASTER_OB(ob) + ".c");
    return 1;
}

/*
 * Goto - go to another location.
 *
 * Syntax   : Goto <object>
 * Arguments: <object> - the object to go to.
 */
int
Goto(string str)
{
    object mark;

    CHECK_SO_WIZ;

    if (!strlen(str))
    {
	notify_fail("Goto which object?\n");
	return 0;
    }

    mark = parse_list(str);
    if (!objectp(mark))
    {
	notify_fail("Object '" + str+ "' not found.\n");
	return 0;
    }

    if (this_interactive()->move_living("X", mark))
    {
	write("Failed to Goto " + mark->short(this_interactive()) + ".\n");
	write("Maybe you wanted to go to its environment?\n");
    }

    return 1;
}

/*
 * In - perform a command in another location.
 *
 * Syntax   : In <object> <command>
 * Arguments: <object>  - the object to perform the command in.
 *            <command> - the command to perform in the object.
 */
int
In(string str)
{
    string path;
    string cmd;
    object ob;
    object old_ob;

    CHECK_SO_WIZ;

    if (!strlen(str) ||
	(sscanf(str, "%s %s", path, cmd) != 2))
    {
	notify_fail("What do you want to do where?\n");
	return 0;
    }

    ob = parse_list(path);
    if (!objectp(ob))
    {
	notify_fail("Object '" + path + "' not found.\n");
	return 0;
    }

    old_ob = environment(this_interactive());

    /* Only bother to command if we succeed to move. */
    if (!(this_interactive()->move(ob, 1)))
    {
	catch(this_interactive()->command(cmd));
	this_interactive()->move(old_ob, 1);
    }
    else
    {
	write("Failed to move you to that location.\n");
    }

    return 1;
}

/*
 * Info - get wizard information (OBJ_S_WIZINFO) on an object.
 *
 * Syntax   : Info [<object>]
 * Arguments: <object> - the object to get the information on.
 * Default  : 'here'
 */
int
Info(string str)
{
    object ob;
    string tmp;

    CHECK_SO_WIZ;

    if (!strlen(str))
    {
	ob = environment(this_interactive());
	write(file_name(ob) + "\n");
    }
    else
    {
	ob = parse_list(str);
    }

    if (!objectp(ob))
    {
	notify_fail("Info on which object?\n");
	return 0;
    }

    write("Info on: " +
	((stringp(tmp = ob->short(this_interactive()))) ? tmp : "---") + "\n");

    if (stringp(tmp = ob->query_prop(OBJ_S_WIZINFO)))
	write(tmp);
    else
	write("--- no OBJ_S_WIZINFO set ---\n");

    return 1;
}

/*
 * Inventory - list the inventory of an object
 * I (short for Inventory)
 *
 * Syntax   : Inventory [<object>]
 * Arguments: <object> - the object to print the inventory of.
 * Default  : 'me'
 */
int
Inventory(string str)
{
    object *inv, target;
    string  tmp;
    int     index;

    CHECK_SO_WIZ;

    if (!strlen(str))
    {
	target = this_interactive();
	write(file_name(target) + "\n");
    }
    else
    {
	target = parse_list(str);

	if (!objectp(target))
	{
	    notify_fail("I(nventory) of which object?\n");
	    return 0;
	}
    }

    inv = all_inventory(target);

    write("Inventory of: " +
	((stringp(tmp = target->short(this_interactive()))) ? tmp : "---") +
	"\n");

    for (index = 0; index < sizeof(inv); index++)
    {
	write(sprintf("%-30s (%-45s\n",
	    ((stringp(tmp = inv[index]->short(this_interactive()))) ?
	    tmp : "  ---"), (file_name(inv[index]) + ")")));
    }

    return 1;
}

/*
 * Items - see which add_items and add_cmd_items were added to an object.
 *
 * Syntax   : Items [<object>]
 * Arguments: <object> - the object to print the linked items on.
 * Default  : 'here'
 */
int
Items(string str)
{
    object  target;
    int     index;
    string *items = ({ });
    string *commands = ({ });
    mixed   tmp;

    CHECK_SO_WIZ;

    if (!strlen(str))
    {
	target = environment(this_interactive());
	write(file_name(target) + "\n");
    }
    else
    {
	target = parse_list(str);
    }

    if (!objectp(target))
    {
	notify_fail("Items of which object?\n");
	return 0;
    }

    write("Items of: " +
	((stringp(tmp = target->short(this_interactive()))) ? tmp : "---") +
	"\n");

    tmp = target->query_item();
    if (!sizeof(tmp))
    {
	write("--- no items listed ---\n");
    }
    else
    {
	for (index = 0; index < sizeof(tmp); index++)
	    items += tmp[index][0];

	write(sprintf("%-*#s\n", 74, implode(sort_array(items), "\n")));
    }

    tmp = target->query_cmd_item();
    if (!sizeof(tmp))
    {
	write("--- no command items listed ---\n");
    }
    else
    {
	items = ({ });

	for(index = 0; index < sizeof(tmp); index++)
	{
	    items += tmp[index][0];
	    items[sizeof(items) - 1] += " (" + (index + 1) + ")";
	    commands += tmp[index][1];
	    commands[sizeof(commands) - 1] += " (" + (index + 1) + ")";
	}

	write(sprintf("Commands:\n%-*#s\n", 74, implode(commands, "\n")));
	write(sprintf("Command items:\n%-*#s\n", 74, implode(items, "\n")));
    }

    return 1;
}

/*
 * Function name: light_status
 * Description  : Print the light status of one object and recurse over
 *                its inventory.
 * Arguments    : object target - the object to process.
 *                int    level  - the how deep the object is.
 */
static nomask void
light_status(object target, int level)
{
    object *inv    = all_inventory(target);
    int    c_light = target->query_prop(CONT_I_LIGHT);
    int    o_light = target->query_prop(OBJ_I_LIGHT);
    int    r_light = target->query_prop(ROOM_I_LIGHT);
    int    index   = -1;
    int    size    = sizeof(inv);
    string desc;

    if ((level == 0) || living(target) || size ||
	c_light || o_light || r_light)
    {
	if (!strlen(desc = target->short(this_player())))
	{
	    desc = RPATH(file_name(target));
	}

	if (level)
	{
	    write(extract(SPACES, 0, ((level * 3) - 1)));
	}

	write(sprintf("%-*s : %3d ", (60 - (level * 3)), desc,
	    o_light));

	if (size || c_light || r_light)
	{
	    if (c_light && r_light)
	    {
		write(sprintf("%4+d %4+d", c_light, r_light));
	    }
	    else if (c_light)
	    {
		write(sprintf("%4+d", c_light));
	    }
	    else if (r_light)
	    {
		write(sprintf("     %4+d", r_light));
	    }
	    else if (o_light)
	    {
		write("(transp.)");
	    }
	}

	write("\n");
    }

    while(++index < size)
    {
	light_status(inv[index], (level + 1));
    }
}

/*
 * Light - print light information on an object.
 *
 * Syntax   : Light [<object>]
 * Arguments: <object> - the object to get light information on.
 * Default  : 'here'
 */
int
Light(string str)
{
    object target;
    int    verbose;

    CHECK_SO_WIZ;

    if (!strlen(str))
    {
	target = environment(this_player());
	write(file_name(target) + "\n");
    }
    else
    {
	target = parse_list(str);
    }

    if (!objectp(target))
    {
	notify_fail("Light on which object?\n");
	return 0;
    }

    write(sprintf("Object: %-52s : OBJ CONT ROOM\n\n",
	RPATH(file_name(target))));

    light_status(target, 0);
    return 1;
}

/*
 * More - more a file.
 *
 * Syntax   : More [<object>]
 * Arguments: <object> - the object to display with more.
 * Default  : 'here'
 */
int
More(string str)
{
    object ob;
    string text;

    if (!strlen(str))
    {
	str = "here";
	ob = environment(this_interactive());
	write(file_name(ob) + "\n");
    }
    else
    {
	ob = parse_list(str);
    }

    if (!objectp(ob))
    {
	notify_fail("Object '" + str + "' not found.\n");
	return 0;
    }

    str = MASTER_OB(ob) + ".c";
    if (!(SECURITY->valid_read(str, geteuid(), 0)))
    {
	notify_fail("No read access to: " + str + "\n");
	return 0;
    }

    this_player()->more(str, 1);
    return 1;
}

/*
 * Move - move an object somewhere.
 *
 * Syntax: Move [-f] <object> [<destination>]
 * Arguments: -f            - move with force.
 *            <object>      - the object to move.
 *            <destination> - the destination to move the object to.
 * Default  : 'here' (destination)
 */
int
Move(string str)
{
    object ob;
    object to;
    string str_ob;
    string str_to;
    int    force = 0;

    CHECK_SO_WIZ;

    if (!strlen(str))
    {
	notify_fail("Syntax: Move [-f] <object> [<destination>]\n");
	return 0;
    }

    if (force = wildmatch("-f *", str))
    {
	str = extract(str, 3);
    }

    if (sscanf(str, "%s %s", str_ob, str_to) == 2)
    {
	to = parse_list(str_to);
    }
    else
    {
	str_ob = str;
	str_to = "here";
	to = environment(this_player());

	if (objectp(to))
	{
	    write(file_name(to) + "\n");
	}
    }

    if (!objectp(to))
    {
	notify_fail("Destination '" + str_to + "' not found.\n");
	return 0;
    }

    ob = parse_list(str_ob);
    if (!objectp(ob))
    {
	notify_fail("Object '" + str_ob + "' not found.\n");
	return 0;
    }

    if (force)
    {
	if (force = ob->move(to, 1))
	{
	    write("Error '" + force + "' moving " + ob->short() + " to " +
		to->short() + " with force.\n");
	}
	else
	{
	    write("Forcefully moved " + ob->short() + " to " + to->short() +
		".\n");
	}

	return 1;
    }

    if (living(ob))
    {
	if (force = ob->move_living("X", to, 1))
	{
	    write("Error '" + force + "' moving " + ob->short() + " to " +
		to->short() + " as living.\n");
	}
	else
	{
	    write("Moved " + ob->short() + " to " + to->short() +
		" as living.\n");
	    return 1;
	}
    }

    if (force = ob->move(to, 0))
    {
	write("Error '" + force + "' moving " + ob->short() + " to " +
	    to->short() + ".\n");
    }
    else
    {
	write("Moved " + ob->short() + " to " + to->short() + ".\n");
    }

    return 1;
}

/*
 * Set - set a variable to an object.
 *
 * Syntax   : Set <variable> <object>
 * Arguments: <variable> - the name of the variable.
 *            <object>   - the object to assign.
 */
int
Set(string str)
{
    object ob;
    string item, var;

    CHECK_SO_WIZ;

    if (!strlen(str) ||
	(sscanf(str, "%s %s", var, item) != 2))
    {
	notify_fail("Set variable to which object?\n");
	return 0;
    }

    ob = parse_list(item);
    if (!objectp(ob))
    {
	notify_fail("Object '" + item + "' not found.\n");
	return 0;
    }

    assign(var, ob);
    return 1;
}

/*
 * Tail - tail a file.
 *
 * Syntax   : Tail [<object>]
 * Arguments: <object> - the object to tail.
 * Default  : 'here'
 */
int
Tail(string str)
{
    object ob;

    if (!strlen(str))
    {
	str = "here";
	ob = environment(this_interactive());
	write(file_name(ob) + "\n");
    }
    else
    {
	ob = parse_list(str);
    }

    if (!objectp(ob))
    {
	notify_fail("Object '" + str + "' not found.\n");
	return 0;
    }

    str = MASTER_OB(ob) + ".c";
    if (!tail(str))
    {
	notify_fail("No read access to: " + str + "\n");
	return 0;
    }

    return 1;
}
