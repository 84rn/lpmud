/*
 * /std/receptacle.c
 *
 * This is a default that has code to support "open" and "close" of containers
 * in general. Furthermore, there is also support for if you want to use a key
 * to "lock" or "unlock" the container. In addition it is possible to "pick"
 * the lock on the container if it has a lock.
 *
 * Note that this code only supports manipulation of objects that inherit
 * this code, and it cannot be used to handle other containers that do not
 * inherit this code.
 *
 * If you want to add your a special check before you manipulate a container,
 * or do something else, you can add extra code. This code should be in the
 * following functions:
 *
 *   varargs int open(object ob)
 *   varargs int close(object ob)
 *   varargs int lock(object ob)   Only if the container has a lock
 *   varargs int unlock(object ob) Only if the container has a lock
 *   varargs int pick(object ob)   Only is the lock is pickable
 *
 * These functions are called with the objectpointer to the currently handled
 * container and are optional to use. To specify in which files the functions
 * can be found you have to use:
 *
 *   void set_cf(object o) Set the object that defines open(), close() etc.
 *
 * The special functions should return 0 if there is no change to the
 * manipulation of the containers, 1 if they can be handled, but the message
 * is written inside the function and 2 if it is NOT possible to handle the
 * container and the default fail message will be displayed and finally 3 if
 * it is NOT possible to manipulate and a fail message will be printed inside
 * the function. Obviously, this_player() is known in those functions.
 *
 * To handle the lock on the container there are some functions to use:
 *
 *   void set_key(mixed keyvalue)  Set the value of the key
 *   void set_pick(int pick_level) Set the pick level
 *   void set_no_pick(1/0)         Set for unable to pick
 *
 * The default pick level is set to 40. You may set the pick level in the
 * range [1..99]. The pick level will be checked against a value that
 * is randomized from the players pick-skill. A value outside the interval
 * will make the lock unpickable. In addition you may set_no_pick to ensure
 * that the lock is not pickable.
 *
 * There are query_ functions for all set_ functions.
 *
 * NOTE
 *
 * You will get ugly plural short descriptions if you do not set the plural
 * short description yourself. If you do not set it, the plural short
 * description might look like "chest (open)s" for I add status information
 * on the container in the short, pshort and long description!
 *
 * ADVISE
 *
 * Do not redefine the functions short, pshort and long in your code. You
 * should better use set_short, set_pshort and set_long with VBFC in the call
 * to get a better result. Note that are able to do exactly the same things
 * with VBFC redefinitions of the functions. If you fail to do this, my
 * definitions of short, pshort and long will be lost, which means that
 * players will not get status information on their containers any more ;-(
 *
 * RECOVERY
 *
 * If you are going to make your container recoverable, and why should you
 * not do that, add the following functions and do not forget to
 * #include <macros.h> for the definition of MASTER.
 *
 *   string
 *   query_recover()
 *   {
 *       return MASTER + ":" + my_recover_args + query_container_recover();
 *   }
 *
 *   void
 *   init_recover(string arg)
 *   {
 *       my_init_recover_code();
 *       init_container_recover(arg);
 *   }
 */
#pragma save_binary
#pragma strict_types

inherit "/std/container";

#include <cmdparse.h>
#include <files.h>
#include <macros.h>
#include <ss_types.h>
#include <stdproperties.h>

#define MANA_TO_PICK_LOCK      10
#define PICK_SKILL_DIFFERENCE  30
#define HANDLE_KEY_FAIL_STRING " co czym?"

/*
 * A few prototypes. I hate them, but they are handy to keep the code clean.
 */
//public nomask void   add_temp_libcontainer_checked(object player);
//public nomask void   remove_temp_libcontainer_checked(object player);
public nomask int    filter_open_close_containers(object ob);
public nomask varargs object * normal_access(string str, string pattern,
    string fail_str);
