#include <unistd.h>	 // write, fork, execve, dup2, close, chdir, STDERR_FILENO
#include <sys/wait.h>   // waitpid
#include <stdlib.h>	 // malloc, free, exit, EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>	 // strcmp
#include <stdbool.h>	// bool, true, false
#include <stdio.h>	  // printf

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

typedef struct s_micro_shell
{
	pid_t		   pid;
	char			**argv;
	char			**envp;
	int			 argc;
	int			 index;
	int			 exit_code;
	int			 pipe_fd[2];
	int			 prev_pipe_fd;
	char			**arguments;
	t_command_type  type;
}   t_micro_shell;

// Fonction de debug pour afficher la structure t_micro_shell
static void ft_print_micro_shell(t_micro_shell *shell)
{
	int index = 0;

	printf("Micro Shell Structure:\n");
	printf("PID: %d\n", shell->pid);
	printf("Argc: %d\n", shell->argc);
	printf("Index: %d\n", shell->index);
	printf("Exit Code: %d\n", shell->exit_code);
	printf("Pipe FD: [%d, %d]\n", shell->pipe_fd[0], shell->pipe_fd[1]);
	printf("Prev Pipe FD: %d\n", shell->prev_pipe_fd);
	printf("Type: ");
	if (shell->type == TYPE_NONE)
		printf("TYPE_NONE\n");
	else if (shell->type == TYPE_PIPE)
		printf("TYPE_PIPE\n");
	else if (shell->type == TYPE_SEMICOLON)
		printf("TYPE_SEMICOLON\n");
	else if (shell->type == TYPE_CD)
		printf("TYPE_CD\n");
	else
		printf("UNKNOWN\n");
	printf("Arguments:\n");
	if (shell->arguments)
	{
		while (shell->arguments[index])
		{
			printf("  arguments[%d]: %s\n", index, shell->arguments[index]);
			index++;
		}
	}
	else
		printf("  No arguments\n");
}

// Autres fonctions inchangées, sauf pour les modifications mentionnées
static size_t   ft_strlen(char *string)
{
	size_t length = 0;

	if (!string)
		return (0);
	while (string[length] != '\0')
		length++;
	return (length);
}

static void ft_putstr_fd(char *string, int fd)
{
	if (!string)
		string = "(null)";
	if (fd >= 0)
		write(fd, string, ft_strlen(string));
}

static void ft_print_error(char *message)
{
	ft_putstr_fd("error: ", STDERR_FILENO);
	ft_putstr_fd(message, STDERR_FILENO);
}

static void ft_fatal_error(void)
{
	ft_print_error("fatal\n");
	exit(EXIT_FAILURE);
}

