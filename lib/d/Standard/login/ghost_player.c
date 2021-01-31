/*
 * /d/Genesis/login/ghost_player
 *
 * This player file is used when a player logging in does not have a valid
 * player file.
 *
 *    There can be three different reasons for not having a player_file:
 *
 *       1 - If this is a new character, then manage the creation process.
 *
 *       2 - The players racefile is not loadable, a new body must be
 *           choosen.
 *
 *       3 - The players racefile is not a legal playerfile, a new body
 *           must be choosen.
 */

#pragma strict_types
#pragma save_binary

inherit "/std/player_pub";

#include <std.h>
#include <macros.h>
#include <stdproperties.h>
#include <filepath.h>
#include "login.h"

#define DEATH_BADGE "death_badge"
#define ODMIENIACZ "/d/Standard/obj/odmieniacz.c"

static private int	time_out_alarm;	/* The id of the alarm */
static private string	rhost;		/* The real host */
static private string	junior_wiza;	/* Imie wiza, o ile junior */

/*
 * This is for skill preparation
 */
int skill_coins;

/*
 * Prototypes
 */
public void ghost_start();
void next_query();
void ask_player();
static void check_identity1(string id);

/*
 * Function name: time_out
 * Description  : Remove this object if it has been idle too long.
 */
void
time_out()
{
    write("\nTwoj czas uplynal. Zapraszamy nastepnym razem.\n");
    remove_object();
}

/*
 * Function name:   legal_player
 * Description:     This is called from the master object to check that
 *                  a specific player object is legal. This is used to
 *		    decide if restore_object / save_object in /players
 *		    is possible.
 * Arguments:       ob: The object to check
 * Returns:         True for legal player object
 */
nomask public int
legal_player(object ob)
{
    string m;
    mapping r;

    m =  MASTER_OB(ob);

    if (m == MASTER)
	return 1;

    r = RACEMAP;
    
    if (member_array(m, m_values(r)) >= 0)
	return 1;

    return 0;
}

/*
 * Function name:  enter_new_player
 * Description:    This is the function called from the login object.
 *                 It is responsible for initialising the correct actions
 *		   to be performed.
 */
public nomask void
enter_new_player(string name, string pass)
{
    if (!interactive(this_object()))
    {
	write("Stracilem polaczenie, pa pa!\n");
	remove_object();
	return;
    }

    /*
     * Prevent intrusion into more privileged players
     */
    if (!previous_object() ||
    	(MASTER_OB(previous_object()) != "/secure/login"
	&& MASTER_OB(previous_object()) != MASTER))
    {
	log_file("SECURITY", "enter_new_player() called from " +
	    file_name(previous_object()) + "\n");
	remove_object();
	return;
    }

    set_name(name);
    seteuid(0);
    set_gender(G_MALE);

    rhost = query_ip_name(this_object());

    /*
     * Check if the player exist
     */
    if (!SECURITY->load_player())
    {
	seteuid(getuid(this_object()));
	set_player_file(MASTER);

	/* This can never be a valid password, so we use this to trigger
	 * internal call.
	 */
	if (pass != "")
	{
	    set_password(pass);
	}

	/*
         * name'jr' is a wizard helper if it is not an old player
         */
	if (wildmatch("*jr", name))
	{
	    write("\nA wiec dobrze. Mowisz, ze jestes czarodziejem. " +
	        "Pozwol, ze sprawdze, czy to prawda.\nWpisz imie " +
	        "swojego czarodzieja: ");
	    input_to(check_identity1, 0);
	    time_out_alarm = set_alarm(120.0, 0.0, time_out);
	    return;
	}

	/*
         * Is the mud open to new players ?
         */
	if (file_size(LOGIN_NO_NEW) > 0)   
	{
	    cat(LOGIN_NO_NEW);
	    log_file("REFUSED_ENTRY", ctime(time()) + ": " + name + "\n");
	    remove_object();
	    return;
	}

	write("Tworzymy nowa postac.\n");
	log_file("CREATE_PLAYER", ctime(time()) + " " + query_name() +
	    " (" + query_ip_name(this_object()) + ")\n", -1);
	set_ghost(GP_NEW);
	cat(LOGIN_FILE_NEW_PLAYER_INFO);

	ask_player();  /* Some questions are still needed */
	return;
    }
    seteuid(getuid(this_object()));

    /*
     * Is this a normal CDlib player with a bad playerfile ?
     */
    if (query_player_file() != MASTER)
    {
	set_ghost(GP_BODY | GP_DEAD);
	set_player_file(MASTER);
	write("Masz niewlasciwe cialo i musisz wybrac nowe.\n");
	save_me(0);
    }
    ghost_start();
}

