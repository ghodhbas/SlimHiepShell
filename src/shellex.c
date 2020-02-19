/* $begin shellmain */
#include "csapp.h"
#include "commands.h"
#define MAXARGS 128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

int main()
{
    char cmdline[MAXLINE]; /* Command line */
    //set promt name
    putenv("lshprompt=lsh");
    //set signal handlers
    last_job_index =0;
    for(int i =0 ; i<100;i++){
        jobs[i]=0;
    }
    foreground=getpid();
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
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    strcpy(buf, cmdline);
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

        /* Parent waits for foreground job to terminate */
        if (!bg)
        {      
            //foreground is child
            wait_foreground(pid);

        }
        else printf("%d %s", pid, cmdline);
    }
    //foreground is shell

    foreground=getpid();

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
        return fg();


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
