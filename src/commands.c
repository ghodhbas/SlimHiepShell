#include "commands.h"

/* $begin set_env_var */
/*set_env_var - Add or deletes environment variables*/
int set_env_var(char **argv)
{
    char *var_name = strtok(argv[0], "=");
    char *v = strtok(NULL, "=");

    if (v == NULL)
        unsetenv(argv[0]);
    else
        setenv(var_name, v, 1);

    return 1;
}
/* $end set_env_var */

int list_jobs(){
    for(int i=1; i<=last_job_index;i++){
        if(jobs[i].pid !=  0){
            printf("[%d] PID: %d    %s  %s\n", jobs[i].jid, jobs[i].pid, get_state(jobs[i].state), trim(jobs[i].command));
        }
    }
    fflush(stdout);
    return 1;
}

int jsum(){
    printf("PID  | Status |   Time   |  Min  |  Maj  | Command\n");
    for(int i=0; i<entry_count;i++){
        Process p = history[i];
        pid_t pid = p.pid;
        char buff[20];
        time_t elapsed = p.endTime - p.startTime;
        strftime(buff, 20, "%H:%M:%S", gmtime(&elapsed));

        int len = strlen(p.command);
        if(p.command[len-1]=='\n') p.command[len-1]='\0';

        printf("%-7d %-7s %-11s %-7ld %-7ld %s\n", pid, get_status(p.stat), buff, p.min, p.maj, trim(p.command));
    }
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
            p = atoi(argv[1]);
            int found = 0;
            //find jid
            for(int j =1 ; j<=last_job_index;j++){
                if(p == jobs[j].pid ){
                    p =j;
                    found =1;
                }
            } 
            if(!found){
                printf("PID not found\n");
                fflush(stdout);
                return 1 ;
            }
        }

        //p is jid here
        pid = jobs[p].pid;
        jobs[p].state = RUNNING;

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


int fg(char **argv){

    int jid =0;
    int p;

    if (argv[1] != NULL){
        if(argv[1][0] == '%') jid =1;

        if(jid){
            p = atoi(strtok(strtok(argv[1], "%"), ""));
        }else{
            p = atoi(argv[1]);
            int found = 0;
            //find jid
            for(int j =1 ; j<=last_job_index;j++){
                if(p == jobs[j].pid ){
                    p =j;
                    found =1;
                }
            } 
            if(!found){
                printf("PID not found\n");
                fflush(stdout);
                return 1 ;
            }
        }
    } else {
        p = last_job_index;
    }

    //p is jid here
    volatile pid_t pid = jobs[p].pid;
    foreground = jobs[p];

    kill(-pid, SIGCONT);
    tcsetpgrp(shell.pid, pid);

    jobs[p].pid=0;
    //correct indexing
    while (jobs[last_job_index].pid == 0 && last_job_index > 0)
    {
        last_job_index--;
    }

    // Store elapsed info
    
    int status;
    struct rusage usage;
    wait4(pid, &status, WUNTRACED, &usage);
    Process temp = jobs[p];
    temp.pid = pid;
    temp.endTime = time(NULL);
    temp.min = usage.ru_minflt;
    temp.maj = usage.ru_majflt;

    if (WIFEXITED(status)){
        temp.stat = OK;
        history[entry_count++] = temp;
    } else if (WIFSIGNALED(status)) {
        temp.stat = ABORT;
        history[entry_count++] = temp;
    }

    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(shell.pid, getpid());
    signal(SIGTTOU, SIG_DFL);
    fflush(stdout);
    return 1;
}


void print_prompt(){
    if (getenv("lshprompt") == NULL)
        printf("> ");
    else
        printf("%s> ", getenv("lshprompt"));

    fflush(stdout);
}

/* $begin handler */
/*Handles SIGING*/
void handler(int sig)
{  
    printf("\n");
    switch(sig){
        case SIGINT:
            if(shell.pid==foreground.pid) {
                print_prompt();
                break;
            }
            //negative pid so we can kill all processes that belong to the pgid of this pid
            Kill(-foreground.pid,SIGINT);
            foreground = shell;
            break;

        case SIGTSTP:
            if(shell.pid==foreground.pid) {
                print_prompt();
                break;
            }else{
                Kill(-foreground.pid,SIGTSTP);
                foreground.state = STOPPED;
                int i =1;
                while( i <= last_job_index+1)
                {
                    if(jobs[i].pid == 0) {
                        foreground.jid = i;
                        jobs[i]=foreground;
                        break;
                    }
                    i++;
                }

                if(i == last_job_index+1){
                    last_job_index = i;
                }
                
                printf("[%d]+ Stopped\n",i);
                //shell becomes foreground
                foreground = shell;
                break;
            }
            break;
        default:
            printf("Signal caught %d\n", sig);
            break;
    }
    fflush(stdout);
    return;
}
/* $end handler */

void wait_process(Process p){

    int status;
    struct rusage usage;
    int w = wait4(p.pid, &status, WUNTRACED | WCONTINUED, &usage);
    p.endTime = time(NULL);
    p.min = usage.ru_minflt;
    p.maj = usage.ru_majflt;

    if (w == -1) { 
        perror("waitpid");
        exit(EXIT_FAILURE); 
    }

    if (WIFEXITED(status)) {
        p.stat = OK;
        history[entry_count++] = p;
    } else if (WIFSIGNALED(status)) {
        p.stat = ABORT;    
        history[entry_count++] = p;
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

char* get_status(enum end_status s){
    switch (s)
    {
    case 0:
        return "ok";
        break;
    case 1:
        return "abort";
        break;
    case 2:
        return "error";
        break;
    default:
        return "Unknown";
        break;
    }
}

char *trim(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}