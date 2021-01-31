/*
 *  /std/living/savevars.c
 *
 *  This file contains all player variables that are saved in the 
 *  player save file. The corresponding set- and query functions
 *  can also be found here.
 *
 *  This file is included into /std/living.c
 */

#pragma strict_types

#include <composite.h>
#include <const.h>
#include <ss_types.h>
#include <macros.h>
#include <stdproperties.h>
#include <formulas.h>
#include <std.h>

private string m_in,	        /* Messages when entering or leaving a room. */
	       m_out,   	/* Messages when entering or leaving a room. */
	       mm_in,	        /* Message when arriving by teleport. */
	       mm_out,		/* Message when leaving by teleport. */
               race_name,	/* The name of the race */
               *rasy=({}),	/* Odmiana nazwy rasy */
               *prasy=({}),	/* To samo, liczba mnoga */
               title,		/* Title of the living */
	       *cmdsoul_list,	/* The command souls */
	       *tool_list,	/* Names of tool souls I want */
#ifndef NO_ALIGN_TITLE
               al_title,	/* Alignment title of the living */
#endif NO_ALIGN_TITLE
	       *textgivers;     /* Filenames of objects giving names for
	       			   stats, skills etc */

private int    hit_points,	/* The hitpoints of this lifeform. */
               mana,            /* The magic points of this lifeform */
               fatigue,         /* How easily this lifeform is fatigued */
	       exp_points,	/* The experience points of this lifeform. */
               exp_combat,	/* Amount of exp gained in combat */
	       is_ghost,	/* If lifeform is dead */
	       is_whimpy,	/* Automatically flee when low on HP */
	       alignment,	/* Depends on good or chaotic lifeform */
	       gender,          /* 0 male("he"),1 female("she"),2 neut("it") */
               headache,	/* Hangover coefficient */
    	       intoxicated,     /* How drunk are we? */
    	       stuffed,         /* Are we fed up or not */
	       soaked,		/* How soaked are we ? */
	       *learn_pref,	/* Prefered % to learn / stat */
	       *acc_exp,	/* Accumulated exp / stat */
               osobno;		/* Czy w odmianye rasy, plec jest osobno? */
private mapping
               skillmap;        /* Our skills in a mapping */
static	mapping
	       skill_extra_map; /* Extra skills added by items for example */
	       
static	int    rodzaj_rasy = -1;/* Rodzaj gramatyczny rasy */

/*
 * Prototypes
 */

void set_max_headache(int h);
int query_max_headache(); 
int query_stuffed();
int query_headache();
int query_intoxicated();
int query_stat(int stat);
int query_skill(int skill);
void set_hp(int hp);
string query_race_name();
nomask public int remove_cmdsoul(string soul);
nomask public int remove_toolsoul(string soul);

/*
 *  These vars keep track of the last time the appropriate var was updated.
 */
static private int headache_time;
static private int intoxicated_time;
static private int stuffed_time;
static private int soaked_time;
static private int hp_time;
static private int mana_time;
static private int fatigue_time;
static private int last_intox;
static private int last_con;
static private int last_stuffed;

static nomask void
savevars_delay_reset()
{
    last_stuffed = query_stuffed();
    last_intox = query_intoxicated();
    last_con = query_stat(SS_CON);
}

/*
 * Function name:   save_vars_reset
 * Description:     Resets some variables which are used to keep track
 *                  of how variables change with time.
 */
static nomask void 
save_vars_reset()
{
    int t = time();

    headache_time     = t;
    intoxicated_time  = t;
    stuffed_time      = t;
    soaked_time       = t;
    hp_time           = t;
    mana_time         = t;
    fatigue_time      = t;

    set_alarm(1.0, 0.0, savevars_delay_reset);
}

/*
 * Function name: skill_extra_map_reset
 * Description  : Reset the skill_extra_map at initialization.
 */
private void
skill_extra_map_reset()
{
    skill_extra_map = ([ ]);
}

/*
 * Odmiana rasy
 */
 
/*
 * Nazwa funkcji : ustaw_odmiane_rasy
 * Opis          : Sluzy do odmieniania nazwy rasy. Rasa powinna byc
 *		   dostosowana do plci, np gdy mamy kobiete elfa, 
 *		   odmieniana rasa bedzie 'elfka'. Dodawane sa rowniez
 *		   nazwy 'kobieta', 'samica' itp, zaleznie od tego, czy
 *		   NPC jest humanoidem.
 * Argumenty     : *pojedyczna - odmiana nazwy rasy w liczbie pojedynczej,
 *		   *mnoga - odmiana nazwy rasy w liczbie mnogiej.
 *		   rodzaj - rodzaj gramatyczny nazwy rasy, np. w przypadku
 *			    elfki bedzie to PL_ZENSKI
 *		   plec_osobno - opcjonalny argument, mowiacy czy przy
 *		   		 wyswietlaniu nazwy rasy, ma byc podany
 *				 czlon kobieta/mezczyzna
 *		   Dwa pierwsze argumenty akceptuja wylacznie 
 *		   6-scio elementowe tablice.
 * Funkcja zwraca: int - 1 w przypadku ustawienia rasy, zas 0 w przeciwnym
 *			 razie.
 */
public varargs int
ustaw_odmiane_rasy(string *pojedyncza, string *mnoga, int rodzaj, 
		   int plec_osobno)
{
    int x, rodz;

    if (pojedyncza)
    {
        if (sizeof(pojedyncza) != 6 || sizeof(mnoga) != 6)
            return 0;

        if (sizeof(rasy) && sizeof(prasy))
            for (x = 0; x < 6; x++)
            {
                remove_name(rasy[x], x);
                remove_pname(prasy[x], x);
            }
        
        rasy = pojedyncza;
        prasy = mnoga;
        
        rodzaj_rasy = (plec_osobno ? query_living_rodzaj()
        		      : rodzaj);
        
        dodaj_nazwy(rasy, prasy, rodzaj);
        
        osobno = plec_osobno;
    }
    else
    {
        if (sizeof(rasy) != 6 || sizeof(prasy) != 6)
        {
            rasy = allocate(6);
            prasy = allocate(6);
            if (query_race_name())
                for (x = 0; x < 6; x++)
                {
                    rasy[x] = query_race_name();
                    prasy[x] = query_race_name();
                    plec_osobno = 1;
                }
                
        }
        
        rodzaj_rasy = this_object()->query_living_rodzaj();
        dodaj_nazwy(rasy, prasy, rodzaj_rasy);
    }
    
    return 1;
}

/*
 * Nazwa funkcji : query_rasa
 * Opis          : Zwraca nazwe rasy livinga, w liczbie pojedynczej,
 *		   w podanym przypadku. 
 * Argumenty     : int - przypadek gramatyczny
 * Funkcja zwraca: Patrz opis.
 */
