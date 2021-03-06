/*
 * This is the standard guild shadow for an layman guild. It defines
 * The necessary functions.
 *
 *     Made by Nick
 */

/*
 * Functions in this object.
 *
 *
 * Arguments:    s = string, i = integer, o = object
 *
 * query_guild_tax_lay()   Should return the tax we want the player to pay.
 *			   If the tax is to change, i.e. the player learns a
 *			   new spell, call notify_guild_tax_changed() to set
 *			   the tax correctly.
 *
 * query_guild_type()      Always returns "layman"
 *
 * query_guild_style_lay() What styles are we? fighter, magic, cleric, thief
 *			   are the standard styles but there are other styles 
 *			   aren't there, perhaps we should enlarge the list.
 *
 * query_guild_name_lay()  The name of the guild.
 *
 * query_guild_title_lay() This is called from the player object to find
 *                         the title of this guild.
 *
 * query_guild_not_allow_join_occ(o, s, s, s) This is called when our member
 *			   tries to join another guild. The type, style and 
 *			   name of the other guild is sent as argument and we
 *			   can deny our member to join that guild if we want.
 *			   Observe, returning 1 will deny the player. This
 *			   function will also be called each time a player 
 *			   logs on, we could test if the race has changed or
 *			   anything....
 *
 * query_guild_member_lay() Is always true, ofcourse.
 *
 * remove_guild_lay()      Call this if you are removeing your guild from the 
 *			   player, and you are an layman guild.
 *
 * autoload_shadow(s)      Is called when a player enters and we shall load
 *			   the autoloading shadows.
 *
 * query_guild_trainer_lay() Returns a reference to the file or files that
 *			   defines sk_query_max for the player in question.
 *			   The returned items could be a string, an object
 *			   pointer or an array of the same.
 *
 * Note: You will probably want to redefine query_guild_tax_lay() and
 *       query_guild_not_allow_join_lay(s, s, s).
 *
 */
#pragma strict_types

inherit "/std/guild/guild_base";

#include <macros.h>
#include <ss_types.h>

/*
 * Function name: query_guild_tax_lay
 * Description:   What's the tax to pay at this guild? You should write your
 *                own version of this function.
 * Returns:       the tax this guild wants
 */
public int
query_guild_tax_lay() { return 0; }

/*
 * Function name: query_guild_type
 * Description:   what type of guild is this, occupational, layman or race?
 * Returns:       a string with the type
 */
public nomask string
query_guild_type() { return "layman"; }

/*
 * Function name: query_guild_style_lay
 * Description:   What styles is this guild? fighter, magic, cleric, thief or  ?
 * Returns:       string - holding the style of guild
 */
public string
query_guild_style_lay() { return "magic"; }

/*
 * Function name: query_guild_name_lay
 * Description:   Returns the name of the guild this shadow represents
 * Returns:	  The name of the guild
 */
public string
query_guild_name_lay() { return "layman"; }

/*
 * Function name: query_guild_title_lay
 * Description  : This function should return the layman title off
 *                the player. Since this function is not called for wizards
 *                you do not have to distinct for them in this function.
 * Returns      : string - the title for the player.
 *                The title will be incorporated in the format
 *                "the <race title>, <occ title> and <lay title>"
 *                if the player is a member of all major guild types.
 */
public string
query_guild_title_lay() { return ""; }

/*
 * Function name: query_allow_join_lay
 * Description:   Test if this guild allowed a player to join another
 * Arguments:     type  - the type of guild to join
 * 		  style - the style of the guild
 *		  name  - the name
 * Returns:       1 - I allow member to join another guild
 */
public int
query_guild_not_allow_join_lay(object player, string type, string style,
			       string name)
{
    notify_fail("One layman guild is enough.\n");
    if (type == "layman")
	return 1;

    return 0;
}

/*
 * Function name: query_guild_member_lay
 * Description:   This is an layman guild
 * Returns:	  1
 */
public nomask int
query_guild_member_lay() { return 1; }

/*
 * Function name: query_guild_trainer_lay
 * Description:   Return one or more references to the object that define
 *                sk_train_max for the player. The returned refernce can
 *                be a string reference, an object pointer or an array of
 *                those.
 * Returns:       See description.
 */
public mixed
query_guild_trainer_lay()
{
    return 0;
}

/*
 * Function name: remove_guild_lay
 * Description:   Remove a player from the guild. Will take care of everything.
 * Returns:       1 if removed
 */
public nomask int
remove_guild_lay()
{
    if (!shadow_who->remove_autoshadow(MASTER + ":"))
	return 0;

    shadow_who->set_guild_pref(SS_LAYMAN, 0);
    remove_shadow();
    return 1;
}

/*
 * Function name: init_lay_shadow()
 * Description: This function is called from autoload_shadow and may
 *              be used to initialize the shadow when it's loaded.
 * Arguments: The argument string sent to autoload_shadow.
 */
public void
init_lay_shadow(string arg) {}

/*
 * Function name: autoload_shadow
 * Description:   Called by the autoloading routine in /std/player_sec
 *		  to ensure autoloading of the shadow.
 * Arguments:	  str - the string holding type, style and name of the guild
 */
public nomask void
autoload_shadow(string str)
{
    if (shadow_who)
	return; /* Already shadowing */

    if (query_guild_not_allow_join_guild(this_player(), "layman",
		query_guild_style_lay(), query_guild_name_lay()))
    {
	write("Your guilds don't seem to get along. Your layman guild " +
	      "is removed.\n");
	this_player()->remove_autoshadow(MASTER);
	return;
    }

    if (!query_guild_keep_player(this_player()))
    {
	this_player()->remove_autoshadow(MASTER);
	return;
    }

    ::autoload_shadow(str);
    this_object()->init_lay_shadow();

    this_player()->set_guild_pref(SS_LAYMAN, query_guild_tax_lay());
}
