/*
 * /secure/login.c
 *
 * This is the object called from the GameDriver to log people into
 * the Game. This object decides which player object is to be used by
 * the player and swaps the socket to that object.
 */

#pragma no_inherit
#pragma no_shadow
#pragma save_binary
#pragma strict_types

inherit "/std/object";

#include <composite.h>
#include <const.h>
#include <files.h>
#include <login.h>
#include <macros.h>
#include <mail.h>
#include <ss_types.h>
#include <std.h>
#include <stdproperties.h>
#include <time.h>
#include <language.h>
#include <debug.h>

#ifdef Arkadia_test
#define MORTALS_OUT "\nKopia Arkadii na tym porcie sluzy administracji do " \
                  + "celow testowych.\nGracze nie maja tu czego szukac - " \
                  + "sprobuj szczescia na porcie 23.\n\n"
#else
#undef MORTALS_OUT	/* Zdefiniowanie zamyka Arkadie dla smiertelnikow */
#endif

#undef WIZARDS_ONLY
#undef EXCEPT_MAYBE ({ })

/*
 * These are the neccessary variables stored in the save file.
 */
private string	name;            /* The real name of the player        */
private string	password;        /* The password of the player         */
private string 	player_file;     /* The racefile to use for the player */
private mapping m_remember_name; /* The players we have remembered.    */
#ifdef FORCE_PASSWORD_CHANGE
private int     password_time;   /* Time the password was changed      */
#endif FORCE_PASSWORD_CHANGE

#define ATTEMPT_LOG  "attempt"
#define GUEST_LOGIN  "gosc"
#define CLEANUP_TIME 120.0 /* two minutes  */
#define TIMEOUT_TIME 180.0 /* three minutes  */
#define PASS_QUEUE   600   /* ten minutes */
#define ONE_DAY      86400 /* one day in seconds */
#define SIX_MONTHS   15552000 /* 180 days */
#define FOUR_MONTHS  10368000 /* 120 dni */

#define ENTER_ENTER  0 /* notify that someone logged in              */
#define ENTER_REVIVE 3 /* notify that someone revived from linkdeath */
#define ENTER_SWITCH 4 /* notify that someone switched terminals     */

#define FUNNY_NAME_LOG "/d/Standard/log/OFFENSIVE"

/*
 * Global valiables that aren't in the save-file.
 */
static int time_out_alarm; /* The id of the alarm used for timeout.     */
static int login_flag = 0; /* True if the player passed the queue.      */
static int login_type = ENTER_ENTER; /* Login/revive LD/switch terminal */
static int password_set = 0; /* New password set or not.                */
static string old_password; /* The old password of the player.          */

/*
 * Prototypes.
 */
static void check_password(string p);
static void tell_password();
static void try_throw_out(string str);
static void queue(string str);
static void waitfun(string str);
static void get_name(string str);

/* General offensiveness check. The list can be added to as you like. Just
 * do not make it too long. Also, remember that you should use banish for
 * individual names. Please keep the list alphabetized. These strings are
 * parsed by wildmatch.
 */
#define OFFENSIVE ({\
    "*alfons*",	\
    "*burdel*",	\
    "*ciol*",	\
    "*ciot*",	\
    "*cip*",	\
    "*condom*",	\
    "*cwel*",	\
    "*cyc*",	\
    "*czlon*",	\
    "*dup*",	\
    "*dziwk*",	\
    "*fallus*",	\
    "*fekal*",	\
    "*fiuc*",	\
    "*fiut*",	\
    "*gowien*",	\
    "*gown*",	\
    "*holer*",	\
    "*huj*",	\
    "*isior*",	\
    "*jajc*",	\
    "*jeba*",	\
    "*jebi*",	\
    "*jebu*",	\
    "*kondom*",	\
    "kup*",	\
    "*kures*",	\
    "*kurew*",	\
    "*kurw*",	\
    "*kutaf*",	\
    "*kutas*",	\
    "*lesb*",	\
    "*minet*",	\
    "*mocz*",	\
    "*pedal*",	\
    "*penis*",	\
    "*phallus*",\
    "*pierd*",	\
    "*pizd*",	\
    "*praci*",	\
    "*prostyt*",\
    "*qre*",	\
    "*qrv*",	\
    "*qrw*",	\
    "*qta*",	\
    "rzyc*",	\
    "*siusi*",	\
    "*smierd*",	\
    "*smrod*",	\
    "sra*",	\
    "*sran*",	\
    "*stol*",	\
    "*wzwod*",	\
    "ass*",	\
    "*bitch*",	\
    "*butt*",	\
    "*cunt*",	\
    "*dick*",	\
    "*fuck*",	\
    "*shit*",	\
    "*suck*"})

/*
 * Function name: clean_up
 * Description  : This function is called every two minutes and if the
 *                player lost or broke connection, we destruct the object. 
 */
static void
clean_up()
{
    if (!query_ip_number(this_object()))
    {
	remove_object();
    }
    else
    {
	set_alarm(CLEANUP_TIME, 0.0, clean_up);
    }
}

/*
 * Function name: create_object
 * Description  : Called to construct this object.
 */
static void
create_object()
{
    set_name("logon");

    set_alarm(CLEANUP_TIME, 0.0, clean_up);
}

/*
 * Function name: query_pl_name
 * Description  : Return the real name of the player who is trying to log in.
 * Returns      : string - the name.
 */
string
query_pl_name()
{
    return name;
}

/*
 * Function name: query_real_name
 * Description  : Return the real name of this object: "logon"
 * Returns      : string - "logon".
 */
string
query_real_name()
{
    return "logon";
}

/*
 * Function name: time_out
 * Description  : Called when the player takes too much time to type a line.
 *                It destructs the object.
 */
