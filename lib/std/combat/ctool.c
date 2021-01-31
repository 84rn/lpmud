/*
  /std/combat/ctool.c

  This is the externalized combat routines for monsters and other mobiles
  that are able to use tools for aid in combat. Typical tools are weapons
  and armours.

  This object is cloned and linked to a specific individual when
  engaged in combat. The actual object resides in 'limbo'.
*/

#pragma save_binary
#pragma strict_types

inherit "/std/combat/cplain";

#include <wa_types.h>
#include <composite.h>
#include <stdproperties.h>
#include <ss_types.h>
#include <macros.h>
#include <filter_funs.h>
#include <formulas.h>
#include <math.h>
#include <debug.h>

/* Prototypes */
public void cb_remove_arm(object wep);
static void adjust_ac(int hid, object arm, int rm);
public mixed cb_query_armour(int which);

static mapping aid_attack = ([]),  /* The 'weapon' aiding the attacks */ 
               aid_hitloc = ([]),  /* The 'armours' protecting the hitlocs */
               tool_slots = ([]);  /* The object occupying a certain slot' */ 
               
static mixed   *aid_defense = ({}); /* Tarcze i bronie do parowania - dane */
static object  *aid_defense_id = ({}); /* jw, wskazniki do obiektow */

/* 
   NOTE
 
     There is a limited number of tool slots and one slot can only be
     occupied by one object at a time. One object may occupy many slots though.
    
     Armours can protect one or more hit locations. What hit locations
     a given armour protects is given by the function 'query_protects' in
     the armour. 

     Weapons can only aid one attack. The attack id is given by the
     function 'query_attack_id' in the weapon.

     The tool slots are used to ensure that the use of a weapon and an armour
     do not conflict. The slots that a weapon or armour occupies are given
     by the function 'query_slots' in weapons and armours.

     Observe that what attacks and hitlocation a weapon and armour is defined
     to aid is independant of what slots it allocates.

     The combat system makes no checks on that relevant tool slots aids
     relevant attacks and hit locations. This is taken care of in
     /std/armour.c and /std/weapon.c

     Tool slots are made as defined by the objects. The only thing that the
     combat system does is to ensure that two tools do not use the same 
     slot.

   MAGICAL armours

     Magical armours can protect one or more hitlocations without allocating
     a tool slot. If the property OBJ_I_IS_MAGIC_ARMOUR is defined the armour
     is considered to be a magic one. The magic armour can of course allocate
     any number of tool slots, just like a normal armour.

   MAGICAL weapons

     Magical weapons work just like normal weapons. A magical 'weapon' that
     allocates no combat slot is not a 'weapon' it is an independant magic
     object and must cause damage onto the enemy on its own. Such magic
     attacks are not supported in the combat system. 

*/

/*
 * Function name: create_cplain
 * Description:   Reset the combat functions
 */
public nomask void
create_cplain()
{
    if (me)
        return;
    this_object()->create_ctool();
}

/*
 * Function name: cb_configure
 * Description:   Configure humanoid attacks and hitlocations.
 * Returns:       True if hit, otherwise 0.
 */
public void
cb_configure()
{
    ::cb_configure();

    qme()->add_subloc(SUBLOC_WIELD);
    qme()->add_subloc(SUBLOC_WORNA);
}

/*
 * Description: Give status information about the combat values
 * 	More info.
 */
public string
cb_status()
{
    string str;
    int size;
    
    size = sizeof(aid_defense_id);
    
    if (!size)
        return ::cb_status();
    
    str = sprintf("\n%-25s %17s %5s\n", "  Krotki opis", 
        "Bonus do parowania", "%use");
    while (--size >= 0)
    {
        str += sprintf("%-30s    %-5d        %-4d\n", 
            aid_defense_id[size]->short(),
            aid_defense[size][0], aid_defense[size][1]);
    }
    
    return ::cb_status() + str;
}

