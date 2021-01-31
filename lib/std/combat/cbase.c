/*
 /std/combat/cbase.c

 This is the externalized combat routines. 

 This object is cloned and linked to a specific individual when
 engaged in combat. The actual object resides in 'limbo'.

 Ver 2.0 JnA: 911220

   This version uses an attack and defence table. Combat no longer have
   a concept of weapons and armours. Only attacks and hitlocations.

   This file is meant to be inherited by more advanced combat systems.

 Note that this is the implementation of the combat system in general. If
 you want to make an entirely different combat system, you are recommended
 to change the 'COMBAT_FILE' define in config.h

*/
#pragma save_binary
#pragma strict_types

#include <std.h>
#include <stdproperties.h>
#include <filter_funs.h>
#include <macros.h>
#include <formulas.h>
#include <composite.h>
#include <ss_types.h>
#include <wa_types.h>
#include <math.h>
#include <comb_mag.h>
#include <options.h>
#include <debug.h>
#include "/std/combat/combat.h"

#define DEBUG 1
#define CB_HIT_REWARD 1

/*
 * Prototypes
 */
public nomask int cb_query_panic();
public nomask void cb_attack(object ob);
static nomask int fixnorm(int offence, int defence);
static nomask void heart_beat();
static void stop_heart();

/*
    Format of each element in the attacks array:
	 ({ wchit, wcpen, dt, %use, skill, weight })
	      wchit: Weapon class to hit
	      wcpen: Weapon class penetration
	      dt:    Damage type
	      %use:  Chance of use each turn
   	      skill: The skill of this attack (defaults to wcpen)
   	      m_hit: The modified tohit used in combat
	      m_pen: The modified pen used in combat
	      weight:The weight of the tool

	 att_id:    Specific id, for humanoids W_NONE, W_RIGHT etc

    Format of each element in the hitloc_ac array:

	 ({ *ac, %hit, desc })
	      ac:    The ac's for each damagetype for a given hitlocation
	      %hit:  The chance that a hit will hit this location
	      desc:  String describing this hitlocation, ie "head", "tail"
	      m_ac:  Modified ac to use in combat
		       .......
	 Note that the sum of all %hit must be 100.

	 hit_id:    Specific id, for humanoids, A_TORSO, A_HEAD etc
*/

static int    *att_id = ({}),    /* Id's for attacks */
	      *hit_id = ({}),    /* Id's for hitlocations */
	      panic,             /* Dont panic... */
	      panic_time,        /* Time panic last checked. */
	      tohit_val,         /* A precalculated tohit value for someone */
	      hit_heart,	 /* Controls the quickness when fighting. */
	      i_am_real,	 /* True if the living object is interactive */
	      alarm_id,          /* The id of the heart_beat alarm */
	      set_speed,	 /* The base speed set externally */
	      real_speed;	 /* Really how often do we hit */

static mixed  *attacks = ({}),   /* Array of each attack */
	      *hitloc_ac = ({}); /* The armour classes for each hitloc */

static object me,                /* The living object concerned */
	      *enemies = ({}),	 /* Array holding all living I hunt */
	      tohit_ob,	         /* Last object we calculated tohit values */
	      attack_ob;	 /* Object to attack == Current enemy. */

/*
 * Description: Give status information about the combat values
 *
 */
public string
cb_status()
{
    string str, str2;
    int il, tmp, size;
    mixed ac;
    object *valid_enemies;

    enemies = enemies - ({ 0 }); /* Maybe some old enemy is dead?? */

    str = "Living object: " + file_name(me) + 
	" (Uid: " + getuid(me) + ", Euid: " + geteuid(me) + ")\n";

    str += "Combat object: " + file_name(this_object()) + 
	" (Uid: " + getuid(this_object()) + 
	    ", Euid: " + geteuid(this_object()) + ")\n";

    if (attack_ob)
	str += "Fighting: " + attack_ob->query_name() +
	    " (" + file_name(attack_ob) +")\n";

    /* if the enemies have been destroyed then it can cause problems
     * so remove the non live ones.  Left
     */
    valid_enemies = FILTER_LIVE(enemies);

    if (sizeof(valid_enemies))
       str += "Enemies:\n" + break_string(COMPOSITE_LIVE(valid_enemies, 0), 
	   76, 3);
    else
	str += "No enemies pending";

/*
    str += sprintf("\nPanic: %3d, Attacks: %3d, Hitlocations: %3d\n",
		   cb_query_panic(), sizeof(att_id), sizeof(hit_id));
*/

    il = -1;
    size = sizeof(att_id);
    while(++il < size)
    {
	if (!il)
	    str += sprintf("\n\n%-20s %@|9s\n","  Attack",
			   ({"wchit",
			     "klute  ciete  obuch  ", "wcskill",
			     "   %use" }));
	ac = attacks[il][ATT_DAMT];

	str2 = this_player()->check_call(
			    this_object()->cb_attack_desc(att_id[il]));
	if (strlen(str2) > 19)
	    str2 = str2[0..18];
			    
	str += sprintf("%-20s %|8s %-6s %-6s %-6s %|9d %|9d\n",
			str2 + ":",
			attacks[il][ATT_WCHIT] + "/" + attacks[il][ATT_M_HIT],
			(ac & W_IMPALE ? attacks[il][ATT_WCPEN][0] + "/" +
	       			attacks[il][ATT_M_PEN][0] : " ---"),
			(ac & W_SLASH ? attacks[il][ATT_WCPEN][1] + "/" +
	       			attacks[il][ATT_M_PEN][1] : " ---"),
			(ac & W_BLUDGEON ? attacks[il][ATT_WCPEN][2] + "/" +
	       			attacks[il][ATT_M_PEN][2] : " ---"),
		       attacks[il][ATT_SKILL],
		       attacks[il][ATT_PROCU]);
    }

    il = -1;
    size = sizeof(hit_id);
    while(++il < size)
    {
	if (!il)
	    str += sprintf("\n%-15s %@|9s\n","  Hit location",
			   ({"impale", "slash", "bludgeon", " %hit" }));
	str += sprintf("%-15s", hitloc_ac[il][HIT_DESC] + ":") + " ";
	ac = hitloc_ac[il][HIT_AC];
	if (!pointerp(ac))
	    ac = ({ ac, ac, ac });
	else 
	    ac = ac[0..2];
	str += sprintf("%@|9d %|9d\n", ac, hitloc_ac[il][HIT_PHIT]);
    }

    str += "\nPanic: " + cb_query_panic() + "  Parry: " + 
	me->query_skill(SS_PARRY) + "  Defense: " + 
	me->query_skill(SS_DEFENSE) + "  Combat stat av: " +
	(tmp = me->query_cmb_average_stat()) + "  Dex: " + me->query_stat(SS_DEX) +
	"  Enc: " + (me->query_encumberance_weight() +
		me->query_encumberance_volume() / 2) + "\nVol: " +
	me->query_prop(CONT_I_VOLUME) + "  Speed: " + (600 / real_speed) + 
	"(" + (600 / set_speed) + ")  Exp at kill: " +
	(F_KILL_GIVE_EXP(tmp) *
		(!i_am_real ? me->query_exp_factor() / 100 : 1)) + "\n";
		
    return str;
}

/*
 * Function name: cb_data
 * Description:   More data about combat stats.
 * Returns:	  A string to write
 */
string
cb_data()
{
    string str;
    int i, val, tmp, t2, ac, size;
    object *arr;

    str = "Living object: " + file_name(me) +
	" (Uid: " + getuid(me) + ", Euid: " + geteuid(me) + ")\n";

    str += "Combat object: " + file_name(this_object()) +
	" (Uid: " + getuid(this_object()) +
	    ", Euid: " + geteuid(this_object()) + ")\n";

    val = 2 * fixnorm(me->query_stat(SS_DEX), 50) -
	fixnorm(me->query_prop(CONT_I_VOLUME), 60000) -
	fixnorm(me->query_encumberance_weight() +
		me->query_encumberance_volume() + 5, 65);

    tmp = 0;
    i = -1;
    size = sizeof(att_id);
    while(++i < size)
    {
	tmp += attacks[i][ATT_WCHIT] * attacks[i][ATT_PROCU];
    }
    tmp /= 100;

    val += 4 * fixnorm(2 * tmp, 50);

    str += sprintf("\n%-20s %5d\n", "Offensive tohit:", val);

    i = -1;
    size = sizeof(att_id);
    while(++i < size)
    {
	ac = attacks[i][ATT_DAMT];
	if (ac & W_IMPALE)
	    tmp = attacks[i][ATT_M_PEN][0];
	else if (ac & W_SLASH)
	    tmp = attacks[i][ATT_M_PEN][1];
	else if (ac & W_BLUDGEON)
	    tmp = attacks[i][ATT_M_PEN][2];
	val += tmp * attacks[i][ATT_PROCU];
    }
    val /= 100;

    str += sprintf("%-20s %5d\n", "Offensive pen:", val);

    val = 2 * fixnorm(50, me->query_stat(SS_DEX)) -
	fixnorm(60000, me->query_prop(CONT_I_VOLUME)) -
	fixnorm(65, me->query_encumberance_weight() +
		me->query_encumberance_volume() + 5);

    if (sizeof((object *)me->query_weapon(-1) - ({ 0 })))
	tmp = me->query_skill(SS_PARRY);
    else
	tmp = me->query_skill(SS_UNARM_COMBAT) / 2;

    tmp += me->query_skill(SS_DEFENSE);

    val += 4 * fixnorm(70, tmp);

    str += sprintf("%-20s %5d\n", "Defensive tohit:", val);

    val = 0;
    i = -1;
    size = sizeof(hit_id);
    while(++i < size)
    {
	val += hitloc_ac[i][HIT_M_AC][0] * hitloc_ac[i][HIT_PHIT];
    }
    val /= 100;

    str += sprintf("%-20s %5d\n", "Defensive ac:", val);

    str += "\nExp at kill: " + (F_KILL_GIVE_EXP(me->query_average_stat()) *
	 (!i_am_real ? me->query_exp_factor() : 100) / 100) +
	 "  Speed: " + me->query_prop(LIVE_I_QUICKNESS);

    arr = all_inventory(me);
    i = -1;
    size = sizeof(arr);
    while(++i < size)
    {
	tmp += arr[i]->query_prop(OBJ_I_VALUE);
    }
    str += "  Carried value: " + tmp + " (" + sizeof(arr) + ") objects.\n";

    return str;
}