static void
time_out()
{
    write_socket("\nTwoj czas uplynal. Zapraszamy nastepnym razem.\n");

    remove_object();
}

/*
 * Function name: login
 * Description  : This function is called when a player wants to login.
 *                A lot of checks are made.
 * Returns      : int 1/0 - true if login is allowed.
 */
public int
logon()
{
    set_screen_width(80);

    if (!query_ip_number(this_object()))
    {
	remove_object();
	return 0;
    }

    /* No players from this site whatsoever. */
    if (SECURITY->check_newplayer(query_ip_number(this_object())) == 1)
    {
	write_socket("\nMiejsce, z ktorego sie laczysz zostalo calkowicie " +
	    "zablokowane ze wzgledu na powtarzajace sie, naganne " +
	    "zachowanie graczy.\n\n");
	remove_object();
	return 0;
    }

    player_file = 0;

    seteuid(creator(this_object()));
    cat(LOGIN_FILE_WELCOME);

    write_socket("Wersja gamedrivera:  " + SECURITY->do_debug("version") +
	"\t\tWersja mudliba:  " + MUDLIB_VERSION +
	"\n\nPodaj swoje imie: ");

    time_out_alarm = set_alarm(TIMEOUT_TIME, 0.0, time_out);

    input_to(get_name);

    return 1;
}

#ifdef LOCKOUT_START_TIME
/*
 * Function name: is_lockout
 * Description  : This function determines if the game is open to players
 *                now. The mud will be open to wizards and their test
 *                characters above the LOCKOUT_LEVEL.
 * Arguments    : string pl_name - The name of the player attempting to
 *                                 enter the game.
 * Returns      : int - True if the mud is closed to this player, false
 *                      otherwise.
 */
int
is_lockout(string pl_name)
{
    int d, h, ob_type;
    string day, mon, wiz_name;

    /*
     * Determine if we are in the lockout period.
     */
    sscanf(efun::ctime(time()), "%s %s %d %d:%d:%d %d", day, mon, d, h, d, d, d);
    if ((h >= LOCKOUT_START_TIME) && (h < LOCKOUT_END_TIME) &&
        (day != "Sat") && (day != "Sun"))
    {
        ob_type = SECURITY->query_wiz_rank(pl_name);
        if (ob_type >= LOCKOUT_LEVEL)
            return 0;

        if (extract(pl_name, -2) == "jr")
        {
            wiz_name = extract(pl_name, 0, strlen(pl_name) - 3);
            ob_type = SECURITY->query_wiz_rank(wiz_name);
            if (ob_type >= LOCKOUT_LEVEL)
                return 0;
        }

        /* Everyone else is locked out */
        return 1;
    }
    return 0;
}
#endif LOCKOUT_START_TIME

/*
 * Function name: start_player2
 * Description  : Swapsocket to player object and if we are not already
 *                in the game enter it.
 * Arguments    : object ob - the playerobject to swap to.
 */
static void
start_player2(object ob)
{
    object dump;

#ifdef STATUE_WHEN_LINKDEAD
    int old_was_live;
    old_was_live = 0;
#endif STATUE_WHEN_LINKDEAD

    /* If the old socket was already interactive, we must swap them
     * nicely. First tell them what is happening, than clone a new
     * object, swap them out and destruct the old one. We use the
     * LOGIN_NEW_PLAYER since that doesn't leave a 'notify' message when
     * destructed.
     */
    if (query_ip_number(ob))
    {
        if (environment(ob))
            tell_roombb(environment(ob), QCIMIE(ob, PL_MIA) + " odnawia "
                      + "kontakt z rzeczywistoscia.\n", ({ob}), ob);

	tell_object(ob, "Nowe interaktywne polaczenie do twej postaci. "+
	    "Zamykam te sesje.\n");
	dump = clone_object(LOGIN_NEW_PLAYER);
	/* Swap old socket to dummy player. */
	exec(dump, ob);
	dump->remove_object();
#ifdef STATUE_WHEN_LINKDEAD
	old_was_live = 1;
#endif STATUE_WHEN_LINKDEAD
    }

    /* Swap to the playerobject. */
    exec(ob, this_object());

    SECURITY->log_public("ENTER", ctime(time()) + " " + name + " from " +
	query_ip_name(ob) + "\n");

    /* If we are not in the game, enter it. */
    if (!environment(ob))
    {
	if (!(ob->enter_game(name, (password_set ? password : ""))))
	{
	    write_socket("Niewlasciwy plik postaci.\n");
	    ob->remove_object();
	}
    }
#ifdef STATUE_WHEN_LINKDEAD
    else if (!old_was_live)
    {
	ob->revive();
	ob->fixup_screen();
    }
#endif STATUE_WHEN_LINKDEAD
    else
    {
	ob->fixup_screen();
    }
    
    /* Notify the wizards of the action. */
    SECURITY->notify(ob, login_type);

    ob->update_hooks();
    remove_object();
}

/*
 * Function name: start_player1
 * Description  : The next step in the startup process.
 */
