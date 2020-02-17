#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

int alive = 0;

void signalHandler(int sig)
{
  alive--;
  return;
}

int main(int argc, char *argv[])
{
  pid_t pid;
  char *argv2[3];
  argv2[0] = "/home/ghodhbas/fork/giveme";
  argv2[1] = "pizza";
  argv2[2] = NULL;
  char *filename[10] = {"pizza", "cookies", "coffee", "candy", "chocolate",
			"spinach", "cake", "apples", "bread", "wine"};

  if (signal(SIGCHLD, signalHandler) == SIG_ERR) {
    fprintf(stderr,"%s signal error: %s", argv[0], strerror(errno));
    exit(0);
  }

  for (int i = 0; i<10; i++) {
    pid = fork();
    if (pid < 0) {
      fprintf(stderr,"%s fork error creating child %d: %s\n",
	      argv[0], i, strerror(errno));
      exit(1);
    }
    if (pid == 0) {              /* Child */
      int fd = open(filename[i], O_WRONLY|O_CREAT, S_IRUSR | S_IWUSR);
      dup2(fd,1);                /* Copy fd to stdout fd*/
      close(fd);
      argv2[1] = filename[i];    /* Filename is also the food item */  
      execv(argv2[0], argv2);
    }
    int tmp = alive;
    sleep(1);
    alive = tmp + 1;
    printf("Child %d created\n",pid);
    printf("Alive: %d\n",alive);
    sleep(1);
}

  while (alive > 0) {
    sleep(1);
    printf("Alive: %d\n",alive);
  }
  printf("Parent completing successfully\n");
  exit(0);
 }
