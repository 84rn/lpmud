/*
  /std/combat/chumanoid.c

  This is the externalized combat routines for humanoids. 

  This combat object predefines a set of attacks and hitlocations 
  for humanoid living objects. It also keeps track of the total percentage
  of attacks that can be made each turn and distributes those percentages
  over the attacks depending on effectiveness of weapon wielded.

  The distribution formula is:

          %use = %maxuse * (wchit*wcpen) / ( sum(wchit*wcpen) )

  This formula is used if the 'set_attuse()' function is called and the
  value set to something different than 0. Otherwise the %use is left
  unaffected.

  This object is cloned and linked to a specific individual when
  engaged in combat. The actual object resides in 'limbo'.
*/

#pragma save_binary
#pragma strict_types

inherit "/std/combat/ctool";

#include "/std/combat/combat.h"
#include <formulas.h>
#include <ss_types.h>
#include <std.h>
#include <wa_types.h>
#include <options.h>
#include <pl.h>
#include <math.h>

static  int             attuse;      /* Total %use, 100% is 1 attack */
static  int		al_mod_id;   /* Alarm na wywolanie cb_modify_procuse() */

/*
 * Function name: create_ctool
 * Description:   Reset the combat functions
 */
public nomask void
create_ctool()
{
    if (me)
	return;
    this_object()->create_chumanoid();
}

/*
 * Function name: cb_configure
 * Description:   Configure humanoid attacks and hitlocations.
 * Returns:       True if hit, otherwise 0.
 */
public void
cb_configure()
{
    object *obs;
    int il, size;

    ::cb_configure();

    me->add_subloc(SUBLOC_WIELD, this_object());
    me->add_subloc(SUBLOC_WORNA, this_object());

    obs = me->subinventory(SUBLOC_WORNA);
    il = -1;
    size = sizeof(obs);
    while(++il < size)
	obs[il]->move(me, 0);
    if (sizeof(obs))
	tell_object(me,"Oops! Chyba musisz jeszcze raz zalozyc wszystkie " +
	    "zbroje.\n");

    obs = me->subinventory(SUBLOC_WIELD);
    il = -1;
    size = sizeof(obs);
    while(++il < size)
	obs[il]->move(me, 0);
    if (sizeof(obs))
	tell_object(me, "Oops! Chyba musisz dobyc broni jeszcze raz.\n");

    add_attack(0, 0, 0, 0, W_RIGHT); me->cr_reset_attack(W_RIGHT);
    add_attack(0, 0, 0, 0, W_LEFT);  me->cr_reset_attack(W_LEFT);
    add_attack(0, 0, 0, 0, W_BOTH);  me->cr_reset_attack(W_BOTH);
    add_attack(0, 0, 0, 0, W_FOOTR); me->cr_reset_attack(W_FOOTR);
    add_attack(0, 0, 0, 0, W_FOOTL); me->cr_reset_attack(W_FOOTL);
    
    add_hitloc(0, 0, 0, A_HEAD);  me->cr_reset_hitloc(A_HEAD);
    add_hitloc(0, 0, 0, A_L_ARM); me->cr_reset_hitloc(A_L_ARM);
    add_hitloc(0, 0, 0, A_R_ARM); me->cr_reset_hitloc(A_R_ARM);
    add_hitloc(0, 0, 0, A_TORSO); me->cr_reset_hitloc(A_TORSO);
    add_hitloc(0, 0, 0, A_LEGS);  me->cr_reset_hitloc(A_LEGS);
}

/*
 * Description:
 *		show_subloc(string subloc, object on_obj, object for_obj)
 *			- Print a description of the sublocation 'subloc'
 *			  on object 'ob_obj' for object 'for_obj'.
 */
public string
show_subloc(string subloc, object on, object for_obj)
{
    if (subloc == SUBLOC_WIELD)
	return cb_show_wielded(for_obj);

    else if (subloc == SUBLOC_WORNA)
	return cb_show_worn(for_obj);

    else
	return "";
}

/*
 * Description: Humanoids might reallocate what attacks they use when the
 *              attacks are modified. (If maxuse is set)
 *              The distribution formula is:
 *
 *                       %use = %maxuse * (wchit*wcpen) / ( sum(wchit*wcpen) )
 */