varargs public string
query_rasa(int przyp)
{
/*
    if (osobno && !query_prop(LIVE_I_NO_GENDER_DESC))
        return LD_HUM_GENDER_MAP[query_gender()][przyp];
*/
        
    if (sizeof(rasy) == 6)
        return rasy[przyp];

    return 0;       
/*
        return this_object()->query_race_name();
*/

/*    
    string text;
    
    if (osobno && !query_prop(LIVE_I_NO_GENDER_DESC))
        text = LD_HUM_GENDER_MAP[query_gender()][przyp] + " ";
    else
        text = "";
        
    if (sizeof(rasy) == 6)
        return text + rasy[przyp];
        
        
    return text + this_object()->query_race_name();
*/
}

/*
 * Nazwa funkcji : query_prasa
 * Opis          : Zwraca nazwe rasy livinga, w liczbie mnogiej,
 *		   w podanym przypadku. 
 * Argumenty     : int - przypadek gramatyczny
 * Funkcja zwraca: Patrz opis.
 */
varargs public string
query_prasa(int przyp)
{
    string text;
    
    if (sizeof(prasy) == 6)
        return prasy[przyp];
        
    return 0;
}

public int
query_osobno()
{
    return osobno;
}

/*
 * Nazwa funkcji : query_rasy
 * Opis          : Zwraca tablice odmiany rasy w liczbie pojedynczej, 
 *		   dokladnie w takiej postaci, w jakiej zostala ona ustawiona.
 * Funkcja zwraca: Tablica stringow, z odmiana rasy
 */
public string
*query_rasy()
{
    return rasy + ({});
}

/*
 * Nazwa funkcji : query_rasy
 * Opis          : Zwraca tablice odmiany rasy w liczbie mnogiej, 
 *		   dokladnie w takiej postaci, w jakiej zostala ona ustawiona.
 * Funkcja zwraca: Tablica stringow, z odmiana rasy
 */
public string
*query_prasy()
{
    return prasy + ({});
}

static void
ustaw_rodzaj_rasy(int rodzaj)
{
    rodzaj_rasy = rodzaj;
}


/*
 * Nazwa funkcji : query_rodzaj_rasy
 * Opis          : Zwraca rodzaj gramatyczny nazwy rasy obiektu.
 * Funkcja zwraca: Patrzy wyzej.
 */
public int
query_rodzaj_rasy()
{
    return rodzaj_rasy;
}

public int
query_rodzaj()
{
    int a;
    
    if (!(a = query_rodzaj_shorta()))
        return rodzaj_rasy;
        
    if (a < 0)
        a = -a;
        
    return (a - 1);
}

/* 
 * Moving messages 
 */

/*
 * Function name:   set_m_in
 * Description:     Set the normal entrance message of this living. You 
 *                  should end the string with a "."
 *                  E.g.: "waddles into the room."
 * Arguments:       m: The message string
 */
public void
set_m_in(string m)
{
    m_in = implode(explode(m, "   "), " ");
}

/*
 * Function name:   query_m_in
 * Description:     Gives the normal entrance message of this living.
 * Returns:         The message string
 */
public string
query_m_in()
{
    return m_in;
}

/*
 * Function name:   set_m_out
 * Description:     Set the normal exit message of this living. Remember
 *                  that the direction is appended to this string, so do
 *                  not end the string with a "."
 *                  E.g.: "waddles"
 * Arguments:       m: The message string
 */
public void
set_m_out(string m)
{
    m_out = implode(explode(m, "   "), " ");
}

/*
 * Function name:   query_m_out
 * Description:     Gives the normal exit message of this living.
 * Returns:         The message string
 */
public string
query_m_out()
{
    return m_out;
}

/*
 * Function name:   set_mm_in
 * Description:     Set the magical entrance message of this living. You 
 *                  should end the string with a "."
 *                  E.g.: "falls out of the sky with his mouth full of spam."
 * Arguments:       m: The message string
 */
public void
set_mm_in(string m)
{
    mm_in = implode(explode(m, "  "), " ");
}

/*
 * Function name:   query_mm_in
 * Description:     Gives the magical entrance message of this living.
 * Returns:         The message string
 */
public string
query_mm_in()
{
    return mm_in;
}

/*
 * Function name:   set_mm_out
 * Description:     Set the magical exit message of this living. You should
 *                  end the string with a "."
 *                  E.g.: "disappears in a puff of smoke."
 * Arguments:       m: The message string
 */
public void
set_mm_out(string m)
{
    mm_out = implode(explode(m, "  "), " ");
}

/*
 * Function name:   query_mm_out
 * Description:     Gives the magical exit message of this living.
 * Returns:         The message string
 */
public string
query_mm_out()
{
    return mm_out;
}

/*
 * Function name: query_wiz_level
 * Description  : Gives the wizard level of the living. This function is
 *                kept here since there are various calls to it from the
 *                living object. The real function is moved to the player
 *                object.
 *
 *                WARNING! This function is not nomasked! People can
 *                redefine this and make themselves appear to be of high
 *                level. In case you need to be certain of the level of
 *                the person, call the following function.
 *
 *                SECURITY->query_wiz_rank(string name);
 *
 * Returns      : int - always 0.
 */
public int
query_wiz_level()
{
    return 0;
}

/*
 * Function name: set_race_name
 * Description  : Sets the race name of this living. // The race name will also
 *                be set as add_name too. //
 * Arguments    : string str - the race string.
 */
public void
set_race_name(string str)
{
/*
    if (id(race_name))
    {
	remove_name(race_name);
    }
*/
	
    race_name = str;
//    add_name(race_name);
}

/*
 * Function name: query_race_name
 * Description  : Gives the race (species) name of this living. This may
 *                be set with set_race_name(). For players, the value
 *                returned for query_race() will always return one of the
 *                default races defined by the mud. For NPC's it is the same
 *                as query_race().
 * Returns      : string - the race name.
 */
public string
query_race_name()
{
    return race_name;
}

/*
 * Function name: query_race
 * Description  : If you define different player objects for different
 *                races you should mask this function in those objects to
 *                always return the true race of the living even though
 *                query_race_name gives the current race of the living.
 *                You should nomask the redefinition of this function.
 * Returns      : string - the race name.
 */
public string
query_race()
{
    return race_name;
}

/*
 * Function name:   query_npc
 * Description:     Checks whether the living is a non-player character
 * Returns:         True if non-player character
 */
public int
query_npc()
{
    return 1;
}

/*
 * Function name:   set_title
 * Description:     Sets the title of a living to something else.
 * Arguments:       t: The new title string
 */
public void
set_title(string t)
{
    title = t;
}

