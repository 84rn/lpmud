/*
 * /lib/shop.c
 *
 * Support for shops, based on the example shop I once written
 * /Nick
 *
 * Revision history:
 * /Mercade, 19 October 1994
 * Added 'show' to appraise an object before you buy it.
 */
#pragma strict_types
#pragma save_binary

inherit "/lib/trade";

#include <stdproperties.h>
#include <macros.h>
#include <language.h>
#include <cmdparse.h>
#include <composite.h>
#include <filter_funs.h>
#include <ss_types.h>

#define MAXLIST	30

static	string	store_room;	/* The storeroom to use */

/*
 * Prototypes
 */
int do_buy(string str);
int do_sell(string str);
int do_value(string str);
int do_show(string str);
int do_list(string str);
int do_store(string str);
int armour_filter(object ob);
int weapon_filter(object ob);

/*
 * Function name: shop_hook_allow_sell
 * Description:	  If you want to do more testing on objects the player intend
 *		  to sell.
 * Argument:	  ob - The object player wants to sell
 * Returns:	  1 if we allow player to sell the object (default)
 */
int
shop_hook_allow_sell(object ob)
{
    return 1;
}

/*
 * Function name: shop_hook_sell_no_match
 * Description:   Hook that is called when players tried to sell something
 *		  but we couldn't understand what he wanted to sell
 * Arguments:	  str - The string player tried to sell
 * Returns:	  0
 */
int
shop_hook_sell_no_match(string str)
{
    notify_fail("Co takiego chcesz sprzedac?\n");
    return 0;
}

/*
 * Function name: shop_hook_sold_items
 * Description:   Hook that is called when player sold something
 * Arguments:	  item - The item array player sold
 * Returns:	  1
 */
int
shop_hook_sold_items(object *item)
{
    write("Sprzedajesz " + COMPOSITE_DEAD(item, PL_BIE) + ".\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " sprzedaje " + QCOMPDEAD(PL_BIE) + 
          ".\n");
    return 1;
}

/*
 * Function name: shop_hook_sold_nothing
 * Description:   Function called if player sold nothing with sell all
 * Returns:	  0
 */
int
shop_hook_sold_nothing()
{
    notify_fail("Nic nie sprzedal" + this_player()->koncowka("es", "as") +
        ".\n");
    return 0;
}

/*
 * Function name: shop_hook_sell_no_value
 * Description:   Called if object has no value
 * Arguments:     ob - The object
 */
void
shop_hook_sell_no_value(object ob)
{
    notify_fail(capitalize(ob->short(PL_MIA)) + " nie ma" +
        (ob->query_tylko_mn() ? "ja" : "") + " zadnej wartosci.\n");
}

void
shop_hook_sell_has_fire(object ob)
{
    notify_fail("Nie przyjmujemy palacych sie przedmiotow.\n");
}

/*
 * Function name: shop_hook_sell_worn_or_wielded
 * Description:   If object is worn or wielded and player has not said he 
 *		  wants to sell such an item
 * Arguments:	  ob - The object
 */
void
shop_hook_sell_worn_or_wielded(object ob)
{
    notify_fail("Chyba nie chcesz sprzedac rzeczy, ktorej dobywasz, lub "+
        "ktora masz na sobie.\n");
}

/*
 * Function name: shop_hook_sell_no_sell
 * Description:   An object has the no sell prop set (OBJ_M_NO_SELL)
 * Arguments:	  ob  - The object
 *		  str - Set if object has an own not sell string
 */
void
shop_hook_sell_no_sell(object ob, string str)
{
    if (stringp(str))
	notify_fail(str);
    else
	notify_fail("Nie chce " + ob->short(PL_DOP) + ".\n");
}

/*
 * Function name: shop_hook_sell_object_stuck
 * Description:   Object didn't want to be moved to the store room
 * Arguments:	  ob  - The object
 *		  err - Error code from move
 */