public nomask mixed  normal_access_key(string str);
public int           do_default_open(string str);
public int           do_default_close(string str);
public int           do_default_lock(string str);
public int           do_default_unlock(string str);
//public int           do_default_pick(string str);

/*
 * Global variables
 */
static object cont_func;   /* the object defining open, close etc.          */
static mixed  cont_key;    /* the identifier to the key to open this object */
static int    cont_pick = 40; /* how hard it is to pick the lock            */

/*
 * Function name: create_receptacle
 * Description  : Creator. You should define this function in order to set
 *                up the object. However, as this is just a container with
 *                some extra functionality, you can also use the function
 *                create_container().
 */
void
create_receptacle()
{
}

/*
 * Function name: create_container
 * Description  : Since this object is called /std/receptacle, you can
 *                use create_receptacle to create it. Also, as normal,
 *                create_container can be used, since this is just a
 *                container with some extra functions.
 */
void
create_container()
{
    create_receptacle();
}

/*
 * Function name: reset_receptacle
 * Description  : If you want to make this receptacle reset at a regular
 *                interval this is the call to make. As this is just a
 *                container with some extra functionality you can also use
 *                the function reset_container(). Do not forget that you
 *                must call enable_reset() from your create function to
 *                actually make this object reset.
 */
void
reset_receptacle()
{
}

/*
 * Function name: reset_container
 * Description  : If you create the container with create_receptacle,
 *                you shall want to reset it with reset_receptacle and
 *                this function allows you to do just that. Do not forget
 *                that you have to call enable_reset() from your create
 *                function in order to make this object actually reset.
 */
void
reset_container()
{
    reset_receptacle();
}

/*
 * Function name: init
 * Description  : Link the commands to manipulate the container to the
 *                player.
 */
public void
init()
{
    ::init(); /* always make the call to the previous definitions. */

    add_action(do_default_open,  "otworz");
    add_action(do_default_close, "zamknij");

    if (cont_key)
    {
        add_action(do_default_lock,   "zamknij");
        add_action(do_default_unlock, "otworz");
#if 0
        if (cont_pick != 0)
        {
            add_action(do_default_pick, "pick");
        }
#endif
    }
}

/*
 * Function name: real_short
 * Description  : Returns the short without the addition of open/close
 * Arguments    : for_obj - who wants to know the short
 * Returns      : The short description
 */
public varargs string
real_short(mixed for_obj, int przyp)
{
    return ::short(for_obj, przyp);
}

/*
 * Function name: short
 * Description  : Add the status of the container to it.
 * Arguments    : for_obj - who wants to know the short
 * Returns      : The short description.
 */
public varargs string
short(mixed for_obj, mixed przyp)
{
    int closed;
    
    if (!objectp(for_obj))
    {
        if (intp(for_obj))
            przyp = for_obj;
        else if (stringp(for_obj))
            przyp = atoi(for_obj);
        
        for_obj = previous_object();
    }
    else
        if (stringp(przyp))
            przyp = atoi(przyp);
            
    closed = query_prop(CONT_I_CLOSED);
            
    return oblicz_przym(closed ? "zamkniety" : "otwarty", closed ? 
        "zamknieci" : "otwarci", przyp, query_rodzaj(), query_tylko_mn()) + 
        " " + ::short(for_obj, przyp);
}

/*
 * Function name: pshort
 * Description  : Add the status of the container to it.
 * Arguments    : for_obj - who wants to know the pshort
 * Returns      : The plural short description.
 */
public varargs string
plural_short(mixed for_obj, int przyp)
{
    string str;
    int closed;
    
    if (intp(for_obj)) 
    {
	przyp = for_obj;
	for_obj = this_player();
    }
    /* nie sprawdzamy czy przyp to int, bo nie ma makr do tej funkcji */

    str = ::plural_short(for_obj, przyp);
    
    if (stringp(str))
    {
	closed = query_prop(CONT_I_CLOSED);

	return oblicz_przym(closed ? "zamkniety" : "otwarty", closed ? 
	    "zamknieci" : "otwarci", przyp, query_rodzaj(), 1) + " " + str;
    }
    else
	return str;
}