#ifndef NO_ALIGN_TITLE
/*
 * Function name:   set_al_title
 * Description:     Sets the alignment title of a living to something else
 * Arguments:       t: The new alignment title string
 */
public void
set_al_title(string t)
{
#ifdef LOG_AL_TITLE
    if (this_player() != this_object() && interactive(this_object()))
	SECURITY->log_syslog(LOG_AL_TITLE, ctime(time()) + " " +
	    query_real_name() + " new title " + t + " by " +
	    this_player()->query_real_name() + "\n");
#endif
    al_title = t;
}
#endif

/*
 * Function name:   query_title
 * Description:     Gives the title of a living.
 * Returns:         The title string
 */
public nomask string
query_title()
{
    string dom, name, *titles = ({ });
    int    family_name = 0;

    if (query_wiz_level())
    {
	if (!strlen(title))
	{
	    title = "";
	}

	name = query_real_name();
	dom = SECURITY->query_wiz_dom(name);

	/* Madwands get a special madwand-title. */
	if (SECURITY->query_domain_madwand(dom) == name)
        {
	    return LD_MADWAND_TITLE(title, dom);
        }

        return title;
    }

    /* This MUST be with this_object()-> or it will not work for we are
     * accessing the function in the shadows of the player!
     */
    if (strlen(name = this_object()->query_guild_title_race()))
    {
	titles += ({ name });

	/* If the player is in a racial guild that gives him a family name,
	 * we do not add the article before the race-title, but we add it
	 * after the title.
	 */
	family_name = this_object()->query_guild_family_name();
    }
    if (strlen(name = this_object()->query_guild_title_occ()))
    {
	titles += ({ name });
    }
    if (strlen(name = this_object()->query_guild_title_lay()))
    {
	titles += ({ name });
    }

    /* An NPC may have guild-titles and set titles.
     */
    if (query_npc())
    {
	if (!strlen(title))
	{
	    title = "";
	}

	if (!sizeof(titles))
	{
	    return title;
	}

        if (strlen(title))
            titles += ({ title });
    }

    /* A mortal player cannot have a title set by a wizard!
     */
    if (!sizeof(titles))
    {
        return "";
    }

    /* If the player has a family name, we add the article after the family
     * name, else we add it before the possible racial title.
     */
#if 0     
    if (family_name)
    {
	if (sizeof(titles) == 1)
	{
	    return titles[0];
	}
	else
	{
	    return titles[0] + ", " + LD_THE + " "
              + COMPOSITE_WORDS(titles[1..]);
	}
    }
    else
    {
	return LD_THE + " " + COMPOSITE_WORDS(titles);
    }
#endif

    return implode(titles, ", "); //COMPOSITE_WORDS(titles);
}

#ifndef NO_ALIGN_TITLE
/*
 * Function name:   query_al_title
 * Description:     Gives the alignment title of a living
 * Returns:         The alignment title string
 */
public string
query_al_title()
{
    return al_title;
}
#endif

/*
 * Function name: add_textgiver
 * Description:   Add a filename of an object that gives skill/stat
 *                descriptions to the living.
 * Arguments:     obfile: the filename string
 */
public void
add_textgiver(string obfile)
{
    if (member_array(obfile, textgivers) < 0)
    {
	if (!sizeof(textgivers))
	    textgivers = ({ obfile });
	else
	    textgivers += ({ obfile });
    }
}

/*
 * Function name:   query_textgivers
 * Description:     Gives an array of filenames that give skill/stat
 *                  descriptions to the living
 * Returns:         The array with filenames
 */
public string *
query_textgivers()
{
    return (sizeof(textgivers) ? textgivers + ({}) : ({}));
}

/*
 * Function name:   remove_textgiver
 * Description:     Remove a filename of an object that gives skill/stat
 *                  from the living.
 * Arguments:       obfile: The filename of the object to remove.
 * Returns:         True if succesfully removed.
 */
public int
remove_textgiver(string obfile)
{
    int pos;

    if ((pos = member_array(obfile, textgivers)) >= 0)
    {
	textgivers = exclude_array(textgivers, pos, pos);
	return 1;
    }
    return 0;
}

void
calculate_hp()
{
    int n, con, intox;
    int tmpcon, tmpintox;

    n = (time() - hp_time) / F_INTERVAL_BETWEEN_HP_HEALING;
    if (n > 0)
    {
        con = query_stat(SS_CON);
        intox = query_intoxicated();
        tmpcon = (con + last_con) / 2;
        tmpintox = (intox + last_intox) / 2;
        set_hp(hit_points + n * F_HEAL_FORMULA(tmpcon, tmpintox));
        last_con = con;
        last_intox = intox;
        hp_time += n * F_INTERVAL_BETWEEN_HP_HEALING;
    }
}

/*
 * Function name:   query_hp
 * Description:     Gives the number of hitpoint left for the living
 * Returns:         The number
 */
public int
query_hp()
{
    this_object()->calculate_hp();
    return hit_points;
}

/*
 * Function name:   query_max_hp
 * Description:     Calculates the maximum number of hitpoints that the
 *                  living can achieve.
 * Returns:         The maximum
 */
public int
query_max_hp()
{
    return F_MAX_HP(query_stat(SS_CON));
}

/*
 * Function name:   set_hp
 * Description:     Sets the number of hitpoints of a living. The number
 *                  can not exceed the maximum calculated by query_max_hp.
 * Arguments:       hp: The new number of hitpoints
 */
void
set_hp(int hp)
{
    int max;

    hit_points = hp;

    if (hit_points < 0)
    {
	hit_points = 0;
	return;
    }

    if (hit_points > (max = query_max_hp()))
    {
	hit_points = max;
    }
}

/*
 * Function name:   heal_hp
 * Description:     Increase the number of hitpoints with a few.
 * Arguments:       hp: The difference
 */
void
heal_hp(int hp)
{
    object o;
    int hit_points;

    hit_points = query_hp(); /* Call the function to make sure we update.*/

#ifdef LOG_REDUCE_HP
    if (!query_npc() && (hp < 0) && (-hp > hit_points))
    {
	SECURITY->log_syslog(LOG_REDUCE_HP, sprintf("%s %d->%d by ",
	    query_real_name(), hit_points, hp));

	if (!this_interactive())
	    SECURITY->log_syslog(LOG_REDUCE_HP, "?");
	else
	    SECURITY->log_syslog(LOG_REDUCE_HP,
		this_interactive()->query_real_name());

	o = previous_object();

	if (o)
	    SECURITY->log_syslog(LOG_REDUCE_HP, " " + file_name(o) + ", " +
		o->short() + " (" + getuid(o) + ")\n");
	else
	    SECURITY->log_syslog(LOG_REDUCE_HP, " ??\n");
    }	
#endif
    set_hp(hit_points + hp);

}

