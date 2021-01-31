/*
    /std/living/stats.c

    This is a subpart of living.c
    All stat and skill related routines are coded here.

    This file is included into living.c
 */

#include <macros.h>
#include <ss_types.h>

static int	*delta_stat;  /* Temporary extra stats. */
static int      *stats;	      /* Stats the calculated values from acc_exp */
static int	*stat_extra;  /* Extra to add to the stats */

/*
 * Function name:   ss_reset
 * Description:     Reset stats and skills at start of character.
 */
static void
ss_reset()
{
    stats = allocate(SS_NO_STATS); 
    delta_stat = allocate(SS_NO_STATS);
    
    learn_pref = allocate(SS_NO_STATS); 
    acc_exp = allocate(SS_NO_STATS); 
    stat_extra = allocate(SS_NO_STATS);
}

/*
 * Function name:   query_skill_cost
 * Description:     Calculates the cost in experience points to raise a skill
 *                  a few levels.
 * Arguments:       oldskill: The old skill level
 *                  newskill: The new skill level
 * Returns:         The cost in experience points
 */
nomask public int
query_skill_cost(int oldskill, int newskill)
{
    return stat_to_exp(newskill) - stat_to_exp(oldskill);
}

/*
 * Stats
 */

/*
 * Function name:   set_stat_extra
 * Description:     Sets an extra number to add to the normal stats. It could be
 * 		    some object changing the livings stats while being held or
 *		    some other not time based stat bonus.
 * Arguments:       stat - What stat
 *		    val  - The extra value to modify stat with
 * Returns:	    The extra stat value
 */
public int
set_stat_extra(int stat, int val)
{
    if (stat < 0 || stat >= SS_NO_STATS)
	return 0;

    stat_extra[stat] = val;
    return val;
}

/*
 * Function name:   query_stat_extra
 * Description:     Query about the setting of the extra modifier of stats
 * Arguments:	    stat - What stat to ask about
 * Returns:	    The extra stat value
 */
public int
query_stat_extra(int stat)
{
    if (stat < 0 || stat >= SS_NO_STATS)
	return 0;

    return stat_extra[stat];
}

/*
 * Function name:   set_base_stat
 * Description:     Sets the value of a specific stat to a new value.
 * Arguments:       stat: The index in the stat array
 *                  val:  The value that the stat is set to
 * Returns:         The value of the stat, 0 if failed.
 */
int
set_base_stat(int stat, int val)
{
    if (stat < 0 || stat >= SS_NO_STATS || val < 1 )
	return 0;

    stats[stat] = val;
    return val;
}  

/*
 * Function name:   query_base_stat
 * Description:     Gives the value of a specific stat
 * Arguments:       stat: The index in the array of stats
 * Returns:         The value of a stat, -1 on failure.
 */
public int
query_base_stat(int stat)
{
    if (stat < 0 || stat >= SS_NO_STATS)
	return -1;
    return stats[stat];
}

/*
 * Function name:   query_cmd_average_stat
 * Description:     Calculate the avarage of all combat stats of a living
 * Returns:         The calculated avarage
 */
public int
query_cmb_average_stat()
{
    return (query_base_stat(SS_STR) + query_base_stat(SS_DEX) +
	query_base_stat(SS_CON)) / 3;
}   

/*
 * Function name:   query_average_stat
 * Description:     Calculate the avarage of all stats of a living
 * Returns:         The calculated avarage
 */
public int
query_average_stat()
{
    return (query_base_stat(SS_STR) + query_base_stat(SS_DEX) +
	query_base_stat(SS_CON) + query_base_stat(SS_INT) +
	query_base_stat(SS_WIS) + query_base_stat(SS_DIS)) / 6;
}   

/*
 * Function name: expire_tmp_stat()
 * Description  : Remove tmp_stat information as it times out.
 * Arguments    : int stat  - the stat that expires
 *                int value - the value to subtract.
 */
void
expire_tmp_stat(int stat, int value)
{
    delta_stat[stat] -= value;
}

/*
 * Function name: add_tmp_stat
 * Description:   add a temporary stat.
 * Arguments:     stat - which stat
 *                ds - change in stat
 *                dt - How many F_INTERVAL_BETWEEN_HP_HEALING intervals
 *                     to keep the change.
 * Returns:       1 - Ok, 0 - Change rejected.
 */
public int
add_tmp_stat(int stat, int ds, int dt)
{
    int tmp, i, start;
    int *end;

    tmp = query_stat(stat) - query_base_stat(stat);
    
    if (ds + tmp > 10 + query_base_stat(stat) / 10 || dt <= 0)
	return 0;

    dt = MIN(dt, F_TMP_STAT_MAX_TIME);

    delta_stat[stat] += ds;
    set_alarm(itof(dt * F_INTERVAL_BETWEEN_HP_HEALING), 0.0, &expire_tmp_stat(stat, ds));

    return 1;
}

