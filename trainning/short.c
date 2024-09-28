/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   short.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gicomlan <gicomlan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/28 09:21:24 by gicomlan          #+#    #+#             */
/*   Updated: 2024/09/28 12:26:16 by gicomlan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>     // write, chdir, dup2, close, execve
#include <sys/wait.h>   // waitpid
#include <stdlib.h>     // exit
#include <string.h>     // strcmp

// Function to print an error msg to stderr
void ft_print_error(char *msg)
{
	while (*msg)
		write(STDERR_FILENO, msg++, 1);
}

// Function to change directory
int ft_execute_cd(char **arg, int arg_count)
{
	if (arg_count != 2)
		return (ft_print_error("error: cd: bad arguments\n"), 1);
	if (chdir(arg[1]) == -1)
		return (ft_print_error("error: cd: cannot change directory to "), \
			ft_print_error(arg[1]), ft_print_error("\n"), 1);
	return (0);
}

// Function to set up a pipe if needed
void ft_configure_pipe(int has_pipe, int *pipe_fds, int end)
{
	if (has_pipe && (dup2(pipe_fds[end], end) == -1 || \
		close(pipe_fds[0]) == -1 || close(pipe_fds[1]) == -1))
		ft_print_error("error: fatal\n"), exit(EXIT_FAILURE);
}

// Function to execute commands and handle pipes
int ft_execute_command(char **arg, int arg_count, char **env)
{
	int has_pipe, pipe_fds[2], pid, code;

	has_pipe = arg[arg_count] && !strcmp(arg[arg_count], "|");
	if (!has_pipe && !strcmp(*arg, "cd"))
		return (ft_execute_cd(arg, arg_count));
	if (has_pipe && pipe(pipe_fds) == -1)
		ft_print_error("error: fatal\n"), exit(EXIT_FAILURE);
	if ((pid = fork()) == -1)
		ft_print_error("error: fatal\n"), exit(EXIT_FAILURE);
	if (pid == 0)
	{
		arg[arg_count] = NULL;
		ft_configure_pipe(has_pipe, pipe_fds, STDOUT_FILENO);
		if (!strcmp(*arg, "cd"))
			exit(ft_execute_cd(arg, arg_count));
		execve(arg[0], arg, env);
		ft_print_error("error: cannot execute "), ft_print_error(arg[0]), \
		ft_print_error("\n"), exit(EXIT_FAILURE);
	}
	waitpid(pid, &code, 0);
	ft_configure_pipe(has_pipe, pipe_fds, STDIN_FILENO);
	return (WIFEXITED(code) && WEXITcode(code));
}

// Main function to parse and execute commands
int main(int argc, char **argv, char **env)
{
	(void)argc;
	int index = 0, code = 0;

	while (argv[index])
	{
		argv += index + 1;
		index = 0;
		while (argv[index] && strcmp(argv[index], "|") && \
		strcmp(argv[index], ";"))
			index++;
		if (index)
			code = ft_execute_command(argv, index, env);
	}
	return (code);
}
