/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gicomlan <gicomlan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/27 13:24:09 by gicomlan          #+#    #+#             */
/*   Updated: 2024/09/27 19:16:09 by gicomlan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>	 // write, fork, execve, dup2, close, chdir, STDERR_FILENO
#include <sys/wait.h>   // waitpid
#include <stdlib.h>	 // malloc, free, exit, EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>	 // strcmp
#include <stdbool.h>	// bool, true, false

#define PIPE "|"
#define SEMICOLON ";"
#define CD "cd"

typedef enum e_command_type
{
	TYPE_NONE,
	TYPE_PIPE,
	TYPE_SEMICOLON,
	TYPE_CD
}	t_command_type;

typedef struct s_command
{
	char			**arguments;
	t_command_type	type;
}	t_command;

typedef struct s_micro_shell
{
	pid_t		pid;
	char		**argv;
	char		**envp;
	int			argc;
	int			index;
	int			exit_code;
	int			pipe_fd[2];
	int			prev_pipe_fd;
	t_command	command;
}	t_micro_shell;

static	size_t	ft_strlen(char *string)
{
	const char	*last_char_string_pointer;

	last_char_string_pointer = string;
	if (!string)
		return (0x0);
	while (*last_char_string_pointer != '\0')
		last_char_string_pointer++;
	return (last_char_string_pointer - string);
}

static void	ft_putstr_fd(char *string, int fd)
{
	if (!string)
		string = "(null)";
	if (fd >= 0)
		write(fd, string, ft_strlen(string));
}

static void	ft_print_error(char *message)
{
	ft_putstr_fd("error: ", STDERR_FILENO);
	ft_putstr_fd(message, STDERR_FILENO);
}

static void	ft_fatal_error(void)
{
	ft_print_error("fatal\n");
	exit(EXIT_FAILURE);
}

static int ft_execute_cd(char **command)
{
	char *path_to_change;

	path_to_change = NULL;
	if (!command[1] || command[2])
	{
		ft_print_error("cd: bad arguments\n");
		return (EXIT_FAILURE);
	}
	path_to_change = command[1];
	if (chdir(path_to_change) == -1)
	{
		ft_print_error("cd: cannot change directory to ");
		ft_putstr_fd(path_to_change, STDERR_FILENO);
		ft_putstr_fd("\n", STDERR_FILENO);
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

static char	**ft_get_command_arguments(t_micro_shell *shell, \
	int start, int end)
{
	char	**cmd_arguments;
	int		index;

	cmd_arguments = (char **)malloc(sizeof(char *) * (end - start + 1));
	if (!cmd_arguments)
		ft_fatal_error();
	index = 0x0;
	while (start < end)
	{
		cmd_arguments[index] = shell->argv[start];
		index++;
		start++;
	}
	cmd_arguments[index] = NULL;
	return (cmd_arguments);
}

static void	ft_close_fd(int fd)
{
	if (fd != -1 && close(fd) == -1)
		ft_fatal_error();
}

static void	ft_setup_pipe(t_micro_shell *shell)
{
	if (shell->command.type == TYPE_PIPE)
		if (pipe(shell->pipe_fd) == -1)
			ft_fatal_error();
}

static void	ft_cannot_execute_commands(t_micro_shell *shell)
{
	ft_print_error("cannot execute ");
	ft_putstr_fd(shell->command.arguments[0], STDERR_FILENO);
	ft_putstr_fd("\n", STDERR_FILENO);
	exit(EXIT_FAILURE);
}

static void	ft_execute_child_process(t_micro_shell *shell)
{
	if (shell->prev_pipe_fd != -1)
		if (dup2(shell->prev_pipe_fd, STDIN_FILENO) == -1)
			ft_fatal_error();
	if (shell->command.type == TYPE_PIPE)
		if (dup2(shell->pipe_fd[STDOUT_FILENO], STDOUT_FILENO) == -1)
			ft_fatal_error();
	ft_close_fd(shell->prev_pipe_fd);
	if (shell->command.type == TYPE_PIPE)
	{
		ft_close_fd(shell->pipe_fd[STDIN_FILENO]);
		ft_close_fd(shell->pipe_fd[STDOUT_FILENO]);
	}
	if (execve(shell->command.arguments[0], \
		shell->command.arguments, shell->envp) == -1)
		ft_cannot_execute_commands(shell);
}

static void	ft_execute_parent_process(t_micro_shell *shell)
{
	waitpid(shell->pid, NULL, 0);
	ft_close_fd(shell->prev_pipe_fd);
	if (shell->command.type == TYPE_PIPE)
	{
		ft_close_fd(shell->pipe_fd[STDOUT_FILENO]);
		shell->prev_pipe_fd = shell->pipe_fd[STDIN_FILENO];
	}
	else
		shell->prev_pipe_fd = -1;
}

static int	ft_execute_external_command(t_micro_shell *shell)
{
	ft_setup_pipe(shell);
	shell->pid = fork();
	if (shell->pid == -1)
		ft_fatal_error();
	if (shell->pid == 0)
		ft_execute_child_process(shell);
	else
		ft_execute_parent_process(shell);
	return (EXIT_SUCCESS);
}

static void	ft_skip_semicolons(t_micro_shell *shell)
{
	while (shell->index < shell->argc && \
		strcmp(shell->argv[shell->index], SEMICOLON) == 0)
		shell->index++;
}

static int	ft_get_command_end(t_micro_shell *shell)
{
	int	index;

	index = shell->index;
	while (index < shell->argc && \
		strcmp(shell->argv[index], PIPE) != 0 && \
			strcmp(shell->argv[index], SEMICOLON) != 0)
				index++;
	return (index);
}

static bool	ft_is_pipe(t_micro_shell *shell)
{
	if (shell->index < shell->argc && \
		strcmp(shell->argv[shell->index], PIPE) == 0)
		return (true);
	return (false);
}

static bool	ft_is_semicolon(t_micro_shell *shell)
{
	if (shell->index < shell->argc && \
		strcmp(shell->argv[shell->index], SEMICOLON) == 0)
		return (true);
	return (false);
}

static void	ft_check_if_cd(t_micro_shell *shell)
{
	if (shell->command.arguments[0] && \
		strcmp(shell->command.arguments[0], CD) == 0)
		shell->command.type = TYPE_CD;
}

static void	ft_parse_arguments(t_micro_shell *shell)
{
	int	start;
	int	end;

	ft_skip_semicolons(shell);
	if (shell->index >= shell->argc)
		return ;
	start = shell->index;
	end = ft_get_command_end(shell);
	shell->command.arguments = ft_get_command_arguments(shell, start, end);
	shell->index = end;
	if (ft_is_pipe(shell))
	{
		shell->command.type = TYPE_PIPE;
		shell->index++;
	}
	else if (ft_is_semicolon(shell))
	{
		shell->command.type = TYPE_SEMICOLON;
		shell->index++;
	}
	else
		shell->command.type = TYPE_NONE;
	ft_check_if_cd(shell);
}

static int	ft_execute_command(t_micro_shell *shell)
{
	int	exit_code;

	exit_code = EXIT_FAILURE;
	if (shell->command.type == TYPE_CD)
		exit_code = ft_execute_cd(shell->command.arguments);
	else
		exit_code = ft_execute_external_command(shell);
	return (exit_code);
}

static void	ft_initialize_micro_shell(t_micro_shell *shell, \
	int argc, char **argv, char **envp)
{
	shell->argv = argv;
	shell->envp = envp;
	shell->argc = argc;
	shell->index = 0x1;
	shell->prev_pipe_fd = -1;
	shell->exit_code = EXIT_SUCCESS;
	shell->command.type = TYPE_NONE;
	shell->command.arguments = NULL;
}

int	main(int argc, char **argv, char **envp)
{
	t_micro_shell	shell;

	ft_initialize_micro_shell(&shell, argc, argv, envp);
	while (shell.index < shell.argc)
	{
		shell.command.arguments = NULL;
		shell.command.type = TYPE_NONE;
		ft_parse_arguments(&shell);
		if (shell.command.arguments)
		{
			shell.exit_code = ft_execute_command(&shell);
			free(shell.command.arguments);
			shell.command.arguments = NULL;
		}
	}
	return (shell.exit_code);
}
