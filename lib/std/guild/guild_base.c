/*
 * This is the standard guild shadow functions that are common to all guild
 * shadows. 
 */

/*
 * Functions in this object.
 *
 *
 * Arguments:    s = string, i = integer, o = object
 *
 * query_guild_not_allow_join_guild(s, s, s) Function called when trying to 
 *			   shadow the player with a guild shadow. It calls each
 *			   query_allow_join_xxx(), described above.
 *
 * shadow_me(o, s, s, s)   Call this function to set up the guild shadow and
 *			   everything needed in the player. Return 1 if
 *			   success. See function below for the error codes.
 *
 * query_guild_keep_player(o) Will be called when the shadow autoloads when a
 * 			   player log on. This is a good opportunity to check
 *			   the player and expell him if you want. This function
 * 			   is ofcourse redefinable by you.
 * query_guild_member(s)   If you want to know if a player is member of a 
 *			   specific guild (type or name) you can call this 
 * 			   function with the appropriate string.
 *
 * query_guild_style(s)    If you want to know if a player is member of a 
 *			   specific guild style you can call this 
 * 			   function with the appropriate string.
 *
 * list_major_guilds()     Returns a nice string with all the guild data.
 *
 * query_skill_name(i)     Returns the name of a special skill.
 */
#pragma strict_types

inherit "/std/shadow";

#include <macros.h>
#include <ss_types.h>

/*
 * Function name: query_not_allow_join_guild
 * Description:   Do the other guild that we are already member of allow you to
 * 	   	  join this one too?
 * Returns:       1 if allowed to join
 */
public nomask int
query_guild_not_allow_join_guild(object player, string type,
			string style, string name)
{
    if ((player->query_guild_not_allow_join_occ(player, type, style, name)) ||
	(player->query_guild_not_allow_join_lay(player, type, style, name)) ||
	(player->query_guild_not_allow_join_race(player, type, style, name)))
	return 1;
    else
	return 0;
}

/*
 * Function name: shadow_me
 * Description:   This function is called when you want a person to join a
 *		  guild, It sets up all necessary variabels and auto load in
 *		  the player
 * Arguments:	  player - the player
 *		  type   - the type of guild
 *		  style  - the guild style
 * 		  name   - the guild name
 * 		  init_arg  - the shadow autoload init string
 * Returns:       1 - everything went right
 *                0 - no player or player already shadowed
 * 		 -1 - no name of guild
 *               -2 - not correct guild type
 *               -3 - no style of guild reported
 *               -4 - the other guilds don't like this guild
 */
public nomask varargs int
shadow_me(object player, string type, string style, string name,
          string init_arg)
{
    int il;

    notify_fail("No guild name set.\n");
    if (!name)
	return -1;

    notify_fail("Not a valid type of guild.\n");
    if (!type ||
	((il = member_array(type, ({"occupational", "layman" , "race"}))) < 0))
	return -2;

    notify_fail("No style set.\n");
    if (!style)
	return -3;

    notify_fail("The guilds you have already joined don't want you in this " +
	"guild.\n");
    if (query_guild_not_allow_join_guild(player, type, style, name))
	return -4;

    notify_fail("Something is seriously wrong with the guild shadow.\n");
    if (!::shadow_me(player))
	return 0;

    player->add_autoshadow(MASTER + ":" + init_arg);

    call_other(this_object(),
	       "init_" + ({ "occ", "lay", "race" })[il] + "_shadow", init_arg);

    player->set_guild_pref(({SS_OCCUP, SS_LAYMAN, SS_RACE})[il],
			   call_other(this_object(),
				      "query_guild_tax_" + 
				      ({"occ", "lay", "race"})[il])); 
    return 1;
}

/* function name: query_guild_keep_player
 * Description:   This gives you a chance to tell if you want to keep the
 *		  the player when the shadow is autoloading when a player
 *		  logs on.
 * Arguments:     player - the player
 * Returns:       1 - keep player, 0 - kick him out.
 */
static int
query_guild_keep_player(object player) { return 1; }

/*
 * Function name: query_guild_member
 * Description:   Is the player already member of an occuptional or layman or
 * 		  race guild? Minor guilds or name of guild also works.
 * Arguments:     str - the type of guild to search for, or guild name
 * Returns:       1 if the shadowed player was member :)
 */
public nomask int
query_guild_member(string str)
{
    int i;

    switch (str)
    {
    case "occupational":
	i = this_object()->query_guild_member_occ();
	break;
    case "layman":
	i = this_object()->query_guild_member_lay();
	break;
    case "race":
	i = this_object()->query_guild_member_race();
	break;
    default:
	if ((str == this_object()->query_guild_name_occ()) ||
	    (str == this_object()->query_guild_name_lay()) ||
	    (str == this_object()->query_guild_name_race()))
		i = 1;
	else
		i = 0;
    }

    return i;
}

/*
 * Function name: query_guild_style
 * Description:   Is the player member of a guild with this style?
 * Arguments:     str - the style to look for
 * Returns:       1 if the member is member of a guild of that style
 */
public nomask int
query_guild_style(string str)
{
    int i;

    if ((str == this_object()->query_guild_style_occ()) ||
	(str == this_object()->query_guild_style_lay()) ||
	(str == this_object()->query_guild_style_race()))
		i = 1;
	else
		i = 0;
    
    return i;
}

/*
 * Function name: list_major_guilds
 * Description:   List the major guilds this player is member of
 * Returns:	  A string holding all the information
 */
public nomask string
list_major_guilds()
{
    string str;
    int ttax;

    str = "";
    if (this_object()->query_guild_member_occ())
	str += sprintf("Occupational guild: %-25s Tax: %3d\n",
	    this_object()->query_guild_name_occ(),
	    this_object()->query_learn_pref(SS_OCCUP));
    else
	str += "No occupational guild\n";

    if (this_object()->query_guild_member_lay())
	str += sprintf("Layman guild:       %-25s Tax: %3d\n",
	    this_object()->query_guild_name_lay(),
	    this_object()->query_learn_pref(SS_LAYMAN));
    else
	str += "No layman guild\n";

    if (this_object()->query_guild_member_race())
	str += sprintf("Race guild:         %-25s Tax: %3d\n",
	    this_object()->query_guild_name_race(),
	    this_object()->query_learn_pref(SS_RACE));
    else
	str += "No race guild\n";

    ttax = this_object()->query_learn_pref(SS_OCCUP) + 
	   this_object()->query_learn_pref(SS_LAYMAN) +
	   this_object()->query_learn_pref(SS_RACE);

    str += "The total tax to pay: " + ttax + ".\n";

    return str;
}

public nomask string
list_mayor_guilds() { return list_major_guilds(); }

/*
 * Function name: query_guild_skill_name
 * Description:   When a players uses the skills command he should get the
 *                the skill names, not the 'special' string.
 * Arguments:     type - the number of the skill
 * Returns:       0 if no skill of mine, else the string.
 */
public mixed
query_guild_skill_name(int type) {}

/*
 * Function name: query_skill_name
 * Description:   When a players uses the skills command he should get the
 *                the skill names, not the 'special' string.
 * Arguments:     type - the number of the skill
 * Returns:       0 if no guild skill, else the string.
 */
public nomask mixed
query_skill_name(int type)
{
    mixed name;
    if (!(name = query_guild_skill_name(type)))
	return shadow_who->query_skill_name(type);

    return name;
}
