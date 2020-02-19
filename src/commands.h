#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "csapp.h"

//VARS
pid_t foreground;
pid_t jobs[1000];
volatile int last_job_index ;


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