public void
cb_modify_procuse()
{
    int il, *attid, *enabled_attacks, swc, puse, weapon_no;
    int unarmed_off;
    mixed *att;

    if (!attuse)
        return;

    attid = query_attack_id();
    att = allocate(sizeof(attid));
    enabled_attacks = allocate(sizeof(attid));
    weapon_no = sizeof(cb_query_weapon(-1));
    //unarmed_off = me->query_option(OPT_UNARMED_OFF);

    for (swc = 0, il = 0; il < sizeof(attid); il++)
    {
        att[il] = query_attack(attid[il]);
 
        /* test to see if this attack is enabled */
        if (/*!unarmed_off || */(weapon_no < 1) || cb_query_weapon(attid[il]))
        {
            enabled_attacks[il] = 1;
            swc += att[il][ATT_WCHIT] * att[il][ATT_M_HIT];
        }
    }

    for (il = 0; il < sizeof(attid); il++)
    {
        if (swc && enabled_attacks[il])
        {
            puse = (attuse * att[il][ATT_WCHIT] * att[il][ATT_M_HIT]) / swc;
        }
        else
        {
            puse = 0;
        }

        ::add_attack(att[il][ATT_WCHIT], att[il][ATT_WCPEN], 
                     att[il][ATT_DAMT], puse, attid[il],
                     (att[il][ATT_SKILL] ? att[il][ATT_SKILL] : -1),
                     att[il][ATT_WEIGHT], att[il][ATT_FCOST] );
    }
}

/*
 * Description: Set the %attacks used each turn. 100% is one attack / turn
 * Arguments:   sumproc: %attack used
 */
public void
cb_set_attackuse(int sumproc)
{
    attuse = sumproc;
    cb_modify_procuse();
}

/*
 * Description: Query the total %attacks used each turn. 100% is one attack / turn
 * Returns:     The attackuse
 */
public int
cb_query_attackuse() { return attuse; }

/*
 * Description: Add an attack, see /std/combat/cbase.c
 */
static varargs int
add_attack(int wchit, mixed wcpen, int damtype, int prcuse, int id,
	   int skill, int weight, int fcost)
{
    int ret;

    ret = ::add_attack(wchit, wcpen, damtype, prcuse, id, skill, weight,
		       fcost);
    
    if (!get_alarm(al_mod_id))
        al_mod_id = set_alarm(1.0, 0.0, &cb_modify_procuse());
    
    return ret;
}

/*
 * Function name: cb_wield_weapon
 * Description:   Wield a weapon.
 * Arguments:	  wep - The weapon to wield.
 * Returns:       True if wielded.
 */
public mixed
cb_wield_weapon(object wep)
{
    int aid, wcskill, owchit, owcpen;
    mixed *att;
    string str;

    if (function_exists("create_object", wep) != WEAPON_OBJECT)
    {
	return capitalize(wep->short(PL_MIA)) + " nie " +
	     (wep->query_tylko_mn() ? "sa" : "jest") + " prawdziwa bronia.\n";
    }

    if (stringp(str = ::cb_wield_weapon(wep)))
    {
	return str;
    }

    aid = (int) wep->query_attack_id();
    if (cb_query_weapon(aid) == wep)
    {
	att = query_attack(aid);
    	/*
         * We get no more use of the weapon than our skill with it allows.
	 */
	wcskill = (int)me->query_skill(SS_WEP_FIRST + 
				       ((int)wep->query_wt() - W_FIRST));
	if (wcskill < 1)
	    wcskill = -1;
	add_attack(att[ATT_WCHIT], att[ATT_WCPEN], att[ATT_DAMT],
	    att[ATT_PROCU], aid, wcskill, att[ATT_WEIGHT], att[ATT_FCOST]);
	
	if (aid == W_BOTH)
	{
	    add_attack(0, 0, W_BLUDGEON, 0, W_LEFT, -1);
	    add_attack(0, 0, W_BLUDGEON, 0, W_RIGHT, -1);
	}
    }
    
    return 1;
}

public void
cb_unwield(object wep)
{
    int aid;
    
    ::cb_unwield(wep);
    
    aid = wep->query_attack_id();
    
    if (aid == W_BOTH)
    {
        me->cr_reset_attack(W_LEFT);
        me->cr_reset_attack(W_RIGHT);
    }
    
    cb_modify_procuse();
    
    return ;
}

/*
 * Function name: cb_wear_arm
 * Description:   Wear an armour
 * Arguments:	  arm - The armour.
 * Returns:       True if worn, errtext if fail
 */
public mixed
cb_wear_arm(object arm)
{
    if (function_exists("create_object", arm) != ARMOUR_OBJECT)
    {
	return capitalize(arm->short(PL_MIA)) + " nie " +
	     (arm->query_tylko_mn() ? "sa" : "jest") + " prawdziwa zbroja.\n";
    }

    return ::cb_wear_arm(arm);
}
