/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gicomlan <gicomlan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/28 12:22:04 by gicomlan          #+#    #+#             */
/*   Updated: 2024/09/28 12:53:48 by gicomlan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>	 // write, fork, execve, dup2, close, chdir, STDERR_FILENO
#include <sys/wait.h>   // waitpid pid_t inside types
//#include <sys/types.h> //pid_t
#include <stdlib.h>	 // malloc, free, exit, EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>	 // strcmp
#include <stdbool.h>	// bool, true, false
#include <stdio.h>	  // printf

#define PIPE "|"
#define SEMICOLON ";"
#define CHANGE_DIRECTORY "cd"

typedef struct s_main_variable
{
	char	**argv;
	char	**env;
	int	 argc;
}   t_main_variable;

typedef struct s_pipe
{
	int	 pipe_fd[2];
	int	 previous_pipe_fd;
}   t_pipe;

typedef enum e_command_type
{
	TYPE_NONE,
	TYPE_PIPE,
	TYPE_SEMICOLON,
	TYPE_CD
}   t_command_type;

typedef enum e_pipe_fd
{
	PIPE_INPUT = 0,
	PIPE_OUTPUT = 1
}   t_pipe_fd;

typedef struct s_micro_shell
{
	t_main_variable main_vars;
	t_pipe		  pipes;
	char			**arguments;
	t_command_type  type;
	pid_t		   pid;
	int			 index;
	int			 exit_code;
}   t_micro_shell;

static void ft_close_fd(int fd, t_micro_shell *shell);

// // Fonction de debug pour afficher la structure t_micro_shell
// static void ft_print_micro_shell(t_micro_shell *shell)
// {
//	 int index = 0;

//	 printf("Micro Shell Structure:\n");
//	 printf("PID: %d\n", shell->pid);
//	 printf("Argc: %d\n", shell->main_vars.argc);
//	 printf("Index: %d\n", shell->index);
//	 printf("Exit Code: %d\n", shell->exit_code);
//	 printf("Pipe FD: [%d, %d]\n", shell->pipes.pipe_fd[0], shell->pipes.pipe_fd[1]);
//	 printf("Prev Pipe FD: %d\n", shell->pipes.previous_pipe_fd);
//	 printf("Type: ");
//	 if (shell->type == TYPE_NONE)
//		 printf("TYPE_NONE\n");
//	 else if (shell->type == TYPE_PIPE)
//		 printf("TYPE_PIPE\n");
//	 else if (shell->type == TYPE_SEMICOLON)
//		 printf("TYPE_SEMICOLON\n");
//	 else if (shell->type == TYPE_CD)
//		 printf("TYPE_CD\n");
//	 else
//		 printf("UNKNOWN\n");
//	 printf("Arguments:\n");
//	 if (shell->arguments)
//	 {
//		 while (shell->arguments[index])
//		 {
//			 printf("  arguments[%d]: %s\n", index, shell->arguments[index]);
//			 index++;
//		 }
//	 }
//	 else
//		 printf("  No arguments\n");
// }

static void ft_cleanup(t_micro_shell *shell)
{
	int index_arguments;

	if (shell->arguments)
	{
		index_arguments = 0;
		while (shell->arguments[index_arguments])
		{
			free(shell->arguments[index_arguments]);
			index_arguments++;
		}
		free(shell->arguments);
		shell->arguments = NULL;
	}
	// Close any open file descriptors
	ft_close_fd(shell->pipes.previous_pipe_fd, shell);
	ft_close_fd(shell->pipes.pipe_fd[PIPE_INPUT], shell);
	ft_close_fd(shell->pipes.pipe_fd[PIPE_OUTPUT], shell);
}

size_t	ft_strlen(char *string)
{
	const char	*last_char_in_string;

	if (!string)
		return (0x0);
	last_char_in_string = string;
	while (*last_char_in_string)
		last_char_in_string++;
	return (last_char_in_string - string);
}

static char	*ft_strcpy(char *destination, char *source)
{
	size_t	index;

	index = 0x0;
	while (source[index] != '\0')
	{
		destination[index] = source[index];
		index++;
	}
	destination[index] = '\0';
	return (destination);
}

char	*ft_strdup(char *source)
{
	size_t	length_source;
	char	*duplicate_string;

	duplicate_string = NULL;
	length_source = ft_strlen(source);
	duplicate_string = (char *)malloc(sizeof(char) * (length_source + 0x1));
	if (duplicate_string == NULL)
		return (NULL);
	ft_strcpy(duplicate_string, source);
	return (duplicate_string);
}

static void ft_putstr_fd(char *string, int fd)
{
	if (!string)
		string = "(null)";
	if (fd >= 0)
		write(fd, string, ft_strlen(string));
}

// Affiche un message d'erreur avec un format uniforme
static void ft_error_message(char *prefix, char *message, char *suffix)
{
	if (prefix)
		ft_putstr_fd(prefix, STDERR_FILENO);
	if (message)
		ft_putstr_fd(message, STDERR_FILENO);
	if (suffix)
		ft_putstr_fd(suffix, STDERR_FILENO);
	ft_putstr_fd("\n", STDERR_FILENO);
}

// Gestion d'une erreur fatale
static void ft_exit_with_fatal_error(t_micro_shell *shell)
{
	ft_error_message("error: ", "fatal", NULL);
	ft_cleanup(shell);
	exit(EXIT_FAILURE);
}

// Gestion de l'erreur lors de l'exécution d'une commande
static void ft_cannot_execute_commands(t_micro_shell *shell)
{
	ft_error_message("error: cannot execute ", shell->arguments[0], NULL);
	ft_cleanup(shell);
	exit(EXIT_FAILURE);
}

// Gestion de l'erreur pour la commande cd
static int ft_execute_cd(char **arguments)
{
	//printf("Executing command...\n"); // Debug
	//ft_print_micro_shell(shell);	 // Debug
	if (!arguments[1] || arguments[2])
	{
		ft_error_message("error: ", "cd: bad arguments", NULL);
		return EXIT_FAILURE;
	}
	if (chdir(arguments[1]) == -1)
	{
		ft_error_message("error: cd: cannot change directory to ", arguments[1], NULL);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

// Libère la mémoire allouée pour les arguments
static void ft_free_arguments(char **cmd_arguments, int max_index)
{
	int index;

	index = 0;
	while (index < max_index)
	{
		free(cmd_arguments[index]);
		index++;
	}
	free(cmd_arguments);
}

// Récupère les arguments de la commande
static char **ft_get_command_arguments(t_micro_shell *shell, int start, int end)
{
	char	**cmd_arguments;
	int	 index;
	int	 cmd_length;

	if (start >= end)
		return NULL;
	cmd_length = end - start;
	cmd_arguments = (char **)malloc(sizeof(char *) * (cmd_length + 1));
	if (!cmd_arguments)
		ft_exit_with_fatal_error(shell);
	index = 0;
	while (start < end)
	{
		cmd_arguments[index] = ft_strdup(shell->main_vars.argv[start]);
		if (!cmd_arguments[index])
		{
			ft_error_message("error: ", "Malloc arguments fail", NULL);
			ft_free_arguments(cmd_arguments, index);
			ft_exit_with_fatal_error(shell);
		}
		index++;
		start++;
	}
	cmd_arguments[index] = NULL;
	return cmd_arguments;
}

static void ft_close_fd(int fd, t_micro_shell *shell)
{
	if (fd != -1 && close(fd) == -1)
	{
		//STDERR_FILENO //strerror(errno) //perror("close"); // Debug
		ft_exit_with_fatal_error(shell);
	}
}

// Fonction dédiée pour rediriger les descripteurs de fichiers
static void ft_redirect_fds(t_micro_shell *shell)
{
	if (shell->pipes.previous_pipe_fd != -1)
	{
		if (dup2(shell->pipes.previous_pipe_fd, PIPE_INPUT) == -1)
		{
			//STDERR_FILENO //strerror(errno) //perror("dup2"); // Debug
			ft_exit_with_fatal_error(shell);
		}
	}
	if (shell->type == TYPE_PIPE)
	{
		if (dup2(shell->pipes.pipe_fd[PIPE_OUTPUT], PIPE_OUTPUT) == -1)
		{
			//STDERR_FILENO //strerror(errno) //perror("dup2"); // Debug
			ft_exit_with_fatal_error(shell);
		}
	}
}

// Fonction dédiée pour fermer les descripteurs de fichiers du processus enfant
static void ft_close_child_fds(t_micro_shell *shell)
{
	ft_close_fd(shell->pipes.previous_pipe_fd, shell);
	if (shell->type == TYPE_PIPE)
	{
		ft_close_fd(shell->pipes.pipe_fd[PIPE_INPUT], shell);
		ft_close_fd(shell->pipes.pipe_fd[PIPE_OUTPUT], shell);
	}
}

static void ft_execute_child_process(t_micro_shell *shell)
{
	// Rediriger les descripteurs de fichiers
	//printf("In child process (PID: %d)\n", getpid()); // Debug
	//ft_print_micro_shell(shell); // Debug
	ft_redirect_fds(shell);

	// Fermer les descripteurs inutilisés pour éviter les fuites
	ft_close_child_fds(shell);

	// Exécuter la commande avec execve
	if (execve(shell->arguments[0], shell->arguments, shell->main_vars.env) == -1)
	{
		//STDERR_FILENO //strerror(errno) //perror("execve"); // Debug
		ft_cannot_execute_commands(shell);
	}
}

// Fonction dédiée pour gérer le code de retour de l'enfant
static void ft_handle_child_status(t_micro_shell *shell, int status)
{
	if (WIFEXITED(status))
	{
		//printf("Child exited with status %d\n", WEXITSTATUS(status)); // Debug
		shell->exit_code = WEXITSTATUS(status);
	}
	else
	{
		// printf("Child terminated abnormally\n"); // Debug
		shell->exit_code = EXIT_FAILURE;
	}
}

static void ft_execute_parent_process(t_micro_shell *shell)
{
	int status;

	if (waitpid(shell->pid, &status, 0) == -1)
	{
		//STDERR_FILENO //strerror(errno) //perror("waitpid"); // Debug
		ft_exit_with_fatal_error(shell);
	}
	// Utilisation de la fonction pour gérer le statut du processus enfant
	ft_handle_child_status(shell, status);

	ft_close_fd(shell->pipes.previous_pipe_fd, shell);
	if (shell->type == TYPE_PIPE)
	{
		ft_close_fd(shell->pipes.pipe_fd[PIPE_OUTPUT], shell);
		shell->pipes.previous_pipe_fd = shell->pipes.pipe_fd[PIPE_INPUT];
	}
	else
	{
		shell->pipes.previous_pipe_fd = -1;
	}
}



// Prépare le pipe si nécessaire
static void ft_prepare_pipe(t_micro_shell *shell)
{
	if (shell->type == TYPE_PIPE)
	{
		if (pipe(shell->pipes.pipe_fd) == -1)
		{
			ft_exit_with_fatal_error(shell);
		}
	}
}

// Fork le processus et retourne le PID
static pid_t ft_fork_process(t_micro_shell *shell)
{
	pid_t pid;

	pid = fork();
	if (pid == -1)
		ft_exit_with_fatal_error(shell);
	return pid;
}

// Exécute la commande externe
static int ft_execute_external_command(t_micro_shell *shell)
{
	ft_prepare_pipe(shell);
	shell->pid = ft_fork_process(shell);
	if (shell->pid == 0)
		ft_execute_child_process(shell);
	else
		ft_execute_parent_process(shell);
	return EXIT_SUCCESS;
}

static void ft_skip_semicolons(t_micro_shell *shell)
{
	while (shell->index < shell->main_vars.argc &&
		   strcmp(shell->main_vars.argv[shell->index], SEMICOLON) == 0)
		shell->index++;
}

static int ft_get_command_end(t_micro_shell *shell)
{
	int index;

	index = shell->index;
	while (index < shell->main_vars.argc &&
		   strcmp(shell->main_vars.argv[index], PIPE) != 0 &&
		   strcmp(shell->main_vars.argv[index], SEMICOLON) != 0)
		index++;
	return (index);
}

static bool ft_is_pipe(t_micro_shell *shell)
{
	if (shell->index < shell->main_vars.argc &&
		strcmp(shell->main_vars.argv[shell->index], PIPE) == 0)
		return (true);
	return (false);
}

static bool ft_is_semicolon(t_micro_shell *shell)
{
	if (shell->index < shell->main_vars.argc &&
		strcmp(shell->main_vars.argv[shell->index], SEMICOLON) == 0)
		return (true);
	return (false);
}

static void ft_check_if_cd(t_micro_shell *shell)
{
	if (shell->arguments[0] && strcmp(shell->arguments[0], CHANGE_DIRECTORY) == 0)
		shell->type = TYPE_CD;
}

static void ft_parse_arguments(t_micro_shell *shell)
{
	int start;
	int end;

	ft_skip_semicolons(shell);
	if (shell->index >= shell->main_vars.argc)
		return;
	start = shell->index;
	end = ft_get_command_end(shell);
	shell->arguments = ft_get_command_arguments(shell, start, end);
	shell->index = end;
	if (ft_is_pipe(shell))
	{
		shell->type = TYPE_PIPE;
		shell->index++;
	}
	else if (ft_is_semicolon(shell))
	{
		shell->type = TYPE_SEMICOLON;
		shell->index++;
	}
	else
		shell->type = TYPE_NONE;
	ft_check_if_cd(shell);
	//ft_print_micro_shell(shell); // Debug
}


static int ft_execute_command(t_micro_shell *shell)
{
	int exit_code;

	exit_code = EXIT_FAILURE;
	//printf("Executing command...\n"); // Debug
	//ft_print_micro_shell(shell);	 // Debug
	if (shell->type == TYPE_CD)
		exit_code = ft_execute_cd(shell->arguments);
	else
		exit_code = ft_execute_external_command(shell);
	return (exit_code);
}

static void ft_initialize_micro_shell(t_micro_shell *shell, int argc, char **argv, char **envp)
{
	shell->main_vars.argv = argv;
	shell->main_vars.env = envp;
	shell->main_vars.argc = argc;
	shell->index = 1;
	shell->pipes.pipe_fd[PIPE_INPUT] = -1;
	shell->pipes.pipe_fd[PIPE_OUTPUT] = -1;
	shell->pipes.previous_pipe_fd = -1;
	shell->exit_code = EXIT_SUCCESS;
	shell->arguments = NULL;
	shell->type = TYPE_NONE;
}

/*
clang -g3 -Wall -Werror -Wextra -o microshell microshell.c && valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes --show-reachable=yes --track-origins=yes ./microshell
*/
int main(int argc, char **argv, char **envp)
{
	t_micro_shell shell;

	//printf("Starting microshell...\n"); // Debug
	ft_initialize_micro_shell(&shell, argc, argv, envp);
	while (shell.index < shell.main_vars.argc)
	{
		shell.arguments = NULL;
		shell.type = TYPE_NONE;
		ft_parse_arguments(&shell);
		if (shell.arguments)
		{
			shell.exit_code = ft_execute_command(&shell);
			free(shell.arguments);
			shell.arguments = NULL;
		}
	}
	//printf("Microshell exited with code %d\n", shell.exit_code); // Debug
	return (shell.exit_code);
}
