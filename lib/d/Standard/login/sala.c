#include "login.h"
#include <ss_types.h>

inherit "/std/room.c";

#pragma strict_types

#include <std.h>
#include <stdproperties.h>
#include <macros.h>

public int dotknij(string str);
public int portal(string str);

public void
create_room()
{
    set_short("Wielka kamienna hala z figurami");
    
    set_long("Znajdujesz sie w kamiennej, slabo oswietlonej sali, " +
        "opartej na planie polkola. Sciany, podloga i sufit " +
        "zbudowane sa z szarego, zupelnie gladkiego, zimnego kamienia. " +
        "Spod polokraglej sciany wpatruje sie w ciebie nieruchomo piec " +
        "symetrycznie rozmieszczonych kamiennych figur. Wydaja sie byc " +
        "wykute dokladnie z tego samego materialu, co reszta pomieszczenia. " +
        "Kazda trzyma w reku zapalona pochodnie, ktore to stanowia jedyne " +
        "zrodlo swiatla w tej sali, rzucajac krwawy blask na sciany i na " +
        "szare oblicza swych wlascicieli. Po przeciwnej stronie, na " +
        "jedynej plaskiej scianie, widac przejscie prowadzace do innego, " +
        "nieco bardziej oswietlonego pomieszczenia. Natomiast na " +
        "samym srodku sali znajduje sie dziwny obiekt, cos jakby portal " +
        "lub framuga. Byc moze spelnia funkcje czysto dekoracyjna, " +
        "ale ten dziwny, delikatny ruch powietrza przezen przechodzacy... " +
        "Wszedzie wokol zdaje sie unosic pyl tak nieuchwytny jak poranna " +
        "mgielka, nadajac temu miejscu wrazenie antycznosci. Panuje tu " +
        "cisza, jesli nie liczyc skwierczenia plonacych pochodni i dziwnego " +
        "halasu dochadzacego od czasu do czasu zza przejscia w poludniowej " +
        "scianie.\n");
        
    add_item( ({ "sale", "pomieszczenie", "miejsce", "lokacje" }),
        "Czerwony, migotliwy blask plomieni pochodni, figury, portal, " +
        "ten dziwny przeciag oraz wciaz unoszacy sie pyl nie tworza " +
        "razem takiej sobie, zwyklej kompozycji. Ale ktozby sie spodziewal " +
        "zwyczajnosci po takim miejscu...\n");
    add_item( ({ "pochodnie", "luczywa" }), "Dziwne w nich jest to, ze " +
        "zdaja sie byc na stale przymocowane do dloni swych wlascicieli, " +
        "tworzac jakby czesc rzezby. Czyzby wiec plonely wiecznie?\n");
    add_item( ({ "podloge", "sufit", "sciany" }), "Bardzo " + 
        "charakterystyczna jest gladkosc powierzchni... Czyzby robota " +
        "krasnoludzkich mistrzow? Nie... W glebi duszy masz wrazenie, " +
        "ze w tym miejscu nie postala nigdy stopa zadnego smiertelnika.\n");
    add_item( ({ "przejscie", "drzwi", "drugie pomieszczenie", 
        "drugi pokoj", "oswietlony pokoj" }), "Za prostokatnym " +
        "przejsciem w jedynej plaskiej scianie, znajduje sie jakas " +
        "druga sala. Jest tam nieco jasniej, zas samo swiatlo ma kolor " +
        "bardziej bialy. Nie widzisz tam nic dokladnie, ale masz wrazenie, " +
        "ze na podlodze jest przycupnieta jakas zgarbiona postac.\n");
    add_item( ({ "pyl", "mgielke" }), "Pyl jest tak drobny, ze go nawet " +
        "nie widac. O jego istnieniu swiadczy jedynie to, ze sie " +
        "na nim delikatnie rozprasza swiatlo pochodni. A moze to tylko " +
        "zludzenie?\n");
    add_item( ({ "portal", "framuge" }), "Przez portal z obu stron widac " +
        "reszte pomieszczenia - pod tym wzgledem jest calkiem normalny. " +
        "Tylko po co stoi na samym srodku sali i dlaczego powietrze " + 
        "zdaje sie przez niego wciaz przelatywac? Mozna by sprobowac " +
        "przez niego przejsc...\n");
    add_item( ({ "figury", "postacie", "statuy", "rzezby" }),
        "Kazda figura jest inna. Tej twarz jest wesola, ta ma tepy wyraz, " +
        "ta zas jest wysoka, a ta krepa. Kazda przedstawia osobnika " + 
        "innej rasy. Pierwsza od lewej przedstawia ogra, druga halflinga, " +
        "srodkowa czlowieka, czwarta od lewej krasnoluda, zas piata elfa. " +
        "Mozna by sie przyjrzec kazdej z nich z osobna.\n");
    add_item( ({ "elfa", "figure elfa", "postac elfa", "piata figure",
        "piata postac" }), "Rzezba mierzy prawie dwa metry wzrostu. " +
        "Przedstawia piekna, smukla postac, o lagodnych, pociaglych rysach " +
        "twarzy. W porownaniu z innymi, elf nie sprawia wrazenia " +
        "najsilniejszego, ale za to harmonijna budowa swiadczy o jego " +
        "duzej zwinnosci. Ma niezdecydowany wyraz twarzy, zas z jego " +
        "oczu bije gleboka madrosc, bystrosc i swiezosc umyslu.\n");
    add_item( ({ "ogra", "figure ogra", "postac ogra", "pierwsza figure",
        "pierwsza postac" }), "Jest to najpotezniejsza ze wszystkich " +
        "postaci, gdyz miezy prawie dwa i pol metra wzrostu! Rozrostowi " +
        "wzwyz towarzyszy rozrost wszerz. Przedstawiciel tej rasy musi " +
        "miec nie lada krzepe! Niestety z takimi rozmiarami wiaze " +
        "sie takze mniejsza ruchliwosc. Z ust, a wlasciwie z pyska " +
        "wystaja dwa lekko zakrzywione kly. Nieco wyzej, wpatruje sie w " +
        "ciebie para bezdennie glupich oczu. Postawa ogra wyraza niezwykla " +
        "brawure, prawdopodobnie wynikajaca z niezbyt rozwinietego " +
        "intelektu.\n");
    add_item( ({ "halflinga", "postac halflinga", "figure halflinga",
        "druga figure", "druga postac" }), "Zdecydowanie najnizsza " +
        "postac, mierzy niewiele ponad metr. To, co nie poszlo we wzrost " +
        "poszlo widocznie w szerokosc, a dokladniej mowiac grubosc. " +
        "Ten osobnik niewatpliwie jest dobrze odzywiony. O dziwo, mimo " +
        "to, wydaje sie byc niezwykle zwinnym i ruchliwym stworzeniem. " +
        "Ma bardzo przyjazny i inteligentny wyraz twarzy. Ta rasa musi " +
        "byc pokojowo usposobiona - na pierwszy rzut oka widac, ze halfling " +
        "nie jest nawykly do zbytniego wysilku.\n");
    add_item( ({ "krasnoluda", "postac krasnoluda", "figure krasnoluda",
        "czwarta figure", "czwarta postac" }), "Rzezba krasnoluda " +
        "przedstawia stosunkowo niska, krepa postac. Jak na swoj wzrost " +
        "jest bardzo szeroka w barach, po czym widac, ze dysponuje " +
        "niemala sila. Widac rowniez, ze nie nalezy do najzwinniejszych " +
        "istot - krasnolud predzej przyjalby na siebie cios bez szwanku, niz " +
        "probowal go uniknac. Na twarzy ma wymalowana odwage i upor, zas z " +
        "calej sylwetki emanuje hart ducha. Nie sposob nie wspomniec " +
        "o dlugiej brodzie, dodajacej postaci majestatycznosci, bedacej " +
        "przedmiotem dumy krasnoluda.\n");    
    add_item( ({ "czlowieka", "postac czlowieka", "figure czlowieka",
        "trzecia figure", "trzecia postac", "srodkowa figure",
        "srodkowa postac" }), "Figura przedstawia postac sredniego " +
        "wzrostu, troche nizsza od figury elfa, lecz duzo wyzsza " +
        "od krasnoluda. Ciezko jest cokolwiek wywnioskowac z budowy " +
        "czy postawy postaci; widocznie ludzie bywaja bardzo " +
        "rozni. Budowa fizyczna nie wyroznia sie niczym szczegolnym, " +
        "stanowi jakby usrednienie wszystkich pozostalych ras. " +
        "Prawdopodobnie dlatego architekci tego dziwnego miejsca " +
        "umiescili rzezbe czlowieka posrodku.\n");
        
    add_exit(PATH + "cechy.c", ({ "poludnie", "przez przejscie w " +
        "poludniowej scianie"}));

    add_prop(ROOM_I_INSIDE, 1);
    add_prop(ROOM_I_LIGHT, 1);

}

