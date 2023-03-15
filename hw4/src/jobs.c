#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <wait.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>


#include "mush.h"
#include "debug.h"
#include "jdata.h"
#include "sdata.h"
#include "data.h"




int JOBCNT = 0;

JOB JOBTABLE[MAX_NUM_JOB];

sigset_t s, os;

FILE *stream = NULL;
char *capbuf = NULL;

int outcapReadReady = -1;

int ocpipefd[2] = {0,0};
/*
 * This is the "jobs" module for Mush.
 * It maintains a table of jobs in various stages of execution, and it
 * provides functions for manipulating jobs.
 * Each job contains a pipeline, which is used to initialize the processes,
 * pipelines, and redirections that make up the job.
 * Each job has a job ID, which is an integer value that is used to identify
 * that job when calling the various job manipulation functions.
 *
 * At any given time, a job will have one of the following status values:
 * "new", "running", "completed", "aborted", "canceled".
 * A newly created job starts out in with status "new".
 * It changes to status "running" when the processes that make up the pipeline
 * for that job have been created.
 * A running job becomes "completed" at such time as all the processes in its
 * pipeline have terminated successfully.
 * A running job becomes "aborted" if the last process in its pipeline terminates
 * with a signal that is not the result of the pipeline having been canceled.
 * A running job becomes "canceled" if the jobs_cancel() function was called
 * to cancel it and in addition the last process in the pipeline subsequently
 * terminated with signal SIGKILL.
 *
 * In general, there will be other state information stored for each job,
 * as required by the implementation of the various functions in this module.
 */

/**
 * @brief  Initialize the jobs module.
 * @details  This function is used to initialize the jobs module.
 * It must be called exactly once, before any other functions of this
 * module are called.
 *
 * @return 0 if initialization is successful, otherwise -1.
 */
int jobs_init(void) {

    for(int i = 0; i < MAX_NUM_JOB; i++)
    {
        JOB newJob = {.id = -1, .pgid = 0, .status = EMPTY, .pline = NULL, .exitStatus = -1};
        JOBTABLE[i] = newJob;
    }

    sigemptyset(&s);
    sigaddset(&s, SIGCHLD);
    return 0;
}

/**
 * @brief  Finalize the jobs module.
 * @details  This function is used to finalize the jobs module.
 * It must be called exactly once when job processing is to be terminated,
 * before the program exits.  It should cancel all jobs that have not
 * yet terminated, wait for jobs that have been cancelled to terminate,
 * and then expunge all jobs before returning.
 *
 * @return 0 if finalization is completely successful, otherwise -1.
 */
int jobs_fini(void) {
    for(int i = 0; i < MAX_NUM_JOB; i++)
    {
        if(JOBTABLE[i].pline != NULL)
        {
            clear_Job(JOBTABLE[i].id);
        }
    }

    clear_program();
    clear_store();
    return 0;
}

/**
 * @brief  Print the current jobs table.
 * @details  This function is used to print the current contents of the jobs
 * table to a specified output stream.  The output should consist of one line
 * per existing job.  Each line should have the following format:
 *
 *    <jobid>\t<pgid>\t<status>\t<pipeline>
 *
 * where <jobid> is the numeric job ID of the job, <status> is one of the
 * following strings: "new", "running", "completed", "aborted", or "canceled",
 * and <pipeline> is the job's pipeline, as printed by function show_pipeline()
 * in the syntax module.  The \t stand for TAB characters.
 *
 * @param file  The output stream to which the job table is to be printed.
 * @return 0  If the jobs table was successfully printed, -1 otherwise.
 */
int jobs_show(FILE *file) {

    for(int i = 0; i < MAX_NUM_JOB; i++)
    {
        if(JOBTABLE[i].status != EMPTY)
        {
            char * status = toStr_status(JOBTABLE[i].status);
            printf("%d\t%d\t%s\t",JOBTABLE[i].id, JOBTABLE[i].pgid, status);
            fflush(stdout);
            show_pipeline(file, JOBTABLE[i].pline);
            printf("\n");
        }
    }


    return 0;
}

char* toStr_status(JOB_STATUS status)
{
    if(status == NEW)
    {
        return "new";
    }
    if(status == RUNNING)
    {
        return "running";
    }
    if(status == COMPLETED)
    {
        return "completed";
    }
    if(status == CANCELED)
    {
        return "canceled";
    }
    if(status == ABORTED)
    {
        return "aborted";
    }
    return "";
}

