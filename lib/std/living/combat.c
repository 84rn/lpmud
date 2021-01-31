/*
 *  /std/living/combat.c
 *
 *  This is a subpart of living.c
 *  All internal combat routines are coded here.
 *
 *  This file is included into living.c
 *
 *  Most of the functionality is moved to the external combat object,
 *  see /std/combat/cbase
 */

#include <std.h>
#include <stdproperties.h>
#include <wa_types.h>

static int      time_to_heal;   /* Healing counter */
static int	quickness;	/* how quick are we? */

static string   whimpy_dir,	/* Direction to wimpy in if needed */
		whimpy_loc;	/* Location we whimpy from */

static object   my_leader,	/* Pointer to team leader if exist */
                *my_team,	/* Array of team members if leader */
		combat_extern;  /* The external combat object */

static mixed    leftover_list;  /* The list of leftovers */

static int	run_alarm;	/* Alarm used for panic code */

#define CEX if (!combat_extern) combat_reload()
varargs public mixed query_leftover(string organ);
public int remove_leftover(string organ);
public void run_away();
public object query_attack();

/*
 * Function name:   query_combat_file
 * Description:     Gives the name of the file to use for combat.
 * Returns:         The name
 */
public string
query_combat_file()
{
    return COMBAT_FILE;
}

/*
 * Function name:   query_combat_object
 * Description:     Gives the object that is currently acting as external
 *                  combat object
 * Returns:         the combat object
 */
public object
query_combat_object()
{
    return combat_extern;
}

static void
combat_reload()
{
    if (combat_extern)
	return;

    combat_extern = clone_object(query_combat_file());

    /*
     * We can define our own combat object, but it must inherit
     * the original.
     */
    if (function_exists("create_object", combat_extern) != COMBAT_FILE)
    {
	write("ILLEGAL COMBAT OBJECT: " +
	      function_exists("create_object", combat_extern) +
	      " should be: " + COMBAT_FILE + "\n");
	destruct();
    }

    combat_extern->cb_link(); /* Link me to the combat object */

    /* 
     * Configure this living object. For humanoids this includes adding
     * the hand attacks and head, torso etc hitlocations.
     */
    combat_extern->cb_configure(); 
}

/*
 * Function name:   combat_reset
 * Description:     Reset the combat functions of the living object.
 * Arguments:       arg: Reset argument.
 */
static nomask void
combat_reset()
{
    my_team = 0;
    add_subloc(SUBLOC_WORNA, this_object());
    add_subloc(SUBLOC_WIELD, this_object());
    combat_reload();
}

/*
 * Function name:   do_die
 * Description:     Called from enemy combat object when it thinks we died.
 * Arguments:       killer: The enemy that caused our death.
 */
public void
do_die(object killer)
{
    object corpse;
    string *temp;
    int i;

    /* Did I die ? */
    if ((query_hp() > 0) ||
	query_wiz_level() ||
	query_ghost())
    {
        return;
    }

    /* Stupid wiz didn't give the objectp to the killer. */
    if (!objectp(killer))
	killer = previous_object();
    /* Bad wiz, calling do_die in someone. */
    if ((MASTER_OB(killer) == WIZ_CMD_NORMAL) ||
        (MASTER_OB(killer) == TRACER_TOOL_SOUL))
        killer = this_interactive();

    CEX; combat_extern->cb_death_occured(killer);

    killer->notify_you_killed_me(this_object());
    
    /* Fix the corpse and possibly the ghost */
    if (this_object()->query_prop(LIVE_I_NO_CORPSE))
    {
	move_all_to(environment(this_object()));
    }
    else
    {
        if (!objectp(corpse = (object)this_object()->make_corpse()))
        {
	    corpse = clone_object("/std/corpse");
	    temp = ({});
	    for (i = 0; i < 6; i++)
	        temp = temp + ({ query_nazwa(i) });
	    corpse->ustaw_imie_denata(temp);
	    corpse->change_prop(CONT_I_WEIGHT, query_prop(CONT_I_WEIGHT));
	    corpse->change_prop(CONT_I_VOLUME, query_prop(CONT_I_VOLUME));
	    corpse->add_prop(CORPSE_M_RACE, query_rasy());
	    corpse->add_prop(CORPSE_I_RRACE, query_rodzaj_rasy() + 1);
	    corpse->add_prop(CONT_I_TRANSP, 1);
	    corpse->change_prop(CONT_I_MAX_WEIGHT,
		query_prop(CONT_I_MAX_WEIGHT));
	    corpse->change_prop(CONT_I_MAX_VOLUME,
		query_prop(CONT_I_MAX_VOLUME));
	    corpse->add_leftover(query_leftover());
        }

	corpse->add_prop(CORPSE_AS_KILLER,
			 ({ killer->query_real_name(),
			    killer->query_nonmet_name() }) );
    	corpse->move(environment(this_object()), 1);
    	move_all_to(corpse);
    }
    
    set_ghost(1);

    if (!this_object()->second_life(killer))
    {
	this_object()->remove_object();
    }
}