/*
 * Function name: long
 * Description  : A the status of the container to it.
 * Arguments    : for_obj - who wants to know the long
 * Returns      : The long description.
 */
public varargs string
long(object for_obj)
{
    return ::long(for_obj) + (query_tylko_mn() ? "Sa " : "Jest ") + 
        ((query_prop(CONT_I_CLOSED)) ? "zamknie" : "otwar") + 
        koncowka("ty", "ta", "te", "ci", "te") + ".\n";
}

/*
 * Function name: do_default_open
 * Description  : Try to open one or more chests. This command does not allow
 *                the use of specific tricks to prevent the command parser to
 *                try the open command on each container in his/her presence
 *                for the command should obviously also work on doors and
 *                such.
 *                However, with the use of the TEMP_LIBCONTAINER_CHECKED
 *                property, there will hardly be any loss of cpu-time.
 * Arguments    : str - the rest of the command line by the mortal
 * Returns      : 1 = success, 0 = failure.
 */
public int
do_default_open(string str)
{
    object tp = this_player();
    object *items;
    int    i;
    int    cres;
    int    succes = 0;
    string what;
    string gFail = "";
    string gSucces = "";

    if (!sizeof(items = normal_access(str, "%i:3")))
    {
        return 0;
    }

    for (i = 0; i < sizeof(items); i++)
    {
        what = (string)items[i]->real_short(tp, PL_MIA);

        if (!(items[i]->query_prop(CONT_I_CLOSED)))
        {
            gFail += capitalize(what) + " juz " + 
                (items[i]->query_tylko_mn() ? "sa" : "jest") + " otwar" + 
                items[i]->koncowka("ty", "ta", "te", "ci", "te") + ".\n";
        }
        else if (items[i]->query_prop(CONT_I_LOCK))
        {
            gFail += capitalize(what) + " " + (items[i]->query_tylko_mn() ? 
                "sa" : "jest") + " zamknie" + 
                items[i]->koncowka("ty", "ta", "te", "ci", "te") + 
                " na klucz.\n";
        }
        else
        {
            if (!objectp(items[i]->query_cf()))
            {
                cres = 0;
            }
            else
            {
                cres = (int)((items[i]->query_cf())->open(items[i]));

                if (cres)
                {
                    gFail += capitalize(what) + "nie chc" +
                        (items[i]->query_tylko_mn() ? "a" : "e") + 
                        " sie otworzyc.\n";
                }
            }

            if (!cres)
            {
                what = items[i]->real_short(tp, PL_BIE);
                
		this_player()->set_obiekty_zaimkow(({ items[i] }));
                gSucces += "Otwierasz " + what + ".\n";
                say(QCIMIE(tp, PL_MIA) + " otwiera " + what + ".\n");
                succes = 1;
                items[i]->remove_prop(CONT_I_CLOSED);
            }
        }
    }

    if (succes)
    {
/*        remove_temp_libcontainer_checked(tp); */
        if (strlen(gFail))
        {
            write(gFail);
        }
        if (strlen(gSucces))
        {
            write(gSucces);
        }
        return 1;
    }

    notify_fail(gFail);
    return 0;
}

/*
 * Function name: do_default_close
 * Description  : Try to close one or more chests. This command does not allow
 *                the use of specific tricks to prevent the command parser to
 *                try the close command on each container in his/her presence
 *                for the command should obviously also work on doors and
 *                such.
 *                However, with the use of the TEMP_LIBCONTAINER_CHECKED
 *                property, there will hardly be any loss of cpu-time.
 * Arguments    : str - the rest of the command line by the mortal
 * Returns      : 1 = success, 0 = failure.
 */
