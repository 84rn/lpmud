#ifndef _lint_h_
#define _lint_h_

#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>
#include <unistd.h>

#include "master.h"

#if defined(linux) && (defined(malloc) || defined(calloc))
/* Linux lossage */
#undef malloc
#undef calloc
#endif

#ifdef MALLOC_dmalloc
#include <dmalloc.h>
#define xalloc malloc
#endif

/*
 * Some structure forward declarations are needed.
 */
#define DEBUG_CHK_REF	1
#define DEBUG_RESET	2
#define DEBUG_CLEAN_UP	4
#define DEBUG_SWAP	8
#define DEBUG_OUTPUT	16
#define DEBUG_CONNECT	32
#define DEBUG_TELNET	64
#define DEBUG_RESTORE	128
#define DEBUG_OB_REF	256
#define DEBUG_PROG_REF	512
#define DEBUG_LOAD	1024
#define DEBUG_DESTRUCT	2048
#define DEBUG_LIVING	4096
#define DEBUG_COMMAND	8192
#define DEBUG_ADD_ACTION 16384
#define DEBUG_SENTENCE	32768
#define DEBUG_BREAK_POINT 65536

struct program;
struct function;
struct svalue;
struct sockaddr;
struct reloc;
struct object;
struct mapping;
struct vector;
struct closure;

#if defined(__GNUC__) && !defined(lint) && !defined(PROFILE)
#define INLINE inline
#else
#define INLINE
#endif

#ifdef SOLARIS
#define signal sigset
#define HAS_PREAD
#endif

#if 0
#ifdef BUFSIZ
#    define PROT_STDIO(x) x
#else /* BUFSIZ */
#    define PROT_STDIO(x) ()
#endif /* BUFSIZ */

#if defined(__alpha)
int mkdir (char *, int);
#endif
int pclose PROT_STDIO((FILE *));
#if !defined(_SEQUENT_) && !defined(__OpenBSD__) && !defined(__NetBSD__) && \
    !defined(__FreeBSD__) && !defined(__bsdi__)
int atoi (char *);
#endif
#if !defined(sgi) && !defined(__alpha) && !defined(__OpenBSD__) && \
    !defined(__NetBSD__) && !defined(__FreeBSD__) && !defined(__bsdi__)
void srandom (int);
#endif
int gethostname (char *, int);
void abort (void);
#if !defined(sgi) && !defined(_SEQUENT_) && !defined(__OpenBSD__) && \
    !defined(__NetBSD__) && !defined(__FreeBSD__) && !defined(__bsdi__)
#endif
#ifdef ultrix
int _flsbuf(unsigned char,FILE *);
#else
int _flsbuf();
#endif
int wait (int *);
int pipe (int *);
int dup2 (int, int);
unsigned int alarm (unsigned int);
int close (int);
int _filbuf();
#if !defined(__OpenBSD__) && !defined(__NetBSD__) && !defined(__FreeBSD__)
char *crypt (char *, char *);
#endif

#ifdef DRAND48
double drand48 (void);
void srand48 (long);
#endif
#ifdef RANDOM
long random (void);
#endif

#if !defined(_SEQUENT_) && !defined(__OpenBSD__) && !defined(__NetBSD__) && \
    !defined(__FreeBSD__) && !defined(__bsdi__)
long strtol (char *, char **, int);
#endif
#endif /* 0 */

#if defined(sun) || defined(SOLARIS)
char *_crypt (char *, char *);
#endif

#ifndef USE_SWAP
#define access_program(prog) 0
#define access_object(ob) 0
#else
void access_program(struct program *);
void access_object(struct object *);
#endif
int write_file (char *, char *);
int file_size (char *);
int file_time (char *);
void remove_all_players (void);
void backend (void);
#ifndef MALLOC_dmalloc
void *xalloc (unsigned int);
#endif
void *tmpalloc (size_t);
int new_call_out (struct closure *, int, int);
void add_action (struct closure *, struct svalue *, int);

void enable_commands (int);
void register_program(struct program*);
int tail (char *);
struct object *get_interactive_object (int);
void enter_object_hash (struct object *);
void remove_object_hash (struct object *);
struct object *lookup_object_hash (char *);
void add_otable_status (char *);
#ifdef DEALLOCATE_MEMORY_AT_SHUTDOWN
void clear_otable(void);
#endif
void free_vector (struct vector *);
char *query_load_av (void);
void update_compile_av (int);
struct vector *map_array (struct vector *arr, struct closure *);
struct vector *make_unique (struct vector *arr,char *func, struct svalue *skipnum);

