/*
 * /d/Genesis/obj/guest_shadow.c
 *
 * Copyright (C) Stas van der Schaaf - December 23 1994
 *               Mercade @ Genesis
 *
 * This is the shadow of the guest player. Guest is a guest of the game.
 * We do not want him to join guilds, be engaged in combat or gain any
 * experience.
 *
 * Revision history:
 */

inherit "/std/shadow";

#pragma no_inherit
#pragma save_binary
#pragma strict_types

#include <mail.h>
#include <macros.h>

#define QSW   query_shadow_who()
#define GUEST "gosc"
#define TITLE "Arkadii"

/* Define this if you do not want Guest to use the mail system. */
#undef GUEST_MAIL_BLOCK

/*
 * Function name: query_guild_not_allow_join_gulid
 * Description  : This function is called before a player may join a guild.
 *                It is used to prevent guest from joining any guild.
 * Arguments    : object player - the player who wants to join;
 *                string type   - the type of guild someone wants to join;
 *                string style  - the style of the guild you want to join;
 *                string name   - the name of the guild you want to join.
 * Returns      : int 1 - always.
 */
nomask public int
query_guild_not_allow_join_guild(object player, string type, string style,
                                 string name)
{
    write("\nBedac gosciem Arkadii nie mozesz wstepowac do gildii.\n\n");
    return 1;
}

/*
 * Function name: attacked_by
 * Description  : When someone attacks us, this function is called. We
 *                disallow all combat with the guest player, so the call
 *                is intercepted and the combat cancelled.
 * Arguments    : object attacker - the player attacking us.
 */
nomask public void
attacked_by(object attacker)
{
    QSW->catch_msg("\n" + attacker->query_Imie(QSW, PL_MIA) + " probowal"
                 + attacker->koncowka("", "a") + " cie zaatakowac. Bedac "
                 + "gosciem Arkadii, jestes jednak chroniony przez bogow "
                 + "stojacych na strazy swietych praw goscinnosci.\n\n");
    attacker->catch_msg("\nProbowal" + attacker->koncowka("es", "as")
                      + " zaatakowac goscia Arkadii. Jest on jednak "
                      + "chroniony przez bogow, stojacych na strazy swietych "
                      + "praw goscinnosci.\n\n");

    QSW->stop_fight(attacker);
    attacker->stop_fight(QSW);
}

/*
 * Function name: attack_object
 * Description  : When the player attacks someone, this function is called.
 *                The guest of Genesis should not get ingaged in combat.
 * Arguments    : object victim - the intended victim.
 */
nomask public void
attack_object(object victim)
{
    QSW->catch_msg("\nDopoki jestes gosciem Arkadii, bogowie nie pozwola ci "
                 + "podejmowac wrogich dzialan wobec swych gospodarzy.\n\n");
    victim->catch_msg("\nProbowal cie zaatakowac gosc Arkadii. Bogowie "
                    + "jednak nie zamierzaja pozwalac mu na podejmowanie "
                    + "wrogich dzialan wobec swych gospodarzy.\n\n");

    QSW->stop_fight(victim);
    victim->stop_fight(QSW);
}

/*
 * Function name: do_die
 * Description  : This function is called whenever someone suspects that
 *                we have died. We do not want the guest to die, so the
 *                call is intercepted.
 * Arguments    : killer - the object that killed us.
 */
nomask public void
do_die(object killer)
{
    object *enemies;

    if (QSW->query_hp() > 0 || QSW->query_ghost())
	return;

    if (!objectp(killer))
	killer = previous_object();

    /* Heal him fully, so this function isn't called again. */
    QSW->set_hp(QSW->query_max_hp());

    tell_room(environment(QSW), "\nMimo ze z goscia Arkadii uszly wszelkie "
            + "sily zyciowe, Smierc nie przyjdzie po niego, lekajac sie "
            + "bogow stojacych na strazy swietych praw goscinnosci.\n\n",
              QSW);
    killer->catch_msg("Zabil" + killer->koncowka("es", "as")
                    + " goscia Arkadii.\n");
    QSW->catch_msg("Bogowie, stojacy na strazy swietych praw goscinnosci, "
                 + "ocalili cie przed smiercia.\n");

    /* Even though the guest cannot be in combat, we remove all enemies
     * just in case.
     */
    enemies = (object *)QSW->query_enemy(-1) - ({ 0 });
    if (sizeof(enemies))
    {
        QSW->stop_fight(enemies);
        enemies->stop_fight(QSW);
    }
}