static void
start_player1()
{
    object ob;

    /* Now we can enter the game, find the player file */
    if (player_file)
    {
	catch(ob = clone_object(player_file));

	if (!objectp(ob))
	{
	    write_socket("Nie moge odnalezc twego ciala.\n" +
	        "W zwiazku z tym musisz wybrac sobie nowe.\n");
	    player_file = 0;
	}
	else 
	{
	    if (function_exists("enter_game", ob) != PLAYER_SEC_OBJECT)
	    {
		ob->remove_object();
	    }
	}
    }

    /* 
     * There can be three different reasons for not having a player_file:
     * 
     *    1 - If this is a new character, let the login player object
     *        manage the creation / conversion / process.
     *    2 - The players racefile is not loadable, a new body must be
     *        choosen.
     *    3 - The players racefile is not a legal playerfile, a new body
     *        must be choosen.
     */
    if (!player_file ||
	(player_file == LOGIN_NEW_PLAYER))
    {
	/* Only clone if we have not done so yet. */
	if (!objectp(ob))
	{
	    ob = clone_object(LOGIN_NEW_PLAYER);
	}
	ob->open_player(); 

	seteuid(BACKBONE_UID);
	export_uid(ob); 
	ob->set_trusted(1); 
	ob->set_ghost(GP_BODY);
	ob->save_me(0);
	exec(ob, this_object());
	ob->enter_new_player(name, password);
	remove_object();
	return;
    }

    /* Print possible news to the player before we alter his/her euid.
     * Since cat() doesn't seem to work, even when setting this_player to
     * this_object, we have to use this construct to make sure the person
     * gets to read the message.
     */
    write_socket(read_file(LOGIN_FILE_NEWS));

    ob->open_player(); 

    if (SECURITY->query_wiz_level(name))
	seteuid(name);
    else
	seteuid(BACKBONE_UID);

    export_uid(ob); 
    ob->set_trusted(1); 
    start_player2(ob);
}

/*
 * Function name: date
 * Description  : Before people are asked to queue, we give them some
 *                information on the uptime of the game, so they won't
 *                have to wait a long time to get into a game that is
 *                about to reboot.
 */
public void
date()
{
    write_socket("Swiat odrodzil sie  : " + 
        ctime(SECURITY->query_start_time(), 1) +
               "\nLokalny czas        : " + ctime(time(), 1) + 
               "\nSwiat istnieje      : " + CONVTIME(time() - 
			SECURITY->query_start_time()) + 
	       "\n" + SECURITY->query_memory_percentage() + 
			"% swiata zostalo opanowane przez Ciemnosc.\n");
#ifdef REGULAR_REBOOT
    write_socket("Regularny restart: Codziennie po " + REGULAR_REBOOT + ":00\n");
#endif
}

/*
 * Function name: start_player
 * Description  : This function checks for linkdeath and sees whether the
 *                player has to queue. If there are no restrictions, log in
 *                immediately.
 */
static void
start_player()
{
    object other_copy;
    int    pos, konc;

    /* If there is no other copy of the player in the game, we can try to
     * log in immediately if the player doesn't have to queue.
     */
    other_copy = find_player(name);
    if (!objectp(other_copy))
    {
	/* Check enter quota. Don't check if the player already queued, which
         * is signalled by a positive 'login_flag'.
	 */
	if (login_flag ||
	    ((pos = QUEUE->should_queue(name)) == 0))
	{
	    start_player1();
	    return;
	}

	write_socket("\nPrzykro mi, ale gra jest w tej chwili " +
	    "przepelniona.\n\n");
	date();
	write_socket("Stan poczty: " +
	    MAIL_FLAGS[MAIL_CHECKER->query_mail(name)] + ".\n\n");
	    
	write_socket("Czy chcesz sie ustawic w kolejce (na " + 
	    LANG_SORD(pos, PL_MIE, PL_ZENSKI) + " pozycji) ? ");
	login_flag = 1;
	input_to(queue);
	return;
    }

    /* When 'login_flag' is true, this means the player already queued (after
     * having been linkdead. Reconnect instantly.
     */
    if (login_flag)
    {
	login_type = ENTER_REVIVE;
	start_player2(other_copy);
	return;
    }

    /* If you already have a link, you are asked to switch terminals */
    if (query_ip_number(other_copy))
    {
	write_socket("Twoja postac jest juz w grze. Czy chcesz przejac " +
	    "nad nia kontrole? ");
	input_to(try_throw_out);
	return;
    }

    konc = other_copy->koncowka(1, 0);

    /* The player is linkdead, but in combat, reconnect immediately. */
    if (other_copy->query_linkdead_in_combat())
    {
        write_socket("\nByl" + (konc ? "es" : "as") +
            " zaangazowan" + (konc ? "y" : "a") + 
            " w walke, gdy stracil" + (konc ? "es" : "as") +
            " kontakt z mudem.\n" +
            "... przywracam polaczenie ...\n\n");
	login_type = ENTER_REVIVE;
	start_player2(other_copy);
	return;
    }

    /* Player was linkdead for less PASS_QUEUE seconds. */
    if ((time() - other_copy->query_linkdead()) < PASS_QUEUE)
    {
	write_socket("\nStracil" + (konc ? "es" : "as") + " kontakt z "+
	    "mudem na mniej niz dziesiec minut ...\n" +
	    "... przywracam polaczenie ...\n\n");
	login_type = ENTER_REVIVE;
	start_player2(other_copy);
	return;
    }

    write_socket("\nStracil" + (konc ? "es" : "as") + " polaczenie na " +
        CONVTIME(time() - other_copy->query_linkdead()) + ".\n");

    /* No need to queue. Connect instantly. */
    if ((pos = QUEUE->should_queue(name)) == 0)
    {
	login_type = ENTER_REVIVE;
	start_player2(other_copy);
	return;
    }

    write_socket("\nGra jest przepelniona w tej chwili, a ty nie mial" +
       (konc ? "es" : "as") + " polaczenia przez\nwiecej niz 10 minut, " +
       "wiec niestety bedziesz musial" + (konc ? "" : "a") + " odczekac " +
       "swoje\nw kolejce, zanim wejdziesz do gry.\n" +
       "Czy chcesz sie ustawic w kolejce (na " + LANG_SORD(pos, PL_MIE, 
       PL_ZENSKI) + " pozycji.? ");
       
    login_flag = 1;
    input_to(queue);
}

