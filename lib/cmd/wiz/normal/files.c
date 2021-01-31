/*
 * /cmd/wiz/normal/files.c
 *
 * This is a subpart of /cmd/wiz/normal.c
 * 
 * The commands in this sub-part all have to do with the manipulation of
 * files and objects.
 *
 * Commands in this file:
 * - aft
 * - clone
 * - cp
 * - destruct
 * - distrust
 * - ed
 * - load
 * - mkdir
 * - mv
 * - odmien
 * - odnow
 * - remake
 * - rm
 * - rmdir
 * - trust
 * - update
 */

#include <cmdparse.h>
#include <filepath.h>
#include <language.h>
#include <options.h>

/*
 * Global variable.
 */
static private object  do_many_wizard;
static private int     do_many_operation;
static private string  do_many_target;
static private string *do_many_files;
static private string *do_many_going = ({ });

static private mapping aft_tracked;
static private mapping aft_current = ([ ]);
static private string *aft_sorted = ({ });

/*
 * Prototypes.
 */
nomask int update_ob(string str);
//private nomask int do_multi_files(string *source, int operation,
//				  string target, object wizard);

/*
 * Maxinum number of files that can be copied/moved/removed/etc with one
 * command. Protects from errors like 'cp /std/* .'
 */
#define DO_MANY_MAX	({ 10, 15, 30, 5, 20 })
#define DO_MANY_DELAY	(9.0)

#define MULTI_CP	0
#define MULTI_MV	1
#define MULTI_RM	2
#define MULTI_LOAD	3
#define MULTI_UPDATE	4
#define MULTI_OPERATION ({ "Copying", "Moving", "Removing", "Loading", "Updating" })


/*
 * Function name: copy
 * Description  : Copy a (text) file. Limited to about 50kB files by the GD.
 *                It could be circumvented but very few files are larger
 *                than that. Maybe I should make this into an simul-efun?
 * Arguments    : string path1 - source path.
 *                string path2 - destination path (including filename).
 * Returns      : int 1/0 - success/failure.
 */
private nomask int
copy(string path1, string path2)
{
    string buf;

    /* Read the source file and test for success. */
    buf = read_file(path1);
    if (!strlen(buf))
    {
	return 0;
    }

    switch(file_size(path2))
    {
	/* Target is a directory. Impossible. */
	case -2:
	    return 0;

	/* Target file does not exist or is empty. Proceed. */
	case -1:
	case  0:
	    break;

	/* Existing target file. Try to remove it first. */
	default:
	    if (!rm(path2))
	    {
		return 0;
	    }
    }

    /* Write the buffer and return the return value of the efun. */
    return write_file(path2, buf);
}



/*
 * Function name: do_many_delayed
 * Description  : When a wizard wants to test many files in one turn, this
 *                function is called by the alarm to prevent the system
 *                from slowing down too much and to prevent 'evaluation too
 *                long' types of errors.
 * Arguments    : object wizard - the wizard handling the object.
 *                string *files - the files to do still.
 */
static nomask void
do_many_delayed(object wizard, string *files, int operation, string target = 0)
{
    do_many_wizard = wizard;

    if (!objectp(wizard) ||
	(member_array(wizard->query_real_name(), do_many_going) == -1) ||
    	(!interactive(wizard)))
    {
    	return;
    }

    do_many_wizard = wizard;
    do_many_operation = operation;
    do_many_target = target;
    do_many_files = files;

    wizard->reopen_soul();
}

/*
 * Function name: do_many
 * Description  : When a wizard wants to test many files in one turn, this
 *                function tests only a few (DO_MANY_MAX to be exact) and
 *                then calls an alarm to test the other files.
 */
static nomask void
do_many()
{
    int    index = -1;
    int    size, done;
    object obj;
    string str, *parts;

    size = min(DO_MANY_MAX[do_many_operation], sizeof(do_many_files));
    
    while(++index < size)
    {
	switch(do_many_operation)
	{
	case MULTI_CP:
	    if (file_size(do_many_files[index]) == -2)
	    {
		tell_object(do_many_wizard,
		    sprintf("Unable to copy directory %s.\n",
			do_many_files[index]));
		continue;
	    }

	    str = do_many_target;
	    if (file_size(str) == -2)
	    {
		parts = explode(do_many_files[index], "/");
		str += "/" + parts[sizeof(parts) - 1];
	    }

	    if (copy(do_many_files[index], str))
	    {
		tell_object(do_many_wizard,
		    sprintf("Copied:           %s.\n", do_many_files[index]));
		continue;
	    }
	    else
	    {
		tell_object(do_many_wizard,
		    sprintf("Copying failed at: %s.\n",
			do_many_files[index]));
	    }
	    continue;
	case MULTI_MV:
	    if ((file_size(do_many_files[index]) == -2) &&
		(size > 1))
	    {
		tell_object(do_many_wizard, sprintf("When moving " +
		    "directory %s the directory must be the only argument.\n",
		    do_many_files[index]));
		continue;
	    }

	    str = do_many_target;
	    if (file_size(str) == -2)
	    {
		parts = explode(do_many_files[index], "/");
		str += "/" + parts[sizeof(parts) - 1];
	    }

	    if (rename(do_many_files[index], str))
	    {
		tell_object(do_many_wizard,
		    sprintf("Moved:            %s.\n", do_many_files[index]));
		continue;
	    }
	    else
	    {
		tell_object(do_many_wizard,
		    sprintf("Moving failed at: %s.\n",
			do_many_files[index]));
	    }
	    continue;

	case MULTI_RM:
	    if (file_size(do_many_files[index]) == -2)
	    {
		tell_object(do_many_wizard,
		    "You cannot remove a directory using 'rm'.\n");
		continue;
	    }

	    if (rm(do_many_files[index]))
	    {
		tell_object(do_many_wizard,
		    sprintf("Removed:          %s.\n", do_many_files[index]));
		continue;
	    }
	    else
	    {
		tell_object(do_many_wizard,
		    sprintf("Removing failed at %s.\n", do_many_files[index]));
		continue;
	    }
	    break;
	case MULTI_LOAD:
	    if (objectp(find_object(do_many_files[index])) &&
		do_many_wizard->query_option(OPT_ECHO))
	    {
		/* If the object is already in memory, destruct it. */
		if (objectp(obj = find_object(do_many_files[index])))
		{
		    write("Trying to update: " + do_many_files[index] + "\n");

 		    if (!update_ob(do_many_files[index]))
		    {
			write("Updating failed...\n");
			continue;
		    }
		}
	    }
	    
	    if (catch(call_other(do_many_files[index], "teleledningsanka")))
	    {
		tell_object(do_many_wizard,
		    sprintf("Error loading:    %s.\n", do_many_files[index]));
		continue;
	    }

	    if (do_many_wizard->query_option(OPT_ECHO))
		tell_object(do_many_wizard,
		    sprintf("Loaded:           %s.\n", do_many_files[index]));
	    break;
	case MULTI_UPDATE:
	    if (update_ob(do_many_files[index]))
	        tell_object(do_many_wizard,
		    sprintf("Updated:          %s\n", do_many_files[index]));
	    else
	        tell_object(do_many_wizard,
		    sprintf("Updating failed:  %s\n", do_many_files[index]));
	        
	    break;
	}
    }

    if (sizeof(do_many_files) > size)
    {
	
	set_alarm(DO_MANY_DELAY, 0.0, &do_many_delayed(do_many_wizard,
	    do_many_files[size..], do_many_operation, do_many_target));
    }
    else
    {
 	tell_object(do_many_wizard, "Done.\n");
 	do_many_going -= ({ do_many_wizard->query_real_name() });
    }

    do_many_files = 0;
    do_many_wizard = 0;
    do_many_operation = 0;
    do_many_target = 0;
}