public int
do_default_close(string str)
{
    object tp = this_player();
    object *items;
    int    i;
    int    cres;
    int    succes = 0;
    string what;
    string gFail = "";
    string gSucces = "";

    if (!sizeof(items = normal_access(str, "%i:3")))
    {
        return 0;
    }

    for (i = 0; i < sizeof(items); i++)
    {
        what = (string)items[i]->real_short(tp);

        if (items[i]->query_prop(CONT_I_CLOSED))
        {
            gFail += capitalize(what) + " juz " + 
                (items[i]->query_tylko_mn() ? "sa" : "jest") + " zamknie" + 
                items[i]->koncowka("ty", "ta", "te", "ci", "te") + ".\n";
        }
        else
        {
            if (!objectp(items[i]->query_cf()))
            {
                cres = 0;
            }
            else
            {
                cres = (int)((items[i]->query_cf())->close(items[i]));

                if (cres)
                {
                    gFail += capitalize(what) + "nie chc" +
                        (items[i]->query_tylko_mn() ? "a" : "e") + 
                        " sie zamknac.\n";
                }
            }

            if (!cres)
            {
                what = items[i]->real_short(tp, PL_BIE);
		this_player()->set_obiekty_zaimkow(({ items[i] }));                
                gSucces += "Zamykasz " + what + ".\n";
                say(QCIMIE(tp, PL_MIA) + " zamyka " + what + ".\n");
                succes = 1;
                items[i]->add_prop(CONT_I_CLOSED, 1);
            }
        }
    }

    if (succes)
    {
/*        remove_temp_libcontainer_checked(tp); */
        if (strlen(gFail))
        {
            write(gFail);
        }
        if (strlen(gSucces))
        {
            write(gSucces);
        }
        return 1;
    }

    notify_fail(gFail);
    return 0;
}

/*
 * Function name: do_default_lock
 * Description  : Try to lock one container. This command does not allow
 *                the use of specific tricks to prevent the command parser to
 *                try the lock command on each container in his/her presence
 *                for the command should obviously also work on doors and
 *                such.
 *                However, with the use of the TEMP_LIBCONTAINER_CHECKED
 *                property, there will hardly be any loss of cpu-time.
 * Arguments    : str - the rest of the command line by the mortal
 * Returns      : 1 = success, 0 = failure.
 */
public int
do_default_lock(string str)
{
    object tp = this_player();
    object container_item;
    object key_item;
    mixed  items;
    mixed  keyval;
    string qvb = query_verb();

    if (!pointerp(items = normal_access_key(str)))
        return items;
    
    container_item = items[0]; key_item = items[1];
    
    if (container_item->query_prop(CONT_I_LOCK))
    {
        write(capitalize(container_item->real_short(tp)) + " juz " +
            (container_item->query_tylko_mn() ? "sa" : "jest") + 
            " zamknie" + container_item->koncowka("ty", "ta", "te", "ci", 
            "te") + ".\n");
        return 1;
    }

    if ((objectp(container_item->query_cf())) &&
        ((container_item->query_cf())->lock(container_item)))
    {
        write("Nie mozesz zamknac " + container_item->real_short(tp, PL_DOP)
            + ".\n");
        return 1;
    }

    this_player()->set_obiekty_zaimkow(({ container_item }));
    container_item->add_prop(CONT_I_LOCK, 1);
    write("Zamykasz " + container_item->real_short(tp, PL_BIE) + " " +
        key_item->short(tp, PL_NAR) + ".\n");
    say(QCIMIE(tp, PL_MIA) + " zamyka " + container_item->real_short(PL_BIE) + " " +
        QSHORT(key_item, PL_NAR) + ".\n");

    return 1;
}

/*
 * Function name: do_default_unlock
 * Description  : Try to unlock one container. This command does not allow
 *                the use of specific tricks to prevent the command parser to
 *                try the lock command on each container in his/her presence
 *                for the command should obviously also work on doors and
 *                such.
 *                However, with the use of the TEMP_LIBCONTAINER_CHECKED
 *                property, there will hardly be any loss of cpu-time.
 * Arguments    : str - the rest of the command line by the mortal
 * Returns      : 1 = success, 0 = failure.
 */