/*
 * Function name: valid_name
 * Description  : Check that a player name is valid. The name must be at
 *                least two characters long and at most eleven characters.
 *                We only allow lowercase letters. Also, generally offensive
 *                names are not allowed.
 * Arguments    : string str - the name to check.
 * Returns      : int 1/0 - true if the name is allowed.
 */
int
valid_name(string str)
{
    int index = -1;
    int length = strlen(str);

    if (length < 3)
    {
	write_socket("\nImie jest za krotkie - musi miec przynajmniej 3 znaki.\n");
	return 0;
    }

    if (length > 11)
    {
	write_socket("\nImie jest za dlugie - moze miec najwyzej 11 znakow.\n");
	return 0;
    }

    while (++index < length)
    {
	if ((str[index] < 'a') ||
	    (str[index] > 'z'))
	{
	    write_socket("\nNiewlasciwy znak w imieniu '" + str + "'.\n");
	    str = sprintf("%" + (index + 1) + "s", "^");
	    write_socket("                            " + str + "\n");
	    write_socket("Dopuszczalne sa jedynie litery (od a do z).\n");
	    return 0;
	}
    }

    return 1;
}

/*
 * Function name: offensive_name
 * Description  : Check whether the name is offensive or not. Note that
 *                this function makes a check for generally offensive parts
 *                only and that you have to use the banish command for
 *                special cases.
 * Arguments    : string str - the name to check.
 * Returns      : int 1/0 - true if the name is offensive.
 */
public int
offensive_name(string str)
{
    int index = -1;
    int size  = sizeof(OFFENSIVE);

    while(++index < size)
    {
	if (wildmatch(OFFENSIVE[index], str))
	{
	    return 1;
	}
    }

    return 0;
}

/*
 * Nazwa funkcji : stupid_name
 * Opis          : Sprawdza, czy w imieniu wystepuja czastki sugerujace,
 *                 ze imie to nie pasuje do polskiego systemu odmiany.
 *                 W razie potrzeby wypisywany jest odpowiedni komunikat.
 * Argumenty     : str - Sprawdzane imie.
 */
public int
stupid_name(string str)
{
    int index = -1;
    int length = strlen(str);
    int char = 0;
    int repetitions = 0;

    while(++index < length)
    {
	if (char == str[index])
	{
	    if (++repetitions == 3)
	    {
        	write_socket("\nTwoje imie zawiera zbyt wiele identycznych "
			   + "liter obok siebie. Byloby\nono niezbyt wygodne "
			   + "dla innych graczy, istnieje bowiem zbyt "
			   + "wielkie\nryzyko popelnienia pomylki podczas "
			   + "jego uzywania.\n\n");
		return 1;
	    }
	}
	else
	{
	    char = str[index];
	    repetitions = 1;
	}
    }

    return 0;
}

static void
new_character(string str)
{
    remove_alarm(time_out_alarm);
    time_out_alarm = set_alarm(TIMEOUT_TIME, 0.0, time_out);

    str = lower_case(str);
    
#ifndef ALWAYS_APPLY
    if (!strlen(str))
    {
	input_to(new_character);
	write_socket("Wymysl imie dla swej postaci: ");
	return;
    }

    if (!valid_name(str))
    {
	input_to(new_character);
	write_socket("Wymysl inne imie: ");
	return;
    }

    if (str == "zakoncz")
    {
	remove_alarm(time_out_alarm);
	remove_object();
	return;
    }

    if (restore_object("/players/" + extract(str, 0, 0) + "/" + str))
    {
	write_socket("\nNiestety, postac o takim imieniu juz istnieje.\n" +
	    "Musisz wymyslec inne imie: ");
	input_to(new_character);
	return;
    }
#endif

#ifdef ALWAYS_APPLY
    if (!wildmatch("*jr", str))
    {
	write_socket("\nW chwili obecnej, aby stworzyc nowa postac w " +
	    "swiecie Arkadii, musisz\nzlozyc podanie. Powinno ono " +
	    "zawierac propozycje imienia, informacje\no tym czy masz juz " +
	    "inne postacie w swiecie Arkadii (jesli tak, jakie),\n" +
	    "oraz krotka informacje o roli jaka chcesz odgrywac " +
	    "swoja postacia\n(o ile masz pojecie o RPG). Istotnym " +
	    "warunkiem jest posiadanie stalego\nadresu email - w przypadku " +
	    "jego braku nie bedziemy w stanie przekazac\nci hasla dla " +
	    "twojej nowej postaci. Aby zlozyc podanie, jako imie " +
	    "przy\nlogowaniu wpisz 'podanie'.\n\n");
#ifndef NO_GUEST_LOGIN
	write_socket("Jesli przed stworzeniem prawdziwej postaci " +
	    "chcesz sie troche rozejrzec\nw swiecie Arkadii, zaloguj " +
	    "sie jako 'gosc'.\n\n");
#endif NO_GUEST_LOGIN
	input_to(get_name);
	write_socket("Wpisz 'podanie' albo sie rozlacz: ");
	return;
    }
#endif ALWAYS_APPLY

    if (!wildmatch("*jr", str) && 
	SECURITY->check_newplayer(query_ip_number(this_object())) == 2)
    {
	write_socket("\nNie dopuszczamy tworzenia nowych postaci z " +
	    "miejsca, z ktorego sie laczysz,\nze wzgledu na " +
	    "powtarzajace sie, naganne zachowanie graczy. Mozesz " +
	    "zlozyc\npodanie o przyznanie nowej postaci - w takim " +
	    "przypadku jako imie podaj 'podanie'.\n");

	input_to(get_name);
	write_socket("Wpisz 'podanie' albo sie rozlacz: ");
	return;
    }

    if (file_size(BANISH_FILE(str)) >= 0)
    {
	write_socket("\nImie '" + capitalize(str) +
	    "' jest zarezerwowane.\nSprobuj wymyslec inne: \n");
	input_to(new_character);
	return;
    }

    if (SECURITY->query_domain_number(capitalize(str)) >= 0)
    {
	write_socket("\nJedna z naszych domen nosi te nazwe.\nSprobuj " +
	    "wymyslec inne imie:\n");
	input_to(new_character);
	return;
    }

    if (offensive_name(str))
    {
	SECURITY->log_public("OFFENSIVE", ctime(time()) + ": " +
	    capitalize(str) + " from " + query_ip_name() + "\n");
	write_socket("\nPodane imie uznane zostalo za obrazliwe. Wymysl " +
	    "cos lepszego, albo\nposzukaj sobie innego muda. Jesli " +
	    "uwazasz, iz nie jest ono jednak brzydkie,\nmozesz zalogowac " +
	    "sie jako 'podanie' i napisac prosbe o przyznanie\npostaci.\n\n");
	input_to(new_character);
	write_socket("Wymysl inne imie: ");
	return;
    }

    if (stupid_name(str))
    {
	input_to(new_character);
	write_socket("Wymysl inne imie: ");
	return;
    }

    /* The new player is an old wizard, that is not remove correctly. */
    if (SECURITY->query_wiz_rank(str))
    {
	write_socket("\nImie to nalezalo niegdys do czarodzieja i nie " +
	    "zostalo uwolnione\nw odpowiedni sposob. Jesli nadal " +
	    "bardzo chcesz uzywac tego imienia,\nmusisz skontaktowac " +
	    "sie z administracja.\n");
#ifndef NO_GUEST_LOGIN
	write_socket("W tym celu mozesz skorzystac z konta goscia.\n");
#endif NO_GUEST_LOGIN
	write_socket("Wymysl inne imie: ");
	input_to(new_character);
	return;
    }

    player_file = 0;
    name = str;

    write_socket("\nTeraz przyszla kolej na ustalenie hasla dla twej " +
	"postaci. Gdy polaczysz sie\nz mudem nastepnym razem, wystarczy " +
	"ze podasz imie postaci i swoje haslo.\nPostaraj sie wiec dobrze " +
	"je zapamietac.\n\n");
    tell_password();
}

