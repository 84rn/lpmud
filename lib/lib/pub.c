/*
 * Support for selling food and drinks
 *
 * Made by Nick
 *
 * Sierpien 96: Generalne spolszczenie /Alvin
 * Maj 98: System zarcia i picia na wynos. Reforma wszystkiego.
 */

#pragma strict_types
#pragma save_binary

#include <stdproperties.h>
#include <macros.h>
#include <money.h>
#include <language.h>
#include <files.h>

static	mixed	drinks;	/* Array holding all drink data */
static	mixed	dr_id;	/* Array holding all names the drinks can be identified
			 * with when bought */
static	mixed	food;
static	mixed	fo_id;

int order(string str);

/*
 * Function name: init_pub
 * Description:   This function adds the buy and order command to this_player()
 *		  call it from your init()
 */
void
init_pub()
{
    add_action(order, "kup");
    add_action(order, "zamow");
}

/*
 * Function name: pub_hook_cant_pay
 * Description:   A hook to redefine if you want own message when player can't 
 *		  pay the price.
 * Arguments:     price - The price he should had payed
 */
void
pub_hook_cant_pay(int price)
{
    write("Nie masz wystarczajacej ilosci pieniedzy, zeby zaplacic.\n");
}

/*
 * Function name: pub_hook_cant_carry
 * Description:   A hook to redefine if you want own message when player can't
 *		  carry what he ordered.
 * Arguments:     ob - The object he couldn't carry
 */
void
pub_hook_cant_carry(object ob)
{
    write(capitalize(ob->short()) + " is too heavy for you. You drop it " +
	"to the ground.\n");
    say(capitalize(ob->short()) + " is too heavy for " + QTNAME(this_player()) +
	" and falls to the ground.\n");
}

/*
 * Nazwa funkcji : pub_hook_buys_food_na_miejscu
 * Opis          : Funkcja wypisujaca odpowiednie teksty, gdy gracz
 *		   kupuje jedzenie na miejscu. Mozesz ja przedefiniowac,
 *		   jesli chcesz zastapic standardowe teksty swoimi.
 * Argumenty     : string short - krotki opis jedzenia w bierniku,
 *		   int    cena	- cena wyrazona w miedziakach.
 */
public void
pub_hook_buys_food_na_miejscu(string short, int cena)
{
    write("Zamawiasz " + short + ", placac " + cena + " miedziak" + 
        (cena == 1 ? "a" : (cena % 10 < 5 && cena % 10 > 1 && 
        (cena % 100)/10 != 1) ? "i" : "ow") + ".\n" +
        "Po chwili otrzymujesz i zjadasz zamowiona potrawe.\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " zamawia i zjada na miejscu " +
	  short + ".\n");
}

/*
 * Nazwa funkcji : pub_hook_buys_food_na_wynos
 * Opis          : Funkcja wypisujaca odpowiednie teksty, gdy gracz
 *		   kupuje jedzenie na wynos. Mozesz ja przedefiniowac,
 *		   jesli chcesz zastapic standardowe teksty swoimi.
 * Argumenty     : object ob    - obiekt zakupionego jedzenia,
 *		   int    cena	- cena wyrazona w miedziakach.
 */
public void
pub_hook_buys_food_na_wynos(object ob, int cena)
{
    write("Kupujesz " + ob->short(this_player(), PL_BIE) + 
        ", placac " + cena + " miedziak" + 
        (cena == 1 ? "a" : (cena % 10 < 5 && cena % 10 > 1 && 
        (cena % 100)/10 != 1) ? "i" : "ow") + ".\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " kupuje " + QSHORT(ob, PL_BIE) + 
          ".\n");
}

/*
 * Nazwa funkcji : pub_hook_buys_drink_na_miejscu
 * Opis          : Funkcja wypisujaca odpowiednie teksty, gdy gracz
 *		   kupuje picie na miejscu. Mozesz ja przedefiniowac,
 *		   jesli chcesz zastapic standardowe teksty swoimi.
 * Argumenty     : string short - krotki opis picia w bierniku,
 *		   int    cena	- cena wyrazona w miedziakach.
 */