static int ft_execute_cd(char **arguments)
{
	if (!arguments[1] || arguments[2])
	{
		ft_print_error("cd: bad arguments\n");
		return (EXIT_FAILURE);
	}
	if (chdir(arguments[1]) == -1)
	{
		ft_print_error("cd: cannot change directory to ");
		ft_putstr_fd(arguments[1], STDERR_FILENO);
		ft_putstr_fd("\n", STDERR_FILENO);
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

static char **ft_get_command_arguments(t_micro_shell *shell, int start, int end)
{
	char	**cmd_arguments;
	int	 index;

	cmd_arguments = (char **)malloc(sizeof(char *) * (end - start + 1));
	if (!cmd_arguments)
		ft_fatal_error();
	index = 0;
	while (start < end)
	{
		cmd_arguments[index] = shell->argv[start];
		index++;
		start++;
	}
	cmd_arguments[index] = NULL;
	return (cmd_arguments);
}

static void ft_close_fd(int fd)
{
	if (fd != -1 && close(fd) == -1)
		ft_fatal_error();
}

static void ft_setup_pipe(t_micro_shell *shell)
{
	if (shell->type == TYPE_PIPE)
		if (pipe(shell->pipe_fd) == -1)
		{
			perror("pipe"); // Debug
			ft_fatal_error();
		}
}

static void ft_cannot_execute_commands(t_micro_shell *shell)
{
	ft_print_error("cannot execute ");
	ft_putstr_fd(shell->arguments[0], STDERR_FILENO);
	ft_putstr_fd("\n", STDERR_FILENO);
	exit(EXIT_FAILURE);
}

static void ft_execute_child_process(t_micro_shell *shell)
{
	printf("In child process (PID: %d)\n", getpid()); // Debug
	ft_print_micro_shell(shell); // Debug
	if (shell->prev_pipe_fd != -1)
		if (dup2(shell->prev_pipe_fd, STDIN_FILENO) == -1)
			ft_fatal_error();
	if (shell->type == TYPE_PIPE)
		if (dup2(shell->pipe_fd[STDOUT_FILENO], STDOUT_FILENO) == -1)
			ft_fatal_error();
	ft_close_fd(shell->prev_pipe_fd);
	if (shell->type == TYPE_PIPE)
	{
		ft_close_fd(shell->pipe_fd[STDIN_FILENO]);
		ft_close_fd(shell->pipe_fd[STDOUT_FILENO]);
	}
	if (execve(shell->arguments[0], shell->arguments, shell->envp) == -1)
		ft_cannot_execute_commands(shell);
}

static void ft_execute_parent_process(t_micro_shell *shell)
{
	int status;

	if (waitpid(shell->pid, &status, 0) == -1)
		ft_fatal_error();
	if (WIFEXITED(status))
	{
		printf("Child exited with status %d\n", WEXITSTATUS(status)); // Debug
		shell->exit_code = WEXITSTATUS(status);
	}
	else
	{
		printf("Child terminated abnormally\n"); // Debug
		shell->exit_code = EXIT_FAILURE;
	}
	ft_close_fd(shell->prev_pipe_fd);
	if (shell->type == TYPE_PIPE)
	{
		ft_close_fd(shell->pipe_fd[STDOUT_FILENO]);
		shell->prev_pipe_fd = shell->pipe_fd[STDIN_FILENO];
	}
	else
		shell->prev_pipe_fd = -1;
}

static int ft_execute_external_command(t_micro_shell *shell)
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

static void ft_skip_semicolons(t_micro_shell *shell)
{
	while (shell->index < shell->argc && \
		strcmp(shell->argv[shell->index], SEMICOLON) == 0)
		shell->index++;
}

static int ft_get_command_end(t_micro_shell *shell)
{
	int index;

	index = shell->index;
	while (index < shell->argc &&
		   strcmp(shell->argv[index], PIPE) != 0 &&
		   strcmp(shell->argv[index], SEMICOLON) != 0)
		index++;
	return (index);
}

static bool ft_is_pipe(t_micro_shell *shell)
{
	if (shell->index < shell->argc && strcmp(shell->argv[shell->index], PIPE) == 0)
		return (true);
	return (false);
}

static bool ft_is_semicolon(t_micro_shell *shell)
{
	if (shell->index < shell->argc && strcmp(shell->argv[shell->index], SEMICOLON) == 0)
		return (true);
	return (false);
}

static void ft_check_if_cd(t_micro_shell *shell)
{
	if (shell->arguments[0] && strcmp(shell->arguments[0], CD) == 0)
		shell->type = TYPE_CD;
}

static void ft_parse_arguments(t_micro_shell *shell)
{
	int start;
	int end;

	ft_skip_semicolons(shell);
	if (shell->index >= shell->argc)
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
	ft_print_micro_shell(shell); // Debug
}

static int ft_execute_command(t_micro_shell *shell)
{
	int exit_code;

	exit_code = EXIT_FAILURE;
	printf("Executing command...\n"); // Debug
	ft_print_micro_shell(shell);	 // Debug
	if (shell->type == TYPE_CD)
		exit_code = ft_execute_cd(shell->arguments);
	else
		exit_code = ft_execute_external_command(shell);
	return (exit_code);
}

static void ft_initialize_micro_shell(t_micro_shell *shell, int argc, char **argv, char **envp)
{
	shell->argv = argv;
	shell->envp = envp;
	shell->argc = argc;
	shell->index = 1;
	shell->prev_pipe_fd = -1;
	shell->exit_code = EXIT_SUCCESS;
	shell->arguments = NULL;
	shell->type = TYPE_NONE;
}

int main(int argc, char **argv, char **envp)
{
	t_micro_shell shell;

	printf("Starting microshell...\n"); // Debug
	ft_initialize_micro_shell(&shell, argc, argv, envp);
	while (shell.index < shell.argc)
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
	printf("Microshell exited with code %d\n", shell.exit_code); // Debug
	return (shell.exit_code);
}
