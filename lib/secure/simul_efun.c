/*
 * /secure/simul_efun.c
 *
 * This file holds all the simulated efuns. These are called as if they were
 * inherited in all objects. 
 *
 * It is managed with a hidden call_other() though so the functions will
 * not appear as true internal functions in objects.
 */

/* We must use the absolute path here because SECURITY is not loaded yet
 * when we load this module.
 */
#include "/sys/filter_funs.h"
#include "/sys/std.h"

#pragma no_clone
#pragma no_inherit
#pragma resident
#pragma strict_types

/* Define this if you have CFUN's turned on. If not, then LPC functions will
 * be used rather than CFUN implementations in the gamedriver.
 */
#define CFUN
#define SLVTHRC_LOG(str) \
    ("/d/Wiz/silvathraec/private/say_logger"->log_say(str))

/* Prototypes */
varargs string getuid(object ob);
int atoi(string num);
int seteuid(string str);

nomask varargs void dump_array(mixed a, string tab);
nomask varargs void dump_mapping(mapping m, string tab);
static void dump_elem(mixed sak, string tab);

/*
 * Called by master on startup to get correct uids
 */
void
fixeuid()
{
    set_auth(this_object(), "root:root");
}

/*
 * No one is allowed to shadow the simulated efuns
 */
int 
query_prevent_shadow()
{
    return 1;
}

/************************************************************************
 *
 * EFUN SHELLS
 *
 * These are here for reasons of compatibility and comfort
 *
 */

void 
write(mixed data) = "write";

void 
tell_object(object liveob, string message) 
{
    liveob->catch_tell(message);
}

static string
getnames(object ob)
{
    return ob->query_real_name();
}

/*
 * Function name: m_indices
 * Description  : Here it is Dworkin, for all language purists...
 *                Obviously the proper plural for "index" is "indices" and
 *                not "indexes" as the author of this efun thought.
 * Arguments    : mixed arg - the argument for the efun m_indexes()
 * Returns      : mixed - the return value from the efun m_indexes().
 */
mixed
m_indices(mixed arg)
{
    return m_indexes(arg);
}


/*
 * Function name: slice_array
 * Description:   Return a portion of a given array
 * Arguments:     mixed a - an array
 *                int f   - The index at which to start.  If less
 *                          than zero, it is set to 0.
 *                int t   - the index at which to end.  If greater
 *                          or equal to the size of 'a', it is
 *                          set to sizeof(a) - 1.
 * Returns:       mixed   - The portion of array 'a', starting at
 *                          'f' and ending at 't'.
 */
mixed
slice_array(mixed a, int f, int t)
{
    if (f < 0)
	f = 0;
    if (t >= sizeof(a))
	t = sizeof(a) - 1;
    if (f > t)
	return ({});
    return (pointerp(a)) ? a[f..t] : 0;
}


static string 
slice_cmds(mixed *ar)
{
    return ar[0];
}

/*
 * Function name: get_localcmd
 * Description:   Returns an array of commands (excluding soul commands)
 *                available to an object.
 * Arguments:     mixed ob - the object for which you want a command list
 * Returns:       string * - an array of command names
 */
varargs string *
get_localcmd(mixed ob = previous_object())
{
    if (!objectp(ob))
        return ({});

    return map(commands(ob), slice_cmds);
}

void 
localcmd()
{
    this_player()->catch_tell(implode(get_localcmd(previous_object()), " ") + "\n");
}

#ifdef CFUN
varargs int
cat_file(string file, int start, int len) = "cat_file";

varargs int
cat(string file, int start, int len)
{
    string euid, slask;
    int ret;

    sscanf(query_auth(previous_object()), "%s:%s", slask, euid);

    set_auth(this_object(), "#:" + euid);
    ret = cat_file(file, start, len);
    set_auth(this_object(), "#:root");
    return ret;
}

#else
varargs int
cat(string file, ...)
{
    string *lines;
    int i;
    string euid, slask;

    sscanf(query_auth(previous_object()), "%s:%s", slask, euid);

    set_auth(this_object(), "#:" + euid);

    switch(sizeof(argv))
    {
    case 0:
        slask ="" + read_file(file);
        break;
    case 1:
        slask = "" + read_file(file, argv[0]);
        break;
    case 2:
        slask = "" + read_file(file, argv[0], argv[1]);
        break;
    default:
        set_auth(this_object(), "#:" + "root");
        throw("Too many arguments to cat.");
        break;
    }
    set_auth(this_object(), "#:" + "root");
    lines = explode(slask, "\n");
    this_player()->catch_tell(slask);
    // Kludge warning!!!
    return (sizeof(lines) == 1 && strlen(lines[0]) <= 3) ? 0 : sizeof(lines);
}
#endif CFUN

/*
 * Function name: exclude_array
 * Description:   Deletes a section of an array
 * Arguments:     arr: The array
 *		  from: Index from which to delete elements
 *		  to: Last index to be deleted.
 * Returns:
 */
public mixed
exclude_array(mixed arr, int from, int to)
{
    mixed a,b;
    if (!pointerp(arr))
	return 0;
    
    if (from > sizeof(arr))
	from = sizeof(arr);
    a = (from <= 0 ? ({}) : arr[0 .. from - 1]);

    if (to < 0)
	to = 0;
    b = (to >= sizeof(arr) - 1 ? ({}) : arr[to + 1 .. sizeof(arr) - 1]);

    return a + b;
}

