#define _GNU_SOURCE /* CPU scheduling */
#include <stdio.h>
#include <stdlib.h> /* exit */
#include <sched.h> /* CPU scheduing */
#include <unistd.h> /* fork */
#include <sys/syscall.h> /* syscall */
#include "process.h"
#include <errno.h>
#include <string.h>
extern int errno;
void allocate_CPU(pid_t pid, int cpu){
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    if(sched_setaffinity(pid, sizeof(mask), &mask) != 0){
        fprintf(stderr, "Allocate %d CPU failed.", cpu);
        exit(0);
    }
    return;
}
void set_high_priority(pid_t pid){
    struct sched_param param;
    param.sched_priority = 0;
    if(sched_setscheduler(pid, SCHED_OTHER, &param) != 0){
        fprintf(stderr, "Set high priority to %d failed.(%s)\n", pid, strerror(errno));
        exit(0);
    }
    return;
}
void set_low_priority(pid_t pid){
    struct sched_param param;
    param.sched_priority = 0;
    if(sched_setscheduler(pid, SCHED_IDLE, &param) != 0){
        fprintf(stderr, "Set low priority failed.\n");
        exit(0);
    }
    return;
}
void do_process(char *name, pid_t *pid, int exec_time){
    *pid = fork();

    /* child process */
    if(*pid == 0){
	
        int Pid = (int)getpid();
        printf("%s %d\n", name, Pid);
        fflush(stdout);
	    long start_sec, start_nsec, end_sec, end_nsec;
        syscall(GET_SYSTEM_TIME, &start_sec, &start_nsec);
        for(int i = 0; i < exec_time; i ++){
            unit_of_time();
        }
        syscall(GET_SYSTEM_TIME, &end_sec, &end_nsec);
        syscall(PRINTK, Pid, start_sec, start_nsec, end_sec, end_nsec);
        exit(0);
    }

    /* parent process */
    else if(*pid > 0){
        fflush(stderr);
        set_low_priority(*pid);
	    allocate_CPU(*pid, PROCESS_CPU);
    }

    /* fork error */
    else{
        fprintf(stderr, "fork failed");
        exit(0);
    }
}
void unit_of_time(){
    volatile unsigned long i;
    for(i=0;i<1000000UL;i++); 
}