/*
 * Function name:   reduce_hit_point
 * Description:     Reduce the number of hitpoints with a few.
 * Arguments:       dam: The number of damage hitpoints.
 */
public int
reduce_hit_point(int dam) 
{
    heal_hp(-dam); 
}

/*
 * Function name:   query_max_mana
 * Description:     Calculates that maximum of mana points that a living
 *                  can get.
 * Returns:         The maximum.
 */
public int
query_max_mana()
{
    return query_stat(SS_INT) * 10;
}

/*
 * Function name:   set_mana
 * Description:     Set the number of mana points that a player has. Mana
 *                  points are more commonly known as spellpoints. The
 *                  mana points can not bet set to more than the amount
 *                  that is calculated by query_max_mana.
 * Arguments:       sp: The new amount of mana points.
 */
void
set_mana(int sp)
{
    int max;
    mana = sp;

    if (mana < 0)
    {
	mana = 0;
	return;
    }

    if (mana > (max = query_max_mana()))
    {
	mana = max;
    }
}

/*
 * Function name:   add_mana
 * Description:     Add a certain amount of mana points
 * Arguments:       sp: The number of mana points to change.
 */
void
add_mana(int sp)
{
    set_mana(mana + sp);
}

/*
 * Function name:   query_mana
 * Description:     Gives the number of mana points that the living has
 * Returns:         The number of mana points.
 */
public int
query_mana()
{
    int n;
    int intel;
    int sc;
    int pintox;

    n = (time() - mana_time) / F_INTERVAL_BETWEEN_MANA_HEALING;
    if (n > 0)
    {
	intel = query_stat(SS_INT);
	if ((pintox = query_headache()) > 0)
	    pintox = pintox * 100 / query_prop(LIVE_I_MAX_INTOX);
	else
	{
            pintox = query_intoxicated();
	    pintox = ((((pintox < 0) ? 0 : pintox) * 100) / query_prop(LIVE_I_MAX_INTOX));
	}
	sc = query_skill(SS_SPELLCRAFT);
        set_mana(mana + n * F_MANA_HEAL_FORMULA(sc, pintox, intel));
        mana_time += n * F_INTERVAL_BETWEEN_MANA_HEALING;
    }
    return mana;
}

/*
 * Function name:   query_max_fatigue
 * Description:     Calculates the maximum number of fatigue points that
 *                  the living can have.
 * Returns:         The maximum.
 */
public int
query_max_fatigue()
{
    return query_stat(SS_CON) + 50;
}

/*
 * Function name:   add_fatigue
 * Description:     Add an amount of fatigue points to the current amount
 *                  of the living. Observe, negative argument makes a player
 *		    more tired.
 * Arguments:       f: the amount of change
 */
public void
add_fatigue(int f)
{
    fatigue += f;
    if (fatigue < 0)
	fatigue = 0;
    if (fatigue > query_max_fatigue())
	fatigue = query_max_fatigue();
}

void
calculate_fatigue()
{
    int n, stuffed, tmpstuffed;

    n = (time() - fatigue_time) / F_INTERVAL_BETWEEN_FATIGUE_HEALING;
    if (n > 0)
    {
        if (query_npc())
        {
            add_fatigue(n * F_NPC_FATIGUE_HEAL);
        }
        else
        {
            stuffed = query_stuffed();
            tmpstuffed = (stuffed + last_stuffed) / 2;
            add_fatigue(n *
                F_FATIGUE_FORMULA(tmpstuffed, query_prop(LIVE_I_MAX_EAT)));
            last_stuffed = stuffed;
        }
        fatigue_time += n * F_INTERVAL_BETWEEN_FATIGUE_HEALING;
    } 

}

/*
 * Function name:   set_fatigue
 * Description:     Set the fatigue points of the living to a certain amount.
 * Arguments:       f: The amount to set.
 */
public void
set_fatigue(int f)
{
    fatigue = 0;
    add_fatigue(f);
}

/*
 * Function name:   query_fatigue
 * Description:     Gives the amount of fatigue points of a living
 * Returns:         The number of fatigue points
 */
public int
query_fatigue()
{
    this_object()->calculate_fatigue();
    return fatigue;
}

/*
 * Function name: refresh_living()
 * Description  : This function is called to give the living full mana,
 *                full hitpoints and full fatigue.
 *                NOTE that this function can only be used for NPC's.
 */
void
refresh_living()
{
    if (!(this_object()->query_npc()))
        return;

    heal_hp(query_max_hp());
    add_mana(query_max_mana());
    add_fatigue(query_max_fatigue());
}

/* 
 * Experience
 */

/* 
 * Function name:   modify_exp
 * Description:     This is a service function, to be used only by the mud
 *                  admins. You should never call it, for any reason
 *                  whatsoever.
 * Arguments:       e: Amount of exp to add, may be negative
 *                  battle: 0 = quest exp (== total exp),
 *			    1 = combat exp and total exp,
 *			    2 = combat exp without changing total exp.
 *                  msg: message stating why the change was made
 * Returns:	    1 upon success; 0 upon failure.
 */
public nomask int
modify_exp(int e, int battle, string msg)
{
    if (!this_interactive())
	return 0;

    if (!msg)
    {
        this_interactive()->catch_msg("You forgot to add a reason." +
            "Type \"sman modify_exp\" for more info.\n");
        return 0;
    }

    if (battle == 0 || battle == 1)
	exp_points += e;

    if (battle == 1 || battle == 2)
	exp_combat += e;

    /* Sanity checks */
    if (exp_points < 0)
        exp_points = 0;
    if (exp_combat < 0)
        exp_combat = 0;
    if (exp_points < exp_combat)
	exp_points = exp_combat;

    /* No need to log exp-change in npc's */
    if (this_object()->query_npc())
        return 0;

    SECURITY->log_syslog("MODIFY_EXP", ctime(time()) + " on " +
	capitalize(query_real_name()) + " " +
        (battle == 0 ? "T  " : (battle == 1 ? "C&T" : "C  ")) + e + " (" +
        sprintf("%8d", exp_points) + "," +
        sprintf("%8d", (exp_points - exp_combat)) + ") by " +
        capitalize(this_interactive()->query_real_name()) + ", " + msg + "\n");

    update_acc_exp();

    return 1;
}

/*
 * Nazwa funkcji : query_brute_exp
 * Opis          : Funkcja zwraca ilosc doswiadczenia bojowego na potrzeby
 *		   obliczania poziomu brutalnosci. Realna ilosc
 *		   doswiadczenia bojowego jest redukowana, tak, by
 *		   doswiadczenie zgromadzone na statach gildiowych
 *		   w ekstremalnych przypadkach nie wplywalo zbytnio
 *		   na zawyzenie poziomu brutalnosci.
 * Funkcja zwraca: int - doswiadczenie bojowe na potrzeby "brute'a".
 */
