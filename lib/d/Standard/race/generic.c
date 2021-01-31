/*
 * /d/Standard/race/generic.c
 *
 */

#pragma save_binary
#pragma strict_types

inherit "/std/player_pub";
inherit "/d/Standard/std/mail_stuff";
inherit "/d/Standard/std/special_stuff";

inherit "/std/combat/humunarmed";

#include <cmdparse.h>
#include <composite.h>
#include <const.h>
#include <macros.h>
#include <std.h>
#include <ss_types.h>
#include <stdproperties.h>
#include "/d/Standard/login/login.h"

#ifdef Arkadia
#define COMMON_BOARD "/d/Arkadia/wiz/wiesciboard"
#endif

/*
 * Function name: finger_info
 * Description  : This function is called when this player is fingered. It
 *                prints special information and mail information about
 *                this player.
 */
public void
finger_info()
{
    finger_mail();
    finger_special();
}

string
query_race()
{
    return "czlowiek";
}

#ifdef Arkadia
/*
 * Function name: common_board_check
 * Description  : This function will check the common board and see whether
 *                a new note has been posted since we last logged in.
 */
static nomask void
common_board_check()
{
    string file;
    int note_time;
    object *tablice, tablica_wiesci;

    if (catch(COMMON_BOARD->teleledningsanka()))
    {
	return;
    }

    tablice = object_clones(find_object(COMMON_BOARD));
    if (sizeof(tablice))
	tablica_wiesci = tablice[0];
    else
	tablica_wiesci = find_object(COMMON_BOARD);
    
    file = tablica_wiesci->query_latest_note();
    if (!file || sscanf(file, "b%d", note_time) != 1)
    {
	return;
    }

    if (note_time > query_login_time())
    {
	write("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - " +
	    "- - - - - -\nPo twoim ostatnim wejsciu do Swiata Arkadii " +
	    "pojawily sie nowe Wiesci.\nZalecamy zapoznanie sie z nimi " +
	    "(przy pomocy komendy 'wiesci'), gdyz\nmoga zawierac istotne " +
	    "informacje!\n- - - - - - - - - - - - - - - - - - - - - - - - - " +
	    "- - - - - - - - - -\n\n");
    }
}
#endif

