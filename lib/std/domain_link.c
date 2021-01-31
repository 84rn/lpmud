/*
 * /std/domain_link.c
 *
 * This module should be inherited by all domain masters and master objects
 * of the independant wizards. It takes care of the preloading and it
 * provides a link for some other general things.
 */

#pragma no_clone
#pragma save_binary
#pragma strict_types

private static string *gPreload = ({});

/*
 * Function name: create
 * Description  : Constructor.
 */
public nomask void
create()
{
    setuid();
    seteuid(getuid());
}

/*
 * Function name: preload
 * Description  : This function should be called from the preload_link()
 *                function of the domain. It enables the domain to load some
 *                modules at boot time.
 * Arguments    : string file - the filename of the file to preload.
 */
nomask void
preload(string file)
{
    gPreload += ({ file });
}

public nomask string *
query_preload()
{
    return secure_var(gPreload);
}

/*
 * Function name: preload_link
 * Description  : This function should be masked by domains that want to
 *                preload some files at boot time. It should contain only
 *                calls to the function preload().
 */
public void
preload_link()
{
}

/*
 * Function name: armageddon
 * Description  : This function is called from SECURITY when it is time to
 *                close down the game. Note that this function should only
 *                be used for some basic domain administration as all domains
 *                should be processed in one run.
 */
public void
armageddon()
{
}
