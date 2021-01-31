/*
 * Obiekt przeznaczony dla graczy, podczas ich pierwszych
 * 12 godzin gry, by mogli sobie poprawic odmiane imienia.
 */
 
inherit "/std/object";

#pragma no_inherit
#pragma strict_types
#pragma save_binary

#include <pl.h>
#include <stdproperties.h>
#include <std.h>
#include <macros.h>

#define CZAS_ZNIKANIA 43200

public int popraw(string str);
static void ask_player();
void	    next_query();

string *imiona;
string *odmiana;
string *new_queries;

public void
create_object()
{
    ustaw_nazwe( ({ "elementarz", "elementarza", "elementarzowi",
        "elementarz", "elementarzem", "elementarzu" }), 
        ({ "elementarze", "elementarzy", "elementarzom", "elementarze", 
        "elementarzami", "elementarzach" }), PL_MESKI_NOS_NZYW);
    
    set_long("Dzieki elementarzowi, podczas pierwszych dwunastu " +
        "godzin gry jestes w stanie poprawic odmiane " +
        "wlasnego imienia. W tym celu skorzystaj z komendy " + 
        "'popraw odmiane'. Strzez sie jednak! Celowe zle ustawianie " +
        "odmiany jest wbrew zasadom - w ten sposob inni gracze " +
        "nie beda w stanie wykonywac na tobie zadnych komend.\n");
        
    add_prop(OBJ_M_NO_DROP, "Elementarza nie mozna sie pozbyc - " +
        "sam zniknie po jakims czasie.\n");
    add_prop(OBJ_M_NO_STEAL, 1);
    add_prop(OBJ_M_NO_SELL, "Elementarza nie da sie sprzedac!\n");
    add_prop(OBJ_M_NO_GIVE, "Elementarza nie mozna sie pozbyc - sam " +
        "zniknie po jakims czasie.\n");
    
    add_prop(OBJ_I_WEIGHT, 0);
    add_prop(OBJ_I_VOLUME, 0);
    add_prop(OBJ_I_VALUE, 0);
}

public void
sprawdz_wiek()
{
    object env;
    
    if (!(env = environment(this_object())) || 
          !interactive(env) ||
          env->query_age() > CZAS_ZNIKANIA)
    {
        remove_object();
        return;
    }
    
    set_alarm(3600.0, 0.0, &sprawdz_wiek());
}

public void
pokaz_odmiane()
{
    string *imiona, imie;
    int ix;
    
    imiona = this_player()->query_imiona();
    if (sizeof(imiona) != 6)
    {
	imiona = allocate(6);
	imie = this_player()->query_real_name();
	for (ix = 0; ix < 6; ix++)
	    imiona[ix] = imie;
	this_player()->ustaw_imiona(imiona);
    }

    write(sprintf("Mianownik:\t%s\nDopelniacz:\t%s\nCelownik:\t%s\n" +
	"Biernik:\t%s\nNarzednik:\t%s\nMiejscownik:\t%s\n",
	imiona[0], imiona[1], imiona[2], imiona[3], imiona[4], imiona[5]));
}

public int
popraw(string str)
{
    string *imiona;
    
    if (str != "odmiane")
    {
        notify_fail("Popraw co? Odmiane?\n");
        return 0;
    }
    
    if (this_player() != this_interactive())
        return 1;
        
    if (environment(this_object()) != this_player())
    {
        write("Musisz miec odmieniacza, zeby moc z niego " +
            "skorzystac.\n");
        return 1;
    }
    
    pokaz_odmiane();
    
    ask_player();
    
    return 1;
}

public void
init()
{
    ::init();
    
    sprawdz_wiek();
    add_action(popraw, "popraw");
}

/*
 * Function name: ask_player
 * Description:   Ask some questions of new players
 */
static void
ask_player()
{
    odmiana = ({ "q_odmien_dopelniacz", 
        "q_odmien_celownik", "q_odmien_biernik", 
        "q_odmien_narzednik","q_odmien_miejscownik", 
        "q_potwierdz_odmiane" });
    new_queries = ({ "dummy", "q_czy_odmienic" });

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
q_czy_odmienic_pretext()
{
    write("Czy chcesz poprawic odmiane swojego imienia[t/n]: ");
    return 1;
}

int
q_odmien_dopelniacz_pretext()
{
    write("Jako mianownik podstawiam Twoje imie.\n");
    imiona[0] = this_player()->query_real_name();
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
    write("\nOto jak wyglada nowa odmiana Twego imienia:\n");
    write("Mianownik:   " + capitalize(imiona[0]) + "\n");
    write("Dopelniacz:  " + capitalize(imiona[1]) + "\n");
    write("Celownik:    " + capitalize(imiona[2]) + "\n");
    write("Biernik:     " + capitalize(imiona[3]) + "\n");
    write("Narzednik:   " + capitalize(imiona[4]) + "\n");
    write("Miejscownik: " + capitalize(imiona[5]) + "\n");
    write("\nPrzeczytaj swe odpowiedzi jeszcze raz, upewniajac sie, ze sa " +
        "one wlasciwe.\nPosiadanie zle odmienionego imienia jest wbrew " +
        "zasadom.\n\n" +
        "Czy odmiana jest prawidlowa[t/n]: ");

    return 1;
}

int
q_potwierdz_again_pretext()
{
    write("Odpowiedz [t]ak, lub [n]ie: ");
    return 1;
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
q_czy_odmienic(string odp)
{
    string o;
    
    o = lower_case(odp[0..0]);
    
    if (o == "t")
    {
        imiona = allocate(6);
        new_queries += odmiana;
        next_query();
        return;
    }
    else 
        next_query();
}

static void
q_odmien_dopelniacz(string przyp)
{
    if (!wlasciwe_imie(przyp))
    {
        again_query();
        return ;
    }
    imiona[1] = lower_case(przyp);
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
    imiona[2] = lower_case(przyp);
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
    imiona[3] = lower_case(przyp);
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
    imiona[4] = lower_case(przyp);
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
    imiona[5] = lower_case(przyp);
    next_query();
}

static void
q_potwierdz_odmiane(string odp)
{
    string o;
    
    o = lower_case(odp[0..0]);
    
    if (o == "n")
    {
        write("W takim razie zostawiam stara odmiane.\n");
        imiona = this_player()->query_imiona();
        next_query();
        return;
    }
    else 
    if (o == "t")
    {
         write("Ustawiam nowa odmiane.\n");
         this_player()->ustaw_imiona(imiona);
         SECURITY->log_syslog("ODM_IMIE", ctime(time()) + "    " +
             this_player()->query_real_name() + "   sam" +
             this_player()->koncowka("", "a") +
             " sobie (odmieniacz graczy)\n Nowa odmiana: (" +
             implode(imiona, ",") + ")\n\n");
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

public string
query_auto_load()
{
    return MASTER + ":";
}
