#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "executor.h"
#include "builtin.h"

static int setup_redirection(command_t *cmd)
{
    if (cmd->input[0] != '\0')
    {
        int fd = open(cmd->input, O_RDONLY);

        if (fd < 0)
        {
            perror(cmd->input);
            return -1;
        }

        if (dup2(fd, STDIN_FILENO) < 0)
        {
            perror("dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    if (cmd->output[0] != '\0')
    {
        int flags = O_WRONLY | O_CREAT;

        if (cmd->append)
            flags |= O_APPEND;
        else
            flags |= O_TRUNC;

        int fd = open(cmd->output, flags, 0644);

        if (fd < 0)
        {
            perror(cmd->output);
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    return 0;
}

int execute_command(command_t *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
        return 0;

    /*
     * Built-in commands run directly in the shell.
     */
    if (is_builtin(cmd))
        return execute_builtin(cmd);

    /*
     * CREATE CHILD PROCESS
     */
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    /*
     * CHILD PROCESS
     */
    if (pid == 0)
    {
        if (setup_redirection(cmd) < 0)
            _exit(1);

        /*
         * Replace child process with requested program.
         */
        execvp(cmd->argv[0], cmd->argv);

        /*
         * execvp only returns when there is an error.
         */
        fprintf(stderr, "%s: %s\n",
                cmd->argv[0],
                strerror(errno));

        _exit(127);
    }

    /*
     * PARENT PROCESS
     */
    if (cmd->background)
    {
        printf("[background pid %d]\n", pid);
        return 0;
    }

    /*
     * Parent waits for child.
     */
    int status;

    if (waitpid(pid, &status, 0) < 0)
    {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);

    return 1;
}

int execute_pipeline(pipeline_t *pipeline)
{
    if (pipeline == NULL || pipeline->command_count == 0)
        return 0;

    if (pipeline->command_count == 1)
        return execute_command(&pipeline->commands[0]);

    int previous_read = -1;

    pid_t pids[MAX_COMMANDS];
    int child_count = 0;

    for (int i = 0; i < pipeline->command_count; i++)
    {
        command_t *cmd = &pipeline->commands[i];

        int pipefd[2] = {-1, -1};

        if (i < pipeline->command_count - 1)
        {
            if (pipe(pipefd) < 0)
            {
                perror("pipe");
                return 1;
            }
        }

        /*
         * CREATE CHILD PROCESS FOR PIPE COMMAND
         */
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("fork");
            return 1;
        }

        /*
         * CHILD
         */
        if (pid == 0)
        {
            if (previous_read != -1)
            {
                if (dup2(previous_read, STDIN_FILENO) < 0)
                {
                    perror("dup2");
                    _exit(1);
                }
            }

            if (i < pipeline->command_count - 1)
            {
                if (dup2(pipefd[1], STDOUT_FILENO) < 0)
                {
                    perror("dup2");
                    _exit(1);
                }
            }

            if (previous_read != -1)
                close(previous_read);

            if (pipefd[0] != -1)
                close(pipefd[0]);

            if (pipefd[1] != -1)
                close(pipefd[1]);

            if (setup_redirection(cmd) < 0)
                _exit(1);

            if (is_builtin(cmd))
                _exit(execute_builtin(cmd));

            execvp(cmd->argv[0], cmd->argv);

            fprintf(stderr, "%s: %s\n",
                    cmd->argv[0],
                    strerror(errno));

            _exit(127);
        }

        /*
         * PARENT
         */
        pids[child_count++] = pid;

        if (previous_read != -1)
            close(previous_read);

        if (pipefd[1] != -1)
            close(pipefd[1]);

        previous_read = pipefd[0];
    }

    if (previous_read != -1)
        close(previous_read);

    /*
     * WAIT FOR ALL CHILDREN
     */
    int last_status = 0;

    for (int i = 0; i < child_count; i++)
    {
        int status;

        if (waitpid(pids[i], &status, 0) < 0)
        {
            perror("waitpid");
            continue;
        }

        if (i == child_count - 1)
        {
            if (WIFEXITED(status))
                last_status = WEXITSTATUS(status);

            else if (WIFSIGNALED(status))
                last_status = 128 + WTERMSIG(status);
        }
    }

    return last_status;
}