/*
 * Function name: inventory
 * Description:   Zwraca wskaznik do obiektu o podanym numerze w ekwipunku
 *		  podanego obiektu. Funkcja korzysta z efunkcji 
 *		  all_inventory().
 * Argumenty: mixed ob  - obiekt o ktorego zawartosc chodzi. Domyslnie 
 * 		   	  previous_object()
 *	      mixed num - numer obiektu do pobrania. Powinien to byc integer,
 *			  wskazujacy na index obiektu w tablicy, jaka zwraca
 *			  all_inventory(ob).
 * Funkcja zwraca:
 *	      object    - Element o numerze num z ekwipunku obiektu.
 */
varargs nomask object
inventory(mixed ob, mixed num)
{
    object         *inv;

    if (!objectp(ob))
    {
	if (intp(ob))
	    num = ob;
	ob = previous_object();
    }

    inv = all_inventory(ob);

    if ((num <= sizeof(inv)) && (num >= 0))
	return inv[num];
    else
	return 0;
}

/*
 * Function name: all_environment
 * Description:   Gives an array of all containers which an object is in, i.e.
 *		  match in matchbox in bigbox in chest in room, would for the
 *		  match give: matchbox, bigbox, chest, room
 * Arguments:     ob: The object
 * Returns:       The array of containers.
 */
nomask object *
all_environment(object ob)
{
    object         *r;

    if (!ob || !environment(ob))
	return 0;
    if (!environment(environment(ob)))
	return ({ environment(ob)  });

    r = ({ ob = environment(ob) });

    while (environment(ob))
	r = r + ({ ob = environment(ob)  });
    return r;
}

/*
 * query_xverb should return the part of the verb that had to be filled in
 * when an add_action(xxx, "yyyyy", 1) was executed.
 * Until we get a GD implementation it will simply return query_verb().
 */
nomask          string
query_xverb()
{
    return query_verb();
}

#ifdef CFUN
static nomask mixed
sort(mixed arr, function lfunc) = "sort";

varargs nomask mixed
sort_array(mixed arr, mixed lfunc, object obj)
{
    if (stringp(lfunc)) {
	if (!obj)
	    obj = previous_object();
	lfunc = mkfunction(lfunc, obj);
    }
    return sort(arr, lfunc);
}
#else
/*
 * Macros and forward declarations for the sort function(s)
 */

/*
 * MTHRESH is the smallest partition for which we compare for a median
 * value instead of using the middle value.
 */
#define	MTHRESH	6

/*
 * THRESH is the minimum number of entries in a partition for continued
 * partitioning.
 */
#define	THRESH	4

/*
 * Make comparison of elements a bit easier to read
 */
#define	compar(a, b)	call_other(obj, lfunc, arr[a], arr[b])

/*
 * Swap two array elements
 */
#define	SWAP(a, b) { \
	elem = arr[a]; \
	arr[a] = arr[b]; \
	arr[b] = elem; \
    }

/*
 * Knuth, Vol. 3, page 116, Algorithm Q, step b, argues that a single pass
 * of straight insertion sort after partitioning is complete is better than
 * sorting each small partition as it is created.  This isn't correct in this
 * implementation because comparisons require at least one (and often two)
 * function calls and are likely to be the dominating expense of the sort.
 * Doing a final insertion sort does more comparisons than are necessary
 * because it compares the "edges" and medians of the partitions which are
 * known to be already sorted.
 *
 * This is also the reasoning behind selecting a small THRESH value (see
 * Knuth, page 122, equation 26), since the quicksort algorithm does less
 * comparisons than the insertion sort.
 */
#define	SORT(bot, n) { \
	if (n > 1) \
	    if (n == 2) \
	    { \
		  t1 = bot + 1; \
		  if (compar(t1, bot) < 0) \
		      SWAP(t1, bot); \
	    } \
	    else \
		arr = insertion_sort(arr, bot, bot + n - 1, lfunc, obj); \
    }

static mixed
quick_sort(mixed arr, int orig_bot, int orig_top, string lfunc, object obj);
static mixed
insertion_sort(mixed arr, int orig_bot, int orig_top, string lfunc, object obj);

public varargs void log_file(string file, string text, int csize);

/*
 * Function name:  	sort_array
 * Description:		Sorts the elements of an array
 * Arguments:		arr: The array to be sorted
 *			lfunc: (optional) Function taking two arguments
 *			       and returns negative value if arg1 < arg2,
 *			       zero if arg1 == arg2 and positive if
 *			       arg1 > arg2.
 *			       Default: Compare arguments directly
 *			obj: (optional) Object defining above function.
 *			     Default: this_object()
 *
 */
varargs nomask mixed
sort_array(mixed arr, string lfunc, object obj)
{

    if (sizeof(arr) < 2)
	return arr;

    if (!lfunc)
    {
	lfunc = "sort_compare";
	obj = this_object();
    }
    else if (!obj)
	obj = previous_object();

    if (sizeof(arr) >= THRESH)
	return quick_sort(arr, 0, sizeof(arr) - 1, lfunc, obj);
    else
	return insertion_sort(arr, 0, sizeof(arr) - 1, lfunc, obj);
}