/*
 * Function name:   move_all_to
 * Description:     Move the entire inventory of this_object to dest
 * Arguments:       dest: destination of the inventory
 */
static nomask void
move_all_to(object dest)
{
    object *oblist;
    int i, ret;

    oblist = all_inventory(this_object());
    if (oblist && sizeof(oblist) > 0)
    {
	for (i = sizeof(oblist) - 1; i >= 0; i--)
        {
            /* remove all poisons.. you are dead, so they wo not bother you
             * in your new body.
             */
            if (function_exists("create_object", oblist[i]) ==
                "/std/poison_effect")
            {
                oblist[i]->remove_object();
                continue;
            }
	    if (catch(ret = oblist[i]->move(dest)))
		log_file("DIE_ERR", ctime(time()) + " " +
		    this_object()->query_name() + " (" +
		    file_name(oblist[i]) + ")\n");
	    else if (ret)
		oblist[i]->move(environment(this_object()));
        }
    }
}

/*************************************************
 *
 * Whimpy routines
 *
 */

/*
 * Function name:   set_whimpy_dir
 * Description:     Sets the favourite direction of the whimpy escape routine
 * Arguments:       str: the direction string
 */
public void
set_whimpy_dir(string str)
{
    whimpy_dir = str;
}

/*
 * Function name:   query_whimpy_dir
 * Description:     Gives the current favourite whimpy escape direction
 * Returns:         The direction string
 */
public string
query_whimpy_dir()
{
    return whimpy_dir;
}


/*************************************************
 *
 * Team routines
 *
 */

/*
 * Function name:    set_leader
 * Description:      Sets this living as a member in a team
 *                   It will fail if this living is a leader itself
 * Arguments:	     leader: The objectpointer to the leader of the team
 * Returns:	     True if successfull
 */
public int
set_leader(object leader)
{
    if (sizeof(my_team))
	return 0;			/* We can't be both leader and led */

    my_leader = leader;

    return 1;
}

/*
 * Function name:   query_leader
 * Description:     Gives the object of the living who is the leader of the
 *                  team that we are in.
 * Returns:         The object with the leader, or 0 if we're not in a team
 */
public object
query_leader()
{
    return my_leader;
}

/*
 * Function name:   team_join
 * Description:     Sets this living as the leader of another
 *                  Fails if we have a leader, then we can't lead others.
 * Arguments:	    member: The objectpointer to the new member of my team
 * Returns:         True if member accepted in the team
 */
public int
team_join(object member)
{
    if (my_leader)
	return 0;

    if (!member->set_leader(this_object()))
	return 0;

    if (member_array(member, query_team()) >= 0)
	return 1;		/* Already member */
    if (!my_team)
	my_team = ({ member });
    else
	my_team = my_team + ({ member });
    return 1;
}

/*
 * Function name:   query_team
 * Description:     Gives an array with all objects of team members in it
 * Returns:         The array with team members
 */
public object *
query_team()
{
    int i;

    if (!my_team)
	return ({});
	
    my_team -= ({ 0 });
    
    return my_team + ({ });
}

/*
 * Function name:   team_leave
 * Description:     Removes this living as the leader of another
 * Arguments:	    member: The objectpointer to the member leaving my team
 */
