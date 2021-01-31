/* 
 * /std/living/wizstat.c
 *
 * Contains the code to implement the various wizardly information commands.
 */

#include <filepath.h>

/*
 * Function name: stat_living
 * Description  : Give status information on this living.
 * Returns      : string - the description.
 */
public string
stat_living()
{
    string str, tmp;
    object to;

    to = this_object();

    str = sprintf("Imie: %-11s Ranga: %-10s (%-2d) " +
		  "Rasa: %-10s Plec: %-10s\n" +
	          "Plik: %-31s Uid: %-11s  Euid: %-11s\n",
		  capitalize(query_real_name()),
		  WIZ_RANK_NAME(SECURITY->query_wiz_rank(query_real_name())),
		  SECURITY->query_wiz_level(query_real_name()),
		  to->query_rasa(),
		  to->query_gender_string(),
		  extract(RPATH(file_name(this_object())), 0, 30),
		  getuid(this_object()),
		  geteuid(this_object()));

    if ((function_exists("enter_game", this_object()) != PLAYER_SEC_OBJECT) ||
	(this_interactive() == this_object()) ||
	wildmatch("*jr*", this_object()->query_real_name()) ||
	(SECURITY->query_wiz_level(this_interactive()->query_real_name()) > 16))
	str += sprintf(
		  "------------------------------------------------------" +
		  "--------------------\n" +
		  "Exp: %-8d   Expquest: %-7d    Hp: %4d(%4d) " +
		  "   Mana: %4d(%4d)\n" +  
		  "Zmeczenie: %5d(%5d)  Waga: %7d(%7d)   Objetosc: %7d(%7d)\n" +
		  "Stats  :   %@7s\n\n"  +
                  "    Val:   %@7d\n" +
                  "    Acc:   %@7d\n" +
	          "  Learn:   %@7d\n\n" +
		  "Intox: %4d  Headache: %3d  Stuffed: %3d Soaked: %3d  " +
		  "Align : %d\n" +
		  "Ghost   : %3d  Invis  : %3d Npc   : %3d  " +
		  "Whimpy: %3d\n",
		  to->query_exp(),
		  (to->query_exp() - to->query_exp_combat()),
		  to->query_hp(),
		  to->query_max_hp(),
		  to->query_mana(),
		  to->query_max_mana(),
		  to->query_fatigue(),
		  to->query_max_fatigue(),
		  to->query_prop(OBJ_I_WEIGHT),
		  to->query_prop(CONT_I_MAX_WEIGHT),
		  to->query_prop(OBJ_I_VOLUME),
		  to->query_prop(CONT_I_MAX_VOLUME),
		  SS_STAT_DESC,
		  ({ to->query_stat(SS_STR),
		     to->query_stat(SS_DEX),
		     to->query_stat(SS_CON),
		     to->query_stat(SS_INT),
		     to->query_stat(SS_WIS),
		     to->query_stat(SS_DIS),
		     to->query_stat(SS_RACE),
		     to->query_stat(SS_GUILD),
		     to->query_stat(SS_OTHER) }),
		  ({ to->query_acc_exp(SS_STR),
		     to->query_acc_exp(SS_DEX),
		     to->query_acc_exp(SS_CON),
		     to->query_acc_exp(SS_INT),
		     to->query_acc_exp(SS_WIS),
		     to->query_acc_exp(SS_DIS),
		     to->query_acc_exp(SS_RACE),
		     to->query_acc_exp(SS_GUILD),
		     to->query_acc_exp(SS_OTHER) }),
		  to->query_learn_pref(-1),
		  to->query_intoxicated(),
		  to->query_headache(),
		  to->query_stuffed(),
		  to->query_soaked(),
		  to->query_alignment(),
		  to->query_ghost(),
		  to->query_invis(),
		  to->query_npc(),
		  to->query_whimpy());

    if (strlen(tmp = to->query_prop(OBJ_S_WIZINFO)))
	str += "Wizinfo:\n" + tmp;

    return str;
}

/*
 * Function name: fix_skill_desc
 * Description  : This function will compose the string describing the
 *                individual skills the player has.
 * Arguments    : int sk_type     - the skill number.
 *                mapping sk_desc - the mapping describing the skills.
 * Returns      : string - the description for this skill.
 */
nomask static string
fix_skill_desc(int sk_type, mapping sk_desc)
{
    string desc;

    if (pointerp(sk_desc[sk_type]))
    {
        desc = sk_desc[sk_type][0];
    }
    else
    {
        if (!(desc = this_object()->query_skill_name(sk_type)))
	{
	    desc = "special";
	}
    }

    return sprintf("%s: %3d (%6d)", extract(desc, 0, 23),
		   this_object()->query_skill(sk_type), sk_type);
}

/*
 * Function name: skill_living
 * Description  : This function returns a proper string describing the
 *                skills of this living.
 * Returns      : string - the description.
 */
public string
skill_living()
{
    string *skills;
    string sk;
    int *sk_types;
    int index;
    int size;
    mapping sk_desc;

    sk_types = sort_array(query_all_skill_types());

    if (!sizeof(sk_types))
    {
	return capitalize(query_real_name()) + " has no skills.\n";
    }

    sk_desc = SS_SKILL_DESC;
    sk = "";
    skills = map(sk_types, &fix_skill_desc(, sk_desc));
    size = ((sizeof(skills) + 1) / 2);
    skills += ({ "" });
    index = -1;
    while(++index < size)
    {
	sk += sprintf("%38s %38s\n", skills[index], skills[index + size]);
    }
    
    return "Skills of " + capitalize(query_real_name()) + ":\n" + sk;
}