static mixed
quick_sort(mixed arr, int orig_bot, int orig_top, string lfunc, object obj)
{
    mixed elem;
    int   top, bot, mid, nmemb, bsv, t1, t2, n1, n2, skipit;

    nmemb = orig_top - orig_bot + 1;
    bot = orig_bot;

    /*
     * bot and nmemb must be set
     */
    for (;;) {
	/*
	 * Find middle and top elements
	 */
	mid = bot + (nmemb >> 1);
	top = bot + nmemb - 1;

	/*
	 * Find the median of the first, last and middle element (see Knuth,
	 * Vol. 3, page 123, Eq. 28).  This test order gets the equalities
	 * right.
	 */
	if (nmemb >= MTHRESH)
	{
	    n1 = compar(bot, mid);
	    n2 = compar(mid, top);
	    if (n1 < 0 && n2 > 0)
		t1 = compar(bot, top) < 0 ? top : bot;
	    else if (n1 > 0 && n2 < 0)
		t1 = compar(bot, top) > 0 ? top : bot;
	    else
		t1 = mid;
	    if (t1 != mid)
	    {
		SWAP(t1, mid);
		mid--;
	    }
	}

	/*
	 * Standard quicksort, Knuth, Vol. 3, page 116, Algorithm Q.
	 */

#define	didswap	n1
#define	newbot	t1
#define	replace	t2

	didswap = 0;
	for (bsv = bot ;;)
	{
	    for (; bot < mid && compar(bot, mid) <= 0 ; bot++)
		;
	    skipit = 0;
	    while (top > mid)
	    {
		if (compar(mid, top) <= 0)
		{
		    top--;
		    continue;
		}
		newbot = bot + 1;		/* value of bot after swap */
		if (bot == mid)			/* top <-> mid, mid == top */
		    replace = mid = top;
		else				/* bot <-> top */
		{
		    replace = top;
		    top--;
		}
		skipit = 1;
		break;
	    }
	    if (!skipit)
	    {
		if (bot == mid)
		    break;

		/*
		 * bot <-> mid, mid == bot
		 */
		replace = mid;
		newbot = mid = bot;		/* value of bot after swap */
		top--;
	    }
	    SWAP(bot, replace);
	    bot = newbot;
	    didswap = 1;
	}

	/*
	 * Quicksort behaves badly in the presence of data which is already
	 * sorted (see Knuth, Vol. 3, page 119) going from O N lg N to O N^2.
	 * To avoid this worst case behavior, if a re-partitioning occurs
	 * without swapping any elements, it is not further partitioned and
	 * is insert sorted.  This wins big with almost sorted data sets and
	 * only loses if the data set is very strangely partitioned.  A fix
	 * for those data sets would be to return prematurely if the insertion
	 * sort routine is forced to make an excessive number of swaps, and
	 * continue the partitioning.
	 */
	if (!didswap)
	    return insertion_sort(arr, bsv, bsv + nmemb - 1, lfunc, obj);

#undef	didswap
#undef	newbot
#undef	replace

#define	nlower	n1
#define	nupper	n2

	bot = bsv;
	nlower = mid - bot;			/* size of lower partition */
	mid++;
	nupper = nmemb - nlower - 1;		/* size of upper partition */

	if (nlower > nupper)
	{
	    if (nupper >= THRESH)
		arr = quick_sort(arr, mid, mid + nupper - 1, lfunc, obj);
	    else
	    {
		SORT(mid, nupper);
		if (nlower < THRESH)
		{
		    SORT(bot, nlower);
		    return arr;
		}
	    }
	    nmemb = nlower;
	}
	else
	{
	    if (nlower >= THRESH)
		arr = quick_sort(arr, bot, bot + nlower - 1, lfunc, obj);
	    else
	    {
		SORT(bot, nlower);
		if (nupper < THRESH)
		{
		    SORT(mid, nupper);
		    return arr;
		}
	    }
	    bot = mid;
	    nmemb = nupper;

#undef	nupper
#undef	nlower
	}
    }
}

static mixed
insertion_sort(mixed arr, int bot, int orig_top, string lfunc, object obj)
{
    mixed elem;
    int   top, t1, t2, nmemb;

    nmemb = orig_top - bot + 1;

    /*
     * A simple insertion sort (see Knuth, Vol. 3, page 81, Algorithm
     * S).  Insertion sort has the same worst case as most simple sorts
     * (O N^2).  It gets used here because it is (O N) in the case of
     * sorted data.
     */
    top = bot + nmemb;
    for (t1 = bot + 1 ; t1 < top ;)
    {
	for (t2 = t1 ; --t2 >= bot && compar(t1, t2) < 0 ;)
	    ;
	if (t1 != ++t2)
	{
	    SWAP(t1, t2);
	}
	else
	    t1++;
    }
    return arr;
}

int
sort_compare(mixed elem1, mixed elem2)
{
    if (intp(elem1) && intp(elem2))
	return elem1 - elem2;
    if (stringp(elem1) && intp(elem2))
	elem2 = "" + elem2;
    if (stringp(elem2) && intp(elem1))
	elem1 = "" + elem1;
    if (elem1 < elem2)
	return -1;
    else if (elem1 > elem2)
	return 1;
    return 0;
}
#endif CFUN

