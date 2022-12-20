#ifndef EXEC_H
#define EXEC_H

#include "defs.h"
#include "types.h"
#include "utils.h"
#include "freecmd.h"

extern struct cmd *parsed_pipe;
extern int status;
void fork_exec(struct execcmd *cmd);
void exec_cmd(struct cmd *c);

#endif  // EXEC_H
