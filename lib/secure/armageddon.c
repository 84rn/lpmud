/*
 * /secure/armageddon.c
 *
 * This object takes the mud down in a gracefull manner.
 *
 * This is supposed to be inherited by the actual Armageddon object
 * for mud-specific actions to be taken when the game closes down.
 * SECURITY is the only object that is allowed to shut down the game.
 * All requests have to go through the master.
 */

#pragma no_clone
#pragma no_shadow
#pragma save_binary
#pragma strict_types

inherit "/std/creature";

#include <macros.h>
#include <std.h>
#include <stdproperties.h>
#include <time.h>

/*
 * Global variables.
 */
static private string shutdown_shutter = 0;
static private string shutdown_reason = 0;
static private int    shutdown_delay = 0;
static private int    shutdown_alarm = 0;

/*
 * Function name: query_init_master
 * Description:   Makes sure that the master object is initialized properly.
 */
public nomask int
query_init_master()
{
    return 1;
}

/*
 * Function name: create_creature
 * Description  : Called to create the statuette.
 */
public void
create_creature()
{
    ustaw_imie(({"armageddon", "armageddonu", "armageddonowi",
                 "armageddon", "armageddonem", "armageddonie"}),
               PL_MESKI_NZYW);

    dodaj_nazwy(({"statua", "statuy", "statui", "statue", "statua",
                  "statui"}),
                ({"statuy", "statui", "statuom", "statuy", "statuami",
                  "statuach"}), PL_ZENSKI);

    dodaj_przym("maly", "mali");

    ustaw_shorty(({"mala statua", "malej statuy", "malej statui",
                   "mala statue", "mala statua", "malej statui"}),
                 ({"male statuy", "malych statui", "malym statuom",
                   "male statuy", "malymi statuami", "malych statuach"}),
                 PL_ZENSKI);

    set_long("Mala statuetka przedstawiajaca Armageddon!\n");

    set_living_name("armageddon");
    set_tell_active(1);

    add_prop(LIVE_I_ALWAYSKNOWN, 1);
}

/*
 * Funkcja   : armageddon_tell
 * Opis      : Wysyla komunikat do jednego z graczy.
 * Argumenty : gracz - wskazany gracz
 *             str - wysylany komunikat
 */
static void
armageddon_tell(object gracz, string str)
{
    gracz->catch_tell("W swoim umysle slyszysz glos Jezdzca Apokalipsy, "
                    + "oznajmiajacy: " + str + "\n");
}

/*
 * Funkcja   : armageddon_tellall
 * Opis      : Wysyla komunikat do wszystkich graczy.
 * Argumenty : str - wysylany komunikat
 *             scenka - czy komunikat ma byc poprzedzony nastrojowa scenka
 */
static void
armageddon_tellall(string str, int scenka = 0)
{
    object *gracze = users();
    int n = -1;
    int ilu = sizeof(gracze);
    string wstep;

    if (scenka)
        while (++n < ilu)
        {
            wstep = "Czujesz drzenie ziemi, jakby pod uderzeniami kopyt "
                  + "tysiecy galopujacych koni. ";
            if (environment(gracze[n])->query_prop(ROOM_I_INSIDE))
                wstep += "Twoja czaszke rozdziera tepy bol. Otoczenie "
                       + "rozplywa sie, ustepujac miejsca wizji widzianej "
                       + "oczami twego umyslu. ";
            else
                wstep += "Tetent zbliza sie, budzac rezonans w twoim "
                       + "umysle. ";
            switch (random(4))
            {
                case 0:
                    wstep += "Widzisz jezdzca na bialym koniu, dzierzacego "
                           + "luk i uwienczonego korona, ktory wyruszyl "
                           + "zwyciezajac i aby zwyciezyc.";
                    break;
                case 1:
                    wstep += "Widzisz jezdzca na koniu barwy ognia, "
                           + "dzierzacego miecz, ktory wyruszyl zabrac pokoj "
                           + "z ziemi, aby mordowali jedni drugich.";
                    break;
                case 2:
                    wstep += "Widzisz jezdzca na czarnym koniu, dzierzacego "
                           + "wage, ktory wyruszyl sadzic sadzacych i "
                           + "sadzonych.";
                    break;
                case 3:
                    wstep += "Widzisz jezdzca na trupiobladym koniu, "
                           + "dzierzacego kose, ktorego imie brzmi Smierc.";
                    break;
                default:
            }
            gracze[n]->catch_tell(wstep + "\n\n");
            armageddon_tell(gracze[n], str);
        }
    else
        while (++n < ilu)
            armageddon_tell(gracze[n], str);
}