/*
 * Function name: create_object
 * Description:   Reset the combat functions
 */
public nomask void
create_object()
{
    panic_time = time();
    set_speed = 40;
    if (me)
	return;
    this_object()->create_cbase();
}

/*
 * Function name: clean_up
 * Description:   This function is called when someone wants to get rid of
 *		  this object, but is not sure if it is needed. Usually
 *		  called from the GD, or via call_out() from remove_object()
 */
public void
clean_up()
{
    if (!objectp(me))
	destruct();
}

/*
 * Function name: remove_object
 * Description:   See if we can safely remove this object. If me exists
 *		  then this object will not be removed. Use the -D flag
 *		  if you really want to get rid of this object.
 */
public void
remove_object()
{
    set_alarm(1.0, 0.0, clean_up);
}

/*
 * Function name: combat_link
 * Description:   Called by the internal combat routines on startup
 */
public void 
cb_link()
{
    if (objectp(me))
	return;

    me = previous_object();
    i_am_real = !(me->query_npc());
}

/*
 * Description: Return the connected living
 */
public object qme() 
{ 
    return me; 
}

/*
 * Function name: cb_configure
 * Description:   Configure attacks and hitlocations.
 */
public void
cb_configure()
{
    att_id = ({}); 
    hit_id = ({});
    hitloc_ac = ({}); 
    attacks = ({});
}

public int
cb_query_real_speed()
{
    int x;
    int fat, maxfat;
    
    fat = me->query_fatigue();
    maxfat = me->query_max_fatigue();

    x = set_speed + (60 * (maxfat - fat) / maxfat);
    
    return x;
}

/*
 * Function name: cb_add_panic
 * Description:   Adjust the panic level.
 * Arguments:     dpan:  The panic increase/decrease
 */
public void
cb_add_panic(int dpan)
{
    int oldpan;

    oldpan = cb_query_panic();

    panic += dpan; 
    if (panic < 0) 
	panic = 0; 

    if (!panic && oldpan)
	tell_object(me, "Uspokajasz sie.\n");
}

/*
 * Function name: cb_query_panic
 * Description:   Give the panic level.
 */
public int
cb_query_panic() 
{ 
    int n, chk;

    if (me->query_enemy(0))
	panic_time = time(); /* So we don't heal panic while fighting */
    else 
    {
	n = (time() - panic_time) / F_INTERVAL_BETWEEN_PANIC_HEALING;
	if (n > 0 && (chk = (1 << n)) > 0)
	{
	    if (panic > 0)
		panic = panic / chk;
	    panic_time += n * F_INTERVAL_BETWEEN_PANIC_HEALING;
	}
    }

    return panic; 
}

/*
 * Function name: cb_may_panic
 * Description:   Check on our panic level, act accordingly.
 */
public void
cb_may_panic()
{
    int il, size;
    object *tm;

    if (random(cb_query_panic()) > (10 + (int)me->query_stat(SS_DIS) * 3))
    {
	tell_object(me,"Wpadasz w panike!\n");
	tell_room(environment(me), QCIMIE(me, PL_MIA) + " wpada w panike!\n",
		  ({me}));
	tm = (object*)me->query_team_others();
	size = sizeof(tm);
	il = -1;
	while(++il < size)
	{
	    if (environment(me) == environment(tm[il]))
		tm[il]->add_panic(25);
	}
	me->run_away();
    }
}

public void
cb_update_combat(int extra_fatigue)
{
    int spd;
    int fatig;
    float f_spd;
    
    /*
     * Fighting is quite tiresome you know
     */
    fatig = me->query_encumberance_weight() / 15 + 1;
    if (me->query_fatigue())
	me->add_fatigue(-(fatig + extra_fatigue));
    else
    {
    	tell_object(me, "Czujesz sie ekstremalnie zmeczon" + 
    	    me->koncowka("y", "a") + ".\n");
	me->reduce_hit_point(1);
    }

    /*
     * Fighting is frightening, we might panic!
     */
    cb_may_panic();

    if (real_speed != (spd = cb_query_real_speed()))
    {
	real_speed = spd;
	remove_alarm(alarm_id);
	
	f_spd = itof(spd) / 10.0;
	
	alarm_id = set_alarm(f_spd, f_spd, heart_beat);
    }
}

/*
 * Nazwa funkcji : cb_set_speed
 * Opis          : Ustawia szybkosc walki danej postaci. 
 * Argumenty     : int speed - ilosc atakow na minute. Przyjmowane
 *			sa wartosci od 1 do 60. Standardowo, kazdy walczy
 *			z szybkoscia 15 atakow na minute.
 */
public void
cb_set_speed(int speed)
{
    if (speed < 1)
	speed = 1;
	
    if (speed > 60)
	speed = 60;
	
    set_speed = 600 / speed;
    real_speed = cb_query_real_speed();
    
    if (alarm_id)
    {
	alarm_id = set_alarm(get_alarm(alarm_id)[2], itof(real_speed) / 10.0,
	    heart_beat);
    }
}

/*
 * Nazwa funkcji : cb_query_speed
 * Opis          : Zwraca szybkosc walki danej postaci.
 * Funkcja zwraca: int - ile razy w ciagu minuty dana postac atakuje.
 */
public int
cb_query_speed()
{
    return 600 / set_speed;
}

/*
 * Function name: cb_adjust_combat_on_intox
 * Description:   Called to let intoxication affect combat. This
 *                is used to do nasty drunk type things *laugh*
 * Arguments:     pintox: %intoxicated      
 */
public void
cb_adjust_combat_on_intox(int pintox)
{
    object *p;

    if (pintox < 90)
	return;

    p = all_inventory(environment(me));

    if (!sizeof(p))
    {
	/* Here we check for neat things to do */
    }
}

/*
 * Normalize offensive / defensive values
 *
 */
static nomask int
fixnorm(int offence, int defence)
{
   if (offence + defence == 0)
       return 0;

   return ((100 * offence) / (offence + defence)) - 50;
}

/*
 * Function name: cb_update_tohit_val
 * Description:   Update the tohit value for some object. Changing ones
 *		  encumberance while fighting will not have any effect
 *		  unless this function is called, but I think it's worth
 *		  it since we save cpu.
 * Arguments:	  ob - The object we shall try to hit
 *		  weight - If the formula should be weighted any way.
 */
varargs void
cb_update_tohit_val(object ob, int weight)
{
    tohit_ob = ob;
    tohit_val = 2 * fixnorm(me->query_stat(SS_DEX), ob->query_stat(SS_DEX)) -
	fixnorm(me->query_prop(CONT_I_VOLUME), ob->query_prop(CONT_I_VOLUME)) -
	fixnorm(me->query_encumberance_weight() + me->query_encumberance_volume() + 5,
	    ob->query_encumberance_weight() + ob->query_encumberance_volume() + 5);
    tohit_val += weight;
}

/*
 * Nazwa funkcji : cb_tohit
 * Opis          : Sprawdza, czy trafilismy nasza ofiare, czy nie. Zalezy
 *		   to od wchit broni, roznicy w statach, umiejetnosci
 *		   walki, liczby przeciwnikow, roznicy rozmiarow i
 *		   ewentualnych modyfikatorow za brak widocznosci.
 * Argumenty     : int aid - identyfikator ataku,
 *		   int wchit - m_hit (modified to_hit) broni,
 *		   int weight - waga broni,
 *		   object vic - obiekt postaci, ktora chcemy trafic.
 * Funkcja zwraca: Zwraca tablice w postaci:
 *		    ({
 *			int - Jesli > 0 oznacza jak dobrze trafilismy, 
 *			      w przeciwnym wypadku jak bardzo spudlowalismy,
 *			mixed - W przypadku nietrafienia, moze przyjmowac
 *				takie wartosci, oznaczajace powod nietrafienia:
 *				    0 - przeciwnik zrobil unik,
 *				    1 - my spudlowalismy,
 *				    ob - obiekt tarczy lub broni ktora parowala.
 *				W przypadku trafienia - 0.
 *		    })
 */
