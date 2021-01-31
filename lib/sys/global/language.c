/*
  /sys/global/language.c

  This file holds some standard natural language functions.
  
  Polskie liczebniki i odmania Alvin@Arkadia
*/

#pragma no_clone
#pragma no_inherit
#pragma no_shadow
#pragma save_binary
#pragma strict_types

#include <stdproperties.h>
#include <pl.h>

string *nums, *numt, *numnt, *numo, // Usunac angielskie potem
       *num_glowne_0, *num_glowne_1, *num_zbior_m, *num_zbior_d,
       *num_zbior_n, *num_zbior_ms, *num_d_0, *num_d_1, *num_n_0, *num_n_1,
       *num_o_0, *num_o_1, 
       *je, *dw, *tr, *cz, *pi, *sz, *si, *os, *dz,
       *ord1, *ord11, *ord20;

void
create()
{
    nums = ({ "one", "two", "three", "four", "five", "six", "seven",
	      "eight", "nine", "ten","eleven","twelve","thirteen",
	      "fourteen", "fifteen", "sixteen", "seventeen", "eighteen",
	      "nineteen"});
    numo = ({ "first", "second", "third", "fourth", "fifth", "sixth",
	      "seventh", "eighth", "ninth", "tenth","eleventh","twelfth",
	      "thirteenth", "fourteenth", "fifteenth", "sixteenth",
	      "seventeenth", "eighteenth", "nineteenth"});
    numt = ({ "twenty", "thirty", "forty", "fifty", "sixty", "seventy",
	      "eighty", "ninety"});
    numnt = ({ "twent", "thirt", "fort", "fift", "sixt", "sevent", "eight",
	       "ninet" });

    je = ({ "jeden", "jednego", "jednemu", "jednym", "jedna", "jednej",
	    "jedno", "jedenascie", "jedenastu", "jedenastoma" });
    dw = ({ "dwa", "dwaj", "dwoch", "dwom", "dwoma", "dwie", "dwanascie", 
	    "dwunastu", "dwunastoma", "dwadziescia", "dwudziestu", 
	    "dwudziestoma" });
    tr = ({ "trzy", "trzech", "trzej", "trzem", "trzema", "trzynascie",
	    "trzynastu", "trzynastoma", "trzydziesci", "trzydziestu",
	    "trzydziestoma" });
    cz = ({ "cztery", "czterech", "czterem", "czterema", "czterej", 
	    "czternastu", "czternascie", "czternastoma", "czterdziesci",
	    "czterdziestu", "czterdziestoma" });
    pi = ({ "piec", "pieciu", "piecioma", "pietnascie", "pietnastu", 
	    "pietnastoma", "piecdziesiat", "piecdziesieciu",
	    "piecdziesiecioma" });
    sz = ({ "szesc", "szesciu", "szescioma", "szesnascie", "szesnastu",
	    "szesnastoma", "szescdziesiat", "szescdziesieciu", 
	    "szescdziesiecioma" });
    si = ({ "siedem", "siedmiu", "siedmioma", "siedemnascie", "siedemnastu",
	    "siedemnastoma", "siedemdziesiat", "siedemdziesieciu", 
	    "siedemdziesiecioma" });
    os = ({ "osiem", "osmiu", "osmioma", "osiemnascie", "osiemnastu", 
	    "osiemnastoma", "osiemdziesiat", "osiemdziesieciu",
	    "osiemdziesiecioma" });
    dz = ({ "dziewiec", "dziewieciu", "dziewiecioma", "dziesiec", "dziesieciu",
	    "dziesiecioma", "dziewietnascie", "dziewietnastu", 
	    "dziewietnastoma", "dziewiecdziesiat", "dziewiecdziesieciu", 
	    "dziewiecdziesiecioma" });
    
    num_glowne_0 = ({ "jeden", "dwa", "trzy", "cztery", "piec", "szesc",
		      "siedem", "osiem", "dziewiec", "dziesiec", "jedenascie",
		      "dwanascie", "trzynascie", "czternascie", "pietnascie",
		      "szesnascie", "siedemnascie", "osiemnascie",
		      "dziewietnascie" });
    num_glowne_1 = ({ "dwadziescia", "trzydziesci", "czterdziesci",
		      "piecdziesiat", "szescdziesiat", "siedemdziesiat",
		      "osiemdziesiat", "dziewiecdziesiat" });

    num_zbior_m = ({ "jedno", "dwoje", "troje", "czworo", "piecioro",
		     "szescioro", "siedmioro", "osmioro", "dziewiecioro" });
    num_zbior_d = ({ "jednego", "dwojga", "trojga", "czworga", "pieciorga",
		     "szesciorga", "siedmiorga", "osmiorga", "dziewieciorga" });                      
	
    num_d_0 = ({ "jednego", "dwoch", "trzech", "czterech", "pieciu",
		    "szesciu", "siedmiu", "osmiu", "dziewieciu", "dziesieciu",
		    "jedenastu", "dwunastu", "trzynastu", "czternastu",
		    "pietnastu", "szesnastu", "siedemnastu", "osiemnastu",
		    "dziewietnastu" });
    num_d_1 = ({ "dwudziestu", "trzydziestu", "czterdziestu", "piecdziesieciu",
		    "szescdziesieciu", "siedemdziesieciu", "osiemdziesieciu",
		    "dziewiecdziesieciu" });

    num_n_0 = ({ "jednym", "dwoma", "trzema", "czterema", "piecioma",
		 "szescioma", "siedmioma", "osmioma", "dziewiecioma",
		 "dziesiecioma", "jedenastoma", "dwunastoma", "trzynastoma",
		 "czternastoma", "pietnastoma", "szesnastoma", 
		 "siedemnastoma", "osiemnastoma", "dziewietnastoma" });
    num_n_1 = ({ "dwudziestoma", "trzydziestoma", "czterdziestoma",
		 "piecdziesiecioma", "szesciesiecioma",
		 "siedemdziesiecioma", "osiemdziesiecioma",
		 "dziewiecdziesiecioma" });

    num_zbior_n = ({ "jednym", "dwojgiem", "trojgiem", "czworgiem",
		     "pieciorgiem", "szesciorgiem", "siedmiorgiem",
		     "osmiorgiem", "dziewieciorgiem" });

    num_zbior_ms = ({ "jednym", "dwojgu", "trojgu", "czworgu", "pieciorgu",
		      "szesciorgu", "siedmiorgu", "osmiorgu",
		       "dziewieciorgu" });

    num_o_0 = ({ "pierwsz", "drug", "trzec", "czwart", "piat",
		     "szost", "siodm", "osm", "dziewiat", "dziesiat", 
		     "jedenast", "dwunast", "trzynast", "czternast",
		     "pietnast", "szesnast", "siedemnast", "osiemnast",
		     "dziewietnast" });
	       
    num_o_1 = ({ "dwudziest", "trzydziest", "czterdziest", 
		     "piecdziesiat", "szescdziesiat", "siedemdziesiat",
		     "osiemdziesiat", "dziewiecdziesiat" });                     
		
    ord1 = ({ "pierwsz", "drug", "trzec", "czwart", "piat", "szost",
	      "siodm", "osm", "dziewiat" });
	      
    ord11 = ({ "dziesiat", "jedenast", "dwunast", "trzynast", "czternast",
               "pietnast", "szesnast", "siedemnast", "osiemnast", 
               "dziewietnast" });
              
    ord20 = ({ "dwudziest", "trzydziest", "czterdziest", "piecdziesiat",
               "szescdziesiat", "siedemdziesiat", "osiemdziesiat",
               "dziewiecdziesiat" });
}