/*
 * Function name: shutdown_now
 * Description  : When the game finally goes down, this is the function
 *                that tells the master to do so.
 */
private nomask void
shutdown_now()
{
    armageddon_tellall("Nadszedl Czas Apokalipsy. Swiat zostaje "
                     + "zniszczony.\n");

    if (!SECURITY->master_shutdown(sprintf("%-11s: %s\n",
        capitalize(shutdown_shutter), shutdown_reason)))
    {
	armageddon_tellall("Dziwne... Moj Pan nie pozwolil mi wyzwolic mej "
	                 + "mocy.");

	shutdown_alarm   = 0;
	shutdown_delay   = 0;
	shutdown_reason  = 0;
	shutdown_shutter = 0;
    }
}

/*
 * Funkcja   : shutdown_dodelay
 * Opis      : Odlicza czas pozostaly do reboota.
 * Argumenty : silent - za pierwszym razem o pozostalym im czasie gracze
 *                      dowiedza sie gdzie indziej
 */
private nomask void
shutdown_dodelay(int silent = 0)
{
    int period;

    /* No more delay, it is closing time. */
    if (!shutdown_delay)
    {
        shutdown_now();
        return;
    }

    if (!silent)
        armageddon_tellall("Juz tylko " + CONVTIME(shutdown_delay * 60)
                         + " do momentu zniszczenia swiata.");

    /* If the shutdown period is longer, we will not notify the players
     * each minute, but use a larger delay.
     */
    if (shutdown_delay >= 1080)
        period = 720;
    else if (shutdown_delay >= 90)
        period = 60;
    else if (shutdown_delay >= 25)
        period = 15;
    else if (shutdown_delay >= 10)
        period = 5;
    else
        period = 1;

    shutdown_alarm = set_alarm((itof(period * 60)), 0.0, shutdown_dodelay);
    shutdown_delay -= period;
}

/*
 * Function name: shutdown_started
 * Description  : This function is called when the game is shut down. It
 *                can be redefined by the local armageddon object at your
 *                mud.
 */
public void
shutdown_started()
{
}

/*
 * Function name: start_shutdown
 * Description  : When the game has to be shut down in a gentle way,
 *                this is the function you are looking for. You should
 *                not try to call it directly, but use the 'shutdown'
 *                command.
 * Arguments    : string reason  - the reason to close the game.
 *                int    delay   - the delay in minutes.
 *                string shutter - who is shutting down the game.
 */
public nomask void
start_shutdown(string reason, int delay, string shutter)
{
    string komunikat;

    if (previous_object() != find_object(SECURITY))
        return;

    switch (reason[-1..])
    {
        case ".":
        case "!":
        case "?":
            break;
        default:
            reason += ".";
    }

    shutdown_shutter = shutter;
    shutdown_reason  = reason;
    shutdown_delay   = delay;

    /* When shutdown is started, we destruct the queue and tell the people
     * to get back later.
     */
    QUEUE->tell_queue("Niedlugo (jeszcze " + CONVTIME(shutdown_delay * 60) +
		    ") nastapi restart Arkadii. Prosze sprobowac polaczyc " +
                    "sie ponownie, kiedy gra znowu wstanie. Sam restart " +
                    "nie powinien trwac dluzej, niz kilka minut.\n");
    (QUEUE->queue_list(0))->remove_object();

    if (shutter == ROOT_UID)
        komunikat = "Nadchodzi Czas Apokalipsy: " + reason;
    else
        komunikat = "Nadchodzi Czas Apokalipsy. " + capitalize(shutter)
                  + " poprosil"
                  + (find_player(shutter)->query_gender() == G_FEMALE ? "a" :
                     "") + " mnie o jej przyspieszenie: " + reason;

#ifdef STATUE_WHEN_LINKDEAD
#ifdef OWN_STATUE
    /* Since people are not allowed to re-link when the game is about to
     * be shut down, we inform the statue-object of the fact so they can
     * save the players and log them out.
     */
    OWN_STATUE->shutdown_activated();
#endif OWN_STATUE
#endif STATUE_WHEN_LINKDEAD

    if (!shutdown_delay)
    {
        armageddon_tellall(komunikat + "\n", 1);
        shutdown_now();
        return;
    }

    shutdown_started();

    komunikat += " Jesli chcesz wrocic tam, gdzie znajdziesz sie gdy swiat "
               + "sie odrodzi, napisz 'powroc do domu'. Pamietaj, juz tylko "
               + CONVTIME(shutdown_delay * 60) + " do momentu zniszczenia "
               + "swiata.";

    armageddon_tellall(komunikat + "\n", 1);


    shutdown_dodelay(1);
}