/*
 * Function name: do_many_delayed_reloaded
 * Description  : After an alarm this object looses its euid, so we have
 *                to reopen the soul. This function could be integrated
 *                with do_many() itself, but I decided to separate them
 *                in order to make do_many() a static function.
 */
public nomask void
do_many_delayed_reloaded()
{
    if ((geteuid(previous_object()) != geteuid()) ||
    	(!interactive(previous_object())) ||
    	(calling_function() != REOPEN_SOUL))
    {
    	do_many_files = 0;
    	do_many_going -= ({ do_many_wizard->query_real_name() });
    	do_many_wizard = 0;
    	do_many_target = 0;
    	do_many_operation = 0;
    	return;
    }

    set_this_player(do_many_wizard);
    do_many();
}
 
static nomask int
prepare_many(string str, int operation)
{
    object obj;
    string *parts, *files, *ret, *dir, target;
    int size, index;

    CHECK_SO_WIZ;

    if (!strlen(str))
    {
	notify_fail("Invalid file name.\n");
	return 0;
    }

    if (str == "stop")
    {
    	if (member_array(this_player()->query_real_name(),
    	    do_many_going) == -1)
    	{
	    notify_fail("Nie wykonujesz w tej chwili zadnej operacji " +
		"na plikach.\n");
    	    return 0;
    	}

	do_many_going -= ({ this_player()->query_real_name() });
    	write(MULTI_OPERATION[operation] + " stopped.\n");
    	return 1;
    }
    
    files = explode(str, " ") - ({ "" });
    size = sizeof(files);
    if (!size)
    {
	notify_fail(sprintf("Komenda %s wymaga podania argumentow.\n",
	    query_verb()));
	return 0;
    }
    
    /* If wildcards are used, the wizard means to check many files. */
    if (wildmatch("*[\\*\\?]*", files[0]))
    {
	if (member_array(this_player()->query_real_name(),
	    do_many_going) != -1)
	{
	    notify_fail(sprintf("Juz wykonujesz dzialanie operujace " +
		"na kilku plikach naraz. Jesli chcesz przerwac aktualnie " +
		"wykonywana sekwencje wpisz '%s stop'." +
		(((operation == MULTI_LOAD) || (operation == MULTI_CP))
		? " Pamietaj, ze operacje ladowania i kopiowania obiektow " +
		"dosc powaznie obciazaja zasoby systemowe. Powinno sie " +
		"z nich korzystac jak najoszczedniej." : "") + "\n",
		query_verb()));
	    return 0;
	}
    }

    str = this_interactive()->query_path();
    index = -1;

    while(++index < size)
    {
	files[index] = FTPATH(str, files[index]);
    }
    
    /* For cp and mv we need at least two arguments, mark the target. */
    if ((operation == MULTI_CP) || (operation == MULTI_MV))
    {
	if (size == 1)
	{
	    notify_fail(sprintf(
		"Syntax: %s <source> <target>\n", query_verb()));
	    return 0;
	}

	target = files[--size];
	files -= ({ target });
    }

    /* Expand the wildcards. */
    index = -1;
    ret = ({ });
    while(++index < size)
    {
	parts = explode(files[index], "/");
	str = implode(parts[..(sizeof(parts) - 2)], "/") + "/";

	if (!sizeof((dir = get_dir(files[index]))))
	{
	    if (!sizeof((dir = get_dir(files[index] + ".c"))))
		return notify_fail(sprintf(
		    "%s: No such file or directory.\n", files[index]));
	    files[index] += ".c";
	}
	
	ret += map(dir - ({ ".", ".." }), &operator(+)(str));
    }

    if ((operation == MULTI_LOAD) || (operation == MULTI_UPDATE))
    {
	size = sizeof(ret);
	index = -1;
	while (++index < size)
	{
	    if (!wildmatch("*.c", ret[index]))
	    {
		if ((file_size(ret[index] + ".c") <= 0) ||
		    (member_array(ret[index] + ".c", ret) != -1))
		{
		    ret = exclude_array(ret, index, index);
		    size--;
		    index--;
		    continue;
		}
		ret[index] += ".c";
	    }
	}

	if (!sizeof(ret))
	{
	    write("No files matching.\n");
	    return 1;
	}
    }

    str = FTPATH(this_interactive()->query_path(), str);
    size = sizeof(ret);

    write(sprintf("%s %d file%s.%s\n", MULTI_OPERATION[operation],
	size, ((size == 1) ? "" : "s"), ((size > DO_MANY_MAX[operation])
	? sprintf(" A delay of %d seconds is used each %d files.",
	    ftoi(DO_MANY_DELAY), DO_MANY_MAX[do_many_operation])
	: "")));


    do_many_files = ret;
    do_many_wizard = this_interactive();
    do_many_going += ({ this_player()->query_real_name() });
    do_many_operation = operation;
    do_many_target = target;

    do_many();
    return 1;
}