struct vector *filter_arr (struct vector *arr, struct closure *);
int match_string (char *, char *);
struct object *get_empty_object (void);
void assign_svalue (struct svalue *, struct svalue *);
void assign_svalue_no_free (struct svalue *to, struct svalue *from);
void add_string_status (char *);
void notify_no_command (void);
void clear_notify (void);
void throw_error (void);
int call_var(int, struct closure *);
char *show_closure(struct closure *f);
char *getclosurename(struct closure *);
struct closure *alloc_closure(int);
int legal_closure(struct closure *);
void set_living_name (struct object *,char *);
void remove_living_name (struct object *);
struct object *find_living_object (char *);
int lookup_predef (char *);
void yyerror (char *);
int hashstr (char *, int, int);
char *dump_trace (int);
void load_parse_information (void);
void free_parse_information (void);
int parse_command (char *, struct object *);
struct svalue *apply (char *, struct object *, int, int);
INLINE void push_string (char *, int);
INLINE void push_mstring (char *);
void push_number (int);
void push_object (struct object *);
struct object *clone_object (char *);
void init_num_args (void);
int restore_object (struct object *, char *);
int m_restore_object (struct object *, struct mapping *);
struct mapping *m_save_object(struct object *);
struct vector *slice_array (struct vector *,int,int);
int query_idle (struct object *);
char *implode_string (struct vector *, char *);
struct object *query_snoop (struct object *);
struct vector *all_inventory (struct object *);
struct vector *deep_inventory (struct object *, int);
struct object *environment (struct svalue *);
struct vector *add_array (struct vector *, struct vector *);
char *get_f_name (int);
void startshutdowngame (int);
void set_notify_fail_message (char *, int);
struct vector *users (void);
void destruct_object (struct object *);
int set_snoop (struct object *, struct object *);
void ed_start (char *, struct closure *);
int command_for_object (char *, struct object *);
int remove_file (char *);
int input_to (struct closure *, int);
int parse (char *, struct svalue *, char *, struct svalue *, int);
struct object *object_present (struct svalue *, struct svalue *);
void call_function (struct object *, int , unsigned int, int);
void store_line_number_info (int, int);
int find_status (struct program *, char *, int);
void free_prog (struct program *);
char *stat_living_objects (void);
#ifdef OPCPROF
void opcdump (void);
#endif
void slow_shut_down (int);
struct vector *allocate_array (int);
void reset_machine (int);
void clear_state (void);
void preload_objects (int);
int random_number (int, int);
int replace_interactive (struct object *ob, struct object *obf, char *);
int get_current_time (void);
char *time_string (int);
char *process_string (char *, int);
#ifdef DEBUG
void update_ref_counts_for_players (void);
void count_ref_from_call_outs (void);
void check_a_lot_ref_counts (struct program *);
#endif
char *read_file (char *file, int, int);
char *read_bytes (char *file, int, size_t);
int write_bytes (char *file, int, char *str);
char *check_valid_path (char *, struct object *, char *, int);
char *get_srccode_position_if_any (void);
void logon (struct object *ob);
struct svalue *apply_master_ob (int, int num_arg);
struct vector *explode_string (char *str, char *del);
char *string_copy (char *);
void remove_object_from_stack (struct object *ob);
#if !defined(sgi) && !defined(_SEQUENT_) && !defined(linux) && !defined(__OpenBSD__)
int getpeername (int, struct sockaddr *, int *);
int  shutdown (int, int);
#endif
void compile_file (void);
#ifdef USE_SWAP
void unlink_swap_file(void);
#ifdef DEALLOCATE_MEMORY_AT_SHUTDOWN
void close_swap_file(void);
#endif
#endif
char *function_exists (char *, struct object *);
void set_inc_list (struct svalue *sv);
int legal_path (char *path);
struct vector *get_dir (char *path);
#if !defined(ultrix) && !defined(M_UNIX) && !defined(sgi) && !defined(NeXT) \
	&& !defined(_SEQUENT_) && !defined(linux) && !defined(__OpenBSD__)
extern int rename (const char *, const char *);
#endif
void get_simul_efun (void);
extern struct object *simul_efun_ob;
struct vector *match_regexp (struct vector *v, char *pattern);

extern char *get_srccode_position(int, struct program *);
#ifdef BINARIES
extern void save_binary(struct program *);
extern int load_binary(FILE *, char *);
#endif

#endif

#if !defined(__OpenBSD__) && !defined(__NetBSD__) && !defined(__FreeBSD__) \
	&& !defined(__bsdi__) && !defined(linux) && !defined(MALLOC_dmalloc)
void *malloc(size_t);
void free(void *);
void *realloc(void *, size_t);
#endif