public void
init_leftovers()
{
    if (!random(5))
	add_leftover("/std/leftover", ({ "zab", "zeba", "zebowi", "zab",
	    "zebem", "zebie" }), ({ "zeby", "zebow", "zebom", "zeby", "zebami",
	    "zebach" }), PL_MESKI_NOS_NZYW, random(5) + 1, 0, 1, 0);
	  
    if (!random(5))
	add_leftover("/std/leftover", ({ "czaszka", "czaszki", "czaszce", 
	    "czaszke", "czaszka", "czaszce" }), ({ "czaszki", "czaszek",
	    "czaszkom", "czaszki", "czaszkami", "czaszkach" }), PL_ZENSKI,
	    1, 0, 1, 1);
    if (!random(5))
	add_leftover("/std/leftover", ({ "kosc", "kosci", "kosci", "kosc",
	    "koscia", "kosci" }), ({ "kosci", "kosci", "kosciom", "kosci", 
	    "koscmi", "kosciach" }), PL_ZENSKI, 2, 0, 1, 1);
	    
    if (!random(5))
        add_leftover("/std/leftover", ({ "rzepka", "rzepki", "rzepce",
            "rzepke", "rzepka", "rzepce" }), ({ "rzepki", "rzepek", 
            "rzepkom", "rzepki", "rzepkami", "rzepkach" }), PL_ZENSKI, 
            2, 0, 1, 1);
            
    if (!random(5))
	add_leftover("/std/leftover", ({ "lopatka", "lopatki", "lopatce",
	    "lopatke", "lopatka", "lopatce" }), ({ "lopatki", "lopatek", 
	    "lopatkom", "lopatki", "lopatkami", "lopatkach" }), 
	    PL_ZENSKI, 2, 0, 1, 1);
	    
    if (query_prop(LIVE_I_UNDEAD))
	return;

    if (!random(5))
        add_leftover("/std/leftover", ({ "ucho", "ucha", "uchu", "ucho",
            "uchem", "uchu" }), ({ "uszy", "uszu", "uszom", "uszy", 
            "uszami", "uszach" }), PL_NIJAKI_OS, 2, 0, 0, 0);
    if (!random(5))
        add_leftover("/std/leftover", ({ "skalp", "skalpu", "skalpowi", 
            "skalp", "skalpem", "skalpie" }), ({ "skalpy", "skalpow",
            "skalpom", "skalpy", "skalpami", "skalpach" }), PL_MESKI_NOS_NZYW,
            1, 0, 0, 1);
    if (!random(5))
        add_leftover("/std/leftover", ({ "paznokiec", "paznokcia",
            "paznokciowi", "paznokiec", "paznokciem", "paznokciu" }), 
            ({ "paznokcie", "paznokci", "paznokciom", "paznokcie",
            "paznokciami", "paznokciach" }), PL_MESKI_NOS_NZYW, 
            random(5) + 1, 0, 0, 0);
    if (!random(5))
	add_leftover("/std/leftover", ({ "serce", "serca", "sercu", "serce",
	    "sercem", "sercu" }), ({ "serca", "serc", "sercom", "serca",
	    "sercami", "sercach" }), PL_NIJAKI_NOS, 1, 0, 0, 1);
    if (!random(5))
	add_leftover("/std/leftover", ({ "nos", "nosa", "nosowi", "nos",
	    "nosem", "nosie" }), ({ "nosy", "nosow", "nosom", "nosy",
	    "nosami", "nosach" }), PL_NIJAKI_NOS, 1, 0, 0, 0);
    if (!random(5))
	add_leftover("/std/leftover", ({ "nerka", "nerki", "nerce", "nerke",
	    "nerka", "nerce" }), ({ "nerki", "nerek", "nerkom", "nerki",
	    "nerkami", "nerkach" }), PL_ZENSKI, 2, 0, 0, 1);
    add_leftover("/std/leftover", ({ "oko", "oka", "oku", "oko", "okiem",
        "oku" }), ({ "oczy", "oczu", "oczom", "oczy", "oczami", "oczach" }),
        PL_NIJAKI_OS, 2, 0, 0, 0);
}

/*
 * Function name: start_player
 * Descripiton  : When the player logs in, we set some race-specific stuff.
 */
public void
start_player()
{
    int plec = query_gender();
    
    /* Add the race-commandsouls. */
    if (!sizeof(query_cmdsoul_list()))
    {
	add_cmdsoul(RACESOULCMD[query_race()]);
	add_cmdsoul(RACEMISCCMD[query_race()]);
    }

    /* Set the weight of this living. That is dependant on the race. */
    if (query_prop(CONT_I_WEIGHT) <= 1)
    {
	add_prop(CONT_I_WEIGHT, RACEATTR[query_race()][plec][1] * 1000);
    }

    /* Set the volume of this living. That is dependant on the race. */
    if (query_prop(CONT_I_VOLUME) <= 1)
    {
	add_prop(CONT_I_VOLUME, RACEATTR[query_race()][plec][1] * 1000);
    }

    /* Set the height of this living. That is dependant on the race. */
    if (query_prop(CONT_I_HEIGHT) <= 1)
    {
	add_prop(CONT_I_HEIGHT, RACEATTR[query_race()][plec][0]);
    }

    /* Give the player a message about his mail-status. */
    start_mail(this_object()->query_def_post());

    /* Add some leftovers to the player. */
    init_leftovers();

#if 0
    /* Log the fact that the player (if not a wizard) entered and the
     * stats he or she has.
     */
    if (!query_wiz_level()) 
    {
	log_file("START_STATS", 
	    sprintf("%s %-11s (%3d, %3d, %3d, %3d, %3d, %3d)\n",
		ctime(time()),
		capitalize(query_real_name()),
		this_object()->query_stat(SS_STR),
		this_object()->query_stat(SS_DEX),
		this_object()->query_stat(SS_CON),
		this_object()->query_stat(SS_INT),
		this_object()->query_stat(SS_WIS),
		this_object()->query_stat(SS_DIS)), -1);
    }
#endif

#ifdef Arkadia
    /* Powiadomienie graczy o pojawieniu sie nowych wiesci. */
    common_board_check();
#endif

    ::start_player();
}

