/* 
 * /lib/keep.c
 *
 * This small module will enable the object it is inherited into to become
 * 'keepable'. This means that the player holding the object will have the
 * ability to set OBJ_M_NO_SELL flag in the object, indicating that he or
 * she wants to hold onto this object even if commands like 'sell all' are
 * given. In order to make the object 'keepable', all the wizard has to do
 * is inherit this module into his/her code. By default the object will not
 * be 'kept' though. To make the object 'kept' when it is cloned, the
 * function:
 *
 *     set_keep();   or   set_keep(1);
 *
 * must be called from the create function. As you see, set_keep() without
 * argument will be defaulted to 'keep' the object. To remove the keep
 * protection again, the function:
 *
 *     set_keep(0);
 *
 * can be called in the object. The player can call these functions by using
 * the mudlib-commands 'keep' and 'unkeep', defined in /cmd/live/things.c
 *
 * If the object in which this module is inherited is recoverable, it is
 * nice if the 'keep' status is recovered too. To do so, you have to make
 * the following calls from your query_recover() and init_recover()
 * functions.
 *
 * string
 * query_recover()
 * {
 *     return MASTER + ":" + <other recover stuff> + query_keep_recover();
 * }
 *
 * void
 * init_recover(string arg)
 * {
 *     <other recover stuff>
 *     init_keep_recover(arg);
 * }
 */

#pragma save_binary
#pragma strict_types

#include <language.h>
#include <stdproperties.h>

/* 
 * Function name: query_keepable
 * Description  : This function will always return true to signal that the
 *                object it is called in is indeed keepable. Notice that this
 *                does not say anything on whether the object is actually
 *                keep-protected at this point.
 * Returns      : int 1 - always.
 */
nomask int
query_keepable()
{
    return 1;
}

/*
 * Function name: keep_obj_m_no_sell
 * Description  : This function is called by VBFC from the OBJ_M_NO_SELL
 *                property if this object is 'keep' protected. If will return
 *                a proper fail message.
 * Returns      : string - the fail message.
 */
public string
keep_obj_m_no_sell()
{
    return sprintf("Nie mozesz sprzedac %s, gdyz %s przed tym " +
        "zabezpiecz%s. Musisz %s wpierw 'odbezpieczyc'.\n",
        this_object()->short(PL_DOP), this_object()->koncowka("jest on",
        "jest ona", "jest ono", "sa oni", "sa one"),
        this_object()->koncowka("ony", "ona", "one", "eni", "one"),
        this_object()->koncowka("go", "ja", "je", "ich", "je"));
}

/*
 * Function name: set_keep
 * Description  : Call this function in order to set or remove the 'keep'
 *                protection of this object. If no argument is passed to the
 *                function, the default will be 1 - i.e. set the 'keep'
 *                protection.
 * Arguments    : int 1 - set the 'keep' protection.
 *                    0 - remove the 'keep' protection.
 */
public void
set_keep(int keep = 1)
{
    if (keep)
    {
	this_object()->add_prop(OBJ_M_NO_SELL, keep_obj_m_no_sell);
    }
    else
    {
	this_object()->remove_prop(OBJ_M_NO_SELL);
    }
}

/*
 * Function name: remove_keep
 * Description  : Call this function in order to remove the 'keep' protection
 *                of this object. This is the same as doing: set_keep(0);
 */
public void
remove_keep()
{
    set_keep(0);
}

/* 
 * Function name: query_keep
 * Description  : Call this function to query the current 'keep' status of
 *                this object.
 * Returns      : int 1/0 - 'keep' protected or not.
 */
public int
query_keep()
{
    return (this_object()->query_prop_setting(OBJ_M_NO_SELL) != 0);
}

/*
 * Function name: query_keep_recover
 * Description  : This function will return the keep-recovery string.
 * Returns      : string - the keep-recovery string.
 */
public nomask string
query_keep_recover()
{
    /* We only need to add a recover string if the object is 'kept'. */
    if (query_keep())
    {
	return "~Kp~";
    }

    return "";
}

/*
 * Function name: init_keep_recover
 * Description  : After recovering this object, the recovery argument needs
 *                to be checked and the 'keep' protection needs to be
 *                enabled if the player had 'kept' the item before the
 *                reboot.
 * Arguments    : string arg - the recovery argument.
 */
public nomask void
init_keep_recover(string arg)
{
    set_keep(wildmatch("*~Kp~*", arg));
}
