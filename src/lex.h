#define MLEN 8192
#define NSIZE 256

struct lpc_predef_s {
    char *flag;
    struct lpc_predef_s *next;
};

extern struct lpc_predef_s *lpc_predefs;
void start_new_file(FILE *f);
void end_new_file(void);
#ifdef DEALLOCATE_MEMORY_AT_SHUTDOWN
void free_inc_list(void);
#endif
