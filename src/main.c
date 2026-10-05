#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "token.h"
#include "history.h"
#include "lexer.h"
#include "parser.h"
#include "expand.h"
#include "builtin.h"
#include "executor.h"

int main(void)
{
    char *line;

    while (1)
    {
        line = readline("shellforge$ ");

        if (line == NULL)
        {
            printf("\n");
            break;
        }

        if (strlen(line) == 0)
        {
            free(line);
            continue;
        }

        if (strcmp(line, "exit") == 0)
        {
            free(line);
            break;
        }

        if (strcmp(line, "history") == 0)
        {
            print_history();
            free(line);
            continue;
        }

        add_history(line);

        token_list_t list;
        token_list_init(&list);

        tokenize(line, &list);

        pipeline_t pipeline;

        if (lexer_validate(&list))
        {
            if (parser(&list, &pipeline))
            {
                expand_variables(&pipeline);
                execute_pipeline(&pipeline);
            }
        }

        free(line);
    }

    return 0;
}
