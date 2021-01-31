/*
 * /secure/finger_player.c
 *
 * This is a special player object that is used when a restore_object
 * is performed in the finger_player() function of the master object.
 * It saves a lot of time compared to loading a normal player object.
 */

#pragma no_inherit
#pragma no_shadow
#pragma strict_types

#include <config.h>

inherit MAIL_INFO_OBJ;
inherit SPECIAL_INFO_OBJ;

#include <formulas.h>
#include <living_desc.h>
#include <login.h>
#include <macros.h>
#include <ss_types.h>
//#include <state_desc.h>
#include <std.h>
#include <debug.h>

#define STAT if (!stats_set) acc_exp_to_stats()

private string login_from,
               mailaddr,
               name,
               password,
               player_file,
               race_name,
               title,
              *imiona,
              *rasy,
              *seconds;

private int age_heart,
            exp_combat,
            exp_points,
            gender,
            is_ghost,
            login_time,
            osobno,
           *acc_exp,
           *learn_pref;

private mixed *przymiotniki;

private mapping m_alias_list;
//private mapping m_remember_name;

private static int wiz_level,
                   stats_set = 0,
                  *stats;

static void acc_exp_to_stats();

public void
create()
{
    seteuid(0);
}

public void
finger_info()
{
    seteuid(getuid());
    finger_mail();
    finger_special();
    seteuid(0);
}

public int
load_player(string pl_name)
{
    int ret;
   
    if (!pl_name || wildmatch("* *", pl_name))
        return 0;

    seteuid(getuid());
    ret = restore_object(PLAYER_FILE(pl_name));
    seteuid(0);

    wiz_level = SECURITY->query_wiz_rank(pl_name);

    return ret;
}

public void
master_set_name(string n)
{
    if (file_name(previous_object()) == SECURITY)
        name = n;
}

public void
open_player()
{
    if (file_name(previous_object()) == SECURITY)
        seteuid(0);
}

/*
 * Function name: query_finger_player
 * Description  : This function identifies this object as a finger-player
 *                object.
 * Returns      : int 1 - always.
 */
public int
query_finger_player()
{
    return 1;
}

#if 0
public int
notmet_me(object obj)
{
    if (obj && query_ip_number(obj))
	return !obj->query_met(this_object());
    return !this_player()->query_met(this_object());
}
#endif

public string query_race_name() { return race_name; }

public string
query_rasa(int przyp = PL_MIA)
{
    return sizeof(rasy) ? rasy[przyp] : 0;
}

public string query_race() { return find_object(player_file)->query_race(); }

public string query_title() { return title; }

public string
query_real_name(int przyp = PL_MIA)
{
    return sizeof(imiona) ? imiona[przyp] : name;
}

public string
query_name(int przyp = PL_MIA)
{
    return capitalize(query_real_name(przyp));
}

static int
query_rodzaj()
{
    string race = query_race();

    /* Oczywiscie to tylko przyblizenie... */
    return race ? ODMIANA_RASY_RODZAJ[race][gender == G_FEMALE] : PL_MESKI_NOS_ZYW;
}

public string *
query_przym(int przyp = PL_MIA)
{
    if (sizeof(przymiotniki) && sizeof(przymiotniki[0]))
    {
        string *przym = ({});
        int n = -1;
        int size = sizeof(przymiotniki[0]);
        int rodzaj = query_rodzaj();

        while (++n < size)
            przym += ({oblicz_przym(przymiotniki[0][n], przymiotniki[1][n],
                                    przyp, rodzaj, 0)});

        return przym;
    }

    return ({});
}