public void
cb_modify_def_procuse()
{
    int sum, size, x;
    size = sizeof(aid_defense);
    
    x = -1;
    
    while(++x < size)
        sum += aid_defense[x][0];
     
    x = -1;
    
    while(++x < size)
        aid_defense[x][1] = (aid_defense[x][0] * 100 / sum);
        
    return ;
}

/*
 * Function name: cb_wield_weapon
 * Description:   Wield a weapon. Here 'weapon' can be any object.
 * Arguments:     wep - The weapon to wield.
 * Returns:       True if wielded.
 */
public mixed
cb_wield_weapon(object wep)
{
    int il, aid, *slots, extra, size, weight, max_weight, str, fcost;
    object *obs;

    aid = (int) wep->query_attack_id();
    slots = (int*) wep->query_slots();
//    prot = (int *) wep->query_protects();

    /* Can we use this weapon ?
     */
    if (!query_attack(aid))
        return "Hmm, ta bron chyba zostala stworzona dla jakiejs innej " +
            "rasy - nie wyobrazasz sobie jak mogl" + qme()->koncowka("bys",
            "abys") + " z niej korzystac.\n";

    /*
     * Are all the slots it needs free.
     */
    il = -1;
    size = sizeof(slots);
    while(++il < size)
        if (tool_slots[slots[il]])
            return "Nie masz wolnych rak.\n";

//    if (sizeof(prot) && (sizeof(query_hitloc_id() & prot) < sizeof(prot)))
//        return "It seems not fit for you to wield.";

    if (wep->move(qme(), SUBLOC_WIELD))
        return "Z jakiegos dziwnego powodu nie mozesz dobyc tej broni. " +
            "Najlepiej zglos w niej blad.\n";

    il = -1;
    /* size is still set from the last assignment. */
    while(++il < size)
        tool_slots[slots[il]] = wep;
    
    aid_attack[aid] = wep; 
    weight = wep->query_prop(OBJ_I_WEIGHT);
    str = qme()->query_stat(SS_STR);
    
    if ((max_weight = F_MAX_WEP_WEIGHT(str)) < weight)
    {
	/* Nie oplaca sie walczyc za ciezka dla nas bronia. Za kazdy
	 * nadmiarowy kilogram wagi broni placimy 1 punkt zmeczenia.
	 */
        fcost = (weight - max_weight) / 1000 + 1;
        
        if (!interactive(qme()))
        log_file("TOO_HEAVY_WEP", 
            sprintf("%-12s fcost %d, str %d, maxw=%d <-> w=%d %s\n",
	    (interactive(qme()) ? capitalize(qme()->query_real_name()) : 
	    file_name(qme())), fcost, str, max_weight, weight, file_name(wep)));
    }
    
    add_attack(wep->query_hit(), wep->query_modified_pen(), wep->query_dt(),
               wep->query_procuse(), aid, 0, weight, fcost);

#if 0  
// \- Wywalone query_protects() z broni. Bronie nie dodaja juz ac.
    il = -1;
    size = sizeof(prot);
    while(++il < size)
    {
        /* Add this weapon to the ones protecting this hitlocation
        */
        obs = aid_hitloc[prot[il]];
        if (pointerp(obs))
            obs = obs + ({ wep });
        else
            obs = ({ wep });

        aid_hitloc[prot[il]] = obs;   
        adjust_ac(prot[il], wep, 0);  /* Fix one hitlocs acc ac */
    }
#endif

    /*
     * If more than two weapons wielded check the 2H combat, only with 2H
     * skill > 20 will it be profitable to wield 2 weapons.
     * Attackuse range from 80 to 150
     */
    if (sizeof(cb_query_weapon(-1)) > 1)
    {
        extra = qme()->query_skill(SS_2H_COMBAT);
        extra = extra > 20 ? extra / 2 : extra - 20;
        this_object()->cb_set_attackuse(100 + extra);
    }

    extra = F_PARRYMOD(wep->query_hit(), 
                       qme()->query_skill(SS_PARRY));

    il = qme()->query_stat(SS_STR) * 150;
    if (wep->query_hands() != W_BOTH)
        il = il * 6 / 10;

    if ((size = wep->query_prop(OBJ_I_WEIGHT)) > il)
        extra = (100 - (165 * (size - il) / il)) * extra / 100;

    if (extra > 0)
    {
        aid_defense_id += ({ wep });
        aid_defense += ({ ({ extra, 0, weight * str }) });
        cb_modify_def_procuse();
    }

    return 1;
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
    mixed *a;
    int il, size;
    string str;

    a = m_values(aid_attack) - ({ 0 });

    if (!sizeof(a))
        return "";
    
    if (ob != qme())
        str = "Trzyma ";
    else
        str = "Trzymasz ";

    il = -1;
    size = sizeof(a);
    while(++il < size) 
    {
        if (objectp(a[il]))
            a[il] = a[il]->query_wield_desc();
        else
            a[il] = 0;
    }

    a = a - ({ 0 });                                       

    if (sizeof(a) < 2)
        str += a[0];
    else
        str += implode(a[0 .. sizeof(a) - 2], ", ") + " oraz " + a[sizeof(a) - 1];

    return str + ".\n";
}