#define CFUN
#ifdef CFUN
string
article(string str) = "article";
#else
string
article(string str)
{
    string a, tmp;

    if (!str)
	return 0;

    tmp = str + " ";
    tmp = lower_case(tmp);
    if (sscanf(tmp, "the %s", a) == 1) return "";
    if (sscanf(tmp, "a%s", a) == 1) return "an";
    if (sscanf(tmp, "e%s", a) == 1) return "an";
    if (sscanf(tmp, "i%s", a) == 1) return "an";
    if (sscanf(tmp, "o%s", a) == 1) return "an";
    if (sscanf(tmp, "u%s", a) == 1) return "an";
    return "a";
}
#endif

string
add_article(string str) 
{
    string s;

    s = article(str);
    return strlen(s) ? (s + " " + str) : str;
}

/* This routine returns a number in text form
*/
string
word_number(int num)
{
    int tmp;
    
    if (num < 1) return "no";
    if (num < 20) return nums[num-1];
    if (num > 99)
    {
	if (num > 999)
	{
	    if (num > 999999)
		return "many";
	    tmp = num % 1000;
	    return word_number(num / 1000) + " thousand" +
		(tmp ? " " + word_number(tmp) : "");
	}
	tmp = num % 100;
	return word_number(num / 100) + " hundred" +
	    (tmp ? " and " +  word_number(tmp) : "");
    }
    tmp = num % 10;
    return numt[num / 10 - 2] + (tmp ? "-" + nums[tmp - 1] : "");
}

string
word_ord_number(int num)
{
    int tmp;

    if (num < 1) return "zero";
    if (num < 20) return numo[num-1];
    if (num > 99)
    {
	if (num > 999)
	{
	    if (num > 9999)   /* was 999999 */
		return "many";
	    if (num == 1000)
		return "thousandth";
	    if (!(tmp = num % 1000))
		return word_number(num / 1000) + " thousandth";
	    return word_number(num / 1000) + " thousand " +
		word_ord_number(tmp);
	}
	if (num == 100)
	    return "hundredth";
	if (!(tmp = num % 100))
	    return word_number(num / 100) + " hundredth";
	return word_number(num / 100) + " hundred " +
	    word_ord_number(tmp);
    }

    if (!(tmp = num % 10))
	return numnt[num / 10 - 2] + "ieth";
    else
	return numt[num / 10 - 2] + " " + numo[tmp - 1];
}

int
number_ord_word(string str)
{
    int i, j;
    string sstr, *nt;

    if (!str)
	return 0;

    if ((i = member_array(str, numo)) > -1)
	return i + 1;

    sstr = str[0..(strlen(str)-5)];
    if ((i = member_array(sstr, numnt)) > -1)
	return (i + 2) * 10;

    if (sizeof(nt = explode(str, " ")) != 2)
	return 0;

    if ((i = member_array(nt[1], numo)) == -1)
	return 0;

    if ((j = member_array(nt[0], numt)) == -1)
	return 0;

    return (j+2)*10 + (i+1); 
}

