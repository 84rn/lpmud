void write_socket(char *, struct object *);
void add_message(char *fmt, ...);
#ifdef DEALLOCATE_MEMORY_AT_SHUTDOWN
void clear_ip_table(void);
#endif