public mixed
cb_tohit(int aid, int wchit, int weight, object vic)
{
    int tmp, whit, bskill, bskill_tmp;
    int *what_defended, deb1, deb2;
    mixed def; 

    /*
     * Four factors are normalized (-50, 50) in the 'to-hit'.
     * 1 - Weapon class 'to hit' <-> Defensive skill
     * 2 - Weight
     * 3 - Volume
     * 4 - Dexterity
     * These are weighted with the factors (4, 1, 1, 2)
     */
     
    /*
     * Tablica what_defended ma postac :
     * [0] - bonus parowania bronia albo tarcza
     * [1] - bonus z unikow
     * [2] - bonus z nietrafienia (nieumiejetnosc poslugiwania sie bronia)
     */

    tmp = (int)vic->query_skill(SS_DEFENCE);
    what_defended = ({ 0, tmp, (200 / wchit) });

#if 0
    if (vic->debugging())
    {
	vic->catch_msg(sprintf("DEF: %d - %d - %d.\n",
	    what_defended[0], what_defended[1], what_defended[2]));
    }
#endif

    def = vic->query_defense(weight * me->query_stat(SS_STR));

    if (sizeof(def) == 2)
    {
	/* Atak bronia lub obrona tarcza -> parowanie.
 	 */
	if (def[0] >= 0)
	    what_defended[0] = def[0];
	else
	{
	    /* Atak unarmed i obrona bronia -> nie paruje sie. Niemniej,
	     * dodaje sie do obrony.
	     */
	    def[0] = -def[0];
	    what_defended[2] = def[0];
	}

	tmp += def[0];
    }
    
    /*
     * Sprytny haczyk autorstwa Silvathraeca na walke z wieloma
     * przeciwnikami. Gdy uderzamy osobe, ktora nie uderza nas,
     * ma ona dwa razy mniejsza obrone.
     */
    if (vic->query_attack() != me)
	tmp /= 2;
    
    /*
     * Is it dark or opponent invis? Then how well do we fight?
     */

    if (!CAN_SEE_IN_ROOM(me) || !CAN_SEE(me, vic))
    {
	/* A npc shouldn't loose all fightingskill if it is dark and no
	 * blind fight skill is set. */
	bskill = me->query_skill(SS_BLIND_COMBAT);
	bskill = ((bskill == 0) && !i_am_real) ? 20 : bskill;
	what_defended[2] += (100 - bskill) * wchit / 25;//
	wchit = bskill * wchit / 100;
    }

    if (!CAN_SEE_IN_ROOM(vic) || !CAN_SEE(vic, me))
    {
	bskill_tmp = vic->query_skill(SS_BLIND_COMBAT);
	bskill_tmp = ((bskill_tmp == 0) && vic->query_npc()) ? 20 : bskill_tmp;
	tmp = bskill_tmp * tmp / 100;
    }

    deb1 = random(wchit) + random(wchit) + random(wchit);
    deb1 /= 3;
    deb2 = random(tmp);

    whit = 4 * fixnorm(deb1, deb2);

    if (vic != tohit_ob)
	cb_update_tohit_val(vic);

#if 0
    if (me->debugging())
	me->catch_msg(sprintf("hit: %d(%d), def: %d(%d) => fn: %d + %d = %d.\n",
	    deb1, wchit, deb2, tmp, whit, tohit_val, whit + tohit_val));
#endif

    whit += tohit_val;


    if (whit > 0)
	return ({ whit, 0 });

    /* Zmienna tohit_val wpisuje sie na konto uniku.
     */
    if (tohit_val < 0)
	what_defended[1] -= tohit_val;
	
    /* Generalnie rzecz biorac, parowanie jest duzo bardziej naturalne
     * i duzo bardziej czeste, niz inne formy unikania ciosow. Nieraz
     * nawet mogac uniknac ciosu, wolimy go sparowac. Powinnismy
     * uwzglednic to w opisach.
     */
    if (what_defended[0] > 0)
    {
	tmp = what_defended[0] / 2;
	what_defended[1] -= tmp;
	what_defended[0] += tmp;
    }

#if 0	
    if (vic->debugging())
    {
	vic->catch_msg(sprintf("DEF: %d - %d - %d.\n",
	    what_defended[0], what_defended[1], what_defended[2]));
    }
#endif

    tmp = random(what_defended[0] + what_defended[1] + what_defended[2]);

    if (tmp < what_defended[0])
    {
	def[1]->did_parry(); // Dajemy znac parujacej broni/tarczy.
	return ({ (whit - 1), def[1] });
    }
    else if (tmp < (what_defended[0] + what_defended[1]))
    {
	return ({ (whit - 1), 0 });
    }
    else
    {        
	return ({ (whit - 1), 1 });
    }
    
    return 0;
}

/*
 * Function name: cb_try_hit
 * Description:   Decide if we a certain attack fails because of something
 *                related to the attack itself, ie specific weapon that only
 *		  works some of the time. This is supposed to be
 *                replaced by a more intelligent routine in creature and
 *                humanoid combat. (called from heart_beat)
 * Arguments:     aid:   The attack id
 * Returns:       True if hit, otherwise 0.
 */
public int
cb_try_hit(int aid) 
{ 
    return 1; 
}

/*
 * Function name: cb_got_hit
 * Description:   Tells us that we got hit. It can be used to reduce the ac
 *                for a given hitlocation for each hit. This is supposed to be
 *                replaced by a more intelligent routine in creature and
 *                humanoid combat. (called from cb_hit_me)
 * Arguments:     hid:   The hitloc id
 *                ph:    The %hurt
 *                att:   Attacker
 *		  aid:   The attack id of the attacker
 *                dt:    The damagetype
 *		  dam:	 The number of hitpoints taken
 */
public varargs object
cb_got_hit(int hid, int ph, object att, int aid, int dt, int dam) 
{
    return 0;
}

/*
 * Function name: cb_attack_desc
 * Description:   Gives the description of a certain attack slot. This is
 *		  supposed to be replaced by more intelligent routines in
 *                humanoid and creature combat
 * Arguments:     aid:   The attack id
 * Returns:       string holding description
 */
public string
cb_attack_desc(int aid)
{
    return "cialem";
}

public string
cb_hitloc_desc(int hid)
{
    int i = member_array(hid, hit_id);

    if (i < 0)
    {
        return "korpus";
    }

    return hitloc_ac[i][HIT_DESC];
}

/* 
 * Function name: tell_watcher
 * Description:   Send the string from the fight to people that want them
 * Arguments:	  str   - The string to send
 *		  enemy - Who the enemy was
 *		  arr   - Array of objects never to send message
 */
varargs void
tell_watcher(string str, object enemy, mixed arr)
{
    object *ob;
    int i, size;

    ob = all_inventory(environment(me)) - ({ me, enemy });

    if (arr)
    {
	if (pointerp(arr))
	    ob -= arr;
	else
	    ob -= ({ arr });
    }

    i = -1;
    size = sizeof(ob);
    while(++i < size)
    {
	if (!ob[i]->query_option(OPT_BLOOD) && CAN_SEE_IN_ROOM(ob[i]))
	{
	    if (CAN_SEE(ob[i], me))
	    	ob[i]->catch_msg(str);
	    else
		tell_object(ob[i], enemy->query_Imie(ob[i], PL_MIA) +
		    " zostaje przez kogos trafion" +
		    enemy->koncowka("y", "a", "e") + ".\n");
	}
    }
}

/*
 * Function name: cb_did_hit
 * Description:   Tells us that we hit something. Should produce combat
 *                messages to all relevant parties. This is supposed to be
 *                replaced by a more intelligent routine in creature and
 *                humanoid combat. (called from heart_beat)
 * Arguments:     aid:    The attack id
 *                hdesc:  The hitlocation description.
 *                phurt:  The %hurt made on the enemy
 *                enemy:  The enemy who got hit
 *		  dt:	  The current damagetype
 *		  phit:   The %success that we made with our weapon
 *			  If this is negative, it indicates fail
 *		  dam:    Damage we did in hit points
 *		  tohit:  How well did we hit
 *		  def_ob: Obj that defended or how we defended (if miss)
 *		  armour: Armour on the hitlocation being hit
 */