/* **************************************************************************
 * aft - [Avernir's] file tracker
 */

/*
 * AFT uses disk-caching and the files are stored in the wizards personal
 * directory. The data type of the save-file is as follows. The information
 * of this file is stored in aft_tracked.
 *
 * ([
 *    (string) path : ({
 *                       (string) private name,
 *                       (int) last modification time,
 *                    }),
 * ])
 *
 * The current file (last tracked) for each wizard is stored in aft_current,
 * with the following data type. This structure is not saved.
 *
 * ([
 *    (string) name : (int) index of last tracked file,
 * ])
 */

#define AFT_PRIVATE_NAME (0)
#define AFT_FILE_TIME    (1)
#define AFT_FILE         ("/.aft")

/*
 * Function name: read_aft_file
 * Description  : This function reads the aft-file of a particular wizard.
 *                If such a file does not exist, the mapping is cleared.
 */
private void
read_aft_file()
{
    string name = this_interactive()->query_real_name();

    aft_tracked = read_cache(SECURITY->query_wiz_path(name) + AFT_FILE);

    /* No such file, set to defaults. */
    if (!m_sizeof(aft_tracked))
    {
	aft_tracked = ([ ]);
	aft_sorted = ({ });
    }
    else
    /* Sort the names in the mapping. */
    {
	aft_sorted = sort_array(m_indices(aft_tracked));
    }
}

/*
 * Function name: save_aft_file
 * Description  : This function saves the aft-file of a particular wizard.
 *                When empty, the save-file is deleted.
 */
private void
save_aft_file()
{
    /* No file being tracked, remove the file completely. */
    if (!m_sizeof(aft_tracked))
    {
	/* We do not need to appent the .o as that is done in rm_cache(). */
	rm_cache(SECURITY->query_wiz_path(
	    this_interactive()->query_real_name()) + AFT_FILE);
	return;
    }

    save_cache(aft_tracked, (SECURITY->query_wiz_path(
	this_interactive()->query_real_name()) + AFT_FILE));
}

/*
 * Function name: aft_find_file
 * Description  : This function will return the full path of the file the
 *                wizard wants to handle. It accepts the path (including
 *                tilde, the private name of the file and the number in the
 *                list of files.
 * Arguments    : string file - the file to find.
 * Returns      : string - path of the file found.
 */
private string
aft_find_file(string file)
{
    int index;
    mapping tmp;

    /* May be the index number in the list of files. */
    if (index = atoi(file))
    {
	if ((index > 0) &&
	    (index <= sizeof(aft_sorted)))
	{
	    return aft_sorted[index - 1];
	}

	/* No such index. Maybe it is a name, who knows ;-) */
    }

    /* May be a private name the wizard assigned to the file. This filter
     * statement will return a mapping with only one element if the private
     * name was indeed found.
     */
    tmp = filter(aft_tracked,
	&operator(==)(file, ) @ &operator([])(, AFT_PRIVATE_NAME));
    if (m_sizeof(tmp))
    {
	return m_indices(tmp)[0];
    }

    /* Could be the path itself. */
    file = FTPATH(this_interactive()->query_path(), file);
    if (pointerp(aft_tracked[file]))
    {
	return file;
    }

    /* File not found. */
    return 0;
}

/*
 * Function name: aft_catchup_file
 * Description  : Map-function called to update the time a (log) file was
 *                last accessed. For convenience we use time() here and not
 *                the actual file time. This is good enough anyway, because
 *                if the file is going to be changed, it is going to be
 *                changed after the current time().
 * Arguments    : mixed data - the array-value for a particular file-name.
 * Returns      : mixed - the same array with the time adjusted.
 */
private mixed
aft_catchup_file(mixed data)
{
    data[AFT_FILE_TIME] = time();

    return data;
}

