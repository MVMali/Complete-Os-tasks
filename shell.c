#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>

// declaring global
int PsIntFlag = 0;

char prmname[1024];

char path[10][1024] = {"/usr/local/sbin/", "/usr/local/bin/", "/usr/sbin/", "/usr/bin/", "/bin/", "/usr/games/", "/usr/local/games/", "/snap/bin/", "/snap/bin/"};

int changedir(char *path)
{
    int succ;
    if (path == NULL)
    {
        succ = chdir("/home/");
    }
    else
    {
        succ = chdir(path);
    }
    if (succ == -1)
    {
        perror("Error has occured");
        return -1;
    }
}

int main()
{
    int pid;
    while (1)
    {
        if (PsIntFlag == 0)
        {
            char usrname[1024];
            getlogin_r(usrname, sizeof(usrname));
            getcwd(prmname, sizeof(prmname));
            strcat(prmname, "$ ");
            strcat(usrname, ":");
            strcat(usrname, prmname);
            strcpy(prmname, usrname);
        }
        printf("%s", prmname);
        // use of gets to get the input string which contains space also.
        char buff[1024];
        char *terminate = fgets(buff, 1024, stdin);
        if (!terminate)
        {
            exit(0);
        }

        buff[strlen(buff) - 1] = '\0';
        if (!strcmp(buff, "exit"))
        {
            exit(0);
            // return 0;
        }
        char *token;
        token = strtok(buff, " ><");
        int i = 0;
        char *arg[128];
        for (int i = 0; i < 128; i++)
        {
            arg[i] = NULL;
        }
        int flaintred = 0;
        int flaoutred = 0;
        char inoutred[256];
        // int flaintred=0;

        while (token != NULL)
        {
            if (flaintred || flaoutred)
            {
                strcpy(inoutred, token);
                break;
                // int fd=open(token,O_RDWR);
            }
            else if (!strcmp(token, ">"))
            {
                flaoutred = 1;
                // close(1);
            }
            else if (!strcmp(token, "<"))
            {
                flaintred = 1;
                // close(0);
            }
            else
            {
                arg[i] = token;
                i++;
            }
            token = strtok(NULL, " ");
        }
        // Upto here the parsing the input which is taken in string and point that tokens by the array of pointers .
        // From below the code which fork and pass that input which is pointed by the array of pointers to that tokens
        if (!strcmp(arg[0], "cd"))
        {
            // changing the directory cd command
            changedir(arg[1]);
            continue;
            // exit(0);
            // when I put this in the if(pid==0) condition then there is new process is created by checking the ps command
            // to avoid this directory  is changed before calling the fork()
        }

        char promtname[1024];
        strcpy(promtname, arg[0]);

        char *prtoken = strtok(promtname, "=");
        if (!strcmp(prtoken, "PS1"))
        {
            PsIntFlag = 1;
            prtoken = strtok(NULL, "\"");
            // printf("prmname:%s\n",prtoken);
            if (strcmp(prtoken, "\\w$"))
            {
                strcpy(prmname, prtoken);
            }
            else
            {
                getcwd(prmname, sizeof(prmname));
                strcat(prmname, "$ ");
            }
            continue;
        }

        if (!strcmp(prtoken, "PATH"))
        {
            int c1 = 0;
            prtoken = strtok(NULL, ":");
            // printf("prtoken:%s\n",prtoken);
            while (prtoken != NULL)
            {
                // printf("prtoken %s\n",prtoken);
                strcpy(path[c1], prtoken);
                prtoken = strtok(NULL, ":");
                c1++;
            }
            
            for (int i = 0; i < c1; i++)
            {
                strcat(path[i], "/");
                // printf("%s\n",path[i]);
            }
            for (c1; c1 < 10; c1++)
            {
                // printf("path[c1] :%s\n",path[c1]);
                strcpy(path[c1], "\0");
            }
            continue;
        }
        pid = fork();
        if (pid == 0)
        {
            char currpath[256];
            int errexec = 0;
            for (int i = 0; i < 9; i++)
            {
                char tmp[256];

                strcpy(tmp, path[i]);
                strcat(tmp, arg[0]);
                int fd=-1;
                if (flaintred || flaoutred)
                {
                    if (flaintred)
                    {
                        close(0);
                        fd = open(inoutred, O_RDWR);
                    }
                    else
                    {
                        // printf("output redirection with file:%s\n", inoutred);
                        close(1);
                        fd = open(inoutred, O_WRONLY | O_CREAT | O_TRUNC);
                        if(fd==-1){
                            perror("Can't open file");
                            exit(0);
                        }
                    }
                }
                
                errexec = execv(tmp, arg);
            }
            // if the execv fails due to executed file not found or other reason the search the command in below path
            // Handling errors
            if (errexec == -1)
            {
                // printf("%s: cannot access '%s': No such file or directory manja\n",tmp,tmp);
                perror("error");
                exit(0);//here exit is necessory to stop or kill the child 
            }
            // }
        }
        else
        {
            wait(0);
        }
    }
}