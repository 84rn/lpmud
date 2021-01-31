#pragma strict_types
#pragma save_binary

inherit "/std/object";

#include <cmdparse.h>
#include <pl.h>
#include <macros.h>
#include <stdproperties.h>

private	int	pojemnosc_naczynia,
		ilosc_plynu,
		procent_alco;
private	string	opis_plynu,
		nazwa_dop,
		dlugi_opis;

private	int	napij(string str);
private	int	wylej(string str);

/*
 * Nazwa funkcji : create_beczulka
 * Opis          : Wywolywana przez create_object(), tworzy beczulke. Mozna
 *		   ja zamaskowac, konfigurujac beczulke przy tworzeniu.
 */
public void
create_beczulka()
{
    
}

/*
 * Nazwa funkcji : create_object
 * Opis          : Konstruktor obiektu. Ustawiona jako nomask, wywoluje
 *		   create_beczulka().
 */
public nomask void
create_object()
{
    pojemnosc_naczynia = 5000;
    ilosc_plynu = 0;
    procent_alco = 0;
    opis_plynu = "";
    nazwa_dop = "";
    
    add_prop(OBJ_I_WEIGHT, "@@query_waga");
    add_prop(OBJ_I_VOLUME, "@@query_objetosc");
    add_prop(OBJ_I_VALUE, 100);
    add_prop(OBJ_M_NO_SELL, "@@query_mozna_sprzedac");
    
    set_long("@@long");

    create_beczulka();
}

/*
 * Nazwa funkcji : query_waga
 * Opis          : Zwraca wage beczulki, w zaleznosci od jej pojemnosci i
 *		   ilosci plynu w srodku.
 * Funkcja zwraca: int - patrz wyzej.
 */
public int
query_waga()
{
    return ilosc_plynu + (pojemnosc_naczynia / 4);
}

/*
 * Nazwa funkcji : query_objetosc
 * Opis          : Zwraca objetosc beczulki, w zaleznosci od jej pojemnosci.
 * Funkcja zwraca: int - patrz wyzej.
 */
public int
query_objetosc()
{
    return 6 * pojemnosc_naczynia / 5;
}

/*
 * Nazwa funkcji : query_mozna_sprzedac
 * Opis          : Zwraca czy mozna sprzedac - w zaleznosci od tego, czy jest
 *		   pusta czy nie.
 * Funkcja zwraca: 	0 - mozna sprzedac,
 *		   string - nie mozna sprzedac, wyjasnienie dlaczego.
 */
public mixed
query_mozna_sprzedac()
{
    return (ilosc_plynu ? "Nie kupujemy pelnych " + plural_short(PL_DOP) + ".\n" 
			: 0);
}

public void
init()
{
     ::init();
     
     add_action(napij, "napij");
     add_action(wylej, "wylej");
}

/*
 * Nazwa funkcji : napij
 * Opis          : Wywolywana, gdy gracz wpisze komende 'napij'. Sprawdza
 *		   z czego gracz chce sie napic i wywoluje drink_from_it()
 *		   w odpowiedniej beczulce. Zwraca to co zwroci 
 *		   drink_from_it().
 * Argumenty     : string - argument do komendy.
 * Funkcja zwraca: W zaleznosci od rezultatu funkcji ob->drink_from_it(),
 *			1 - komenda przyjeta, lub
 *			0 - nie przyjeta, szukaj w innym obiekcie.
 */
private int
napij(string str)
{
    object *obs;
    int ret;
    
    notify_fail("Napij sie z czego?\n");
    
    if (!strlen(str))
        return 0;
    
    if (!parse_command(str, all_inventory(this_player()), 
        "'sie' 'z' %i:" + PL_DOP, obs))
        return 0;
    
    obs = NORMAL_ACCESS(obs, 0, 0);
    
    if (!sizeof(obs))
        return 0;
        
    if (sizeof(obs) > 1)
    {
        notify_fail("Mozesz sie napic z tylko jednej rzeczy naraz.\n");
        return 0;
    }
    
    ret = obs[0]->drink_from_it(this_player());
    if (ret)
	this_player()->set_obiekty_zaimkow(obs);
    return ret;
}

/*
 * Nazwa funkcji : wylej
 * Opis          : Wywolywana, gdy gracz wpisze komende 'wylej'. Sprawdza
 *		   z czego gracz chce wylac zawartosc i wywoluje
 *		   wylej_from_it() w odpowiedniej beczulce. Zwraca to co
 *		   zwroci wylej_from_it().
 * Argumenty     : string - argument do komendy.
 * Funkcja zwraca: W zaleznosci od rezultatu funkcji ob->wylej_from_it(),
 *			1 - komenda przyjeta, lub
 *			0 - nie przyjeta, szukaj w innym obiekcie.
 */
