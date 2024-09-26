#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stddef.h>


typedef	struct		s_list
{
	char			**argv;
	int				argc;
	int				pipe[2];
	int				write_pipe;
	int				read_pipe;
	struct s_list	*prev;
	struct s_list	*next;
}					t_list;

enum				e_pipe_sides
{
	FD_READ = 0,
	FD_WRITE = 1
};

int					ft_strlen(const char *str)
{
	const char		*beg;

	beg = str;
	while (*str)
		++str;
	return (str - beg);
}

void				ft_putstr_fd(int fd, const char *str)
{
	write(fd, str, ft_strlen(str));
}

void				exit_fatal(void)
{
	ft_putstr_fd(STDERR_FILENO, "error: fatal\n");
	exit(EXIT_FAILURE);
}

char				*ft_strdup(const char *str)
{
	char		*copy;
	int			len;

	len = ft_strlen(str);
	if (!(copy = malloc(len + 1)))
		exit_fatal();
	for (int i = 0; i <= len; ++i)
		copy[i] = str[i];
	return (copy);
}

t_list				*lst_new(void)
{
	t_list		*elem;

	if (!(elem = malloc(sizeof(t_list))))
		exit_fatal();
	elem->argv = NULL;
	elem->argc = 0;
	elem->write_pipe = 0;
	elem->read_pipe = 0;
	elem->prev = NULL;
	elem->next = NULL;
	return (elem);
}

void				lst_clear(t_list *lst)
{
	t_list		*elem;
	
	while (lst)
	{
		elem = lst;
		lst = lst->next;
		if (elem->argc > 0)
			for (int i = 0; i < elem->argc; ++i)
				free(elem->argv[i]);
		free(elem->argv);
		free(elem);
	}
}

t_list				*lst_get_last(t_list *lst)
{
	while (lst && lst->next)
		lst = lst->next;
	return (lst);
}

void				lst_push_back(t_list **lst, t_list *new)
{
	t_list		*last;

	if (!*lst)
	{
		*lst = new;
		return ;
	}
	last = lst_get_last(*lst);
	last->next = new;
	new->prev = last;
}

/******************************************************************************/

void				add_arg(t_list *cmd, const char *arg)
{
	char		**new_argv;

	if (!(new_argv = malloc((cmd->argc + 2) * sizeof(char *))))
		exit_fatal();
	for (int i = 0; i < cmd->argc; ++i)
		new_argv[i] = cmd->argv[i];
	new_argv[cmd->argc] = ft_strdup(arg);
	new_argv[cmd->argc + 1] = NULL;
	free(cmd->argv);
	cmd->argv = new_argv;
	cmd->argc += 1;
}

t_list				*parser_build(int argc, char **argv)
{
	static int		idx = 1;
	t_list			*cmd_list;
	t_list			*temp;

	cmd_list = NULL;
	while (idx < argc)
	{
		if (!strcmp(argv[idx], ";"))
		{
			if (cmd_list)
			{
				++idx;
				return (cmd_list);
			}
		}
		else if (!strcmp(argv[idx], "|"))
		{
			if (!cmd_list)
				exit_fatal();
			temp = lst_get_last(cmd_list);
			temp->write_pipe = 1;
			temp = lst_new();
			temp->read_pipe = 1;
			lst_push_back(&cmd_list, temp);
		}
		else
		{
			if (!cmd_list)
				cmd_list = lst_new();
			add_arg(lst_get_last(cmd_list), argv[idx]);
		}
		++idx;
	}
	return (cmd_list);
}

int					builtin_cd(t_list *cmd)
{
	if (cmd->argc != 2)
	{
		ft_putstr_fd(STDERR_FILENO, "error: cd: bad arguments\n");
		return (1);
	}
	else if (chdir(cmd->argv[1]))
	{
		ft_putstr_fd(STDERR_FILENO, "error: cd: cannot change directory to ");
		ft_putstr_fd(STDERR_FILENO, cmd->argv[1]);
		ft_putstr_fd(STDERR_FILENO, "\n");
		return (1);
	}
	return (0);
}

int					exec_cmd(t_list *cmd, char **envp)
{
	int			ret;
	int			stat_loc;
	pid_t		pid;

	if (cmd->write_pipe && pipe(cmd->pipe) < 0)
		exit_fatal();
	if ((pid = fork()) < 0)
		exit_fatal();
	if (pid == 0)		// Child process.
	{
		if (cmd->write_pipe && dup2(cmd->pipe[FD_WRITE], STDOUT_FILENO) < 0)
			exit_fatal();
		else if (cmd->read_pipe && dup2(cmd->prev->pipe[FD_READ], STDIN_FILENO) < 0)
			exit_fatal();
		if ((ret = execve(cmd->argv[0], cmd->argv, envp)))
		{
			ft_putstr_fd(STDERR_FILENO, "error: cannot execute ");
			ft_putstr_fd(STDERR_FILENO, cmd->argv[0]);
			ft_putstr_fd(STDERR_FILENO, "\n");
		}
		exit(ret);
	}
	else				// Parent process.
	{
		waitpid(pid, &stat_loc, 0);
		if (cmd->write_pipe && close(cmd->pipe[FD_WRITE]) < 0)
			exit_fatal();
		else if (cmd->read_pipe && close(cmd->prev->pipe[FD_READ]) < 0)
			exit_fatal();
		if (WIFEXITED(stat_loc))
			return (WEXITSTATUS(stat_loc));
		return (EXIT_FAILURE);
	}
}

int					execute(t_list *cmd_list, char **envp)
{
	int		ret;

	while (cmd_list)
	{
		if (!strcmp(cmd_list->argv[0], "cd"))
			ret = builtin_cd(cmd_list);
		else
			ret = exec_cmd(cmd_list, envp);
		cmd_list = cmd_list->next;
	}
	return (ret);
}

int					main(int argc, char **argv, char **envp)
{
	t_list		*cmd_list;
	int			ret;

	while ((cmd_list = parser_build(argc, argv)))
	{
		ret = execute(cmd_list, envp);
		lst_clear(cmd_list);
	}
#ifdef TEST_SH
	while (1);
#endif
	return (ret);
}