/*
 * Function name: shutdown_stopped
 * Description  : This function is called when the shutdown process is
 *                stopped. It may be redefined by the local armageddon
 *                object at your mud.
 * Arguments    : string stopper - the one who decided not to stop.
 */
public void
shutdown_stopped(string stopper)
{
}

/*
 * Function name: cancel_shutdown
 * Description  : If the wizard who was shutting the game down changed
 *                his mind, this is the way to stop it. Do not call the
 *                function directly, though use: 'shutdown abort'
 * Arguments    : string shutter - the person canceling the shutdown.
 */
public nomask void
cancel_shutdown(string shutter)
{
    if (previous_object() != find_object(SECURITY))
        return;

    set_this_player(this_object());
    armageddon_tellall(capitalize(shutter) + " poprosil"
                     + (find_player(shutter)->query_gender() == G_FEMALE ?
                        "a" : "") + " mnie o powstrzymanie zniszczenia "
                     + "swiata. Na razie Apokalipsa nie nastapi.");

    shutdown_stopped(capitalize(shutter));

    remove_alarm(shutdown_alarm);

    shutdown_shutter = 0;
    shutdown_reason  = 0;
    shutdown_alarm   = 0;
    shutdown_delay   = 0;
}

/*
 * Function name: query_shutter
 * Description  : Return the name of the person shutting us down.
 * Returns      : string - the name.
 */
public nomask string
query_shutter()
{
    return shutdown_alarm ? shutdown_shutter : 0;
}

/*
 * Function name: query_reason
 * Description  : Return the reason for the shutdown.
 * Returns      : string - the reason.
 */
public nomask string
query_reason()
{
    return shutdown_alarm ? shutdown_reason : 0;
}

/*
 * Function name: shutdown_active
 * Description  : Returns true if Armageddon is active.
 * Returns      : int 1/0 - true if Armageddon is active.
 */
public nomask int
shutdown_active()
{
    return shutdown_alarm != 0;
}

/*
 * Function name: shutdown_time
 * Description  : This function returns how long it will take before the
 *                game is shut down.
 * Returns      : int - the remaining time in minutes.
 */
public nomask int
shutdown_time()
{
    if (!shutdown_active())
        return 0;

    /* Get the remaining time until the next alarm and the time needed
     * after the next alarm is called.
     */
    return ftoi(get_alarm(shutdown_alarm)[2]) + (shutdown_delay * 60);
}

/*
 * Funkcja : send_me_home
 * Opis    : Wysyla do lokacji startowej gracza wywolujacego ja za
 *           pomoca obiektu /d/Standard/obj/arma_tell.
 */
public void
send_me_home()
{
    string home;

    if (!shutdown_active())
    {
        armageddon_tell(this_player(), "Nie ma powodu abym umozliwial ci "
                      + "powrot do domu, gdyz Czas Apokalipsy jeszcze nie "
                      + "nadchodzi.");
        return;
    }
    if (!(home = this_player()->query_temp_start_location()) &&
        !(home = this_player()->query_default_start_location()))
    {
        armageddon_tell(this_player(), "Niestety, twoj dom nie istnieje.");
        return;
    }

    if (environment(this_player()) &&
        file_name(environment(this_player())) == home)
    {
        armageddon_tell(this_player(), "Znajdujesz sie juz tam, dokad "
                      + "jak twierdzisz pragniesz sie udac.");
        return;
    }

    armageddon_tell(this_player(), "Sprobuje wyslac cie do domu.");

    /* Third argument idicates group should not try to follow this
     * player.
     */
    if (this_player()->move_living("X", home, 1))
        armageddon_tell(this_player(), "Coz, nie zawsze wszystko dziala "
                                     + "tak, jak bym tego oczekiwal.");
}