/*
 * Function name: get_name
 * Description  : At login time, this function is called with the name the
 *                player intends to use. Some checks are made and when it
 *                is all correct, the player may login.
 * Arguments    : string str - the name the player wants to use.
 */
static void
get_name(string str)
{
    object g_info;
    object a_player;
    object f_player;
    int i, new;
    int runlevel;

    remove_alarm(time_out_alarm);
    time_out_alarm = set_alarm(TIMEOUT_TIME, 0.0, time_out);

    str = lower_case(str);
    
    if (str == "zakoncz")
    {
	remove_alarm(time_out_alarm);
	remove_object();
	return;
    }

    if (!valid_name(str))
    {
	input_to(get_name);
	write_socket("Podaj swoje imie: ");
	return;
    }
    
    if (str == "nowa")
	new = 1;

#ifdef MORTALS_OUT
    if (!(SECURITY->query_wiz_rank(str)) && (new || !wildmatch("*jr", str)))
    {
        write_socket(MORTALS_OUT ?: "\Mud jest obecnie zamknieta dla "
                   + "graczy. Sprobuj ponownie kiedy indziej.\n");
        remove_alarm(time_out_alarm);
        remove_object();
        return;
    }
#endif MORTALS_OUT

    /* If the runlevel is set, the not all players may enter. */
    if (runlevel = SECURITY->query_runlevel())
    {
#ifdef ATTEMPT_LOG
        write_file(ATTEMPT_LOG, ctime(time()) + " " + capitalize(str) + "\n");
#endif ATTEMPT_LOG

        switch(runlevel)
        {
        case WIZ_MAGE..WIZ_KEEPER:
            write_socket("\nW tej chwili mud jest otwarty wylacznie dla " +
                "czlonkow administracji.\n");
            break;

        default:
	    write_socket("\nW tej chwili mud jest otwarty wylacznie dla " +
		"czarodzieji.\n");
        }

        /* Player is not allowed in, but do allow juniors. */
        if ((SECURITY->query_wiz_rank(str) < runlevel) &&
            !(wildmatch("*jr", str) &&
              (SECURITY->query_wiz_rank(extract(str, 0, -3)) >= runlevel)))
        {
            if (file_size(LOGIN_FILE_RUNLEVEL) > 0)
            {
                cat(LOGIN_FILE_RUNLEVEL);
            }

            remove_alarm(time_out_alarm);
            remove_object();
            return;
	}
    }

    /* When Armageddon is active, no players are allowed to connect. */
    if (ARMAGEDDON->shutdown_active())
    {
	/* But 'full' wizards (++) are always allowed access. */
	if (!new && SECURITY->query_wiz_rank(str) >= WIZ_NORMAL)
	{
	    write_socket("\nW tej chwili wlasnie trwa restart Arkadii, ale "+
	        "ty mimo wszystko mozesz sie\nzalogowac.\n");
	}
	else if (new || !find_player(str))
	{
	    write_socket("\nPrzykro mi, ale na Arkadii trwa wlasnie " +
		"Apokalipsa. Zapraszamy za kilka\nminut, kiedy to mud " +
		"bedzie z powrotem.\n\n");
            remove_alarm(time_out_alarm);
            remove_object();
            return;
	}
	else
	{
	    write_socket("\nW tej chwili wlasnie na Arkadii trwa " +
		"Apokalipsa. Mimo wszystko ty mozesz wejsc, gdyz kopia " +
		"twojej postaci juz znajduje sie w grze.\n");
	}
    } 

#ifdef LOCKOUT_START_TIME
    /* Check if the Mud is closed. */
    if (is_lockout(str))
    {
        write_socket("Przykro mi, ale mud jest w tej chwili zamkniety. " +
            "Zapraszamy\npomiedzy " + LOCKOUT_END_TIME + ":00 a " +
            LOCKOUT_START_TIME + ":00.\n" +
            "Lokalny czas: " + ctime(time(), 1) + ".\n");
        remove_alarm(time_out_alarm);
        remove_object();
        return;
    }
#endif LOCKOUT_START_TIME

    if (new)
    {
#ifndef ALWAYS_APPLY
	write_socket("\nWitaj w Arkadii, smiertelniku! Musisz wymyslec imie " +
	    "dla swej postaci.\n\n" +
	    "Mud ten jest swiatem zbudowanym w konwencji fantasy. " +
	    "Oczekujemy, ze imiona\ngraczy beda pasowaly do klimatu " +
	    "naszego swiata. Postacie o niewlasciwych\nimionach powinny sie " +
	    "liczyc z usunieciem.\n\nPamietaj rowniez, ze mud jest " +
	    "stworzony w jezyku _polskim_. Twoje imie\npowinno sie dac w " +
	    "latwy i oczywisty sposob odmienic.\n\nJesli chcesz sie " +
	    "rozlaczyc wpisz 'zakoncz'.\nWymysl imie dla swej postaci: ");
	input_to(new_character);
	return;
#else
	new_character("");
	return;
#endif ALWAYS_APPLY
    }

    if (str == GAMEINFO_LOGIN) /* Does own cleanup */
    {
	g_info = clone_object("/secure/gameinfo_player");
	exec(g_info, this_object());
	g_info->enter_game();
        remove_alarm(time_out_alarm);
	remove_object();
	return;
    }

    if (str == APPLICATION_LOGIN) /* Does own cleanup */
    {
	a_player = clone_object("/secure/application_player");
	exec(a_player, this_object());
	a_player->enter_game();
        remove_alarm(time_out_alarm);
	remove_object();
	return;
    }

#ifdef WIZARDS_ONLY
    if (member_array(query_ip_number(this_object()), WIZARDS_ONLY) != -1)
    {
	if (!SECURITY->query_wiz_rank(str) && !wildmatch("*jr", str) &&
	    member_array(str, EXCEPT_MAYBE) == -1)
	{
	    write_socket("Przykro mi, ale mud nie akceptuje polaczen " +
	        "z tego hosta.\n");
	    remove_alarm(time_out_alarm);
	    remove_object();
	    return;
	}
    }
#endif

    /* Restore the player. If that fails, someone gave nonexistant
     * name, or the playerfile is damaged. We handle all those situations.
     */
    if (!restore_object("/players/" + extract(str, 0, 0) + "/" + str))
    {
	input_to(get_name);
	write_socket("Nie ma postaci o takim imieniu. Aby stworzyc nowa " +
	     "postac, wpisz 'nowa'.\nPodaj swoje imie: ");
	return;
    }
    
    if (name == GUEST_LOGIN)
    {
#ifdef NO_GUEST_LOGIN
	write_socket("Aktualnie na Arkadii nie akceptujemy wejsc poprzez " +
	    "konto goscia.\nMozesz jednak sprobowac stworzyc sobie u nas " +
	    "prawdziwa postac.\n\n");
        remove_alarm(time_out_alarm);
	remove_object();
	return;
#endif NO_GUEST_LOGIN

	write_socket("Witaj gosciu. Nie potrzebujesz hasla...\n" +
	    "... lacze ...\n");

	start_player();
	return;
    }
    
    if (player_file)
    {
        f_player = SECURITY->finger_player(name);
	write_socket("Witaj, " +
	             (f_player ? f_player->query_wolacz() : "Przybyszu") +
	             ". Podaj swe haslo: ");
	f_player->remove_object();
	input_to(check_password, 1);
    }
    else
    {
	write_socket("Najprawdopodobniej masz uszkodzona postac. " +
	    "Postaraj sie o tym niezwlocznie\npowiadomic administracje " +
	    "Arkadii, na przyklad jako imie podajac 'podanie'.\n\n");
	remove_alarm(time_out_alarm);
	remove_object();
	return;
    }
}