public int
wylej(string str)
{
    object *obs;
    int ret;
    
    notify_fail("Wylej co? Moze zawartosc " + short(PL_DOP) + ".\n");
    
    if (!strlen(str))
        return 0;
        
    if (!parse_command(str, all_inventory(this_player()), 
        "'zawartosc' %i:" + PL_DOP, obs))
        return 0;
    
    obs = NORMAL_ACCESS(obs, 0, 0);
    
    if (!sizeof(obs))
        return 0;
        
    if (sizeof(obs) > 1)
    {
        notify_fail("Mozesz wylac zawartosc tylko jednej rzeczy naraz.\n");
        return 0;
    }

    ret = obs[0]->wylej_from_it();
    if (ret)
	this_player()->set_obiekty_zaimkow(obs);

    return ret;
}


/*
 * Nazwa funkcji : drink_from_it
 * Opis          : Gracz pije konkretnie z tej beczulki. Pije albo nie,
 *		   wypisuje odpowiednie teksty.
 * Funkcja zwraca: 1 - komenda przyjeta.
 */
public int
drink_from_it()
{
    int lyk;
    string str;
    
    if (!ilosc_plynu)
    {
        write(capitalize(short()) + " jest zupelnie pust" + 
            koncowka("y", "a", "e") + ".\n");
        return 1;
    }
    
    lyk = MIN(this_player()->drink_max() / 20, this_player()->drink_max() - 
    		this_player()->query_soaked());
    if (!lyk)
    {
        write("Nie wmusisz w siebie ani kropelki wiecej.\n");
        return 1;
    }
    
    lyk = MIN(ilosc_plynu, lyk);
    
    if (!this_player()->drink_alco((lyk * procent_alco) / 100, 0))
    {
        write("Trunek w " + short(PL_MIE) + " jest taki mocny, ze chyba " +
            "nie dasz rady go wypic...\n");
        return 1;
    }
    
    if (!this_player()->drink_soft(lyk, 0))
    {
         write("Nie wmusisz w siebie ani kropelki wiecej.\n");
         return 1;
    }
    
    ilosc_plynu -= lyk;
    
    if (!ilosc_plynu)
        str = ", oprozniajac " + koncowka("go", "ja", "je") + " zupelnie.\n";
    else
        str = ".\n";
        
    write("Pijesz troche " + nazwa_dop + " z " + short(PL_DOP) + str);
    say(QCIMIE(this_player(), PL_MIA) + " pociaga troche " + nazwa_dop + 
        " z " + QSHORT(this_object(), PL_DOP) + str);
        
    if (!ilosc_plynu)
    {
        procent_alco = 0;
        opis_plynu = "";
        nazwa_dop = "";
    }
        
    return 1;
}

/*
 * Nazwa funkcji : wylej_from_it
 * Opis          : Gracz chce wylac konkretnie z tej beczulki. Wylewa albo
 *		   nie, wypisuje odpowiednie teksty.
 * Funkcja zwraca: 1 - komenda przyjeta.
 */
public int
wylej_from_it()
{
    if (!ilosc_plynu)
    {
        write(capitalize(short()) + " jest zupelnie pust" + 
            koncowka("y", "a", "e") + ".\n");
        return 1;
    }
    
    write("Wylewasz cala zawartosc " + short(PL_DOP) + ", w postaci " + 
        opis_plynu + " na ziemie.\n");
    say(QCIMIE(this_player(), PL_MIA) + " wylewa cala zawartosc " +
        QSHORT(this_object(), PL_DOP) + ", w postaci " + opis_plynu + 
        " na ziemie.\n");
        
    ilosc_plynu = 0;
    procent_alco = 0;
    opis_plynu = "";
    nazwa_dop = "";
    
    return 1;
}

public string
long()
{
    string str;
    
    str = (dlugi_opis ? dlugi_opis : "Jest to " + short() + " sluzac" + 
	koncowka("y", "a", "e") + " do przechowywania w " + 
	koncowka("nim", "niej") + " roznych trunkow.") + 
	" W tej chwili ";
        
    switch(100 * ilosc_plynu / pojemnosc_naczynia)
    {
        case 0: str += "jest zupelnie pust" + koncowka("y", "a", "e"); break;
        case 1..20: str += "zawiera mala ilosc " + opis_plynu; break;
        case 21..50: str += "zawiera troche " + opis_plynu; break;
        case 51..70: str += "zawiera sporo " + opis_plynu; break;
        case 71..90: str += "jest prawie peln" + koncowka("y", "a", "e") +
            " " + opis_plynu; break;
        default: str += "jest peln" + koncowka("y", "a", "e") + " " +
            opis_plynu; break;
    }
    
    str += ".\n";
    
    return str;
}