/*
 * Function name: cb_unwield
 * Description:   Unwield a weapon.
 * Arguments:     wep - The weapon to unwield.
 * Returns:       None.
 */
public void
cb_unwield(object wep)
{
    int aid;

    aid = (int)wep->query_attack_id();

    /* Are we using it?
     */
    if (aid_attack[aid] != wep)
        return;

    /* Take it away from the wield subloc
    */
    if (environment(wep) == qme())
        wep->move(qme());

    aid_attack[aid] = 0;
    cb_remove_arm(wep); 
    
    me->cr_reset_attack(aid);

   /* If we wield no more than 1 weapon our % use should go back to 100 */

    if (sizeof(cb_query_weapon(-1)) < 2)
        this_object()->cb_set_attackuse(100);
        
    if ((aid = member_array(wep, aid_defense_id)) >= 0)
    {
        aid_defense_id = exclude_array(aid_defense_id, aid, aid);
        aid_defense = exclude_array(aid_defense, aid, aid);
    }
}

static int
empty_slot(object ob, object curw)
{
    return (ob != curw);
}

static int
weapons_out(object ob)
{
    return (ob && ob->check_armour());
}

/*
 * Function name: cb_query_weapon
 * Description  : Returns the weapon held in a specified location or all the
 *                weapons the living wields when -1 is given as argument.
 * Arguments    : int which - a numeric label describing a weapon location.
 *                            On humanoids this is W_RIGHT etc. Give -1 to
 *                            list all weapons.
 * Returns      : object   - the corresponding weapon or 0.
 *                object * - all weapons in an array for which == -1.
 */
public mixed
cb_query_weapon(int which)
{
    if (which >= 0)
    {
        return aid_attack[which];
    }
    else
    {
        return m_values(aid_attack) - ({ 0 });
    }
}

/*
 * Nazwa funkcji : cb_query_slot
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
cb_query_slot(int slot_num)
{
    if (slot_num >= 0)
    {
        return tool_slots[slot_num];
    }
    else
    {
        return m_indices(tool_slots);
    }
}

public string
zaloz_tarcze(object arm, int bonus)
{
    int udzwig, waga, minus;

    udzwig = qme()->query_stat(SS_STR) * 85;

//qme()->catch_msg("Waga[" + udzwig + "/" + arm->query_prop(OBJ_I_WEIGHT) +
//       "], Bonus[" + bonus + "/");

    if ((waga = arm->query_prop(OBJ_I_WEIGHT)) > udzwig)
    {
        minus = (100 - (135 * (waga - udzwig) / udzwig));
        minus = max(40, minus);
        bonus = bonus * minus / 100;
    }

//qme()->catch_msg(bonus + "], zabrany %[" + (100 - minus) + "]\n");

    if (bonus > 0)
    {
        aid_defense_id += ({ arm });
        aid_defense += ({ ({ bonus, 0, waga * qme()->query_stat(SS_STR) }) });
        cb_modify_def_procuse();
    }
    
    return 0;
}


/*
 * Function name: adjust_ac
 * Description:   Adjust relevant hitlocations for a given armour slot
 *                when we wear an armour or remove an armour.
 * Arguments:     hid:   The hitlocation id as given in /sys/wa_types.h
 *                arm:   The armour.
 *                rm:    True if we remove armour
 */