/**
 * @brief  Create a new job to run a pipeline.
 * @details  This function creates a new job and starts it running a specified
 * pipeline.  The pipeline will consist of a "leader" process, which is the direct
 * child of the process that calls this function, plus one child of the leader
 * process to run each command in the pipeline.  All processes in the pipeline
 * should have a process group ID that is equal to the process ID of the leader.
 * The leader process should wait for all of its children to terminate before
 * terminating itself.  The leader should return the exit status of the process
 * running the last command in the pipeline as its own exit status, if that
 * process terminated normally.  If the last process terminated with a signal,
 * then the leader should terminate via SIGABRT.
 *
 * If the "capture_output" flag is set for the pipeline, then the standard output
 * of the last process in the pipeline should be redirected to be the same as
 * the standard output of the pipeline leader, and this output should go via a
 * pipe to the main Mush process, where it should be read and saved in the data
 * store as the value of a variable, as described in the assignment handout.
 * If "capture_output" is not set for the pipeline, but "output_file" is non-NULL,
 *  the last process in the pipeline should be redirected
 * to the specified output file.   If "input_file" is set for the pipeline, then
 * the standard input of the process running the first command in the pipeline should
 * be redirected from the specified input file.
 *
 * @param pline  The pipeline to be run.  The jobs module expects this object
 * to be valid for as long as it requires, and it expects to be able to free this
 * object when it is finished with it.  This means that the caller should not pass
 * a pipeline object that is shared with any other data structure, but rather should
 * make a copy to be passed to this function.
 *
 * @return  -1 if the pipeline could not be initialized properly, otherwise the
 * value returned is the job ID assigned to the pipeline.
 */


int get_jobindex_by_pid(pid_t pid)
{
    for(int i = 0; i < MAX_NUM_JOB; i++)
    {
        if (JOBTABLE[i].pgid == pid && JOBTABLE[i].status != EMPTY)
        {
            return i;
        }
    }
    return -1;
}

void change_job_status(int jobindex, JOB_STATUS status, int exitStatus)
{
    JOBTABLE[jobindex].status = status;
    JOBTABLE[jobindex].exitStatus = exitStatus;
}



int add_job(PIPELINE *pline, pid_t lpid)
{
    if(pline == NULL)
    {
        printf("ERR: jobs.c:add_job");
        return -1;
    }
    int emptyIndex = find_empty_table_index();
    if(emptyIndex == -1)
    {
        printf("ERR: maxjob jobs.c:add_job");
        return -1;
    }

    JOB newJob = {.id = emptyIndex, .pgid = lpid, .status = RUNNING, .pline = pline};
    JOBTABLE[emptyIndex] = newJob;
    JOBCNT++;
    return emptyIndex;
}

int find_empty_table_index()
{
    for(int i = 0; i < MAX_NUM_JOB; i++)
    {
        if(JOBTABLE[i].status == EMPTY)
        {
            return i;
        }
    }
    return -1;
}

int clear_Job(int index)
{
    if(index < 0 || index >= MAX_NUM_JOB)
    {
        printf("ERR: jobs.c:clear_Job");
        return -1;
    }
    JOBTABLE[index].id = -1;
    JOBTABLE[index].pgid = -1;
    JOBTABLE[index].status = EMPTY;
    if(JOBTABLE[index].pline != NULL)
    {
        free(JOBTABLE[index].pline);
    }
    JOBTABLE[index].exitStatus = -1;
    JOBCNT--;
    return 0;
}

void sigio_handler(int sig)
{
    outcapReadReady = 1;
}

void sigio_action(int readfd)
{
    if(stream != NULL)
    {
        // fclose(stream);
    }
    char buf2;
    size_t len;

    stream = open_memstream(&capbuf, &len);

    while(1){
        if(read(readfd, &buf2, 1) != 1)
        {
            break;
        }
        fprintf(stream, "%c", buf2);
    }
    fflush(stream);
    outcapReadReady = -1;
}