public varargs void
cb_did_hit(int aid, string hdesc, int phurt, object enemy, int dt,
	   int phit, int dam, int tohit, mixed def_ob, object armour)
{
    object wep;
    string zaimek;
    string what;
    string with;
    string zbroja;

    if ((!objectp(enemy)) || (!objectp(me)))
	return;
    
    with = cb_attack_desc(aid);

    if (phurt == -1)
    {
	cb_add_panic(1);

	if (objectp(def_ob))
	{
	    zaimek = enemy->koncowka("ten", "ta", "ono");

	    if (def_ob->check_weapon()) // paruje bron
	    {
		if (dt == W_IMPALE)
		{
		    if (i_am_real)
		    {
			me->catch_msg(sprintf("Wyprowadzasz szybkie " +
			    "pchniecie %s w %s, lecz %s zbija je z lini " +
			    "ataku %s.\n", with, enemy->query_imie(me, PL_BIE),
			    zaimek, def_ob->short(me, PL_NAR)));
		    }
		    if (interactive(enemy))
		    {
			enemy->catch_msg(sprintf("%s wyprowadza szybkie " +
			    "pchniecie %s w ciebie, lecz tobie udaje sie " +
			    "zbic je z lini ataku %s.\n",
			    me->query_Imie(enemy, PL_MIA), with,
			    def_ob->short(enemy, PL_NAR)));
		    }
		    tell_watcher(sprintf("%s wyprowadza szybkie pchniecie " +
			"%s w %s, lecz %s zbija je z lini ataku %s.\n",
			QCIMIE(me, PL_MIA), with, QIMIE(enemy, PL_BIE),
			zaimek, QSHORT(def_ob, PL_NAR)), enemy);
		    
		    return ;
		}
		
		if (dt == W_BLUDGEON)
		{
		    if (i_am_real)
		    {
			me->catch_msg(sprintf("Wykonujesz zamach %s mierzac " +
			    "w %s, lecz %s paruje go %s.\n", with,
			    enemy->query_imie(me, PL_BIE), zaimek, 
			    def_ob->short(me, PL_NAR)));
		    }
		    if (interactive(enemy))
		    {
			enemy->catch_msg(sprintf("%s wykonuje zamach %s " +
			    "mierzac w ciebie, lecz tobie udaje sie go " +
			    "sparowac %s.\n", me->query_Imie(enemy, PL_MIA),
			    with, def_ob->short(enemy, PL_NAR)));
		    }
		    tell_watcher(sprintf("%s wykonuje zamach %s mierzac w " +
			"%s, lecz %s paruje go %s.\n", QCIMIE(me, PL_MIA),
			with, QIMIE(enemy, PL_BIE), zaimek,
			QSHORT(def_ob, PL_NAR)), enemy);
		    
		    return ;
		}
		if (dt == W_SLASH)
		{
		    if (i_am_real)
		    {
			me->catch_msg(sprintf("Wykonujesz zamaszyste " +
			    "ciecie %s mierzac w %s, lecz %s zbija je z " +
			    "lini ataku %s.\n", with,
			    enemy->query_imie(me, PL_BIE), zaimek,
			    def_ob->short(enemy, PL_NAR)));
		    }
		    if (interactive(enemy))
		    {
			enemy->catch_msg(sprintf("%s wykonuje zamaszyste " +
			    "ciecie %s mierzac w ciebie, lecz tobie udaje " +
			    "je zbic z lini ataku %s.\n",
			    me->query_Imie(enemy, PL_MIA), with,
			    def_ob->short(me, PL_NAR)));
		    }
		    tell_watcher(sprintf("%s wykonuje zamaszyste ciecie %s " +
			"mierzac w %s, lecz %s zbija je z lini ataku %s.\n",
			QCIMIE(me, PL_MIA), with, QIMIE(enemy, PL_BIE),
			zaimek, QSHORT(def_ob, PL_NAR)), enemy);
		    
		    return ;
		}
	    }
	    else
	    {
		if (dt == W_IMPALE)
		{
		    if (i_am_real)
		    {
			me->catch_msg("Wyprowadzasz szybkie pchniecie " + with +
			    " w " + enemy->query_imie(me, PL_BIE) +
			    ", lecz " + zaimek + " oslania sie " + 
			    def_ob->short(me, PL_NAR) + ".\n");
		    }
		    if (interactive(enemy))
		    {
			enemy->catch_msg(me->query_Imie(enemy, PL_MIA) +
			    " wyprowadza szybkie pchniecie " + with + 
			    " w ciebie, lecz tobie udaje sie oslonic " + 
			    def_ob->short(enemy, PL_NAR) + ".\n");
		    }
		    tell_watcher(QCIMIE(me, PL_MIA) + " wyprowadza " +
			"szybkie pchniecie " + with + " w " + 
			QIMIE(enemy, PL_BIE) + ", lecz " + zaimek + " oslania " +
			"sie " + QSHORT(def_ob, PL_NAR) + ".\n", enemy);
		    
		    return ;
		}
		
		if (dt == W_BLUDGEON)
		{
		    if (i_am_real)
		    {
			me->catch_msg("Robisz zamach " + with + " mierzac w " +
			    enemy->query_imie(me, PL_BIE) + ", lecz " + zaimek + 
			    " oslania sie " + def_ob->short(me, PL_NAR) + ".\n");
		    }
		    if (interactive(enemy))
		    {
			enemy->catch_msg(me->query_Imie(enemy, PL_MIA) + 
			    " wykonuje zamach " + with + " mierzac w ciebie, " +
			    "lecz tobie udaje sie oslonic " + 
			    def_ob->short(enemy, PL_NAR) + ".\n");
		    }
		    tell_watcher(QCIMIE(me, PL_MIA) + " wykonuje zamach " +
			with + " mierzac w " + QIMIE(enemy, PL_BIE) + ", lecz " +
			zaimek + " paruje go " + QSHORT(def_ob, PL_NAR) + ".\n", 
			enemy);
		    
		    return ;
		}
		if (dt == W_SLASH)
		{
		    if (i_am_real)
		    {
			me->catch_msg("Wykonujesz zamaszyste ciecie " + with +
			    " mierzac w " + enemy->query_imie(me, PL_BIE) +
			    ", lecz " + zaimek + " oslania sie " + 
			    def_ob->short(enemy, PL_NAR) + ".\n");
		    }
		    if (interactive(enemy))
		    {
			enemy->catch_msg(me->query_Imie(enemy, PL_MIA) + 
			    " wykonuje zamaszyste ciecie " + with + " mierzac w " +
			    "ciebie, lecz udaje ci sie oslonic " + 
			    def_ob->short(me, PL_NAR) + ".\n");
		    }
		    tell_watcher(QCIMIE(me, PL_MIA) + " wykonuje " +
			"zamaszyste ciecie " + with + " mierzac w " +
			QIMIE(enemy, PL_BIE) + ", lecz " + zaimek + 
			" oslania sie " + QSHORT(def_ob, PL_NAR) + ".\n", enemy);
		    
		    return ;
		}
	    }
	}
	else if (!def_ob) /* unik broniacego */
	{
	    zaimek = enemy->koncowka("ten", "ta", "ono");

	    if (i_am_real)
	    {
		me->catch_msg("Probujesz trafic " + 
		    enemy->query_imie(me, PL_BIE) + " " + with + ", lecz " +
		    zaimek + " uskakuje przed twoim ciosem.\n");
	    }
	    if (interactive(enemy))
	    {
		enemy->catch_msg(me->query_Imie(enemy, PL_MIA) + " probuje " +
		    "cie trafic " + with + ", lecz tobie udaje sie uniknac " +
		    "tego ciosu.\n");
	    }
	    tell_watcher(QCIMIE(me, PL_MIA) + " probuje trafic " +
		QIMIE(enemy, PL_BIE) + " " + with + ", lecz " + zaimek +
		" uskakuje przed tym ciosem.\n", enemy);
	    return;
	}
	else /* nietrafienie (nieumiejetnosc atakujacego) */
	{
	    if (i_am_real)
	    {
		me->catch_msg("Nie udaje ci sie trafic " + 
		    enemy->query_imie(me, PL_DOP) + " " + with + ".\n");
	    }
	    if (interactive(enemy))
	    {
		enemy->catch_msg(me->query_Imie(enemy, PL_CEL) + " nie udaje " +
		    "sie trafic ciebie " + with + ".\n");
	    }
	    tell_watcher(QCIMIE(me, PL_CEL) + " nie udaje sie trafic " +
		QIMIE(enemy, PL_DOP) + " " + with + ".\n", enemy);
	    return;
	}
    }
    
    if ((phurt == 0) && armour) // zbroja wyparowala
    {
	zbroja = armour->short(PL_BIE);

	if (i_am_real)
	{
	    me->catch_msg("Trafiasz " + enemy->query_imie(me, PL_BIE) + " " +
		with + " w " + hdesc + ", lecz caly impet uderzenia " +
		"wyparowany zostaje przez " + zbroja + ".\n");
	}
	if (interactive(enemy))
	{
	    enemy->catch_msg(me->query_Imie(enemy, PL_MIA) + " trafia cie " + 
		with + " w " + hdesc + ", lecz caly impet uderzenia " +
		"wyparowany zostaje przez " + zbroja + ".\n");
	}
	tell_watcher(QCIMIE(me, PL_MIA) + " trafia " + QIMIE(enemy, PL_BIE) +
	    " " + with + " w " + hdesc + ", lecz caly impet uderzenia " +
	    "wyparowany zostaje przez " + zbroja + ".\n", enemy);
    
	return ;
    }

    cb_add_panic(-3 - (phurt / 5));
    zaimek = enemy->query_zaimek(PL_BIE, 0);

/*
dobrze/kiepsko trafiasz Genowefe dlugim mieczem w glowe, zadajac 
  lekkie/powazne obrazenia.
  
Trafiasz Genowefe dlugim mieczem w glowe, zadajac lekkie/powazne obrazenia.

Ciezko ranisz Genowefe w glowe, zadajac lekkie/powazne obrazenia.

Ciezko ranisz Genowefe, trafiajac ja w glowe dlugim mieczem.
Ciezko ranisz Genowefe dlugim mieczem, trafiajac ja w glowe.

ledwo muskasz		5
lekko ranisz		5
ranisz			10
powaznie ranisz		30
bardzo ciezko ranisz	30
masakrujesz		20

stare:
[0..4, 5..9, 10..19, 20..29, 30..49, 50..69, 70..89, 90..100]
    5,    5,     10,     10,     20,     20,     20,      10
*/

    switch (phurt)
    {
    case 0..4:   /* [5] */
	what = "ledwo muska";
	break;
    case 5..9:   /* [5] */
	what = "lekko rani";
	break;
    case 10..19: /* [10] */
	what = "rani";
	break;
    case 20..49: /* [30] */
	what = "powaznie rani";
	break;
    case 50..79: /* [30] */
	what = "bardzo ciezko rani";
	break;
    default:     /* [20] */
	what = "masakruje";
	break;
    }

    if (i_am_real)
    {
	me->catch_msg(capitalize(what) + "sz " + enemy->query_imie(me, PL_BIE)
	    + " " + with + ", trafiajac " + zaimek + " w " + hdesc + ".\n");
    }
    if (interactive(enemy))
    {
	enemy->catch_msg(me->query_Imie(enemy, PL_MIA) + " " + what +
	    " cie " + with + ", trafiajac cie w " + hdesc + ".\n");
    }
    tell_watcher(QCIMIE(me, PL_MIA) + " " + what + " " + QIMIE(enemy, PL_BIE) +
	" " + with + ", trafiajac " + zaimek + " w " + hdesc + ".\n", enemy);


#if 0
    switch (phurt)
    {
    case 0..4:
	how = ".\n";
	what = "tickle";
	owhat = "tickles";
	break;
    case 5..9:
	how = ".\n";
	what = "graze";
	owhat = "grazes";
	break;
    case 10..19:
	how = ".\n";
	what = "hurt";
	owhat = "hurts";
	break;
    case 20..29:
	how = ", rather bad.\n";
	what = "hurt";
 	owhat = "hurts";
	break;
    case 30..49:
	how = ", very bad.\n";
	what = "hurt";
	owhat = "hurts";
	break;
    case 50..69:
	how = ", very hard.\n";
	what = "smash";
	owhat = "smashes";
	break;
    case 70..89:
	how = ", with a bone crushing sound.\n";
	what = "smash";
	owhat = "smashes";
	where = " ";
	break;
    default:
	how = ".\n";
	what = "massacre";
	owhat = "massacres";
	where = " ";
    }

    if (i_am_real)
    {
	me->catch_msg("You " + what + where + enemy->query_the_name(me) +
	    " with your " + with + how);
    }
    if (interactive(enemy))
    {
	enemy->catch_msg(QCTNAME(me) + " " + owhat + " your " + hdesc +
	    " with " + me->query_possessive() + " " + with + how);
    }
    tell_watcher(QCTNAME(me) + " " + owhat + where + QTNAME(enemy) +
	" with " + me->query_possessive() + " " + with + how, enemy);
#endif
}

