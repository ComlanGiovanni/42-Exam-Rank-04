/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   micro-shell.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gicomlan <gicomlan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/23 23:10:39 by gicomlan          #+#    #+#             */
/*   Updated: 2024/09/26 17:33:28 by gicomlan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

// Fonction pour afficher une erreur sur STDERR
void print_error(const char *msg)
{
    while (*msg)
        write(2, msg++, 1);
}

// Fonction pour gérer la commande built-in 'cd'
int execute_cd(char **argv, int argc)
{
    if (argc != 2)
    {
        print_error("error: cd: bad arguments\n");
        return 1;
    }
    if (chdir(argv[1]) == -1)
    {
        print_error("error: cd: cannot change directory to ");
        print_error(argv[1]);
        print_error("\n");
        return 1;
    }
    return 0;
}

// Fonction pour configurer les pipes
void setup_pipe(int has_pipe, int *fd, int end)
{
    if (has_pipe)
    {
        if (dup2(fd[end], end) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1)
        {
            print_error("error: fatal\n");
            exit(1);
        }
    }
}

// Fonction pour exécuter une commande
int execute_command(char **argv, int argc, char **envp)
{
    int has_pipe = 0;
    int fd[2];
    pid_t pid;
    int status;

    // Vérifier si la commande est suivie d'un pipe
    if (argv[argc] && strcmp(argv[argc], "|") == 0)
        has_pipe = 1;

    // Gérer la commande built-in 'cd' si elle n'est pas dans un pipe
    if (!has_pipe && strcmp(argv[0], "cd") == 0)
        return execute_cd(argv, argc);

    // Créer un pipe si nécessaire
    if (has_pipe && pipe(fd) == -1)
    {
        print_error("error: fatal\n");
        exit(1);
    }

    // Forker le processus
    pid = fork();
    if (pid == -1)
    {
        print_error("error: fatal\n");
        exit(1);
    }

    if (pid == 0)
    {
        // Processus enfant
        if (has_pipe)
        {
            if (dup2(fd[1], 1) == -1)
            {
                print_error("error: fatal\n");
                exit(1);
            }
            if (close(fd[0]) == -1 || close(fd[1]) == -1)
            {
                print_error("error: fatal\n");
                exit(1);
            }
        }

        execve(argv[0], argv, envp);
        // Si execve échoue
        print_error("error: cannot execute ");
        print_error(argv[0]);
        print_error("\n");
        exit(1);
    }

    // Processus parent
    if (has_pipe)
    {
        if (dup2(fd[0], 0) == -1)
        {
            print_error("error: fatal\n");
            exit(1);
        }
        if (close(fd[0]) == -1 || close(fd[1]) == -1)
        {
            print_error("error: fatal\n");
            exit(1);
        }
    }

    // Attendre la fin du processus enfant
    if (waitpid(pid, &status, 0) == -1)
    {
        print_error("error: fatal\n");
        exit(1);
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    return 0;
}

int main(int argc, char **argv, char **envp)
{
    int i = 1; // Commencer à 1 pour ignorer le nom du programme
    int status = 0;

    while (i < argc)
    {
        char **cmd = &argv[i];
        int cmd_length = 0;

        // Trouver la longueur de la commande actuelle
        while (i + cmd_length < argc && strcmp(argv[i + cmd_length], "|") != 0 && strcmp(argv[i + cmd_length], ";") != 0)
            cmd_length++;

        if (cmd_length > 0)
        {
            status = execute_command(cmd, cmd_length, envp);
        }

        i += cmd_length;

        // Gérer les séparateurs
        if (i < argc && strcmp(argv[i], "|") == 0)
            i++; // Passer le '|'
        else if (i < argc && strcmp(argv[i], ";") == 0)
            i++; // Passer le ';'
    }

    return status;
}
