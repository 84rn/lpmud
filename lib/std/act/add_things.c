/* std/act/add_things.c
 *
 * This is a file inherited into monster.c that enables the player to
 * add weapons and armour to the monster in a very convinient way. The
 * monster will automaticly wield/wear the things. 
 *
 * just do add_armour(string filename);        or
 *         add_weapon(string filename); 
 * 
 * and the monster will clone, move and wield/wear the things.
 *      (The functions return the objects)
 *
 *     Dimitri, the mighty Avatar !
 *
 * We thank PaderMUD for this File
 */

#pragma save_binary
#pragma strict_types

#include <macros.h>
#include <stdproperties.h>

int alarm_id;
object *wep = ({});
object *arm = ({});

void
move_and_wearwield()
{
    int x;
    
    alarm_id = 0;
    
    set_this_player(this_object()); // dla wield/wear _me().
    
    x = sizeof(wep);
    while (--x >= 0)
    {
        if (wep[x]->move(this_object()))
            wep[x]->remove_object();
        else
            wep[x]->wield_me(1);
    }
    
    x = sizeof(arm);
    while (--x >= 0)
    {
        if (arm[x]->move(this_object()))
            arm[x]->remove_object();
        else
            arm[x]->wear_me(1);
    }
    
    arm = ({}); wep = ({});
}

/*
 * Function name:  add_weapon
 * Descriptien:    clones a weapon to the monster and makes the monster wield it.
 * Arguments:      filename:  The filename of the weapon. [string]
 * Returns:        the weapon
 */
public object
add_weapon(string file)
{
    object weapon;

    if (!strlen(file))
    	return 0;

    seteuid(getuid(this_object()));
    
    weapon = clone_object(file);
    
    if (!weapon)
    	return 0;

    wep += ({ weapon });
    
    if (!alarm_id)
        alarm_id = set_alarm(1.0, 0.0, &move_and_wearwield());


    return weapon;
}

/*
 * Function name:  add_armour
 * Description:    clones an armour to the monster and makes the monster wear it.
 * Arguments:      filename: The filename of the armour. [string]
 * Returns:        the armour
 */
public object
add_armour(string file)
{
    object armour;
  
    if (!strlen(file))
    	return 0;

    seteuid(getuid(this_object()));
    armour = clone_object(file);

    if (!armour)
    	return 0;

    if (!alarm_id)
        alarm_id = set_alarm(1.0, 0.0, &move_and_wearwield());

    arm += ({ armour });

    return armour;
}