public int
query_brute_exp()
{
    int eff_exp, tax, normal, ix;

    eff_exp = tax = normal = 0;

    for (ix = 0; ix < SS_NO_EXP_STATS; ix++)
	normal += acc_exp[ix];

    for (; ix < SS_NO_STATS; ix++)
    {
	tax += learn_pref[ix];
	eff_exp += acc_exp[ix];
    }

    eff_exp = min(eff_exp, tax * normal / 100);
    eff_exp += normal;

    return eff_exp;
}

/*
 * Function name:   add_exp
 * Description:     Add a certain amount of experience to the living. If the
 *                  battle-flag is true, use another formula to add the
 *                  experience points than with the flag false.
 * Arguments:       e: The amount of experience
 *                  battle: True if the experience was gained in battle.
 */
public void
add_exp(int e, int battle)
{
    float fact;

    /*
     * Exp tax are gathered in update_acc_exp()
     */

    if (battle && e > 0)
    {
	fact = itof(exp_points - exp_combat) / itof(query_brute_exp() + 1);

#ifdef MAX_EXP_RED_FRIENDLY
	if (exp_points < MAX_EXP_RED_FRIENDLY && fact > MAX_COMB_EXP_RED)
	    fact = MAX_COMB_EXP_RED;
#endif
	
	e = ftoi(itof(e) * fact);
    
	exp_combat += e;
	exp_points += e;
	SECURITY->bookkeep_exp(0, e);
    }
    else if (e < 0)
    {
	e = -e;
	if (exp_combat < e)
	    e = exp_combat;
	
	exp_combat -= e;
	exp_points -= e;
	SECURITY->bookkeep_exp(0, -e);
    }
    else if (!battle && e > 0) /* Nonbattle positive xp, wee! */
    {
	exp_points += (QUEST_FACTOR * e) / 10;
	SECURITY->bookkeep_exp(e, 0);
    }
    
    /*
     * This should never ever occur
     */
    if (exp_points < 0)
	exp_points = 0;

    update_acc_exp();
}

/*
 * Function name:   set_exp
 * Description:     Set the total number of experience points the living
 *                  currently has.
 * Arguments:       e: The total number of experience
 */
static void
set_exp(int e)
{
    exp_points = e;
}

/*
 * Function name:   query_exp
 * Description:     Gives the total amount of experience of the living.
 *                  (Both combat and quest experience)
 * Returns:         The experience points.
 */
public int
query_exp()
{
    return exp_points;
}

/*
 * Function name:   set_exp_combat
 * Description:     Set the amount of combat experience the living has.
 * Arguments:       e: The amount of combat experience
 */
static void
set_exp_combat(int e)
{
    exp_combat = e;
}

/*
 * Function name:   query_exp_combat
 * Description:     Gives the amount of combat experience the living has.
 * Returns:         The combat experience points
 */
public int
query_exp_combat()
{
    return exp_combat;
}

/*
 * Function name:   set_ghost
 * Description:     Change the living into a ghost or change the ghost-status
 *                  of a player.
 * Arguments:       flag: A flag to recognise the ghost-status. If flag is 0,
 *                        make the ghost a living again.
 */
public void
set_ghost(int flag)
{
    int x;

    is_ghost = flag;

    if (flag)
    {
	set_m_in(F_GHOST_MSGIN);
	set_m_out(F_GHOST_MSGOUT);

	dodaj_nazwy(LD_DUCH, allocate(6), PL_MESKI_NOS_ZYW);
    }
    else 
    {
	set_m_in(F_ALIVE_MSGIN);
	set_m_out(F_ALIVE_MSGOUT);

	for (x = 0; x < 6; x++)
	    remove_name(LD_DUCH[x], x);
    }
}

/*
 * Function name:   query_ghost
 * Description:     Return the ghost-status of a living.
 * Returns:         0 if the living is not a ghost, the status otherwise.
 */
public int
query_ghost()
{
    return is_ghost;
}

/* 
 * Invisible
 */

/*
 * Function name:   set_invis
 * Description:     Change the visibility of the living
 * Arguments:       flag: If true turn the living invisible, else make the
 *                        living visible again.
 */
public void
set_invis(int flag)
{
    if (!flag)
	add_prop(OBJ_I_INVIS, 0);
    else if (query_wiz_level())
	add_prop(OBJ_I_INVIS, 100);
    else
	add_prop(OBJ_I_INVIS, flag);
}

/*
 * Function name:   query_invis
 * Description:     Gives back the current visibility of the living
 * Returns:         True if invisible
 */
public int
query_invis()
{
    return this_object()->query_prop(OBJ_I_INVIS);
}

/*
 * Whimpy mode.
 */

/*
 * Function name: set_whimpy
 * Description  : When a living gets too hurt, it might try to run from
 *                the combat it is engaged in. This will happen if the
 *                percentage of hitpoints left is lower than the whimpy
 *                level, ie: (100 * query_hp() / query_max_hp() < flag)
 * Arguments    : int flag - the whimpy level.
 */
public void
set_whimpy(int flag)
{
    is_whimpy = flag;
}

/*
 * Function name: query_whimpy
 * Description  : This function returns the whimpy state of this living.
 *                If the percentage of hitpoints the living has left is
 *                lower than the whimpy level, the player will try to
 *                whimp, ie: (100 * query_hp() / query_max_hp() < level).
 * Returns      : int - the whimpy level.
 */
public int
query_whimpy()
{
    return is_whimpy;
}

/*
 * Alignment
 */

/*
 * Function name: set_alignment
 * Description  : Set the amount of alignment points of the living. There is
 *                a maximum alignment a player can get. There is a Dutch
 *                proverb about trying to be more Roman-Catholic than the
 *                pope himself. We don't need that.
 * Arguments    : int a - the new alignment.
 */
public void
set_alignment(int a)
{
    if (ABS(a) > F_MAX_ABS_ALIGNMENT)
    {
	a = ((a > 0) ? F_MAX_ABS_ALIGNMENT : -F_MAX_ABS_ALIGNMENT);
    }

    alignment = a;

#ifndef NO_ALIGN_TITLE
    if (!query_wiz_level())
    {
	al_title = query_align_text();
    }
#endif NO_ALIGN_TITLE
}

/*
 * Function name:   query_alignment
 * Description:     Gives the current amount of alignment points of a living
 * Returns:         The amount.
 */
public int
query_alignment()
{
    return alignment;
}