static void
adjust_ac(int hid, object arm, int rm)
{
    int il, *ac;
    mixed *oldloc;

    ac = (int *)arm->query_ac(hid);
    oldloc = query_hitloc(hid);

    for (il = 0; il < W_NO_DT; /*il < sizeof(am) && il < sizeof(oldloc[0]) */ il++)
    {
        if (rm)
            oldloc[0][il] -= ac[il];
        else
            oldloc[0][il] += ac[il];
    }
    add_hitloc(oldloc[0], oldloc[1], oldloc[2], hid);
}

/*
 * Function name: cb_wear_arm
 * Description:   Wear an armour
 * Arguments:     arm - The armour.
 * Returns:       True if worn, text if fail
 */
public mixed
cb_wear_arm(object arm)
{
    int *hid, il, *slots, size, extra, bonus;
    object *obs, tmp;
    string str;
/*
    if (arm->query_prop(OBJ_I_IS_MAGIC_ARMOUR))
        hid = arm->query_shield_slots();
    else
*/
    hid = arm->query_protects();   /* The hitlocations */
    slots = arm->query_slots();  /* The needed tool slot */
    hid &= query_hitloc_id();
    
#if 0
/* Warunek wykasowany. Od teraz zbroje moga deklarowac ac na toolsloty
 * nie nalezace do hiutlokacji humanoidow. Po prostu nie beda uwzgledniane.
 * Potrzebna jest dlatego operacja powyzej. /Alvin
 */

    /* Can we use this armour ? We must define all the hitlocs it protects
     */
    if (sizeof(hid) && (sizeof(query_hitloc_id() & hid) < sizeof(hid)))
        return capitalize(arm->short(me, PL_MIA)) + " zupelnie na ciebie" +
            " nie pasuj" + (arm->query_tylko_mn() ? "a" : "e") + ".\n";
#endif

    /*
     * Are all the slots it needs free ?
     */
    il = -1;
    size = sizeof(slots);
    while(++il < size)
        if (tool_slots[slots[il]])
            return "Jakas inna zbroja przeszkadza ci.";

    if ((bonus = arm->query_parry_bonus()) &&
        (str = zaloz_tarcze(arm, F_SHIELD_PARRYMOD(bonus, 
		qme()->query_skill(SS_SHIELD_PARRY)))))
        return str;

    if (arm->move(qme(), SUBLOC_WORNA))
        return "Z jakiegos dziwnego powodu nie mozesz zalozyc " +
	    arm->short(me, PL_DOP) + ". Najlepiej zglos w " +
	    arm->koncowka("nim", "niej") + " blad.\n";

    il = -1;
    /* No need to set 'size' since that is still set from last time. */
    while(++il < size)
        tool_slots[slots[il]] = arm;

    il = -1;
    size = sizeof(hid);
    while(++il < size)
    {
        /* Add this armour to the ones protecting this hitlocation
        */
        obs = aid_hitloc[hid[il]];
        if (pointerp(obs))
            obs = obs + ({ arm });
        else
            obs = ({ arm });

        aid_hitloc[hid[il]] = obs;   
        adjust_ac(hid[il], arm, 0);  /* Fix one hitlocs acc ac */
    }
    return 1;
}

