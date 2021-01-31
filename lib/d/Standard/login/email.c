inherit "/std/room";

#include <login.h>
#include <stdproperties.h>

static void ask_player();
void next_query();

create_room()
{
    set_long("");
    set_short("");
    
    add_prop(ROOM_I_LIGHT, -10);
    add_prop(ROOM_I_INSIDE, 1);
    add_prop(ROOM_S_DARK_LONG, "");
}

public void
enter_inv(object ob, object from)
{
    ::enter_inv(ob, from);

    if (ob->query_ghost() & GP_EMAIL)
	set_alarm(1.0, 0.0, &ask_player());
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
    new_queries = ({ "dummy" }) + odmiana + (this_player()->query_mailaddr() ? ({}) 
                                             : ({ "q_mail" }));
    przypadki = allocate(6);
    przypadki[0] = lower_case(this_player()->query_real_name());

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
    this_player()->set_ghost((this_player()->query_ghost()) & (~GP_EMAIL));
    this_player()->catch_msg("\n\n");
    this_player()->move_living("X",
	this_player()->query_default_start_location(), 1, 0);
}

/*
 * Function name: next_query
 * Description:   Asks the next question of the user interactively.
 */
void
next_query()
{
    while (1)
    {
	if (sizeof(new_queries) < 2)
	    return end_query();	/* does not return */
	new_queries = slice_array(new_queries, 1, sizeof(new_queries));
	if (call_other(this_object(), new_queries[0] + "_pretext"))
	{
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
    if (this_player()->query_mailaddr())
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
    this_player()->set_mailaddr(maddr);
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
         this_player()->ustaw_imiona(przypadki);
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
