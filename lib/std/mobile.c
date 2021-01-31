/*
  /std/mobile.c

  This is the base for all nonplayer living objects. This file holds the
  code that is common to both non humanoid creatures and humanoid npc's.

*/
#pragma save_binary
#pragma strict_types

inherit "/std/living";
inherit "/std/act/seqaction" ;
inherit "/std/act/trigaction";

#include <ss_types.h>
#include <stdproperties.h>
#include <macros.h>
#include <std.h>
#include <options.h>

/* Private static global reactions.
 */
private	static object mobile_link;	 /* The linked master if controlled */
private static int    mobile_exp_factor; /* The xp modifier to give to killer */
private	static int    no_accept;	 /* Flag when mobile stops accepting */
private	static int    test_alarm;	 /* Alarm used to check for presence */

/*
 * Prototypes
 */
varargs void default_config_mobile(int lvl);

void
create_mobile()
{
    /* 
     * Observe that we do not continue the 'default' calls here
     * All default values are already set.
     */
    ustaw_nazwe( ({ "enpec", "enpeca", "enpecowi", "enpeca", "enpecem", 
        "enpecu" }), ({ "enpece", "enpecow", "enpecom", "enpece",
        "enpecami", "enpecach" }), PL_MESKI_NOS_ZYW);
}

nomask void
create_living()
{
    add_prop(CONT_I_HEIGHT, 160); /* Default height for monsters, 160 cm */
    add_prop(NPC_M_NO_ACCEPT_GIVE, "@@mobile_deny_objects");
    mobile_exp_factor = 100;
    this_object()->seq_reset();
    default_config_mobile();
    create_mobile();
    stats_to_acc_exp();
}

void
reset_mobile()
{
    ::reset_living();
}

nomask void
reset_living()
{
    reset_mobile();
}

/*
 * Function name: team_join
 * Description:   Sets this living as the leader of another
 * Arguments:	  member: The objectpointer to the new member of my team
 * Returns:       True if member accepted in the team
 */
int
team_join(object member)
{
    if (query_ip_number(member))
	return 0;		/* mobile leaders overplayers */
    return ::team_join(member);
}

static int   
test_live_here(object ob)
{
    return (living(ob) && interactive(ob) && (ob != this_object()));
}

/*
 * Function name: test_if_any_here
 * Description:   Turn of heart_beat if we are alone.
 * Returns:
 */
void
test_if_any_here()
{
    if (environment(this_object()) &&
	(!sizeof(filter(all_inventory(environment(this_object())), 
			test_live_here))))
    {
	stop_heart();
	test_alarm = 0;
    }
    else
	test_alarm = set_alarm(50.0, 0.0, test_if_any_here);
}

/* 
 * start_heart
 * Description:  When a mobile has an active heartbeat we must test
 *		 now and then if we can turn it off.
 */
static void
start_heart()
{
    ::start_heart();
    if (test_alarm != 0)
	remove_alarm(test_alarm);
    test_alarm = set_alarm(50.0, 0.0, test_if_any_here);
}

/*
 * Function name:  default_config_mobile
 * Description:    Sets all neccessary values for this mobile to function
 */
varargs void
default_config_mobile(int lvl)
{
    int i;

    for (i = 0; i < SS_NO_EXP_STATS ; i++)
	set_base_stat(i, (lvl ? lvl : 5));

    set_hp(500 * 500); /* Will adjust for CON above */

    stats_to_acc_exp();

    add_prop(CONT_I_WEIGHT, 70 * 1000);
    add_prop(CONT_I_VOLUME, 70 * 1000);

    mobile_exp_factor = 100;
    set_fatigue(query_max_fatigue());
}

/*
 * Function name: set_link_remote
 * Description:   Links a player to the output of the mobile
 * Arguments:	  player: Player to link mobile to
 */
void
set_link_remote(object player)
{
    object control;

    if (!player)
	player = this_player();

    if (!living(player) || mobile_link)
	return;

    control = clone_object(REMOTE_NPC_OBJECT); /* Get the remote control */
    control->set_npc(this_object());
    mobile_link = control;
    control->move(player);
}

/*
 * Description: Return the pointer to the current controller
 */
object
query_link_remote() { return mobile_link; }

/*
 *  Description: For monster link purposes
 */