/* lpc singular to plural converter
*/
#ifdef CFUN
string
plural_word(string str) = "plural_word";
#else
string
plural_word(string str)
{
    string tmp, slask;
    int sl, ch;
    
    if (!str) return 0;

    switch (str)
    {
    case "tooth":
	return "teeth";
    case "foot":
	return "feet";
    case "man":
	return "men";
    case "woman":
	return "women";
    case "child":
	return "children";
    case "sheep":
	return "sheep";
    case "dwarf":
	return "dwarves";
    case "elf":
	return "elves";
    }  
    sl = strlen(str) - 1;
    if (sl < 2)
	return str;
    ch = str[sl];
    tmp = extract(str, 0, sl - 2);
    slask = extract(str, sl - 1, sl - 1);
    switch (ch)
    {
    case 's':
	return tmp + slask + "ses";
    case 'x':
	return tmp + slask + "xes";
    case 'h':
	return tmp + slask + "hes";
    case 'y':
	if (member_array(slask, ({ "a", "e", "o" })) >= 0)
	    return tmp + slask + "ys";
	else
	    return tmp + slask + "ies";
    case 'e':
	if (slask == "f")
	    return tmp + "ves";
    }
    return str + "s";
}
#endif

string
plural_sentence(string str)
{
    int  c;
    string *a;
    
    if (!str)
	return 0;
    
    a = explode(str + " ", " ");
    if ((!a) || (sizeof(a) < 1))
	return 0;
    for (c = 1; c < sizeof(a); c++)
    {
	if (lower_case(a[c]) == "of")
	{
	    a[c - 1] = plural_word(a[c - 1]);
	    return implode(a, " ");
	}
    }
    a[sizeof(a) - 1] = plural_word(a[sizeof(a) - 1]);
    return implode(a, " ");
}

/* Verb in present tence (not ready yet)
*/
string
verb_present(string str)
{
  return str;
}

/* Name in possessive form
*/
string
name_possessive(string str)
{
    if (!strlen(str))
	return 0;

    switch (str)
    {
    case "it": return "its";
    case "It": return "Its";
    case "he": return "his";
    case "He": return "His";
    case "she": return "her";
    case "She": return "Her";
    }
    if (extract(str, strlen(str) - 1) == "s")
	return (str + "'");
    return (str + "'s");
}

/* This routine returns a textform as a number
 */
int
number_word(string str)
{
    string *ex;
    int value, pos;

    if (sscanf(str, "%d", value))
	return value;

    value = 0;

    ex = explode(str, "y");

    if (!sizeof(ex))
	return 0;

    if (sizeof(ex) > 1)
    {
	if ((pos = member_array(ex[0], numnt)) == -1)
	    return 0;
	else
	    value = (pos + 2) * 10;
    }

    if (value)
    {
	if (ex[1][0] == ' ' || ex[1][0] == '-')
	    ex[1] = extract(ex[1], 1, strlen(ex[1]) - 1);
	if ((pos = member_array(ex[1], nums)) == -1)
	    return 0;
	else
	    value += pos + 1;
    }

    if (!value)
	if ((pos = member_array(ex[0], nums)) == -1)
	    {
		if ((pos = member_array(ex[0], numnt)) == -1)
		    return 0;
		else
		    value = (pos + 2) * 10;
	    }
	else
	    value = pos + 1;

    return value;
}

string
singular_form(string str)
{
    string singular, one, two, three;
    int last;

    last = strlen(str);
    one  = extract(str, last - 1, last - 1);
    two  = extract(str, last - 2, last - 2);
    three = extract(str, last - 3, last - 3);

    if (one != "s")
	return str;
    if (two != "e" || three == "c")
	return extract(str, 0, last - 2);
    if (three != "v")
	return extract(str, 0, last - 3);

    return extract(str, 0, last - 4) + "fe";
}

string
lang_a_short(object ob)
{
    if (ob->query_prop(HEAP_I_IS))
	return ob->short();
    return add_article(ob->short());
}

string
lang_the_short(object ob)
{
    if (ob->query_prop(HEAP_I_IS))
	return ob->short();
    return "the " + ob->short();
}


/*
 * Nazwa funkcji : num_slowo
 * Opis          : Zwraca postac wartosc podanego w postaci tekstowej
 *		   liczebnika glownego.
 * Argumenty     : string - liczebnik glowny w postaci tekstowej, w 
 *			    dowolnym przypadku i rodzaju.
 * Funkcja zwraca: int - wartosc liczebnika glownego, jego numeryczna 
 *                       postac.
 */