public void
pub_hook_buys_drink_na_miejscu(string short, int cena)
{
    write("Zamawiasz " + short + ", placac " + cena + " miedziak" + 
        (cena == 1 ? "a" : (cena % 10 < 5 && cena % 10 > 1 && 
        (cena % 100)/10 != 1) ? "i" : "ow") + ".\n" +
        "Po chwili otrzymujesz i wypijasz zamowiony trunek.\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " zamawia i po chwili wypija " +
	  short + ".\n");
}

/*
 * Nazwa funkcji : pub_hook_buys_drink_na_wynos
 * Opis          : Funkcja wypisujaca odpowiednie teksty, gdy gracz
 *		   kupuje picie na wynos. Mozesz ja przedefiniowac,
 *		   jesli chcesz zastapic standardowe teksty swoimi.
 * Argumenty     : object ob    - obiekt zakupionej beczulki z piciem,
 *		   int    cena	- cena wyrazona w miedziakach.
 */
public void
pub_hook_buys_drink_na_wynos(object ob, int cena)
{
    string str;

    str = ob->short(PL_BIE) + " pel" + ob->koncowka("en", "na", "ne") +
        " " + ob->query_opis_plynu();

    write("Nabywasz " + str + ", placac " + cena + " miedziak" + 
       (cena == 1 ? "a" : (cena % 10 < 5 && cena % 10 > 1 && 
       (cena % 100)/10 != 1) ? "i" : "ow") + ".\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " kupuje " + str + ".\n");
}

public int
food_na_miejscu(int i, int num)
{
    int cena, ix;
    string short;
    
    if (num > 1)
    {
	write("Hola, zamawiaj pojedyczno.\n");
	return 1;
    }
    
    cena = food[i][4];
    
    if (!this_player()->eat_food(food[i][3], 1))
    {
	write("Nie sadzisz, zebys byl" + this_player()->koncowka("", "a") +
	    " w stanie zjesc tyle.\n");
	return 1;
    }

    if (!MONEY_ADD(this_player(), -cena))
    {
	pub_hook_cant_pay(cena);
	return 1;
    }
    
    this_player()->eat_food(food[i][3], 0);

    if (pointerp(food[i][8]))
        short = food[i][8][PL_BIE];
    else
    if (stringp(food[i][8]))
	short = food[i][8];
    else
    {
	short = "";
	ix = (food[i][2] ? sizeof(food[i][2][0]) : 0);
	while (--ix >= 0)
	    short += oblicz_przym(food[i][2][0][ix], food[i][2][1][ix],
		PL_BIE, food[i][6][0], 0) + " ";
	short += fo_id[i][0][0];
    }
    
    pub_hook_buys_food_na_miejscu(short, cena);
    
    return 1;
}

public int
food_na_wynos(int i, int num)
{
    int cena, ix;
    object ob;
    
    if (!food[i][0])
    {
        write("Tej potrawy nie mozna wziac na wynos. Mozesz ja jedynie " +
            "'zamowic' i zjesc na miejscu.\n");
        return 1;
    }

    cena = num * food[i][4];
    
    if (!MONEY_ADD(this_player(), -cena))
    {
	pub_hook_cant_pay(cena);
	return 1;
    }

    seteuid(getuid(this_object()));
    ob = clone_object(FOOD_OBJECT);

    ix = sizeof(food[i][0]) / 6;
    while (--ix >= 0)
    {
	ob->ustaw_nazwe(food[i][0][(ix * 6)..(ix * 6 + 5)], 
	    food[i][1][(ix * 6)..(ix * 6 + 5)], food[i][6][ix]);
    }
	    
    if (sizeof(food[i][2]) == 2)
    {
	ix = sizeof(food[i][2][0]);
	while (--ix >= 0)
	    ob->dodaj_przym(food[i][2][0], food[i][2][1]); 
    }

    if (food[i][8] && food[i][9])
	ob->ustaw_shorty(food[i][8], food[i][9], food[i][6][0]);
    ob->set_amount(food[i][3]);
    ob->set_long(food[i][5]);
    ob->add_prop(HEAP_I_UNIT_VOLUME, food[i][3]);
    ob->add_prop(HEAP_I_UNIT_WEIGHT, food[i][3]);
    ob->add_prop(HEAP_I_UNIT_VALUE, food[i][4]);
    
    ob->set_decay_time(food[i][7]);
    if (sizeof(food[i][10]) == 2)
        ob->set_decay_adj(food[i][10]);
    
    ob->set_heap_size(num);
    ob->move(this_player(), 1);
    ob->start_decay();

    pub_hook_buys_food_na_wynos(ob, cena);
    
    return 1;
}

public int
drink_na_miejscu(int i, int num, int wynos)
{
    int cena, alco;
    string short;

    if (wynos)
    {
         write((wynos == 1 ? "Beczulke" : "Buklak") + " mozesz 'kupic' " +
             "tylko na wynos.\n");
         return 1;
    }

    if (num > 1)
    {
	write("Hola, zamawiaj pojedyczno.\n");
	return 1;
    }
    
    cena = drinks[i][6] * drinks[i][8] / 1000;
    alco = drinks[i][5] * drinks[i][6] / 100;
    
    if (!this_player()->drink_alco(alco, 1) ||
        !this_player()->drink_soft(drinks[i][6], 1))
    {
	write("Nie sadzisz, zebys byl" + this_player()->koncowka("", "a") +
	    " w stanie wypic tyle.\n");
	return 1;
    }

    if (!MONEY_ADD(this_player(), -cena))
    {
	pub_hook_cant_pay(cena);
	return 1;
    }
    
    this_player()->drink_alco(alco, 0);
    this_player()->drink_soft(drinks[i][6], 0);

    short = drinks[i][2];
    
    pub_hook_buys_drink_na_miejscu(short, cena);

    return 1;
}

public int
drink_na_wynos(int i, int num, int wynos)
{
    object ob;
    string str;
    int cena, cena_pojemnika;

    if (wynos && !drinks[i][7])
    {
         write("Nie sprzedajemy " + drinks[i][0][0] + " na wynos.\n");
         return 1;
    }

    if (!wynos)
    {
        if (drinks[i][7])
            write("Nie sprzedajemy " + drinks[i][0][0] + " (na wynos) tak " +
                "po prostu. Mozesz " + (drinks[i][9][0] > PL_ZENSKI ? "je" :
                drinks[i][9][0] == PL_ZENSKI ? "ja" : "go") + 
                (drinks[i][7] ? " 'kupic' w " + (drinks[i][7] >= 5 ? 
                "beczulce" : "buklaku") + ", lub" : "") +
                " 'zamowic' (na miejscu).\n");
        else
            write("Nie sprzedajemy " + drinks[i][0][0] + " na wynos. Mozesz " +
                (drinks[i][9][0] > PL_ZENSKI ? "je" : 
                drinks[i][9][0] == PL_ZENSKI ? "ja" : "go") + " 'zamowic' " +
                "(na miejscu).\n");
        return 1;
    }
    
    if ((wynos == 1 && drinks[i][7] < 5) || 
        (wynos == 2 && drinks[i][7] >= 5))
    {
        write("Nie sprzedajemy " + drinks[i][0][0] + " w " +
             (drinks[i][7] >= 5 ? "buklakach, tylko w beczulkach"
                                : "beczulkach, tylko w buklakach") + 
             ".\n");
        return 1;
    }
    
    switch(drinks[i][7])
    {
        case 1..4: cena_pojemnika = (30 + 4 * drinks[i][7]);
		   break;
	default:   cena_pojemnika = (140 + 10 * drinks[i][7]);
                   break;
    }

    cena = drinks[i][8] * drinks[i][7] + cena_pojemnika;
    if (!MONEY_ADD(this_player(), -cena))
    {
	pub_hook_cant_pay(cena);
	return 1;
    }

    ob = clone_object(BECZULKA_OBJECT);
    ob->set_pojemnosc(drinks[i][7] * 1000);
    ob->set_opis_plynu(drinks[i][3]);
    ob->set_nazwa_plynu_dop(drinks[i][0][0]);
    ob->set_vol(drinks[i][5]);
    ob->set_ilosc_plynu(drinks[i][7] * 1000);
    if (drinks[i][10])
        ob->set_dlugi_opis(drinks[i][10]);
        
    switch(drinks[i][7])
    {
        case 1..4: ob->ustaw_nazwe( ({ "buklak", "buklaka", "buklakowi",
		       "buklak", "buklakiem", "buklaku" }),
		    ({ "buklaki", "buklakow", "buklakom", "buklaki",
		        "buklakami", "buklakach" }), PL_MESKI_NOS_NZYW);
		   break;
	default:   ob->ustaw_nazwe( ({ "beczulka", "beczulki", "beczulce", "beczulke",
                       "beczulka", "beczulce" }), ({ "beczulki", "beczulek",
                       "beczulkom", "beczulki", "beczulkami", "beczulkach" }),
                       PL_ZENSKI);
                   break;
    }
    ob->add_prop(OBJ_I_VALUE, cena_pojemnika);

    ob->move(this_player(), 1);

    pub_hook_buys_drink_na_wynos(ob, cena);
    
    return 1;
}

/*
 * Function name: order
 * Description:   The player has ordered something, let's see if we can satisfy
 *		  him.
 * Arguments:	  str - The order from the player
 * Returns:	  1 or 0
 */
int
order(string str)
{
    string *words, *przym, produkt, prz;
    int num = 1, tmp, i, price, przyp, przyp_gen, size, ix, wynos, x, y;
    object ob;

    if (!str)
    {
	notify_fail(capitalize(query_verb()) + " co?\n");
	return 0;
    }

    words = explode(str, " ");
    tmp = sizeof(words);
    if (tmp > 1)
    {
	if (tmp > 2 && (num = LANG_NUMS(words[0] + " " + words[1])))
	    tmp = 2;
	else
	if ( (num = LANG_NUMS(words[0])) || (sscanf(words[0], "%d", num)))
	    tmp = 1;
	else
	{
	    tmp = 0;
	    num = 1;
	}
	    
	if (tmp)
	{
	    words = words[tmp..];
	    str = implode(words, " ");
	}
    }
    
    if (query_verb() == "zamow" && num > 1)
    {
         notify_fail("Zamawiac mozna tylko pojedynczo.\n");
         return 0;
    }
    

    if (wildmatch("buklak *", str) || wildmatch("buklaki *", str))
        wynos = 2;
    else if (wildmatch("beczk? *", str) || wildmatch("beczulk? *", str))
        wynos = 1;
    else
        wynos = 0;
        
    if (wynos)
    {
        words = words[1..];
        str = implode(words, " ");
        przyp = PL_DOP;
    }
    else
        przyp = PL_BIE;

    prz = 0;
    i = sizeof(drinks);
    while (--i >= 0)
    {
	size = sizeof(drinks[i][0]);
	ix = -1;
	
	while (++ix < size)
	{
	    przyp = LANG_PRZYP(num, przyp, drinks[i][9][ix]);
	    produkt = drinks[i][(!wynos)][ix];

	    if ((produkt == str) || sscanf(str, "%s " + produkt, prz))
	    {
	        if (prz)
	        {
	            if (sizeof(drinks[i][4]) != 2)
	                continue;

		    przym = ({});
		    tmp = sizeof(drinks[i][4][0]);
		    while (--tmp >= 0)
			przym += ({ oblicz_przym(drinks[i][4][0][tmp],
			    drinks[i][4][1][tmp], przyp, drinks[i][9][ix],
			    (num > 1)) });
		    tmp = sizeof(explode(prz, " "));
		    while (--tmp >= 0)
		    {
			if (member_array(words[tmp], przym) == -1)
			    break;
		    }
		    if (tmp != -1)
			continue;
		}
		
		if (query_verb() == "zamow")
		    return drink_na_miejscu(i, num, wynos);
		else 
		    return drink_na_wynos(i, num, wynos);
		    
		break;
	    }
	}
    }

    prz = 0;
    i = sizeof(food);
    while (--i >= 0)
    {
        if (num > 1 && !food[i][0])
            continue; // Zarcia na miejscu nie daje sie zamowic wiecej niz 1.
            
	size = sizeof(fo_id[i][0]);
	ix = -1;
    
	while (++ix < size)
	{
	    przyp = LANG_PRZYP(num, PL_BIE, food[i][6][ix]);
	    
	    if (przyp == PL_BIE)
		produkt = fo_id[i][(num > 1)][ix];
	    else
		produkt = food[i][(num > 1)][przyp + ix * 6];
	
	    if ((produkt == str) || sscanf(str, "%s " + produkt, prz))
	    {
		if (prz)
		{
		    if (sizeof(food[i][2]) != 2)
		        continue;
		        
		    przym = ({});
		    tmp = sizeof(food[i][2][0]);
		    while (--tmp >= 0)
			przym += ({ oblicz_przym(food[i][2][0][tmp],
			    food[i][2][1][tmp], przyp, food[i][6][ix],
			    (num > 1)) });
			    
		    tmp = sizeof(explode(prz, " "));
		    while (--tmp >= 0)
		    {
			if (member_array(words[tmp], przym) == -1)
			    break;
		    }
		    if (tmp != -1)
			continue;
		}
		
		if (query_verb() == "zamow")
		    return food_na_miejscu(i, num);
		else
		    return food_na_wynos(i, num);

		break;
	    }
	}
    }



    notify_fail("Nie ma na skladzie niczego takiego.\n");
    return 0;
}

/*
 * Nazwa funkcji : add_drink
 * Opis          : Dodaje trunek serwowany na miejscu i (lub) na wynos.
 * Argumenty     : nazwy_dop - Nazwa lub nazwy trunku w dopelniaczu w lp
 *		   nazwy_bie - Nazwa lub nazwy trunku w bierniku w lp
 *		   picie_tu  - Opis przy piciu (Xxx wychyla 'kufel jasnego piwa')
 *		   short_dop - Krotki opis trunku w dopelniaczu.
 *		   adj	     - Tablica z parami przymiotnikow
 * 		   vol	     - Ile % alkoholu jest w plynie
 *		   porcja    - Ile ml napoju sprzedaje sie w porcji na miejscu
 *		   obj_pojemn- W ilu litrowych pojemnikach sprzedaje sie na wynos
 * 		   cena	     - Cena litra napoju.
 *		   rodzaj    - Rodzaj gramatyczny nazwy napoju
 *		   opis_pojemn-Opcjonalny opis pojemnika, w jakim trunek
 *			       jest sprzedawany. 
 */
varargs void
add_drink(mixed nazwy_dop, mixed nazwy_bie, string picie_tu, string short_dop,
          string *adj, int vol, int porcja, int obj_pojemn, int cena,
          mixed rodzaj, string opis_pojemn = 0)
{
    if (!nazwy_dop)
        return ;
    
    if (!drinks)
	drinks = ({ });
    if (!dr_id)
	dr_id = ({ });
	
    if (!pointerp(rodzaj))
	rodzaj = ({ rodzaj });
    if (!pointerp(nazwy_dop))
	nazwy_dop = ({ nazwy_dop });
    if (!pointerp(nazwy_bie))
        nazwy_bie = ({ nazwy_bie });
	
    if (vol > 97) vol = 97;
    if (obj_pojemn < 0) obj_pojemn = 0;
    if (obj_pojemn > 50) obj_pojemn = 50;

//    dr_id += ({ ({ nazwy_dop, ({ }) }) });
    drinks += ({ ({ nazwy_dop, nazwy_bie, picie_tu, short_dop, adj, vol, 
        porcja, obj_pojemn, cena, rodzaj, opis_pojemn }) });
}

/*
 * Function name: query_drinks
 * Description:   Query the drink array
 * Returns:	  The drink array
 */
mixed
query_drinks() { return drinks; }

/*
 * Function name: query_drink_id
 * Description:   Query the drink id:s
 * Returns:	  The array holding all drink id:s
 */
mixed
query_drink_id() { return dr_id; }

/*
 * Function name: remove_drink
 * Description:   Remove a special drink, identified with id
 * Arguments:	  id - A identifying string
 * Returns:	  1 if removed
 */
int
remove_drink(string id)
{
    int i;

    for (i = 0; i < sizeof(dr_id); i++)
	if (member_array(id, dr_id[i]) >= 0)
	{
	    dr_id = exclude_array(dr_id, i, i);
	    drinks = exclude_array(drinks, i, i);
	    return 1;
	}

    return 0;
}

/*
 * Nazwa funkcji : add_food
 * Opis		 : Dodaje do menu posilek do zjedzenia 'na miejscu'.
 * Argumenty     : nazwa     - Nazwa lub nazwy jedzenia w bierniku lp
 *		   adj	     - Tablica z parami przymiotnikow
 *		   ilosc     - Ilosc (integer)
 *		   cena      - Cena zarcia (koszt kupna)
 *		   rodzaj    - Rodzaj gramatyczny nazwy jedzenia
 *		   short     - Krotki opis w bierniku lp (opcjonalne)
 */
public varargs void
add_food(mixed nazwa, mixed *adj, int ilosc, int cena, mixed rodzaj,
	string short)
{
    if (!food)
	food = ({ });
    if (!fo_id)
	fo_id = ({ });

    if (!nazwa)
        return ;
	
    if (!pointerp(rodzaj))
	rodzaj = ({ rodzaj });
	
    if (!pointerp(nazwa))
	nazwa = ({ nazwa });

    if (!pointerp(adj) || !sizeof(adj))
        adj = 0;

    fo_id += ({ ({ nazwa, ({ }) }) });
    food += ({ ({ 0, 0, adj, ilosc, cena, 0, rodzaj, 0, short }) });
}

/*
 * Nazwa funkcji : add_wynos_food
 * Opis		 : Dodaje do menu posilek do zjedzenia 'na wynos'.
 * Argumenty     : nazwy     - Odmiana nazw jedzenia w lp
 *		   pnazwy    - Odmiana nazw jedzenia w lmn
 *		   adj	     - Tablica z parami przymiotnikow
 *		   ilosc     - Ilosc zarcia (waga)
 *		   cena      - Cena zarcia (koszt kupna)
 *		   long      - Dlugi opis
 *		   rodzaj    - Rodzaj gramatyczny nazwy jedzenia
 *		   decay     - Czas psucia sie, ususzania sie, itp.
 *			       w sekundach (0 - nie psuje sie)
 *		   decay_adj - Tablica z para przymiotnikow, gdy jedzenia
 *			       nie da sie zjesc (standardowo - 'zepsuty')
 *				(opcjonalne)
 *		   short     - Odmiana krotkiego opisu w lp (opcjonalne)
 *		   pshort    - Odmiana krotkiego opisu w lmn (opcjonalne)
 */
public varargs void
add_wynos_food(string *nazwy, string *pnazwy, mixed *adj,
	 int ilosc, int cena, string long, mixed rodzaj, int decay,
	 string *decay_adj, string *short, string *pshort)
{
    string *tmp = ({ }), *tmp2 = ({ }); 
    int ix, num;
    
    if (sizeof(nazwy)%6 != 0) return;
    if (sizeof(pnazwy)%6 != 0) return;
    
    if (!food)
	food = ({ });
    if (!fo_id)
	fo_id = ({ });
	
    if (!pointerp(rodzaj))
	rodzaj = ({ rodzaj });
	
    if (!pointerp(adj) || !sizeof(adj))
        adj = 0;

    ix = -1; num = sizeof(nazwy) / 6;
    while (++ix < num)
    {
	tmp += ({ nazwy[PL_BIE + ix * 6] });
	tmp2 += ({ pnazwy[PL_BIE + ix * 6] });
    }

    fo_id += ({ ({ tmp, tmp2 }) });
    food += ({ ({ nazwy, pnazwy, adj, ilosc, cena, long, rodzaj, decay,
	short, pshort, decay_adj }) });
}


/*
 * Function name: query_food
 * Description:   Query the food array
 * Returns:       The food array
 */
mixed
query_food() { return food; }

/*
 * Function name: query_food_id
 * Description:   Query the food id:s
 * Returns:       The array holding all food id:s
 */
mixed
query_food_id() { return fo_id; }

/*
 * Function name: remove_food
 * Description:   Remove a special food, identified with id
 * Arguments:     id - A identifying string
 * Returns:       1 if removed
 */
int
remove_food(string id)
{
    int i;

    for (i = 0; i < sizeof(fo_id); i++)
	if (member_array(id, fo_id[i][0]) >= 0)
	{
	    fo_id = exclude_array(fo_id, i, i);
	    food = exclude_array(food, i, i);
	    return 1;
	}

    return 0;
}