/*
 * Function name: adjust_alignment
 * Description  : When a player has solved a quest, his alignment may be
 *                adjusted if the quest is considered good or evil. This
 *                may only be done when the player receives experience and
 *                the quest bit is subsequently being set. When a quest is
 *                considered solvable for all players in the game, ie both
 *                'good' and 'evil' players, no alignment should be given
 *                out.
 * Arguments    : int align - the alignment of the quest. this should be
 *                            a value in the range -1000 .. 1000 and acts
 *                            the same as alignment in combat, though in
 *                            this case, 'good' players should naturally
 *                            receive positive alignment (ie solve good
 *                            quests).
 */
public void
adjust_alignment(int align)
{
    if (ABS(align) > F_MAX_ABS_ALIGNMENT)
    {
	align = ((align > 0) ? F_MAX_ABS_ALIGNMENT : -F_MAX_ABS_ALIGNMENT);
    }

    set_alignment(alignment + F_QUEST_ADJUST_ALIGN(alignment, align));
}

/*
 * Gender
 */

/*
 * Function name:   set_gender
 * Description:     Set the gender code (G_MALE, G_FEMALE or G_NEUTER)
 *		    Ustawia tez rodzaj gramatyczny obiektu w zaleznosci od
 *		    plci.
 * Arguments:       g: The gender code
 */
public void
set_gender(int g)
{
    gender = g;
    
    if (g < 0)
        return ;
        
    if (osobno)
        rodzaj_rasy = (g == G_FEMALE ? PL_ZENSKI : 
            (this_object()->query_humanoid() ? PL_MESKI_OS :
            				       PL_MESKI_NOS_ZYW));

#if 0
    if (this_object()->query_humanoid())
    {
        if (g == G_FEMALE)
        {
            dodaj_nazwy( ({ "kobieta", "kobiety", "kobiecie", "kobiete",
                "kobieta", "kobiecie" }), ({ "kobiety", "kobiet",
                "kobietom", "kobiety", "kobietami", "kobietach" }),
                PL_ZENSKI);
            if (osobno)
                rodzaj_rasy = PL_ZENSKI;
        }
        else
        {
            dodaj_nazwy( ({ "mezczyzna", "mezczyzny", "mezczyznie", 
                "mezczyzne", "mezczyzna", "mezczyznie" }), 
                ({ "mezczyzni", "mezczyzn", "mezczyznom", 
                "mezczyzn", "mezczyznami", "mezczyznach" }), 
                PL_MESKI_OS);
                
            if (osobno)
                rodzaj_rasy = PL_MESKI_OS;
        }
    }
    else
    {
        if (rodzaj_rasy == PL_ZENSKI)
            dodaj_nazwy( ({ "samica", "samicy", "samicy", "samice",
                "samica", "samicy" }), ({ "samice", "samic", 
                "samicom", "samice", "samicami", "samicach" }), PL_ZENSKI);
        else
        {
            dodaj_nazwy( ({ "samiec", "samca", "samcowi", "samca", 
                "samcem", "samcu" }), ({ "samce", "samcow", "samcom",
                "samce", "samcami", "samcach" }), PL_MESKI_NOS_ZYW);
                
            if (osobno)
                rodzaj_rasy = PL_MESKI_NOS_ZYW;
        }
    }
#endif

}

/*
 * Function name:   query_gender
 * Description:     Returns the gender code of the living.
 * Returns:         The code. (0 - male, 1 - female, 2 - netrum)
 */
public int
query_gender()
{
    return gender;
}

/*
 * Function name:   query_headache
 * Description:     Gives the amount of headache of a living.
 *                  Updates the value when queried.
 * Returns:         The amount.
 */
public int
query_headache()
{
    int n;

    n = (time() - headache_time) / F_INTERVAL_BETWEEN_HEADACHE_HEALING;

    if (n == 0)
        return headache;

    if (headache) 
    {
        headache -= F_HEADACHE_RATE * n;
        headache = MAX(0, headache);
        if (headache == 0)
        {
            tell_object(this_object(), LD_GONE_HEADACHE);
            set_max_headache(query_max_headache() / 2);   /* Funny is it not */
        }
    }
    headache_time += n * F_INTERVAL_BETWEEN_HEADACHE_HEALING;
    
    return headache;
}

/*
 * Function name:   query_intoxicated
 * Description:     Gives the level of intoxication of a living.
 * Returns:         The intoxication level.
 */
public int
query_intoxicated()
{
    int n;

    n = (time() - intoxicated_time ) / F_INTERVAL_BETWEEN_INTOX_HEALING;

    if (n == 0)
        return intoxicated;

    if (intoxicated > 0) 
    {
        intoxicated -= n * F_SOBER_RATE;
        intoxicated = MAX(0, intoxicated);
        if (intoxicated == 0)
        {
            headache = query_max_headache();   
            tell_object(this_object(), LD_SUDDEN_HEADACHE);
        }
    }
    intoxicated_time += n * F_INTERVAL_BETWEEN_INTOX_HEALING;
    
    return intoxicated;
}

/*
 * Function name:   query_stuffed
 * Description:     Gives the level of stuffedness of a living.
 * Returns:         The level of stuffedness.
 */
public int
query_stuffed()
{
    int t, n;

    n = (time() - stuffed_time) / F_INTERVAL_BETWEEN_STUFFED_HEALING;

    if (n == 0)
        return stuffed;

    stuffed -= F_UNSTUFF_RATE * n;
    stuffed = MAX(0, stuffed);

    stuffed_time += n * F_INTERVAL_BETWEEN_STUFFED_HEALING;
    
    return stuffed;
}

/*
 * Function name:   query_soaked
 * Description:     Gives the level of soakedness of  a living.
 * Returns:         The level of soakedness.
 */
public int
query_soaked()
{
    int n;

    n = (time() - soaked_time) / F_INTERVAL_BETWEEN_SOAKED_HEALING;

    if (n == 0)
        return soaked;

    soaked -= F_UNSOAK_RATE * n;
    soaked = MAX(0, soaked);

    soaked_time += n * F_INTERVAL_BETWEEN_SOAKED_HEALING;
    
    return soaked;
}

/*
 * Function name:   set_intoxicated
 * Description:     Set the level of intoxication of a living.
 * Arguments:       i: The level of intoxication.
 */
static void
set_intoxicated(int i)
{
    this_object()->calculate_hp();
    intoxicated = (i < 0 ? 0 : i);
}

/*
 * Function name:   set_headache
 * Description:     Set the level of headache of a living
 * Arguments:       i: The level of headache
 */
public void
set_headache(int i)
{
    if (i > query_prop(LIVE_I_MAX_INTOX))
         i = query_prop(LIVE_I_MAX_INTOX);
         
    headache = i;
}