public void 
team_leave(object member)
{
    int a;

    a = member_array(member, my_team);
    if (a >= 0)
    {
	my_team[a]->set_leader(0);
	my_team = exclude_array(my_team, a, a);
	if (!sizeof(my_team))
	    my_team = 0;
    }
}

/*
 * Function name:   query_team_others
 * Description:     Gives all members/leader that we are joined up with
 * Returns:         The array with all other members
 */
public /* object * */
query_team_others()
{
    if (my_leader)
	return (object*)my_leader->query_team() + 
	    ({ my_leader }) - ({ this_object() });
    else
	return query_team();
}


/************************************************************
 * 
 * Redirected functions to the external combat object
 *
 */

/*
 * Function name:   hit_me
 * Description:     Called to make damage on this object. The actually
 *		    made damage is returned and will be used to change
 *		    the score of the aggressor.
 * Arguments:	    wcpen 	  - ModifiedWeapon class penetration (mpen)
 *		    dt    	  - damagetype, use MAGIC_DT if ac will not
 *				    help against this attack.
 *                  hitsuc        - how well did we hit.
 *		    attacker   	  - Object hurting us
 *		    attack_id 	  - Special id saying what attack hit us. If 
 *				    you have made a special attack, let the 
 *                                  id be -1
 *                  target_hitloc - Optional argument specifying a hitloc
 *                                  to damage.  If not specified or an
 *                                  invalid hitloc is given, a random
 *                                  one will be used.
 * Returns:         The hitresult as given by the external combat object.
 *			({ proc_hurt, hitloc description, phit, dam, armour })
 */
varargs public mixed
hit_me(int wcpen, int dt, int hitsuc, object attacker, int attack_id,
       int target_hitloc = -1)
{
    mixed hres;
    int wi;

    /*
     * Start nonplayers when attacked
     */
    start_heart();

    CEX;
    hres = (mixed)combat_extern->cb_hit_me(wcpen, dt, hitsuc, attacker, 
        attack_id, target_hitloc);

    if (!(wi = query_whimpy()))
	return hres;

    if (((100 * query_hp()) / query_max_hp()) < wi)
    {
	if (run_alarm != 0)
	    remove_alarm(run_alarm);
	whimpy_loc = file_name(environment(this_object()));
	run_alarm = set_alarm(1.0, 0.0, run_away);
    }

    return hres;
}

public mixed
query_defense(int weight)
{
    CEX;
    
    return combat_extern->cb_query_defense(weight);
}

/*
 * Function name:   attack_object
 * Description:     Start attacking, the actual attack is done in heart_beat
 * Arguments:	    The object to attack
 */
public void
attack_object(object ob)
{
    /* 
     * For monsters, start the heart beat
     */
    start_heart();

    CEX; combat_extern->cb_attack(ob);
}

/*
 * Function name:   attacked_by
 * Description:     This routine is called when we are attacked.
 * Arguments:	    ob: The attacker
 */
public void
attacked_by(object ob)
{
    CEX; combat_extern->cb_attacked_by(ob);
}

/*
 * Function name:   query_not_attack_me
 * Description:     The intended victim may force a fail when attacked.
 *                  If fail, the cause must produce explanatory text himself.
 * Arguments:       who: The attacker
 *                  aid: The attack id
 * Returns:         True if the attacker fails hitting us, false otherwise.
 */
public int
query_not_attack_me(object who, int aid)
{
    return 0;
}

/*
 * Function name:   combat_init
 * Description:     Notes when players are introduced into our environment
 *                  Used to attack known enemies on sight.
 */
nomask void
combat_init()
{
    /*
     * Is this_player() in list of known enemies ?
     * Use attacked_by() so that not forced to swap current enemy
     */
    CEX;

    /* Can't attack people you can't see */
    if (!CAN_SEE(this_object(), this_player()))
	return;

    if (member_array(this_player(), this_object()->query_enemy(-1)) >= 0)
    {
	this_object()->reveal_me(1);
	this_player()->reveal_me(1);
	this_object()->attacked_by(this_player());
    }
}

/*
 * Function name:   heal_living
 * Description:     Heals the living object
 * Arguments:	    num: Should be 0, otherwise >0 for lost heart_beats
 */