/*
 * Function name: new_password
 * Description  : This function is used to let a new character set his
 *                password.
 * Arguments    : string p - the intended password.
 */
static void
new_password(string p)
{
    write_socket("\n");
    remove_alarm(time_out_alarm);

    /* If the player does not want to use this character, he can type "quit"
     * as password.
     */
    if (p == "zakoncz")
    {
        write_socket("Trudno sie mowi - do nastepnego razu.\n");
	remove_object();
	return;
    }

    time_out_alarm = set_alarm(TIMEOUT_TIME, 0.0, time_out);

    /* Player decided to enter a different name. */
    if (p == "nowe")
    {
        write_socket("Wpisz swoje imie: ");
        input_to(get_name);
        return;
    }
    
    if (!password)
    {
	if (strlen(p) < 6)
	{
	    write_socket("Haslo musi miec przynajmniej 6 znakow.\n");
	    input_to(new_password, 1);
	    write_socket("Haslo: ");
	    return;
	}
	
	if (!(SECURITY->proper_password(p)))
	{
	    write_socket("Podane haslo nie spelnia ustalonych przez nas " +
	        "podstawowych kryteriow\nbezpieczenstwa.\n");
	    input_to(new_password, 1);
	    write_socket("Haslo: ");
	    return;
	}
	
	if (strlen(old_password) &&
	    (crypt(p, old_password) == old_password))
	{
	    write_socket("Haslo musi sie roznic od poprzedniego.\n");
	    write_socket("Haslo: ");
	    input_to(new_password, 1);
	    return;
	}
	
	password = p;
	input_to(new_password, 1);
	write_socket("Dobrze. Ponownie wpisz to samo haslo, zeby je " +
	    "zweryfikowac.\n");
	write_socket("Haslo (to samo): ");
	return;
    }

    if (password != p)
    {
	password = 0;
	write_socket("Hasla sie nie zgadzaja. Wiecej konsekwencji "+
	    "nastepnym razem!\n");
	input_to(new_password, 1);
	write_socket("Haslo (nowe haslo, pierwsze podejscie): ");
	return;
    }

    /* Crypt the password. Use a new seed. */
    password = crypt(password, 0);

    if (password_set)
    {
	start_player();
    }
    else
    {
	start_player1();
    }
}

