/* $begin shellmain */
#include "csapp.h"
#include "commands.h"
#define MAXARGS 128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void exec_command(char* cmd);
void exec_pipe_command(char* cmd, int pipefd[],int j , int num_pipes, int pids[]);

int main()
{
    char cmdline[MAXLINE]; /* Command line */
    //set promt name./
    putenv("lshprompt=lsh");
    //set signal handlers
    last_job_index =0;
    for(int i =0 ; i<100;i++){
        jobs[i].pid=0;
    }
    foreground.pid =  getpid();
    strcpy(foreground.command, "./shellex");
    foreground.state = RUNNING;
    foreground.jid =0;
    shell = foreground;

    signal(SIGINT, handler);
    signal(SIGTSTP, handler);

    while (1)
    {
        print_prompt();
        fflush(stdout);
        /* Read */

        Fgets(cmdline, MAXLINE, stdin);
        if (feof(stdin))
            exit(0);

        /* Evaluate */
        eval(cmdline);
        
        // Reap zombies
        for(int i=0; i<=last_job_index; i++){
            int status;
            if(jobs[i].pid !=  0){
                int w = waitpid(jobs[i].pid, &status,  WNOHANG);
                if (w == -1) { perror("waitpid"); exit(EXIT_FAILURE); }

                if (w>0){
                    jobs[i].pid = 0;
                }
            }
        }
        while (jobs[last_job_index].pid == 0 && last_job_index > 0)
        {
            last_job_index--;
        }
    }
}
/* $end shellmain */

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline)
{
    //split cmdline into commands with delim |/
    char* commands [1000];
    char* cmd;
    int i =-1;
    cmd=strtok(cmdline, "|");
    while ( cmd != NULL   ){
        i++;
        //commands[i] = malloc(sizeof(char*) * 1000);
        commands[i] = cmd; 
        cmd=strtok(NULL, "|");
    }


    if(i == 0){
        exec_command(commands[i]);
    }else{
        //pipe setup
        int nb_pipes = i;
        int pipefd[2*nb_pipes];
        //init pipes
        for(int p=0;p<nb_pipes; p++){
            if(pipe(pipefd+ 2*p)<0){
                perror("Pipe Creation Error");
                exit(EXIT_FAILURE);
            }
        }

        int j =0;
        int pids[i+1];
        while(j<=i){
            exec_pipe_command(commands[j], pipefd, j , nb_pipes, pids);
            j++;
        }
        
        //close all pipes       
        for(int p=0;p<nb_pipes*2; p++){
            close(pipefd[p]);
        }
        //wait for children
        for(int p=0;p<i+1; p++){
            fprintf(stderr,"waiting for pid %d\n",pids[p]);
            wait_foreground(pids[p]);
            
        }

    }
    //for(int p=0;p<i+1; p++){
    //    //free(commands[p]);
    //}
    
    //foreground is shell
    foreground = shell;

    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv)
{
    if (!strcmp(argv[0], "quit")) /* quit command */
        exit(0);
    if (!strcmp(argv[0], "&")) /* Ignore singleton & */
        return 1;

    //replace with environment cariable path
    int i=0;
    while(argv[i]!=NULL){
        if(argv[i][0]=='$'){
            argv[i]= getenv(argv[i]+1);
        }
        i++;
    }

    //Adding and deleting environment variables
    if (strchr(argv[0], '=') != NULL)
        return set_env_var(argv);

    //list background jobs
    if (!strcmp(argv[0], "jobs")) /* Ignore singleton & */
        return list_jobs();

    //run bg
    if (!strcmp(argv[0], "bg")) /* Ignore singleton & */
        return bg(argv);

    //run fg
    if (!strcmp(argv[0], "fg")){
        int pid;
        if((pid = fork() == 0)){
            printf("PID is %d\n", getpid());
            exit(0);
        }
        return fg(argv);
    }

    

    return 0; /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv)
{
    char *delim; /* Points to first space delimiter */
    int argc;    /* Number of args */
    int bg;      /* Background job? */

    buf[strlen(buf) - 1] = ' ';   /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' ')))
    {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;

    if (argc == 0) /* Ignore blank line */
        return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0)
        argv[--argc] = NULL;

    return bg;
}
/* $end parseline */


void exec_command(char* cmd){
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    strcpy(buf, cmd);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
        return; /* Ignore empty lines */

    if (!builtin_command(argv))
    {
        if ((pid = Fork()) == 0)
        { /* Child runs user job */
            //update pgid
            Setpgid(getpid(),getpid());
            if (execvp(argv[0], argv) < 0)
            {   
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }


        Process p;
        p.pid =pid;
        strcpy(p.command , cmd);
        p.state = RUNNING;


        /* Parent waits for foreground job to terminate */
        if (!bg)
        {    
            foreground = p;
            //foreground is child
            wait_foreground(p.pid);
        }
        else{
            last_job_index++;
            p.jid=last_job_index;
            jobs[last_job_index]= p;
            printf("%d %s", pid, cmd);
        } 
    }
}

void exec_pipe_command(char* cmd, int pipefd[], int j , int num_pipes, int pids[]){
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    pid_t pid;           /* Process id */

    strcpy(buf, cmd);
    parseline(buf, argv);
    if (argv[0] == NULL)
        return; /* Ignore empty lines */

    if (!builtin_command(argv))
    {
        if ((pid = Fork()) == 0)
        { /* Child runs user job */
            
            //get input from prev command read part
            if(j!=0) dup2(pipefd[(j-1)*2], 0);
            //output goes to next command
            if(j != num_pipes) dup2(pipefd[j*2+1], 1);
            //close all pipes
            for(int p=0;p< num_pipes*2; p++){
                close(pipefd[p]);
            }
            //update pgid
            if(j==0)Setpgid(getpid(),getpid());
            fprintf(stderr,"Starting command %s with pud %d\n", argv[0], getpid());
            if (execvp(argv[0], argv) < 0)
            {   
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }

        pids[j]=pid;
    }

}