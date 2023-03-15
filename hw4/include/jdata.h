
typedef enum
{
    NEW,
    RUNNING,
    COMPLETED,
    ABORTED,
    CANCELED,
    EMPTY
}JOB_STATUS;


typedef struct job {
    int id;
    pid_t pgid;
    JOB_STATUS status;
    PIPELINE *pline;
    int exitStatus;
} JOB;

#define MAX_NUM_JOB 30

void leader_handler(int sig);
int count_commands(PIPELINE *pline);
int leader_process(PIPELINE *pline, int ocwfd);
int count_args(COMMAND *cmds);
int set_argv(char **argv, ARG *arg);
int chld_process(COMMAND *cmd, int pipefd[][2], int pr, int pw, int pNum);
void close_pipefd(int pipefd[][2], int pNum);

int get_jobindex_by_pid(pid_t pid);
int get_jobindex_by_jobid(int jobid);

void change_job_status(int jobid, JOB_STATUS status, int exitStatus);
int find_empty_table_index();
int clear_Job(int index);

int is_job_terminated(int jobid);

char* toStr_status(JOB_STATUS status);

int check_jobid(int jobid);

int io_pipe_setting(PIPELINE *pline, int pipefd[][2], int pNum, int ocwfd);
void sigio_action(int readfd);