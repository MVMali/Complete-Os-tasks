#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include<stdlib.h>
#include<signal.h>
#include<stdbool.h>

//declaring global 
char prmname[1024]="Tabaki>>";


char *path[10] = {"/usr/local/sbin/", "/usr/local/bin/", "/usr/sbin/", "/usr/bin/", "/bin/", "/usr/games/", "/usr/local/games/", "/snap/bin/", "/snap/bin/"};


int changedir(char *path)
{
    int succ;
    char buff[256];
    succ = chdir(path);
    if (succ == -1)
    {
        perror("Error has occured");
        return -1;
    }
}

int main()
{
    int pid;
    // getcwd(prmname,sizeof(prmname));
    // strcat(prmname,"$ ");
    while (1)
    {
        
        char buff[128];
        // printf("prompt>>");
        // getcwd(prmname,sizeof(prmname));
        // strcat(prmname,"$ ");
        printf("%s",prmname);
        // use of gets to get the input string which contains space.
        gets(buff);
        if(buff==NULL){
            // continue;
            // break;
            exit(0);
        }
        char *token;
        token = strtok(buff, " ");
        int i = 0;
        char *arg[128];
        for (int i = 0; i < 128; i++)
        {
            arg[i] = NULL;
        }
        while (token != NULL)
        {
            arg[i] = token;
            token = strtok(NULL, " ");
            i++;
        }
        // Upto here the parsing the input which is taken in string and point that tokens by the array of pointers .
        // From below the code which fork and pass that input which is pointed by the array of pointers to that tokens

        pid = fork();

        if (pid == 0)
        {
            char currpath[256];
            // char *path[10] = {"/usr/local/sbin/", "/usr/local/bin/", "/usr/sbin/", "/usr/bin/", "/bin/", "/usr/games/", "/usr/local/games/", "/snap/bin/", "/snap/bin/", getcwd(currpath, sizeof(currpath))};
            path[9]=getcwd(currpath, sizeof(currpath));
            int errexec = 0;
            if(!strcmp(arg[0],"exit")){
                exit(0);
            }


            //Here is the code for the PS1 commands.
            char promtname[1024];
            strcpy(promtname,arg[0]);
            
            char* prtoken=strtok(promtname,"=");
            if(promtname[0]=='P' && promtname[1]=='S' && promtname[2]=='1'){
                prtoken=strtok(NULL,"\"");
                strcpy(prmname,prtoken);
            }

            else if(!strcmp(prtoken,"PATH")){
                int m1=0;
                prtoken=strtok(NULL,":");
                while(m1<10){
                    path[m1]=NULL;
                    m1++;
                }
                m1=0;
                while(prtoken!=NULL){
                    char temp[1024];
                    strcpy(temp,prtoken);
                    strcat(temp,"/");
                    printf("temp is:%s\n",temp);
                    path[m1]=temp;
                    printf("newpath:%s\n",path[m1]);
                    m1=m1+1;
                    prtoken=strtok(NULL,":");
                }
                m1=0;
                while(m1<10){
                    // path[m1]=NULL;
                    printf("kyapath:%s\n",path[m1]);
                    m1++;
                }

                printf("Your are in right direction\n");
            }
            
            else if (!strcmp(arg[0], "cd"))
            {
                // changing the directory cd command
                changedir(arg[1]);
            }
            else
            {
                for (int i = 0; i < 10; i++)
                {
                    char tmp[256];
                    strcpy(tmp,path[i]);
                    strcat(tmp, arg[0]);
                    // printf("path is:%s\n", tmp);
                    errexec = execv(tmp, arg);
                }
                // if the execv fails due to executed file not found or other reason the search the command in below path

                // Handling errors
                if (errexec == -1)
                {
                    // printf("%s: cannot access '%s': No such file or directory manja\n",tmp,tmp);
                    perror("error");
                }
            }
        }
        else
        {
            wait(0);
        }
    }
}