/*
 * Get the password of the old wizard.
 */
static void
check_identity2(string p, string id)
{
    object wiz;

    write("\n");
    remove_alarm(time_out_alarm);

    if (!objectp(wiz = find_player(id)))
    {
	wiz = SECURITY->finger_player(id);
    }

    if (!(wiz->match_password(p)))
    {
	if (wiz->query_finger_player())
	{
	    wiz->remove_object();
	}

	write("Zle haslo!\n");
	remove_object();
	return;
    }
    log_file("HELPER", ctime(time()) + ": " +
	capitalize((wiz->query_real_name())) + " -> " +
	query_name() + "\n", -1);

    write("\nKopiuje email z postaci czarodzieja.\n");
    set_mailaddr(wiz->query_mailaddr());
    if (wiz->query_finger_player())
    {
	wiz->remove_object();
    }
    
    junior_wiza = wiz->query_real_name();

    log_file("CREATE_PLAYER", ctime(time()) + " " + query_name() +
	     " (" + query_ip_name(this_object()) + ")\n", -1);
    set_ghost(GP_NEW);
    
    ask_player();  /* Some questions are still needed */
    
    return ;
}

/*
 * Get the name of the old wizard and check out the identity.
 */
static void
check_identity1(string id)
{
    string name;

    remove_alarm(time_out_alarm);

    if (!strlen(id))
    {
	write("Nie podano zadnego imienia.\n");
	remove_object();
	return;
    }
    
    name = lower_case(id);

    if (!(SECURITY->query_wiz_rank(name)))
    {
	write("Nie ma takiego czarodzieja jak " + capitalize(name) + ".\n");
	remove_object();
	return;
    }

    write("Wpisz haslo swojego czarodzieja: ");
    input_to(&check_identity2(, name), 1);
    time_out_alarm = set_alarm(120.0, 0.0, time_out);
    return;
}

/*****************************************************************
 *
 * The questions to ask an entirely new player, which is not handled
 * in the configuration process.
 *
     Ask for email adress

 */
static string   *new_queries,	/* Kolejne pytania */
		*odmiana,	/* Pytania o przypadki */
		*przypadki;	/* Tablica z odmiana imienia */

/*
 * Function name: ask_player
 * Description:   Ask some questions of new players
 */
static void
ask_player()
{
    odmiana = ({ "q_odmien_dopelniacz", "q_odmien_celownik", 
        "q_odmien_biernik", "q_odmien_narzednik",
        "q_odmien_miejscownik", "q_potwierdz_odmiane" });
    new_queries = ({ "dummy" }) + odmiana + (query_mailaddr() ? ({}) 
                                             : ({ "q_mail" }));
    przypadki = allocate(6);
    przypadki[0] = lower_case(query_name());

    write("\nPodaj jak odmienia sie Twoje imie przez przypadki.\n" +
          "Postaraj sie to prawidlowo wpisac, gdyz moze zajsc sytuacja, gdy "+
          "inny gracz\nnie bedzie mogl wykonac na Tobie jakiejs komendy.\n\n" +
          "Jako mianownik podstawiam Twoje imie.\n");

    next_query();
    return;
}

/*
 * Function name: end_query
 * Description:   
 * Return:        
 */
static void
end_query()
{
    ghost_start();
}

/*
 * Function name: next_query
 * Description:   Asks the next question of the user interactively.
 */
void
next_query()
{
    remove_alarm(time_out_alarm);
    while (1)
    {
	if (sizeof(new_queries) < 2)
	    return end_query();	/* does not return */
	new_queries = slice_array(new_queries, 1, sizeof(new_queries));
	if (call_other(this_object(), new_queries[0] + "_pretext"))
	{
	    time_out_alarm = set_alarm(120.0, 0.0, time_out);
	    input_to(new_queries[0]);
	    return;
	}
    }
}

/*
 * Function name: again_query
 * Description:   Asks the same question again.
 */
static void
again_query()
{
    if (call_other(this_object(), new_queries[0] + "_pretext"))
    {
	input_to(new_queries[0]);
	return;
    }
    next_query();
}