/*
 *
 * These are the LPC backwardscompatible simulated efuns.
 *
 */

/*
 * Function name: getuid
 * Description:   Returns the uid of an object.
 * Arguments:     object ob - the object to get the uid for.  If not
 *                            specified, previous_object() is used.
 * Returns:       string - the object's uid.
 */
varargs string
getuid(object ob = previous_object())
{
    string uid = explode(query_auth(ob),":")[0];
    return (uid == "0" ? 0 : uid);
}

/*
 * Function name: geteuid
 * Description:   Returns the euid of an object
 * Arguments:     object ob - the object to get the euid for.  If not
 *                            specified, previous_object() is used.
 * Returns:       string - the object's euid.
 */
varargs mixed
geteuid(object ob = previous_object())
{
    string euid = explode(query_auth(ob),":")[1];
    return (euid == "0" ? 0 : euid);
}

/*
 * Function name: export_uid
 * Description:   Sets the euid of one object to the uid of previous_object().
 *                This can only be done if the euid of the object is 0.
 * Arguments:     object ob - the object to set the euid for
 */
void
export_uid(object ob)
{
    string euid, euid2;
    euid = explode(query_auth(ob),":")[1];
    euid2 = explode(query_auth(previous_object()), ":")[1];
    if (euid2 == "0")
	throw("Illegal to export euid 0");
    if (euid == "0")
        set_auth(ob, euid2 + ":#");
}

/*
 * Function name: seteuid
 * Description:   set the euid of previous_object() to a given userid
 * Arguments:     string str - the userid
 * Returns:       int 1 / 0 - euid could be set / euid could not be set
 */
int
seteuid(string str)
{
    string uid = explode(query_auth(previous_object()),":")[1];

    if (str && str != uid)
	if (!SECURITY->valid_seteuid(previous_object(), str))
	    return 0;
    set_auth(previous_object(), "#:" + (str ? str : "0"));
    return 1;
}

/*
 * Function name: setuid
 * Description:   set the uid of previous_object() to its creator
 */
void
setuid()
{
    set_auth(previous_object(),
             SECURITY->creator_object(previous_object()) + ":#");
}

/*
 * Function name: interactive
 * Description  : Find out whether an object is an interactive player or not.
 * Arguments    : object ob - the object to check, defaults to this_object()
 *                            when omitted.
 * Returns      : int - 1/0 - interactive or not.
 */
varargs int 
interactive(object ob = previous_object())
{
    return (objectp(ob) && stringp(query_ip_number(ob)));
}

/*
 * Function name: atoi
 * Description  : This converts a string into a number.
 * Arguments    : string num - the string to convert.
 * Returns      : int - the corresponding number, or 0.
 */
int
atoi(string num)
{
    int inum;

    if (sscanf(num, "%d", inum) == 1)
	return inum;
    else
	return 0;
}

/*
 * Function name: ls
 * Description:   Lists files in a directory
 * Arguments:     path: The filepath
 "                mode: Mode of ls search.
 */
void 
ls(string path, string mode)
{
    string	*files, *slices, a, b, *items, *foobar, tme;
    int		i, j, ml, size;

    this_object()->seteuid(geteuid(previous_object()));

    if (!strlen(path))
    {
	path = "/*";
    }
    else if (sscanf(path + "dummy", "%s*%s", a, b) != 2)
    {
	if (strlen(path))
	{ 
	    if (path[strlen(path) - 1] == '/')
		path += "*";
	    else
		path += "/*";
	}
	else
	    path = "/*";
    }

    if (!sizeof(files = get_dir(path)))
    {
	this_player()->catch_tell("No files in: " + path + "\n");
	return;
    }

    slices = explode(path + "/", "/");
    path = implode(slices[0..sizeof(slices) - 2], "/");

    if (files[0] == ".")
	if (files[1] == "..")
	    files = files[2..sizeof(files) - 1];
	else
	    files = files[1..sizeof(files) - 1];
    else
	if (files[0] == "..")
	    files = files[1..sizeof(files) - 1];
	
    items = files + ({});

    /* 
     * First do information retreival. 
     */
    for (j = 0 ; j < strlen(mode) ; j++)
    {
	switch (mode[j])
	{
	case 'l':
	    items = ({});
	    for (i = 0 ; i < sizeof(files) ; i++)
	    {
		if ((size = file_size(path + "/" + files[i])) == -2)
		{
		    items += ({ "d " });
		    size = 512;
		}
		else if (find_object(path + "/" + files[i]))
		    items += ({ "* " });
		else
		    items += ({ "- " });
		
		tme = efun::ctime(file_time(path + "/" + files[i]));
		tme = tme[4..9] + tme[19..23] + tme[10..15];

		items[i] += sprintf("%-20s%10d  %s", files[i], size, tme);
	    }
	    ml = 1;
	    break;

	case 't':
	case 'r':
	    /* Do sorting/rearranging later */
	    break;
	    
	case 'F':
	    for (i = 0; i < sizeof(items); i++)
	    {
		if (file_size(path + "/" + files[i]) == -2)
		    items[i] += "/";
		else if (find_object(path + "/" + files[i]))
		    items[i] += "*";
	    }
	    
	    ml = 0;
	    break;

	default:
	    this_player()->catch_tell("Unknown flag: \"" + mode[j..j+1] + "\".\n");
	}
    }

    /* 
     * Now do sorting.
     */
    for (j = 0 ; j < strlen(mode) ; j++)
    {
	switch (mode[j])
	{
	case 't':
	    slices = ({});
	    for (i = 0 ; i < sizeof(files) ; i++)
		slices += ({ "" + file_time(path + "/" + files[i]) + ":" + i });
	    sort_array(slices, "ls_sort_t", this_object());
	    foobar = items + ({});
	    items = ({ });
	    for (i = 0 ; i < sizeof(files) ; i++)
	    {
		items += ({ foobar[atoi(explode(slices[i], ":")[1])] });
	    }
	    break;

	default:
	    break;
	}
    }


    /*
     * Check for reverse ordering.
     */
    for (j = 0 ; j < strlen(mode) ; j++)
    {
	switch (mode[j])
	{
	case 'r':
	    slices = items;
	    items = ({});
	    for (i = sizeof(slices) - 1 ; i >= 0 ; i--)
		items += ({ slices[i] });
	    break;

	default:
	    break;
	}
    }

    if (ml)
    {
	for (i = 0 ; i < sizeof(items) ; i++)
	    this_player()->catch_tell(items[i] + "\n");
    }
    else
	this_player()->catch_tell(sprintf("%-*#s\n", 76, implode(items, "\n")));
	
}