public mixed
cb_query_defense(int weight)
{
    return 0;
}

public int
filter_to_reward(object ob)
{
    return (living(ob) &&
		((member_array(ob, enemies) != -1) ||
		 (member_array(me, ob->query_enemy(-1)) != -1)));
}

/*
 * Function name: cb_reward
 * Description:   Reward the attacker of 'me'. Can be replaced.
 * Arguments:	  attacker:   Enemy that attacks 'me'
 *                dam:        The amount of damage in hitpoints made
 *                kill:       True if the attack killed me
 */
public void
cb_reward(object attacker, int dam, int kill)
{
    int tmp, j, align, size;
    object *share, *others;

    if (!kill)
    {
	attacker->add_exp(dam, 1);
	return;
    }

    tmp = F_KILL_GIVE_EXP(me->query_cmb_average_stat());
    if (!i_am_real)
	tmp = me->query_exp_factor() * tmp / 100;

    share = filter(all_inventory(environment(me)), filter_to_reward);

    if (size = sizeof(share))
    {	
	tmp /= size;

	while (--size >= 0)
	    share[size]->add_exp(tmp, 1);
    }

#if 0
    /*
     * Let the team share the xp
     */
    others = attacker->query_team_others();
    
    if (others)
	share = others & all_inventory(environment(me));
	 
    if (size = sizeof(share))
    {	
	share += ({ attacker });
	tmp /= (size + 1);
	
	while (size >= 0)
	{
	    /* A slight check to be sure the exp is honestly received. */
	    if(member_array(share[size], enemies) != -1 ||
	       member_array(me, share[size]->query_enemy(-1)) != -1)
	    {
		share[size]->add_exp(tmp, 1);
	    }
	    size--;
	}
    }
    else
    {
	attacker->add_exp(tmp, 1);
    }
#endif

    /*
     * Change the alignment of the killer
     */
    attacker->set_alignment((align = attacker->query_alignment()) +
	F_KILL_ADJUST_ALIGN(align, me->query_alignment()));
}

/*
 * Function name: cb_death_occured
 * Description:   Called when 'me' dies
 * Arguments:     killer: The enemy that caused our death.
 */
public void
cb_death_occured(object killer)
{
    int il, size;
    object *tm;

    /*
     * Tell everyone the bad (good) news.
     */
    tell_room(environment(me), QCIMIE(me, PL_MIA) + " polegl" +
	me->koncowka("", "a", "o") + ".\n", ({me}));
    tell_object(killer, "Zabil" + killer->koncowka("es", "as") + " " + 
	me->query_imie(killer, PL_BIE) + ".\n");
	
    if (living(killer))
	tell_roombb(environment(me), QCIMIE(killer, PL_MIA) + " zabil" +
	    killer->koncowka("", "a", "o", "i", "y") + " " + 
	    QIMIE(me, PL_BIE) + ".\n", ({me, killer}), killer);
	
    stop_heart();

    /*
     * Reward for kill (den enes d|d, den andres br|d)
     */
    cb_reward(killer, 0, 1);

    /*
     * We forget our enemies when we die.
     */
    enemies = ({});
    attack_ob = 0;

    /* 
     * Adjust panic values
     */
    killer->add_panic(-25); /* Killing the enemy reduces panic */
    tm = (object*)killer->query_team_others();
    il = -1;
    size = sizeof(tm);
    while(++il < size)
    {
	if (environment(killer) == environment(tm[il]))
	{
	    tm[il]->add_panic(-15);
	}
    }

    tm = (object*)me->query_team_others();
    il = -1;
    size = sizeof(tm);
    while(++il < size)
    {
	if (environment(me) == environment(tm[il]))
	{
	    tm[il]->add_panic(25);
	}
    }
}

/*
 * Function name: cb_add_enemy
 * Description:   Used to add enemies to 'me'
 * Arguments:     enemy: The enemy to be
 *                force: If true and enemy array full one other is replaced
 */
public varargs void
cb_add_enemy(object enemy, int force)
{
    int pos;
    enemies = enemies - ({ 0 });
  /* Make sure panic value is updated before we add enemies */
    cb_query_panic(); 
    pos = member_array(enemy, enemies);

    if (force && pos >= 0)
	enemies = ({enemy}) + exclude_array(enemies, pos, pos);
    else if (force)
	enemies = ({ enemy }) + enemies;
    else if (pos < 0)
	enemies = enemies + ({ enemy });

    if (sizeof(enemies) > MAX_ENEMIES)
	enemies = slice_array(enemies, 0, MAX_ENEMIES - 1);
}

/*
 * Function name: cb_adjust_combat_on_move
 * Description:   Called to let movement affect the ongoing fight. This
 *                is used to print hunting messages or drag enemies along.
 * Arguments:	  True if leaving else arriving
 */
public void 
cb_adjust_combat_on_move(int leave)
{
    int i, pos, size;
    object *inv, enemy, *all, *rest, *drag;

    if (sizeof(enemies) && environment(me))
    {
	all = all_inventory(environment(me));
	inv = all & enemies;
	size = sizeof(inv);
	if (leave)
	{
	    /*
	     * If the aggressors are around.
	     */
	    if (size)
	    {
		drag = ({ });
		rest = ({ });
		i = -1;
		while(++i < size)
		{
		    if (inv[i]->query_prop(LIVE_O_ENEMY_CLING) == me)
		    {
			drag += ({ inv[i] });
			tell_object(inv[i], 
			    me->query_Imie(inv[i], PL_MIA) + 
			    " wybywa, ciagnac ciebie za soba.\n");
		    }
		    else
		    {
			rest += ({ inv[i] });
/*
			tell_object(inv[i], "You are now hunting " +
			    me->query_the_name(inv[i]) + ".\n");
*/
			tell_object(inv[i], me->query_Imie(inv[i], PL_MIA) + 
			    " uciekl" + me->koncowka("", "a", "o") +
			    " ci.\n");
		    }
		}

		if (sizeof(drag))
		{
		    if (i_am_real)
		    {
			me->catch_msg("Wybywasz, ciagnac za soba " +
			    COMPOSITE_LIVE(drag, PL_BIE) + ".\n");
		    }
		    me->add_prop(TEMP_DRAGGED_ENEMIES, drag);
		}

		if (sizeof(rest) && i_am_real)
		{
		    me->catch_msg("Uciekl" + me->koncowka("es", "as") +
			" " + COMPOSITE_LIVE(rest, PL_CEL) + ".\n"); 
		}

		/* Stop fighting all the enemies that don't follow us.
		   We must still fight the enemies that do, since otherwise
		   we can move so quickly that we don't update our enemies
		   to include them when they attack again, although they
		   will autofollow and attack again on entry.
		*/
		this_object()->cb_stop_fight(rest);
		rest->notify_i_escaped(me);
	    }
    	} 
	else 
	{
	    if (size)
	    {
		i = -1;
		while(++i < size)
		{
		    if (CAN_SEE(me, inv[i]) && CAN_SEE_IN_ROOM(me) &&
			!NPATTACK(inv[i]))
		    {
			me->attack_object(inv[i]);
			cb_update_tohit_val(inv[i], 30); /* Give hunter bonus */
			tell_room(environment(me), QCIMIE(me, PL_MIA) + 
			    " atakuje " + QIMIE(inv[i], PL_BIE) + ".\n",
			    ({ inv[i], me }));
			tell_object(inv[i], me->query_Imie(inv[i], PL_MIA) +
			    " atakuje ciebie!\n");
			tell_object(me, "Atakujesz " +
			    inv[i]->query_imie(me, PL_BIE) + ".\n");
		    }
		}
	    }
/*
	    else
	    {
		dump_array("+CZYSZCZE+");
		attack_ob = 0; // dodane ostatnio - czy sie cos kopie ?
	    }
*/
	}
    }
}

