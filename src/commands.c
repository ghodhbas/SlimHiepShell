#include "commands.h"

/* $begin set_env_var */
/*set_env_var - Add or deletes environment variables*/
int set_env_var(char **argv)
{
    char *var_name = strtok(argv[0], "=");
    //char* v = strtok(argv[0]+strlen(var_name)+1,"=");
    char *v = strtok(NULL, "=");

    if (v == NULL)
    {
        unsetenv(argv[0]);
    }
    else
    {

        setenv(var_name, v, 1);
    }

    return 1;
}
/* $end set_env_var */


int list_jobs(){
    for(int i=1; i<=last_job_index;i++){
        if(jobs[i].pid !=  0){
            printf("[%d] PID: %d    %s  %s\n", jobs[i].jid, jobs[i].pid, get_state(jobs[i].state), jobs[i].command);
        }
    }
    fflush(stdout);
    return 1;
}

int bg (char** argv){
    //repeat for every pid/jid supplied
    int i = 1;
    while(argv[i] !=NULL){
        //flag for jid
        int jid =0;
        pid_t pid=0;
        //check first char for % sign
        if(argv[i][0] == '%') jid =1;

        int p;
        if(jid){
            p = atoi(strtok(strtok(argv[i], "%"), ""));
        }else{
            p = atoi(argv[i]);
            //find jid
            for(int j =1 ; j<=last_job_index;j++){
                if(p == jobs[j].pid) p =j;
            } 
        }

        //p is jid here
        pid = jobs[p].pid;
        jobs[p].pid = 0;

        // if the job continued is the last index in the background processes

        if(p == last_job_index){
            do{
                last_job_index--;
            } while(jobs[last_job_index].pid==0);
        } 

        if(pid ==0){
            fprintf(stderr, "Error continuing child -- wrong pid/jid\n");
            fprintf(stderr, "Usage: bg <PID/JID>\n");
            fflush(stdout); 
            return 1;
        }

        //continue process in background
        kill(pid, SIGCONT);

        i++;
    }

    
    return 1;
}


int fg(){
    //foreground is shell
    volatile pid_t pid = jobs[last_job_index].pid;

    kill(pid, SIGCONT);
    tcsetpgrp(0, pid);
    //wait till end of process
    int status;
    waitpid(pid, &status, WUNTRACED);
    //wait_foreground(pid);

    jobs[last_job_index].pid=0;
    //correct indexing
    do{
        last_job_index--;
    } while(jobs[last_job_index].pid==0);

    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(0, getpid());
    signal(SIGTTOU, SIG_DFL);

    return 1;
}


void print_prompt(){
    if (getenv("lshprompt") == NULL)
    {
        printf("> ");
    }
    else
    {
        printf("%s> ", getenv("lshprompt"));
    }
}

/* $begin handler */
/*Handles SIGING*/
void handler(int sig)
{  
    printf("\n");
    switch(sig){
        case SIGINT:
            if(getpid()==foreground.pid) {
                print_prompt();
                break;
            }
            //negative pid so we can kill all processes that belong to the pgid of this pid
            Kill(-foreground.pid,SIGINT);
            foreground = shell;
            break;

        case SIGTSTP:
            if(getpid()==foreground.pid) {
                print_prompt();
                break;
            }else{
                Kill(-foreground.pid,SIGTSTP);
                foreground.state = STOPPED;
                last_job_index++;
                foreground.jid=last_job_index;
                jobs[last_job_index]= foreground;
                printf("[%d]+ Stopped\n",last_job_index);
                //shell becomes foreground
                foreground = shell;
                break;
            }
            break;
        default:
            break;
    }
    fflush(stdout);
    return;
}
/* $end handler */

void wait_foreground(pid_t pid){

    
    int status;
    int w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
    if (w == -1) { perror("waitpid"); exit(EXIT_FAILURE); }

    if (WIFEXITED(status)) {
        printf("exited, status=%d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("killed by signal %d\n", WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
        printf("stopped by signal %d\n", WSTOPSIG(status));
    } else if (WIFCONTINUED(status)) {
        printf("continued\n");
    }
}

char* get_state(enum state s){
    switch (s)
    {
    case 0:
        return "Running";
        break;
    case 1:
        return "Stopped";
        break;
    case 2:
        return "Zombie";
        break;
    default:
        return "Unknown";
        break;
    }
}