/*
 * Function name: query_stat
 * Description:   Get the compound value of a stat. Never less than 1.
 * Arguments:     stat - Which stat to find.
 */
public int
query_stat(int stat)
{
    int i, tmp;

    if (stat < 0 || stat >= SS_NO_STATS)
	return -1;

    tmp = query_base_stat(stat);
    tmp += delta_stat[stat];
    tmp += stat_extra[stat];

    return (tmp > 0 ? tmp : 1);
}

/*
 *  acc_exp <=> stats
 *
 *  Formulas for translating between accumulated exp and stat values.
 *
 *   Total number of stats: 9
 */

/*
 * Function name: exp_to_stat
 * Description  : Translates given number of exp to a stat/skill value.
 * Arguments    : int exp - the experience points to be translated.
 * Returns      : int     - the new skill/stat value
 */
nomask int
exp_to_stat(int exp)
{
    return F_EXP_TO_STAT(exp);
}

/*
 * Function name:   stat_to_exp
 * Description:     Translates given stat value to minimum number of
 *                  experience points required to get to that stat value.
 * Arguments:       exp: The number of experience points to be translated.
 * Returns:         The amount of experience
 */
nomask int
stat_to_exp(int stat)
{
    return F_STAT_TO_EXP(stat);
}

/*
 * Function name: stats_to_acc_exp
 * Description:   Translates the current base stats into acc_exp. This is used
 *                used only from default setup in player_sec::new_init()
 */
static void
stats_to_acc_exp()
{
    int il, sum, tmp;

    for (il = SS_STR, sum = 0; il < SS_NO_STATS; il++)
    {
	tmp = stat_to_exp(query_base_stat(il));
	if (tmp > 0)
	{
	    set_acc_exp(il, tmp);
	    sum += tmp;
	}
	else
	    set_acc_exp(il, 0);
    }
    set_exp(sum);
    set_exp_combat(0);
}

/*
 * Function name: acc_exp_to_stats
 * Description:   Translates the current accumulated exp into stats.
 */
static void
acc_exp_to_stats()
{
    int il, tmp;

    for (il = SS_STR; il < SS_NO_STATS; il++)
    {
	if (query_base_stat(il) >= 0)
	{
	    tmp = exp_to_stat(query_acc_exp(il));
	    set_base_stat(il, tmp);
	}
    }
}

/*
 * Function name: update_stat
 * Description:   Convert exp to stat for a single stat. This usually used
 *		  by a guild that wants its stat to behave like the normal.
 * Arguments:     stat - Which stat to update.
 */
public void
update_stat(int stat)
{
    set_base_stat(stat, exp_to_stat(query_acc_exp(stat)));
}

/*
 * Function name: update_acc_exp
 * Description:   Fills in the acc_exp for each stat from the total exp
 *                depending on the learning preferences. If the acc_exp
 *                is reduced, the experience is removed relatively to
 *                the current values.
 */
static void
update_acc_exp()
{
    int il  = -1;
    int sum = query_exp();
    int stat, p, f;

    while(++il < SS_NO_STATS)
    {
	sum -= query_acc_exp(il);
    }

    /* We only update the stats only of the abs() of the difference
     * exceeds 1000. If not, we'll wait and update them the next time
     * you gain some experience.
     */
    if (sum > 1000)
    {
	sum /= 100;
	il = -1;

	while(++il < SS_NO_STATS)
	{
	    set_acc_exp(il, query_acc_exp(il) + (query_learn_pref(il) * sum));
	}
	acc_exp_to_stats();
    }
    else if (sum < -1000)
    {
	il = -1;

	/* This is to avoid exceeding MAXINT */
	p = -sum > query_exp() ? -sum : query_exp();
	if (p > 2000000)
	    f = 2000000000/p;
	else
	    f = 1000;
	while(++il < SS_NO_STATS)
	{
	    stat = query_acc_exp(il);
	    p = (stat * f) / query_exp(); 
	    set_acc_exp(il, stat + ((sum * p) / f));
	}
	acc_exp_to_stats();
    }
}

/*
 * Function name: object_random
 * Description:   Get a random number depending on player object number
 *		  and the given object's object number. This number will
 *		  always be the same for a given object.
 * Arguments:	  ival - The random number interval,
 *		  obj - The object.
 * Returns:	  -1 if the given object doesn't exist.
 */
public nomask int
object_random(int ival, object obj)
{
    string *list, s_num;
    int num;

    if (!objectp(obj))
	return -1;

    list = explode(file_name(this_object()), "#");
    if (sizeof(list) > 1)
	s_num = list[1];
    else
	s_num = "0";
    list = explode(file_name(obj), "#");
    s_num += list[1];
    sscanf(s_num, "%d", num);
    
    return random(ival, num);
}

