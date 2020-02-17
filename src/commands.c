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
            if(getpid()==foreground) {
                print_prompt();
                break;
            }
            //negative pid so we can kill all processes that belong to the pgid of this pid
            Kill(-foreground,SIGINT);
            foreground = getpid();
            break;

        case SIGTSTP:
            if(getpid()==foreground) {
                print_prompt();
                break;
            }else{
                Kill(-foreground,SIGTSTP);
                jid++;
                jobs[jid]= foreground;
                printf("[%d]+ Stopped\n", jid);
                //shell becomes foreground
                foreground = getpid();
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