public int
aft(string str)
{
    string *args;
    int    size;
    int    index = -1;
    int    flag = 0;
    int    changed;
    string name = this_interactive()->query_real_name();
    string *files;
    mapping tmp;

    CHECK_SO_WIZ;

    /* Set to default when there is no argument. */
    if (!strlen(str))
    {
	str = "lu";
    }

    args = explode(str, " ");
    size = sizeof(args);

    /* Read the wizards aft-file. */
    read_aft_file();

    /* Wizard is not tracking any files and does not want to select a file
     * to track either.
     */
    if (!m_sizeof(aft_tracked) &&
	args[0] != "s")
    {
	write("You are not tracking any files.\n");
	return 1;
    }

    switch(args[0])
    {
    case "c":
	if (size != 2)
	{
	    notify_fail("Syntax: aft c <file>\n");
	    return 0;
	}

	str = aft_find_file(args[1]);
	if (!stringp(str))
	{
	    notify_fail("You are not tracking a file \"" + args[1] + "\".\n");
	    return 0;
	}

	/* Mark the file as being up to date and make it current. */
	aft_tracked[str] = aft_catchup_file(aft_tracked[str]);
	aft_current[name] = member_array(str, aft_sorted);
	save_aft_file();

	write("Caught up on " + str + ".\n");
	return 1;

    case "C":
	if (size != 1)
	{
	    notify_fail("Syntax: aft C\n");
	    return 0;
	}

	/* Mark all files as being up to date. */
	aft_tracked = map(aft_tracked, aft_catchup_file);
	save_aft_file();

	write("Caught up on all files.\n");
	return 1;

    case "l":
	flag = 1;
	/* Continue at "lu". */

    case "lu":
	if (size != 1)
	{
	    notify_fail("Syntax: aft " + args[0] + "\n");
	    return 0;
	}

	/* Loop over all files being tracked. */
	size = sizeof(aft_sorted);
	while(++index < size)
	{
	    changed = (file_time(aft_sorted[index]) >
		aft_tracked[aft_sorted[index]][AFT_FILE_TIME]);
	    /* Only print if the file actually changed, or if the wizard
	     * signalled that he wanted all files.
	     */
	    if (flag || changed)
	    {
		write(sprintf("%2d %-10s%-1s %-50s %8d\n",
		    index + 1,
		    aft_tracked[aft_sorted[index]][AFT_PRIVATE_NAME],
		    index == aft_current[name] ? ">" : (changed ? "*" : ":"),
		    aft_sorted[index],
		    file_size(aft_sorted[index])));

		args[0] = "oke";
	    }
	}

	/* No output of any files. Give him a "fail" message. */
	if (args[0] != "oke")
	{
	    write("No changes in any of the tracked files.\n");
	}

	return 1;

    case "r":
	flag = 1;
	/* Continue at "rr". */

    case "rr":
	switch(size)
	{
	case 1:
	    /* user wants to see next changed file. Loop over all files,
	     * starting at the current file.
	     */
	    index = aft_current[name] - 1;
	    size = sizeof(aft_sorted);
	    while(++index < (size + aft_current[name]))
	    {
		/* If there is a change, break. */
		if (file_time(aft_sorted[index % size]) >
		    aft_tracked[aft_sorted[index % size]][AFT_FILE_TIME])
		{
		    index %= size;
		    args[0] = "oke";
		    break;
		}
	    }

	    /* No change to any files. Give him a "fail" message. */
	    if (args[0] != "oke")
	    {
		write("No changes in any of the tracked files.\n");
		return 1;
	    }

	    /* Add the found file to the list of arguments. */
	    args += ({ aft_sorted[index] });
	    break;

	case 2:
	    /* user specified a file to read. */
	    str = aft_find_file(args[1]);
	    if (!stringp(str))
	    {
		notify_fail("You are not tracking a file \"" + args[1] +
		    "\".\n");
		return 0;
	    }

	    args[1] = str;
	    break;

	default:
	    notify_fail("Syntax: aft " + args[0] + " [<file>]\n");
	    return 0;
	}

	/* Mark as read and file to current. Then save. */
	aft_current[name] = member_array(args[1], aft_sorted);
	aft_tracked[args[1]] = aft_catchup_file(aft_tracked[args[1]]);
	save_aft_file();

	write("AFT on " + args[1] + "\n");
	/* Force the wizard to use the tail command. We can force since we
	 * have his/her euid.
	 */
	return this_interactive()->command("tail " + (flag ? "" : "-r ") +
	    args[1]);
	/* not reached */

    case "s":
	switch(size)
	{
	case 2:
	    /* User does not want a private name. */
	    args += ({ "" });
	    break;

	case 3:
	    /* Specified a private name. See whether it is not a duplicate. */
	    tmp = filter(aft_tracked,
		&operator(==)(args[2], ) @ &operator([])(, AFT_PRIVATE_NAME));
	    if (m_sizeof(tmp))
	    {
		notify_fail("Name \"" + args[2] + "\" already used for " +
		    m_indices(tmp)[0] + ".\n");
		return 0;
	    }

	    break;

	default:
	    notify_fail("Syntax: aft s <path> [<name>]\n");
	    return 0;
	}

        args[1] = FTPATH(this_interactive()->query_path(), args[1]);
	if (aft_tracked[args[1]])
	{
	    notify_fail("You are already tracking " + args[1] + ".\n");
	    return 0;
	}

	if (file_size(args[1]) < 0)
	{
	    notify_fail("There is no file " + args[1] + ".\n");
	    return 0;
	}

	/* Add the file, and mark as unread. Then save. */
	aft_tracked[args[1]] = ({ args[2], 0 });
	aft_sorted = sort_array(m_indices(aft_tracked));
	save_aft_file();

	write("Started tracking on " + args[1] + ".\n");
	return 1;

    case "u":
	if (size != 2)
	{
	    notify_fail("Syntax: aft u <file>\n");
	    return 0;
	}

	str = aft_find_file(args[1]);
	if (!stringp(str))
	{
	    notify_fail("You are not tracking a file \"" + args[1] + "\".\n");
	    return 0;
	}

	if (member_array(str, aft_sorted) >= aft_current[name])
	{
	    aft_current[name] -= 1;
	}

	aft_tracked = m_delete(aft_tracked, str);
	aft_sorted -= ({ str });
	save_aft_file();

	write("Unselected file " + str + ".\n");
	return 1;

    case "U":
	aft_tracked = ([ ]);
        aft_current = m_delete(aft_current,
	    this_interactive()->query_real_name());
	aft_sorted = ({ });
	save_aft_file();

	write("Unselected all files. Stopped all tracking.\n");
	return 1;

    default:
	notify_fail("No subcommand \"" + args[0] + "\" to aft.\n");
	return 0;
    }

    write("Impossible end of aft switch. Please report to an archwizard!\n");
    return 1;
}

/* **************************************************************************
 * clone - clone an object
 */

/*
 * Function name: clone_message
 * Description  : This function returns the proper message to be displayed
 *                to people watching the cloning. Wizards get a message,
 *                mortal players see 'something'.
 * Arguments    : mixed cloned - the file_name with object number of the
 *                               object cloned.
 * Returns      : string - the description.
 */