/*
 * Function name:   set_stuffed
 * Description:     Set the level of stuffedness of a living
 * Arguments:       i: The level of stuffedness
 */
static void
set_stuffed(int i)
{
    this_object()->calculate_fatigue();
    stuffed = i;
}

/*
 * Function name:   set_soaked
 * Description:     Set the level of soakedness of a living
 * Arguments:       i: The level of soakedness
 */
static void
set_soaked(int i)
{
    soaked = i;
}

/*
 * Learning preferences on stats
 */

/*
 * Function name:   set_learn_pref
 * Description:     Calculate learning preferences summing up to 100
 *                  from an array containing arbitrary numbers. 
 * Arguments:       pref_arr: An array with relative preference settings
 */
void
set_learn_pref(int *pref_arr)
{
    int sum;
    int i;
    int mval;
    int tmp;

    mval = 100;
    
    if (sizeof(pref_arr) < SS_NO_EXP_STATS)
    {
	pref_arr = allocate(SS_NO_STATS);
    }
    
    /* Take away the tax */
    for(i = SS_NO_EXP_STATS; i < SS_NO_STATS; i++)
    {
	mval -= learn_pref[i];
    }

    if (mval < 1)
    {
	for(i = 0; i < SS_NO_EXP_STATS; i++)
	{
	    learn_pref[i] = 0;
	}
	return;
    }

    for(i = 0, sum = 0; i < SS_NO_EXP_STATS; i++)
    {
	sum += pref_arr[i];
    }

    if (sum > 0)
    {
	/* Try to avoid some rounding errors using this tmp */
	tmp = sum / 2;
	for (i = 0; i < SS_NO_EXP_STATS; i++)
	{
	    learn_pref[i] = (mval * pref_arr[i] + tmp) / sum;
	}
    }
    else
    {
	tmp = mval / SS_NO_EXP_STATS;
	for (i = 0; i < SS_NO_EXP_STATS; i++)
	{
	    learn_pref[i] = tmp;
	}
    }
	
    for(i = 0, sum = 0; i < SS_NO_EXP_STATS; i++)
    {
	sum += learn_pref[i];
    }
    
    sum = mval - sum;
    i = 0;

    if (sum > 0)
    {
	while (sum--)
        {
            learn_pref[i++]++;
	    if (i >= SS_NO_EXP_STATS)
                i = 0;
        }
    }
    else if (sum < 0)
    {
	while (sum++)
        {
            if (learn_pref[i] > 0)
                learn_pref[i]--;
            if (++i >= SS_NO_EXP_STATS)
                i = 0;
        }
    }
}

/*
 * Function name:   query_learn_pref
 * Description:     Return one or all values of the learn preferences, this
 *		    includes the 'guildtax' learn_prefs of race, occup, layman
 * Arguments:       Index of learn_pref or -1
 * Returns:         learn_pref value if arg >= 0, 
 *                  entire learn_pref array if arg < 0
 */
public mixed
query_learn_pref(int stat)
{
    if (stat < 0) return learn_pref;

    if (stat < 0 || stat >= SS_NO_STATS)
	return -1;

    return learn_pref[stat];
}

/*
 * Function name:   set_guild_pref
 * Description:     Sets the guild tax/learn_pref for a specific guild.
 * Arguments:       guildstat: SS_RACE / SS_OCCUP / SS_LAYMAN
 *		    tax: taxrate for guild (in %)
 */
public void
set_guild_pref(int guildstat, int tax)
{
    if (guildstat >= SS_NO_EXP_STATS && 
	guildstat < SS_NO_STATS &&
	tax >= 0)
    {
	learn_pref[guildstat] = tax;
	set_learn_pref(query_learn_pref(-1));
    }
}

/*
 * Function name:   set_acc_exp
 * Description:     Set the accumulated experience for each of the stats
 * Arguments:       stat: The stat to set
 *                  val:  The amount of experience to set the stat to
 * Returns:         0
 */
static int
set_acc_exp(int stat, int val)
{
    if (stat < 0 || stat >= SS_NO_STATS || val < 0)
	return 0;

    acc_exp[stat] = val;
}  

/*
 * Function name:   query_acc_exp
 * Description:     Get the accumulated experience points for a given stat.
 * Arguments:       stat: The stat to check
 * Returns:         The amount of experience belonging to the stat.
 */
public int
query_acc_exp(int stat)
{
    if (stat < 0 || stat >= SS_NO_STATS)
	return -1;

    return acc_exp[stat];
}

/*
 * Function name: set_skill
 * Description  : Set a specific skill to a specific value.
 * Arguments    : int skill - the skill-number to set.
 *                int val   - the value to set the skill to.
 * Returns      : int 1/0   - true if successfull, else 0.
 */
public int
set_skill(int skill, int val)
{
    if (!mappingp(skillmap))
	skillmap = ([]);

    if (!intp(skill))
	return 0;

#ifdef LOG_SET_SKILL
    if (interactive(this_object()) &&
	this_interactive() != this_object() &&
	(previous_object() != this_object() ||
	 calling_function() != "decay_skills") &&
	!(SECURITY->query_wiz_rank(this_object()->query_real_name())) &&
	!wildmatch("*jr", this_object()->query_real_name()))
    {
	SECURITY->log_syslog(LOG_SET_SKILL,
	    sprintf("%s %-11s: %6d %3d -> %3d by %s\n", ctime(time()),
		capitalize(query_real_name()), skill, skillmap[skill], val,
		(objectp(this_interactive()) ?
		    capitalize(this_interactive()->query_real_name()) :
		    MASTER_OB(previous_object()))));
    }
#endif LOG_SET_SKILL

    skillmap[skill] = val;
    return 1;
}

/*
 * Function name: set_skill_extra
 * Description:   This is the function to call if you have an object that wants
 * 		  to temporarily change the skill of someone. Perhaps some
 *		  grappling hooks to help climbing skill, or a fumble spell to
 *		  lower fighting skills?
 * Arguments:	  skill - Number of skill
 *		  val   - The new val of the extra variable
 */
public void
set_skill_extra(int skill, int val)
{
    if (val == 0)
    {
	skill_extra_map = m_delete(skill_extra_map, skill);
	return;
    }

    skill_extra_map[skill] = val;
}

/*
 * Function name: query_skill_extra
 * Description:	  Query how much extra skill (or less) someone has in a
 *                particular skill.
 * Arguments:     skill - What skill to query
 * Returns:	  The extra modifying value
 */
public int
query_skill_extra(int skill)
{
    return skill_extra_map[skill];
}

/*
 * Function name:   remove_skill
 * Description:     Remove a specific skill from a player
 * Arguments:       skill: The skill number to remove
 */