/*
 * Function name: cb_run_away
 * Description:   'me' runs away from the fight
 * Arguments:     dir: The first dir tried
 */
public void
cb_run_away(string dir)
{
    object          here;
    int             size, pos,
		    i,
		    j;
    mixed	    *exits;
    string	    *std_exits, old_mout, old_min, exit;


    if (me->query_ghost() ||
		(!i_am_real && me->query_prop(NPC_I_NO_RUN_AWAY)))
	return;

    here = environment(me);
    i = 0;
    std_exits = ({ "polnoc", "poludnie", "zachod", "wschod", "gora", "dol" });
    if (stringp(dir))
	me->command(dir);

    exits = here->query_exit();
    size = sizeof(exits);
    j = random(size / 3);

    while (i < size && here == environment(me))
    {
	i += 3;
	exit = (pointerp(exits[j * 3 + 1]) ? exits[j * 3 + 1][0]
					   : exits[j * 3 + 1]);
	if ((pos = member_array(exit, std_exits)) > -1)
	    std_exits[pos] = "";
	old_mout = me->query_m_out();
	me->set_m_out("w panice wybiega");
	old_min = me->query_m_in();
	me->set_m_in("wbiega, drzac ze strachu.");
	catch(me->command(exit));
	me->set_m_out(old_mout);
	me->set_m_in(old_min);
	j++;
	if (j * 3 >= size)
	    j = 0;
    }

    size = sizeof(std_exits);
    j = random(size);
    i = 0;
    while (i < size && here == environment(me))
    {
	i++;
	if (strlen(std_exits[j]))
	{
	    old_mout = me->query_m_out();
	    me->set_m_out("w panice wybiega");
	    old_min = me->query_m_in();
	    me->set_m_in("wbiega, drzac ze strachu.");
	    catch(me->command(std_exits[j]));
	    me->set_m_out(old_mout);
	    me->set_m_in(old_min);
	}
	j++;
	if (j >= size)
	    j = 0;
    }

    if (here == environment(me))
    {
	tell_room(environment(me), QCIMIE(me, PL_MIA) + " sprobowal" +
	    me->koncowka("", "a", "o") + " uciec, ale " +
	    me->koncowka("mu", "jej") + " sie to nie udalo.\n", ({me}));

	tell_object(me, "Sprobowal" + me->koncowka("es", "as") + " uciec, " +
		    "ale ci sie to nie udalo.\n");
    }
    else
    {
	tell_object(me, "Udalo ci sie gdzies uciec!\n");
    }
}

public void
cb_enemy_escaped()
{
    object *new;

    /*
     * To cling to an enemy we must fight it.
     */
    me->remove_prop(LIVE_O_ENEMY_CLING);

    /*
     * Switch enemy if we have an alternate
     */
    enemies = enemies - ({ 0 });
    new = (all_inventory(environment(me)) & enemies) - ({ attack_ob });

    if (sizeof(new))
	attack_ob = new[0];
    else
    {
	if (attack_ob && attack_ob->query_ghost())
	{
	    me->remove_prop(LIVE_I_ATTACK_DELAY);
	    me->remove_prop(LIVE_I_STUNNED);
	}
	    attack_ob = 0;
    }

   /*
    * We attack another enemy when old enemy left.
    */
    if (attack_ob)
    {
	tell_object(me, "Koncentrujesz sie na walce z " +
	    attack_ob->query_imie(me, PL_NAR) +".\n");
	heart_beat();
	return;
    }
    else
    {
	stop_heart();
	return;
    }
}

/*
 * Function name: cb_wield_weapon
 * Description:   Wield a weapon. 'Weapon' is here a general term for any tool
 *                used as a weapon. Only players are limited to /std/weapon
 *		  weapons.
 * Arguments:	  wep - The weapon to wield.
 * Returns:       True if wielded.
 */
public mixed
cb_wield_weapon(object wep) 
{ 
    return ""; 
}

/*
 * Function name: cb_show_wielded
 * Description:   Describe the currently wielded weapons.
 * Argumensts:    ob: The object to give the description
 * Returns:       Description string.
 */
public string 
cb_show_wielded(object ob) 
{ 
    return ""; 
}

/*
 * Function name: unwield
 * Description:   Unwield a weapon.
 * Arguments:	  wep - The weapon to unwield.
 * Returns:       None.
 */
public void
cb_unwield(object wep) 
{
}

/*
 * Function name: cb_query_weapon
 * Description:   Returns the weapon held in a specified location.
 *		  A list of all if argument -1 is given.
 * Arguments:	  which: A numeric label describing a weapon
 *                       location. On humanoids this is W_RIGHT etc.
 * Returns:       The corresponding weapon.
 */
public mixed
cb_query_weapon(int which)
{
    if (which == -1)
	return ({});
    else
	return 0; 
}

/*
 * Function name: cb_wear_arm
 * Description:   Wear an armour
 * Arguments:	  arm - The armour.
 */
public int
cb_wear_arm(object arm) 
{ 
    return 0; 
}

/*
 * Function name: cb_show_worn
 * Description:   Describe the currently worn armours
 * Argumensts:    ob: The object to give the description
 * Returns:       Description string.
 */
public string
cb_show_worn(object ob) 
{ 
    return ""; 
}

/*
 * Function name: cb_remove_arm
 * Description:   Remove an armour
 * Arguments:	  arm - The armour.
 */
public void
cb_remove_arm(object arm) 
{
}

/*
 * Function name: cb_query_armour
 * Description:   Returns the armour of a given position.
 *		  A list of all if argument -1 is given.
 * Arguments:	  which: A numeric label describing an armour
 *                       location. On humanoids this is TS_HEAD etc.
 * Returns:       The corresponding armour
 */
public mixed
cb_query_armour(int which)
{
    if (which == -1)
	return ({});
    else
	return 0; 
}

/***********************************************************
 * The non redefinable functions follows below
 */

static void
restart_heart()
{
    if (!alarm_id || !get_alarm(alarm_id))
    {
	float spd;
	real_speed = cb_query_speed();
	spd = itof(real_speed) / 10.0;
	alarm_id = set_alarm(spd, spd, heart_beat);
    }
}

static void
stop_heart()
{
    me->remove_prop(LIVE_I_ATTACK_DELAY);
    remove_alarm(alarm_id);
    alarm_id = 0;
}

/*
 * Function name: heart_beat
 * Description:   Do 1 round of fighting with the choosen enemy. This is
 *                done as long as both me and enemy is alive and in the
 *                same place.
 */