public int
num_slowo(string str)
{
    string temp, *slowa;
    int liczba_slow, numer, x;
    
    slowa = explode(str, " ");
    if ((liczba_slow = sizeof(slowa)) == 1)
    {
	if (sscanf("x " + str + " x", "x %d x", numer))
	    return numer;
    }
    else if (liczba_slow > 2)
        return 0;
    
    if (strlen(str) < 3)
        return 0;

    temp = extract(str, 0, 1);
    
    if (liczba_slow == 1)
    {
	switch (temp)
	{
	    case "je" : if ((numer = member_array(str, je)) != -1)
			{
			    if (numer < 7) return 1;
			    else return 11;
			}
			return 0;
			break; 
 	    case "dw" : if ((numer = member_array(str, dw)) != -1)
			{
			    if (numer < 6) return 2;
			    else if (numer < 9) return 12;
			    else return 20;
			}
			return 0;
			break;
	    case "tr" : if ((numer = member_array(str, tr)) != -1)
			{
			    if (numer < 5) return 3;
			    else if (numer < 8) return 13;
			    else return 30;
			}
			return 0;
			break;
	    case "cz" : if ((numer = member_array(str, cz)) != -1)
			{
			    if (numer < 5) return 4;
			    else if (numer < 8) return 14;
			    else return 40;
			}
			return 0;
			break;
	    case "pi" : if ((numer = member_array(str, pi)) != -1)
			{
			    if (numer < 3) return 5;
			    else if (numer < 6) return 15;
			    else return 50;
			}
			return 0;
			break;
	    case "sz" : if ((numer = member_array(str, sz)) != -1)
			{
			    if (numer < 3) return 6;
			    else if (numer < 6) return 16;
			    else return 60;
			}
			return 0;
			break;
	    case "si" : if ((numer = member_array(str, si)) != -1)
			{
			    if (numer < 3) return 7;
			    else if (numer < 6) return 17;
			    else return 70;
			}
			return 0;
			break;
	    case "os" : if ((numer = member_array(str, os)) != -1)
			{
			    if (numer < 3) return 8;
			    else if (numer < 6) return 18;
			    else return 80;
			}
			return 0;
			break;
	    case "dz" : if ((numer = member_array(str, dz)) != -1)
			{
			    if (numer < 3) return 9;
			    else if (numer < 6) return 10;
			    else if (numer < 9) return 19;
			    else return 90;
			}
			return 0;
			break;
	    default: return 0; 
	} 
    } 
    else
    {
       switch (temp)
       {
	   case "dw" : if (member_array(slowa[0], dw) < 9) return 0;
			  x = 20; break;
	   case "tr" : if (member_array(slowa[0], tr) < 8) return 0;
			  x = 30; break;
	   case "cz" : if (member_array(slowa[0], cz) < 8) return 0;
			  x = 40; break;
	   case "pi" : if (member_array(slowa[0], pi) < 6) return 0;
			  x = 50; break;       
	   case "sz" : if (member_array(slowa[0], sz) < 6) return 0;
			  x = 60; break;
	   case "si" : if (member_array(slowa[0], si) < 6) return 0;
			  x = 70; break;
	   case "os" : if (member_array(slowa[0], os) < 6) return 0;
			  x = 80; break;
	   case "dz" : if (member_array(slowa[0], dz) < 9) return 0;
			  x = 90; break;       
	   default : return 0;
       }
       
       switch (extract(slowa[1], 0, 1))
       {
	   case "je" : if (member_array(slowa[1], je) > 6) return 0;
			 else return (x + 1); break;
	   case "dw" : if (member_array(slowa[1], dw) > 5) return 0;
			 else return (x + 2); break;
	   case "tr" : if (member_array(slowa[1], tr) > 4) return 0;
			 else return (x + 3); break;
	   case "cz" : if (member_array(slowa[1], cz) > 4) return 0;
			 else return (x + 4); break;
	   case "pi" : if (member_array(slowa[1], pi) > 2) return 0;
			 else return (x + 5); break;
	   case "sz" : if (member_array(slowa[1], sz) > 2) return 0;
			 else return (x + 6); break;           
	   case "si" : if (member_array(slowa[1], si) > 2) return 0;
			 else return (x + 7); break;           
	   case "os" : if (member_array(slowa[1], os) > 2) return 0;
			 else return (x + 8); break;
	   case "dz" : if (member_array(slowa[1], dz) > 2) return 0;
			 else return (x + 9); break;
	   default : return 0;
       }
    }
    return 0;
}


/*
 * Nazwa funkcji : num_ord_slowo
 * Opis          : Zwraca postac wartosc podanego w postaci tekstowej
 *		   liczebnika porzadkowego.
 * Argumenty     : string - liczebnik porzadkowy w postaci tekstowej, w 
 *			    dowolnym przypadku i rodzaju.
 * Funkcja zwraca: int - wartosc liczebnika porzadkowego, jego numeryczna 
 *                       postac.
 */