/*
 * Function name: cb_show_worn
 * Description:   Describe the currently worn armours
 * Arguments:     ob: The object that the description is for
 * Returns:       Description string.
 */
public string
cb_show_worn(object ob)
{
    mixed *a, *b, *c;
    string str;
    int il, size;

    b = m_values(aid_hitloc); /* All protecting objects */
    c = ({ });
    size = sizeof(b);
    il = -1;
    while(++il < size)
        c += pointerp(b[il]) ? b[il] : ({});

    c += m_values(tool_slots); /* All objects used */

// Mozna by to chyba nieco uproscic. Do aid_hitloc dodaje niepowtarzajace
// sie tool_slots'y. Czy taki manewr jest potrzebny ?  /Alvin

    a = ({});
    while(sizeof(c))
    {
        a += ({ c[0] });
        c -= ({ c[0] });
    }

    a = filter(a, weapons_out);
    a = FILTER_SHOWN(a);

    if (!sizeof(a))
        return ""; 

    if (ob != qme())
        str = "Ma na sobie ";
    else
        str = "Masz na sobie ";

    str += COMPOSITE_DEAD(a, PL_BIE);

    return str + ".\n";
}

/*
 * Function name: cb_remove_arm
 * Description:   Remove an armour
 * Arguments:     arm - The armour.
 */
public void
cb_remove_arm(object arm) 
{
    int il, pos, *hids, size;
    mixed *a, *b;

    /* Are we wearing it ?
     */
    if (member_array(arm, m_values(tool_slots)) < 0)
    {
        a = m_values(aid_hitloc); /* All protecting objects */
        b = ({ });
        il = -1;
        size = sizeof(a);
        while(++il < size)
            b += pointerp(a[il]) ? a[il] : ({});
        if (member_array(arm, b) < 0)
            return 0;
    }

    if (environment(arm) == qme())
        arm->move(qme());
    
    tool_slots = filter(tool_slots, &empty_slot(, arm));

    hids = query_hitloc_id();

    /*
     * Remove the armours effect for each hit location it protects
     */
    il = -1;
    size = sizeof(hids);
    while(++il < size)
    {
        if (pointerp(aid_hitloc[hids[il]]) &&
            (pos = member_array(arm, aid_hitloc[hids[il]])) >= 0)
        {
            adjust_ac(hids[il], arm, 1);
            if (!sizeof(aid_hitloc[hids[il]] = 
                exclude_array(aid_hitloc[hids[il]], pos, pos)))
            {
                qme()->cr_reset_hitloc(hids[il]);
            }
        }
    }
    if ((pos = member_array(arm, aid_defense_id)) >= 0)
    {
        aid_defense_id = exclude_array(aid_defense_id, pos, pos);
        aid_defense = exclude_array(aid_defense, pos, pos);
    }

}

/*
 * Function name: cb_query_armour
 * Description  : Returns the armour of a given position or lists all armours
 *                worn when -1 is given as argument.
 * Arguments    : int which - a numeric label describing an armour location.
 *                            On humanoids this is TS_HEAD etc. Give -1 to
 *                            list all.
 * Returns      : object   - the corresponding armour or 0.
 *                object * - all armours when -1 is given.
 * BUGS/features:
 *         If an object can be both weapon and armour it will not show up
 *         here as an armour if wielded too.
 */
public mixed
cb_query_armour(int which)
{
    mixed *a, *b;
    int il, size;

    if (which >=0)
    {
        return tool_slots[which];
    }

    a = m_values(aid_hitloc);
    b = ({ });
    il = -1;
    size = sizeof(a);
    while(++il < size)
        b += pointerp(a[il]) ? a[il] : ({});
    b += m_values(tool_slots);

    a = ({});
    while(sizeof(b))
    {
        a += ({ b[0] });
        b -= ({ b[0] });
    }

    return a - m_values(aid_attack) - ({0});
}