/*
 * Function name: tell_password
 * Description  : This function tells the player what we expect from his
 *                new password and then prompt him for it.
 */
static void
tell_password()
{
    write_socket(
"Aby ustrzec twe haslo przed zlamaniem, uwazamy ze powinno ono spelniac\n" +
" podstawowe kryteria:\n" +
" - musi miec przynajmniej 6 znakow;\n" +
" - musi miec przynajmniej 1 znak nie bedacy litera;\n" +
" - musi zaczynac sie i konczyc litera.\n\n" +

"Nowe haslo: ");
    
    input_to(new_password, 1);
}

/*
 * Function name: check_password
 * Description  : If an existing player tries to login, this function checks
 *                for the password. If you fail, you are a granted a second
 *                try.
 * Arguments    : string p - the intended password.
 */
static void
check_password(string p)
{
    int wiz;
    
    write_socket("\n");

    remove_alarm(time_out_alarm);
    time_out_alarm = set_alarm(TIMEOUT_TIME, 0.0, time_out);

    /* Player has no password, force him/her to set a new one. */
    if (password == 0)
    {
        write_socket("Nie masz zadnego hasla!\nMusisz ustawic nowe " +
           "zanim dopuscimy cie dalej.\n\n");
	password_set = 1;
	old_password = password;
	password = 0;
	tell_password();
	return;
    }

    /* Password matches. Go! */
    if (crypt(p, password) == password)
    {
	/* Reset the login flag so people won't skip the queue. */
	login_flag = 0;

#ifdef FORCE_PASSWORD_CHANGE
	wiz = SECURITY->query_wiz_rank(name);
	    
	if ((password_time + (wiz ? FOUR_MONTHS : SIX_MONTHS)) < time())
        {
	    write_socket("Minel" + (wiz ? "y ponad 4 miesiace"
	        : "o ponad 6 miesiecy") + " od momentu ostatniej " +
		"zmiany twego hasla.\nW zwiazku z tym musisz je zmienic, " +
		"zanim dopuscimy cie dalej.\n\n");
	    password_set = 1;
	    old_password = password;
	    password = 0;
	    tell_password();
	    return;
        }
#endif FORCE_PASSWORD_CHANGE

	start_player();
	return;
    }

    write_socket("Niewlasciwe haslo!\n");

    /* Player already had a second chance. Kick him/her out. */
    if (login_flag)
    {
	remove_object();
	return;
    }

    login_flag = 1;
    write_socket("Haslo (druga i ostatnia proba): ");
    input_to(check_password, 1);
}

/*
 * Function name: try_throw_out
 * Description  : If the player tries to login while another interactive
 *                player with the same name is active, we ask whether to
 *                kick out the other copy.
 * Arguments    : string str - the answer, should start with 'y' or 'n'.
 */
static void
try_throw_out(string str)
{
    object ob;

    /* Only allow valid answers. */
    str = lower_case(str);
    if (strlen(str) &&
    	(str[0] == 'n'))
    {
	write_socket("Do zobaczenia innym razem!\n");
	remove_object();
	return;
    }

    remove_alarm(time_out_alarm);
    time_out_alarm = set_alarm(TIMEOUT_TIME, 0.0, time_out);

    if ((!strlen(str)) ||
    	(str[0] != 't'))
    {
	write_socket("Odpowiedz [t]ak lub [n]ie.\n" +
	    "Czy chcesz przejac nad nia kontrole? ");
	input_to(try_throw_out);
	return;
    }

    ob = find_player(name);
    if (!objectp(ob))
    {
        write_socket("W miedzyczasie kopia twojej postaci w grze sie " +
            "wylogowala. Wchodzisz\nwiec jak zazwyczaj...\n");
	login_type = ENTER_ENTER;
	start_player();
	return;
    }

    login_type = ENTER_SWITCH;
    start_player2(ob);
}

/*
 * Function name: query_race_name
 * Description  : Return the race name of this object.
 * Returns      : string - "logon".
 */
public string
query_race_name()
{
    return "logon";
}

/*
 * Function name: catch_tell
 * Description  : This function can be called externally to print a text to
 *                the logon-player.
 * Arugments    : string msg - the text to print.
 */
public void
catch_tell(string msg)
{
    write_socket(msg);
}

/*
 * Function name: queue
 * Description  : If the game is full, you are asked whether or not to
 *                queue. This function is called with the answer.
 * Arguments    : string str - the answer, either 'y' or 'n'.
 */