public int
num_ord_slowo(string str)
{
    int numer, liczba_slow, member, szf, str_len, x;
    string *slowa, koncowka, temat;

    slowa = explode(str, " ");
    if ((liczba_slow = sizeof(slowa)) == 1) 
    {
	if (sscanf(str, "%d", numer))
	    return numer;
    }
    else if (liczba_slow > 2)
        return 0;

    if (strlen(str) < 4)
        return 0;

    if (liczba_slow == 2)
    {
        str = slowa[0];
        
        str_len = strlen(str);

        switch(str[str_len - 1])
        {
            case 'o': x = 1; koncowka = "ego"; break;
            case 'u': x = 2; koncowka = "emu"; break;
            case 'j': x = 3; koncowka = "ej"; break;
            case 'e': x = 4; koncowka = "e"; break;
            case 'a': x = 5; koncowka = "a"; break;
            case 'm': x = 6; koncowka = "m"; break;
            case 'i':
            case 'y': x = 7; koncowka = ""; break;
            default: return 0;
        }
        
        if (x == 7)
        {
            temat = str[0..str_len - 2];
        }
        else
        if (x != 6)
        {
            szf = strlen(koncowka);
            if (koncowka != str[-szf..str_len - 1])
                return 0;
            if (str[str_len - (szf + 1)] == 'i')
                temat = str[0..str_len - (szf + 2)];
            else
                temat = str[0..str_len - (szf + 1)];
        }
        else
        {
            switch(str[str_len - 2])
            {
                case 'y':
                case 'i': temat = str[0..str_len - 3]; break;
                default: return 0;
            }
        }
        
        if ((member = member_array(temat, ord20)) == -1)
            return 0;
        
        numer = (member + 2) * 10;
        
        /* --- slowo pierwsze --- */
        
        str = slowa[1];
        
        str_len = strlen(str);

        if (x == 7)
        {
            temat = str[0..str_len - 2];
        }
        else
        if (x != 6)
        {
            szf = strlen(koncowka);
            if (koncowka != str[-szf..str_len - 1])
                return 0;
            if (str[str_len - (szf + 1)] == 'i')
                temat = str[0..str_len - (szf + 2)];
            else
                temat = str[0..str_len - (szf + 1)];
        }
        else
        {
            switch(str[str_len - 2])
            {
                case 'y':
                case 'i': temat = str[0..str_len - 3]; break;
                default: return 0;
            }
        }
        
        if ((member = member_array(temat, ord1)) == -1)
            return 0;
        
        numer += (member + 1);
        
        return numer;
        
    }

    str_len = strlen(str);

    switch(str[str_len - 1])
    {
        case 'o': x = 1; koncowka = "ego"; break;
        case 'u': x = 2; koncowka = "emu"; break;
        case 'j': x = 3; koncowka = "ej"; break;
        case 'e': x = 4; koncowka = "e"; break;
        case 'a': x = 5; koncowka = "a"; break;
        case 'm': x = 6; koncowka = "m"; break;
        case 'i':
        case 'y': x = 7; koncowka = ""; break;
        default: return 0;
    }
    
    if (x == 7)
    {
        temat = str[0..str_len - 2];
    }
    else
    if (x != 6)
    {
        szf = strlen(koncowka);
        if (koncowka != str[-szf..str_len - 1])
            return 0;
        if (str[str_len - (szf + 1)] == 'i')
            temat = str[0..str_len - (szf + 2)];
        else
            temat = str[0..str_len - (szf + 1)];
    }
    else
    {
        switch(str[str_len - 2])
        {
            case 'y':
            case 'i': temat = str[0..str_len - 3]; break;
            default: return 0;
        }
    }
    
    if ((member = member_array(temat, ord1)) != -1)
        return (member + 1);
    else if ((member = member_array(temat, ord11)) != -1)
        return (member + 10);
    else if ((member = member_array(temat, ord20)) != -1)
        return ((member + 2) * 10);
    else
        return 0;
}


/*
 * Nazwa funkcji : slowo_num
 * Opis          : Zwraca tekstowa postac liczebnika glownego, o podanej
 *		   liczbie, przypadku i rodzaju gramatycznym.
 * Argumenty     : int liczba - wartosc szukanego liczebnika,
 *	           int przyp  - przypadek szukanego liczebnika,
 *		   int rodzaj - rodzaj gramatyczny szukanego liczebnika.
 * Funkcja zwraca: string - szukany liczebnik glowny, w postaci
 *		            tekstowej.
 */
