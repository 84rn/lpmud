/*
 * A supporting file for banks
 *
 * Made by Nick
 */
#pragma strict_types
#pragma save_binary

inherit "/lib/trade";

#include <stdproperties.h>
#include <macros.h>
#include <money.h>

static	int	testflag;	/* To indicate that a test is going on, no money given. */
static	int	bank_fee;	/* The bank fee */
static	int	money_num;	/* Num of money types. */

int change(string str);
int minimize(string str);
int test(string str);
int query_bank_fee();

/*
 * Function name: config_trade_data
 * Description:   Here we configure our own settings for the trade data
 */
void
config_trade_data()
{
    /* You have to set these two to the same number in order to get the
     * right calculations.
     */
    set_money_greed_buy(100 + query_bank_fee());
    set_money_greed_change(100 + query_bank_fee());

    /* A bank is rich. And if you can't give out the max with each type
     * of money you'll have to use another formula than I have below,
     * I think.
     */
	             /* Copper Silver Gold  Platinum  */
    set_money_give_out(({40000, 4000, 2000, 200}));
    set_money_give_max(40000);
    set_money_give_reduce(({0, 0, 0, 0}));

    money_num = sizeof(query_money_types());
}

/*
 * Function name: standard_bank_sign
 * Description:   returns a string with a sign message
 * Arguments:     none
 * Returns:       message string
 */
string
standard_bank_sign()
{
    return "Czytasz:\n\n" + 
	"Nasz bank oferuje ci tani sposob na pozbycie sie uciazliwych, "+
	"ciezkich monet. Za kazda transakcje pobieramy tylko " + query_bank_fee() +
	"% prowizji.\n" +
	"A oto przyklad rzeczy, ktore mozesz tu zrobic:\n" + 
	" -  zamienic miedz i srebro na zloto\n" +
	"  W wyniku czego zamienilibysmy ci wszystkie srebrne i miedziane\n" +
	"  w tyle zlota, ile tylko sie da.\n" +
	" -  probnie zamienic miedz i srebro na zloto\n" +
	"  Co pozwala ci na sprawdzenie efektu danej operacji, bez\n"+
	"  jej wykonywania.\n" +
	" -  zamienic 1 mithrylowa monete na miedziane monety\n" +
	"  Dzieki czemu rozmienisz mithryl na miedziaki.\n" +
	" -  zamienic mithryle na 100 srebrnych monet\n" +
	"  W wyniku czego na 100 srebrnych monet zostanie rozmienione tyle\n"+
	"  mithrylu, ile potrzeba.\n" +
	" -  zdenominowac monety\n" +
	"  Co spowoduje zamiane monet na najbardziej wartosciowe.\n\n";
}

/*
 * Function name: bank_init
 * Description:   Add commands to someone who enters the room
 */
void
bank_init()
{
    add_action(change, "zamien");
    add_action(change, "wymien");
    add_action(minimize, "zdenominuj");
    add_action(test, "probnie");
}

/*
 * Function name: set_bank_fee
 * Description:   Set the fee in % we take for our services
 * Argument:	  fee - The fee
 */
void set_bank_fee(int fee) { bank_fee = fee; }

/*
 * Function name: query_bank_fee
 * Description:   Query the fee we take
 * Returns:	  The fee
 */
int query_bank_fee() { return bank_fee; }

/*
 * Function name: bank_hook_pay
 * Description:   Change this function if you want a pay message of your own
 * Argument:	  text - The text describing what coins we payed
 */
void
bank_hook_pay(string text)
{
    write("Placisz " + text + ".\n");
}

/*
 * Function name: bank_hook_change
 * Description:   This function is called when a change text is supposed to
 *		  be written. Redefine this if you wish.
 * Arguments:     text - The text describing the change
 */
void
bank_hook_change(string text)
{
    write("Dostajesz " + text + " reszty.\n");
}

/*
 * Function name: bank_hook_other_see
 * Description:   This function writes what the other players sees.
 *		  Redefine it if you want own messages.
 * Arguments:	  test - If a test is going on
 */
void
bank_hook_other_see(int testflag)
{
    if (!testflag)
	saybb(QCIMIE(this_player(), PL_MIA) + " wymienia jakies pieniadze.\n");
    else
	saybb(QCIMIE(this_player(), PL_MIA) + " przelicza i spradza cos.\n");
}

/*
 * Function name: bank_hook_already_minimized
 * Description:   Write this when your coins already are minimized
 */
void
bank_hook_already_minimized()
{
    write("Twoje pieniadze juz sa maksymalnie zdenominowane.\n");
}

/*
 * Function name: bank_hook_no_idea
 * Description:   When there is no idea to minimize since it will all be
 *   		  eaten up by the tax
 */
void
bank_hook_no_idea()
{
    write("Nie ma zadnego powodu, dla ktorego powin" +
        this_player()->koncowka("ienes", "nas") + " denominowac pieniadze.\n");
}

