/*
 * /std/player/pcombat.c
 *
 * This is a subpart of player_sec.c
 *
 * All combat routines that are player specific are handled here. Interactive
 * players are not allowed to be in the same team with NPC's.
 */

/*
 * Global variables that are not saved.
 */
private static object   *team_invited;	/* Array of players invited to team */

/* 
 * Prototype.
 */
static nomask void linkdeath_remove_enemy(object enemy);

/*
 * Function name:   team_invite
 * Description:     Invites a new member to my team. This does NOT join the
 *                  member to my team. It only makes it possible for the
 *                  player to join my team.
 * Arguments:	    member: The objectpointer to the invited member.
 */
public void
team_invite(object member)
{
    if (!member)
    {
	return;
    }

    if (member_array(member, team_invited) >= 0)
	return;

    if (!team_invited)
	team_invited = ({ member });
    else
	team_invited = team_invited + ({ member });
	
    team_invited -= ({ 0 });
}

/*
 * Function name:   query_invited
 * Description:     Give back an array with objects of players who are
 *                  invited.
 * Returns:         An array of objects
 */
public object *
query_invited()
{
    if (!team_invited)
        return ({ });
        
    team_invited -= ({ 0 });

    return ({ }) + team_invited;
}

/*
 * Function name:   remove_invited
 * Description:     Remove an object from the invited list.
 * Argumnents:      ob - The object to remove from the list.
 */
public void
remove_invited(object ob)
{
    if (team_invited)
	team_invited -= ({ 0, ob });
}

/*
 * Function name: attacked_by
 * Description  : When someone attacks us, this function is called. It makes
 *                a test, preventing anyone from attacking us while we are
 *                linkdeath.
 * Arguments    : object attacker - who is attacking us.
 */
public void
attacked_by(object attacker)
{
    if (!query_ip_number(this_object()))
    {
	tell_object(this_object(), "Masz zerwane polaczenie, wiec " +
	    attacker->query_imie(this_object(), PL_MIA) + " nie moze "+
	    "cie zaatakowac.\n");
	tell_object(attacker, "Nie mozesz zaatakowac " +
	    this_object()->query_imie(attacker, PL_DOP) +
	    ", gdyz nie ma " + query_zaimek(PL_MIA) +
	    " kontaktu z rzeczywistoscia.\n");

	set_alarm(0.5, 0.0, &linkdeath_remove_enemy(attacker));
	return;
    }

    ::attacked_by(attacker);
}

/*
 * Function name: attack_object
 * Description  : When we attack someone, this function is used. It has a
 *                check preventing us from attacking anyone while we are
 *                linkdeath.
 * Arguments    : object victim - the intended victim.
 */
public void
attack_object(object victim)
{
    if (!query_ip_number(this_object()))
    {
	tell_object(this_object(), "Masz zerwane polaczenie i nie mozesz" +
	    "zaatakowac " + victim->query_imie(this_object(), PL_DOP) + ".\n");
	tell_object(victim, this_object()->query_Imie(victim, PL_MIA) +
	    " nie moze cie zaatakowac, gdyz nie ma kontaktu z "+
	    "rzeczywistoscia.\n");

	set_alarm(0.5, 0.0, &linkdeath_remove_enemy(victim));
	return;
    }

    ::attack_object(victim);
}

/*
 * Function name: linkdeath_remove_enemy
 * Description  : This function removes a player from the list of enemies
 *                if combat was initiated between this object and the
 *                enemy.
 * Arguments    : object enemy - the enemy we should not fight.
 */
static nomask void
linkdeath_remove_enemy(object enemy)
{
    this_object()->stop_fight(enemy);
    enemy->stop_fight(this_object());
}