public string
query_nonmet_name(int przyp = PL_MIA)
{
    string *przym;
    string str;

    if (is_ghost & GP_INTRO)
    {
        str = LD_DUCH[przyp];

        if (!sizeof(rasy))
            return str;

        str += " ";
        przyp = PL_DOP;
    }
    else if (!sizeof(rasy))
        return 0;
    else
        str = "";

#if 0
    if (sizeof(przym = query_przym(przyp)))
        str += implode(przym, " ") +  " ";
#else
    if (sizeof(przym = query_przym(przyp)))
    {
        int index = sizeof(przym);

        while (index--)
            str += przym[index] + " ";
    }
#endif

    str += osobno ? LD_HUM_GENDER_MAP[gender][przyp] : query_rasa(przyp);

    if (wiz_level)
        str += " " + LD_CZARODZIEJ[przyp];

    return str;
}

#if 0
string
query_the_name(object pobj)
{
    if (!objectp(pobj))
	pobj = previous_object(-1);

    if (notmet_me(pobj))
	return LD_THE + " " + query_nonmet_name();
    else
	return query_met_name();
}

string query_The_name(object pobj) { return capitalize(query_the_name(pobj)); }
#endif

public string
query_wolacz()
{
    return LANG_FILE->wolacz(query_name(PL_MIA), query_name(PL_MIE));
}

public string query_mailaddr() { return mailaddr; }

public int query_gender() { return gender; }

public string query_player_file() { return player_file; }

public string query_login_from() { return login_from; }

public int query_login_time() { return login_time; }

public string query_pronoun() { return LD_PRONOUN_MAP[gender]; }

public string query_possessive() { return LD_POSSESSIVE_MAP[gender]; }

public string query_gender_string() { return LD_GENDER_MAP[gender]; }

public int query_age() { return age_heart; }

/*
 * Function name: query_password
 * Description  : Return the password of the player. Only SECURITY may do
 *                this.
 * Returns      : string - the password, else 0.
 */
public string
query_password()
{
    return file_name(previous_object()) == SECURITY ? password :
           "Kocham Arkadie";
}

/*
 * Function name: match_password
 * Description  : Match the password of a player with an arbitrary string
 *                that is claimed to be the password of a player. NOTE that
 *                if the player has NO password, everything matches.
 * Arguments    : string p - the password to match.
 * Returns      : int - true/false.
 */
nomask int
match_password(string p)
{
    return !password || password == crypt(p, password);
}

public int query_exp() { return exp_points; }

public int query_exp_combat() { return exp_combat; }

public string
query_exp_title()
{
    return wiz_level ? "czarodziej" : "smiertelnik";
}

public string short(int przyp = PL_MIA) { return query_name(przyp); }

public int query_ghost() { return is_ghost; }

public int query_wiz_level() { return wiz_level; }

public string
query_presentation()
{
    if (wiz_level)
        return SECURITY->query_wiz_pretitle(this_object()) + " "
             + query_name(PL_MIA) + (strlen(title) ? " " + title : "") + ", "
             + query_rasa(PL_MIA);

    return query_name(PL_MIA) + " " + query_exp_title() + ", "
         + query_rasa(PL_MIA);
}

public string 
long(mixed for_obj)
{
    return "Jest " + this_object()->query_nonmet_name(PL_NAR) + ", znan"
         + (gender == G_FEMALE ? "a" : "ym") + " jako:\n"
         + query_presentation() + ".\n";
}

public string query_objective() { return LD_OBJECTIVE_MAP[gender]; }

public mixed
query_learn_pref(int stat)
{
    if (stat < 0)
        return secure_var(learn_pref);

    if (stat >= SS_NO_STATS)
        return -1;

    return learn_pref[stat];
}

public int
query_acc_exp(int stat)
{
    if (stat < 0 || stat >= SS_NO_STATS)
        return -1;

    return acc_exp[stat];
}

static int
set_base_stat(int stat, int val)
{
    if (stat < 0 || stat >= SS_NO_STATS || val < 1)
        return 0;

    stats[stat] = val;
    return val;
}

public int
exp_to_stat(int exp)
{
    return F_EXP_TO_STAT(exp);
}

public int
query_base_stat(int stat)
{
    STAT;

    if (stat < 0 || stat >= SS_NO_STATS)
        return -1;

    return stats[stat];
}