public nomask string
clone_message(mixed cloned)
{
    string str;
    object proj = previous_object(-1);

    if (!(proj->query_wiz_level()))
        return "cos";

    if ((!stringp(cloned)) || (!strlen(cloned)))
        return "cos";
    if (!objectp(cloned = find_object(cloned)))
        return "cos";

    if (living(cloned))
        return (string)cloned->query_imie(proj, PL_BIE);

    return (strlen(str = (string)cloned->short(proj, PL_BIE)) ?
        str : file_name(cloned));
}

/*
 * Function name: clone_ob
 * Description  : This function actually clones the object a wizard wants
 *                to clone.
 * Arguments    : string what - the filename of the object to clone.
 * Returns      : object - the object cloned.
 */
static nomask object
clone_ob(string what)
{
    string str, mess;
    object ob;

    str = FTPATH((string)this_interactive()->query_path(), what);
    if (!strlen(str))
    {
	notify_fail("Invalid file.\n");
	return 0;
    }

    if (file_size(str + ".c") < 0 && file_size(str) < 0)
    {
	notify_fail("No such file.\n");
	return 0;
    }
    
    ob = clone_object(str);
    if (!ob)
    {
	notify_fail("You can not clone: " + str + "\n");
	return 0;
    }
    say(QCIMIE(this_interactive(), PL_MIA) + " wydobywa @@clone_message:" +
	file_name(this_object()) + "|" + file_name(ob) +
        "@@ z innego wymiaru.\n");
    return ob;
}

nomask int
clone(string str)
{
    object ob;
    int num, argc;
    string *argv;

    CHECK_SO_WIZ;

    if (!stringp(str))
    {
	notify_fail("Clone what object ?\n");
	return 0;
    }

    argv = explode(str, " ");
    argc = sizeof(argc);

    switch (argv[0])
    {
    case "-i":
	ob = clone_ob(argv[1]);
	if (!ob)
	    return 0;
	ob->move(this_interactive(), 1);
	write("Ok.\n");
	break;

    case "-e":
	ob = clone_ob(argv[1]);
	if (!ob)
	    return 0;
	ob->move(environment(this_interactive()), 1);
	write("Ok.\n");
	break;

    default:
	ob = clone_ob(argv[0]);
	if (!ob)
	    return 0;

	num = (int)ob->move(this_interactive());
	switch (num)
	{
	case 0:
	    write("Ok.\n");
	    break;
	    
	case 1:
	    write("Too heavy for destination.\n");
	    break;
	    
	case 2:
	    write("Can't be dropped.\n");
	    break;
	    
	case 3:
	    write("Can't take it out of it's container.\n");
	    break;
	    
	case 4:
	    write("The object can't be inserted into bags etc.\n");
	    break;
	    
	case 5:
	    write("The destination doesn't allow insertions of objects.\n");
	    break;
	    
	case 6:
	    write("The object can't be picked up.\n");
	    break;
	    
	case 7:
	    write("Other (Error message printed inside move() function).\n");
	    break;
	    
	case 8:
	    write("Too big volume for destination.\n");
	    break;
	    
	default:
	    write("Strange, very strange error in move: " + num + "\n");
	    break;
	}
	if (num)
	    num = (int)ob->move(environment(this_interactive()));
	break;
    }
    return 1;
}

/* **************************************************************************
 * cp - copy multiple files
 */
nomask int
cp_cmd(string str)
{
    return prepare_many(str, MULTI_CP);
}

/* **************************************************************************
 * destruct - destruct an object
 */
nomask int
destruct_ob(string str)
{
    object *oblist;
    int    dflag;

    CHECK_SO_WIZ;

    if (!stringp(str))
    {
	notify_fail("Destruct what?\n");
	return 0;
    }

    if (sscanf(str, "-D %s", str) == 1)
    {
	dflag = 1;
    }

    if (!parse_command(str, environment(this_interactive()), "[the] %i",
	oblist))
    {
	notify_fail("Destruct what?\n");
	return 0;
    }

    oblist = NORMAL_ACCESS(oblist, 0, 0);
    if (!sizeof(oblist))
    {
	notify_fail("Destruct what?\n");
	return 0;
    }

    if (sizeof(oblist) > 1)
    {
	notify_fail("You can destruct only one object at a time.\n");
	return 0;
    }
    
    if (interactive(oblist[0]))
    {
        SECURITY->log_syslog("DESTRUCT", ctime(time()) + 
           ":  UID [" + getuid(this_interactive()) + "] EUID [" + 
           geteuid(this_interactive()) + "] -> " + 
           oblist[0]->query_name() + ".\n", 10000);
    }

    if (living(oblist[0]))
    {
        say(QCIMIE(oblist[0], PL_MIA) + " zostaje zmiecion"
          + oblist[0]->koncowka("y", "a", "e") + " z powierzchni ziemi przez "
          + QIMIE(this_interactive(), PL_BIE) + ".\n");
	if (this_player()->query_option(OPT_ECHO))
	    write("Destructed " + oblist[0]->query_the_name(this_player()) +
		  " (" + RPATH(MASTER_OB(oblist[0])) + ").\n");
	else
	    write("Ok.\n");
    }
    else
    {
        say(QCIMIE(this_interactive(), PL_MIA) + " dematerializuje "
          + QSHORT(oblist[0], PL_BIE) + ".\n");
	if (this_player()->query_option(OPT_ECHO))
	    write("Destructed " + LANG_THESHORT(oblist[0]) + " (" +
		  RPATH(MASTER_OB(oblist[0])) + ").\n");
	else
	    write("Ok.\n");
    }

    if (dflag)
    {
	SECURITY->do_debug("destroy", oblist[0]);
    }
    else
    {
	oblist[0]->remove_object();
    }

    return 1;
}

/* **************************************************************************
 * distrust - distrust an object
 */