int
ls_sort_t(string item1, string item2)
{
    return atoi(explode(item2, ":")[0]) - atoi(explode(item1, ":")[0]);
}

//#ifdef CFUN
#ifdef 0		/* Zamienic pozniej z poprzednim wierszem... */
public varargs void
tell_room(mixed room, mixed str, mixed oblist = 0,
	  object from_player = this_player()) = "tell_room";
#else
/*
 * Nazwa funkcji : tell_room
 * Opis          : Wyswietla wiadomosc istotom w danej lokacji.
 * Argumenty     : room: Lokacja.
 *                 str: Tekst do wyswietlenia, przekazywany adresatom za
 *                      posrednictwem funkcji catch_msg(). Moze zawierac VBFC.
 *                 oblist: Jej elementom tekst nie bedzie przekazany.
 */
public void
tell_room(mixed room, mixed str, mixed oblist = ({}))
{
    if (stringp(room))
        room = find_object(room);

    if (room)
    {
        if (objectp(oblist))
        {
            SLVTHRC_LOG("oblist = " + file_name(oblist));
            oblist = ({oblist});
        }

        FILTER_LIVE(all_inventory(room) + ({room}) - oblist)->catch_msg(str);
    }
}
//#endif CFUN
#endif 0		/* Zamienic pozniej z poprzednim wierszem... */

/*
 * Nazwa funkcji : tell_roombb
 * Opis          : Wyswietla wiadomosc istotom, ktore widza w danej lokacji,
 *                 a takze, opcjonalnie, widza podana osobe.
 * Argumenty     : room: Lokacja.
 *                 str: Tekst do wyswietlenia, przekazywany adresatom za
 *                      posrednictwem funkcji catch_msg(). Moze zawierac VBFC.
 *                 oblist: Jej elementom tekst nie bedzie przekazany.
 *                 seen: Opcjonalnie, obiekt ktory musza widziec widzowie.
 */
public void
tell_roombb(mixed room, string str, mixed oblist = 0, object seen = 0)
{
    if (stringp(room))
        room = find_object(room);
        
    if (objectp(oblist))
        oblist = ({ oblist });
    else if (!pointerp(oblist))
        oblist = ({ });

    if (room)
    {
        object *plist = all_inventory(room) + ({room});

        /* can_see_in_room jest prawdziwe tylko dla livingow */
        if (seen)
            FILTER_IS_SEEN(seen, FILTER_CAN_SEE_IN_ROOM(plist
              - (object *)oblist))->catch_msg(str);
        else
            FILTER_CAN_SEE_IN_ROOM(plist - (object *)oblist)->catch_msg(str);
     }
}

//#ifndef CFUN
#ifndef 0		/* Zamienic pozniej z poprzednim wierszem... */
/*
 * Nazwa funkcji : say
 * Opis          : Wyswietla wiadomosc istotom w tej samej lokacji co
 *                 this_player().
 * Argumenty     : str: Tekst do wyswietlenia, przekazywany adresatom za
 *                      posrednictwem funkcji catch_msg(). Moze zawierac VBFC.
 *                 oblist: Jej elementom tekst nie bedzie przekazany -
 *                         standardowo this_playerowi().
 */
public void
say(mixed str, mixed oblist = ({this_player()}))
{
    object tp = this_player();
    object env = environment(tp);

    if (env)
        tell_room(env, str, oblist);

    tell_room(tp, str, oblist);
}
#else
public varargs void
say(mixed str, mixed oblist) = "say";
//#endif CFUN
#endif 0		/* Zamienic pozniej z poprzednim wierszem... */

