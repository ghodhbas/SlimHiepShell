#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include<signal.h> 

int nb =0;

void handler(int num){
       nb++;
}

int main(int argc, char *argv[]){
	signal(SIGINT, handler);

	struct sigaction sa;
	sa.sa_handler =  handler;
	sigaction(SIGINT, &sa, NULL);

       while(1){
	sleep(2);
      	 printf("Ctlr+c: %d\n",nb);
        fflush(stdout);
       }
}