public string
slowo_num(int liczba, int przypadek, int rodzaj)
{
    string tmp;

    if (liczba < 0)
        throw("Liczba ujemna\n");

    if (przypadek < PL_MIA || przypadek > PL_MIE)
        throw("Niewlasciwy przypadek\n");

    if (rodzaj < PL_MESKI_OS || rodzaj > PL_NIJAKI_NOS)  
        throw("Niewlasciwy rodzaj\n");

    if (liczba == 0)
        return "zero";
    else if (liczba >= 100)
        return "" + liczba;

    if (rodzaj == PL_NIJAKI_OS || rodzaj == PL_NIJAKI_NOS)
    {
        if (liczba == 1)
        {
	    switch(przypadek)
	    {
	        case PL_MIA: return "jedno";
	        case PL_DOP: return "jednego";
	        case PL_CEL: return "jednemu";
	        case PL_BIE: return "jedno";
	        case PL_NAR:
	        case PL_MIE: return "jednym";
	    }
        }
        if (rodzaj == PL_NIJAKI_NOS)
            rodzaj = PL_MESKI_NOS_NZYW;
    }
    
    if (przypadek == PL_BIE)
        przypadek = ((rodzaj == PL_MESKI_OS) ? PL_DOP : PL_MIA);
    
    switch (przypadek)
    {
	case PL_MIA:
	      switch (rodzaj)
	      {
	          case PL_MESKI_OS:
		      if (liczba == 1) return "jeden";
		      if (liczba == 2) return "dwaj";
		      if (liczba == 3) return "trzej";
		      if (liczba == 4) return "czterej";
		      if (liczba < 20) return (num_d_0[liczba-1]);
		      if (liczba < 100) return (num_d_1[liczba/10-2]+
			 (liczba % 10 == 0 ? "" : " " + 
			    num_d_0[liczba%10 - 1]));
		      return ("wielu");
		      
		  case PL_ZENSKI:
		      if (liczba == 1) return "jedna";
		      if (liczba == 2) return "dwie";
		      if (liczba < 20) return (num_glowne_0[liczba - 1]);
		      if (liczba > 99) return "wiele";
		      if (liczba%10 == 0)
		          return (num_glowne_1[liczba / 10 - 2]);
		      if (liczba%10 == 1) 
		      	    return (num_glowne_1[liczba / 10 - 2] + " jeden");
		      if (liczba%10 == 2) 
		            return (num_glowne_1[liczba / 10 - 2] + " dwie");
		      return (num_glowne_1[liczba / 10 - 2] + " " +
			      num_glowne_0[liczba%10 - 1]);
			      
		  case PL_NIJAKI_OS:
		      if (liczba < 10) return (num_zbior_m[liczba - 1]);
		      if (liczba < 20) return (num_glowne_0[liczba - 1]);
		      if (liczba < 100) 
		          return (num_glowne_1[liczba / 10 - 2] +
		            (liczba%10 == 0 ? "" : " " +
		            num_glowne_0[liczba%10 - 1]));
		      return "wiele";
		      
		  case PL_MESKI_NOS_ZYW: 
		  case PL_MESKI_NOS_NZYW:
		      if (liczba < 20) 
		          return (num_glowne_0[liczba - 1]);
		      if (liczba < 100) 
		          return (num_glowne_1[liczba / 10 - 2] +
		            (liczba%10 == 0 ? "" : " " + 
		            num_glowne_0[liczba%10 - 1]));
		      return "wiele";
	      }
	case PL_DOP:
		if (rodzaj == PL_ZENSKI && liczba == 1) 
		    return "jednej";
		if (rodzaj == PL_NIJAKI_OS && liczba < 10) 
		    return (num_zbior_d[liczba - 1]);
		if (liczba < 20) 
		    return (num_d_0[liczba - 1]);
		if (liczba < 100 && liczba%10 == 1) 
		    return (num_d_1[liczba / 10 - 2] + " jeden");
		if (liczba < 100 && liczba%10 == 0) 
		    return (num_d_1[liczba / 10 - 2]);
		if (liczba < 100) 
		    return (num_d_1[liczba / 10 - 2] + " " + 
		        num_d_0[liczba%10 - 1]);
		return "wielu";
	case PL_CEL:  /* celownik */
		if (liczba == 1 && rodzaj != PL_ZENSKI) return "jednemu";
		if (liczba%10 == 2) tmp = "dwom";
		if (liczba%10 == 3) tmp = "trzem";
		if (liczba%10 == 4) tmp = "czterem";
		if (liczba < 5) return tmp;
		if (liczba < 20) return (num_d_0[liczba-1]);
		if (liczba%10 > 1 && liczba%10 < 5) 
		    return (num_d_1[liczba / 10 - 2] + " " + tmp);
		if (liczba < 100)
		    return (num_d_1[liczba / 10 - 2]+
		    (liczba % 10 == 0 ? "" : " "+num_d_0[liczba%10 - 1]));
		return "wielu";
	case PL_NAR:
		if (liczba == 1 && rodzaj == 1) 
		    return "jedna";
		if (rodzaj == 2 && liczba < 10) 
		    return (num_zbior_n[liczba - 1]);
		if (liczba < 20) 
		    return (num_n_0[liczba - 1]);
		if (liczba < 100 && liczba%10 == 1) 
		    return (num_n_1[liczba / 10 - 2] + " jednoma");
/* Tu moze byc troche niezrecznie... czterdziestoma jednoma... Alvin */
		    
		if (liczba < 100) 
		    return (num_n_1[liczba / 10 - 2] + (liczba%10 == 0 
		        ? "" : " " + num_n_0[liczba%10 - 1]));
		return "wieloma";
		
	case PL_MIE:
		if (liczba == 1 && rodzaj == 1) 
		    return "jednej";
		if (rodzaj == 2 && liczba < 10) 
		    return (num_zbior_ms[liczba - 1]);
		if (liczba < 20) 
		    return (num_d_0[liczba - 1]);
		if (liczba < 100 && liczba%10 == 1) 
		    return (num_d_1[liczba/10 - 2] + " jeden");
		if (liczba < 100) return (num_d_1[liczba / 10 - 2] +
		    (liczba%10 == 0 ? "" : " " + num_d_0[liczba%10 - 1]));
		return "wielu";
    }
}

/*
 * Nazwa funkcji : slowo_ord_num
 * Opis          : Zwraca tekstowa postac liczebnika porzadkowego, 
 *		   o podanej liczbie, przypadku i rodzaju.
 * Argumenty     : int liczba - wartosc szukanego liczebnika,
 *	           int przyp  - przypadek szukanego liczebnika,
 *		   int rodzaj - rodzaj gramatyczny szukanego liczebnika.
 * Funkcja zwraca: string - szukany liczebnik porzadkowy, w postaci
 *		            tekstowej.
 */