static void
queue(string str)
{
    int pos;

    /* Only allow valid answers. */
    str = lower_case(str);
    if (strlen(str) &&
    	(str[0] == 'n'))
    {
        write_socket("Trudno sie mowi - do nastepnego razu.\n");
	remove_object();
	return;
    }

    remove_alarm(time_out_alarm);

    if ((!strlen(str)) ||
    	(str[0] != 't'))
    {
	time_out_alarm = set_alarm(TIMEOUT_TIME, 0.0, time_out);

	write_socket("Odpowiedz [t]ak lub [n]ie.\n" +
	    "Czy chcesz sie ustawic w kolejce? ");
	input_to(queue);
	return;
    }

    /* Maybe the player got lucky after all.*/
    if (pos = QUEUE->enqueue(this_object()))
    {
	write_socket("Jestes na " + LANG_SORD(pos, PL_MIE, PL_ZENSKI) +
	    " pozycji w kolejce.\n\nWpisz 'zakoncz', jesli nie chcesz wiecej " +
	    "stac w kolejce, 'system', zeby\npobrac informacje o stanie " +
	    "pamieci i o lokalnym czasie lub tez\n'kto', zeby zobaczyc kto " +
	    "znajomy jest w grze. 'p' wyswietli ci numer\ntwej pozycji w " +
	    "kolejce.\nKOLEJKA> ");

	input_to(waitfun);
    }
    else
    {
        write_socket("Masz szczescie. Ktos wyszedl z gry, gdy czekalismy " +
            "na twoja reakcje. Wchodzisz wiec natychmiast do gry...\n");
	start_player();
    }
}

/*
 * Function name: who
 * Description  : Called when the player wants to see which other players
 *                are logged on.
 */
static void
who()
{
    object  *players;
    int     index;
    int     size;

    if (!mappingp(m_remember_name) ||
    	!m_sizeof(m_remember_name))
    {
	write_socket("Nie znasz jeszcze zadnej osoby.\n");
	return;
    }

    players = users() - ({ 0 }) - (object *)QUEUE->queue_list(0);
    size = sizeof(players);
    index = -1;

    if (!SECURITY->query_wiz_level(name))
    {
	while(++index < size)
	{
	    if (((!m_remember_name[players[index]->query_real_name()]) &&
		 (!players[index]->query_prop(LIVE_I_ALWAYSKNOWN))) ||
		(players[index]->query_prop(OBJ_I_INVIS)))
	    {
		players[index] = 0;
	    }
	}
    }

    players -= ({ 0 });
    if (!sizeof(players))
    {
        write_socket("Nie znasz zadnej osoby sposrod tych, ktore przebywaja "+
            "w Arkadii.\n");
    }
    else
    {
        write_socket("Sposrod osob przebywajacych w Arkadii znasz:\n" +
	    sprintf("%-75#s\n",
	    implode(sort_array(players->query_real_name(PL_BIE)), "\n")));
    }
}

/*
 * Function name: position
 * Description  : Print the position of the player in the queue.
 */
static void
position()
{
    int pos = QUEUE->query_position(name);

    if (pos == -1)
    {
        write_socket("Hmm, dziwne! Nie wiedziec czemu jestes poza kolejka!\n"+
            "Sprobuj polaczyc sie jeszcze raz.\n");
	remove_object();
	return;
    }
    else
    {
	write_socket("Masz " + LANG_SORD((pos + 1), PL_BIE, PL_ZENSKI) + 
	    " pozycje w kolejce.\n");
    }
}

/*
 * Function name: waitfun
 * Description  : While the player is in the queue, the input from the
 *                player is put in this function.
 * Arguments    : string str - the input from the player.
 */
static void
waitfun(string str)
{
    /* If login_flag is 2, this means that the player already queued and
     * that he/she only needs to enter a command to unidle. We don't need
     * to check the actual command. Just run the show.
     */
    if (login_flag == 2)
    {
	set_this_player(this_object());

	start_player();

	return;
    }

    input_to(waitfun);

    if (!strlen(str))
    {
	write_socket("KOLEJKA> ");
	return;
    }

    str = lower_case(str);
    switch(str[0])
    {
    case 's':
	date();
	write_socket("KOLEJKA> ");
	return;

    case 'p':
	position();
	write_socket("KOLEJKA> ");
	return;

    case 'z':
        write_socket("Trudno sie mowi - do nastepnego razu.\n");
	remove_object();
	return;

    case 'k':
	who();
	write_socket("KOLEJKA> ");
	return;

    default:
	write_socket("Zla komenda. Dostepne to: [s]ystem, [p]ozycja, " +
	   "[k]to i [z]akoncz.\nKOLEJKA> ");
    }
}

/*
 * Function name: advance
 * Description  : When someone leaves the game or the queue, a new player
 *                may log in. This function is called to give player his
 *                new queue-position or make him enter the game.
 * Arguments    : int num - if 0 the player may enter, else the new number.
 */
public void
advance(int num)
{
    if (!CALL_BY(QUEUE))
    {
	return;
    }

    set_this_player(this_object());
    if (!num)
    {
	write_socket(" ... lacze ...\n");

	/* We have to do this to reset the idle flag in the GameDriver. */
	if (query_idle(this_object()) > (MAX_IDLE_TIME / 2))
	{
	    write_socket(break_string("Ze wzgledu na twoja nieaktywnosc " +
	        "przez " + CONVTIME(query_idle(this_object())) + ", musisz "+
	        "wpisac cokolwiek, zanim dopuscimy cie dalej.",
                76) + "\nWpisz cokolwiek > ");

	    /* A 'true' login flag means player already queued, 2 means
	     * the player has to press a key to continue.
	     */
	    login_flag = 2;
	    return;
	}

	start_player();
    }
    else
    {
	write_socket("Jestes na " + LANG_SORD(num, PL_MIE, PL_ZENSKI) +
	    " pozycji w kolejce.\nKOLEJKA> ");
    }
}

public string
query_name()
{
    return name;
}

/*
 * Function name: query_login_flag
 * Description  : Returns the current login flag.
 * Returns      : int - the login flag.
 */
public int
query_login_flag()
{
    return login_flag;
}

/*
 * Function name: query_prevent_shadow
 * Description  : This function prevents shadowing of this object.
 * Returns      : int 1 - always.
 */
nomask public int
query_prevent_shadow()
{
    return 1;
}