int
q_mail_pretext()
{
    /*
     * Do not ask if there is already an email
     */
    if (query_mailaddr())
	return 0;
    write("\nPodaj swoj email (lub 'nie mam'): ");
    return 1;
}

int
q_odmien_dopelniacz_pretext()
{
    write("Dopelniacz [kogo? czego?]: ");
    return 1;
}

int
q_odmien_celownik_pretext()
{
    write("Celownik [komu? czemu?]: ");
    return 1;
}

int
q_odmien_biernik_pretext()
{
    write("Biernik [kogo? co?]: ");
    return 1;
}

int
q_odmien_narzednik_pretext()
{
    write("Narzednik [kim? czym?]: ");
    return 1;
}

int
q_odmien_miejscownik_pretext()
{
    write("Miejscownik [o kim? o czym?]: ");
    return 1;
}

int 
q_potwierdz_odmiane_pretext()
{
    write("\nOto jak wyglada odmiana Twego imienia:\n");
    write("Mianownik:   " + capitalize(przypadki[0]) + "\n");
    write("Dopelniacz:  " + capitalize(przypadki[1]) + "\n");
    write("Celownik:    " + capitalize(przypadki[2]) + "\n");
    write("Biernik:     " + capitalize(przypadki[3]) + "\n");
    write("Narzednik:   " + capitalize(przypadki[4]) + "\n");
    write("Miejscownik: " + capitalize(przypadki[5]) + "\n");
    write("\nPrzeczytaj swe odpowiedzi jeszcze raz, upewniajac sie, ze sa " +
        "one wlasciwe.\nPosiadanie zle odmienionego imienia jest wbrew " +
        "zasadom. Jesli masz\nwatpliwosci, co do poprawnosci odmiany, " +
        "lepiej zapytaj kogos.\n(najlepiej wogole nie wybierac " +
        "kontrowersyjnych w odmianie imion).\nPrzez pierwsze 6 godzin " +
        "gry bedziesz w stanie poprawic swoja odmiane.\n\n" + 
        "Czy odmiana jest prawidlowa[t/n]: ");

    return 1;
}

int
q_potwierdz_again_pretext()
{
    write("Odpowiedz [t]ak, lub [n]ie: ");
    return 1;
}

/*
 * Function:    q_mail
 * Description: This function is called using input_to, and sets the
 *              email adress of this player.
 */
static void
q_mail(string maddr)
{
    set_mailaddr(maddr);
    next_query();
}

static int
wlasciwe_imie(string str)
{
    int x;
    
    x = strlen(str);
    if (x < 3)
    {
        write("Za krotkie imie.\n");
        return 0;
    }
    
    while (--x >= 0)
        if (str[x] < 'a' || str[x] > 'z')
        {
            write("Imie nie moze zawierac zadnych spacji, apostrofow, " +
                "ani innych znakow specjalnych. Dopuszczalne sa tylko " +
                "male litery, od a do z.\n\n");
            return 0;
        }
    return 1;
}

static void
q_odmien_dopelniacz(string przyp)
{
    if (!wlasciwe_imie(przyp))
    {
        again_query();
        return ;
    }
    przypadki[1] = lower_case(przyp);
    next_query();
}

static void
q_odmien_celownik(string przyp)
{
    if (!wlasciwe_imie(przyp))
    {
        again_query();
        return ;
    }
    przypadki[2] = lower_case(przyp);
    next_query();
}

static void
q_odmien_biernik(string przyp)
{
    if (!wlasciwe_imie(przyp))
    {
        again_query();
        return ;
    }
    przypadki[3] = lower_case(przyp);
    next_query();
}

static void
q_odmien_narzednik(string przyp)
{
    if (!wlasciwe_imie(przyp))
    {
        again_query();
        return ;
    }
    przypadki[4] = lower_case(przyp);
    next_query();
}

static void
q_odmien_miejscownik(string przyp)
{
    if (!wlasciwe_imie(przyp))
    {
        again_query();
        return ;
    }
    przypadki[5] = lower_case(przyp);
    next_query();
}

static void
q_potwierdz_odmiane(string odp)
{
    string o;
    
    o = lower_case(odp[0..0]);
    
    if (o == "n")
    {
        new_queries = odmiana + new_queries[1..];
        again_query();
        return;
    }
    else 
    if (o == "t")
    {
         ustaw_imiona(przypadki);
         next_query();
    }
    else
    {
        new_queries = ({ "q_potwierdz_again" }) + new_queries[1..];
        again_query();
        return;
    }
}