void
shop_hook_sell_object_stuck(object ob, int err)
{
    notify_fail("Zdaje sie, ze jestes skazan" + this_player()->koncowka("y",
         "a") + " na " + ob->short(PL_BIE) + ".\n");
}

/*
 * Function name: shop_hook_sell_get_money
 * Description:   Called when player gets money for the stuff he sold
 * Arguments:     str - String describing the money he got
 */
void
shop_hook_sell_get_money(string str)
{
    write("Dostajesz " + str + ".\n");
}

/*
 * Function name: shop_hook_allow_buy
 * Description:   A hook to use if you want to test the object further if you
 *		  allow it to be bought
 * Arguments:	  ob - The object player wants to buy.
 * Returns:	  1 if we allow it to be bought (default)
 */
int
shop_hook_allow_buy(object ob)
{
    return 1;
}

/*
 * Function name: shop_hook_buy_no_match
 * Description:   Called if we can't find what player wants to buy
 * Arguments:	  str - The string the player typed in
 * Returns:	  0
 */
int
shop_hook_buy_no_match(string str)
{
    notify_fail("Nie ma niczego takiego w magazynie.\n");
}

/*
 * Function name: shop_hook_bought_items
 * Description:   Called when player has bought something
 * Arguments:	  arr - The array of objects
 * Returns: 	  1
 */
