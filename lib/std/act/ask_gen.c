/*
 * /std/act/asking.c
 *
 * A simple support for helping answering questions
 * Idea from Padermud, buy coded by Nick
 */

#pragma save_binary
#pragma strict_types

#include <macros.h>
#include <stdproperties.h>

static	mixed	ask_arr;		/* The triggers and answers */
static	string	default_answer;		/* A default answer if you want. */
static	string	posed_question;		/* The exact question the player posed. */
static	string	not_here_func;	 	/* A function to call if player not here */
static	int	dont_answer_unseen;	/* Flag if not to answer unseen */

/*
 * Function name: set_dont_answer_unseen
 * Description:   This mobile will look confused if he can't see who asked
 *		  him and if this flag is set
 * Arguments:     flag - How the flag should be set
 */
public void
set_dont_answer_unseen(int flag)
{
    dont_answer_unseen = flag;
}

/*
 * Function name: query_dont_answer_unseen
 * Description:   Ask about the state of the dont_answer_unseen flag
 * Returns:       The flag
 */
public int
query_dont_answer_unseen()
{
    return dont_answer_unseen;
}

/*
 * Function name: ask_id
 * Description:   Identify questions in the object.
 * Arguments:     str: String to test with.
 * Returns:       True or false.
 */
public int
ask_id(string str)
{
    int i;

    if (!ask_arr) return 0;

    for (i = 0; i < sizeof(ask_arr); i++)
        if (member_array(str, ask_arr[i][0]) >= 0) return 1;
}

/*
 * Function name: add_ask
 * Description:   Adds an question this mobile can answer. The first
 *                argument is a single string or an array of
 *                strings holding the possible question(s) of the item.
 *                The second argument is the long description of
 *                the answer. add_ask can be repeatedly called with
 *                new questions.
 * Arguments:     questions - Alternate questions for the answer,
 *                answer    - answer of the question
 *		  command   - A flag if this answer is a command
 * Returns:       True or false.
 */
public varargs int
add_ask(mixed questions, string answer, int command)
{
    if (!pointerp(questions))
	questions = ({ questions });
    if (ask_arr)
	ask_arr = ask_arr + ({ ({ questions, answer, command }) });
    else
	ask_arr = ({ ({ questions, answer, command }) });
}

/*
 * Function name: query_ask
 * Description:   Get the additional questions array.
 * Returns:       Question array, see below:

  [0] = array
     [0] ({ "name1 of question1", "name2 of question1",... })
     [1] "This is the answer of the question1."
     [2] command (1 or 0)
  [1] = array
     [0] ({ "name1 of question2", "name2 of question2", ... })
     [1] "This is the answer of the question2."
     [2] command (1 or 0)
*/
public mixed
query_ask()
{
    return ask_arr;
}

/*
 * Function name: remove_ask
 * Description:   Removes one additional answer from the additional item list
 * Arguments:     question - question to answer to remove.
 * Returns:       True or false. (True if removed successfully)
 */
public int
remove_ask(string question)
{
    int i;

    if (!pointerp(ask_arr))
        return 0;

    for (i = 0; i < sizeof(ask_arr); i++)
        if (member_array(question, ask_arr[i][0]) >= 0 )
        {
            ask_arr = exclude_array(ask_arr, i, i);
            return 1;
        }
    return 0;
}

/*
 * Function name: set_default_answer
 * Description:   Set the default answer you want your creature to say if he
 * 		  cannot identify the question.
 *		  NOTE that no reaction will come if a player asks a question
 * 		  to this mobile that we have no answer set for if this default
 *		  is not set.
 * Argument:	  answer - The default answer
 */
public void
set_default_answer(string answer)
{
    default_answer = answer;
}

/*
 * Function name: query_default_answer
 * Description:   Query the setting of the default answer
 * Returns:	  answer
 */
public string
query_default_answer()
{
    return default_answer;
}

/*
 * Function name: set_not_here_func
 * Description:   Set a function to call if the player who posed the question
 *		  has left the room before he got the answere. 
 * Arguments:	  func - The function name
 */
public void
set_not_here_func(string func)
{
    not_here_func = func;
}

/*
 * Function name: query_not_here_func
 * Description:   Query what the not here function is set to
 * Returns:	  The function name
 */
public string
query_not_here_func() { return not_here_func; }

/*
 * Function name: query_question
 * Description:   This function will return the true question the mortals posed
 * 		  to us. It can be called from a VBFC for example to do some 
 *		  testing or referring to the actual question if you want.
 * Returns: 	  question
 */
public string
query_question()
{
    return posed_question;
}

/*
 * Function name: unseen_hook
 * Description:   This function gets called if this mobile couldn't see
 *		  who asked the question and is not supposed to answer
 *		  in that case.
 */
public void
unseen_hook()
{
    command("confused");
    command("say Who asked me that?");
}

/*
 * Function name: answer_question
 * Description:   This function is called after a short delay when this mobile
 * 		  wants to react to a question
 * Arguments:  	  An array ({ string msg, int cmd })
 */
void
answer_question(string msg, int cmd)
{
    object env;

    if ((env = environment(this_object())) == environment(this_player()) ||
	    env == this_player() || (not_here_func &&
		call_other(this_object(), not_here_func, this_player())))
    {
	if (cmd)
	    command(this_object()->check_call(msg, this_player()));
	else
	    this_player()->catch_msg(msg);
    }
}

/*
 * Function name: catch_question
 * Description:	  This function is called in each living being someone asks a
 *		  question to.
 * Arguments:	  question - The question as put
 */
public void
catch_question(string question)
{
    int i;

    if (dont_answer_unseen && (!this_player()->check_seen(this_object()) ||
		!CAN_SEE_IN_ROOM(this_object())))
    {
	set_alarm(rnd() * 3.0 + 1.0, 0.0, unseen_hook);
	return;
    }

    i = strlen(question);
    if (question[i - 1] == "."[0] || question[i - 1] == "?"[0])
	question = extract(question, 0, i - 2);

    posed_question = lower_case(question);

    for (i = 0; i < sizeof(ask_arr); i++)
	if (member_array(posed_question, ask_arr[i][0]) >= 0)
	{
	    set_alarm(rnd() * 4.0, 0.0, &answer_question(ask_arr[i][1], ask_arr[i][2]));
	    return ;
	}

    if (default_answer)
	set_alarm(rnd() * 4.0, 0.0, &answer_question(default_answer, 0));
}