void
remove_skill(int skill)
{
    if (mappingp(skillmap))
    {
#ifdef LOG_SET_SKILL
	if (interactive(this_object()) &&
	    (this_interactive() != this_object()))
	{
	    SECURITY->log_syslog(LOG_SET_SKILL,
		sprintf("%s %-11s: %6d %3d ->    0 by %s\n", ctime(time()),
		    capitalize(query_real_name()), skill, skillmap[skill],
		    (objectp(this_interactive()) ?
			capitalize(this_interactive()->query_real_name()) :
			MASTER_OB(previous_object()))));
	}
#endif LOG_SET_SKILL

	skillmap = m_delete(skillmap, skill);
    }
}

/*
 * Function name:   query_base_skill
 * Description:	    Gives the value of a specific skill. If there is need to
 *		    know the true skill of the player call this since it is
 *		    unshadowable.
 * Arguments:       skill: The number of the skill to check
 * Returns:	    The true value of the skill
 */
nomask int
query_base_skill(int skill)
{
    if (!mappingp(skillmap))
	return 0;

    return skillmap[skill];
}

/*
 * Function name:   query_skill
 * Description:     Gives the value of a specific skill.
 * Arguments:       skill: The number of the skill to check
 * Returns:         The value of the skill
 */
public int
query_skill(int skill)
{
    if (!mappingp(skillmap))
	return 0;

    return skillmap[skill] + skill_extra_map[skill];
}

/*
 * Function name:   query_all_skill_types
 * Description:     Gives list of all current skills != 0
 * Returns:         an array with all skill-values
 */
public int *
query_all_skill_types()
{
    if (!mappingp(skillmap))
	return 0;
    return m_indexes(skillmap);
}

/*
 * Function name:   query_skill_descriptor
 * Description:     Gives the path to an object which defines the function
 *                  sk_rank, which assigns names to the skill levels.
 * Returns:         The path
 */
public string
query_skill_descriptor()
{
    return "/lib/skill_raise";
}

/*
 * Function name:
 * Description:
 * Arguments:
 * Returns:
 */

 /*************************************************************************
 * 
 * Command soul routines.
 *
 */

/* 
 * Function name: valid_change_soul
 * Description  : This function checks whether the soul of a wizard may
 *                may be added or removed.
 * Returns      : 1/0; 1 = change allowed.
 */
nomask static int
valid_change_soul()
{
    object wizard;

    /* May always alter the soul of a mortal player. */
    if (!query_wiz_level())
	return 1;

    /* You may alter your own souls */
    if (geteuid(previous_object()) == geteuid(this_object()))
        return 1;

    /* Root may change everyones souls */
    if (geteuid(previous_object()) == ROOT_UID)
        return 1;

    if (!objectp(wizard = this_interactive()))
        return 0;

    if (wizard != this_player())
        return 0;

    /* You may change someones soul if you are allowed to snoop him. This
     * means Lords change their members, arch changed everyone < arch and
     * you may use snoop sanction to allow someone to patch your souls.
     */
    if (SECURITY->valid_snoop(wizard, wizard, this_object()))
	return 1;

    return 0;
}

/*
 * Function name:   add_cmdsoul
 * Description:	    Add a command soul to the list of command souls. Note
 *                  that adding a soul is not enough to get the actions
 *                  added as well. You should do player->update_hooks()
 *                  to accomplish that.
 * Arguments:       soul: String with the filename of the command soul.
 * Returns:         1 if successfull,
 *                  0 otherwise.
 */
nomask public int
add_cmdsoul(string soul)
{

    if (!valid_change_soul())
        return 0;

    if (!((int)soul->query_cmd_soul()))
	return 0;

    /*
     * There can only be one!
     */
    remove_cmdsoul(soul);

    if (!sizeof(cmdsoul_list))
	cmdsoul_list = ({ soul });
    else
	cmdsoul_list += ({ soul });
    return 1;
}

/*
 * Function name:   remove_cmdsoul
 * Description:	    Remove a command soul from the list.
 * Arguments:       soul: De filename of the soul to remove
 * Returns:         1 if successfull,
 *                  0 otherwise.
 */
nomask public int
remove_cmdsoul(string soul)
{
    int index;

    if (!valid_change_soul())
        return 0;

    if ((index = member_array(soul, cmdsoul_list)) < 0)
	return 0;

    cmdsoul_list = exclude_array(cmdsoul_list, index, index);
    return 1;
}

/*
 * Function name:   update_cmdsoul_list
 * Description:	    Update the list of command souls
 * Arguments:       souls: The new filenames
 */
nomask static void
update_cmdsoul_list(string *souls)
{
    cmdsoul_list = souls;
}

/*
 * Function name:   query_cmdsoul_list
 * Description:	    Give back the array with filenames of command souls.
 * Returns:         The command soul list.
 */
nomask public string *
query_cmdsoul_list()
{
    return secure_var(cmdsoul_list);
}

/*************************************************************************
 * 
 * Tool soul routines.
 *
 */

/*
 * Function name:   add_toolsoul
 * Description:	    Add a tool soul to the list of tool souls. Note that
 *                  adding a soul is not enough to get the actions added
 *                  as well. You should do player->update_hooks() to
 *                  accomplish that.
 * Arguments:       soul: String with the filename of the tool soul.
 * Returns:         1 if successfull,
 *                  0 otherwise.
 */
nomask public int
add_toolsoul(string soul)
{
    if (!((int)SECURITY->query_wiz_level(geteuid(this_object()))))
	return 0;

    if (!((int)soul->query_tool_soul()))
	return 0;

    if (!valid_change_soul())
        return 0;

    /*
     * There can only be one!
     */
    remove_toolsoul(soul);

    if (!sizeof(tool_list))
	tool_list = ({ soul });
    else
	tool_list += ({ soul });
    return 1;
}

/*
 * Function name:   remove_toolsoul
 * Description:	    Remove a tool soul from the list.
 * Arguments:       soul: De filename of the tool soul to remove
 * Returns:         1 if successfull,
 *                  0 otherwise.
 */
nomask public int
remove_toolsoul(string soul)
{
    int index;

    if (!valid_change_soul())
        return 0;

    if ((index = member_array(soul, tool_list)) < 0)
	return 0;

    tool_list = exclude_array(tool_list, index, index);
    return 1;
}

/*
 * Function name:   update_tool_list
 * Description:	    Update the list of tool souls
 * Arguments:       souls: The new filenames
 */
nomask static void
update_tool_list(string *souls)
{
    tool_list = souls;
}

/*
 * Function name:   query_tool_list
 * Description:	    Give back the array with filenames of tool souls.
 * Returns:         The tool soul list.
 */
nomask public string *
query_tool_list()
{
    return tool_list ? secure_var(tool_list) : ({});
}
