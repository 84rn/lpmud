/*
 * player/death_sec.c
 *
 * This is a subpart of player_sec.c
 *
 * Handling of what happens when the player dies and resurrects.
 * Scars all also handled here.
 *
 * This file is included into player_sec.c
 */

#include <formulas.h>
#include <log.h>
#include <ss_types.h>
#include <std.h>

#define WSTEP_SMIERCI "/d/Standard/smierc/wstep_smierci.txt"

/*
 * Prototypes
 */
void modify_on_death();
int query_average();

/*
 * Function name:   second_life
 * Description:     Handles all that should happen to a dying player.
 * Argument:        Object that caused the kill.
 * Returns:         True if the living object should get a second life
 */
public int
second_life(object killer)
{
    string log_msg, wstep;

    if (query_wiz_level())
	return 0;

    log_msg = ctime(time()) + " " +
        capitalize(this_object()->query_real_name()) +
        " (" + this_object()->query_average_stat() + ") by ";

    if (objectp(killer))
    {
	if (interactive(killer))
        {
	    log_msg += capitalize(killer->query_real_name()) +
		" (" + killer->query_average_stat() + ")";
            if (environment() && file_name(environment()) ==
                    query_default_start_location())
                log_msg += " [startloc]";
        }
	else
	    log_msg += MASTER_OB(killer);
    }
    else
	log_msg += "poison";

    log_msg += "\n";

#ifdef LOG_PLAYERKILLS
    if (objectp(killer) && interactive(killer))
    {
        log_file(LOG_PLAYERKILLS, log_msg, -1);
    }
    else
#endif LOG_PLAYERKILLS
    {
#ifdef LOG_KILLS
        log_file(LOG_KILLS, log_msg, -1);
#endif LOG_KILLS
    }

    modify_on_death();
    inc_death_count();

    set_m_in(F_GHOST_MSGIN);
    set_m_out(F_GHOST_MSGOUT);

    set_headache(0);
    set_intoxicated(0);
    set_stuffed(0);
    set_soaked(0);

    stop_fight(query_enemy(-1)); /* Forget all enemies */

    wstep = read_file(WSTEP_SMIERCI);
    if (strlen(wstep))
	tell_object(this_object(), wstep);

    this_object()->death_sequence();
    save_me(0); /* Save the death badge if player goes linkdead. */

    return 1;
}

/*
 * Function name:   modify_on_death
 * Description:     Modifies some values (e.g. exp, stats and hp) when a
 *                  player has died.
 */
static void 
modify_on_death()
{
    /* reduction exp, stat exp, total exp, total reduction exp */
    int rex, sex, tex, trex, il;
    float stat_factor;

    /*
     * We should just reduce our combat exp and affect the stats accordingly.
     * Every stat is lowered by ratio of the combat exp death takes and the
     * total exp. This way a 50% higher stat will loose 50% more exp points.
     * The construct with rex is used because of the fact that integers will
     * be rounded and the total exp should match the total of the stat's exp.
     */
    tex = query_exp();

    if (!tex) /* safety! we don't want division by 0 */
    {
        /* but we still need to set the hitpoints */
        set_hp(F_DIE_START_HP(query_max_hp()));
        return;
    }

    rex = 0;
    stat_factor = ( itof(F_DIE_REDUCE_XP(query_exp_combat())) / itof(tex) );

    for (il = SS_STR; il < SS_NO_EXP_STATS; il++)
    {
	if (query_base_stat(il) >= 0) /* do we need this query? */
        {
            sex = query_acc_exp(il);
            rex = ftoi(stat_factor * itof(sex) );
            trex += rex;
	    set_acc_exp(il, (sex - rex));
        }
    }

    set_exp(tex - trex);
    set_exp_combat(query_exp_combat() - trex);

    this_object()->acc_exp_to_stats();
    this_object()->add_exp(0, 0);

    /*
     * We should reset our hitpoints to something above 0
     */
    set_hp(F_DIE_START_HP(query_max_hp()));
}

/*
 * Function name:   reincarnate
 * Description:     Manages the reincarnation of a player
 */
public void
reincarnate()
{
//  set_player_file(LOGIN_NEW_PLAYER);
    LOGIN_NEW_PLAYER->reincarnate_me();
}

/*
 * Function name:   remove_ghost
 * Description:     This is normally not called except when a poor soul has
 *		    not been fetched by Death and reincarnated.
 * Arguments:       quiet : true if no messages should be printed
 * Returns:         0 if the player is not a ghost
 *                  1 otherwise.
 */
public int
remove_ghost(int quiet)
{
    if (!quiet && query_ghost())
    {
	tell_object(this_object(), "Zostajesz nagle wskrzeszon" + 
	    koncowka("y", "a") + "!\n");
	say(QCIMIE(this_object(), PL_MIA) + " zostaje nagle wszkrzeszon" +
	    koncowka("y", "a") + "!\n");
    }

    set_ghost(0);

    save_me(0);
    return 1;
}