public void
pomoc(string str)
{
    write("Aby stworzyc postac, musisz wpierw przyobrac sobie jakas " +
        "rase. W tej sali znajduje sie piec rzezb - kazda z wyrzezbionych " +
        "postaci jest przedstawicielem innej rasy. Mozesz obejrzec kazda " +
        "z nich, by dowiedziec sie troszeczke wiecej o danej rasie. " +
        "Do blizszego ogladania rzeczy sluzy komenda 'obejrzyj', lub " +
        "w skrocie 'ob'. Prawidlowe komendy to na przyklad: 'obejrzyj " +
        "portal', 'obejrzyj sale', 'obejrzyj figury', 'obejrzyj postac " +
        "czlowieka', 'obejrzyj pierwsza figure', itd.\n" +
        "  Kiedy juz sie zdecydujesz na jakas rase (jesli do tej pory nie " +
        "zetknales sie z psychologia innych ras, najlepiej bedzie jesli " +
        "wybierzesz czlowieka), 'dotknij' figury, ktora ja przedstawia.\n" + 
        "  Pozniej udaj sie do pomieszczenia, do ktorego prowadzi " +
        "jedyne stad wyjscie. Tam dokreujesz swoja postac. Kiedy juz " +
        "uznasz, ze jest ona gotowa, sprobuj przejsc przez portal.\n");
}