/*
 * Function name: bank_hook_minimized
 * Description:   Player minimized his coins
 */
void
bank_hook_minimized(string pay_text, string got_text)
{
    write("Twoje pieniadze zostaly zdenominowane.\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " denominuje swoje pieniadze.\n");
}

/*
 * Function name: valid_type
 * Description:   Find if a str is holding a valid type of money
 * Arguments:     str - The string describing the types
 * Returns:       The array number of the 'lowest' type of money identified
 */
int
valid_type(string str)
{
    int i, j, *tmp_arr;
    string *m_names;
    string *m_tematy = query_money_tematy();

    m_names = explode(str, " ");
    tmp_arr = allocate(money_num);
    
    for (i = 0; i < sizeof(m_names); i++)
        for (j = 0; j < money_num; j++)
	    if (wildmatch(m_tematy[j] + "*", m_names[i]))
                tmp_arr[j] = 1;

    for (i = 0; i < money_num; i++)
	if (tmp_arr[i] == 1)
	    return i;

    return -1;		/* This should not happen */
}

/*
 * Function name: change
 * Description:   Perform a change of money in the player
 * Arguments:     str - A string describing what to change into what
 */
int
change(string str)
{
    string str1, str2, dummy, change;
    int price, i, j, *arr, *hold_arr, *change_arr, number, greed;

    greed = query_money_greed_buy();

    if (!str)
    {
	notify_fail(capitalize(query_verb()) + " co?\n");
	return 0;
    }

    if (!parse_command(str, ({}), "%s 'na' / 'w' %s", str1, str2))
    {
	notify_fail("Musisz sprecyzowac co chcesz zamienic w co.\n");
	return 0;
    }

    notify_fail("Mozesz wymienic tylko te monety, ktore masz przy sobie.\n");

    /* First find out how many coins player maximum can change to
     * Arguments: price = 0, changer = this_player(), str1 = what changer
     * wants to change, 1 = this is a test, 0 = a nil object (we),
     * str2 = how changer wants the change 
     *
     * These settings returns an array of what the changer wants to
     * change and how much that would be in the change the changer has
     * chosen. Then we can calculate the exact amount to change.
     */
    if (sizeof(arr = pay(0, this_player(), str1, 1, 0, str2)) == 1)
	return 0;

    hold_arr = exclude_array(arr, money_num, 2 * money_num - 1);
    change_arr = exclude_array(arr, 0, money_num - 1);

    if ((i = valid_type(str1)) >= 0)
    {
	if (sscanf(str1, "%d %s", number, dummy) == 2)
        {
	    if (number > hold_arr[i])
	    {
		notify_fail("Nie mozesz wydac wiecej pieniedzy niz masz.\n");
		return 0;
	    }
	    hold_arr[i] = number;
	    change_arr = calc_change(0, hold_arr, str2);
	    notify_fail("Nie uwazasz, ze warto by podac liczbe wieksza " +
	        "od 0?\n");
	}

	if (hold_arr[i] <= 0) 
	    return 0; 

	if ((i = valid_type(str2)) >= 0)
	{
	/* We need the price in order to take money from the player */
	    if (sscanf(str2, "%d %s", number, dummy) == 2)
            {
	        if (number > change_arr[i])
	        {
	   	    notify_fail("Nie stac cie na zamiane na taka ilosc.\n");
		    return 0;
	        }
	        change_arr[i] = number;
		notify_fail("Don't you think you ought to give a number higher " +
			"than 0?\n");
		notify_fail("Nie uwazasz, ze warto by podac liczbe wieksza " +
		    "od 0?\n");
	        if (change_arr[i] <= 0)
		    return 0;
	    }
	    if ((change_arr[i] * query_money_values()[i] * greed / 100) >
				query_money_give_max())
		change_arr[i] = (query_money_give_max() * 100 / greed) /
				query_money_values()[i];

	    for (j = i - 1; j >= 0; j--)
	        change_arr[j] = 0;

    	    price = money_merge(change_arr);
	    if (price < 1)
	    {
		notify_fail("Nie stac cie na te zamiane z taka mala "+
		     "iloscia pieniedzy.\n");
		return 0;
	    }
        }
        else
        {
	    notify_fail("Musisz wybrac wlasciwy rodzaj pieniedzy do wymiany.\n");
            return 0;
        }
    }
    else
    {
	notify_fail("Musisz wybrac wlasciwy rodzaj pieniedzy do wymiany.\n");
	return 0;
    }
  
    /* Here is the actual change taking place */
    if (!(arr = pay(price, this_player(), str1, testflag, 0, str2)))
	return 0;
    give(price, this_player(), str2, testflag, 0, 0);

    /* Now, in the pay() the player could have been given some change back,
     * add it.
     */
    for (i = 0; i < money_num; i++)
	change_arr[i] = change_arr[i] + arr[i + money_num];

    /*
     * Some hooks for people who wants different messages.
     */
    bank_hook_pay(text(arr[0 .. money_num - 1], PL_BIE));
    if (change = text(change_arr, PL_BIE))
	bank_hook_change(change);
    bank_hook_other_see(testflag);

    return 1;
}

/*
 * Function name: minimize
 * Description:   changes all coins into the most expensive type, minus a fee
 *		  BUGS - This function still not supports unstandard coins
 * Arguments:     str: predicate
 * Returns:       success
 */
int minimize(string str)
{
#if 0
    int *money_arr, *money_arr2, value, i, new_sum, total_sum;
    string change;

    money_arr = what_coins(this_player());

    money_arr2 = split_values(total_sum = money_merge(money_arr));
    for (i = 0; i < money_num; i++)
    {
	money_arr2[i] -= money_arr[i];
	if (money_arr2[i] < 0)
	    money_arr2[i] = 0;
    }
    value = money_merge(money_arr2);
    if (!value)
    {
	bank_hook_already_minimized();
	return 1;
    }
    new_sum = total_sum - query_bank_fee() * value / 100;
    money_arr2 = split_values(new_sum);
    for (i = 0; i < money_num; i++)
    {
	money_arr2[i] -= money_arr[i];
	if (money_arr2[i] < 0)
	    money_arr2[i] = 0;
    }
    value = money_merge(money_arr2);
    if (!value)
    {
	bank_hook_no_idea();
	return 1;
    }

    MONEY_ADD(this_player(), new_sum);
    MONEY_ADD(this_player(), -total_sum);

    money_arr2 = what_coins(this_player());

    bank_hook_minimized(text(money_arr, PL_BIE), text(money_arr2, PL_BIE));

#endif 0

    int *money_arr, *money_arr2, total_sum, to_spend, money_value, ix, tmp,
	roznica, flag, procent;

/*
    notify_fail("Niestety, decyzja Banku Centralnego wykonywanie tej " +
	"uslugi zostalo czasowo wstrzymane. Za niedogodnosci z tym " +
	" zwiazane przepraszamy.\n");
    return 0;
*/

    money_arr = what_coins(this_player());
    total_sum = to_spend = money_merge(money_arr);

    /* Petla majaca na celu sprawdzenie, czy pieniadze nie sa juz
     * zdenominowane. Wyrzucenie jej nie zaszkodzi funkcji, a jedynie
     * uprosci komunikaty wyswietlane graczowi.
     */
    for (ix = (money_num - 1); ix >= 0; ix--)
    {
	if (money_arr[ix] != (to_spend / MONEY_VALUES[ix]))
	    break;
	to_spend %= MONEY_VALUES[ix];
    }

    if (ix == -1)
    {
	bank_hook_already_minimized();
	return 1;
    }

    money_arr2 = allocate(money_num);
    procent = (100 - query_bank_fee());
    total_sum = to_spend = total_sum * procent / 100;
    flag = 1;
    
    /* Glowna petla denominujaca. Upewniamy sie, ze gracz nie straci
     * pieniedzy na podatku, nie denominujac niczego wlasciwie.
     * Staramy sie zdenominowac wszystkie mozliwe typy monet.
     */
    for (ix = (money_num - 1); ix >= 0; ix--)
    {
	money_arr2[ix] += to_spend / MONEY_VALUES[ix];
	to_spend %= MONEY_VALUES[ix];
	roznica = (money_arr2[ix] - money_arr[ix]) * MONEY_VALUES[ix];
	if (flag)
	{
	    if (roznica <= 0)
	    {
		if (ix == 1)
		{
		    bank_hook_no_idea();
		    return 1;
		}
		
		if (flag == 2)
		{
		    flag = 1;
		    continue;
		}
		
		money_arr2[ix] = money_arr[ix];
		total_sum = money_merge(money_arr[0..(ix-1)] +
		    allocate(money_num - ix)) * procent / 100;
		to_spend = total_sum;
		ix++;
		flag = 2;
		continue;
	    }
	
	    flag = 0;
	}

	MONEY_ADD(this_player(), roznica);
    }

    bank_hook_minimized(text(money_arr, PL_BIE), text(money_arr2, PL_BIE));
        
    return 1;
}

/*
 * Function name: test
 * Description:   To allow the player to see what would happen with a change 
 *                command about to be given
 * Arguments:     str - The string holding the change command
 */
int
test(string str)
{
    int i;
    string str1;
    
    notify_fail("Co chcesz probnie zrobic?\n");
    if (!str)
	return 0;

    write("Oto co byloby efektem zamiany:\n");

    if (parse_command(str, ({}), "'zamien' / 'wymien' %s", str1))
    {
	testflag = 1;
	i = change(str1);
	testflag = 0;
	return i;
    }
}