public string
slowo_ord_num(int liczba, int przypadek, int rodzaj)
{
    string koncowka, dwa_konc = "";
    
    if (przypadek < PL_MIA || przypadek > PL_MIE)
	throw("Niewlasciwy przypadek w slowo_ord_num\n");
	
    if (liczba == 0) return "zero";

    if (liczba < 0) throw("Ujemna liczba\n");
    
    if (rodzaj < 0 || rodzaj > 5) throw("Niewlasciwy rodzaj\n");
    
    switch (przypadek)
    {
    case PL_MIA:
	switch (rodzaj)
	{
	    case PL_MESKI_OS:
	    case PL_MESKI_NOS_ZYW:
	    case PL_MESKI_NOS_NZYW:  /* Wszystkie meskie */
		koncowka = "y";
		break;
		
	    case PL_ZENSKI:
		koncowka = "a";
		break;
		
	    case PL_NIJAKI_OS:
	    case PL_NIJAKI_NOS:
		koncowka = "e";
	}
	break;
	
    case PL_DOP:
	switch(rodzaj)
	{
	    case PL_MESKI_OS:
	    case PL_MESKI_NOS_ZYW:
	    case PL_MESKI_NOS_NZYW:  /* Wszystkie meskie */
		koncowka = "ego";
		break;
		
	    case PL_ZENSKI:
		koncowka = "ej";
		break;
		
	    case PL_NIJAKI_OS:
	    case PL_NIJAKI_NOS:
		koncowka = "ego";
	}
	break;
	
    case PL_CEL:
	switch(rodzaj)
	{
	    case PL_MESKI_OS:
	    case PL_MESKI_NOS_ZYW:
	    case PL_MESKI_NOS_NZYW:  /* Wszystkie meskie */
		koncowka = "emu";
		break;
		
	    case PL_ZENSKI:
		koncowka = "ej";
		break;
		
	    case PL_NIJAKI_OS:
	    case PL_NIJAKI_NOS:
		koncowka = "emu";
	}
	break;
	
    case PL_BIE:
	switch(rodzaj)
	{
	    case PL_MESKI_OS:
	    case PL_MESKI_NOS_ZYW:
	    case PL_MESKI_NOS_NZYW:  /* Wszystkie meskie */
		koncowka = "ego";
		break;
		
            case PL_ZENSKI:
       	        koncowka = "a";
       	        break;
       	 
            case PL_NIJAKI_OS:
            case PL_NIJAKI_NOS:
       	        koncowka = "e";
	}
	break;
	
    case PL_NAR:
	switch(rodzaj)
	{
	    case PL_MESKI_OS:
	    case PL_MESKI_NOS_ZYW:
	    case PL_MESKI_NOS_NZYW:  /* Wszystkie meskie */
		koncowka = "ym";
		break;
		
	    case PL_ZENSKI:
		koncowka = "a";
		break;
		
	    case PL_NIJAKI_OS:
	    case PL_NIJAKI_NOS:
		 koncowka = "ym";
	}
	break;
	
    case PL_MIE:
	switch(rodzaj)
	{
	    case PL_MESKI_OS:
	    case PL_MESKI_NOS_ZYW:
	    case PL_MESKI_NOS_NZYW:  /* Wszystkie meskie */
		koncowka = "ym";
		break;
		
	     case PL_ZENSKI:
		 koncowka = "ej";
		 break;
		 
	     case PL_NIJAKI_OS:
	     case PL_NIJAKI_NOS:
		 koncowka = "ym";
	}
    }
    
/* A teraz, jak to w jezyku polskim... */
/*    .......WYJATKI.......            */

    if (liczba % 10 == 2 || liczba % 10 == 3)
    {
        switch (rodzaj)
	{
	    case PL_MESKI_OS:
	    case PL_MESKI_NOS_ZYW:
	    case PL_MESKI_NOS_NZYW:
		switch (przypadek)
		{
		    case PL_MIA: dwa_konc = "i"; break;
		    case PL_DOP:
		    case PL_CEL:
		    case PL_BIE: dwa_konc = "i" + koncowka; break;
		    case PL_NAR:
		    case PL_MIE: dwa_konc = "im";
		}
		break;
		
	    case PL_ZENSKI:
	        switch (przypadek)
	        {
	            case PL_MIA: 
	            case PL_BIE: 
	            case PL_NAR: if (liczba % 10 == 3) dwa_konc = "ia";
	                	    else dwa_konc = koncowka; 
	            	break;
		    default:     dwa_konc = "iej"; break;
	        }
	        break;
	        
	    case PL_NIJAKI_OS:
	    case PL_NIJAKI_NOS:
	        switch (przypadek)
	        {
		    case PL_MIA:
		    case PL_DOP:
		    case PL_CEL:
		    case PL_BIE: dwa_konc = "i" + koncowka; break;
		    default: dwa_konc = "im";
	      }
	}
	
	if (liczba == 2) return "drug" + dwa_konc;
	if (liczba == 12) return "dwunast" + koncowka;
	if (liczba == 3) return "trzec" + dwa_konc;
	if (liczba == 13) return "trzynast" + koncowka;
	
    }    
    /* Koniec wyjatkow */
    
    /* teraz czesc wspolna dla wszystkich */

    if (liczba < 20) return (num_o_0[liczba - 1] + koncowka);
    
    if (liczba < 100) return (num_o_1[liczba / 10 - 2] + koncowka +
	(liczba % 10 == 0 ? "" : " " + num_o_0[liczba % 10 - 1] + 
	(dwa_konc == "" ? koncowka : dwa_konc)));

    /* Ta czesc jest wywolywana gdy liczba > 99 */
    return "" + liczba;
/*
    switch (rodzaj)
    {
	case PL_MESKI_OS:
	case PL_MESKI_NOS_ZYW:
	case PL_MESKI_NOS_NZYW: switch (przypadek)
		{
		    case PL_MIA: return "ktorys";
		    case PL_DOP: return "ktoregos";
		    case PL_CEL: return "ktoremus";
		    case PL_BIE: return "ktoregos";
		    case PL_NAR: return "ktoryms";
		    case PL_CEL: return "ktoryms";
		}
	case PL_ZENSKI: switch (przypadek)
		{
		    case PL_MIA: return "ktoras";
		    case PL_DOP: return "ktorejs";
		    case PL_CEL: return "ktorejs";
		    case PL_BIE: return "ktoras";
		    case PL_NAR: return "ktoras";
		    case PL_MIE: return "ktorejs";
		}
	case PL_NIJAKI_OS:
	case PL_NIJAKI_NOS: switch (przypadek)
		{
		    case PL_MIA: return "ktores";
		    case PL_DOP: return "ktoregos";
		    case PL_CEL: return "ktoremus";
		    case PL_BIE: return "ktores";
		    case PL_NAR: return "ktoryms";
		    case PL_MIE: return "ktoryms";
		}
    }
*/
}


