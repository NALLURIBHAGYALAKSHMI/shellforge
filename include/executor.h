#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

/* Execute one command */
int execute_command(command_t *cmd);

/* Execute multiple commands connected by pipes */
int execute_pipeline(pipeline_t *pipeline);

#endif
