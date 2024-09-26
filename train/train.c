/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   train.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gicomlan <gicomlan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/26 16:57:26 by gicomlan          #+#    #+#             */
/*   Updated: 2024/09/26 21:19:09 by gicomlan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdbool.h>    // bool
#include <unistd.h>     // write, fork, execve, dup2, close, chdir, STDERR_FILENO
#include <sys/wait.h>   // waitpid
#include <stdlib.h>     // exit, EXIT_FAILURE
#include <string.h>     // strcmp

static void	ft_putstr_fd(char *str, int fd)
{
	while (*str)
		write(fd, str++, 1);
}

void	ft_print_error(char *msg)
{
	ft_putstr_fd("error: ", STDERR_FILENO);
	ft_putstr_fd(msg, STDERR_FILENO);
}

void	ft_print_fatal_error(void)
{
	ft_print_error("fatal\n");
	exit(EXIT_FAILURE);
}

int ft_execute_cd(char **args)
{
    if (!args[1] || args[2])
    {
        ft_print_error("cd: bad arguments\n");
        return EXIT_FAILURE;
    }
    if (chdir(args[1]) == -1)
    {
        ft_print_error("cd: cannot change directory to ");
        ft_putstr_fd(args[1], STDERR_FILENO);
        ft_putstr_fd("\n", STDERR_FILENO);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int has_pipe(char **argv)
{
    return (argv && *argv && strcmp(*argv, "|") == 0);
}

// Fonction pour analyser la commande et déterminer si un pipe est présent
int ft_parse_command(char **argv, int *is_pipe)
{
    int len;

	len = 0;
    while (argv[len] && strcmp(argv[len], "|") != 0 && strcmp(argv[len], ";") != 0)
        len++;
    *is_pipe = has_pipe(&argv[len]);
    if (argv[len])
        argv[len] = NULL;
    return len;
}

// Fonction pour exécuter le processus enfant
void ft_child_process(char **args, char **envp, int input_fd, int *fd_pipe, int is_pipe)
{
    if (input_fd != 0)
    {
        if (dup2(input_fd, STDIN_FILENO) == -1)
            ft_print_fatal_error();
        close(input_fd);
    }
    if (is_pipe)
    {
        if (dup2(fd_pipe[1], STDOUT_FILENO) == -1)
            ft_print_fatal_error();
        close(fd_pipe[0]);
        close(fd_pipe[1]);
    }
    execve(args[0], args, envp);
    ft_print_error("cannot execute ");
    ft_putstr_fd(args[0], STDERR_FILENO);
    ft_putstr_fd("\n", STDERR_FILENO);
    exit(EXIT_FAILURE);
}

// Fonction pour exécuter le processus parent
void ft_parent_process(pid_t pid, int input_fd, int *fd_pipe, int is_pipe)
{
    int status;

    if (waitpid(pid, &status, 0) == -1)
        ft_print_fatal_error();
    if (input_fd != 0)
        close(input_fd);
    if (is_pipe)
        close(fd_pipe[1]);
}

// Fonction pour exécuter les commandes avec gestion des pipes
int ft_run_command(char **args, char **envp, int input_fd, int is_pipe)
{
    pid_t pid;
    int fd_pipe[2];

    if (is_pipe && pipe(fd_pipe) == -1)
        ft_print_fatal_error();

    pid = fork();
    if (pid == -1)
        ft_print_fatal_error();
    else if (pid == 0)
        ft_child_process(args, envp, input_fd, fd_pipe, is_pipe);
    else
        ft_parent_process(pid, input_fd, fd_pipe, is_pipe);

    if (is_pipe)
        return fd_pipe[0]; // Retourner le descripteur de lecture pour le prochain input_fd
    return 0;
}

int main(int argc, char **argv, char **envp)
{
    int input_fd;
    int exit_code;
    int is_pipe;
    int cmd_len;

    (void)argc;
    argv++;
    while (*argv)
    {
        if (strcmp(*argv, ";") == 0)
        {
            argv++;
            continue;
        }
        cmd_len = ft_parse_command(argv, &is_pipe);
        if (argv[0] && strcmp(argv[0], "cd") == 0)
            exit_code = ft_execute_cd(argv);
        else
            input_fd = ft_run_command(argv, envp, input_fd, is_pipe);
        argv += cmd_len;
        if (*argv)
            argv++; // Passer le séparateur '|' ou ';'
    }
    return exit_code;
}

// // Fonction pour déterminer si un pipe est nécessaire
// int has_pipe(char **argv)
// {
//     return (argv && *argv && strcmp(*argv, "|") == 0);
// }

// // Fonction pour configurer les pipes dans le processus enfant
// void setup_child_pipe(int file_descriptor[2])
// {
//     if (dup2(file_descriptor[1], STDOUT_FILENO) == -1)
//         ft_print_fatal_error();
//     if (close(file_descriptor[0]) == -1 || close(file_descriptor[1]) == -1)
//         ft_print_fatal_error();
// }

// // Fonction pour configurer les pipes dans le processus parent
// void setup_parent_pipe(int file_descriptor[2])
// {
//     if (dup2(file_descriptor[0], STDIN_FILENO) == -1)
//         ft_print_fatal_error();
//     if (close(file_descriptor[0]) == -1 || close(file_descriptor[1]) == -1)
//         ft_print_fatal_error();
// }

// // Fonction pour exécuter une commande dans le processus enfant
// void execute_child(char **commands, char **envp)
// {
//     execve(commands[0], commands, envp);
//     // Si execve échoue
//     ft_print_error("cannot execute ");
//     ft_putstr_fd(commands[0], STDERR_FILENO);
//     ft_putstr_fd("\n", STDERR_FILENO);
//     exit(EXIT_FAILURE);
// }

// // Fonction pour déterminer si un pipe est nécessaire
// bool has_pipe(char **commands, int commands_index)
// {
//     if (commands[commands_index] && strcmp(commands[commands_index], "|") == 0)
//         return true;
//     return false;
// }

// // Fonction pour exécuter le processus enfant
// void ft_child_process(bool has_pipe_flag, int file_descriptor[2], int input_fd, char **commands, char **envp)
// {
//     // Rediriger l'entrée standard si nécessaire
//     if (input_fd != 0)
//     {
//         if (dup2(input_fd, STDIN_FILENO) == -1)
//             ft_print_fatal_error();
//         close(input_fd);
//     }
//     // Configurer le pipe si nécessaire
//     if (has_pipe_flag)
//     {
//         if (dup2(file_descriptor[1], STDOUT_FILENO) == -1)
//             ft_print_fatal_error();
//         close(file_descriptor[0]);
//         close(file_descriptor[1]);
//     }
//     // Exécuter la commande
//     execve(commands[0], commands, envp);
//     // Si execve échoue
//     ft_print_error("cannot execute ");
//     ft_putstr_fd(commands[0], STDERR_FILENO);
//     ft_putstr_fd("\n", STDERR_FILENO);
//     exit(EXIT_FAILURE);
// }

// // Fonction pour exécuter le processus parent
// void ft_parent_process(bool has_pipe_flag, int file_descriptor[2], pid_t pid, int input_fd, int *exit_code)
// {
//     if (waitpid(pid, exit_code, 0) == -1)
//         ft_print_fatal_error();
//     if (input_fd != 0)
//         close(input_fd);
//     if (has_pipe_flag)
//         close(file_descriptor[1]); // Fermer le côté écriture du pipe
// }

// // Fonction pour gérer l'exécution des commandes avec gestion des pipes
// int ft_execute_commands(char **commands, int commands_index, char **envp, int input_fd)
// {
//     bool has_pipe_flag;
//     int file_descriptor[2];
//     pid_t pid;
//     int exit_code = 0;
//     has_pipe_flag = has_pipe(commands, commands_index);

//     if (!has_pipe_flag && strcmp(commands[0], "cd") == 0)
//         return ft_execute_cd(commands, commands_index);
//     if (has_pipe_flag && pipe(file_descriptor) == -1)
//         ft_print_fatal_error();
//     pid = fork();
//     if (pid == -1)
//         ft_print_fatal_error();
//     else if (pid == 0)
//         ft_child_process(has_pipe_flag, file_descriptor, input_fd, commands, envp);
//     else
//         ft_parent_process(has_pipe_flag, file_descriptor, pid, input_fd, &exit_code);
//     if (WIFEXITED(exit_code))
//         exit_code = WEXITSTATUS(exit_code);
//     else
//         exit_code = EXIT_FAILURE;
//     if (has_pipe_flag)
//         return file_descriptor[0]; // Retourner le descripteur de lecture pour le prochain input_fd
//     return exit_code;
// }


// int	main(int argc, char **argv, char **envp)
// {
// 	int		index;
// 	bool	exit_code;
// 	char	**commands = &argv[index];
// 	int		commands_length = 0x0;

// 	index = 0x1;
// 	exit_code = false;
// 	while (index < argc)
// 	{
// 		while ((((index + commands_length) < argc) && \
// 			strcmp(argv[index + commands_length], "|") != 0x0 && \
// 				strcmp(argv[index + commands_length], ";") != 0x0))
// 				commands_length++;
// 		if (commands_length > 0x0)
// 			exit_code = ft_execute_commands(commands, commands_length, envp);
// 		index += commands_length;

// 		if ((index < argc) && strcmp(argv[index], "|") == 0x0)
// 			index++;
// 		else if ((index < argc) && strcmp(argv[index], ";") == 0x0)
// 			index++;
// 	}
// 	return (exit_code);
// }
