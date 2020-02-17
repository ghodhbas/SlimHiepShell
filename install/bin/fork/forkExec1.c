#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
  pid_t pid1, pid2;
  char *argv2[3];
  argv2[0] = "/home/ghodhbas/fork/giveme";
  argv2[1] = "pizza";
  argv2[2] = NULL;

 char* argv3[3];
  argv3[0] = "/home/ghodhbas/fork/giveme";
  argv3[1] = "cookies";
  argv3[2] = NULL;

  pid1 = fork();
  if (pid1 < 0) {
    fprintf(stderr,"%s fork error: %s\n", argv[0], strerror(errno));
    exit(1);
  }
 
  if (pid1==0){
   pid2= fork();
   if(pid2==0){
    execv(argv3[0], argv3);
    wait();    
   }
    execv(argv2[0], argv2);
  }
 
  /* Parent */
  printf("Parent completing successfully leaving child %d.\n", pid1);
  exit(0);
}