/*
 * Function name: cb_attack_desc
 * Description:   Gives the description of a certain attack slot.
 * Arguments:     aid:   The attack id
 * Returns:       string holding description on VBFC form. Do not use write()
 */
public string
cb_attack_desc(int aid)
{
    if (objectp(aid_attack[aid]))
    {
        return QSHORT(aid_attack[aid], PL_NAR);
    }
    else
    {
        return qme()->cr_attack_desc(aid);
    }
}

/*
 * Function name: cb_try_hit
 * Description:   Decide if a certain attack fails because of something
 *                related to the attack itself, ie specific weapon that only
 *                works some of the time. 
 * Arguments:     aid:   The attack id
 * Returns:       True if hit, otherwise 0.
 */
public int
cb_try_hit(int aid)
{
    if (objectp(aid_attack[aid]))
    {
        return aid_attack[aid]->try_hit(cb_query_attack());
    }
    else
    {
        return (int)qme()->cr_try_hit(aid);
    }
}

/*
 * Function name: cb_did_hit
 * Description:   Tells us that we hit something. Should produce combat
 *                messages to all relevant parties. 
 * Arguments:     aid:    The attack id
 *                hdesc:  The hitlocation description.
 *                phurt:  The %hurt made on the enemy
 *                enemy:  The enemy who got hit
 *                dt:     The current damagetype
 *                phit:   The %success that we made with our weapon
 *                dam:    The damamge made in hit points
 *		  tohit:  How well did we hit
 *		  def_ob: Obj that defended or how we defended (if miss)
 *		  armour: Armour on the hit hitlocation
 */
public varargs void
cb_did_hit(int aid, string hdesc, int phurt, object enemy, int dt,
           int phit, int dam, int tohit, mixed def_ob, object armour)
{
    if ((!enemy) || (!qme()))
        return;

    if (objectp(aid_attack[aid]) && 
        aid_attack[aid]->did_hit(aid, hdesc, phurt, enemy, dt, phit, dam, 
                tohit, def_ob, armour))
    {
        /*
         * Adjust our panic level
         */
        if (phurt >= 0)
            cb_add_panic(-3 - phurt / 5);
        else
            cb_add_panic(1);
        return;
    }
    else
        ::cb_did_hit(aid, hdesc, phurt, enemy, dt, phit, dam, tohit, def_ob,
            armour);
}

/*
 * Function name: cb_got_hit
 * Description:   Tells us that we got hit. It can be used to reduce the ac
 *                for a given hitlocation for each hit.
 * Arguments:     hid:   The hitloc id
 *                ph:    The %hurt
 *                att:   Attacker
 *                aid:   The attack id
 *                dt:    The damagetype
 *                dam:   The damage done in hitpoints
 * Returns: The armour that parried the attack.
 */
public varargs object
cb_got_hit(int hid, int ph, object att, int aid, int dt, int dam)
{
    int il, size, sum, ran, tmp;
    object *arms, arm;

    /* 
     * Many armours may help to cover the specific bodypart: hid
     */
    arms = aid_hitloc[hid];
    
    if (pointerp(arms))
    {
        sum = query_hitloc(hid)[0][QUICK_FIND_EXP(dt)] - 1;
        ran = random(sum);
        il = -1;
        size = sizeof(arms);
        while(++il < size)
        {
            arms[il]->got_hit(hid, ph, att, aid, dt, dam);
            if (!(dam || arm))
            {
		sum -= arms[il]->query_ac(hid, dt);
		if ((sum <= ran) && 
		    (!arms[il]->query_prop(ARMOUR_I_MAGIC_PARRY)))
		    arm = arms[il];
	    }
        }
    }

    qme()->cr_got_hit(hid, ph, att, aid, dt, dam);
    return arm /*tool_slots[hid]*/;
}

/*
 * Function namn: cb_update_armour
 * Description:   Call this function if the ac of a shielding object has changed
 * Arguments:     obj - the object which ac has changed
 */
