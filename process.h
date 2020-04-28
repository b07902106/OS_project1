#include <sys/types.h> /* pid_t */

#define SCHEDULER_CPU 0
#define PROCESS_CPU 1
#define GET_SYSTEM_TIME 333
#define PRINTK 334
#define MAX_PROCESS_NAME_LENGTH 16


struct process{
    char name[MAX_PROCESS_NAME_LENGTH];
    pid_t pid;
    int ready_time;
    int exec_time;
};

void allocate_CPU(pid_t pid, int cpu);
void set_high_priority(pid_t pid);
void set_low_priority(pid_t pid);
void do_process(char *name, pid_t *pid, int exec_time);
void unit_of_time();