public void
init()
{
    ::init();
    add_action(&dotknij(), "dotknij");
    add_action(&portal(), "przejdz");
    add_action(&portal(), "wejdz");
    add_action("komendy", "", 1);
}

public int
mozna_dotknac()
{
    return (this_player()->query_ghost());
}

private void
wybral_rase(object kto, string dop)
{
    kto->catch_msg("Plomien pochodni trzymanej przez " + dop +
        " rozblyska silniej na moment. Przebiega cie niesamowity " +
        "dreszcz.\n");
    return ;
}

private void
dodaj_skille()
{
    mapping skille;
    string *tab;
    int x;
    
    skille = RACESKILL[this_player()->query_race_name()];
    tab = m_indices(skille);
    
    x = sizeof(tab);
    while (--x >= 0)
        this_player()->set_skill(tab[x], skille[tab[x]]);
}

public int
dotknij(string str)
{
    string dop;
    
    if (!str)
    {
        write("Dotknij czego?\n");
        return 1;
    }
        
    if (str == "rzezby" || str == "postaci" || str == "figury")
    {
        write("Dotknij ktorej " + str + "?\n");
        return 1;
    }
    
    switch (str)
    {
        case "pierwszej figury":
        case "pierwszej postaci":
        case "postaci ogra":
        case "figury ogra":
        case "ogra": if (!mozna_dotknac()) break;
                     this_player()->set_race_name("ogr"); 
        	     dop = "ogra";
        	     break;
        case "drugiej figury":
        case "drugiej postaci":
        case "postaci halflinga":
        case "figury halflinga":
        case "halflinga": if (!mozna_dotknac()) break;
        		  this_player()->set_race_name("halfling"); 
        		  dop = "halflinga";
        		  break;
        case "trzeciej figury":
        case "trzeciej postaci":
        case "postaci czlowieka":
        case "figury czlowieka":
        case "srodkowej postaci":
        case "srodkowej figury":
        case "czlowieka": if (!mozna_dotknac()) break;
        		  this_player()->set_race_name("czlowiek"); 
        		  dop = "czlowieka";
        		  break;
        case "czwartej figury":
        case "czwartej postaci":
        case "postaci krasnoluda":
        case "figury krasnoluda":
        case "krasnoluda":if (!mozna_dotknac()) break; 
        		  this_player()->set_race_name("krasnolud"); 
        		  dop = "krasnoluda";
        		  break;
        case "piatej figury":
        case "piatej postaci":
        case "postaci elfa":
        case "figury elfa":
        case "elfa": if (!mozna_dotknac()) break; 
        		  this_player()->set_race_name("elf"); 
        		  dop = "elfa";
        		  break;
        default: write("Dotknij czego?\n"); return 1;
    }
        
    if (!dop)
        write("Dotykasz rzezby, lecz nic sie nie dzieje. Decyzja co do " +
            "wyboru rasy juz dawno zapadla - niepodobna ja teraz zmienic.\n");
    else
    {
	write("Ostroznie dotykasz figury " + dop + "...\n");
	dodaj_skille();
	set_alarm(2.0, 0.0, &wybral_rase(this_player(), dop));
    }
	
    return 1;
    
}

