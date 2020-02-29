#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "csapp.h"

enum state{RUNNING, STOPPED, ZOMBIE};
enum end_status{OK, ABORT, ERROR};

typedef struct Process
{
    pid_t pid;
    int jid;
    char command[128];
    enum state state;

    enum end_status stat;
    time_t startTime;
    time_t endTime;

    long min;
    long maj;
}Process;

//VARS
Process shell;
Process foreground;
Process jobs[1000];
volatile int last_job_index ;

Process history[1000];
volatile int entry_count;

char* get_state(enum state s);
char* get_status(enum end_status s);

char* trim(char *str);

//BUILT IN COMMADS
int set_env_var(char **argv);
int list_jobs();
int bg(char** argv);
int fg(char **argv);
void print_prompt();

int jsum();

//SIGNAL HANDLERS
void handler(int signal);
void wait_process(Process p);

#endif