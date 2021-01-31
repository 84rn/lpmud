/*
 * application_player.c
 * 
 * Blocked site player application procedures.
 */

#pragma no_inherit
#pragma save_binary
#pragma strict_types

#include <config.h>
#include <std.h>
#include <macros.h>
#include <files.h>

/*
 * Prototypes
 */
static void time_out();
static void input_enter(string str);
static void quit();

/*
 * Global variables.
 */
static string imie;
static string plec;
static string secondzi;
static string email;
static string rpg;

static int    time_out_alarm;

#define TIME_OUT	300.0

static void
dolacz_podanie()
{
    object ob;
    string result = "";

    catch(LOAD_ERR(APPLICATION_BOARD_LOC));
    ob = present("tablica z podaniami", find_object(APPLICATION_BOARD_LOC));

    if (!objectp(ob))
    {
	write_socket("\nNagranie podania okazalo sie niemozliwe. " +
	    "Sprobuj napisac je troche\npozniej, kiedy to blad " +
	    "zostanie znaleziony i poprawiony. Jesli znasz\n" +
	    "kogos, kto ma dostep do gry, popros go by powiadomil " +
	    "administracje\no tym problemie.\n");
    }
    else
    {
	ob->get_subject("Podanie z " + query_ip_number(this_object()));
	
	if (SECURITY->query_wiz_rank(imie))
	    result += "\tImie nalezy do bylego czarodzieja\n";

	if (SECURITY->check_newplayer(query_ip_number(this_object())) == 2)
	    result += "\tHost zabanowany na tworzenie nowych postaci\n";
	    
	result = sprintf("%s\n\n%s\t%s@%s\t\n\nImie: %s\nPlec: %s\nSecondzi: %s\n" +
	    "Kontakt z rpg: %s\nEmail: %s\nUwagi: %s\n\n", ctime(time()),
	    query_ip_number(this_object()), (query_ip_ident(this_object())
	    ?: ""), query_ip_name(this_object()), imie, plec, secondzi, rpg,
	    email, result);
	    
	ob->done_editing(result);
    
	write_socket("\n\n\n\n  Podanie zostalo wypelnione. Ewentualna " +
	    "odpowiedz powinna nadejsc w ciagu tygodnia (w zaleznosci " +
	    "od naszych mozliwosci czasowych). Brak odpowiedzi\nw tym " +
	    "czasie moze oznaczac kilka rzeczy - to, ze adres email " +
	    "zostal\nzle podany, lub list nie mogl pod niego dotrzec; " +
	    "to, ze podanie zostalo\nodrzucone z jakichs wzgledow " +
	    "(propozycja kiepskiego imienia, naganne\nzachowanie osob " +
	    "grajacych z miejsca, z ktorego sie laczysz). W tym\ndrugim " +
	    "przypadku zazwyczaj staramy sie napisac stosowna " +
	    "odpowiedz,\nmoze jednak zdarzyc sie, ze nie starczy nam na " +
	    "to czasu.\n");
    }
    
    destruct();

    return;
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
write_imie_info()
{
    write_socket("\n\n\n  Twym zadaniem jest teraz wybranie imienia dla " +
	"twojej postaci. Musi ono\nspelniac pewne, ustalone przez nas kryteria. " +
	"Zaproponowane przez ciebie\nimie bedzie jednym z podstawowych " +
	"kryteriow, wedlug ktorych zadecydujemy\no ewentualnym przyznaniu " +
	"ci postaci.\n\nOto garsc wskazowek, majacych na celu ulatwienie " +
	"ci wyboru dobrego imienia:\n\n - mud ten jest swiatem zbudowanym " +
	"w konwencji fantasy. Oczekujemy wiec,\n   ze imiona postaci w " +
	"naszym swiecie beda pasowaly do jego klimatu.\n - mud jest " +
	"stworzony w jezyku polskim. Twoje imie powinno sie dac\n   w latwy " +
	"i oczywisty sposob odmienic przez przypadki.\n - Twoje imie " +
	"powinno byc w miare mozliwosci oryginalne; imiona znanych\n" +
	"   postaci z literatury i z filmow nie sa mile widziane.\n\n" +
	"Tym samym imiona Zdzisiek, Oeqweqwepaow, Gandalf oraz Szturmix " +
	"z pewnoscia\nnie zostana przez nas zaakceptowane.\n\n");
}

static void
input_rpg(string str)
{
    remove_alarm(time_out_alarm);
    
    if (str == "zakoncz")
    {
	quit();
	return;
    }
    
    rpg = str;
    
    dolacz_podanie();
}

static void
input_secondzi(string str)
{
    remove_alarm(time_out_alarm);
    
    if (str == "zakoncz")
    {
	quit();
	return;
    }
    
    secondzi = str;
    
    write_socket("\n\n\nCzy grywasz w jakies gry fabularne, wymagajace od " +
	"graczy\nwczuwania sie w swoje role? Czy jest cos, czym mozesz " +
	"sie\nprzed nami pochwalic pod tym katem?: ");
    input_to(input_rpg);
    time_out_alarm = set_alarm(TIME_OUT, 0.0, time_out);
}

static int
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

static void
input_plec(string str)
{
    remove_alarm(time_out_alarm);
    
    if (str == "zakoncz")
    {
	quit();
	return;
    }
    
    time_out_alarm = set_alarm(TIME_OUT, 0.0, time_out);
    
    plec = str;

    write_socket("\n\n\nCzy masz jakies inne postacie w swiecie Arkadii?\n" +
	"Jesli tak, jakie: ");
    input_to(input_secondzi);
    return;
}

static void
input_imie(string str)
{

    if (!valid_name(str))
    {
	input_to(input_imie);
	write_socket("Wymysl inne imie: ");
	return;
    }

    if (restore_object("/players/" + extract(str, 0, 0) + "/" + str))
    {
	write_socket("\nNiestety, postac o takim imieniu juz istnieje.\n" +
	    "Musisz wymyslec inne imie: ");
	input_to(input_imie);
	return;
    }

    if (file_size(BANISH_FILE(str)) >= 0)
    {
	write_socket("\nImie '" + capitalize(str) +
	    "' jest zarezerwowane.\nSprobuj wymyslec inne: ");
	input_to(input_imie);
	return;
    }

    if (SECURITY->query_domain_number(capitalize(str)) >= 0)
    {
	write_socket("\nJedna z naszych domen nosi te nazwe.\nSprobuj " +
	    "wymyslec inne imie: ");
	input_to(input_imie);
	return;
    }

    if (LOGIN_OBJECT->offensive_name(str))
    {
	write_socket("\nPodane przez ciebie imie uznane zostalo za " +
	    "obrazliwe. Nastepnym razem\nwymysl cos lepszego, albo " +
	    "poszukaj sobie innego muda.\n");
	remove_alarm(time_out_alarm);
	destruct();
	return;
    }

    if (stupid_name(str))
    {
	input_to(input_imie);
	write_socket("Wymysl inne imie: ");
	return;
    }

    imie = str;

    write_socket("\nPodaj plec swojej (miejmy nadzieje) nowej postaci: ");
    input_to(input_plec);
    return;
}

static void
input_email(string str)
{
    remove_alarm(time_out_alarm);
    
    if (str == "zakoncz")
    {
	quit();
	return;
    }
    
    if (!strlen(str) || (member_array("@", explode(str, "")) == -1))
    {
	write_socket("Nieprawidlowy adres email.\n\n");
	quit();
	return;
    }
    email = str;
    
    write_imie_info();
    input_to(input_imie);
    time_out_alarm = set_alarm(TIME_OUT, 0.0, time_out);
    write_socket("\nZaproponuj imie dla swej postaci: ");
}

static void
input_enter(string str)
{
    remove_alarm(time_out_alarm);
    
    if (str == "zakoncz")
    {
	quit();
	return;
    }
    
    if (imie && email && rpg)
    {
	destruct();
    }

    write_socket("\n  Na wstepie musisz nam podac swoj adres email. " +
	"Potrzebujemy go chociazby\npo to, by moc pozniej przeslac ci " +
	"haslo dla twojej ewentualnej postaci.\nJesli nie posiadasz " +
	"_wlasnego_ adresu pocztowego, niestety nie mozesz\nstworzyc " +
	"sobie u nas postaci.\n\nPodaj swoj adres email " +
	"(lub wpisz 'zakoncz'): ");
    input_to(input_email);
    time_out_alarm = set_alarm(TIME_OUT, 0.0, time_out);
}

/*
 * Function name: enter_game
 * Description  : This function is called from the login object and enables
 *                the player to connect. It takes care of the necessary
 *                initialization before the player is allowed to file his
 *                request.
 */
void
enter_game()
{
    string data;
    int bl;

    set_screen_width(80);

    bl = SECURITY->check_newplayer(query_ip_number(this_object()));

/*
    if (bl == 0)
    {
	write_socket("Your site isn't blocked. Log in as usual.\n");

	destruct();
	return;
    }
    else */if (bl == 1)
    {
        write_socket("Miejsce, z ktorego sie laczysz zostalo calkowicie " +
            "zablokowane. Oznacza to, ze nikt stad nie zostanie " +
            "wpuszczony.\n");

	destruct();
	return;
    }

    enable_commands();
    setuid();
    seteuid(getuid());
    
    write_socket("\n\n  Wypelnione podanie zostanie dostarczone " +
	"Administracji Arkadii.\nPostaraj sie wypelnic je starannie. " +
	"Wpisywanie mozesz w kazdej\nchwili przerwac wpisujac 'zakoncz'.\n\n" +
	"Nacisnij enter. ");

    input_to(input_enter);
    time_out_alarm = set_alarm(TIME_OUT, 0.0, time_out);
}

/*
 * Function name: quit
 * Description  : Remove the object from memory when the player aborts.
 */
static void
quit()
{
    write_socket("Wpisywanie przerwane.\n");

    destruct();
}

/*
 * Function name: time_out
 * Description  : Called after some time to close the connection.
 */
static void
time_out()
{
    write_socket("\nTwoj czas uplynal. Zapraszamy nastepnym razem.\n");

    destruct();
}

/*
 * Function name: query_real_name
 * Description  : Return the real name of the this object.
 * Returns      : string - always 'application'.
 */
public string
query_real_name(int przyp = 0)
{
    return "podanie";
}

/*
 * Function name: query_prevent_shadow
 * Description  : Prevent shadowing of this object.
 * Returns      : int 1 - always.
 */
nomask public int
query_prevent_shadow()
{
    return 1;
}