void leader_handler(int sig)
{
    int leader_status;

    pid_t wpid = waitpid(-1, &leader_status, WNOHANG);
    int jobindex = get_jobindex_by_pid(wpid);

    if(WIFEXITED(leader_status))
    {
        change_job_status(jobindex, COMPLETED, leader_status);
    }
    else
    {
        if(leader_status == 9)
        {
            change_job_status(jobindex, CANCELED, leader_status);
        }
        else
        {
            change_job_status(jobindex, ABORTED, leader_status);
        }
    }
}

int jobs_run(PIPELINE *pline) {


    if(pline == NULL)
    {
        return -1;
    }
    int curjobid;
    PIPELINE *cpypline = copy_pipeline(pline);


    int isCapureOutput = -1;
    if(cpypline->capture_output != 0)
    {
        if(ocpipefd[0] != 0)
        {
            close(ocpipefd[0]);
        }
        if(ocpipefd[1] != 0)
        {
            close(ocpipefd[1]);
        }
        isCapureOutput = 1;
        pipe(ocpipefd);
    }

    pid_t lpid;

    lpid = fork();

    if(lpid == 0)
    {
        int lid = getpid();
        int res = setpgid(lid, lid);

        if(res == -1)
        {
            printf("jobs.c:job_run :: setpgid error");
        }

        if(isCapureOutput != -1)
        {
            close(ocpipefd[0]);
        }

        int leaderStatus = leader_process(cpypline, ocpipefd[1]);

        if(isCapureOutput != -1)
        {
            close(ocpipefd[1]);
        }

        if(WIFEXITED(leaderStatus))
        {
            exit(leaderStatus);
        }
        else
        {
            printf("ledader err\n");
            fflush(stdout);
            exit(SIGABRT);
        }
    }
    else
    {
        // signal(SIGIO, sigio_handler);
        if(isCapureOutput == 1)
        {
            close(ocpipefd[1]);
            sigio_action(ocpipefd[0]); // check capture output ready.
        }

        curjobid = add_job(cpypline, lpid);
        if(curjobid == -1) return -1; // if job count exceed max -> return -1;

        signal(SIGCHLD, leader_handler); // if leader process ends, called
    }

    // need to add new job here.

    return curjobid;
}

void close_pipefd(int pipefd[][2], int pNum)
{
    for(int i = 0; i < pNum; i++)
    {
        close(pipefd[i][0]);
        close(pipefd[i][1]);
    }
}

int chld_process(COMMAND *cmd, int pipefd[][2], int pr, int pw, int pNum)
{
    fflush(stdout);
    for(int i = 0; i < pNum; i++)
    {
        if(i != pr)
        {
            close(pipefd[i][0]);
        }
        if(i != pw)
        {
            close(pipefd[i][1]);
        }
    }

    int cntArgs = count_args(cmd);
    char* jargv[cntArgs+1];
    set_argv(jargv, cmd->args);

    dup2(pipefd[pr][0], 0);
    dup2(pipefd[pw][1], 1);
    execvp(jargv[0], jargv);

    printf("ERR : (jobs.c:child_process)");
    return -1;
}

int io_pipe_setting(PIPELINE *pline, int pipefd[][2], int pNum, int ocwfd)
{
    dup2(0, pipefd[pNum-1][0]); // stdin copy
    dup2(1, pipefd[pNum-1][1]); // stdout copy

    if(pline->input_file != NULL)
    {
        char* filename = pline->input_file;
        int openfd = open(filename, O_RDONLY); // open file readonly.
        dup2(openfd, pipefd[pNum-1][0]); // RDfile copy
    }
    if(pline->output_file != NULL)
    {
        char* filename = pline->output_file;
        int openfd = open(filename, O_WRONLY); // open file writeonly.
        dup2(openfd, pipefd[pNum-1][1]); // WRfile copy
        return 0;
    }
    if(ocwfd != 0)
    {
        dup2(ocwfd, pipefd[pNum-1][1]); // capout copy
        return 0;
    }
    return 0;
}

int io_setting(PIPELINE *pline, int ocwfd)
{

    if(pline->input_file != NULL)
    {
        char* filename = pline->input_file;
        int openfd = open(filename, O_RDONLY); // open file readonly.
        if(openfd != -1)
        {
            dup2(openfd, STDIN_FILENO); // RDfile copy
        }
    }
    if(pline->output_file != NULL)
    {
        char* filename = pline->output_file;
        int openfd = open(filename, O_WRONLY); // open file writeonly.
        if(openfd == -1)
        {
            openfd = open(filename, O_CREAT);
            openfd = open(filename, O_WRONLY);
        }
        dup2(openfd, STDOUT_FILENO); // WRfile copy
        return 0;
    }
    if(ocwfd != 0)
    {
        dup2(ocwfd, STDOUT_FILENO); // capout copy
        return 0;
    }
    return 0;
}