static nomask void
heart_beat()
{
    int             il, dt, hitsuc, tmp, size, phit, extra_fatigue;
    string 	    logtext;
    mixed	    hitresult, *dbits, pen, fail;
    object          *new, ob, defend_how;
    
    if (!objectp(me) || me->query_ghost())
    {
	attack_ob = 0;
	stop_heart();
	return;
    }

    /*
     * Do something when the enemy is somehow lost
     */
    if (!attack_ob || attack_ob->query_ghost() ||
	environment(attack_ob) != environment(me))
    {
	me->notify_enemy_gone(attack_ob);
	cb_enemy_escaped();
	return;
    }

    /* 
     * The enemy is still here, run through our attacks
     */

    /* First do some check if we actually attack
    */
    if (pointerp(fail = me->query_prop(LIVE_AS_ATTACK_FUMBLE)) &&
	sizeof(fail))
    {
	if (i_am_real)
	{
	    me->catch_msg(fail[0]);
	}
	return;
    }

    if ((tmp = me->query_prop(LIVE_I_ATTACK_DELAY)))
    {
	if ((tmp -= (real_speed / 10)) > 0)
	{
	    me->add_prop(LIVE_I_ATTACK_DELAY, tmp);
	    return;
	}
	else
	    me->remove_prop(LIVE_I_ATTACK_DELAY);
    }

#if 0
    if (((hit_heart * 100) + me->query_prop(LIVE_I_QUICKNESS)) < random(100))
    	return 1; /* Wait one round more then. */
    hit_heart = -1;
#endif

    if (me->query_prop(LIVE_I_STUNNED))
	return;

    /*
	This is a hook for NPC's so that they can do spells or any
	special actions when in combat. See /std/mobile.c
    */
    if (!i_am_real && me->special_attack(attack_ob))
	return;

    /*
	This is the hook for single special attacks, normally spells,
	that is done instead of the normal attacks, one turn.
    */
    if (objectp(ob = me->query_prop(LIVE_O_SPELL_ATTACK)))
    {
	me->remove_prop(LIVE_O_SPELL_ATTACK);
	ob->spell_attack(me, attack_ob);
	return;
    }

    if (me->query_prop(LIVE_I_CONCENTRATE))
	return;

    il = -1;
    size = sizeof(attacks);
    while(++il < size)
    {
    	/*
	 * Will we use this attack this round? (random(100) < %use)
	 */
	if (!intp(attacks[il][ATT_PROCU]))
	{
	    log_file("BAD_ATT", ctime(time()) + ": " + 
		file_name(me) + "\n");
	}
	if (random(100) >= attacks[il][ATT_PROCU])
	    continue;

	/*
	 * The attack has a chance of failing. If for example the attack
	 * comes from a wielded weapon, the weapon can force a fail or
	 * if the wchit is to low for this opponent.
	 */
	hitsuc = cb_try_hit(att_id[il]);
    	if (hitsuc <= 0)
	    continue;

	/*
	 * The intended victim can also force a fail. like in the weapon
	 * case, if fail, the cause must produce explanatory text himself.
	 */
	hitsuc = attack_ob->query_not_attack_me(me, att_id[il]);
	if (hitsuc > 0)
	    continue;

	hitresult = cb_tohit(att_id[il], attacks[il][ATT_M_HIT],
			     attacks[il][ATT_WEIGHT], attack_ob);
    
	defend_how = hitresult[1];
	hitsuc = hitresult[0];
	
	/* Istnieje zawsze szansa zadania kilku ciosow na raz. Wybieramy
	 * watrosc zmeczenia najbardziej meczacego z nich.
	 */
	extra_fatigue = max(extra_fatigue, attacks[il][ATT_FCOST]);

	/* Choose one damage type.
	 */
	dt = attacks[il][ATT_DAMT];
	dbits = ({ dt & W_IMPALE, dt & W_SLASH, dt & W_BLUDGEON }) - ({ 0 });
	dt = dbits[random(sizeof(dbits))];

	if (hitsuc > 0)
	{
	    pen = attacks[il][ATT_M_PEN];
	    if (sizeof(pen))
	    {
		tmp = MATH_FILE->quick_find_exp(dt);
		if ((tmp < sizeof(pen)))
		    pen = pen[tmp];
		else
		    pen = pen[0];
	    }
	    
	    if (!random(30000))
	    {
#if 0
		// Critical hit!
		if (random(10))
		{
		    pen = attacks[il][ATT_M_PEN];
		    if (sizeof(pen))
			pen = pen[0];
		    pen *= 5;
		}
		else
		    pen = attack_ob->query_hp() + me->query_hp();
		dt = -1;
#endif
		pen *= 6;

		SECURITY->log_syslog("CRITICAL",
		    sprintf("%s: %-11s on %-11s (pen = %4d)\n  %s on %s\n",
		    ctime(time()), me->query_real_name(),
		    attack_ob->query_real_name(), pen, file_name(me),
		    file_name(attack_ob)));
	    }

	    hitresult = (mixed*)attack_ob->hit_me(pen, dt, hitsuc, me,
						  att_id[il]);
	}
	else
	    hitresult = (mixed*)attack_ob->hit_me(-1, dt, hitsuc, me,
						  att_id[il]);

	/*
	 * Generate combat message, arguments Attack id, hitloc description
	 * proc_hurt, Defender
	 */
	if (hitsuc > 0)
	{
	    phit = attacks[il][ATT_M_PEN][tmp];
	    if (phit > 0)
		phit = 100 * hitresult[2] / phit;
	    else
		phit = 0;
	}
	if (hitresult[1])
	    cb_did_hit(att_id[il], hitresult[1], hitresult[0], attack_ob,
		       dt, phit, hitresult[3], hitsuc, defend_how,
		       hitresult[4]);
	else
	    break; /* Ghost, linkdeath, immortals etc */

	/*
	 * Ooops, Lifeform turned into a deadform. Reward the killer.
	 */
	if ((int)attack_ob->query_hp() <= 0)
	{
	    attack_ob->do_die(me);
	    enemies = enemies - ({ attack_ob });
	    break;
	}
    }

    cb_update_combat(extra_fatigue);
    
    if (attack_ob && !attack_ob->query_ghost())
    	return;
    else
    {
    	new = (all_inventory(environment(me)) & enemies) - ({ attack_ob });
    	if (sizeof(new))
    	{
	    attack_ob = new[0];
	    if (attack_ob)
		tell_object(me, "Koncentrujesz sie na walce z " +
		    attack_ob->query_imie(me, PL_NAR) +".\n");
	}
    	else
    	{
	    attack_ob = 0;
	    stop_heart();
	    return;
    	}
    }

    return;
}


/*
 * Function name: cb_hit_me
 * Description:   Called to decide damage for a certain hit on 'me'.
 * Arguments:	  wcpen:         ModifiedWeapon class penetration
 *                dt:            Damage type, MAGIC_DT if no ac helps
 *                hitsuc:        How well did we hit
 *                attacker: 
 *                attack_id:     -1 if a special attack
 *                target_hitloc: The hit location to damage.  If left
 *                               unspecified or an invalid hitloc is
 *                               given, a random hitlocation will be
 *                               used.
 * Returns:       Result of hit: ({ proc_hurt, hitloc description, phit, dam, armour })
 */
varargs public nomask mixed
cb_hit_me(int wcpen, int dt, int hitsuc, object attacker, int attack_id,
	  int target_hitloc = -1)
{
    object      *list, armour;
    int		proc_hurt, hp,
    		tmp, dam, phit,
    		hloc,
    		j, size;
    string      msg;
    mixed	ac;
    
    if (!objectp(me))
    {
	cb_link();
    }
    
    /*
     * You can not hurt the dead.
     */
    if (me->query_ghost())
    {
	tell_object(attacker, me->query_Imie(attacker, PL_MIA) +
	    " juz nie zyje, naprawde.\n");
	tell_roombb(environment(me), QCIMIE(attacker, PL_MIA) + 
	    " probuje zabic trupa.\n", ({attacker}));
	tell_object(me, attacker->query_Imie(me, PL_MIA) +
	    " bezskutecznie probuje cie zaatakowac.\n");
	me->stop_fight(attacker);
	attacker->stop_fight(me);
	return ({ 0, 0, 0, 0, 0 });
    }

    /*
     * Update the list of aggressors. If we hit ourselves: no update. 
     */
    cb_add_enemy(attacker);
    if (!attack_ob && attacker != me) 
	attack_ob = attacker;
    restart_heart();

    /*
     * Choose a hit location, and compute damage if wcpen > 0
     */
    if ((target_hitloc == -1) ||
	((hloc = member_array(target_hitloc, hit_id)) < 0))
    {
	tmp = random(100);
	j = 0;
	hloc = -1;
	size = sizeof(hitloc_ac);
	
	if (size == 0)
	{
	    attacker->catch_msg("Zglos blad w " + me->query_imie(attacker,
		PL_MIE) + "! Nie ma zdefiniowanych hitlokacji.\n");
	    log_file("BAD_HITLOC", me->query_real_name() + " (" + file_name(me) +
		     "): " + file_name(this_object()) + ", brak hitlokacji.\n");
 
   	    me->stop_fight(attacker);
   	    attacker->stop_fight(me);
	    return ({ 0, 0, 0, 0, 0 });
	}
	
	while(++hloc < size)
	{
	    j += hitloc_ac[hloc][HIT_PHIT];
	    if (j >= tmp)
		break;
	}

	if (hloc >= sizeof(hitloc_ac))
	{
	    hloc = sizeof(hitloc_ac) - 1;
	    log_file("BAD_HITLOC", me->query_real_name() + " (" + file_name(me) +
		     "): " + file_name(this_object()) + ", suma szans trafienia "+
		     "hitlokacji < 100\n");
	}
    }

    ac = hitloc_ac[hloc][HIT_M_AC];

    if (wcpen > 0)
    {
	if (dt == MAGIC_DT)
	    ac = 0;
	else
	{
	    tmp = (int)MATH_FILE->quick_find_exp(dt);

	    if (sizeof(ac) && (tmp < sizeof(ac)))
		ac = ac[tmp];
	    else if (sizeof(ac))
		ac = ac[0];
	    else if (!intp(ac))
		ac = 0;

	    /* Za kazde 100 hitsuc odejmujemy okolo 15% ac.
	     */
	    if (ac > 0)
	    {
		ac -= (ac * hitsuc / 670);
		ac = random(ac);
	    }
	}

	phit = wcpen / 2;
	phit = random(phit) + random(phit);

	dam = max(0, F_DAMAGE(phit, ac));
    }
    else
    {
	dam = 0;
	phit = (wcpen < 0 ? wcpen : -1);
    }

    hp = me->query_hp();

    /*
     * Wizards are immortal. (immorale ??)
     */
    if ((int)me->query_wiz_level() && dam >= hp)
    {
	tell_object(me, "Bycie czarodziejem uchrania cie przed strata zycia.\n");
	tell_room(environment(me),
		  QCIMIE(me, PL_MIA) + " jest niesmierteln" +
		  me->koncowka("y", "a") + ", przez co nie traci zycia.\n", 
		  ({me}));
	return ({ 0, 0, 0, 0, 0 });
    }
    
    /*
     * Ok, hurt me.
     */
    if (dam > 0 && hp)
    {
	proc_hurt = (100 * dam) / hp;
	if (dam && !proc_hurt)
	    proc_hurt = 1;     /* Less than 1% damage */
    }
    else if (dam > 0)
	proc_hurt = 100;
    else if (wcpen >= 0)
	proc_hurt = 0;
    else
	proc_hurt = -1;   /* Enemy missed */
    
    if (dam > 0)
	me->heal_hp(-dam);
    
    /*
     * Adjust our panic level
     */
    if (proc_hurt >= 0)
	cb_add_panic(2 + proc_hurt / 5);
    
    /*
     * Tell us where we were attacked and by which damagetype
     * proc_hurt == -1 -> przeciwnik spudlowal.
     */
    if (proc_hurt >= 0)
	armour = cb_got_hit(hit_id[hloc], proc_hurt, attacker, attack_id, dt, dam);

    /*
     * Reward attacker for hurting me
     */
    if (dam)
    {
#ifdef CB_HIT_REWARD
	cb_reward(attacker, dam, 0);
#endif
	if (random(dam) > random(me->query_stat(SS_DIS)))
	    me->cmdhooks_break_spell();
    }
    
    return ({ proc_hurt, hitloc_ac[hloc][HIT_DESC], phit, dam, armour });
}

