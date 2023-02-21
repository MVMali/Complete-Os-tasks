#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>
#include <termios.h>
#include <assert.h>
#include <setjmp.h>
// Mohan Mali 112003081 Div 1
//  declaring global
int PsIntFlag = 0;

char prmname[1024];

char path[11][1024] = {"/usr/local/sbin/", "/usr/local/bin/", "/usr/sbin/", "/usr/bin/", "/bin/", "/usr/games/", "/usr/local/games/", "/snap/bin/", "/snap/bin/", "/usr/local/lib/nodejs/node-v18.13.0-linux-x64/bin/"};


static sigjmp_buf env;	
int kill_pid;			
int bg[64];			
char background_cmds[64][128];	
int numOfbgProcess = 0;
pid_t main_parent_id;			
char t_cmd[128];		
int check_bg = 0;
static volatile sig_atomic_t act_process = 0;	


typedef struct History_Data
{
    int Num;
    char CmdName[1024];
} History_Data;

#define MAX_HISTORY_COUNT 2000
History_Data *History[MAX_HISTORY_COUNT];
int History_Track = 0;

typedef struct job {
    pid_t pid;
    int jobid;
    char status[100];
}job;

int MAX_JOBS=1024;
job *jobs[1024];
int current_jid = 0;



void HandleInt(int signum) {
	if (!act_process) {
        return;
    }
	if(check_bg == 1) {
		return;
	}
	siglongjmp(env, 73);

}




void HandleStopInt(int signum) {
	if(check_bg == 1) {
		return;
	}
	if(kill_pid == main_parent_id || kill_pid == bg[numOfbgProcess-1]) {
		fprintf(stdout, "\n");
        printf("%s",prmname);
        fflush(stdout);
		return;
	}
	bg[numOfbgProcess] = kill_pid;
	strcpy(background_cmds[numOfbgProcess], t_cmd);
	numOfbgProcess++;
	fprintf(stdout, "\n[%d]+:	Stopped		Pid: %d\n", numOfbgProcess, kill_pid);
    jobs[current_jid]=(job*)malloc(sizeof(job));
    jobs[current_jid]->pid=kill_pid;
    jobs[current_jid]->jobid=numOfbgProcess;
    strcpy(jobs[current_jid]->status,"stopped");
    current_jid++;
    // strcpy(jobs[current_jid]->cmd);
	if(!act_process) {
        return;
    }
	siglongjmp(env, 73);

}


//jobs
void printJobs(){
    for(int i=0;i<current_jid;i++){
        printf("[%d]  %s          %d\n",jobs[i]->jobid,jobs[i]->status,jobs[i]->pid);
    }
    return;
}






void change_bg(int process) {
	strcpy(background_cmds[process], "");
	bg[process] = -1;
    if(current_jid!=0){
        current_jid--;
        jobs[current_jid]=NULL;
    }

}


int HandleDir(char *path)
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

void executeCmd(char *arg[], int flaintred, int flaoutred, char inoutred[])
{
    // printf("Size of path is :%d \n",sizeof(path)/sizeof(path[0]));
    getcwd(path[10], sizeof(path[10]));
    strcat(path[10], "/");
    long int sizeOfarr = sizeof(path) / sizeof(path[0]);
    char currpath[256];
    int errexec = 0;
    for (int i = 0; i < sizeOfarr; i++)
    {
        char tmp[256];

        strcpy(tmp, path[i]);
        strcat(tmp, arg[0]);
        int fd = -1;
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
            }
            if (fd == -1)
            {
                perror("Can't open file");
                exit(0);
            }
        }
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        errexec = execv(tmp, arg);
    }
    if (errexec == -1)
    {
        // printf("%s: cannot access '%s': No such file or directory manja\n",tmp,tmp);
        perror("error");
        exit(0); // here exit is necessory to stop or kill the child
    }
}

void HandlePath(char *prtoken)
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
}