static nomask void
heal_living(int num)
{
    time_to_heal += (num + 1);
    CEX;
    time_to_heal = (int)combat_extern->cb_heal(time_to_heal);
}

/*
 * Function name: run_away
 * Description:   Runs away from the fight
 */
public void
run_away()
{
    int dont_whimpy;
    
    if (run_alarm)
    {
	remove_alarm(run_alarm);
	run_alarm = 0;
	if (whimpy_loc && whimpy_loc != file_name(environment(this_object())))
	    dont_whimpy = 1;
	whimpy_loc = 0;
    }

    if (dont_whimpy)
	return;

    CEX; combat_extern->cb_run_away(whimpy_dir);
}

/*
 * Nazwa funkcji : notify_i_escaped
 * Opis          : Wywolywana przez combat object osoby, ktora uciekla
 *		   z walki we wszystkich swoich przeciwnikach.
 * Argumenty     : ob - obiekt osoby, ktora uciekla
 */
public void
notify_i_escaped(object ob)
{
    if (query_attack() == ob)
    {
	CEX;
	combat_extern->cb_enemy_escaped();
    }
}

/*
 * Function name:   stop_fight
 * Description:     Stops the current fight
 */
public void
stop_fight(mixed elist)
{
    CEX; combat_extern->cb_stop_fight(elist);
}

/*
 * Function name:   query_enemy
 * Description:     Gives information of recorded enemies. If you want to
 *		    know currently fought enemy (if any) call query_attack()
 * Arguments:       arg: Enemy number (-1 == all enemies)
 * Returns:         Object pointer to the enemy
 */
public mixed
query_enemy(int arg) 
{
    CEX; return combat_extern->cb_query_enemy(arg); 
}

/*
 * Function name:   query_attack
 * Description:     Return the attacked object.
 * Returns:         The attacked object.
 */
public object
query_attack()
{
    CEX; return (object)combat_extern->cb_query_attack();
}

/*
 * Nazwa funkcji : set_combat_speed
 * Opis          : Ustawia szybkosc walki danej postaci. 
 * Argumenty     : int speed - ilosc atakow na minute. Przyjmowane
 *			sa wartosci od 1 do 60. Standardowo, kazdy walczy
 *			z szybkoscia 15 atakow na minute.
 */
public void
set_combat_speed(int speed)
{
    CEX; combat_extern->cb_set_speed(speed);
}

/*
 * Nazwa funkcji : query_combat_speed
 * Opis          : Zwraca szybkosc walki danej postaci.
 * Funkcja zwraca: int - ile razy w ciagu minuty dana postac atakuje.
 */
public int
query_combat_speed()
{
    CEX; return combat_extern->cb_query_speed();
}

/*******************************************
 *
 * Weapon and Armour routines. 
 *
 * These are merely registration routines for objects used in combat.
 * The actual management of their function is done in the external combat
 * object. The terminology of weapons, armours, wield and wear remain only
 * for backwards compatibility and confusion.
 */

/*
 * Function name:   wield
 * Description:     Wield a weapon.
 * Arguments:	    wep - The weapon to wield.
 * Returns:         True if wielded.
 *                  String with failure message if not wielded.
 */
public mixed
wield(object wep)
{
     CEX; return combat_extern->cb_wield_weapon(wep);
}

/*
 * Function name:   unwield
 * Description:     Unwield a weapon.
 * Arguments:	    wep: The weapon to unwield.
 * Returns:         None.
 */
public void
unwield(object wep)
{
    CEX; combat_extern->cb_unwield(wep);
}

/*
 * Function name: query_weapon
 * Description  : Returns the weapon held in a specified location or all the
 *                weapons the living wields when -1 is given as argument.
 * Arguments    : int which - a numeric label describing a weapon location.
 *                            On humanoids this is W_RIGHT etc. Give -1 to
 *                            list all weapons.
 * Returns      : object   - the corresponding weapon or 0.
 *                object * - all weapons in an array for which == -1.
 */
varargs public mixed
query_weapon(int which)
{
    CEX; return combat_extern->cb_query_weapon(which);
}