/*
 * Function name: add_exp
 * Description  : There is no reason for guest to gain experience. Therefore
 *                we disallow any experience other than the experience
 *                added by the player itself. This is vital since it is done
 *                in the player start-sequence.
 * Arguments    : int experience - the experience to add;
 *                int battle     - true if gained in battle.
 */
nomask public void
add_exp(int experience, int battle)
{
    if (previous_object() == QSW)
        QSW->add_exp(experience, battle);
}

/*
 * Nazwa funkcji : set_bit
 * Opis          : Nie widzimy powodu, aby gosc mial ustawiane bity questowe.
 * Argumenty     : group - 0-4
 *                 bit - 0-19
 * Funkcja zwraca: Czy udalo sie ustawic bit.
 */
nomask public int
set_bit(int group, int bit)
{
    return 0;
}

/*
 * Function name: query_guild_title_race
 * Description  : The guest of Genesis has his own title.
 * Returns      : string - the title.
 */
nomask public string
query_guild_title_race()
{
    return TITLE;
}

/*
 * Function name: query_guild_family_name
 * Description  : If this function returns true, the article 'the' is
 *                omitted in the title.
 * Returns      : int 1 - always.
 */
nomask public int
query_guild_family_name()
{
    return 1;
}

/*
 * Function name: set_race_name
 * Description  : The guest of Genesis is a human being. We do not wants
 *                his race to be set to anything else.
 * Arguments    : string name - the race name to set.
 */
nomask public void
set_race_name(string name)
{
    QSW->set_race_name("czlowiek");
}

/*
 * Function name: ustaw_odmiane_rasy
 * Description  : The guest of Genesis is a human being. We do not wants
 *                his race to be set to anything else.
 * Arguments    : string name - the race name to set.
 */
nomask public void
ustaw_odmiane_rasy(string *pojedyncza, string *mnoga, int rodzaj,
                   int plec_osobno)
{
    QSW->set_race_name(PL_CZLOWIEK);
}

/*
 * Function name: autoload_shadow
 * Description  : This function is called to ensure that the shadow
 *                autoloads. If the player is anyone else than the guest,
 *                the shadow selfdestructs.
 * Arguments    : mixed arg - the possible arguments.
 */
nomask public void
autoload_shadow(mixed arg)
{
    ::autoload_shadow(arg);

    if (!objectp(QSW) || QSW->query_real_name() != GUEST)
    {
        QSW->remove_autoshadow(MASTER);
        destruct();
    }
}

#ifdef GUEST_MAIL_BLOCK
/*
 * Function name: enter_inv
 * Description  : This function will mask the enter_inv() function of the
 *                guest to prevent the mail reader from entering.
 * Arguments    : object obj  - the object entering.
 *                object from - the object it came from.
 */
nomask public void
enter_inv(object obj, object from)
{
    if (obj->id(READER_ID))
    {
        set_alarm(1.0, 0.0, &tell_object(QSW, "\nBedac gosciem Arkadii nie "
                + "mozesz korzystac z poczty.\n\n"));
        obj->remove_object();
        return;
    }

    QSW->enter_inv(obj, from);
}
#endif GUEST_MAIL_BLOCK

/*
 * Function name: query_prevent_shadow
 * Description  : No additional shadows should be added the player. This
 *                function returns true to prevent addition of shadows to
 *                the player.
 * Returns      : int 1 - always.
 */
nomask public int
query_prevent_shadow()
{
    return 1;
}