/*
 * Function name: find_stat_describer
 * Description:   Finds the textgiver that describes a certain stat
 * Arguments:     stat: The number of the stat
 * Returns:       Objectpointer of object to call for whatever desc needed
 */
public object
find_stat_describer(int stat)
{
    string *obf;
    int il;
    object ob;

    obf = query_textgivers();

    for (il = 0; il < sizeof(obf); il++)
    {
	ob = find_object(obf[il]);
	if (!ob)
	{
	    catch(obf[il]->teleledningsanka());
	    ob = find_object(obf[il]);
	}
	if (ob && ob->desc_stat(stat))
	    return ob;
    }
    return 0;
}

/*
 * Function name:   modify_stat
 * Description:     This is a service function, to be used only by the mud
 *                  admins. You should never call it, for any reason
 *                  whatsoever.
 * Arguments:       s:   The stat to affect
 *                  to:  The value the stat should be
 *                  msg: message stating why the change was made
 * Returns:         1 if set, 0 if not.
 */
public nomask int
modify_stat(int s, int to, string msg)
{
    int from;
    string log;

    if (s < SS_STR || s >= SS_NO_STATS)
        return 0;

    from = query_base_stat(s);
    set_base_stat(s,to);

#if 0
    log = sprintf("%s %s %s: %d(%d) by %s. Reason: %s\n",
         ctime(time()), 
         interactive(this_object()) ? query_real_name() : file_name(this_object()),
         SS_STAT_DESC[s], to, from, 
         DEBUG_POBJS(0) + (this_player() ? this_player()->query_real_name() : ""),
         msg);
#endif

    SECURITY->log_syslog("CHANGE_STAT", ctime(time()) + ": "
          + (interactive() ? query_real_name() : file_name()) + " "
          + SS_STAT_DESC[s] + " " + from + "->" + to + " by "
          + this_player()->query_real_name() + "\n");

    set_base_stat(s,to);
    stats_to_acc_exp();
    return 1;
}

/*
 * Function name: set_guild_stat
 * Description  : This function allows the guildmaster to alter the
 *                experience a player has gathered in the guild. It can
 *                be set to any value.
 * Arguments    : int stat - the stat to set (guild stats only!)
 *                int exp  - the experience value to set the stat to.
 * Returns      : int      - 1/0 - set/unset.
 */
public nomask int
set_guild_stat(int stat, int exp)
{
    int change  = exp - query_acc_exp(stat);
    int stat_value;

    /* Not a guild stat... Naughty wizard.
     * or non-positive exp... Impossible.
     */
    if ((stat < SS_NO_EXP_STATS) ||
	(stat >= SS_NO_STATS) ||
	(exp < 1))
    {
	return 0;
    }

    stat_value = exp_to_stat(exp);
    if (interactive(this_object()) && query_base_stat(stat) != stat_value)
        SECURITY->log_syslog("CHANGE_STAT", ctime(time()) + ": "
              + query_real_name() + " " + SS_STAT_DESC[stat] + " "
              + query_base_stat(stat) + "->" + stat_value
              + (this_player() != this_object() ? " by "
              + this_player()->query_real_name() : "") + "\n");

    /* We set the new stat to to the guild stat, both in experience and
     * in stat value.
     */
    set_acc_exp(stat, exp);
    set_base_stat(stat, stat_value);

    /* Remove the guild experience from total experience and from the
     * combat experience as much as possible.
     */
    set_exp(query_exp() + change);
    set_exp_combat(MAX(1, (query_exp_combat() + change)));

    /* In the original function clear_guild_stat, Tricky called
     * update_acc_exp, though since the total experience is balanced,
     * that is not necessary. /Mercade
     */
    return 1;
}

/*
 * Function name: clear_guild_stat
 * Description  : This function allows guilds to clear the accumulated
 *                experience when a player leaves the guild. It is only
 *                a front for set_guild_stat.
 * Arguments    : int stat - the stat to clear (guild stat only!)
 * Returns      : int      - 1/0 - cleared/not cleared
 */
public nomask int
clear_guild_stat(int stat)
{
    return set_guild_stat(stat, 1);
}

/*
 * Function name: find_skill_describer
 * Description:   Finds the textgiver that describes a certain skill
 * Arguments:     skill: The number of the skill
 * Returns:       Objectpointer of object to call for whatever desc needed
 */
public object
find_skill_describer(int stat)
{
    string *obf;
    int il;
    object ob;

    obf = query_textgivers();

    for (il = 0; il < sizeof(obf); il++)
    {
	ob = find_object(obf[il]);
	if (!ob)
	{
	    catch(obf[il]->teleledningsanka());
	    ob = find_object(obf[il]);
	}
	if (ob && ob->desc_skill(stat))
	    return ob;
    }
    return 0;
}
