#include <stdio.h>
#include <stdlib.h> /* malloc, qsort */
#include <sys/types.h> /* pid_t */
#include <string.h> /* strcmp */
#include <unistd.h> /* getpid */
#include <sys/wait.h> /* wait */
#include "process.h"

#define FIFO 0
#define RR 1
#define SJF 2
#define PSJF 3
#define MAX_N 20
#define TIME_QUANTEM 500
#define MAX_QUEUE_SIZE 1048576

int N; /* number of process*/
char S[8]; /* scheduling policy */
int policy;
struct process proc[MAX_N];
int clock;
int executing_proc;
int finished_proc_num;
int last_context_switch_time;
int mask[MAX_N] = {0};
int queue[MAX_QUEUE_SIZE] = {0}, front = -1, back = -1;

int compare_function_ready_time(const void *a, const void *b);
int compare_function_execution_time(const void *a, const void *b);
void read_input();
int pick_next_process();
void Push(int x);
void Pop();
int IsEmpty();
int getFront();

int main(){
    /* 讀取sceduling policy和N個process的資訊 */
    read_input();

    /* 若為PSJF或SJF => 將process依照execution time -> ready time由小到大排列 
       若為FIFO或RR => 將process依照ready time由小到大排列 */
    if(policy == PSJF || policy == SJF) qsort(proc, N, sizeof(struct process), compare_function_execution_time);
    qsort(proc, N, sizeof(struct process), compare_function_ready_time);

    /* allocate SCHEDULER_CPU給scheduler */
    allocate_CPU(getpid(), SCHEDULER_CPU);

    /* 調高scheduler的priority */
    set_high_priority(getpid());

    /* 初始化時鐘、正在執行的process、執行完畢的process數量 */
    clock = 0;
    executing_proc = -1; /* -1:表示沒有process正在執行 */
    finished_proc_num = 0;

    /* 初始化mask(mask = 1:process已經ready好但尚未被fork出) */
    for(int i = 0; i < N; i ++) mask[i]= 0;

    /* 執行一次while迴圈 = a unit of time */
    while(1){
        /* 檢查是否有process執行完畢 */
        if(executing_proc != -1 && proc[executing_proc].exec_time == 0){
            waitpid(proc[executing_proc].pid, NULL, 0);		    
	        executing_proc = -1;
            finished_proc_num ++;
            
            /* 若所有的process皆執行完畢則跳出while迴圈 */
            if(finished_proc_num == N)
                break;
        }

        /* 找出所有在這一秒ready的process，把它們的mask值設為1(表已經ready好但尚未被fork出) */
        for(int i = 0; i < N; i ++){
            if(proc[i].ready_time == clock){
    		    mask[i] = 1;
                /* 因為已經ready了，所以把他加入queue中 */
                if(policy == RR){
                    Push(i);
		        }  
            }
        }

        /* 挑出下一個要execute的process */
        int next_proc = pick_next_process();

        /* 若next process的mask = 1，這時才真正fork出該process */
        if(mask[next_proc] == 1){
            mask[next_proc] = 0;
            do_process(proc[next_proc].name, &proc[next_proc].pid, proc[next_proc].exec_time);
        }

        /* 若需要context switch */
        if(next_proc != -1 && executing_proc != next_proc){
            if(executing_proc != -1) set_low_priority(proc[executing_proc].pid);
            set_high_priority(proc[next_proc].pid);
            executing_proc = next_proc;
            last_context_switch_time = clock;
        }

        /* 經過a unit of time */
        unit_of_time();

        if(executing_proc != -1)
            proc[executing_proc].exec_time --;

        clock ++;
    }

    return 0;
}

void read_input(){
    scanf("%s%d", S, &N);
    if(strcmp(S, "FIFO") == 0){
        policy = FIFO;
    }
    else if(strcmp(S, "RR") == 0){
        policy = RR;
    }
    else if(strcmp(S, "SJF") == 0){
        policy =SJF;
    }
    else if(strcmp(S, "PSJF") == 0){
        policy = PSJF;
    }
    else{
        fprintf(stderr, "Wrong policy.");
        exit(0);
    }

    for(int i = 0; i < N; i ++){
        scanf("%s%d%d", proc[i].name, &proc[i].ready_time, &proc[i].exec_time);
    }
}
int compare_function_execution_time(const void *a, const void *b) {
	return (((struct process *)a)->exec_time - ((struct process *)b)->exec_time);
}
int compare_function_ready_time(const void *a, const void *b) {
	return (((struct process *)a)->ready_time - ((struct process *)b)->ready_time);
}
int pick_next_process(){
    int ret = -1;

    /* Non-preemptive（如果有process在執行則執行中不能被打斷）=> FIFO、SJF */
    if(executing_proc != -1 && (policy == FIFO || policy == SJF)){
        return executing_proc;
    }

    /* Preemptive（process在執行中可被打斷）=> PSJF、RR 或 Non-preemptive但CPU閒置中 */
    /* FIFO => 找ready time最小的process(也就是第finished_proc_num個process) */
    if(policy == FIFO){
        if(proc[finished_proc_num].ready_time <= clock) ret = finished_proc_num;
    }

    /* RR */
    if(policy == RR){
        /* 若CPU使用中，則檢查是否已經到一個time quantum了 */
        if(executing_proc != -1){
            /* 已到一個time quantem，executing process必需放棄CPU */
            if((clock - last_context_switch_time) % TIME_QUANTEM == 0){ 
                /* 若execution process還沒執行完，則把它丟到queue中 */
                if(proc[executing_proc].exec_time > 0){
                    Push(executing_proc);
		        }   
                /* 從queue中取出一個process來跑（若queue是空的則讓CPU閒置） */
                ret = getFront();
                Pop();
            }
            /* 尚未到一個time quantem，executing process可繼續使用CPU */
            else{
                ret = executing_proc;
            }
        }

        /* 若CPU正在閒置，則從queue中取出一個process來跑（若queue是空的則讓CPU閒置） */
        else{
            ret = getFront();
            Pop();
        }
    }

    /* PSJF、SJF => 找execution time最小的process */
    if(policy == PSJF || policy == SJF){
        for(int i = 0; i < N; i ++){
            /* 跳過還沒ready好或已經執行完畢的process */
            if(proc[i].ready_time > clock || proc[i].ready_time <= clock && proc[i].exec_time == 0)
                continue;
            if(ret == -1 || proc[i].exec_time < proc[ret].exec_time)
                ret = i;
        }
    }

    return ret;
}
void Push(int x){
    queue[++back] = x;
}
void Pop(){
    if(IsEmpty()) return;
    front ++;
}
int IsEmpty(){
    return (front == back);
}
int getFront(){
    if(IsEmpty()) return -1;
    return queue[front + 1];
}
