/* $begin shellmain */
#include "csapp.h"
#include "commands.h"
#define MAXARGS 128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void exec_command(char* cmd);
void exec_pipe_command(char* cmd, int pipefd[]);

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
        
    }
}
/* $end shellmain */

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline)
{
    //split cmdline into commands/
    char* commands [100];
    char* cmd;
    int i =-1;
    cmd=strtok(cmdline, "|");
    while ( cmd != NULL   ){
        i++;
        commands[i] = cmd; 
        cmd=strtok(NULL, "|");
    }

    if(i == 0){
        exec_command(commands[i]);
    }else{
        //pipes
        int pipefd[2];
        int ret = pipe(pipefd);
        char readbuf[1000];
        int input_size =0;

        if(ret == -1){
            perror("pipe error");
            exit(1);
        } 


        int j =0;
        while(j<=i){

            fprintf(stderr,"command: %s\n", commands[j]);
            fflush(stderr);
            exec_pipe_command(commands[j], pipefd);
            
            input_size = read(pipefd[0],readbuf, sizeof(readbuf));
            readbuf[input_size]='\0';
            //save in tmp file
            fprintf(stderr,"Read buffer: %s\n", readbuf);
            fflush(stderr);

            j++;
        }
        //reset stdin and out
        dup(1); //stdout
        dup(0); //stdin
       
    }

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
    if (!strcmp(argv[0], "fg")) /* Ignore singleton & */
        return fg(argv);

    

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

void exec_pipe_command(char* cmd, int pipefd[]){
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
            
            close(pipefd[0]);
            dup2(pipefd[1],1);
            //update pgid
            Setpgid(getpid(),getpid());
            if (execvp(argv[0], argv) < 0)
            {   
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }


        close(pipefd[1]);
        dup2(pipefd[0],0);
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