#include <unistd.h>     // write, fork, execve, dup2, close, chdir, STDERR_FILENO
#include <sys/wait.h>   // waitpid pid_t inside types
#include <stdlib.h>     // malloc, free, exit, EXIT_FAILURE, EXIT_SUCCESS, strdup
#include <string.h>     // strcmp, strdup
#include <stdbool.h>    // bool, true, false
#include <stdio.h>      // printf

#define PIPE "|"
#define SEMICOLON ";"
#define CHANGE_DIRECTORY "cd"

typedef enum e_pipe_fd
{
    PIPE_INPUT = 0,
    PIPE_OUTPUT = 1
}   t_pipe_fd;

typedef struct s_main_variable
{
    char    **argv;
    char    **env;
    int     argc;
}   t_main_variable;

typedef struct s_pipe_node
{
    int                 pipe_fds[2];        // [PIPE_INPUT, PIPE_OUTPUT]
    struct s_pipe_node  *next;
}   t_pipe_node;

typedef enum e_command_type
{
    TYPE_NONE,
    TYPE_PIPE,
    TYPE_SEMICOLON,
    TYPE_CD
}   t_command_type;

typedef struct s_micro_shell
{
    t_main_variable main_vars;
    t_pipe_node     *pipe_list;       // Liste chaînée des pipes
    char            **arguments;
    t_command_type  type;
    pid_t           pid;
    int             index;
    int             exit_code;
}   t_micro_shell;

// Déclarations des fonctions
static void ft_close_fd(int fd, t_micro_shell *shell);
static void ft_print_error(char *message);
static void ft_fatal_error(t_micro_shell *shell);
static int ft_execute_cd(char **arguments);
static void ft_free_fail_malloc_arguments(t_micro_shell *shell, char **cmd_arguments, int max_index);
static char **ft_get_command_arguments(t_micro_shell *shell, int start, int end);
static void ft_pipe_list_add_back(t_pipe_node **head);
static t_pipe_node *ft_pipe_list_init(void);
static void ft_pipe_list_close_and_free(t_pipe_node **head);
static void ft_setup_pipe(t_micro_shell *shell);
static void ft_redirect_fds(t_micro_shell *shell);
static void ft_close_child_fds(t_micro_shell *shell);
static void ft_execute_child_process(t_micro_shell *shell);
static void ft_handle_child_status(t_micro_shell *shell, int status);
static void ft_execute_parent_process(t_micro_shell *shell);
static int ft_execute_external_command(t_micro_shell *shell);
static void ft_skip_semicolons(t_micro_shell *shell);
static int ft_get_command_end(t_micro_shell *shell);
static bool ft_is_pipe(t_micro_shell *shell);
static bool ft_is_semicolon(t_micro_shell *shell);
static void ft_check_if_cd(t_micro_shell *shell);
static void ft_parse_arguments(t_micro_shell *shell);
static int ft_execute_command(t_micro_shell *shell);
static void ft_cleanup(t_micro_shell *shell);
static void ft_initialize_micro_shell(t_micro_shell *shell, int argc, char **argv, char **envp);


// Implémentation des fonctions

static t_pipe_node *ft_pipe_list_init(void)
{
    return (NULL); // Liste vide au départ
}

static void ft_pipe_list_add_back(t_pipe_node **head)
{
    t_pipe_node *new_node;
    t_pipe_node *temp;

    new_node = malloc(sizeof(t_pipe_node));
    if (!new_node)
    {
        ft_print_error("error: malloc failed\n");
        ft_fatal_error(NULL); // Adapter ft_fatal_error pour accepter NULL
    }
    if (pipe(new_node->pipe_fds) == -1)
    {
        ft_print_error("error: pipe failed\n");
        free(new_node);
        ft_fatal_error(NULL);
    }
    new_node->next = NULL;

    if (*head == NULL)
    {
        *head = new_node;
    }
    else
    {
        temp = *head;
        while (temp->next)
            temp = temp->next;
        temp->next = new_node;
    }
}

static void ft_pipe_list_close_and_free(t_pipe_node **head)
{
    t_pipe_node *temp;

    while (*head)
    {
        temp = *head;
        ft_close_fd(temp->pipe_fds[PIPE_INPUT], NULL); // Adapter ft_close_fd pour accepter NULL
        ft_close_fd(temp->pipe_fds[PIPE_OUTPUT], NULL);
        *head = (*head)->next;
        free(temp);
    }
}

size_t ft_strlen(char *string)
{
    const char *last_char_in_string;

    if (!string)
        return (0);
    last_char_in_string = string;
    while (*last_char_in_string)
        last_char_in_string++;
    return (last_char_in_string - string);
}

static char *ft_strcpy(char *destination, char *source)
{
    size_t index = 0;
    while (source[index] != '\0')
    {
        destination[index] = source[index];
        index++;
    }
    destination[index] = '\0';
    return (destination);
}