static void
acc_exp_to_stats()
{
    int il, tmp;

    stats = allocate(SS_NO_STATS);
    stats_set = 1;

    for (il = SS_STR; il < SS_NO_STATS; il++)
    {
        if (query_base_stat(il) >= 0)
        {
            tmp = exp_to_stat(query_acc_exp(il) * 
                RACESTATMOD[query_race()][il] / 10);
            set_base_stat(il, tmp);
        }
    }
}

public int
query_stat(int stat)
{
    int tmp;

    if (stat < 0 || stat >= SS_NO_STATS)
        return -1;

    tmp = query_base_stat(stat);

    return tmp > 0 ? tmp : 1;
}

public string
stat_living()
{
    string str;
    object to;

    to = this_object();

    str = sprintf("Name: %15s(%-15s   Race: %-10s    Gender: %-10s\n",
                  to->query_name(), query_real_name()+")",
                  to->query_race_name(),
                  to->query_gender_string());

    if ((this_interactive() == this_object()) ||
	wildmatch("*jr*", this_object()->query_real_name()) ||
	(SECURITY->query_wiz_level(this_interactive()->query_real_name()) > 16))
	str += sprintf(
                  "------------------------------------------------------" +
                  "----------------------\n" +
                  "Exp: %-7d    Expquest: %-7d\n" +
                  "Stats:     %@7s\n"  +
                  "    Val:   %@7d\n" +
                  "   Base:   %@7d\n" +
                  "    Acc:   %@7d\n" +
                  "  Learn:   %@7d\n",
                  to->query_exp(), to->query_exp() - to->query_exp_combat(),
                  SS_STAT_DESC,
                  ({ to->query_stat(SS_STR), to->query_stat(SS_DEX),
                     to->query_stat(SS_CON), to->query_stat(SS_INT),
                     to->query_stat(SS_WIS), to->query_stat(SS_DIS),
                     to->query_stat(SS_RACE), to->query_stat(SS_GUILD),
                     to->query_stat(SS_OTHER) }),
                  ({ F_EXP_TO_STAT(to->query_acc_exp(SS_STR)),
                     F_EXP_TO_STAT(to->query_acc_exp(SS_DEX)),
                     F_EXP_TO_STAT(to->query_acc_exp(SS_CON)),
                     F_EXP_TO_STAT(to->query_acc_exp(SS_INT)),
                     F_EXP_TO_STAT(to->query_acc_exp(SS_WIS)),
                     F_EXP_TO_STAT(to->query_acc_exp(SS_DIS)),
                     F_EXP_TO_STAT(to->query_acc_exp(SS_RACE)),
                     F_EXP_TO_STAT(to->query_acc_exp(SS_GUILD)),
                     F_EXP_TO_STAT(to->query_acc_exp(SS_OTHER)) }),
                  ({ to->query_acc_exp(SS_STR), to->query_acc_exp(SS_DEX),
                     to->query_acc_exp(SS_CON), to->query_acc_exp(SS_INT),
                     to->query_acc_exp(SS_WIS), to->query_acc_exp(SS_DIS),
                     to->query_acc_exp(SS_RACE), to->query_acc_exp(SS_GUILD),
                     to->query_acc_exp(SS_OTHER) }),
                  to->query_learn_pref(-1)
                  );
    return str;
}

public void 
remove_object()
{
    destruct();
}

public mapping
query_aliases()
{
    return m_alias_list ? secure_var(m_alias_list) : ([]);
}

public string *
query_seconds()
{
    if (this_interactive() != this_object() && WIZ_CHECK < WIZ_ARCH &&
        this_interactive()->query_real_name() !=
            SECURITY->query_domain_lord(SECURITY->query_wiz_dom(name)))
        return 0;

    return seconds ? secure_var(seconds) : ({});
}

#if 0
public mapping
query_remember_name()
{
    return m_remember_name ? save_var(m_remember_name) : ([]);
}
#endif