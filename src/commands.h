#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "csapp.h"

enum state{RUNNING, STOPPED, ZOMBIE};

typedef struct Process
{
    pid_t pid;
    int jid;
    char command[128];
    enum state state;
}Process;

//VARS
Process shell;
Process foreground;
Process jobs[1000];
volatile int last_job_index ;
char* get_state(enum state s);


//BUILT IN COMMADS
int set_env_var(char **argv);
int list_jobs();
int bg(char** argv);
int fg();
void print_prompt();




//SIGNAL HANDLERS
void handler(int signal);
void wait_foreground(pid_t pid);





#endif