public void
cb_update_armour(object obj)
{
    if (!obj)
        return ;

    cb_remove_arm(obj);
    cb_wear_arm(obj);

#if 0
    int *index, i, j, size, size2;
    object *arms;
    mixed *oldloc;

    if (!obj)
        return;

    index = m_indexes(aid_hitloc);
    i = -1;
    size = sizeof(index);
    while(++i < size)
    {
        arms = aid_hitloc[index[i]];
        if (pointerp(arms) && obj && (member_array(obj, arms) > - 1))
        {
            me->cr_reset_hitloc(index[i]);
            j = -1;
            size2 = sizeof(arms);
            while(++j < size2)
                adjust_ac(index[i], arms[j], 0);
        }
    }
#endif
}
            
/*
 * Function namn: cb_update_weapon
 * Description:   Call this function when something has caused the weapon
 *                stats to change, skill raise or sharpening the weapon or so.
 * Arguments:     wep - The weapon
 */
public void
cb_update_weapon(object wep)
{
    if (!wep)
        return;
        
    cb_unwield(wep);
    cb_wield_weapon(wep);
    
#if 0
    add_attack(wep->query_hit(), wep->query_modified_pen(), wep->query_dt(),
                wep->query_procuse(), wep->query_attack_id(),
                me->query_skill(wep->query_wt()), 
                wep->query_prop(OBJ_I_WEIGHT) * qme()->query_stat(SS_STR));

    extra = F_PARRYMOD(wep->query_hit(), 
                       qme()->query_skill(SS_PARRY));
    
    il = qme()->query_stat(SS_STR) * 150;
    if (wep->query_hands() != W_BOTH)
        il = il * 6 / 10;
        
//set_this_player(find_player("alvin"));
//    write("Waga[" + il + "/" + wep->query_prop(OBJ_I_WEIGHT) +
//        "], Bonus[" + extra + "/");
        
    if ((size = wep->query_prop(OBJ_I_WEIGHT)) > il)
        extra = (100 - (165 * (size - il) / il)) * extra / 100;
        
//    write(extra + "], zabrany %[" + (175 * (size - il) / il) + "]\n");
    
    size = member_array(wep, aid_defense_id);
    if (extra > 0)
    {
        if (size == -1)
        {
            size = sizeof(aid_defense_id);
            aid_defense_id += 0;
            aid_defense += 0;
        }
        
        aid_defense_id[size] = wep;
        aid_defense[size] = ({ extra, 0, weight });
    }
    else
    {
        
    }
        cb_modify_def_procuse();


    cb_update_armour(wep);
#endif
}

public mixed
cb_query_defense(int weight)
{
    int size, x, rnd, bonus;
     
    size = sizeof(aid_defense_id);
    rnd = random(100); x = 0;
    while (--size >= 0)
    {
        x += aid_defense[size][1];
        if (rnd < x)
        {
            bonus = aid_defense[size][0];
            
            if (weight > aid_defense[size][2])
            {
		/* Jesli 'ped' (sila * masa) broni atakujacej jest
		 * wiekszy od broniacej, broniacy ma minusy do parowania.
		 */
                bonus -= (((2 * bonus * weight + 30000) / 
                           (weight + aid_defense[size][2] + 30000)) -
		          bonus);

                if (bonus <= 0)
                    return 0;
            }
            else
            /* Przeciwnik atakuje unarmed, my parujemy bronia. Odwracamy
             * wartosc, zeby dac znac, zeby nie pisalo ze parowalismy
             * bronia - bo to glupiutko wyglada.
             */
	    if (!weight && !aid_defense_id[size]->check_armour())
	        bonus = -bonus;

            return ({ bonus, aid_defense_id[size] });
        }
    }
    
    return 0;
}

/*
 * Nazwa funkcji : 
 * Opis          : 
 * Argumenty     : 
 * Funkcja zwraca: 
 */