public int
do_default_unlock(string str)
{
    object tp = this_player();
    object container_item;
    object key_item;
    mixed  items;
    mixed  keyval;
    string qvb = query_verb();

    if (!pointerp(items = normal_access_key(str)))
        return items;
    
    container_item = items[0]; key_item = items[1];

    if (!(container_item->query_prop(CONT_I_LOCK)))
    {
        write(capitalize(container_item->real_short(tp)) + " nie " +
            (container_item->query_tylko_mn() ? "sa" : "jest") + 
            " zamknie" + container_item->koncowka("ty", "ta", "te", "ci", 
            "te") + " na klucz.\n");
        return 1;
    }

    if ((objectp(container_item->query_cf())) &&
        ((container_item->query_cf())->lock(container_item)))
    {
        write("Nie mozesz otworzyc " + container_item->real_short(tp, PL_DOP)
            + ".\n");
        return 1;
    }

    this_player()->set_obiekty_zaimkow(({ container_item }));
    container_item->remove_prop(CONT_I_LOCK);
    write("Otwierasz " + container_item->real_short(tp, PL_BIE) + " " +
        key_item->short(tp, PL_NAR) + ".\n");
    say(QCIMIE(tp, PL_MIA) + " otwiera " + container_item->real_short(PL_BIE) +
        " " + QSHORT(key_item, PL_NAR) + ".\n");

    return 1;
}

#if 0
/*
 * Function name: do_default_pick
 * Description  : Allows someone to try to pick the lock. The players pick
 *                skill will be randomized a little and then checked against
 *                the pick level of this lock.
 * Arguments    : str - the argument to the command line verb
 * Reurns       : 1   - success on picking a lock
 *                0   - on failure
 */
public int
do_default_pick(string str)
{
    object tp = this_player();
    object *items;
    int skill;

    if (!sizeof(items = normal_access(str, " 'lock' 'on' %i ",
        " lock on what?")))
    {
        return 0;
    }

    skill = (int)tp->query_skill(SS_OPEN_LOCK);
    if (skill == 0)
    {
        write("You do not know how to pick a lock.\n");
        return 1;
    }

    if (tp->query_mana() <= MANA_TO_PICK_LOCK)
    {
        write("You cannot concentrate enough to pick a lock.\n");
        return 1;
    }
    tp->add_mana(-(MANA_TO_PICK_LOCK));

    if (!(items[0]->query_prop(CONT_I_LOCK)))
    {
        write("Much to your surprise you find that the " +
            items[0]->real_short(tp) + " is not locked.\n");
        return 1;
    }

    if (items[0]->query_no_pick())
    {
        write("Close examination of the lock on the " +
            items[0]->real_short(tp) +
            " learns you that it is unpickable.\n");
        return 1;
    }

    /*
     * /std/door.c uses total random on the skill and I do not like that. In
     * fact, I do not even like this function. :-(
     */
    skill = ((skill * 3 / 4) + random(skill / 2));

    if ((skill >= items[0]->query_pick()) &&
        (objectp(items[0]->query_cf()) ?
            (!((items[0]->query_cf())->pick(items[0]))) : 1))
    {
        items[0]->remove_prop(CONT_I_LOCK);
        
/*        remove_temp_libcontainer_checked(tp); */
        say(QCTNAME(tp) + " fumbles with the lock on the " +
            items[0]->real_short() +
            ((environment(this_object()) == this_player()) ?
            (" " + tp->query_pronoun() + " carries") : "") +
            " and you hear soft 'click' comming from it.\n");
        write("You feel very satisfied when you hear a soft 'click' from " +
            "the lock on the " + items[0]->real_short(tp) +
            " after you picked it.\n");
        return 1;
    }

/*    remove_temp_libcontainer_checked(tp);*/
    say(QCTNAME(tp) + " fumbles with the lock on the " +
        items[0]->real_short() +
        ((environment(this_object()) == this_player()) ?
        (" " + tp->query_pronoun() + " carries") : "") +
        ". Nothing happens though.\n");
    write("You fail to pick the lock on the " + items[0]->real_short(tp) +
        "." + ((skill + PICK_SKILL_DIFFERENCE < items[0]->query_pick()) ?
        " The lock seems unpickable to you.\n" : "\n"));

    return 1;
}
#endif