/*
 * Function name: update_weapon
 * Description:   Call this function if the stats or skills of a weapon has
 *		  changed.
 * Arguments:     wep - the weapon 
 */
public void
update_weapon(object wep)
{
    CEX; combat_extern->cb_update_weapon(wep);
}

/*
 * Function name:   wear_arm
 * Description:     Wear an armour
 * Arguments:	    arm: The armour.
 * Returns:         True if armour worn.
 *                  String with failure message if not worn.
 */
public mixed
wear_arm(object arm)
{
    CEX; return combat_extern->cb_wear_arm(arm);
}

/*
 * Function name:   remove_arm
 * Description:     Remove an armour
 * Arguments:	    arm - The armour.
 */
public void
remove_arm(object arm)
{
    CEX; combat_extern->cb_remove_arm(arm);
}

/*
 * Function name: query_armour
 * Description  : Returns the armour of a given position or lists all armours
 *                worn when -1 is given as argument.
 * Arguments    : int which - a numeric label describing an armour location.
 *                            On humanoids this is TS_HEAD etc. Give -1 to
 *                            list all.
 * Returns      : object   - the corresponding armour or 0.
 *                object * - all armours when -1 is given.
 */
varargs public mixed
query_armour(int which) 
{
    CEX; return (int)combat_extern->cb_query_armour(which); 
}

/*
 * Function name: update_armour
 * Description:   Call this function when the ac of an armour has changed
 * Arguments:     arm - the armour
 */
public void
update_armour(object arm)
{
    CEX; combat_extern->cb_update_armour(arm);
}

/*
 * Nazwa funkcji : query_slot
 * Opis          : Zwraca informacje o zajetych slotach. Jesli jako argument
 *		   poda sie konkretny identyfikator slota, funkcja zwroci
 *		   obiekt go okupujacy (lub 0, gdy slot jest niezajety).
 *		   Jesli zas jako argument poda sie -1, funkcja zwroci
 *		   tablice z identyfikatorami wszystkich zajetych slotow.
 * Argumenty     : int - identyfikator slota lub -1.
 * Funkcja zwraca: object - obiekt okupujacy podany slot, lub
 *		   int * - tablica z identyfikatorami okupowanych slotow.
 */
public mixed
query_slot(int slot_num)
{
    CEX; return combat_extern->cb_query_slot(slot_num);
}


/*
 * Function name:   adjust_combat_on_move
 * Description:     Called to let movement affect the ongoing fight. This
 *                  is used to print hunting messages.
 * Arguments:	    True if leaving else arriving
 */
public void
adjust_combat_on_move(int leave)
{
    CEX; combat_extern->cb_adjust_combat_on_move(leave);
}

/*
 * Function name:   adjust_combat_on_intox
 * Description:     Called to let intoxication affect combat. This
 *                  is used to do nasty drunk type things *laugh*
 * Arguments:       pintox: %intoxicated      
 */
public void
adjust_combat_on_intox(int pintox)
{
    CEX; combat_extern->cb_adjust_combat_on_intox(pintox);
}

/*
 * Function name:   add_panic
 * Description:     Adjust the panic level.
 * Arguments:       dpan: The panic increase/decrease
 */
public void
add_panic(int dpan)
{
    CEX; combat_extern->cb_add_panic(dpan);
}

/*
 * Function name:   query_panic
 * Description:     Give panic value
 * Returns:         The panic value
 */
public int
query_panic()
{
    CEX; return (int)combat_extern->cb_query_panic();
}

/*
 * Function name:   combat_status
 * Description:     Let the combat object describe the combat status
 * Returns:         Description as string
 */
public string
combat_status() 
{
    CEX; return combat_extern->cb_status(); 
}

/*
 * Function name:   combat_data
 * Description:	    Let the combat object describe some combat data
 * Returns:	    Description as string
 */
public string
combat_data()
{
    CEX; return combat_extern->cb_data();
}

/*
 * Function name:   tell_watcher
 * Description:     Send a string to people who wants to see fights
 * Arguments:	    str   - The string to send
 * 		    enemy - The enemy we fought
 *		    arr   - Array of objects not to send this message to
 *			    If not used, message sent to all spectators who
 *			    wants to see blood.
 */
