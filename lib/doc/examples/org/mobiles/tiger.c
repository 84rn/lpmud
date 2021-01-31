/*
    /doc/examples/mobiles/tiger2.c

    JnA 920111

    A sample creature 

       This creature uses no tools for fighting. 
       It inherits the routines for unarmed combat.

    This is a rather tough beast. You need on average 30 or more in your
    stats to handle it. You also need wc30 weapon with skills to match.

*/

inherit "/std/creature";
inherit "/std/combat/unarmed";   /* This gets us standard unarmed routines */

inherit "/std/act/domove"; /* Include this if you want the creature to move */
inherit "/std/act/action"; /* Include this if you want your creature to act */

#include <wa_types.h>
#include <ss_types.h>
#include <macros.h>
#include <filter_funs.h>

/*
 * Define some attack and hitloc id's (only for our own benefit)
 */
#define A_BITE  0
#define A_LCLAW 1
#define A_RCLAW 2

#define H_HEAD 0
#define H_BODY 1

create_creature()
{
    set_name("tiger"); 
    set_race_name("tiger");
    set_short("white tiger");
    set_adj(({"white", "vicious" }));
    set_long("It looks rather vicious!\n");

    /* str, con, dex, int, wis, dis
    */
    set_stats(({ 90, 30, 80, 20, 5, 75}));

    set_hp(query_max_hp());

    set_skill(SS_DEFENCE, 30);
    set_skill(SS_SWIM, 80);

    set_attack_unarmed(A_BITE,  20, 30, W_IMPALE, 40, "jaws");
    set_attack_unarmed(A_LCLAW, 40, 20, W_SLASH,  30, "left paw");
    set_attack_unarmed(A_RCLAW, 40, 20, W_SLASH,  30, "right paw");
   
    set_hitloc_unarmed(H_HEAD, ({ 15, 25, 20, 20 }), 20, "head");
    set_hitloc_unarmed(H_BODY, ({ 10, 15, 30, 20 }), 80, "body");
}

/*
 * Function name: tell_watcher
 * Description:   Send the string from the fight to people that want them
 * Arguments:     The string to send
 */
static void
tell_watcher(string str, object enemy)
{
    object me,*ob;
    int i;

    me = this_object();
    ob = FILTER_LIVE(all_inventory(environment(me))) - ({ me });
    ob -= ({ enemy });
    for (i = 0; i < sizeof(ob); i++)
        if (ob[i]->query_see_blood())
            ob[i]->catch_msg(str);
}

/*
 * Here we redefine the special_attack function which is called from
 * within the combat system. If we return 1 then there will be no
 * additional ordinary attack.
 *
 */
int
special_attack(object enemy)
{
    object me;
    mixed* hitresult;
    string how;

    me = this_object();
    if(random(10))
	return 0;                         /* Continue with the attack. */

    hitresult = enemy->hit_me(20+random(30), W_IMPALE, me, -1);
    how = " without effect";
    if (hitresult[0] > 0)                 /* hitresult[0] yields the % hurt. */
	how == "";
    if (hitresult[0] > 10)
	how = " hard";
    if (hitresult[0] > 20)
	how = " very hard";
    
    me->catch_msg("You leap into your opponent's throut!\n" +
		  capitalize(enemy->query_pronoun()) + " is hit" + 
		  how + ".\n");
    enemy->catch_msg(QCTNAME(me) + " leaps into your throut!\n"+
		     "You are hit" + how + ".\n");
    tell_watcher(QCTNAME(me)+" leaps into "+QTNAME(enemy)+"!\n"+
		 capitalize(enemy->query_pronoun()) + " is hit" + 
		 how + ".\n", enemy);
    if(enemy->query_hp() <= 0)
	enemy->do_die(me);
    
    return 1; /*  Important! Should not have two attacks in a round. */
}