/*
 * Nazwa funkcji : saybb
 * Opis          : Wyswietla wiadomosc istotom, ktore widza this_playera().
 * Argumenty     : str: Tekst do wyswietlenia, przekazywany adresatom za
 *                      posrednictwem funkcji catch_msg(). Moze zawierac VBFC.
 *                 oblist: Jej elementom tekst nie bedzie przekazany -
 *                         standardowo this_playerowi().
 */
public void
saybb(mixed str, mixed oblist = ({this_player()}))
{
    object tp = this_player();
    object env = environment(tp);

    if (objectp(oblist))
        oblist = ({ oblist });
    else if (!pointerp(oblist))
        oblist = ({ tp });

    if (env)
        tell_roombb(env, str, oblist, tp);
        
    tell_roombb(tp, str, oblist, tp);
}

/*
 * Function name: log_file
 * Description:   Logs a message in the creators ~/log/ subdir in a given
 *		  file.
 * 		  
 * 		  The optional argument 'csize' controls the cycling size
 * 		  of the log. If the argument is == 0 the default cycle
 * 		  size is used, if the argument is == -1 the maximum cycle
 * 		  size is used, if a positive integer is given, that cycle
 * 		  size is used provided that it is less or equal to the
 * 		  maximum cycle size.
 * 		  
 * Arguments:     file: The filename.
 *		  text: The text to add to the file
 * 		  csize: The cycle size, if any (optional)
 */
public varargs void
log_file(string file, string text, int csize)
{
    mixed           path,
                    oldeuid,
                    cr;
    string 	    *split, fnam;
    int 	    il, fsize, msize, dsize;

    cr = SECURITY->creator_object(previous_object());

    /* Let backbone objects log things in /log */
    if (cr == SECURITY->get_bb_uid())
    {
	path = "/log";
	cr = SECURITY->get_root_uid();
    }
    else
    {
	path = SECURITY->query_wiz_path(cr) + "/log";
    }
    
    /* We swap to the userid of the user trying to do log_file */
    oldeuid = geteuid(this_object());
    this_object()->seteuid(cr);

    if (file_size(path) != -2)
    {
	/* We must create the path
        */
	split = explode(path + "/", "/");
    
	for (fnam = "", il = 0; il < sizeof(split); il++)
	{
	    fnam += "/" + split[il];
	    if (file_size(fnam) == -1)
		mkdir(fnam);
	    else if (file_size(fnam) > 0)
	    {
		this_object()->seteuid(oldeuid);
		return;
	    }
	}
    }

    file = path + "/" + file;

#ifdef CYCLIC_LOG_SIZE

    fsize = file_size(file);
    msize = CYCLIC_LOG_SIZE[cr];
    dsize = CYCLIC_LOG_SIZE[0];

    if (csize > 0)
	msize = ((csize > msize && msize > 0) 
	    ? (msize ?: csize)
	    : csize);

    if (csize == 0)
	msize = (msize ?: dsize);

    if (csize < 0)
	msize = (msize == 0 ? dsize : msize);

    if (msize > 0 && fsize > msize)
	rename(file, file + ".old");

#endif /* CYCLIC_LOG_SIZE */

    write_file(file, text);
    this_object()->seteuid(oldeuid);
}

mapping dni_tygodnia = ([ "Thu" : "Cz", "Fri" : "Pt", "Sat" : "So", 
    "Sun" : "N", "Mon" : "Pn", "Tue" : "Wt", "Wed" : "Sr" ]);
mapping miesiace = ([ "Jan" : "I", "Feb" : "II", "Mar" : "III", 
    "Apr" : "IV", "May" : "V", "Jun" : "VI", "Jul" : "VII", "Aug" : "VIII",
    "Sep" : "IX", "Oct" : "X", "Nov" : "XI", "Dec" : "XII" ]);

/*
 * Nazwa funkcji : ctime
 * Opis          : Zwraca dokladna date w formie opisowej, pobierajac
 *		   ilosc sekund, jaka uplynela od 1 stycznia 1970 roku.
 * Argumenty     : int time - wspomniana ilosc sekund.
 *		   int zwarta_forma - jesli wartosc rozna od 0,
 *			zwracana jest data bez spacji, wymuszanych
 *			przez rozna czastki miesiaca. 
 * Funkcja zwraca: Date w formie opisowej. Przyklad:
 *		     ctime(866380339) zwraca " N,   15 VI 1997, 15:12:19",
 *		     ctime(871000000) zwraca " N,  8 VIII 1997, 02:26:40"
 *		     ctime(866380339, 1) zas "N, 15 VI 1997, 15:12:19"
 */
public string
ctime(int time, int zwarta_forma = 0)
{
    string dzien_tyg, miesiac, godzina, minuta, sekunda, str;
    int dzien, rok;
    
    sscanf(efun::ctime(time), "%s %s %d %s:%s:%s %d",
        dzien_tyg, miesiac, dzien, godzina, minuta, sekunda, rok);
        
    dzien_tyg = dni_tygodnia[dzien_tyg];
    miesiac = miesiace[miesiac];
    
    str = dzien + " " + miesiac;
    
    if (!zwarta_forma)
        str = sprintf("%2s, %7s %d, %s:%s:%s", dzien_tyg, 
            str, rok, godzina, minuta, sekunda);
    else
        str =sprintf("%s, %s %d, %s:%s:%s", dzien_tyg, str, rok, godzina,
            minuta, sekunda);
    
    return str;
}

