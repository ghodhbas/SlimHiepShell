#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "csapp.h"


//VARS
pid_t foreground;
pid_t jobs[1000];
int jid ;


//BUILT IN COMMADS
int set_env_var(char **argv);
void print_prompt();



//SIGNAL HANDLERS
void handler(int signal);




#endif