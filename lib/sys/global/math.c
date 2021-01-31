/*
 * /sys/global/math.c
 *
 * Some useful math routines.
 */

#pragma no_clone
#pragma no_inherit
#pragma no_shadow
#pragma save_binary
#pragma strict_types

/*
 * Function name: find_exp
 * Description  : Find the solution to 2^x = sum.
 * Arguments    : sum - The sum to find the solution to.
 * Returns      : x
 */
public nomask int
find_exp(int sum)
{
    int x, i;
    
    x = 1;
    while (sum > x)
    {
	i++;
	x += x;
    }
    
    return i;
}

/*
 * Global variable used in the function quick_find_exp().
 */
static private mapping lookup =
    ([ 2:1, 4:2, 8:3, 16:4, 32:5, 64:6, 128:7, 256:8, 512:9 ]);

/*
 * Function name: quick_find_exp
 * Description  : This function is a quick hack that returns the solution
 *                to the function 2^x = sum for a rather limited set of
 *                sum. Accepted values are the powers of 2 ranging from 1
 *                to 512.
 * Arguments    : int sum - the sum to find x for.
 * Returns      : int - the matching x, else 0.
 */
public nomask int
quick_find_exp(int sum)
{
    return lookup[sum];
}

/*
 * Function name: decimal_to_binary
 * Description  : Convert a base 10 unsigned integer to its base 2
 *                representation.
 * Arguments    : num: The number to convert.
 * Returns      : A binary array with the base 2 number, MSB first.
 */
public nomask int *
decimal_to_binary(int num)
{
    int *bit_array = ({ });

    /* Should we also check for MAXINT here? */
    if (num <= 0)
    {
        return ({ 0 });
    }

    /* Put more significant bits on top of the array */
    while(num)
    {
        bit_array = ({ (num % 2) }) + bit_array;

        num /= 2;
    }

    return bit_array;
}

/*
 * Function name: binary_to_decimal
 * Description  : Convert a base 2 unsigned integer to its base 10
 *                representation.
 * Arguments    : A binary array of the base 2 number, MSB first.
 * Returns      : The unsigned decimal integer.
 */
public nomask int
binary_to_decimal(int *bit_array)
{
    int num = 0;
    int bit = -1;
    int size = sizeof(bit_array);

    while(++bit < size)
    {
        num += num + bit_array[bit];
    }

    return num;
}

/*
 * Function name: square_root
 * Description  : Find the x for which x^2 = y, the square root
 *                If y < 0, x will be 0
 * Arguments    : square - the square to find the root to
 * Returns      : int - the root
 */
public nomask int
square_root(int square)
{
    if (square <= 0)
    {
	return 0;
    }

    return ftoi(pow(itof(square), 0.5));
}

#define PRIMES   ({ 3, 7, 13, 17, 23, 37, 43, 47, 53, 67, 73 })
#define NAME_LEN 11
#define CHAR_a   ('a' - 1)

/*
 * Function name: name_to_random
 * Description  : With this function, each combination of name, seed and
 *                range will return the same value each time you call it.
 *                This way, for quests or other hints, the same player
 *                gets the same value each time.
 * Arguments    : string name  - the lower case name of the player
 *                int    seed  - 
 *                int    range - the range to compute the number in
 * Returns      : int          - the randomized value.
 */
public nomask int
name_to_random(string name, int seed, int range)
{
    int index  = -1;
    int result = 0;
    int size;
    int char;

    /* Do not ever believe people follow your instructions. */
    name = lower_case(name);

    /* Player names cannot be larger than NAME_LEN characters, though
     * people may feed us with other strings.
     */
    if (strlen(name) > NAME_LEN)
    {
	name = extract(name, 0, (NAME_LEN - 1));
    }

    /* To find the result, we apply the same formula to each character
     * of the name, in addition with its rank, the seed and the first
     * primes ending with 3 or 7.
     */
    size = strlen(name);
    while(++index < size)
    {
	char = name[index] - CHAR_a;
	result += ((seed / char) + (char * PRIMES[NAME_LEN - index - 1]) +
	    (seed % (index + 1)));
    }

    /* Reduce the result to a number in the range 0 .. (range - 1). */
    return (result % range);
}

