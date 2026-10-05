#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "builtin.h"

int is_builtin(const command_t *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
        return 0;

    if (strcmp(cmd->argv[0], "cd") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "pwd") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "echo") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "exit") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "fork") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "forkexec") == 0)
        return 1;

    return 0;
}

int execute_builtin(command_t *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
        return 0;

    /*
     * =========================
     * cd
     * =========================
     */
    if (strcmp(cmd->argv[0], "cd") == 0)
    {
        const char *path;

        if (cmd->argc > 1)
            path = cmd->argv[1];
        else
            path = getenv("HOME");

        if (path == NULL)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }

        if (chdir(path) != 0)
        {
            perror("cd");
            return 1;
        }

        return 0;
    }

    /*
     * =========================
     * pwd
     * =========================
     */
    if (strcmp(cmd->argv[0], "pwd") == 0)
    {
        char cwd[4096];

        if (getcwd(cwd, sizeof(cwd)) == NULL)
        {
            perror("pwd");
            return 1;
        }

        printf("%s\n", cwd);

        return 0;
    }

    /*
     * =========================
     * echo
     * =========================
     */
    if (strcmp(cmd->argv[0], "echo") == 0)
    {
        for (int i = 1; i < cmd->argc; i++)
        {
            printf("%s", cmd->argv[i]);

            if (i < cmd->argc - 1)
                printf(" ");
        }

        printf("\n");

        return 0;
    }

    /*
     * =========================
     * exit
     * =========================
     */
    if (strcmp(cmd->argv[0], "exit") == 0)
    {
        exit(0);
    }

    /*
     * =========================
     * FORK
     *
     * Demonstrates:
     * parent process
     * child process
     * fork()
     * waitpid()
     * =========================
     */
    if (strcmp(cmd->argv[0], "fork") == 0)
    {
        pid_t pid;

        printf("Creating child process...\n");

        pid = fork();

        /*
         * fork failed
         */
        if (pid < 0)
        {
            perror("fork");
            return 1;
        }

        /*
         * Child process
         */
        if (pid == 0)
        {
            printf("Child process created\n");
            printf("Child PID  : %d\n", getpid());
            printf("Parent PID : %d\n", getppid());

            _exit(0);
        }

        /*
         * Parent process
         */
        printf("Parent process\n");
        printf("Parent PID : %d\n", getpid());
        printf("Child PID  : %d\n", pid);

        /*
         * Parent waits for child.
         */
        if (waitpid(pid, NULL, 0) < 0)
        {
            perror("waitpid");
            return 1;
        }

        printf("Child process finished\n");

        return 0;
    }

    /*
     * =========================
     * FORKEXEC
     *
     * Usage:
     *
     * forkexec ls
     * forkexec pwd
     * forkexec echo hello
     *
     * Demonstrates:
     * fork()
     * execvp()
     * waitpid()
     * =========================
     */
    if (strcmp(cmd->argv[0], "forkexec") == 0)
    {
        pid_t pid;

        if (cmd->argc < 2)
        {
            printf("Usage: forkexec <command> [arguments...]\n");
            return 1;
        }

        printf("Creating child process...\n");

        pid = fork();

        /*
         * fork failed
         */
        if (pid < 0)
        {
            perror("fork");
            return 1;
        }

        /*
         * Child process
         */
        if (pid == 0)
        {
            printf("Child process created\n");
            printf("Child PID  : %d\n", getpid());
            printf("Executing  : %s\n", cmd->argv[1]);

            /*
             * Execute the requested command.
             *
             * Example:
             * forkexec ls
             *
             * becomes:
             *
             * execvp("ls", {"ls", NULL});
             */
            execvp(cmd->argv[1], &cmd->argv[1]);

            /*
             * execvp only reaches here
             * when execution fails.
             */
            perror("execvp");

            _exit(127);
        }

        /*
         * Parent process
         */
        printf("Parent process\n");
        printf("Parent PID : %d\n", getpid());
        printf("Child PID  : %d\n", pid);

        /*
         * Wait for child execution.
         */
        if (waitpid(pid, NULL, 0) < 0)
        {
            perror("waitpid");
            return 1;
        }

        printf("Child execution finished\n");

        return 0;
    }

    return 1;
}