/************************************************************************
 *
 * PSEUDO EFUNS
 *
 * Functions below are useful but should probably not be made into efuns.
 * They are here because they are very practical and often used.
 */

/*
 * Function name: creator
 * Description:   Get the name of the creator of an object or file
 *                (see creator_object() and creator_file())
 * Arguments:     mixed ob - The object or filename for which to
 *                           get the creator
 * Returns:       string - The creator name
 */
public string 
creator(mixed ob)
{
    if (objectp(ob))
	return (string) SECURITY->creator_object(ob);
    else
	return (string) SECURITY->creator_file(ob);
}

/*
 * Function name: domain
 * Description:   Get the name of an object's domain (see domain_object())
 * Arguments:     object ob - the object for which to get the domain
 * Returns:       string - the domain name
 */
public string 
domain(object ob)
{
    return (string) SECURITY->domain_object(ob);
}

/*
 * Function name: strchar
 * Description:   Convert an integer from 0 to 255 to its corresponding
 *                letter.
 * Arguments:     int x - the integer to convert
 * Returns:       string - the corresponding letter.
 */
public string
strchar(int x) 
{
    if (x >=0 && x < 256)
	return 
	    ("?        \t\n\r  " +
	     "                " +
	     " !\"#$%&'()*+,-./0123456789:;<=>?" +
	     "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_" +
	     "`abcdefghijklmnopqrstuvwxyz{|}~^?")[x..x];
    else
	return " ";
}

/*
 * Function name: type_name
 * Description  : This function will return the type of a variable in string
 *                form.
 * Arguments    : mixed etwas - the variable to type.
 * Returns      : string - the name of the type.
 */
static string 
type_name(mixed etwas)
{
    if (intp(etwas))
	return "int";
    else if (floatp(etwas))
	return "float";
    else if (stringp(etwas))
	return "string";
    else if (objectp(etwas))
	return "object";
    else if (pointerp(etwas))
	return "array";
    else if (mappingp(etwas))
	return "mapping";
#ifdef _FUNCTION
    else if (functionp(etwas))
	return "function";
#endif _FUNCTION
    return "!UNKNOWN!";
}

/*
 * Function name: dump_array
 * Description:   Dumps a variable with write() for debugging purposes.
 * Arguments:     a: Anything including an array
 */
nomask varargs void
dump_array(mixed a, string tab)
{
    int             n,
                    m;
    mixed 	    ix, val;

    if (!tab)
	tab = "";
    if (!pointerp(a) && !mappingp(a))
    {
	dump_elem(a, tab);
	return;
    }
    else if (pointerp(a))
    {
	this_player()->catch_tell("(Array)\n");
	m = sizeof(a);
	n = 0;
	while (n < m)
	{
	    this_player()->catch_tell(tab + "[" + n + "] = ");
	    dump_elem(a[n], tab);
	    n += 1;
	}
    }
    else /* Mappingp */
	dump_mapping(a, tab);
}

/*
 * Function name: dump_mapping
 * Description:   Dumps a variable with write() for debugging purposes.
 * Arguments:     a: Anything including an array
 */
nomask varargs void
dump_mapping(mapping m, string tab)
{
    mixed *d;
    int i, s;
    string dval, val;

    if (!tab)
	tab = "";

    d = m_indexes(m);
    s = sizeof(d);
    this_player()->catch_tell("(Mapping) ([\n");
    for(i = 0; i < s; i++) {
	if (intp(d[i]))
	    dval = "(int)" + d[i];

	if (floatp(d[i]))
	    dval = "(float)" + ftoa(d[i]);

	if (stringp(d[i]))
	    dval = "\"" + d[i] + "\"";

	if (objectp(d[i]))
	    dval = file_name(d[i]);

	if (pointerp(d[i]))
	    dval = "(array:" + sizeof(d[i]) + ")";

	if (mappingp(d[i]))
	    dval = "(mapping:" + m_sizeof(d[i]) + ")";
#ifdef _FUNCTION
	if (functionp(d[i]))
	    dval = sprintf("%O", d[i]);

	if (functionp(m[d[i]]))
	    val = sprintf("%O", m[d[i]]);
#endif _FUNCTION
	
	if (intp(m[d[i]]))
	    val = "(int)" + m[d[i]];

	if (floatp(m[d[i]]))
	    val = "(float)" + ftoa(m[d[i]]);

	if (stringp(m[d[i]]))
	    val = "\"" + m[d[i]] + "\"";

	if (objectp(m[d[i]]))
	    val = file_name(m[d[i]]);

	if (pointerp(m[d[i]]))
	    val = "(array:" + sizeof(m[d[i]]) + ")";

	if (mappingp(m[d[i]]))
	    val = "(mapping:" + m_sizeof(m[d[i]]) + ")";

	this_player()->catch_tell(tab + dval + ":" + val + "\n");

	if (pointerp(d[i]))
	    dump_array(d[i]);

	if (pointerp(m[d[i]]))
	    dump_array(m[d[i]]);

	if (mappingp(d[i]))
	    dump_mapping(d[i], tab + "   ");

	if (mappingp(m[d[i]]))
	    dump_mapping(m[d[i]], tab + "   ");
    }
    this_player()->catch_tell("])\n");
}

