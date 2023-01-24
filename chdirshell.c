#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<unistd.h>
#include<sys/wait.h>
int main(){
    char usrname[102];
    getlogin_r(usrname,sizeof(usrname));
    printf("username is :%s\n",buff);
}