void
catch_tell(string str)
{
    if (mobile_link)
	mobile_link->link_intext(str);
    ::catch_tell(str);
}

/*
 * Function name: catch_msg
 * Description:   This function is called for every normal message sent
 *                to this living.
 * Arguments:     str:       Message to tell the living
 *                from_player: The object that generated the message
 *			     This is only valid if the message is on the
 *			     form ({ "met message", "unmet message",
 *				     "unseen message" })
 */
public void 
catch_msg(mixed str, object from_player)
{
    if(!query_ip_number(this_object()) && !query_tell_active())
	return;

    if (pointerp(str))
    {
	if (!from_player)
	    from_player = this_player();
	if ((sizeof(str) > 2) && (!CAN_SEE_IN_ROOM(this_object()) ||
		!CAN_SEE(this_object(), from_player)))
	    catch_tell(str[2]);
	else if (this_object()->query_met(from_player))
	    catch_tell(str[0]);
	else 
	    catch_tell(str[1]);
    }
    else
	catch_tell(process_string(str, 1));
}

/*
 * Function name:  set_stats
 * Description:    Set the basic stats (see /sys/ss_types.h)
 */
public void
set_stats(int *stats)
{
    int il, max;

    max = (sizeof(stats) > SS_NO_STATS ? SS_NO_STATS : sizeof(stats));

    for (il = 0; il < max; il++)
	set_base_stat(il, stats[il]);
}


/*
 * Nazwa funkcji : set_exp_factor
 * Opis          : Ustawia wspolczynnik wartosci expowej npca - procent
 *                 standardowej ilosci punktow doswiadczenia jakie otrzymuje
 *                 gracz, ktory go zabil (od 0 do 250 %).
 * Uwaga         : Przy korzystaniu z tej funkcji, nalezy bezwzglednie
 *                 przestrzegac ograniczen zawartych w manualu, znajdujacym
 *                 sie chwilowo w ~silvathraec/open/set_exp_factor.
 */
static void
set_exp_factor(int proc_xp)
{
    if (proc_xp > -1 && proc_xp < 251)
	mobile_exp_factor = proc_xp;
}

/*
 * Nazwa funkcji : query_exp_factor
 * Opis          : Zwraca wspolczynnik wartosci expowej npca. (Patrz funkcja
 *                 set_exp_factor.)
 */
public int
query_exp_factor()
{
    return mobile_exp_factor;
}
   
/*
 * Function name: init_living
 * Description:   A patch for the automatic attack if this mobile can do that
 */
public void
init_living()
{
    this_object()->init_attack();
}

/*
 * Function name: special_attack
 * Description:   Called from the external combat object's heart_beat
 *                routine. By redefining this, monsters can easily attack
 *                with spells or take special actions when engaged
 *                in combat.
 * Arguments:     victim (the one we are fighting right now)
 * Returns:       0 - if we want the round to continue
 *                1 - if we are done with this round
 * Note:          By always returning 1 the mobile is unable
 *                to do an ordinary attack.
 */
public int
special_attack(object victim)
{
    return 0;
}
 
/*
 * Function name: mobile_deny_objects
 * Description:   This function is called from VBFC and NPC_M_NO_ACCEPT_GIVE
 *		  prop checking. If this mobile is intelligent enough he might
 *		  recognize that someone is trying to load him down before
 *		  attacking and then he doesn't accept any more objects. This
 *		  is merely default behaviour. Feel free to code something
 *		  different
 * Returns:       A message that will be printed to the player or 0
 */
mixed
mobile_deny_objects()
{
    string str;

    if (no_accept || random(100) < query_stat(SS_INT))
    {
	str = " nie chce przyjac niczego od ciebie.\n";
	no_accept = 1;
	return str;
    }
    return 0;
}

/*
 * Function name: refresh_mobile
 * Description:   This function is kept here for backwards compatibility,
 *                but is obsolete itself.
 */
void
refresh_mobile()
{
    refresh_living();
}

/*
 * Function name: query_option
 * Description:   Returns default option settings for mobiles
 * Arguments:     int opt - the option to check
 * Returns:       the setting for the specified option
 */
public int
query_option(int opt)
{
    if (opt == OPT_BLOOD)
    {
        return 1;
    }

    return 0;
}