varargs void
tell_watcher(string str, object enemy, mixed arr)
{
    CEX; combat_extern->tell_watcher(str, enemy, arr);
}

/*
 * Function name: add_leftover
 * Description:   Dodaje organ do ciala.
 * Arguments:	  obj - Sciezka do obiektu organu.
 *		  lp - Odmiana nazwy organu w lp.
 *		  lmn - Odmiana nazwy organu w lmn.
 *		  rodzaj - Rodzaj gramatyczny nazwy organu.
 *		  nitems - Liczba organow tego typu. (-1 = nieskonczenie wiele)
 *		  vbfc - VBFC do sprawdzenia.
 *		  hard - Twarde organy, np kosci (Ktore zostaja po rozlozeniu
 *				sie ciala).
 *		  cut - Czy organ musi byc wyciety(1), czy da sie go 
 *			tez wyrwac(0).
 */
varargs public void
add_leftover(string obj, string *lp, string *lmn, int rodzaj, int nitems, string vbfc,
	     int hard, int cut)
{
    if (!sizeof(leftover_list))
	leftover_list = ({ });

    remove_leftover(lp[PL_BIE]);

    leftover_list += ({ ({ obj, lp, lmn, rodzaj, nitems, vbfc, hard, cut }) });
}

/*
 * Function name: query_leftover
 * Description:   Return the leftover list. If an organ is specified, that
 *		  actual entry is looked for, otherwise, return the entire
 *		  list.
 *		  The returned list contains the following entries:
 *		  ({ objpath, organ, nitems, vbfc, hard })
 * Arguments:	  organ - The organ to search for. W MIANOWNIKU.
 */
varargs public mixed
query_leftover(string organ)
{
    int i;

    if (!sizeof(leftover_list))
	return ({ });

    if (!strlen(organ))
	return leftover_list;

    for (i = 0 ; i < sizeof(leftover_list) ; i++)
	if (leftover_list[i][1][PL_MIA] == organ)
	    return leftover_list[i];
}

/*
 * Function name: remove_leftover
 * Description:   Remove a leftover entry from a body.
 * Arguments:	  organ - Which entry to remove. W MIANOWNIKU
 * Returns:       1 - Ok, removed, 0 - Not found.
 */
public int
remove_leftover(string organ)
{
    int i;

    if (!sizeof(leftover_list))
	return 0;

    for (i = 0 ; i < sizeof(leftover_list) ; i++)
	if (leftover_list[i][1][PL_MIA] == organ)
	    leftover_list = leftover_list[0..(i - 1)] +
		leftover_list[(i + 1)..(sizeof(leftover_list))];
}

/*
 * Function name: add_attack_delay
 * Description:   Set the LIVE_I_ATTACK_DELAY prop properly.
 *                Use this function if possible instead of altering the prop.
 * Arguments:	  secs - How many seconds
 *                type - How it should be added.
 *                       0 - Just add it.
 *                       1 - Minimum value, i.e newtime=MAX(oldtime,secs)
 */
public void
add_attack_delay(int secs, int type)
{
    int old, new;
    old = new = query_prop(LIVE_I_ATTACK_DELAY);
    if (type)
    {
        if (secs > old)
	    new = secs;
    }
    else
        new += secs;
    if (new != old) add_prop(LIVE_I_ATTACK_DELAY, new);
}

/*
 * Function name: add_stun
 * Description:   Stun the living with the LIVE_I_STUNNED prop.
 *                Use this function if possible instead of altering the prop.
 */
public void
add_stun()
{
    add_prop(LIVE_I_STUNNED, query_prop(LIVE_I_STUNNED) + 1);
}

/*
 * Function name: remove_stun
 * Description:   Remove a stun made by add_stun.
 *                Use this function if possible instead of altering the prop.
 */
public void
remove_stun()
{
    int tmp = query_prop(LIVE_I_STUNNED) - 1;
    if (tmp <=  0)
        remove_prop(LIVE_I_STUNNED);
    else
        add_prop(LIVE_I_STUNNED, tmp);
}