/*
 * Function name: set_cf
 * Description  : Allows you to specify an object that holds code that is
 *                called before you try to open a specific object. This code
 *                will be called with:
 *                  open() and
 *                  close()
 *                These functions should return 1 if it is not possible to
 *                open/close the container and 0 if it is oke to manipulate
 *                it.
 * Arguments    : object obj - the object specifying the code.
 */
public void
set_cf(object obj)
{
    /* All changes to the object might have been ruled out. */
    if (query_lock())
    {
        return;
    }

    cont_func = obj;
}

/*
 * Function name: query_cf
 * Description  : Query the object that holds protection code.
 * Returns      : object - the object specifying the code.
 */
public object
query_cf()
{
    return cont_func;
}

/*
 * Function name: set_no_pick
 * Description  : Make sure the lock on the container is not pickable.
 */
public void
set_no_pick()
{
    /* all changes to the object might have been ruled out. */
    if (query_lock())
    {
        return;
    }

    cont_pick = 0;
}

/*
 * Function name: query_no_pick
 * Description  : Returns whether the lock on the container can be picked.
 * Returns      : 1 - lock on container is not pickable
 *                0 - lock on container is pickable
 */
public int
query_no_pick()
{
    return (cont_pick == 0);
}

/*
 * Function name: set_pick
 * Description  : Set the difficulty to pick the lock.
 * Arguments    : i - the pick level
 */
public void
set_pick(int i)
{
    /* all changes to the object might have been ruled out. */
    if (query_lock())
    {
        return;
    }

    if ((i > 0) && (i < 100)) /* a legal pick value */
    {
        cont_pick = i;
        return;
    }

    cont_pick = 0; /* not pickable */
}

/*
 * Function name: query_pick
 * Description  : Returns how easy it is to pick the lock on a door
 * Returns      : int - the pick level
 */
public int
query_pick()
{
    return cont_pick;
}

/*
 * Function name: set_key
 * Description  : Set the number of the key that fits this container.
 * Arguments    : keyval - the key to the container
 */
public void
set_key(mixed keyval)
{
    /* All changes might have been locked out. */
    if (query_lock())
    {
        return;
    }

    cont_key = keyval;
}

/*
 * Function name: query_key
 * Description  : Query the key that fits this container.
 * REturns      : mixed - the key to the container
 */
public mixed
query_key()
{
    return cont_key;
}

/*
 * Function name: normal_access
 * Description  : This function is used by all commands attached to the
 *                container to get an array of objects that the player wanted
 *                to manupulate
 * Arguments    : str      - the argument to the command line verb
 *                pattern  - the pattern to match the argument
 *                fail_str - the string to add to notify_fail on failure
 * Returns      : object * - the objects the player wants to handle
 *                0        - on failure.
 */
