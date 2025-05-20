/***********************************************************************
EP01 - Shell

Disciplina: MAC0422 - Sistemas Operacionais
Professor: Daniel Batista
Aluno: Francisco Membrive

Data: 22.04.2025

uso: 
    make uspsh && ./uspsh
    comandos: cd, whoami, chmod, ls, top, ep1

***********************************************************************/

#include "uspsh.h"

int main(void){
    /*nome do pc*/
    char hostname[256];
    gethostname(hostname, sizeof(hostname));

    while(1){

        /* diretorio */
        char cwd[512];
        getcwd(cwd, sizeof(cwd));

        /* prompt */
        char prompt[1024];
        snprintf(prompt, sizeof(prompt), "[%s:%s]$ ", hostname, cwd);

        /* le a entrada */
        char *linha = readline(prompt);
        if (!linha)
            break;
        if (strlen(linha) == 0) {
            free(linha);
            continue;
        }
        add_history(linha);

        char *args[100];
        int argc = 0;
        char *token = strtok(linha, " \t");
        while (token != NULL && argc < 99){
            args[argc++] = token;
            token = strtok(NULL, " \t");
        }
        args[argc] = NULL;
        if (argc == 0) {
            free(linha);
            continue;
        }

        /* comandos internos: */
        if(strcmp(args[0], "cd") == 0){
            if(argc > 1){
                chdir(args[1]);
            }
        }

        if(strcmp(args[0], "whoami") == 0){
            uid_t uid = getuid();
            struct passwd *pw = getpwuid(uid);
            printf("%s\n", pw->pw_name);
        }

        if(strcmp(args[0], "chmod") == 0){
            if(argc > 2){
                int mode = strtol(args[1], NULL, 8);
                chmod(args[2], mode);
            }
        }

        /*externos; */
        if(strcmp(args[0], "ls") == 0){
            pid_t pid = fork();
            if(pid == 0){
                char *argus[] = {"ls", "-1aF", "--color=never", NULL};
                execv("/bin/ls", argus);
                exit(0);
            }
            else{
                wait(NULL);
            }
        }

        if(strcmp(args[0], "top") == 0){
            pid_t pid = fork();
            if(pid == 0){
                char *argus[] = {"top", "-b", "-n", "1", NULL};
                execv("/usr/bin/top", argus);
                exit(0);
            }
            else{
                wait(NULL);
            }
        }

        if(strcmp(args[0], "ep1") == 0 || strcmp(args[0], "./ep1") == 0){
            pid_t pid = fork();
            if(pid == 0){
                execv("./ep1", args);
                exit(0);
            }
            else{
                wait(NULL);
            }
        }

        free(linha);
    }
    return 0;
}
