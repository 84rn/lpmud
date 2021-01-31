/*
 * This file specifies types and arguments for efuns.
 * An argument can have two different types with the syntax 'type1 | type2'.
 * An argument is marked as optional if it also takes the type 'void'.
 *
 * Look at the end for the list of functions that are optionally available.
 * If you don't want them, simply comment out them. All other functions must
 * remain defined.
 */

#include "config.h"

void	add_action(string|function, void|string, void|int);
object	*all_inventory(object default: F_THIS_OBJECT);
mixed	*allocate(int);
#ifdef DEBUG
void	break_point();
#endif
string	break_string(int|string, int, void|int|string);
mixed	call_other(mapping|object|string|int|object *, string, ...);
mixed	call_otherv(mapping|object|string|int|object *, string, mixed *);
mixed   call_self(string, ...);
mixed   call_selfv(string, mixed *);
object  calling_object(int default: F_CONST0);
string  calling_program(int default: F_CONST0);
string  calling_function(int default: F_CONST0);
string	capitalize(string|int);
string	clear_bit(string, int);
object	clone_object(string);
int	command(string);
mixed  *commands(object|int default: F_THIS_OBJECT);
string	crypt(string, string|int);
string	ctime(int);
mixed	debug(string, ...);
object	*deep_inventory(int|object default: F_THIS_OBJECT);
void	destruct();
void	disable_commands();
void	ed(void|string, void|string|function);
void	enable_commands(void);
object	environment(object default: F_THIS_OBJECT);
int	exec(object|int, object);
string	*explode(string, string);
string	extract(string, void|int, void|int);
string	file_name(object default: F_THIS_OBJECT);
int	file_size(string);
int	file_time(string);
mixed	*filter(int|mapping|mixed *, string|function, void|object|string, void|mixed);
mixed	find_living(string, void|int);
object	find_object(string);
object	find_player(string);
int     floatp(mixed);
string	function_exists(string, object default: F_THIS_OBJECT);
object	function_object(function);
string	function_name(function);
int	functionp(mixed);
string	*get_dir(string);
mixed   *get_all_alarms();
mixed   *get_alarm(int);
string	implode(int|string *, string);
void	input_to(string|function, ...);
int	intp(mixed);
int     last_reference_time();
int	living(object|int);
string	lower_case(int|string);
string	upper_case(int|string);
mapping	m_delete(int|mapping, mixed);
mixed	*m_indexes(int|mapping);
mixed	*m_values(int|mapping);
int	m_sizeof(int|mapping);
void    m_restore_object(mapping);
mapping m_save_object();
int	mappingp(mixed);
mixed	*map(int|mapping|mixed *, string|function, void|object|string, void|mixed);
mixed	max(int|string|float, ...);
int	member_array(mixed, int|mixed *);
mixed	min(int|string|float, ...);
int	mkdir(string);
function mkfunction(string, object default: F_THIS_OBJECT);
mapping	mkmapping(int|mixed *, int|mixed *);
void	move_object(object|string);
int	notify_fail(string, void|int);
string	oblicz_przym(string, string, int, int, int);
object *object_clones(object);
int	object_time(object default: F_THIS_OBJECT);
int	objectp(mixed);
void	obsolete(string);
function papplyv(function, mixed *);
int	pointerp(mixed);
object	present(int|object|string, object *|object default: F_THIS_OBJECT);
object	previous_object(int default: F_CONST0);
string	process_string(string, int default: F_CONST0); 
mixed	process_value(string, int default: F_CONST0); 
mixed	query_auth(object);
string	query_host_name();
int	query_idle(object);
int	query_interactive(object|int);
string	query_ip_name(void|object);
string	query_ip_number(void|object);
string  query_ip_ident(object default: F_THIS_OBJECT);
object	query_snoop(object);
string	query_trigverb();
string	query_verb();
int	random(int, void|int);
string	read_bytes(string, void|int, void|int);
string	read_file(string, void|int, void|int); 
string  readable_string(string);
string	*regexp(string *, string);
void    remove_alarm(int);
int	rename(string, string);
int	restore_object(string);
mapping	restore_map(string);
int	rm(string);
int	rmdir(string);
void	save_object(string);
void	save_map(mapping, string);
int     set_alarm(float, float, string|function, ...);
int     set_alarmv(float, float, string, mixed *);
void	set_auth(object,mixed);
string	set_bit(string, int);
void	set_living_name(string);
object	shadow(object, int);
void	set_this_player(int|object default: F_THIS_OBJECT);
int	sizeof(int|mixed *);
object	snoop(void|object, void|object);
string	sprintf(string, ...);
float	sqrt(float);
int	stringp(mixed);
int	strlen(int|string);
mixed   str2val(string);
void	tail(string);
int	test_bit(string, int);
object	this_object();
object	this_player();
object	this_interactive();
void	throw(mixed);
int	time();
mixed	*unique_array(int|mixed *, string, void|mixed);
void	update_actions(object default: F_THIS_OBJECT);
object	*users();
string  val2str(mixed);
int	wildmatch(string, string);
int	write_bytes(string, int, string);
int	write_file(string, string);
void    write_socket(string|int);
float	itof(int);
int	ftoi(float);
float	sin(float);
float	cos(float);
float	tan(float);
float	asin(float);
float	acos(float);
float	atan(float);
float	atan2(float, float);
float	exp(float);
float	log(float);
float	pow(float, float);
float	sinh(float);
float	cosh(float);
float	tanh(float);
#if !defined(_SEQUENT_)
float	asinh(float);
float	acosh(float);
float	atanh(float);
#endif
float	abs(float);
float	fact(float);
string	ftoa(float);
float	rnd();

#ifdef WORD_WRAP
/*
 * These are needed to control the word wrap mechanism in comm1.
 */
void set_screen_width(int);
int query_screen_width();
#endif

