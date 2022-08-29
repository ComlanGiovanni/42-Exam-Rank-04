#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

// A single command
typedef struct s_cmd
{
	char *bin;
	char **argv;
	int piped;
	int in;
	int out;
} t_cmd;

// Static array of commands, no malloc, no linked-list
t_cmd cmds[1000];
// Copy of envp, available everywere
char **env;

// If a fatal error occurs
void fatal(void)
{
	printf("error: fatal\n");
	exit(1);
}

// Free all argv arrays for all commands
void free_cmds(void)
{
	int i = 0;
	while (cmds[i].bin)
		free(cmds[i++].argv);
}

// Are a and b equals?
int equ(char *a, char *b)
{
	int i = 0;
	while (a[i] && b[i] && a[i] == b[i])
		i++;
	return (a[i] == b[i]);
}

// Create an empty commands array
void init_cmds(void)
{
	int i = -1;
	while (++i < 1000)
	{
		cmds[i].bin = NULL;
		cmds[i].argv = NULL;
	}
}

// Debug only, to print the array of commnds
void print_cmds(void)
{
	int i = -1;
	while (cmds[++i].bin)
	{
		printf("cmd \"%s\", in: %d, out: %d\n", cmds[i].bin, cmds[i].in, cmds[i].out);
		int j = -1;
		while (cmds[i].argv[++j])
			printf("    arg: %s\n", cmds[i].argv[j]);
		if (cmds[i].piped)
			printf("    |  \n    |  \n   \\ /  \n    v  \n");
	}
}

// Copy the argv of the last command, add str, free the previous argv
void add_argv(char *str)
{
	int i = 0;
	while (i < 1000 && cmds[i].bin)
		++i;
	if (i > 0)
		--i;
	int size = 0;
	while (cmds[i].argv[size])
		++size;
	char **tmp = malloc(sizeof(char *) * (2 + size));
	if (!tmp)
		fatal();
	int j = -1;
	while (++j < size)
		tmp[j] = cmds[i].argv[j];
	tmp[j++] = str;
	tmp[j] = NULL;
	free(cmds[i].argv);
	cmds[i].argv = tmp;
}

// Create a command
// Malloc argv
void add_cmd(char *str)
{
	int i = 0;
	while (i < 1000 && cmds[i].bin)
		++i;
	cmds[i].bin = str;
	cmds[i].argv = malloc(sizeof(char *));
	if (!cmds[i].argv)
		fatal();
	cmds[i].argv[0] = NULL;
	cmds[i].piped = 0;
	cmds[i].in = 0;
	cmds[i].out = 1;
}

// Set the last created command to piped
void set_piped()
{
	int i = 0;
	while (i < 1000 && cmds[i].bin)
		++i;
	if (i > 0)
		--i;
	cmds[i].piped = 1;
}

// Loop over args
// If first we use arg to create a new command, else we consider arg as an argument
// If arg is ; or |, we skip, we set last created command to piped and pass first to false
void parse_args(int argc, char **argv)
{
	int i = 0;
	int is_first = 1;
	while (++i < argc)
	{
		if (equ(argv[i], ";"))
			is_first = 1;
		else if (equ(argv[i], "|"))
		{
			set_piped();
			is_first = 1;
		}
		else if (is_first)
		{
			add_cmd(argv[i]);
			add_argv(argv[i]);
			is_first = 0;
		}
		else
		{
			add_argv(argv[i]);
		}
	}
}

// Create and set io for a group of piped commands
// We connect every n command to the n + 1 with a pipe
// Don't forget: pipe output is index 0, pipe input index 1
// So output of n is pipe[0], input of n + 1 is pipe[1]
void io(int start, int end)
{
	int i = start;
	int fds[2];
	while (i  < end)
	{
		pipe(fds);
		cmds[i].out = fds[1];
		cmds[i + 1].in = fds[0];
		++i;
	}
}

// Exec a single command, in order:
// fork
// redirect io
// exec the command
// exit the process
// wait
// close our redirections
void exec(int i)
{
	int pid = fork();
	if (pid < 0)
		fatal();
	if (pid == 0)
	{
		dup2(cmds[i].out, 1);
		dup2(cmds[i].in, 0);
		int res = execve(cmds[i].bin, cmds[i].argv, env);
		if (res < 0)
			printf("error: cannot execute \"%s\"\n", cmds[i].bin);
		exit(res);
	}
	else
	{
		waitpid(pid, NULL, 0);
		if (cmds[i].in != 0)
			close(cmds[i].in);
		if (cmds[i].out != 1)
			close(cmds[i].out);
	}
}

// cd built-in
void cd(int i)
{
	int size = 0;
	while (cmds[i].argv[size])
		++size;
	if (size != 2)
	{
		printf("error: cd: bad arguments\n");
		return;
	}
	chdir(cmds[i].argv[1]);
}

// Execute a group of piped commands
void exec_pipe(int start, int end)
{
	int i = start;
	if (end > start)
		io(start, end);
	while (i < end + 1)
	{
		if (equ(cmds[i].bin, "cd"))
			cd(i);
		else
			exec(i);
		++i;
	}
}

// After all the parsing is done, we group command by pipe group and we execute each group
// ex: ls | wc; ps; ps | rev | cut
// 3 groups: "ls | wc" (0, 1), "ps" (2, 2) and "ps | rev | cut" (3, 5)
void exec_cmds(void)
{
	int i = 0;
	while (cmds[i].bin)
	{
		int j = i;
		while (cmds[i].piped)
			++i;
		exec_pipe(j, i);
		++i;
	}
}


// Entry point of the program, don't forget to get envp
int main(int argc, char **argv, char **envp)
{
	env = envp;
	parse_args(argc, argv);
	exec_cmds();
	free_cmds();
	return (0);
}