int leader_process(PIPELINE *pline, int ocwfd) // ocpipefd -> output capture fd
{
    sigprocmask(SIG_BLOCK, &s, &os);
    int cmdsNum = count_commands(pline);
    int child_status;
    pid_t pid[cmdsNum];

    COMMAND *curcmd = pline->commands;

    if(cmdsNum == 1)
    {
        if((pid[0] = fork()) == 0)            // if cmdNum == 1 -> no pipeline needed
        {
            io_setting(pline, ocwfd);         // redirection & capturing
            int cntArgs = count_args(curcmd); // count args
            char* jargv[cntArgs+1];
            set_argv(jargv, curcmd->args);    // make into argv[][]

            execvp(jargv[0], jargv);          // execute
        }
        else
        {
            pid_t wpid = wait(&child_status);
            if(WIFEXITED(child_status))
            {
                return child_status;
            }
            else
            {
                printf("child %d terminated abnormally\n", wpid);
                return -1;
            }
        }
    }

    int i, pipefd[cmdsNum][2];
    int lastfd = cmdsNum-1;

    for(i = 0; i < cmdsNum; i++) // create pipeline
    {
        if(pipe(pipefd[i]) < 0)
        {
            return -1;
        }
    }

    io_pipe_setting(pline, pipefd, cmdsNum, ocwfd); // redirection & output capture control

    for(i = 0; i < cmdsNum; i++)
    {
        if((pid[i] = fork()) == 0)
        {
            if(i == 0) // first child
            {
                chld_process(curcmd, pipefd, lastfd, i, cmdsNum); // exit automatically
            }
            else if(i == cmdsNum-1) // last child
            {
                chld_process(curcmd, pipefd, i-1, lastfd, cmdsNum); // exit automatically
            }
            else // middle child
            {
                chld_process(curcmd, pipefd, i-1, i, cmdsNum); // exit automatically
            }
        }
        else
        {
            curcmd = curcmd->next;
        }
    }

    close_pipefd(pipefd, cmdsNum); // close all fd of parents

    for(i = 0; i < cmdsNum; i++) // wait and reap all child of leader process
    {
        pid_t wpid = wait(&child_status);
        if(WIFEXITED(child_status))
        {
            //nothing

        }
        else
        {
             printf("child %d terminated abnormally\n", wpid);
        }
    }
    return child_status;
}



int set_argv(char **argv, ARG *arg)
{
    if(arg == NULL)
    {
        printf("ERR (job.c:set_argv)");
        return -1;
    }
    while(arg != NULL)
    {
        *argv = eval_to_string(arg->expr);
        argv++;
        arg = arg->next;
    }
    *argv = NULL;
    return 0;
}

int count_args(COMMAND *cmds)
{
    int cnt = 0;

    ARG *arg = cmds->args;
    while(arg != NULL)
    {
        cnt++;
        arg = arg->next;
    }
    return cnt;
}


int count_commands(PIPELINE *pline)
{
    int cnt = 0;

    COMMAND *cmd = pline->commands;
    while(cmd != NULL)
    {
        cnt++;
        cmd = cmd->next;
    }
    return cnt;
}




/**
 * @brief  Wait for a job to terminate.
 * @details  This function is used to wait for the job with a specified job ID
 * to terminate.  A job has terminated when it has entered the COMPLETED, ABORTED,
 * or CANCELED state.
 *
 * @param  jobid  The job ID of the job to wait for.
 * @return  the exit status of the job leader, as returned by waitpid(),
 * or -1 if any error occurs that makes it impossible to wait for the specified job.
 */
int jobs_wait(int jobid) {

    int check = -1;


    while(is_job_terminated(jobid) == -1)
    {
        if(check == -1)
        {
            check = 0;
        }

        // wait for signal called;
    }

    return JOBTABLE[jobid].exitStatus;
}