static char *ft_strdup(const char *s1)
{
    size_t len = ft_strlen((char *)s1);
    char *dup = malloc(len + 1);
    if (!dup)
        return (NULL);
    ft_strcpy(dup, (char *)s1);
    return (dup);
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

static void ft_fatal_error(t_micro_shell *shell)
{
    ft_print_error("fatal\n");
    if (shell)
        ft_cleanup(shell);
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

static void ft_free_fail_malloc_arguments(t_micro_shell *shell, char **cmd_arguments, int max_index)
{
    while (max_index > 0)
    {
        free(cmd_arguments[max_index - 1]);
        max_index--;
    }
    free(cmd_arguments);
    ft_print_error("Malloc arguments fail\n");
    ft_fatal_error(shell);
}

static char **ft_get_command_arguments(t_micro_shell *shell, int start, int end)
{
    char    **cmd_arguments;
    int     index;
    int     cmd_length;

    if (start >= end)
        return (NULL);
    cmd_length = end - start;
    cmd_arguments = (char **)malloc(sizeof(char *) * (cmd_length + 1));
    if (!cmd_arguments)
        ft_fatal_error(shell);
    index = 0;
    while (start < end)
    {
        cmd_arguments[index] = ft_strdup(shell->main_vars.argv[start]);
        if (!cmd_arguments[index])
            ft_free_fail_malloc_arguments(shell, cmd_arguments, index);
        index++;
        start++;
    }
    cmd_arguments[index] = NULL;
    return (cmd_arguments);
}

static void ft_close_fd(int fd, t_micro_shell *shell)
{
    if (fd != -1 && close(fd) == -1)
    {
        // perror("close"); // Debug
        if (shell)
            ft_fatal_error(shell);
        else
            exit(EXIT_FAILURE);
    }
}

static void ft_setup_pipe(t_micro_shell *shell)
{
    if (shell->type == TYPE_PIPE)
    {
        ft_pipe_list_add_back(&shell->pipe_list);
    }
}

static void ft_redirect_fds(t_micro_shell *shell)
{
    t_pipe_node *current;

    current = shell->pipe_list;
    if (current && current->pipe_fds[PIPE_INPUT] != -1)
    {
        if (dup2(current->pipe_fds[PIPE_INPUT], STDIN_FILENO) == -1)
        {
            ft_print_error("dup2 failed\n");
            ft_fatal_error(shell);
        }
    }

    if (shell->type == TYPE_PIPE && current)
    {
        if (dup2(current->pipe_fds[PIPE_OUTPUT], STDOUT_FILENO) == -1)
        {
            ft_print_error("dup2 failed\n");
            ft_fatal_error(shell);
        }
    }
}

static void ft_close_child_fds(t_micro_shell *shell)
{
    t_pipe_node *current;

    current = shell->pipe_list;
    if (current)
    {
        ft_close_fd(current->pipe_fds[PIPE_INPUT], shell);
        ft_close_fd(current->pipe_fds[PIPE_OUTPUT], shell);
    }
}

static void ft_cannot_execute_commands(t_micro_shell *shell)
{
    ft_print_error("cannot execute ");
    ft_putstr_fd(shell->arguments[0], STDERR_FILENO);
    ft_putstr_fd("\n", STDERR_FILENO);
    ft_cleanup(shell);
    exit(EXIT_FAILURE);
}

static void ft_execute_child_process(t_micro_shell *shell)
{
    // Rediriger les descripteurs de fichiers
    ft_redirect_fds(shell);

    // Fermer les descripteurs inutilisés pour éviter les fuites
    ft_close_child_fds(shell);

    // Exécuter la commande avec execve
    if (execve(shell->arguments[0], shell->arguments, shell->main_vars.env) == -1)
    {
        ft_cannot_execute_commands(shell);
    }
}

static void ft_handle_child_status(t_micro_shell *shell, int status)
{
    if (WIFEXITED(status))
    {
        shell->exit_code = WEXITSTATUS(status);
    }
    else
    {
        shell->exit_code = EXIT_FAILURE;
    }
}

static void ft_execute_parent_process(t_micro_shell *shell)
{
    int status;

    if (waitpid(shell->pid, &status, 0) == -1)
    {
        ft_print_error("waitpid failed\n");
        ft_fatal_error(shell);
    }

    ft_handle_child_status(shell, status);

    // Fermer le pipe courant si c'est un pipe
    if (shell->type == TYPE_PIPE)
    {
        if (shell->pipe_list)
        {
            ft_close_fd(shell->pipe_list->pipe_fds[PIPE_OUTPUT], shell);
            // Déplacer le pointeur vers le prochain pipe si nécessaire
            shell->pipe_list = shell->pipe_list->next;
        }
    }
}

static int ft_execute_external_command(t_micro_shell *shell)
{
    ft_setup_pipe(shell);
    shell->pid = fork();
    if (shell->pid == -1)
    {
        ft_print_error("fork failed\n");
        ft_fatal_error(shell);
    }
    if (shell->pid == 0)
        ft_execute_child_process(shell);
    else
        ft_execute_parent_process(shell);
    return (EXIT_SUCCESS);
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
}

static int ft_execute_command(t_micro_shell *shell)
{
    int exit_code;

    exit_code = EXIT_FAILURE;
    if (shell->type == TYPE_CD)
        exit_code = ft_execute_cd(shell->arguments);
    else
        exit_code = ft_execute_external_command(shell);
    return (exit_code);
}

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

    // Fermer et libérer tous les pipes
    ft_pipe_list_close_and_free(&shell->pipe_list);
}

static void ft_initialize_micro_shell(t_micro_shell *shell, int argc, char **argv, char **envp)
{
    shell->main_vars.argv = argv;
    shell->main_vars.env = envp;
    shell->main_vars.argc = argc;
    shell->index = 1;
    shell->pipe_list = ft_pipe_list_init();
    shell->exit_code = EXIT_SUCCESS;
    shell->arguments = NULL;
    shell->type = TYPE_NONE;
}

int main(int argc, char **argv, char **envp)
{
    t_micro_shell shell;

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
    return (shell.exit_code);
}