public varargs nomask object *
normal_access(string str, string pattern, string fail_str)
{
    object tp = this_player();
    object *items;

#if 0
    /* No access on another container, so don't bother to check this one. */
    if (tp->query_prop(TEMP_LIBCONTAINER_CHECKED))
    {
        return 0;
    }
#endif


/*    add_temp_libcontainer_checked(tp); */
    if (!CAN_SEE_IN_ROOM(tp))
    {
        notify_fail("Jest zbyt ciemno, zeby to zrobic.\n");
        return 0;
    }

    notify_fail(capitalize(query_verb()) +
        (strlen(fail_str) ? fail_str : " co?") + "\n");
    if (!strlen(str))
    {
        return 0;
    }

    if (!sizeof(items =
    	filter((all_inventory(environment(tp)) + all_inventory(tp)),
    	       filter_open_close_containers)))
    {
        return 0;
    }

    if (fail_str == HANDLE_KEY_FAIL_STRING)
    {
        /* this means that this function is just used for basic access */
        return ({ tp });
    }

    if (!(parse_command(str, items, pattern, items)))
    {
        return 0;
    }
    if (!sizeof(items = NORMAL_ACCESS(items, 0, 0)))
    {
        return 0;
    }

    return items;
}

/*
 * Function name: normal_access_key
 * Description  : This function is used by all commands attached to the
 *                container to get an array of objects that the player wanted
 *                to manupulate and the keys he wants to manipulate the
 *                containers with.
 * Arguments    : str   - the argument to the command line verb
 * Returns      : mixed - the objects the player wants to handle and what he
 *                        wants to handle them with in an array of arrays:
 *                        ({ ({ container_items }), ({ key_items }) })
 *                0     - on failure.
 */
public nomask mixed
normal_access_key(string str)
{
    object *container_items;
    object *key_items;
    int keyval;

    notify_fail(capitalize(query_verb()) + HANDLE_KEY_FAIL_STRING + "\n");
    
    if (!CAN_SEE_IN_ROOM(this_player()))
    {
        notify_fail("Jest zbyt ciemno, zeby to zrobic.\n");
        return 0;
    }


    if (!parse_command(str, environment(this_player()), "%i:3 %i:4",
        container_items, key_items))
    {
        return 0;
    }
    
    key_items = NORMAL_ACCESS(key_items, 0, this_object());
    
    if (!key_items || !sizeof(key_items))
        return 0;
        
     container_items = NORMAL_ACCESS(container_items, 
         "filter_open_close_containers", this_object());

    if (!container_items || !sizeof(container_items))
        return 0;
        
    if ((sizeof(container_items) > 1) || (sizeof(key_items) > 1))
    {
        write("Musisz konkretniej sprecyzowac, o co ci chodzi.\n");
        return 1;
    }

    if (!(keyval = container_items[0]->query_key()))
    {
        write(capitalize(container_items[0]->real_short(this_player())) +
            " nie ma" + (container_items[0]->query_tylko_mn() ? "ja" : "") +
            " zamka.\n");
        return 1;
    }

    if (keyval != key_items[0]->query_key())
    {
        write(capitalize(key_items[0]->short(PL_MIA)) + " nie pasuje.\n");
        return 1;
    }

    return ({ container_items[0], key_items[0] });
}

/*
 * Function name: filter_open_close_containers
 * Description  : Filter to get only the containers that inherit this file as
 *                well for we don't want to close a container that does not
 *                have an open command.
 * Arguments    : ob - the object to process
 * Returns      : 1 - this container is oke.
 *                0 - apparently it is not a good container.
 */
public nomask int
filter_open_close_containers(object ob)
{
    return (function_exists("filter_open_close_containers", ob) ==
    	RECEPTACLE_OBJECT);
}

/*
 * Function name: add_temp_libcontainer_checked
 * Description  : If a player handled a container and failed, he will not
 *                try to handle other containers in the same heartbeat.
 * Arguments    : player - the player to add it to.
 */
public nomask void
add_temp_libcontainer_checked(object player)
{
    set_alarm(1.0, 0.0, &player->remove_prop(TEMP_LIBCONTAINER_CHECKED));
    player->add_prop(TEMP_LIBCONTAINER_CHECKED, 1);
}

/*
 * Function name: remove_temp_libcontainer_checked
 * Description  : If a player handled a container and failed, he will not
 *                try to handle another containers in the same heartbeat.
 * Arguments    : player - the player to remove it from
 */