static void
q_potwierdz_again(string odp)
{
    q_potwierdz_odmiane(odp);
    return;
}

/*
 * Here we start the actual configuration routine
 */
public void
ghost_start()
{
    remove_alarm(time_out_alarm);
    if (!query_race_name())
    {
//	set_race_name("newbie");
	save_me(0); /* Only needed if you are a firstimer isn't it? */
    }
    if (this_player() != this_object())
	set_this_player(this_object());
	
    /*
	We now have a correct .o file
    */
//    write("\nEntering the hall of the bodies in waiting...\n\n");
    write("\n\n\n");
    enter_game(query_real_name(), "");
}

void
start_player()
{
    if (!sizeof(this_object()->query_cmdsoul_list()))
    {
	this_object()->add_cmdsoul("/d/Standard/cmd/soul_cmd_ghost");
	this_object()->add_cmdsoul("/d/Standard/cmd/misc_cmd_ghost");
    }
    ::start_player();
}

public string
query_default_start_location()
{
    if (query_ghost() & GP_EMAIL)
	return "/d/Standard/login/email";
    if (query_ghost() & GP_INTRO)
        return "/d/Standard/login/intro";
    else if (query_ghost() & GP_BODY)
	return "/d/Standard/login/sala";
    else if (query_ghost() == 0)
    {
        return "/d/Standard/login/sala";
    }
//	return ::query_default_start_location();

    /*
     * Should not happen
     */
    return "/d/Standard/login/sala";
}

private int
valid_exp_operation()
{
    string cprog;
    
    cprog = calling_program(-1);
    if (( cprog == (PATH + "set_cechy.c")[1..]) ||
        ( cprog == "std/player_sec.c") ||
        ( cprog == "std/living.c"))
        return 1;
    
    log_file("SECURITY", ctime(time()) + ": ghost_player -> operacja expowa przez " +
        "niepowolany\n      obiekt (" + calling_program(-1) + 
        (this_interactive() ? "); interactive: (" + 
         this_interactive()->query_real_name() : "") + ")\n");

    return 0;
}

/*
 * Function name: stats_to_acc_exp
 * Description:   Translates the current base stats into acc_exp. This is used
 *                used when getting stats from a body.
 */
public nomask void
stats_to_acc_exp()
{
    if (valid_exp_operation())
        ::stats_to_acc_exp();
}

public nomask int
set_acc_exp(int stat, int val)
{
    if (valid_exp_operation())
	return ::set_acc_exp(stat, val);
    return 0;
}

public nomask void
acc_exp_to_stats()
{
    if (valid_exp_operation())
	::acc_exp_to_stats();
}

/*
 * Function name: damn_stubborn_object
 * Description  : This function is apparently needed to get rid of objects
 *                that refuse to destruct the first time.
 */
void
damn_stubborn_object()
{
    set_alarm(1.0, 0.0, damn_stubborn_object);
    destruct();
}    

private string
wylosuj_start_location()
{
    string rasa = this_object()->query_race_name();
    string *starty;
    string tmp;

    if (!rasa || !(starty = RACESTART[rasa]))
    {
        set_ghost(query_ghost() | GP_BODY);
        return "/d/Standard/login/sala";
    }
    
    if (wildmatch("*jr", query_real_name()) && junior_wiza)
         tmp = TPATH(junior_wiza, "~/workroom");
    else
        tmp = starty[random(sizeof(starty), time())];
        
    return tmp;
}

/*
 * Now the configuration is ready, and we want to swap to the correct
 * playerfile. 
 */
