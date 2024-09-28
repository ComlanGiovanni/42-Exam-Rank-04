/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   short_not_micro_shell.c                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gicomlan <gicomlan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/28 12:17:14 by gicomlan          #+#    #+#             */
/*   Updated: 2024/09/28 12:18:22 by gicomlan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define PIPE "|"
#define SEMICOLON ";"
#define CD "cd"

void    ft_putstr_fd(char *s, int fd)
{
    if (s)
        write(fd, s, strlen(s));
}

void    error_fatal(void)
{
    ft_putstr_fd("error: fatal\n", 2);
    exit(1);
}

void    error_cd(char *msg, char *arg)
{
    ft_putstr_fd("error: cd: ", 2);
    ft_putstr_fd(msg, 2);
    if (arg)
    {
        ft_putstr_fd(" ", 2);
        ft_putstr_fd(arg, 2);
    }
    ft_putstr_fd("\n", 2);
}

void    error_exec(char *cmd)
{
    ft_putstr_fd("error: cannot execute ", 2);
    ft_putstr_fd(cmd, 2);
    ft_putstr_fd("\n", 2);
}

int ft_execute_cd(char **args)
{
    if (!args[1] || args[2])
    {
        error_cd("bad arguments", NULL);
        return (1);
    }
    if (chdir(args[1]) == -1)
    {
        error_cd("cannot change directory to", args[1]);
        return (1);
    }
    return (0);
}

int main(int argc, char **argv, char **envp)
{
    int i = 1, pid, status, fd[2], tmp_fd;
    char **cmd;
    int   cmd_start;
    char *separator;

    tmp_fd = dup(0);
    if (tmp_fd == -1)
        error_fatal();
    while (i < argc)
    {
        cmd_start = i;
        while (i < argc && strcmp(argv[i], PIPE) && strcmp(argv[i], SEMICOLON))
            i++;
        separator = NULL;
        if (i < argc)
            separator = argv[i];
        argv[i] = NULL;

        cmd = &argv[cmd_start];

        if (cmd[0])
        {
            if (!strcmp(cmd[0], CD))
            {
                if (ft_execute_cd(cmd))
                    ;
            }
            else
            {
                if (separator && !strcmp(separator, PIPE))
                {
                    if (pipe(fd) == -1)
                        error_fatal();
                }
                pid = fork();
                if (pid == -1)
                    error_fatal();
                if (pid == 0)
                {
                    if (dup2(tmp_fd, 0) == -1)
                        error_fatal();
                    if (separator && !strcmp(separator, PIPE))
                    {
                        if (dup2(fd[1], 1) == -1)
                            error_fatal();
                        close(fd[0]);
                        close(fd[1]);
                    }
                    close(tmp_fd);
                    if (execve(cmd[0], cmd, envp) == -1)
                    {
                        error_exec(cmd[0]);
                        exit(1);
                    }
                }
                else
                {
                    waitpid(pid, &status, 0);
                    if (tmp_fd != 0)
                        close(tmp_fd);
                    if (separator && !strcmp(separator, PIPE))
                    {
                        tmp_fd = fd[0];
                        close(fd[1]);
                    }
                    else
                    {
                        tmp_fd = dup(0);
                        if (tmp_fd == -1)
                            error_fatal();
                    }
                }
            }
        }
        i++;
    }
    if (tmp_fd != 0)
        close(tmp_fd);
    return (0);
}