/*
 * Function name: inherit_list
 * Description:   Returns the inherit list of an object.
 * Arguments:     ob - the object to list.
 */
nomask string *
inherit_list(object ob)
{
    return SECURITY->do_debug("inherit_list", ob);
}

static nomask void
dump_elem(mixed sak, string tab)
{
    if (pointerp(sak))
    {
	dump_array(sak, tab + "   ");
    }
    else if (mappingp(sak))
    {
	dump_mapping(sak, tab + "   ");
    }
    else
    {
	this_player()->catch_tell("(" + type_name(sak) + ") ");
	if (objectp(sak))
	    this_player()->catch_tell(file_name(sak));
	else if (floatp(sak))
	    this_player()->catch_tell(ftoa(sak));
	else
	    this_player()->catch_tell(sprintf("%O",sak));
    }
    this_player()->catch_tell("\n");
}

/*
 * Function:    secure_var
 * Description: Return a secure copy of the given variable
 * Arguments:   var - the variable
 * Returns:     the secured copy
 */
mixed
secure_var(mixed var)
{
    if (pointerp(var) ||
	mappingp(var))
    {
	return map(var, secure_var);
    }
    return var;
}

#ifdef _FUNCTION
/*
 * Function name: composite_util
 * Description  : Returns the composition of function f applied to the result
 *                of function g applied on variable x.
 * Arguments    : function f - the outer function in the composition.
 *                function g - the inner function in the composition.
 *                mixed x    - the argument to perform the functions on.
 * Returns      : mixed - the result of the composition.
 */
static mixed 
compose_util(function f, function g, mixed x)
{
    return f(g(x));
}

/*
 * Function name: compose
 * Description:   The returns the composition of two functions
 *                (in the mathematical sense).  So if
 *                h = compose(f,g)  then  h(x) == f(g(x))
 * Arguments:     f, g: the functions to compose.
 * Returns:       a function which is the composition of f and g.
 */
function
compose(function f, function g)
{
    return &compose_util(f,g);
}

/*
 * Function name: apply_array
 * Description:   apply a function to an array of arguments
 * Arguments:     f: the function, v: the array
 * Returns:       f(v1,...vn) if v=({v1,...vn})
 */
mixed
applyv(function f, mixed *v)
{
    function g = papplyv(f, v);
    return g();
}

/*
 * Function name: for_each
 * Description:   For each of the elements in the array 'elements', for_each
 *                calls func with it as as parameter.
 *                This is the same functionality as the efun map, but without
 *                the return value.
 * Arguments:     mixed elements      (the array/mapping of elements to use)
 *                function func       (the function to recieve the elements)
 */
void
for_each(mixed elements, function func)
{
    int i;
    int sz;
    mixed arr;

    arr = elements;
    if (mappingp(elements))
        arr = m_values(elements);
    sz = sizeof(arr);
    for(i = 0; i < sz; i++)
        func(arr[i]);
}

/*
 * Function name: constant
 * Description:   returns the first argument and ignores the second
 * Arguments:     x, y
 * Returns:       x
 */
mixed
constant(mixed x, mixed y)
{
    return x;
}

/*
 * Function name: identity
 * Description:   returns its argument unmodified
 * Arguments:     x
 * Returns:       x
 */
mixed
identity(mixed x)
{
    return x;
}


/*
 * Function name: not
 * Description:   returns the locigal inverse of the argument
 * Arguments:     x
 * Returns:       !x
 */
int 
not(mixed x)
{
    return !x;
}

/*
 * Function name: mkcompare_util
 * Description:   Compare two items and return a value meaningful for
 *                sort_array() (that is, 1, 0, or -1).
 * Arguments:     function f            - If specified, the comparison values
 *                                        will be derived by calling this
 *                                        function rather than simply comparing
 *                                        items directly.
 *                function compare_func - A function that determines how the
 *                                        items will be compared.  This is
 *                                        typically &operator(<)().
 *                mixed x               - The first item to compare.
 *                mixed y               - The second item to compare.
 * Returns:       1, 0, or -1, as determined by compare_func
 */
int
mkcompare_util(function f, function compare_fun, mixed x, mixed y)
{
    if (f)
    {
    	x = f(x);
    	y = f(y);
    }

    if (compare_fun(x, y))
    {
        return -1;
    }

    if (compare_fun(y, x))
    {
        return 1;
    }

    return 0;
}

/*
 * Function name: mkcompare
 * Description:   Takes a normal comparison function, like < and returns a
 *                pointer to the mkcompare_util() function, which is suitable
 *                for use with sort_array().
 *                See mkcompare_util() above.
 * Arguments:     function f - an optional function pointer argument.  If
 *                             specified, the comparison will be done on the
 *                             values derived from this function.  If not
 *                             specified, the objects to be sorted will be
 *                             compared directly.
 *                function compare_fun - This function determines how the
 *                                       sorted items are to be compared.  It
 *                                       defaults to &operator(<)().
 * Returns:       A function pointer useful with sort_array().
 */
varargs function
mkcompare(function f, function compare_fun = &operator(<)())
{
    return &mkcompare_util(f, compare_fun);
}

#endif _FUNCTION