/*
 * Nazwa funkcji : query_przyp_rzeczow
 * Opis          : Funkcja zwraca przypadek, w jakim bedzie musial byc
 *		   rzeczownik stojacy obok liczebnika, o podanej liczbie,
 *		   przypadku i rodzaju. Albowiem jest on czesto
 *		   rozny od przypadku liczebnika.
 *		   Np. liczebnik meskoosobowy 'pieciu' (mianownik),
 *		   wymaga rzeczownika w dopelniaczu - 'mezczyzn', a wiec
 *		   query_przyp_rzeczow(5, PL_MIA, PL_MESKI_OS) == PL_DOP.
 * Argumenty     : int liczba - wartosc liczebnika, przy ktorym stoi
 *				rzeczownik,
 *		   int przypadek - przypadek tego liczebnika,
 *		   int rodzaj - rodzaj gramatyczny owego liczebnika.
 * Funkcja zwraca: int - przypadek, w ktorym powinien stac rzeczownik.
 */
public int
query_przyp_rzeczow(int liczba, int przyp, int rodzaj)
{
    if (liczba < 0)
        throw("Liczba ujemna.\n");
        
    if (rodzaj < PL_MESKI_OS || rodzaj > PL_NIJAKI_NOS)
        throw("Niewlasciwy rodzaj gramatyczny.\n");
        
    if (przyp < PL_MIA || przyp > PL_MIE)
        throw("Niewlasciwy przypadek.\n");
        
    if (liczba == 1) return przyp;
    if (liczba == 0) return PL_DOP;
    
    if (przyp != PL_MIA && przyp != PL_BIE)
        return przyp;
        
    if (rodzaj == PL_NIJAKI_OS)
        return PL_DOP;
    
    if ((liczba % 10 > 1) && (liczba % 10 < 5) && ((liczba / 10) % 10 != 1))
        return przyp;
        
    return PL_DOP;
}

/*
 * Nazwa funkcji  : wolacz
 * Opis           : Funkcja znajduje wolacz liczby pojedynczej imienia na
 *                  podstawie pozostalych przypadkow.
 * Argumenty      : mia: Mianownik liczby pojedynczej
 *                  mie: Miejscownik liczby pojedynczej.
 * Uwaga          : Argumenty powinny zaczynac sie duza litera.
 * Funkcja zwraca : Znaleziony wolacz.
 */
public string
wolacz(string mia, string mie)
{
    if (strlen(mia) < 3 || strlen(mie) < 3)
        return "Przybyszu";

    switch (mia[-1..])
    {
        case "a":
            if (mia == mie)
                return mia;
            switch (mia)
            {
                case "Adela":
                case "Ala":
                case "Aniela":
                case "Ela":
                case "Katia":
                case "Mariola":
                case "Ola":
                    return mia[..-2] + "u";
            }
            switch (mie[-2..])
            {
                case "ej":
                    return mia;
                case "ii":
                case "ji":
                    return mia[..-2] + "o";
            }
            switch (mia[-2..])
            {
                case "ia":
                case "ja":
                    return mia[..-2] + "u";
            }
            if (strlen(mia) == 4)
                switch (mia[-3..])
                {
                    case "ela":
                    case "ola":
                        return mia[..-2] + "u";
                }
            return mia[..-2] + "o";
        case "e":
        case "u":
            return mia;
        case "i":
        case "o":
        case "y":
            if (mie[-1..] == "u")
                return mie;
            return mia;
    }

    if (mia[-2..] == "ec" && mie[-2..] == "cu" && mie[-3..] != "ecu")
        return mie[..-3] + "cze";

    return mie;
}