public void
gotow(object gracz)
{
    if (gracz->query_player_file() == LOGIN_NEW_PLAYER)
    {
        gracz->ghost_ready();
        tell_room(this_object(), "... i znika!\n");
        return ;
    }
    
    tell_room(this_object(), "... I nic sie nie dzieje.\n");
    
        
}

public int
portal(string str)
{
    if (query_verb() == "wejdz")
    {
	if (str != "w portal" && str != "przez portal")
	{
	    notify_fail("Wejdz w co?\n");
	    return 0;
	}
    }
    else if (str != "przez portal")
    {
	notify_fail("Przejdz przez co?\n");
	return 0;
    }

    say(QCIMIE(this_player(), PL_MIA) + " przechodzi przez portal i...\n");
    
    if (this_player()->query_ghost())
    {
        write("Ostroznie przechodzisz przez portal, ale zupelnie nic " +
            "sie nie dzieje. Widac jest to zwykla framuga, element " +
            "dekoracyjny. W glebi duszy, cos ci jednak mowi, ze czegos " +
            "musisz wpierw dokonac... Moze musisz przybrac cielesna " +
            "postac?\n");
        return 1;
    }
    
    write("Ostroznie przechodzisz przez portal, i...\n");
    
    set_alarm(1.5, 0.0, &gotow(this_player()));
    
    return 1;
}

public int
komendy(string str)
{
    if (SECURITY->query_wiz_rank(this_player()->query_real_name()) >= WIZ_ARCH)
        return 0;
        
    switch(query_verb())
    {
        case "sp":
        case "spojrz":
        case "ob":
        case "obejrzyj":
        case "l":
        case "look":
        case "save":
        case "nagraj":
        case "quit":
        case "zakoncz":
        case "zglos":
        case "bug":
        case "praise":
        case "idea":
        case "goto":
        case "Goto":
        case "polnoc":
        case "polnocny-zachod":
        case "zachod":
        case "poludniowy-zachod":
        case "poludnie":
        case "poludniowy-wschod":
        case "wschod":
        case "polnocny-wschod":
        case "home": return 0;
        case "?": this_object()->pomoc(str); return 1;
        case "dotknij":
        case "przejdz":
        case "wejdz": return 0;
        default: write("To nie jest mozliwe w tym miejscu. Wpisz '?', zeby " +
            "uzyskac pomoc.\n"); return 1;
    }

}