/*
 * Function name: cb_attack
 * Description:   Called by the internal combat routines to attack.
 * Arguments:     victim: The object of the attack                
 */
public nomask void
cb_attack(object victim)
{
    if (!me)
	return;

    restart_heart();
    
    if (victim == me || victim == attack_ob || victim->query_ghost())
	return;

    me->reveal_me(1);
    victim->reveal_me(1);
    /*
     * Swap attack
     */
    cb_add_enemy(victim, 1);
    attack_ob = victim;

    victim->attacked_by(me);
}

/*
 * Function name:  cb_attacked_by
 * Description:    This routine is called when we are attacked or when 
 *                 someone we are hunting appears in our location.
 * Arguments:	   ob: The attacker
 */
public nomask void
cb_attacked_by(object ob)
{
    cb_add_enemy(ob);
    
    if (!attack_ob || (!query_ip_number(attack_ob) && query_ip_number(ob)))
	attack_ob = ob;
    
    restart_heart();

    if (me)
	me->cr_attacked_by(ob);
}

/*
 * Function name: cb_stop_fight
 * Description:   Stop fighting certain enemies
 */
public nomask void
cb_stop_fight(mixed elist)
{
    if (objectp(elist))
	elist = ({ elist });
    else if (!pointerp(elist))
	elist = ({});
    
    if (member_array(attack_ob, elist) >= 0)
	attack_ob = 0;
    
    if (pointerp(enemies) && pointerp(elist))
	enemies = enemies - (object *)elist;
    
    if (sizeof(elist = (enemies & all_inventory(environment(me)))))
	attack_ob = elist[0];
}

/*
 * Function name: cb_query_enemy
 * Description:   Gives our current enemy
 * Arguments:     arg: Enemy number, (-1 == all enemies)
 * Returns:       Object pointer to the enemy
 */
public nomask mixed
cb_query_enemy(int arg)
{
    enemies = enemies - ({ 0 });
    if (arg == -1)
	return enemies + ({});
    else if (arg < sizeof(enemies))
	return enemies[arg];
    else 
	return 0;
}

/*
 * Function name: cb_query_attack_ob
 * Description:   Gives the enemy we are fighting, if we are fighting...
 * Returns:       Object pointer to the enemy
 */
public nomask mixed
cb_query_attack() 
{ 
    return attack_ob; 
}

/*
 * Function name:  cb_heal
 * Description:    Heals the living object. Adds hp, mana and fatigue, panic
 * Arguments:	   delay: Number of heart_beats since last heal
 * Returns:        0 if we healed 'me'
 */
public nomask int
cb_heal(int delay)
{
    return 0;
}

/**********************************************************
 * 
 * Below is internal functions, only used by the inheritor of
 * this standard combat object.
 */

/*
 * Function name: add_attack
 * Description:   Add an attack to the attack array.
 * Arguments:	  
 *             wchit: Weapon class to hit
 *             wcpen: Weapon class penetration
 *	       dt:    Damage type
 *             %use:  Chance of use each turn
 *	       id:    Specific id, for humanoids W_NONE, W_RIGHT etc
 *	       skill: Optional skill with this attack
 *	       weight:Optional weight of this attack * strength of the user
 *	       fcost: Optional fatigue cost for performing this attack
 *
 * Returns:       True if added.
 */
static varargs int
add_attack(int wchit, mixed wcpen, int damtype, int prcuse, int id,
	   int skill, int weight, int fcost)
{
    int pos, *pen, *m_pen, m_hit, strength_mod;

    if (sizeof(attacks) >= MAX_ATTACK)
	return 0;

    pen = allocate(W_NO_DT);
    m_pen = allocate(W_NO_DT);

    if (skill == 0)
	skill = wchit;
    else if (skill < 1)
	skill = 0;
	
    m_hit = F_TOHITMOD(wchit, skill);
    strength_mod = F_STRENGTH_DAMAGE_MOD(me->query_stat(SS_STR));

/*
    if (skill < 0)
	skill = 0;
*/

    pos = -1;
    while(++pos < W_NO_DT)
    {
  	if (!pointerp(wcpen))
	{
	    m_pen[pos] = F_PENMOD(wcpen, skill);
	    pen[pos] = wcpen;
	}
	else if (pos >= sizeof(wcpen))
	{
	    m_pen[pos] = (pos ? m_pen[0] : 0);
	    pen[pos] = (pos ? pen[0] : 0);
	}
	else
	{
	    m_pen[pos] = F_PENMOD(wcpen[pos], skill);
	    pen[pos] = wcpen[pos];
	}
	
	m_pen[pos] += strength_mod;
    }

    if ((pos = member_array(id, att_id)) < 0)
    {
	att_id += ({ id });
	attacks += ({ ({ wchit, pen, damtype, prcuse, skill, m_hit, m_pen,
			 weight, fcost }) });
	return 1;
    }
    else
	attacks[pos] = ({ wchit, pen, damtype, prcuse, skill, m_hit, m_pen,
			  weight, fcost });
    
    return 1;
}

/*
 * Function name: remove_attack
 * Description:   Removes a specific attack
 * Arguments:     id: The attack id
 * Returns:       True if removed
 */
static int
remove_attack(int id) 
{
    int pos;
    
    if ((pos = member_array(id, att_id)) >= 0)
    {
	attacks = exclude_array(attacks, pos, pos);
	att_id = exclude_array(att_id, pos, pos);
	return 1;
    }
    return 0;
}

/*
 * Function name: query_attack_id
 * Description:   Give all attack id's
 * Returns:       Array with elements as described in add_attack
 */
public int *
query_attack_id() 
{ 
    return att_id + ({}); 
}

/*
 * Function name: query_attack
 * Description:   Give the attack for a certain id
 * Arguments:     id: The id to return attack array for
 * Returns:       Array with elements as described in add_attack
 */
public mixed *
query_attack(int id) 
{
    int pos;
    
    if ((pos = member_array(id, att_id)) >= 0)
    {
	return attacks[pos];
    }
    return 0;
}

/*
 * Function name: add_hitloc
 * Description:   Add a hitlocation to the hitloc array
 * Arguments:	  
 *	      ac:    The ac's for a given hitlocation, can be an int
 *	      %hit:  The chance that a hit will hit this location
 *	      desc:  String describing this hitlocation, ie "head", "tail"
 *	      id:    Specific id, for humanoids A_TORSO, A_HEAD etc
 *
 * Returns:       True if added.
 */
static int
add_hitloc(mixed ac, int prchit, string desc, int id)
{
    int pos, *act, *m_act;

    if (sizeof(hitloc_ac) >= MAX_HITLOC)
	return 0;
    
    act = allocate(W_NO_DT);
    m_act = allocate(W_NO_DT);

    pos = -1;
    while(++pos < W_NO_DT)
    {
	if (!pointerp(ac))
    	{
	    m_act[pos] = F_AC_MOD(ac);
	    act[pos] = ac;
	}
	else if (pos >= sizeof(ac))
	{
	    act[pos] = (pos ? act[0] : 0);
	    m_act[pos] = (pos ? F_AC_MOD(act[0]) : 0);
	}
	else
	{
	    m_act[pos] = F_AC_MOD(ac[pos]);
	    act[pos] = ac[pos];
	}
    }
    if ((pos = member_array(id, hit_id)) < 0)
    {
	hit_id += ({ id });
	hitloc_ac += ({ ({ act, prchit, desc, m_act }) });
    }
    else
	hitloc_ac[pos] = ({ act, prchit, desc, m_act });
    
    return 1;
}

/*
 * Function name: remove_hitloc
 * Description:   Removes a specific hit location
 * Arguments:     id: The hitloc id
 * Returns:       True if removed
 */
static int
remove_hitloc(int id) 
{
    int pos;
    
    if ((pos = member_array(id, hit_id)) >= 0)
    {
	hitloc_ac = exclude_array(hitloc_ac, pos, pos);
	hit_id = exclude_array(hit_id, pos, pos);
	return 1;
    }
    return 0;
}

/*
 * Nazwa funkcji : query_hitloc_id
 * Opis          : Zwraca tablice z identyfikatorami hitlokacji.
 * Funkcja zwraca: Tablica intow - patrz wyzej.
 */
public int *
query_hitloc_id() 
{ 
    return hit_id + ({}); 
}

/*
 * Nazwa funkcji : query_hitloc
 * Opis          : Zwraca informacje o konkretnej hitlokacji.
 * Argumenty     : int id - identyfikator hitlokacji
 * Funkcja zwraca: Tablice w postaci:
 *	({
 *	        mixed	ac   - ac danej lokacji, moze byc 3 elem. tablica 
 *				(na kazdy typ ataku),
 *		int	hit  - % szansa trafienia danej hitlokacji,
 *		string	desc - opis danej hitlokacji,
 *		mixed	m_ac - to samo co ac, tyle ze zawiera modified ac.
 *	})
 */
public nomask mixed *
query_hitloc(int id) 
{
    int pos;
    
    if ((pos = member_array(id, hit_id)) >= 0)
    {
	return hitloc_ac[pos];
    }
    return 0;
}

/*
 * Nazwa funkcji : 
 * Opis          : 
 * Argumenty     : 
 * Funkcja zwraca: 
 */