nomask int
distrust(string str)
{
    object ob;

    CHECK_SO_WIZ;

    if (!str)
    {
	notify_fail("Distrust what object?\n");
	return 0;
    }

    ob = present(str, this_interactive());

    if (!ob)
	ob = present(str, environment(this_interactive()));

    if (!ob)
	ob = parse_list(str);

    if (!ob) 
    {
	notify_fail("Object not found: " + str + "\n");
	return 0;
    }

    if (geteuid(ob) != geteuid(this_object()))
    {
	notify_fail("Object not trusted by you.\n");
	return 0;
    }

    /* Remove the previous euid */
    ob->set_trusted(0);

    write("Ok.\n");
    return 1;
}

/* **************************************************************************
 * du - calculate disk usage
 */
static nomask int xdu(string p, int af);

nomask int
du(string str)
{
    int aflag;
    string p, flag, path;
    
    if (!str)
    {
	path = ".";
    }
    else
    {
	/* There is no getopt in CDlib... or? */
	if (sscanf(str, "%s %s", flag, path) == 2)
	{
	    if (flag != "-a")
	    {
		notify_fail("usage: du [-a] [path]\n");
		return 0;
	    }
	    else
		aflag = 1;
	}
	else
	{
	    if (str == "-a")
	    {
		aflag = 1;
		path = ".";
	    }
	    else
		path = str;
	}
    }
    p = FTPATH(this_interactive()->query_path(), path);
    
    if (p == "/")
	p = "";
    
    xdu(p, aflag);

	return 1;
}

static nomask int
xdu(string path, int aflag)
{
    int sum, i;
    string *files, output;
    
    files = get_dir(path + "/*");
    
    sum = 0;
    
    for (i = 0; i < sizeof(files); i++)
    {
	if (files[i] == "." || files[i] == "..")
	    continue;
	
	if (aflag && file_size(path + "/" + files[i]) > -1)
	{
	    write(file_size(path + "/" + files[i]) / 1024 + "\t" +
		  path + "/" + files[i] + "\n");
	}

	if (file_size(path + "/" + files[i]) == -2)
	    sum += xdu(path + "/" + files[i], aflag);
	else
	    sum += file_size(path + "/" + files[i]);
    }
    
    write(sum / 1024 + "\t" + path + "\n");
    return sum;
}

/* **************************************************************************
 * ed - edit a file
 */
nomask int 
ed_file(string file)
{
    CHECK_SO_WIZ;

    if (!stringp(file))
    {
	ed();
	return 1;
    }
    file = FTPATH((string)this_interactive()->query_path(), file);
    ed(file);
    return 1;
}

/* **************************************************************************
 * load - load a file
 */

nomask int
load(string str)
{
    object obj;

    if (!str || wildmatch("*[\\*\\?]*", str) || str == "stop")
	return prepare_many(str, MULTI_LOAD);

    str = FTPATH((string)this_interactive()->query_path(), str);

    /* File does not exists. */
    if ((file_size(str + ".c") < 0) &&
    	(file_size(str) < 0))
    {
	notify_fail("No such file.\n");
	return 0;
    }

    /* If the object is already in memory, destruct it. */
    if (objectp(obj = find_object(str)))
    {
	write("Trying to update: " + str + "\n");

    	if (!update_ob(str))
    	{
    	    write("Updating failed...\n");
    	    return 0;
    	}
    }

    if (catch(str->teleledningsanka()))
    {
    	write("Error loading: " + str + "\n");
	return 1;
    }
    
    if (this_player()->query_option(OPT_ECHO))
	write("Loaded: " + str + "\n");
    else
	write("Ok.\n");
    return 1;
}

/* **************************************************************************
 * mkdir - make a directory
 */
nomask int
makedir(string str)
{
    CHECK_SO_WIZ;

    if (!stringp(str))
    {
	notify_fail("Make what dir?\n");
	return 0;
    }
    if (mkdir(FTPATH((string)this_interactive()->query_path(), str)))
	write("Ok.\n");
    else
	write("Fail.\n");
    return 1;
}


/* **************************************************************************
 * mv - move multiple files or a single directory.
 */
nomask int
mv_cmd(string str)
{
    return prepare_many(str, MULTI_MV);
}

/* **************************************************************************
 * odmien - podstaw odmiany w podanym pliku.
 */