public nomask void
remove_temp_libcontainer_checked(object player)
{
    player->remove_prop(TEMP_LIBCONTAINER_CHECKED);
}

/*
 * Function name: stat_object
 * Description  : This function is called when a wizard wants to know more
 *                about the container.
 * Returns      : string - the stats of the container.
 */
public string
stat_object()
{
    string str = ::stat_object();

    if (cont_key)
    {
        str += "Kod klucza : \"" + cont_key + "\"\n";

        if (cont_pick == 0)
        {
            str += "Zamek nie to wlamania.\n";
        }
        else
        {
            str += "Pick level : " + cont_pick + "\n";
        }
    }
    else
    {
        str += "Nie ma zamka.\n";
    }

    return str;
}

/*
 * Function name: get_pick_chance
 * Description  : Make a string from the difference in pick-skill of the
 *                player and pick-level of the container for the appraise
 *                by the player
 * Arguments    : pick_val - the mentioned difference
 * Returns      : string   - a nice string with the mentioned description.
 */
public nomask string
get_pick_chance(int pick_val)
{
    if (pick_val >= 40)
        return "zamek da sie otworzyc przez samo spojrzenie na niego";
    else if (pick_val >= 30)
        return "zamek jest bardzo latwo otworzyc";
    else if (pick_val >= 20)
        return "zamek jest calkiem latwo otworzyc";
    else if (pick_val >= 10)
        return "zamek jest latwo otworzyc";
    else if (pick_val >= 0)
        return "zamek da sie otworzc";
    else if (pick_val >= -10)
        return "zamek da sie otworzyc z pewnymi trudnosciami";
    else if (pick_val >= -20)
        return "otwarcie zamka moze nastreczyc troche trudnosci";
    else if (pick_val >= -30)
        return "zamek jest bardzo ciezko otworzyc";
    else if (pick_val >= -40)
        return "zamka prawie na pewno nie da sie otworzyc";

    /* pick_val apparently < -40 */
    return "zamka nie da sie w zaden sposob otworzyc";
}

#if 0
/*
 * Function name: appraise_object
 * Description  : This function is called when a player appraises the
 *                container to find out more about it.
 * Arguments    : num - use this num rather than the players appraise skill
 */
public varargs void
appraise_object(int num)
{
    int pick_level = cont_pick;
    int seed;
    int skill;

    ::appraise_object(num);

    if (!cont_key)
    {
        return;
    }

    if (!num)
    {
	skill = (int)this_player()->query_skill(SS_APPR_OBJ);
    }
    else
    {
	skill = num;
    }

    sscanf(OB_NUM(this_object()), "%d", seed);
    skill = random((1000 / (skill + 1)), seed);
    pick_level = (int)this_player()->query_skill(SS_OPEN_LOCK) -
        cut_sig_fig(pick_level + (skill % 2 ? -skill % 70 : skill) *
	pick_level / 100, 2);

    write ("Masz wrazenie, ze " + get_pick_chance(pick_level) +
        ".\n");
}
#endif

/*
 * Function name: query_container_recover
 * Description  : Return the recover string with information on the status
 *                of the container.
 * Returns      : part of recover string
 */
public nomask string
query_container_recover()
{
    return ("#c_c#" + query_prop(CONT_I_CLOSED) + "#" +
            "#c_l#" + query_prop(CONT_I_LOCK)   + "#");
}

/*
 * Function name: init_container_recover
 * Description  : Initialize the container to the status it had before the
 *                reboot.
 * Arguments    : arg - the total recover string
 */
public nomask void
init_container_recover(string arg)
{
    string foobar;
    int    tmp;

    sscanf(arg, "%s#c_c#%d#%s", foobar, tmp, foobar);
    add_prop(CONT_I_CLOSED, tmp);
    sscanf(arg, "%s#c_l#%d#%s", foobar, tmp, foobar);
    add_prop(CONT_I_LOCK, tmp);
}
