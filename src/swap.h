#ifdef USE_SWAP
void init_swap(void);
void load_ob_from_swap(struct object *);
void load_prog_from_swap(struct program *prog);
void load_lineno_from_swap(struct program *prog);
void swap_lineno(struct program *prog);
void remove_ob_from_swap(struct object *ob);
void remove_prog_from_swap(struct program *prog);
#endif