nomask int
odmien(string str)
{
    string *odmiana, sciezka, *pliki, c, nazwa, to_insert, indent_str;
    int size, tmp_size, elem_len, ix, cx, linia, begin, end, indent, left,
	newline, mark_flag;

    CHECK_SO_WIZ;

    if (!strlen(str))
    {
        write("Musisz podac nazwe pliku.\n");
        return 1;
    }
    
    /* Jaka obiekt mial uzytkownik na mysli? Dopuszczamy gwiazdki,
     * ale tylko wskazujace na jeden obiekt.
     */
    sciezka = FTPATH(this_player()->query_path(), str);
    if (sciezka[-2..-1] != ".c")
	sciezka += ".c";
    pliki = get_dir(sciezka);
    size = sizeof(pliki);
    if (!size)
    {
        write("Nie moge znalezc pliku '" + sciezka + "'.\n");
	return 1;
    }
    if (size > 1)
    {
	write("Mozesz odmienic tylko jeden plik naraz.\n");
	return 1;
    }
    
    sciezka = implode(explode(sciezka, "/")[0..-2], "/");
    sciezka += "/" + pliki[0];

    if (!SECURITY->valid_write(sciezka, this_player(), 0))
    {
        write("Niestety nie masz praw zapisu do pliku '" + sciezka + "'.\n");
        return 1;
    }

    /* Dobrze pokazac, ktory obiekt zmienimy.
     */
    write(sciezka + "\n");
    
    c = read_file(sciezka);
    size = strlen(c);

    /* Petla glowna, az do konca pliku - znak po znaku.
     */
    ix = -1; begin = -1; end = 0;
    while(++ix < size)
    {
	/* Szukamy charakterystycznych znakow - konca lini, podkreslen
	 * (zaznaczajacych poczatek i koniec nazw do odmiany), oraz
	 * nawiasow. Zarowno znaki konca lini, jak i nawiasy resetuja
	 * szukanie parujacego podkreslenia - nazwy funkcji i zmiennych
	 * rowniez moga miec podkreslenia.
	 */
	if (c[ix] == '\n')
	{
	    linia++;
	    newline = ix;
	    begin = -1;
	    continue;
	}
	else if (c[ix] == '(')
	{
	    begin = -1;
	    continue;
	}
	else if (c[ix] != '_')
	    continue;
	if (++ix < size)
	{
	     if (c[ix] != '_')
	     {
	         begin = -1;
	         continue;
	     }
	}
	else
	    continue;
	
	if (begin == -1)
	{
	    begin = ix - 1;
	    continue;
	}
	
	end = ix;
	
	/* Wycinamy nazwe.
	 */
	nazwa = c[(begin+2)..(end-2)]; // -2, bo obcinamy drugie podkreslenie.
	
	odmiana = SLOWNIK->query_odmiana(nazwa);
	if (!odmiana)
	{
	    begin = -1; end = 0;
	    write("Brak nazwy \"" + nazwa + "\".\n");
	    continue;
	}
	tmp_size = sizeof(odmiana);
	
	/* Zapamietujemy wciecie przed funkcja zawierajaca nazwe do odmiany.
	 */
	cx = newline;
	indent = 0;

	while(++cx)
	{
	    if (c[cx] == ' ')
	    {
	        indent++;
	        continue;
	    }
	    else if (c[cx] == '\t')
	    {
	        indent += (8 - (indent % 8)); // Dopelnienie do tabulatora.
	        continue;
	    }
	    else
	        break;
	}
	
	left = 75 - ((begin - newline) - (cx - newline) + indent);

	indent += 4;
//	indent = min(indent, 52);
	cx = indent;
	indent_str = "";
	while (--cx >= 0)
	   indent_str += " ";
	
	/* Preparujemy odmiane do podstawienia.
	 */
	to_insert = "";
	cx = -1;
	while(++cx < tmp_size)
	{
	    elem_len = strlen(odmiana[cx]) + 1;
	    
	    if (elem_len > left)
	    {
		left = 75 - indent - elem_len;
		to_insert += ("\n" + indent_str + odmiana[cx]);
	    }
	    else
	    {
		left -= elem_len;
		to_insert += (" " + odmiana[cx]);
	    }
	}

	to_insert = to_insert[1..]; // Zbedna spacja na poczatku.

	cx = strlen(to_insert);
	ix = begin + cx;
	size = begin + cx + (size - end - 1);
	c = (begin > 0 ? c[0..(begin - 1)] : "") + to_insert + c[(end + 1)..];
	write("Podstawiam nazwe \"" + nazwa + "\".\n");
	begin = -1; end = 0;
    }
    
    rm(sciezka);
    write_file(sciezka, c);
    write("Ok.\n");
    return 1;
}


/* **************************************************************************
 * odnow - destruct-update-load-clone.
 */
nomask int
odnow(string str)
{
    mixed ob;
    int size, err;
    string sciezka, plik;
    object envir;
    
    CHECK_SO_WIZ;
    
    notify_fail("Odnow co?\n");
    
    if (!str)
        return 0;
   
    if (!parse_command(str, all_inventory(environment(this_player())) + 
        all_inventory(this_player()), "%i:" + PL_BIE, ob))
    {
	sciezka = FTPATH(this_interactive()->query_path(), str);
	if (sciezka[-2..-1] != ".c")
	    sciezka += ".c";
	ob = get_dir(sciezka);
	size = sizeof(ob);
	if (!size)
	    return 0;
	if (size > 1)
	{
	    notify_fail("Mozesz odnowic tylko jeden obiekt naraz.\n");
	    return 0;
	}
	plik = ob[0];
	sciezka = implode(explode(sciezka, "/")[0..-2], "/");
	sciezka += "/" + plik;
	ob = 0;
    }
    else
    {
	ob = NORMAL_ACCESS(ob, 0, 0);
	size = sizeof(ob);
	if (!size)
	    return 0;

	if (size > 1)
	{
	    write("Nie mozesz odnawiac wiecej niz jednego obiektu naraz.\n");
	    return 1;
	}
	
	ob = ob[0];

    }
    
    if (ob)
    {
	sciezka = MASTER_OB(ob);
	envir = environment(ob);
	write(sprintf("Odnawiam %s (%s).\n", ob->short(this_player(), PL_BIE),
	    sciezka));
    }
    else
        write(sprintf("Odnawiam obiekt '%s'.\n", sciezka));
    
    /* 
     * Destruct...
     */
    if (ob)
    {
	ob->remove_object();
	if (ob)
	    SECURITY->do_debug("destroy", ob);
    }
    
    /*
     * Update...
     */
    ob = find_object(sciezka);
    if (ob)
    {
	ob->remove_object();
	if (ob)
	    SECURITY->do_debug("destroy", ob);
    }

    /*
     * Load...
     */
    if (catch(sciezka->teledningsanka()))
    {
        write("Blad w ladowaniu nowej wersji obiektu.\n");
        return 1;
    }
    
    /*
     * Clone.
     */
    ob = clone_object(sciezka);
    if (!ob)
    {
        write("Blad w klonowaniu nowej wersji obiektu.\n");
        return 1;
    }
    
    if (!envir)
    {
        if (function_exists("create_living", ob))
	    envir = environment(this_player());
	else
	    envir = this_player();
    }
    
    if (function_exists("create_living", ob))
        err = ob->move_living("M", envir, 0, 1);
    else
        err = ob->move(envir, 1);
        
    set_this_player(this_interactive());
    
    if (err || environment(ob) != envir)
    {
        write("Blad w przenoszeniu obiektu spowrotem.\n");
        ob->remove_object();
	if (ob)
	    SECURITY->do_debug("destroy", ob);
        
        return 1;
    }
    
    write("Ok.\n");
    return 1;
}

/* **************************************************************************
 * remake - Remake an object, checks entire dependency of inherited files
 */