int
shop_hook_bought_items(object *arr)
{
    write("Kupujesz " + COMPOSITE_DEAD(arr, PL_BIE) + ".\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " kupuje " + QCOMPDEAD(PL_BIE) + ".\n");
    return 1;
}

/*
 * Function name: shop_hook_buy_no_buy
 * Description:   The item isn't for sale (OBJ_M_NO_BUY set)
 * Arguments:     ob  - the object
 *                str - the fail text (if any)
 */
void
shop_hook_buy_no_buy(object ob, string str)
{
    if (stringp(str))
        notify_fail(str);
    else
        notify_fail(capitalize(ob->short(PL_MIA)) +
            " nie " + (ob->query_tylko_mn() ? "sa" : "jest") + " na " +
            "sprzedaz.\n");

    ob->remove_object();
}

/*
 * Function name: shop_hook_buy_cant_carry
 * Description:   Player can't carry the object he tried to buy
 * Arguments:     ob  - The object
 *		  err - Error code from move()
 */
void
shop_hook_buy_cant_carry(object ob, int err)
{
    notify_fail("Nie mozesz uniesc " + ob->short(PL_DOP) + ".\n");
}

/*
 * Function name: shop_hook_buy_cant_pay
 * Description:   Called when player can't pay for what he wants to buy.
 *		  The /lib/trade.c sets some default error messages but
 *		  perhaps you are not happy with them?
 * Arguments:	  ob - The object
 *		  arr - The error code as it comes from trade.c
 */
void
shop_hook_buy_cant_pay(object ob, int *arr) {}

/*
 * Function name: shop_hook_buy_magic_money
 * Description:	  Called if we failed to take the money from the player for
 *		  some strange reason (magic money , fool's gold ? )
 * Arguments:	  ob - The object player tried to buy
 */
void
shop_hook_buy_magic_money(object ob)
{
    write("Hmm, masz bardzo dziwne pieniadze! Nie ma mowy!\n");
}

/*
 * Function name: shop_hook_buy_pay_money
 * Description:   Called when player pays the money for something
 * Arguments:     str    - How much he pays in text form
 *		  change - The change he gets back (if any)
 */
void
shop_hook_buy_pay_money(string str, string change)
{
    write("Placisz " + str);
    if (change)
	write(" i dostajesz " + change + " reszty");
    write(".\n");
}

/*
 * Function name: shop_hook_value_not_interesting
 * Description:   Called when player values something we don't want to buy
 * Arguments:	  ob - The object
 */
void
shop_hook_value_not_interesting(object ob)
{
    notify_fail(capitalize(ob->short(PL_MIA)) + " nie jest zbyt " +
        "interesujacym towarem.\n");
}

/*
 * Function name: shop_hook_value_held
 * Description:   Player values an object he's holding
 * Arguments:	  ob   - The object
 *		  text - The price in text form
 */
void
shop_hook_value_held(object ob, string text)
{
    write("Za " + ob->short(PL_BIE) + " dostal" + 
        this_player()->koncowka("bys", "abys") + " " + text + ".\n");
}

/*
 * Function name: shop_hook_value_store
 * Description:   Player values object in store
 * Arguments:     ob   - The object 
 *		  text - The value in text form
 */
void
shop_hook_value_store(object ob, string text)
{
    write(capitalize(ob->short(PL_MIA)) + " kosztowal" + 
        ob->koncowka("by", "aby", "oby", "iby", "yby") + " cie " + 
	text + ".\n");
}

/*
 * Function name: shop_hook_value_asking
 * Description:   What other see when someone evaluates something
 * Arguments:     str - The text form what the player is asking about
 */
void
shop_hook_value_asking(string str)
{
    saybb(QCIMIE(this_player(), PL_MIA) + " pyta sie o jakies ceny.\n");
}

/*
 * Function name: shop_hook_appraise_object
 * Description  : Called when a player asks to see an object for sale.
 */
void
shop_hook_appraise_object(object ob)
{
    saybb(QCIMIE(this_player(), PL_MIA) + " prosi o pokazanie " + 
          ob->short(PL_DOP) + ".\n");
}

/*
 * Function name: shop_hook_value_no_match
 * Description:   Called if there were no match with what the player asked
 *		  about
 * Arguments:     str - The string player asked about
 * Returns:	  0
 */
int
shop_hook_value_no_match(string str)
{
    notify_fail("Wydaje mi sie, iz nie posiadasz niczego takiego.\n");
}

/*
 * Function name: shop_hook_list_empty_store
 * Description:   If the storeroom is empty when the player wants a list
 *		  of its items
 * Arguments:	  str - If player specified string
 */
void
shop_hook_list_empty_store(string str)
{
    notify_fail("Magazyn jest pusty.\n");
}

/*
 * Function name: shop_hook_list_no_match
 * Description:   Called if player specified the list and no matching
 *		  objects where found
 * Arguments:	  str - The string he asked for
 * Returns:	  0
 */
int
shop_hook_list_no_match(string str)
{
    notify_fail("Nie mamy niczego takiego na skladzie.\n");
}

/*
 * Function name: shop_hook_fail_storeroom
 * Description  : This function is called when a player tries to enter
 *                the storeroom. The function operates on this_player().
 */
void
shop_hook_fail_storeroom()
{
    write("Nie jestes upowaznion" + this_player()->koncowka("y", "a") +
        " do wejscia do magazynu.\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " na prozno probuje wejsc do " +
          "magazynu.\n");
}

/*
 * Function name: shop_hook_list_object
 * Description:   List an object
 * Arguments:	  ob - The object
 */
void
shop_hook_list_object(object ob, int price)
{
    string str, mess;
    
    str = sprintf("%-25s", capitalize(ob->short(PL_MIA)));
    if (mess = text(split_values(price), 0))
	write(str + mess + ".\n");
    else
	write(str + "Za ten przedmiot nic bys nie zaplacil" + 
	    this_player()->koncowka("", "a") + ".\n");
}

/*    
 * Function name: query_buy_price
 * Description:   What price should the player pay
 * Arguments:     ob - The object to test
 * Returns: 	  The price
 */
int
query_buy_price(object ob)
{
    int seed;

    sscanf(OB_NUM(ob), "%d", seed);
    return 3 * ob->query_prop(OBJ_I_VALUE) * (query_money_greed_buy() +
	15 - this_player()->query_skill(SS_TRADING) / 4 +
	random(15, seed)) / 200;
}

/*    
 * Function name: query_sell_price
 * Description:   What price will the player get when selling an object?
 * Arguments:	  ob - The object
 * Returns:	  The price
 */
int
query_sell_price(object ob)
{
    int seed;

    sscanf(OB_NUM(ob), "%d", seed);
    return ob->query_prop(OBJ_I_VALUE) * 100 / (query_money_greed_sell() +
	15 - this_player()->query_skill(SS_TRADING) / 3 + 
	random(15, seed + 1)); /* Use another seed than on buying */
}

/*
 * Function name: set_store_room
 * Description:   Tell us wich storeroom to use
 * Arguments:	  str - The storeroom
 */
void
set_store_room(string store)
{
    store_room = store;
}

/*
 * Function name: query_store_room
 * Description:   What store room do we use?
 * Returns:	  The file name to the store room
 */
string
query_store_room()
{
    return store_room;
}

/*
 * Function name: init_shop
 * Description  : This function is called for each living that enters the
 *                room. It adds the necessary commands to the players.
 *                You should call this from the init() function in your
 *                room.
 */
void
init_shop()
{
    add_action(do_buy,   "kup");
    add_action(do_sell,  "sprzedaj");
    add_action(do_value, "wycen");
    add_action(do_show,  "pokaz");
    add_action(do_list,  "przejrzyj");
    add_action(do_store, "store");
}

/*
 * Function name: wiz_check
 * Description:   Check if a player is a wizard
 * Returns:       0 if the player is a wizard
 *                1 otherwise
 */
int
wiz_check()
{
    if (this_player()->query_wiz_level())
      	return 0;

    shop_hook_fail_storeroom();
    return 1;
}

/*
 * Function name: do_store
 * Description  : This function is called when a wizard uses the 'store'
 *                command. They can use it to access the storeroom of
 *                the shop in case of a necessity.
 * Arguments    : string str - the command line argument (should be void)
 * Returns      : int 1/0 - success/failure.
 */
int
do_store(string str)
{
    /* This command is only valid for wizards. */
    if (!this_player()->query_wiz_level())
    {
	return 0;
    }

    store_room->load_me();
    if (!objectp(find_object(store_room)))
    {
	notify_fail("Nie moge znalezc magazynu.\n");
	return 0;
    }

    this_player()->move_living("X", store_room);
    return 1;
}

/*
 * Function name: standard_shop_sign
 * Description:   returns a string with a sign message
 * Arguments:     none
 * Returns:       message string
 */
string
standard_shop_sign()
{
    return
"Oto przyklady tego, co mozesz tu uczynic:\n" +
"    kup miecz za zlote monety    (standardowo najmniejsza denominacja)\n" +
"    kup miecz za zloto i wez miedziaki reszty\n" +
"    sprzedaj miecz za miedziaki  (standardowo najwieksza denominacja)\n" +
"    sprzedaj wszystko  - dzieki czemu sprzeasz wszystkie przedmioty, poza\n"+
"                         tymi, ktore dzierzysz lub masz na sobie. Pamietaj,\n"+
"                         ze gdy masz za duzo przedmiotow do sprzedania, byc\n"+
"                         moze zajdzie koniecznosc ponownego wykonania tej\n"+
"                         komendy.\n"+
"    sprzedaj wszystko! - pozwoli ci sprzedac WSZYSTKIE przedmioty, ktore\n"+
"                         masz przy sobie - za wyjatkiem tych, ktorych nie\n" +
"                         da sie odlozyc i z wyjatkiem monet.\n"+
"                         (patrz ostrzezenie dla 'sprzedaj wszystko')\n"+
"    sprzedaj miecze, sprzedaj drugi miecz, sprzedaj dwa miecze - te komendy\n"+
"                         takze dzialaja. Pamietaj jednak, ze nigdy\n" +
"                         nie bedziesz w stanie kupic wiecej, niz jednego\n"+
"                         przedmiotu.\n" +
"    wycen              - Sprzedawca wyceni ile wart jest dany przedmiot,\n"+
"                         zanim zdecydujesz sie go sprzedac.\n" +
"    pokaz              - Pozwoli ci obejrzec dokladnie jakis przedmiot\n"+
"                         z magazynu, zanim podejmiesz decyzje o kupnie go.\n"+
"    przejrzyj          - Pozwala ci przejrzec zawartosc magazynu. Mozesz\n"+
"                         tez 'przejrzec zbroje' i 'przejrzec bronie'.\n";
}

/*
 * Function name: sell_it
 * Description:   Try to let the player sell the item array
 * Arguments:     ob - the object array
 *                check - wheather check for worn or wielded stuff
 *                str - string describing how the money should be paid
 * Returns:	  An array with the objects sold
 */
object *
sell_it(object *ob, string str, int check) 
{
    int price, i, j, k, *tmp_arr, *null, *value_arr, *null_arr, err;
    object *sold;
    mixed tmp;

    call_other(store_room, "load_me"); /* Make sure store room is loaded */

    value_arr = allocate(sizeof(query_money_types()));
    null_arr = value_arr + ({});
    sold = allocate(sizeof(ob));

    for (i = 0; i < sizeof(ob); i++)
    {
	if (!shop_hook_allow_sell(ob[i]))
	    continue;

        if (ob[i]->query_prop(OBJ_I_VALUE) == 0)
	{
	    shop_hook_sell_no_value(ob[i]);
	    continue;
        }

	if (check && (ob[i]->query_worn() ||
		      ob[i]->query_wielded()))
	{
	    shop_hook_sell_worn_or_wielded(ob[i]);
	    continue;
        }
    
	if (tmp = ob[i]->query_prop(OBJ_M_NO_SELL))
	{
	    shop_hook_sell_no_sell(ob[i], tmp);
	    continue;
	}
	
	if (ob[i]->query_prop(OBJ_I_HAS_FIRE))
	{
	    shop_hook_sell_has_fire(ob[i]);
	    continue;
	}

	/* Save price if ob destructed in move */
	price = query_sell_price(ob[i]);

        if (price <= 0)
	{
	    shop_hook_sell_no_value(ob[i]);
	    continue;
        }

        if (err = ob[i]->move(store_room))
    	{
	    shop_hook_sell_object_stuck(ob[i], err);
	    continue;
    	}

        if (price > 0)
	{
            tmp_arr = calc_change(price, null, str);
            for (k = 0; k < sizeof(value_arr); k++)
                value_arr[k] += tmp_arr[k];

	    sold[j] = ob[i];
            j++;
	    if (j >= 20)
        	break;
    /*
     * Only let people sell 20 objects at once and hopefully we wont get
     * those too long evaluation problems.
     */
	}
    }

    sold = sold - ({ 0 });

    if (sizeof(sold) > 0)
    {
        change_money(null_arr + value_arr, this_player());
	shop_hook_sell_get_money(text(value_arr, 0));
    }

    return sold;
}

/*
 * Function name: do_sell
 * Description:   Try to let the player sell the_item
 *                Observe there is no message written when sold item
 *                has a value higher than the shop gives out.
 * Returns:       1 on sucess
 * Arguments:     str - string holding name of item, hopefully
 */
int
do_sell(string str)
{
    object *item;
    int value, check;
    string str1, str2;

    if (!str || str =="")
    {
	notify_fail("Sprzedaj co?\n");
	return 0;
    }

    /*  Did player specify how to get the money? */
    if (sscanf(str, "%s za %s", str1, str2) != 2)
    {
 	str1 = str;
	str2 = "";
    }

    if (str1 == "wszystko!")
    {
	str1 = "wszystko";
        check = 0; /* Sell worn or wielded objects. */
    }
    else
    {
	check = 1; /* Don't sell worn or wielded objects. */
    }

    item = FIND_STR_IN_OBJECT(str1, this_player(), PL_BIE);
    if (!sizeof(item))
	return shop_hook_sell_no_match(str1);

    item = sell_it(item, str2, check);
    if (sizeof(item))
	return shop_hook_sold_items(item);

    if (str1 == "wszystko")
	return shop_hook_sold_nothing();

    return 0; /* Player tried to sell a non sellable object. */
}

/*
 * Function name: buy_it
 * Description:   Try to let the player buy the item array
 * Arguments:     ob - the object array
 *                str2 - string describing how the money should be paid
 *                str3 - what coin types to get chainge back in
 * Returns:       1 on sucess
 */
object *
buy_it(object *ob, string str2, string str3) 
{
    int price, i, j, k, *value_arr, *arr, error, num, err;
    object *bought;
    mixed tmp;

    num = sizeof(query_money_types());
    value_arr = allocate(2 * num);
    bought = allocate(sizeof(ob));

    for (i = 0; i < sizeof(ob); i++)
    {
	if (!shop_hook_allow_buy(ob[i]))
	    continue;

        if (tmp = ob[i]->query_prop(OBJ_M_NO_BUY))
        {
            shop_hook_buy_no_buy(ob[i], tmp);
            continue;
        }

	price = query_buy_price(ob[i]);

	/* If you don't feel greedy you can shorten the calculation above. */
	if (err = ob[i]->move(this_player()))
	{
	    shop_hook_buy_cant_carry(ob[i], err);
	    continue;
	}

	if (sizeof(arr = pay(price, this_player(), str2, 0, 0, str3)) == 1)
	{
	    ob[i]->move(store_room, 1);
	    shop_hook_buy_cant_pay(ob[i], arr);
            continue;  /* pay() can handle notify_fail() call */
	}

	/* Detect if there was a move error. */
	if (error = arr[sizeof(arr) - 1])
	{
	    if (error < -1)
	    {
	    /* Couldn't take the money from player, the coins were stuck */
	        shop_hook_buy_magic_money(ob[i]);
	        ob[i]->move(store_room, 1);
		continue;
	    }
	    /* We don't want the money so no move error to us, if there was one
	       it was because the player couldn't hold all coins, and if so the
	       drop text is already written, but the deal is still on :) */
	}

        for (k = 0; k < 2 * num; k++)
            value_arr[k] += arr[k];

	bought[j] = ob[i];
        j++;
	if (j >= 1)
       	    break;
	/* Well, we don't want to let a player accidentily buy too much :) */
    }

    bought = bought - ({ 0 });

    if (sizeof(bought) > 0)
	shop_hook_buy_pay_money(
		text(arr[0 .. num - 1], PL_BIE), 
		text(arr[num .. 2 * num - 1], PL_BIE));

    return bought;
}

/*
 * Function name: do_buy
 * Description:   Try to let the player buy an item
 * Arguments:     string - describing how to pay and get change
 * Returns:       1 on sucess
 */
int
do_buy(string str)
{
    object *item, store;
    string str1, str2, str3;

    if (!str || str =="")
    {
	notify_fail("Kup co?\n");
	return 0;
    }
 
    call_other(store_room, "load_me"); /* Make sure storeroom is loaded */

    /*  Did the player specify payment and change? */
    if (sscanf(str, "%s za %s i wez %s", str1, str2, str3) != 3)
    {
    /* Well, maybe player has defined how payment will be done atleast? */
        str3 = "";
        if (sscanf(str, "%s za %s", str1, str2) != 2)
        {
            str2 = "";
            str1 = str;
        }
    }

    store = find_object(store_room);
    item = FIND_STR_IN_OBJECT(str1, store, PL_BIE);

    if (!sizeof(item))
	return shop_hook_buy_no_match(str1);

    item = buy_it(item, str2, str3);
    if (sizeof(item))
	return shop_hook_bought_items(item);

    return 0; /* Player tried to sell a non sellable object. */
}

/*
 * Function name:   do_value
 * Description:     Let the player value an item, carried
 * Returns:         1 on success
 */
int
do_value(string str)
{
    object *item, store;
    int *arr, price, i, j, num, no_inv;

    if (!str || str =="")
    {
	notify_fail("Wycen co?\n");
	return 0;
    }

    num = sizeof(query_money_types());
    item = FIND_STR_IN_OBJECT(str, this_player(), PL_BIE);
    if (!sizeof(item))
	no_inv = 1;

    for (i = 0; i < sizeof(item); i++)
    {
	if (!shop_hook_allow_sell(item[i]) ||
		!item[i]->query_prop(OBJ_I_VALUE) ||
		item[i]->query_prop(OBJ_M_NO_SELL)) 
	{
	    shop_hook_value_not_interesting(item[i]);
	    continue;
	}

	price = query_sell_price(item[i]);
	arr = give(price, this_player(), "", 1);
	shop_hook_value_held(item[i], text(arr[num .. 2 * num - 1], 0));
	j++;
    }

#if 0
 * Nie sprawdzamy do wyceniania tego co w magazynie. Alvin.
 
    call_other(store_room, "load_me");
    store = find_object(store_room);
    item = FIND_STR_IN_OBJECT(str, store, PL_BIE);
    if (!sizeof(item) && no_inv)
	return shop_hook_value_no_match(str);

    for (i = 0; i < sizeof(item); i++)
    {
	price = query_buy_price(item[i]);
	arr = split_values(price); /* A price with few coins possible */
	shop_hook_value_store(item[i], text(arr, 0));
	j++;
    }	
#endif

    if (no_inv)
	return shop_hook_value_no_match(str);
	
    shop_hook_value_asking(str);
    
    if (j > 0)
	return 1;

    return 0;
}

/*
 * Function name:   do_list
 * Description:     Provide a list of objects in the store room
 * Returns:         0 if not recognised
 *                  1 otherwise
 * Arguments: 	    str - the name of the objects to search for
 */
int
do_list(string str)
{
    object *item_arr;
    int i, max, price, *arr;

    call_other(store_room, "load_me");

    item_arr = all_inventory(find_object(store_room));

    if (!sizeof(item_arr))
    {
	shop_hook_list_empty_store(str);
	return 0;
    }

    if (str == "bronie")
        item_arr = filter(item_arr, weapon_filter);
    else if (str == "zbroje")
            item_arr = filter(item_arr, armour_filter);
    else if (str)
	item_arr = FIND_STR_IN_ARR(str, item_arr, PL_BIE);

    if (sizeof(item_arr) < 1)
	return shop_hook_list_no_match(str);
 
    max = MIN(MAXLIST, sizeof(item_arr));
    for (i = 0; i < max; i++)
    {
	price = query_buy_price(item_arr[i]);
	shop_hook_list_object(item_arr[i], price);
    }

#if 0
    if (max < sizeof(item_arr))
    {
	write("Truncated...\n");
    }
#endif

    return 1;
}

/*
 * Function name: do_show
 * Description  : Allow the player to appraise one of the objects in stock.
 * Returns      : int - 1/0 - true if success.
 * Arguments    : string str - the name of the objects to search for.
 */
int
do_show(string str)
{
    object *item_arr;
    int i, *arr;

    call_other(store_room, "load_me");

    item_arr = all_inventory(find_object(store_room));

    if (!sizeof(item_arr))
    {
	shop_hook_list_empty_store(str);
	return 0;
    }

    item_arr = FIND_STR_IN_ARR(str, item_arr, PL_BIE);

    if (sizeof(item_arr) < 1)
    {
	return shop_hook_list_no_match(str);
    }

    shop_hook_appraise_object(item_arr[0]);
    item_arr[0]->appraise_object();

    return 1;
}

/*
 * Function name: weapon_filter
 * Description  : Filter out weapons
 * Arguments    : object ob - the object to test
 * Returns      : int - 1/0 - true if it is a weapon.
 */
int
weapon_filter(object ob)
{
    return (function_exists("create_object", ob) == "/std/weapon");
}

/*
 * Function name: armour_filter
 * Description  : Filter out armours
 * Arguments    : object ob - the object to test
 * Returns      : int - 1/0 - true if it is an armour.
 */
int
armour_filter(object ob)
{
    return (function_exists("create_object", ob) == "/std/armour");
}