int is_job_terminated(int jobid)
{
    if(JOBTABLE[jobid].status == COMPLETED ||
        JOBTABLE[jobid].status == ABORTED  ||
        JOBTABLE[jobid].status == CANCELED)
    {
        return 1;
    }
    return -1;
}



/**
 * @brief  Poll to find out if a job has terminated.
 * @details  This function is used to poll whether the job with the specified ID
 * has terminated.  This is similar to jobs_wait(), except that this function returns
 * immediately without waiting if the job has not yet terminated.
 *
 * @param  jobid  The job ID of the job to wait for.
 * @return  the exit status of the job leader, as returned by waitpid(), if the job
 * has terminated, or -1 if the job has not yet terminated or if any other error occurs.
 */
int jobs_poll(int jobid) {
    if(is_job_terminated(jobid) == -1)
    {
        return -1;
    }

    if(check_jobid(jobid) == -1)
    {
        return -1;
    }
    int exitStatus = JOBTABLE[jobid].exitStatus;

    clear_Job(jobid);

    return exitStatus;
}

int check_jobid(int jobid)
{
    if(jobid < 0 || jobid >= MAX_NUM_JOB)
    {
        return -1;
    }
    if(JOBTABLE[jobid].status == EMPTY)
    {
        return -1;
    }
    return 0;
}
/**
 * @brief  Expunge a terminated job from the jobs table.
 * @details  This function is used to expunge (remove) a job that has terminated from
 * the jobs table, so that space in the table can be used to start some new job.
 * In order to be expunged, a job must have terminated; if an attempt is made to expunge
 * a job that has not yet terminated, it is an error.  Any resources (exit status,
 * open pipes, captured output, etc.) that were being used by the job are finalized
 * and/or freed and will no longer be available.
 *
 * @param  jobid  The job ID of the job to expunge.
 * @return  0 if the job was successfully expunged, -1 if the job could not be expunged.
 */
int jobs_expunge(int jobid) {

    if(is_job_terminated(jobid) == -1)
    {
        printf("jobs.c:jobs_expunge.\n");
        return -1;
    }

    if(clear_Job(jobid) == -1) {return -1;}
    return 0;
}

/**
 * @brief  Attempt to cancel a job.
 * @details  This function is used to attempt to cancel a running job.
 * In order to be canceled, the job must not yet have terminated and there
 * must not have been any previous attempt to cancel the job.
 * Cancellation is attempted by sending SIGKILL to the process group associated
 * with the job.  Cancellation becomes successful, and the job actually enters the canceled
 * state, at such subsequent time as the job leader terminates as a result of SIGKILL.
 * If after attempting cancellation, the job leader terminates other than as a result
 * of SIGKILL, then cancellation is not successful and the state of the job is either
 * COMPLETED or ABORTED, depending on how the job leader terminated.
 *
 * @param  jobid  The job ID of the job to cancel.
 * @return  0 if cancellation was successfully initiated, -1 if the job was already
 * terminated, a previous attempt had been made to cancel the job, or any other
 * error occurred.
 */
int jobs_cancel(int jobid) {
    if(is_job_terminated(jobid) == 1)
    {
        return -1;
    }

    pid_t pgid = JOBTABLE[jobid].pgid;

    printf("pgid: %d\n", pgid);

    kill(pgid,SIGKILL);

    while(is_job_terminated(jobid) != 1)
    {
        //notiong
    }

    if(JOBTABLE[jobid].status == CANCELED)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/**
 * @brief  Get the captured output of a job.
 * @details  This function is used to retrieve output that was captured from a job
 * that has terminated, but that has not yet been expunged.  Output is captured for a job
 * when the "capture_output" flag is set for its pipeline.
 *
 * @param  jobid  The job ID of the job for which captured output is to be retrieved.
 * @return  The captured output, if the job has terminated and there is captured
 * output available, otherwise NULL.
 */
char *jobs_get_output(int jobid) {
    if(capbuf == NULL) return NULL;
    return capbuf;
}

/**
 * @brief  Pause waiting for a signal indicating a potential job status change.
 * @details  When this function is called it blocks until some signal has been
 * received, at which point the function returns.  It is used to wait for a
 * potential job status change without consuming excessive amounts of CPU time.
 *
 * @return -1 if any error occurred, 0 otherwise.
 */
int jobs_pause(void) {
    sigset_t temps;
    sigemptyset(&temps);
    sigsuspend(&temps);
    return 0;
}