/*
 * Function name: query_def_start
 * Description  : This function returns the very basic starting location
 *                for people of this race. Notice that we use query_race
 *                and not query_race_name since query_race always returns
 *                a valid race name. To alter the starting location of the
 *                player, use set_default_start_location.
 */
public nomask string
query_def_start()
{
    return RACESTART[query_race()];
}

/*
 * Function name: query_def_post
 * Description  : This function returns the location of the post office for
 *                people of this race. It might be good to shadow this
 *                function in guild shadows if the post office is supposed
 *                to be located somewhere special for the guild.
 */
public string
query_def_post()
{
    return RACEPOST[query_race()];
}

/*
 * Function name: query_def_start
 * Description  : This function returns initial stats for people of this
 *                race. Notice that we use query_race, not query_race_name
 *                since query_race always returns a valid race name.
 * Returns      : int * - the initial stats.
 */
public int *
query_orig_stat()
{
    return RACESTAT[query_race()];
}

/*
 * Function name: query_combat_file
 * Description  : Give the name of the file to use for combat.
 * Returns      : string - the file to use.
 */
public string
query_combat_file()
{
    return "/std/combat/chumlock";
}

/*
 * Function name: stats_to_acc_exp
 * Description  : Translates the current base stats into acc_exp and take
 *                the race modofiers into account. This function is only
 *                used from the default setup in player_sec::new_init()
 */
static nomask void
stats_to_acc_exp()
{
    int il  = (SS_STR - 1);
    int sum = 0;
    int tmp;

    while(++il < SS_NO_STATS)
    {
        tmp = stat_to_exp(query_base_stat(il)) * 10 /
	    RACESTATMOD[query_race()][il];

        if (tmp > 0)
        {
            set_acc_exp(il, tmp);
            sum += tmp;
        }
        else
        {
            set_acc_exp(il, 0);
        }
    }

    set_exp(sum);
    set_exp_combat(0);
}

/*
 * Function name: acc_exp_to_stats
 * Description  : Translates the current accumulated exp into stats and
 *                takes the racial modifiers into account.
 */
static void
acc_exp_to_stats()
{
    int il = (SS_STR - 1);

    while(++il < SS_NO_STATS)
    {
        if (query_base_stat(il) >= 0)
        {
            set_base_stat(il, exp_to_stat(query_acc_exp(il) *
		RACESTATMOD[query_race()][il] / 10));
        }
    }
}

/*
 * Function name: update_stat
 * Description  : Convert exp to stat for a single stat and take the racial
 *                modifiers into account. This usually used by a guild that
 *                wants its stat to behave like the normal.
 * Arguments    : int stat - the stat to update.
 */
public nomask void
update_stat(int stat)
{
    set_base_stat(stat, exp_to_stat(query_acc_exp(stat)) *
	RACESTATMOD[query_race()][stat] / 10);
}

/*
 * Function name: query_statmod
 * Description  : This function returns the racial stat modifier for this
 *                player for a particular stat.
 * Arguments    : int stat - the stat to get the modifier for.
 * Returns      : int - the racial stat modifier for that stat.
 */
public nomask int
query_statmod(int stat)
{
    return RACESTATMOD[query_race()][stat];
}
