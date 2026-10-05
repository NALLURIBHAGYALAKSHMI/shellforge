#include <stdio.h>

#include "lexer.h"
#include "token.h"


/*
 * Validate the token list produced by tokenize().
 *
 * Checks:
 *   - commands
 *   - pipes |
 *   - input redirection <
 *   - output redirection >
 *   - append redirection >>
 *   - background &
 *   - missing filenames
 *   - invalid pipe positions
 */
int lexer_validate(const token_list_t *list)
{
    int expecting_command = 1;
    int expecting_filename = 0;
    int command_found = 0;
    int background_found = 0;

    if (list == NULL || list->count == 0)
    {
        fprintf(stderr, "LEXER ERROR: Empty command\n");
        return 0;
    }

    for (int i = 0; i < list->count; i++)
    {
        /*
         * list is const, so token must also be const.
         */
        const token_t *token = &list->tokens[i];

        switch (token->type)
        {
            /*
             * =========================
             * WORD
             * =========================
             */
            case TOKEN_WORD:

                /*
                 * A WORD after <, > or >>
                 * is the filename.
                 */
                if (expecting_filename)
                {
                    expecting_filename = 0;
                }
                else
                {
                    command_found = 1;
                    expecting_command = 0;
                }

                break;


            /*
             * =========================
             * PIPE |
             * =========================
             */
            case TOKEN_PIPE:

                /*
                 * Pipe cannot be:
                 *
                 * | command
                 * command |
                 * command | |
                 */
                if (!command_found ||
                    expecting_command ||
                    expecting_filename ||
                    background_found)
                {
                    fprintf(stderr,
                            "LEXER ERROR: Invalid pipe position\n");

                    return 0;
                }

                /*
                 * After a pipe we expect
                 * another command.
                 */
                expecting_command = 1;
                command_found = 0;
                background_found = 0;

                break;


            /*
             * =========================
             * INPUT <
             * =========================
             */
            case TOKEN_INPUT:

                if (!command_found)
                {
                    fprintf(stderr,
                            "LEXER ERROR: Input redirection before command\n");

                    return 0;
                }

                if (expecting_filename)
                {
                    fprintf(stderr,
                            "LEXER ERROR: Missing filename before <\n");

                    return 0;
                }

                expecting_filename = 1;

                break;


            /*
             * =========================
             * OUTPUT >
             * =========================
             */
            case TOKEN_OUTPUT:

                if (!command_found)
                {
                    fprintf(stderr,
                            "LEXER ERROR: Output redirection before command\n");

                    return 0;
                }

                if (expecting_filename)
                {
                    fprintf(stderr,
                            "LEXER ERROR: Missing filename before >\n");

                    return 0;
                }

                expecting_filename = 1;

                break;


            /*
             * =========================
             * APPEND >>
             * =========================
             */
            case TOKEN_APPEND:

                if (!command_found)
                {
                    fprintf(stderr,
                            "LEXER ERROR: Append redirection before command\n");

                    return 0;
                }

                if (expecting_filename)
                {
                    fprintf(stderr,
                            "LEXER ERROR: Missing filename before >>\n");

                    return 0;
                }

                expecting_filename = 1;

                break;


            /*
             * =========================
             * BACKGROUND &
             * =========================
             */
            case TOKEN_BACKGROUND:

                /*
                 * & must come after a command.
                 */
                if (!command_found ||
                    expecting_command ||
                    expecting_filename ||
                    background_found)
                {
                    fprintf(stderr,
                            "LEXER ERROR: Invalid background symbol\n");

                    return 0;
                }

                background_found = 1;

                break;


            /*
             * =========================
             * END
             * =========================
             */
            case TOKEN_END:

                /*
                 * Example:
                 *
                 * echo hello >
                 */
                if (expecting_filename)
                {
                    fprintf(stderr,
                            "LEXER ERROR: Missing filename after redirection\n");

                    return 0;
                }

                /*
                 * Examples:
                 *
                 * |
                 * echo |
                 * echo hello |
                 */
                if (!command_found || expecting_command)
                {
                    fprintf(stderr,
                            "LEXER ERROR: Missing command\n");

                    return 0;
                }

                /*
                 * Command is valid.
                 */
                return 1;


            /*
             * =========================
             * ERROR TOKEN
             * =========================
             */
            case TOKEN_ERROR:

                fprintf(stderr,
                        "LEXER ERROR: Invalid token '%s'\n",
                        token->text);

                return 0;


            /*
             * =========================
             * UNKNOWN TOKEN
             * =========================
             */
            default:

                fprintf(stderr,
                        "LEXER ERROR: Unknown token\n");

                return 0;
        }
    }

    /*
     * Safety checks.
     */
    if (expecting_filename)
    {
        fprintf(stderr,
                "LEXER ERROR: Missing filename after redirection\n");

        return 0;
    }

    if (expecting_command || !command_found)
    {
        fprintf(stderr,
                "LEXER ERROR: Incomplete command\n");

        return 0;
    }

    return 1;
}