nomask int 
remake_object(string str)
{
    object ob, our_ob;
    string *inherits, *updatem;
    int il, ix, szf;

    CHECK_SO_WIZ;

    if (!stringp(str))
    {
	notify_fail("Remake what object ?\n");
	return 0;
    }
    str = FTPATH((string)this_interactive()->query_path(), str);
    if (!strlen(str))
    {
	notify_fail("Invalid file name.\n");
	return 0;
    }
    our_ob = find_object(str);
    if (!our_ob)
    {
	notify_fail("No such object loaded.\n");
	return 0;
    }
    inherits = SECURITY->do_debug("inherit_list", our_ob);
    
    szf = sizeof(inherits);
    updatem = ({});
    il = -1;
    
    while(++il < szf)
    {
	ob = find_object(inherits[il]);
	
	if (!ob)
	    continue;

	if (object_time(ob) > object_time(our_ob))
	    break;
	
#if 0
	if (file_time(inherits[il]) > object_time(ob))
	    updatem += ({ inherits[il] });
	else if (sizeof(updatem & SECURITY->do_debug("inherit_list", ob)))
	{
	    dump_array(updatem);
	    dump_array(SECURITY->do_debug("inherit_list", ob));
	    updatem += ({ inherits[il] });
	}
#endif

    }
    for (ix = il; ix < szf; ix++)
    {
	write("Updating: " + inherits[ix] + "\n");
	SECURITY->do_debug("destroy", find_object(inherits[ix]));
    }
    write("\n-----------\nUpdated " + (szf - il) + " objects.\n");
    /* call_other(str,"teleledningsanka"); */
    return 1;
}

/* **************************************************************************
 * rm - remove multiple files
 */
nomask int
rm_cmd(string str)
{
    return prepare_many(str, MULTI_RM);
}

/* **************************************************************************
 * rmdir - delete a directory
 */
nomask int
removedir(string str)
{
    CHECK_SO_WIZ;

    if (!stringp(str))
    {
	notify_fail("Remove what dir?\n");
	return 0;
    }
    if (rmdir(FTPATH((string)this_interactive()->query_path(), str)))
	write("Ok.\n");
    else
	write("Fail.\n");
    return 1;
}

/* **************************************************************************
 * trust - trust an object.
 */
nomask int
trust_ob(string str)
{
    object ob;

    CHECK_SO_WIZ;

    if (!str) 
    {
	notify_fail("Trust what object?\n");
	return 0;
    }

    ob = parse_list(str);

    if (!ob) 
    {
	notify_fail("Object not found: " + str + "\n");
	return 0;
    }

    if (geteuid(ob))
    {
	notify_fail("Object already trusted by: " + geteuid(ob) + "\n");
	return 0;
    }

    /* Install the euid of this player as uid in the object */
    export_uid(ob);
    /* Activate the object */
    ob->set_trusted(1);

    write("Beware! You have just trusted: " + str + ".\n");
    return 1;
}

/* **************************************************************************
 * update - update an object
 */
private nomask int
update_ob(string str)
{
    object ob, *obs;
    int kick_master, i, error;

    if (!strlen(str))
    {
	ob = environment(this_player());
	str = MASTER_OB(ob);

	if (!ob)
	{
	    write("Update what object?\n");
	    return 0;
	}

	obs = filter(all_inventory(ob), "is_player", this_object());

	error = 0;
	for (i = 0; i < sizeof(obs); i++)
	{
	    if (obs[i]->query_default_start_location() == str)
	    {
		error = 1;	
		write("Cannot update the start location of "
		    + capitalize(obs[i]->query_real_name()) + ".\n");
	    }
	}

	if (error == 1)
	{
	    notify_fail("Update failed.\n");
	    return 0;
	}

	write("Uaktualniasz okolice.\n");
	say(QCIMIE(this_player(), PL_MIA) + " uaktualnia okolice.\n");

	/* Move all objects out of the room */
	for (i = 0; i<sizeof(obs); i++)
	{
	    obs[i]->move(obs[i]->query_default_start_location());
	}

	ob->remove_object();
	ob = find_object(str);
	if (ob)
	    SECURITY->do_debug("destroy", ob);

	for (i = 0; i < sizeof(obs); i++)
	    obs[i]->move(str);
	return 1;
    }
    else
    {
	ob = find_object(str);
	if (!ob)
	{
	    notify_fail(sprintf("No such object: %s\n", str));
	    return 0;
	}
    }

    if (ob == find_object(SECURITY))
	kick_master = 1;

    if (ob != this_object())
    {
	ob->remove_object();
	ob = find_object(str);
	if (ob)
	    SECURITY->do_debug("destroy", ob);

	/* When updating the master object it must be reloaded at once
	   and from within the GD
	 */
	if (kick_master)
	{
	    write(sprintf("%s was updated and reloaded.\n", SECURITY));
	    SECURITY->teleledningsanka();
	    return 1;
	}
	else if (!ob)
	{
	    return 1;
	}
	else
	{
	    notify_fail(sprintf("Could not be updated: %s\n", str));
	    return 0;
	}
    }
    else
    {
	write("Updating myself. Will be reloaded at next reference.\n");
	destruct();
	return 1;
    }
    
    return 1;

}

public nomask int
dupd(string s)
{
    int i;
    string *dir, path;
    
    CHECK_SO_WIZ;

    /* catch 'update' without args */

    if (!strlen(s))
    {
	update_ob(s);
	return 1;
    }

    if (wildmatch("*[\\*\\?]*", s))
	return prepare_many(s, MULTI_UPDATE);

    s = FTPATH((string)this_interactive()->query_path(), s);
    if (!strlen(s))
    {
	notify_fail("Invalid file name.\n");
	return 0;
    }

    if (update_ob(s))
    {
	write(sprintf("Updated: %s\n", s));
	return 1;
    }
    else
	return 0; // update_ob() powinno ustawic odpowiedni komunikat.
}

nomask int
is_player(object ob)
{
    return (living(ob) &&
	    !(ob->query_npc()));
}
