/*
 *  Plik zawierajacy kilka funkcji przydatnych podczas debugowania i przy
 *  tworzeniu logow.
 *
 *  /sys/global/debug.c
 *
 *  dzielo Silvathraeca 1997
 */

#pragma no_clone
#pragma no_inherit
#pragma no_shadow
#pragma save_binary
#pragma strict_types

/*
 * Nazwa funkcji : calling_functions
 * Opis          : Zwraca tablice niezerowych elementow ciagu
 *                 calling_function(start), calling_function(start - 1), ...
 * Argumenty     : int start - sensowne wartosci to <= 1 (gdy start == 1,
 *                             pierwszym elementem tablicy bedzie funkcja,
 *                             ktora wywolala calling_functions).
 */
public string *
calling_functions(int start = 0)
{
    string *result = ({});
    string element;

    while (element = calling_function(--start))
        result += ({element});

    return result;
}

/*
 * Nazwa funkcji : calling_objects
 * Opis          : Zwraca tablice niezerowych elementow ciagu
 *                 calling_object(start), calling_object(start - 1), ...
 * Argumenty     : int start - sensowne wartosci to <= 1 (gdy start == 1,
 *                             pierwszym elementem tablicy bedzie obiekt,
 *                             ktory wywolal calling_objects).
 */
public string *
calling_objects(int start = 0)
{
    string *result = ({});
    object element;

    while (element = calling_object(--start))
        result += ({file_name(element)});

    return result;
}

/*
 * Nazwa funkcji : previous_objects
 * Opis          : Zwraca tablice niezerowych elementow ciagu
 *                 previous_object(start), previous_object(start - 1), ...
 * Argumenty     : int start - sensowne wartosci to <= 1 (gdy start == 1,
 *                             pierwszym elementem tablicy bedzie obiekt,
 *                             ktory wywolal previous_objects).
 */
public string *
previous_objects(int start = 0)
{
    string *result = ({});
    object element;

    while (element = previous_object(--start))
        result += ({file_name(element)});

    return result;
}

/*
 * Nazwa funkcji : calling_programs
 * Opis          : Zwraca tablice niezerowych elementow ciagu
 *                 calling_program(start), calling_program(start - 1), ...
 * Argumenty     : int start - sensowne wartosci to <= 1 (gdy start == 1,
 *                             pierwszym elementem tablicy bedzie program,
 *                             ktory wywolal calling_programs).
 */
public string *
calling_programs(int start = 0)
{
    string *result = ({});
    string element;

    while (element = calling_program(--start))
        result += ({element});

    return result;
}

/*
 * Nazwa funkcji : all_calling
 * Opis          : Zwraca tablice ({calling_function(start),
 *                                  calling_object(start),
 *                                  calling_program(start)}), ...
 * Argumenty     : int start - Jak w calling_functions/objects/programs.
 */
public mixed *
all_calling(int start = 0)
{
    mixed *result = ({});
    string element;

    while (element = calling_function(--start))
        result += ({({element, file_name(calling_object(start)),
                      calling_program(start)})});

    return result;
}

/*
 * Nazwa funkcji : format_all_calling
 * Opis          : Zwraca tabelke wywolan funkcji, od zapoczatkowujacej
 *                 wywolanie do calling_function(start), w formacie podobnym
 *                 do uzywanego w debuglogu:
 *                     "funkcja1     in program1
 *                                      obiekt1
 *                      ...
 *                      funkcjaN     in progrmN
 *                                      obiektN"
 * Argumenty     : int start - sensowne wartosci to <= 1 (gdy start == 1,
 *                             ostatnim elementem tabelki bedzie funkcja,
 *                             ktora wywolala format_all_calling).
 */
public string
format_all_calling(int start = 0)
{
    string result = "";
    string element;

    while (element = calling_function(--start))
        result = sprintf("%-28s", element) + " in /" + calling_program(start)
               + "\n\t\t\t\t" + file_name(calling_object(start)) + "\n"
               + result;

    return result;
}