/*
 * Nazwa funkcji : set_dlugi_opis
 * Opis          : Ustawia dlugi opis beczulki. Do opisu ustawionego
 *		   przez te funkcje dodawany jest opisik o zawartosci
 *		   pojemnika.
 * Argumenty     : string opis - opis.
 */
public void
set_dlugi_opis(string opis)
{
    dlugi_opis = opis;
}

/*
 * Nazwa funkcji : query_dlugi_opis
 * Opis          : Zwraca dlugi opis beczulki ustawiony przez wiza.
 * Funkcja zwraca: string - dlugi opis.
 */
public string
query_dlugi_opis()
{
    return dlugi_opis;
}

/*
 * Nazwa funkcji : set_pojemnosc
 * Opis          : Sluzy do ustawiania pojemnosci beczulki w mililitrach.
 * Argumenty     : int - pojemnosc w mililitrach.
 */
public void
set_pojemnosc(int pojemnosc)
{
    pojemnosc_naczynia = pojemnosc;
}

/*
 * Nazwa funkcji : query_pojemnosc
 * Opis          : Zwraca pojemnosc beczulki w mililitrach.
 * Funkcja zwraca: int - pojemnosc w mililitrach.
 */
public int
query_pojemnosc()
{
    return pojemnosc_naczynia;
}

/*
 * Nazwa funkcji : set_ilosc_plynu
 * Opis          : Sluzy do ustawienia ile ml plynu znajduje sie akt w
 *		   pojemniku.
 * Argumenty     : int - liczba mililitrow plynu.
 */
public void
set_ilosc_plynu(int ilosc)
{
    if (ilosc > pojemnosc_naczynia)
        ilosc = pojemnosc_naczynia;
        
    ilosc_plynu = ilosc;
}

/*
 * Nazwa funkcji : query_ilosc_plynu
 * Opis          : Zwraca ile aktualnie ml. plynu znajduje sie w pojemniku.
 * Funkcja zwraca: int - patrz wyzej.
 */
public int
query_ilosc_plynu()
{
    return ilosc_plynu;
}

/*
 * Nazwa funkcji : set_nazwa_plynu_dop
 * Opis          : Ustawia nazwe plynu w dopelniaczu. Nazwa jest
 *		   wykorzystywana w komunikatach wysylanych graczowi,
 *		   np. przy piciu.
 * Argumenty     : string - nazwa plynu w dopelniaczu.
 */
public void
set_nazwa_plynu_dop(string str)
{
    nazwa_dop = str;
}

/*
 * Nazwa funkcji : query_nazwa_plynu_dop
 * Opis          : Zwraca nazwe plynu, ktory znajduje sie akt. w beczulce
 *		   w dopelniaczu.
 * Funkcja zwraca: string - patrz wyzej.
 */
public string
query_nazwa_plynu_dop()
{
    return nazwa_dop;
}

/*
 * Nazwa funkcji : set_opis_plynu
 * Opis          : Sluzy do ustawienia dlugiego opisu plynu znajdujacego
 *		   sie akt. w beczulce. Powinien on byc w dopelniaczu.
 *		   Wykorzystywany jest np. w dlugim opisie beczulki.
 * Argumenty     : string - dlugi opis w dopelaniczu.
 */
public void
set_opis_plynu(string opis)
{
    opis_plynu = opis;
}

/*
 * Nazwa funkcji : query_opis_plynu
 * Opis          : Zwraca dlugi opis plynu znajdujacego sie akt. w beczulce,
 *		   w dopelniaczu.
 * Funkcja zwraca: string - patrz wyzej.
 */
public string
query_opis_plynu()
{
    return opis_plynu;
}

/*
 * Nazwa funkcji : set_vol
 * Opis          : Ustawia ile procent alkoholu jest w cieczy, ktora
 *		   jest w beczulce.
 * Argumenty     : int - procent alkoholu
 */
public void
set_vol(int vol)
{
    procent_alco = vol;
}

/*
 * Nazwa funkcji : query_vol
 * Opis          : Zwraca jak bardzo procentowy jak napoj w beczulce.
 * Funkcja zwraca: int - ile procent alkoholu jest w plynie w beczulce.
 */
public int
query_vol()
{
    return procent_alco;
}
