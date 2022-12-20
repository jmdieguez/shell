#ifndef BUILTIN_H
#define BUILTIN_H

#include "defs.h"

#define PERFORMED 1
#define UNPERFORMED 0

#define HISTORY_MAX_SIZE 256

extern char prompt[PRMTLEN];

void update_prompt(char *directory, char *curdir);
int cd(char *cmd);
int exit_shell(char *cmd);
int print_history(char *cmd);
int pwd(char *cmd);

// History
void add_command_to_history(const char *cmd);
void load_history(void);
void write_history(void);
int print_history(char *cmd);

// Non-canonical mode : sh.c
void reset_input_mode(void);
void set_input_mode(void);

#endif  // BUILTIN_H
