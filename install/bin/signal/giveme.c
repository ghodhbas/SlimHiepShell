#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include<signal.h> 

volatile int shutDown = 0;

void handler(int signal){
   shutDown =1;

}



int main(int argc, char *argv[])
{
    if (argc != 2) { 
      fprintf(stderr, "usage: %s <string>\n", argv[0]);
      exit(1); 
    }
  // signal(SIGINT, handler);

   struct sigaction sa;
   sa.sa_handler =  handler;
   sigaction(SIGINT, &sa, NULL);
   sigaction(SIGQUIT, &sa, NULL);
   sigaction(SIGTERM, &sa, NULL);

    while (1) {
	if(shutDown){
	fclsoe(stdout);
	exit(1);
	 }
      fprintf(stdout,"I want %s!\n", argv[1]);
      sleep(1);
    }

    return 0;
}