void HandlePs1(char *prtoken)
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
}
int NumOfPipes(char buff[])
{
    int i = 0, count = 0;
    while (buff[i] != '\0')
    {
        if (buff[i] == '|')
        {
            count++;
        }
        i++;
    }
    return count;
}

void HandleMultiplePipes(char buff[])
{
    char *token;
    int t1 = 0;
    char *arg[128];
    int NumOfpipes = NumOfPipes(buff);
    // printf("Number of pipes %d\n", NumOfpipes);
    int i = 0;
    int j = 0;
    pid_t pid;
    int cmd_len = NumOfpipes + 1;
    int fd[2 * cmd_len];
    for (i = 0; i < cmd_len; i++)
    {
        if (pipe(fd + i * 2) < 0)
        {
            perror("couldn't pipe");
            exit(EXIT_FAILURE);
        }
    }

    token = strtok(buff, " ");
    // NumOfpipes++;
    int lplength = NumOfpipes + 1;
    while (lplength--)
    {
        for (int i = 0; i < 128; i++)
        {
            arg[i] = NULL;
        }
        t1 = 0;
        int flred = 0;
        if (!strcmp(token, "|"))
        {
            flred = 1;
            token = strtok(NULL, " ");
        }
        while (token && strcmp(token, "|"))
        {
            arg[t1] = token;
            t1++;
            token = strtok(NULL, " ");
        }

        if ((pid = fork()) == -1)
        {
            perror("fork");
            exit(1);
        }

        else if (pid == 0)
        {
            // if there is next
            // if (arg[t1+1]!= NULL)
            signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
            if (lplength)
            {
                if (dup2(fd[j + 1], 1) < 0)
                {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            // if there is previous
            if (j != 0)
            {
                if (dup2(fd[j - 2], 0) < 0)
                {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            for (i = 0; i < 2 * cmd_len; i++)
            {
                close(fd[i]);
            }

            executeCmd(arg, 0, 0, "Nofile");
        }

        j += 2;
    }

    // close fds in parent process
    for (i = 0; i < 2 * cmd_len; i++)
    {
        close(fd[i]);
    }
    // wait for children
    for (i = 0; i < cmd_len; i++)
        wait(NULL);
}

void bg_handle(int process) {
	
	if(process >= numOfbgProcess) {
		printf("Process not exists\n");
		siglongjmp(env,73);

	}
	if(bg[process] == -1 ) {
		printf("fg: %d :There is no process\n", process);
		siglongjmp(env,73);
	}
	printf("\n%s\n",background_cmds[process]);
	kill(bg[process],SIGCONT);
	change_bg(process);
	siglongjmp(env,73);

}


void fg(int process) {
	if(process > numOfbgProcess) {
		printf("Background Process doesn't exist\n");
		siglongjmp(env,73);

	}
	if(bg[process] == -1 ) {
		printf("fg: %d :No such job\n", process);
		siglongjmp(env,73);
	}
	printf("\n%s\n",background_cmds[process]);
	kill(bg[process],SIGCONT);
	change_bg(process);
	siglongjmp(env,73);
}


void Add_To_History(char Hist[])
{
    if (History_Track >= MAX_HISTORY_COUNT)
    {
        return;
    }
    if (strlen(Hist) == 0)
    {
        return;
    }
    History[History_Track] = (History_Data *)malloc(sizeof(History_Data));
    if (!History[History_Track])
    {
        printf("Unable to Malloc for History\n");
        return;
    }
    History[History_Track]->Num = History_Track + 1;
    strcpy(History[History_Track]->CmdName, Hist);
    History_Track++;
    return;
}

void Print_History()
{
    for (int i = 0; i < History_Track; i++)
    {
        printf("%d %s", History[i]->Num, History[i]->CmdName);
    }
    printf("\n");
}

void Write_Hist_to_file()
{
    // writing into the history.txt after the user exists the terminal
    int fd;
    fd = open("history.txt", O_WRONLY | O_CREAT | O_APPEND);
    for (int i = 0; i < History_Track; i++)
    {
        write(fd, History[i]->CmdName, strlen(History[i]->CmdName));
    }
    close(fd);
}

int main()
{
    int pid;
    // Signal Handling
    main_parent_id = getpid();
    signal(SIGINT, HandleInt);
	signal(SIGTSTP, HandleStopInt);
    while (1)
    {
        kill_pid = main_parent_id;
		check_bg = 0;
        int argcount = 0;
		if (sigsetjmp(env, 1) == 73) {
            printf("\n");
        }
		act_process = 1;

		
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
        int len=strlen(buff);
        if(buff[0]=='\n'){
            // break;
            continue;
        }
        // Add to History
        Add_To_History(buff);
        
        buff[strlen(buff) - 1] = '\0';

        if (!terminate || !strcmp(buff, "exit"))
        {
            Write_Hist_to_file();
            exit(0);
        }
        int t1 = 0;
        while (buff[t1] != '\0')
        {
            if (buff[t1] == '|')
            {
                HandleMultiplePipes(buff);
                t1 = -1;
                break;
            }
            t1++;
        }
        if (t1 == -1)
        {
            continue;
        }

        char *token;
        token = strtok(buff, " ><");

        if (!strcmp(token, "history"))
        {
            token = strtok(NULL, " ");
            if (token)
            {
                int x = atoi(token);
                if (!strcmp(token, "-c"))
                {
                    for (int i = 0; i < History_Track; i++)
                    {
                        free(History[i]);
                    }
                    History_Track = 0;
                }
                else if (x)
                {
                    for (int i = History_Track - x; i < History_Track; i++)
                    {
                        if (i < 0)
                        {
                            i = 0;
                        }
                        printf("%d %s", History[i]->Num, History[i]->CmdName);
                    }
                }
            }
            else
            {
                Print_History();
            }
            continue;
        }
        int i = 0;

        char *arg[128];
        for (int i = 0; i < 128; i++)
        {
            arg[i] = NULL;
        }

        int flaintred = 0;
        int flaoutred = 0;
        char inoutred[1024];

        while (token != NULL)
        {
            if (flaintred || flaoutred)
            {
                strcpy(inoutred, token);
                break;
            }
            else if (!strcmp(token, ">"))
            {
                flaoutred = 1;
            }
            else if (!strcmp(token, "<"))
            {
                flaintred = 1;
            }
            else
            {
                arg[i] = token;
                i++;
                argcount++;
            }
            token = strtok(NULL, " ");

        }
		if(strcmp(arg[0], "bg") == 0) {
			check_bg = 1;
			if(numOfbgProcess != 0) {
				if(argcount >= 2) {
					bg_handle(atoi(arg[1]));
				}
				bg_handle(numOfbgProcess-1);
			}
			else {
				printf("Nothing in the background\n");
			}
			continue;
		}

        if(strcmp(arg[0], "fg") == 0) {
			if(numOfbgProcess != 0) {
				if(argcount >= 2) {
					fg(atoi(arg[1]));
				}
				fg(numOfbgProcess-1);
			}
			else {
				printf("Nothing in the background\n");
			}
			
			continue;
		}

        if(strcmp(arg[0],"jobs")==0){
            printJobs();
            continue;
        }
        // Upto here the parsing the input which is taken in string and point that tokens by the array of pointers .
        // From below the code which fork and pass that input which is pointed by the array of pointers to that tokens
        if (!strcmp(arg[0], "cd"))
        {
            // changing the directory cd command
            HandleDir(arg[1]);
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
            HandlePs1(prtoken);
            continue;
        }
        if (!strcmp(prtoken, "PATH"))
        {
            HandlePath(prtoken);
            continue;
        }

        pid = fork();
        kill_pid = pid;
        if (pid == -1)
        {
            perror("Error occured");
            exit(0);
        }

        if (pid == 0)
        {
            
            signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
            executeCmd(arg, flaintred, flaoutred, inoutred);
        }
        else
        {
            waitpid(pid,0, WUNTRACED);
            // wait for child to be terminated otherwise child will be in zombie state.
            
        }
    }
    exit(EXIT_SUCCESS);
    return 0;
}
