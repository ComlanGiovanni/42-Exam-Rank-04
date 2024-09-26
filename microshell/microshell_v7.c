#include "microshell.h"

void	ft_putstr_fd(char const *str, int fd)
{
	while (*str)
		write(fd, str++, 1);
}

void	*fatal_ptr(void)
{
	ft_putstr_fd("error: fatal\n", STDERR);
	exit(EXIT_FAILURE);
	return (NULL);
}

int		fatal_int(void)
{
	ft_putstr_fd("error: fatal\n", STDERR);
	exit(EXIT_FAILURE);
	return (EXIT_FAILURE);
}

size_t	ft_strlen(char const *str)
{
	size_t len;

	len = 0;
	while (*str++)
		len++;
	return (len);
}

void	push_arg(t_cmds *cmds, char *arg)
{
	char **temp;
	int i;

	if (!(temp = (char **)malloc(sizeof(char *) * (cmds->len + 2))))
		fatal_int();
	i = -1;
	while (++i < cmds->len)
	{
		temp[i] = cmds->args[i];
	}
	temp[i++] = arg;
	temp[i] = NULL;
	free(cmds->args);
	cmds->args = temp;
	cmds->len++;
}

t_cmds	*clear_list(t_cmds *cmds)
{
	void *tmp;

	while (cmds)
	{
		free(cmds->args);
		cmds->args = NULL;
		tmp = cmds->next;
		free(cmds);
		cmds = tmp;
	}
	return (NULL);
}

t_cmds	*new_cmd_node(void)
{
	t_cmds *new;

	if (!(new = (t_cmds *)malloc(sizeof(t_cmds))))
		return (fatal_ptr());
	new->next = NULL;
	new->prev = NULL;
	new->args = NULL;
	new->len = 0;
	new->type = END;
	return (new);
}

void	push_front_cmd(t_cmds **cmds_list, char *arg)
{
	t_cmds *new;

	new = new_cmd_node();
	push_arg(new, arg);
	if (*cmds_list)
	{
		(*cmds_list)->next = new;
		new->prev = *cmds_list;
	}
	*cmds_list = new;
}

void	parse_args(t_cmds **cmds, int ac, char **av)
{
	int i;
	char is_s_colon;

	i = 0;
	while (++i < ac)
	{
		is_s_colon = (strcmp(av[i], ";") == 0);
		if (is_s_colon && !*cmds)
			continue;
		if (!is_s_colon && (!*cmds || (*cmds)->type != END))
			push_front_cmd(cmds, av[i]);
		else if (is_s_colon)
			(*cmds)->type = S_COLON;
		else if (!strcmp(av[i], "|"))
			(*cmds)->type = PIPE;
		else
			push_arg(*cmds, av[i]);
	}
	while (*cmds && (*cmds)->prev)
		*cmds = (*cmds)->prev;
}

int		cd(t_cmds *cmds)
{
	int ret;

	ret = EXIT_SUCCESS;
	if (cmds->len != 2)
	{
		ft_putstr_fd("error: cd: bad arguments\n", STDERR);
		ret = EXIT_FAILURE;
	}
	else
	{
		if ((ret = chdir(cmds->args[1])))
		{
			ft_putstr_fd("error: cd: cannot change directory to ", STDERR);
			ft_putstr_fd(cmds->args[1], STDERR);
			ft_putstr_fd("\n", STDERR);
		}
	}
	return (ret);
}

void	run_child_process(t_cmds *cmd, char **env)
{
	int ret;
	
	ret = EXIT_SUCCESS;
	if (cmd->type == PIPE)
	{
		if (dup2(cmd->fds[1], STDOUT) == -1)
			fatal_int();
	}
	if (cmd->prev && cmd->prev->type == PIPE)
	{
		if (dup2(cmd->prev->fds[0], STDIN) == -1)
			fatal_int();
	}
	if ((ret = execve(cmd->args[0], cmd->args, env)) == -1)
	{
		ft_putstr_fd("error: cannot execute ", STDERR);
		ft_putstr_fd(cmd->args[0], STDERR);
		ft_putstr_fd("\n", STDERR);
	}
	exit(ret);
}

int		single_cmd_exec(t_cmds *cmd, char **env)
{
	int ret;
	pid_t pid;
	int status;
	char is_pipe_open;

	ret = EXIT_SUCCESS;
	is_pipe_open = 0;
	if (cmd->type == PIPE || (cmd->prev && cmd->prev->type == PIPE))
	{
		is_pipe_open = 1;
		if (pipe(cmd->fds))
			return (fatal_int());
	}
	if ((pid = fork()) == -1)
		return (fatal_int());
	if (pid == 0)
		run_child_process(cmd, env);
	else
	{
		waitpid(pid, &status, 0);
		if (is_pipe_open)
		{
			close(cmd->fds[1]);
			if (cmd->type != PIPE)
				close(cmd->fds[0]);
			if (cmd->prev && cmd->prev->type == PIPE)
				close(cmd->prev->fds[0]);
		}
		if (WIFEXITED(status))
			ret = WEXITSTATUS(status);
	}
	return (ret);
}

int		cmds_exec(t_cmds *cmds, char **env)
{
	int ret;

	ret = EXIT_SUCCESS;
	while (cmds)
	{
		if (!strcmp(cmds->args[0], "cd"))
			ret = cd(cmds);
		else
			ret = single_cmd_exec(cmds, env);
		cmds = cmds->next;
	}
	return (ret);
}

int		main(int ac, char **av, char **env)
{
	t_cmds *cmds;
	int ret;

	cmds = NULL;
	ret = EXIT_SUCCESS;
	parse_args(&cmds, ac, av);
	ret = cmds_exec(cmds, env);
	getchar();
	cmds = clear_list(cmds);
	return (ret);
}


/*

#ifndef MICROSHELL_H
# define MICROSHELL_H 

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

# define STDIN		0
# define STDOUT		1
# define STDERR		2

# define END		0
# define S_COLON	1
# define PIPE		2

typedef struct		s_cmds
{
	char 			**args;
	char 			type;
	int 			len;
	int 			fds[2];
	struct s_cmds 	*next;
	struct s_cmds 	*prev;
}					t_cmds;

#endif // !MICROSHELL_H

*/