/*
 *  /std/act/attack.c
 *
 *  Selective attacking: Standard action module for mobiles
 *
 *  set_aggressive(int)       1 - will make this mobile aggressive
 *  set_attack_chance(int)    % chance that this monster will attack a player
 *                            0 will have the same affect has 100 so no need
 *			      to set this as well
 *
 *  Currently you can make a monster aggressive and attack the entering player
 *  on sight or at a %chance. Significatively no npc's will be attacked.
 *  There is a lot of improvement that can be done.
 *
 *  Nick
 *
 *  Revision history:
 *  23-07-93 /Mercade : added VBFC to init_attack()
 *  29-01-99 Alvin : zmniejszona do minimum ilosc alarmow, kolejkowanie
 *		     nowych przybyszy, reagowanie na rozjasnienie pokoju,
 *		     zlikwidowanie ograniczenia reakcji tylko do NPCow.
 */

#pragma save_binary
#pragma strict_types

#include <pl.h>
#include <macros.h>
#include <stdproperties.h>
#include <formulas.h>

static	int	monster_attack_chance;   /* %attack chance */
public	int	attack_alarm;		 /* attack alarm number */
static	int	npcs_allowed;		 /* allowed attacking NPCs */
static	mixed	monster_aggressive;   	 /* aggressive  */
public	object	*to_attack;		 /* list of objects pending to attack */

/*
 * Nazwa funkcji : set_aggressive
 * Opis          : Sluzy do determinowania, czy npc ma byc agresywny.
 * Argumenty     : mixed: 1 - agresywny, 0 - nieagresywny; moze byc rowniez
 *			  podane wyraznie VBFC, z wlasnym kodem regulujacym
 *			  agresywnosc. Ewentualna funkcja VBFC bedzie zawsze
 *			  wywolywana z this_player()'em ustawionym na
 *			  osobe, ktorej zaatakowanie wlasnie rozpatrujemy.
 */
public void
set_aggressive(mixed i)
{
    monster_aggressive = i;
}

/*
 * Nazwa funkcji : set_attack_chance
 * Opis          : Ustawia szanse, z jaka npc bedzie atakowal przybyszy
 * Argumenty     : int - procentowa szansa zaatakowania; 0 == 100%
 */
public void
set_attack_chance(int i)
{
    monster_attack_chance = i;
}

/*
 * Nazwa funkcji : set_npc_react
 * Opis          : Umozliwia rozszerzenie reakcji rowniez na NPCe. W takiej
 *		   sytuacji raczej nalezy w set_aggressive() ustawic VBFC
 *		   na funkcje, ktora bedzie dopuszczac atakowanie tylko
 *		   wybranych NPCow.
 * Argumenty     : int: 1 - wlaczenie reakcji na NPCe, 0 - wylaczenie.
 */
public void
set_npc_react(int i)
{
    npcs_allowed = i;
}

object *
query_to_attack()
{
    if (!to_attack)
        return ({ });
    return to_attack + ({ });
}

/*
 * Nazwa funkcji : aggressive_attack
 * Opis          : Powoduje faktyczne zaatakowanie nowych przybyszy, zebranych
 *		   w kolejce.
 */
public void
aggressive_attack()
{
    int ix;
    object ob;
    
    attack_alarm = 0;

    if (!CAN_SEE_IN_ROOM(this_object()))
    {
	to_attack = 0;
	return ;
    }
    
    /* Powoduje, ze po zaatakowaniu przybyszy spowrotem przestawiamy sie
     * na starego przeciwnika.
     */
    if (ob = this_object()->query_attack())
	to_attack = ({ ob }) + (to_attack - ({ ob }));
    
    ix = sizeof(to_attack);
    
    while (--ix >= 0)
    {
        ob = to_attack[ix];
        
	if (!objectp(ob) || !present(ob, environment(this_object())) ||
	    !CAN_SEE(this_object(), ob) || !F_DARE_ATTACK(this_object(), ob))
	    continue;

	tell_room(environment(this_object()), QCIMIE(this_object(), PL_MIA) +
	    " atakuje " + QIMIE(ob, PL_BIE) + ".\n", ({ this_object(), ob }));
	tell_object(ob, this_object()->query_Imie(ob, PL_MIA) +
	    " atakuje cie!\n");
	this_object()->attack_object(ob);
    }
    
    to_attack = 0;
}

/*
 * Nazwa funkcji : init_attack
 * Opis          : Wywolywane przez init_living() w monster.c. Powoduje
 *		   dodanie przybysza do kolejki osob do zaatakowania, oraz
 *		   ustawienie krotkiego alarmu na zaatakowanie owej kolejki.
 */
public void
init_attack()
{
    if ((!npcs_allowed && this_player()->query_npc()) ||
	(!this_object()->check_call(monster_aggressive)))
	return;

    if ((monster_attack_chance == 0) || (random(100) < monster_attack_chance))
    {
	if (attack_alarm)
	{
	    if (pointerp(to_attack))
	    {
		if (member_array(this_player(), to_attack) == -1)
		    to_attack += ({ this_player() });
	    }
	    else
		to_attack = ({ this_player() });
	}
	else
	{
	    to_attack = ({ this_player() });
	    attack_alarm = set_alarm(2.0, 0.0, &aggressive_attack());
	}
    }
}

/*
 * Nazwa funkcji : notify_light_change
 * Opis	         : Wywolywana w momencie zmiany poziomu swiatla w pokoju.
 *		   Reaguje tylko na zmiane z ciemnosci na jasnosc
 *		   (wzgledem this_object()'u). Powoduje ewentualne
 *		   zaatakowanie intruzow.
 * Argumenty     : int - zmiana poziomu swiatla.
 */
void
notify_light_change(int l)
{
    int tmp;
    object *inv, old_tp;
    
    if (l <= 0)
	return;

    tmp = environment()->query_prop(OBJ_I_LIGHT) -
	this_object()->query_prop(LIVE_I_SEE_DARK);

    /* Test na to, czy pokoj z ciemnego dla nas zmienia sie w jasny. */
    if ((tmp - l > 0) || (tmp <= 0))
	return;

    if ((monster_attack_chance != 0) && (random(100) >= monster_attack_chance))
	return;

    old_tp = this_player();
    inv = all_inventory(environment()) - ({ this_object() });
    tmp = sizeof(inv);
    while (--tmp >= 0)
    {
	if (!living(inv[tmp]) ||
	    (!npcs_allowed && inv[tmp]->query_npc()))
	    continue;

	/* Grzecznie zapewniamy ewentualnemu wyrazeniu VBFC w
	 * monster_aggressive, ze this_player() bedzie ustawiony
	 * na aktualnie rozpatrywana osobe.
	 */
	set_this_player(inv[tmp]);
	if (!this_object()->check_call(monster_aggressive))
	    continue;

	if (attack_alarm)
	{
	    if (pointerp(to_attack))
	    {
		if (member_array(inv[tmp], to_attack) == -1)
		    to_attack += ({ inv[tmp] });
	    }
	    else
		to_attack = ({ inv[tmp] });
	}
	else
	{
	    to_attack = ({ inv[tmp] });
	    attack_alarm = set_alarm(2.0, 0.0, &aggressive_attack());
	}
    }

    set_this_player(old_tp);
}

/*
 * Nazwa funkcji : 
 * Opis          : 
 * Argumenty     : 
 * Funkcja zwraca: 
 */