public int
ghost_ready()
{
    string plfile;
    object ob, badge;
    int wizlev;

    if (query_ghost())
    {
	write("You still have things to do before entering the world.\n");
	return 0;
    }
    plfile = RACEMAP[query_race_name()];
    if (!plfile)
    {
	write("You cannot be a " + query_race_name() + 
	      ", choose a new body!\n");
	set_ghost(GP_BODY | GP_DEAD);
	enter_new_player(query_real_name(), "");
	return 0;
    }
    
    ob = clone_object(plfile);
    if (!ob)
    {
	write(capitalize(query_race_name()) + 
	      " is a faulty race. Choose a new body!\n");
	set_ghost(GP_BODY | GP_DEAD);
	enter_new_player(query_real_name(), "");
	return 0;
    }	
    
    /* 
     * Prepare the new player object. We must change:
     *      We should have the correct souls.
     *      We should have the correct playerfile.
     */
    clone_object(ODMIENIACZ)->move(this_object());
    exec(ob, this_object());
    if (badge = present(DEATH_BADGE, this_object()))
	badge->move(ob, 1);

    set_player_file(plfile);
    this_object()->transmsg_reset();
    set_default_start_location(wylosuj_start_location());
    save_me(0);

    /*
     * Add a property to show that the player has just been created. 
     */
    ob->add_prop("just_created", 1); 

    /*
     * Enter the game and load the save file
     */
    if (!ob->enter_game(query_real_name(), ""))
    {
	write(capitalize(query_race_name()) + 
	      " is an illegal race. Choose a new body!\n");
	set_ghost(GP_BODY | GP_DEAD);
	enter_new_player(query_real_name(), "");
	return 0;
    }

    ob->update_hooks();
    ob->save_me(0);

    /* Notify security of the fact that a new player enters the game.
     * Let the wizards know a new player logged in.
     */
    SECURITY->notify(ob, 0);

    /*
     * Now if we hammer hard enough it probably goes away
     */
    set_alarm(1.0, 0.0, damn_stubborn_object);
    remove_object();
    destruct();
}

/*
 * Function name: 
 * Description:   Called by a player object that is a ghost and needs to
 *		  be reincarnated.
 */
public void
reincarnate_me()
{
    string plfile;
    object nowy, stary;

    setuid();
    seteuid(getuid());
    
    stary = previous_object();
    if (!interactive(stary) || !stary->query_ghost())
	return;
    
    nowy = clone_object(stary->query_player_file());
    
    if (!nowy)
    {
        SECURITY->log_syslog("BAD_PFILE", ctime(time()) + ": '" + 
            stary->query_real_name() + "' ma skopana postac!\n" +
                "\tNie klonuje sie jego playerfile (" + 
                stary->query_player_file() + ").\n");
        stary->catch_msg("Masz nie istniejaca rase! " +
            "Informacja o tym zostala dostarczona administracji Arkadii - " +
            "jak tylko bedzie to mozliwe, twoja postac zostanie " +
            "naprawiona.\n");
        stary->remove_object();
    }
    
    exec(nowy, stary);

    nowy->add_prop("just_created", 1); 
    
    stary->set_ghost(0);
    stary->save_me(0);

    /*
     * Enter the game and load the save file
     */
    if (!nowy->enter_game(stary->query_real_name()))
    {
        SECURITY->log_syslog("BAD_PFILE", ctime(time()) + ": '" + 
            stary->query_real_name() + "' ma skopana postac!\n" +
                "\tenter_game() zwraca 0.\n");
        stary->catch_msg("Nie moge zaladowac twojej postaci! " +
            "Informacja o tym zostala dostarczona administracji Arkadii - " +
            "jak tylko bedzie to mozliwe, twoja postac zostanie " +
            "naprawiona.\n");
        stary->remove_object();
        nowy->remove_object();
    }

    nowy->update_hooks();
    nowy->save_me(0);

    /* Notify security of the fact that a new player enters the game.
     * Let the wizards know a new player logged in.
     */
    SECURITY->notify(nowy, 0);
    
    SECURITY->do_debug("destroy", stary);
    
    
#if 0
    object pl, gh, n, p, ob;

    pl = previous_object();
    if (!interactive(pl) || !pl->query_ghost())
	return;
    pl->set_ghost(GP_BODY | GP_DEAD);
    pl->set_temp_start_location(0);
    pl->save_me(0);

    /* This makes the real hostname carry on to the new player object
    */
    rhost = query_ip_name(pl);

  /* Without this uid/euid setting, this file does not have permission */
  /* to clone a copy of itself */
    setuid();
    seteuid(getuid());

    gh = clone_object(MASTER);
    exec(gh, pl);
    if (ob = present(DEATH_BADGE, pl))
	ob->move(gh, 1);

    gh->set_bit_array(pl->query_bit_array());
    n = pl->query_real_name();
    SECURITY->do_debug("destroy", pl);
    gh->enter_new_player(n);
#endif
}

public string
query_race()
{
    return "duch";
}

/*
 * Set and query the skill coins
 *
 */
public void
set_skill_coins(int c)
{
    skill_coins = c;
}

public int
query_skill_coins()
{
    return skill_coins;
}
