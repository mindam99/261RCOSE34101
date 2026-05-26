#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * CPU Scheduling Simulator - Process Generator
 * ------------------------------------------------------------
 * 생성 항목:
 *  - PID
 *  - Arrival Time
 *  - Priority
 *  - CPU Burst Count / CPU Burst Time
 *  - I/O Burst Count / I/O Burst Time
 *
 * 프로세스 '7개'를 생성한 뒤 arrival time 순서로 정렬해서 출력
 */

/*
* Config
* ------------------------------------------------------------
* ready queue, waiting queue를 초기화하기 위함 함수
*/

/* 생성할 프로세스 개수: 과제 시뮬레이션 결과 확인이 용이하도록 7개로 고정했다. */
#define PROCESS_COUNT 7

/* arrival time은 0부터 8 사이의 값으로 랜덤 생성 */
#define MIN_ARRIVAL_TIME 0
#define MAX_ARRIVAL_TIME 8

/*
 * 각 프로세스가 가질 CPU burst 개수의 범위
 *
 * cpu_burst_count가 1:
 *   CPU
 * cpu_burst_count가 2:
 *   CPU -> I/O -> CPU
 * cpu_burst_count가 3:
 *   CPU -> I/O -> CPU -> I/O -> CPU
 */
#define MIN_CPU_BURST_COUNT 1
#define MAX_CPU_BURST_COUNT 3

/* CPU burst time은 1부터 9 사이의 값으로 랜덤 생성 */
#define MIN_CPU_BURST_TIME 1
#define MAX_CPU_BURST_TIME 9

/* I/O burst time은 1부터 7 사이의 값으로 랜덤 생성 */
#define MIN_IO_BURST_TIME 1
#define MAX_IO_BURST_TIME 7

/*
 * priority는 1부터 5 사이의 값으로 랜덤 생성
 * 숫자가 작을수록 우선순위가 높다고 가정
*/
#define MIN_PRIORITY 1
#define MAX_PRIORITY 5

/* 실제 OS의 PID처럼 1000부터 9999 사이에서 PID를 랜덤 생성 */
#define MIN_PID 1000
#define MAX_PID 9999

/* round robin 구현 시 사용할 기본 time quantum */
#define DEFAULT_TIME_QUANTUM 2

/* Gantt chart에 저장할 구간 수 */
#define MAX_GANTT_SEGMENTS 200

/* Gantt chart에서 CPU가 idle한 구간을 표시하기 위한 값 */
#define IDLE_PROCESS -1

/* 다음 event time을 찾을 때 사용할 충분히 큰 값
* process의 arrival이나 I/O completion 같이 scheduler의 상태 관리에 큰 영향을 주는 요인들을 발생시키는 시점 탐색용
*/
#define INF 1000000000

#define ALG_FCFS 1
#define ALG_NON_PREEMPTIVE_SJF 2
#define ALG_PREEMPTIVE_SJF 3
#define ALG_NON_PREEMPTIVE_PRIORITY 4
#define ALG_PREEMPTIVE_PRIORITY 5
#define ALG_ROUND_ROBIN 6
#define ALG_LOTTERY 7

/*
 * Process 구조체
 * ------------------------------------------------------------
 * 하나의 프로세스가 가져야 할 정보들의 묶음
 *
 * scheduling algorithm 구현 시
 * 이 Process 내 data들을 ready queue의 입력 데이터로서 사용
 */
typedef struct {
    /* 랜덤 생성되는 PID */
    int pid;

    /* process의 arrival time */
    int arrival_time;

    /* process의 priority. lower value, higher priority */
    int priority;

    /* process의 CPU burst 횟수 */
    int cpu_burst_count;

    /*
     * process의 I/O burst 횟수
     * CPU burst 사이에만 I/O가 발생한다고 가정
     * 따라서 io_burst_count는 항상 cpu_burst_count - 1
     */
    int io_burst_count;

    /*
     * CPU burst time의 배열
     * 최대 3번의 CPU burst 정보 저장 가능
     * cpu_bursts[0]: 최초 CPU burst
     * cpu_bursts[1]: 최초 I/O burst 이후 두 번째 CPU burst
     * ...
     */
    int cpu_bursts[MAX_CPU_BURST_COUNT];

    /*
     * I/O burst time의 배열
     * CPU burst 횟수가 최대 3회이므로, 그 사이의 I/O는 최대 2회
     */
    int io_bursts[MAX_CPU_BURST_COUNT - 1];
} Process;

/*
 * Queue 구조체
 * ------------------------------------------------------------
 * ready queue와 waiting queue를 표현하기 위한 circular queue를 정의
 * queue에는 Process 자체를 복사하지 않고 process의 index만 저장
 * 예: data로 3이 들어가 있는 경우 이는 곧 P3을 의미
 */
typedef struct {
    int data[PROCESS_COUNT];
    int front;
    int rear;
    int count;
} Queue;

/*
 * SchedulerConfig 구조체
 * ------------------------------------------------------------
 * scheduling 시작 전 필요한 시스템 환경 정보 정의
 *
 * ready_queue      : CPU를 할당받을 수 있는(대기 중인) process들의 queue
 * waiting_queue    : I/O 때문에 기다리는 process들의 queue
 * current_time     : simulator의 현재 시간
 * next_arrival_index : 아직 ready queue에 들어가지 않은 다음 process의 index
 * time_quantum     : Round Robin에서 사용할 time quantum
 */
typedef struct {
    Queue ready_queue;
    Queue waiting_queue;
    int current_time;           // simulator의 작동 시점을 기준으로 하는 시계 역할 - scheduling의 시간 경과 확인
    int next_arrival_index;     // 매번 전체 index의 arrival을 검사하는 것을 피하기 위한 마킹 역할
    int time_quantum;
} SchedulerConfig;

/* GanttSegment 구조체
 * ------------------------------------------------------------
 * Gantt chart의 한 단위의 구간을 저장
 *
 * process_index : 해당 구간에서 실행된 process index
 *                 IDLE_PROCESS이면 idle 구간
 * start_time    : 구간 시작 시간
 * end_time      : 구간 종료 시간
 */
typedef struct {
    int process_index;
    int start_time;
    int end_time;
} GanttSegment;

/*
 * random_between()
 * ------------------------------------------------------------
 * min부터 max 사이의 정수 1개를 random하게 return
 * 코드 중복을 줄이기 위함
 */
static int random_between(int min, int max) {
    return min + rand() % (max - min + 1);
}

/*
 * pid_exists()
 * ------------------------------------------------------------
 * 이미 무작위적으로 생성된 프로세스의 PID 중에서 새로 생성된 PID와 중복되는지 검사
 * processes[0]부터 processes[count - 1]까지의 값과 검사
 */
static int pid_exists(const Process processes[], int count, int pid) {
    int i;

    for (i = 0; i < count; i++) {
        if (processes[i].pid == pid) {
            return 1;   /* 중복 PID 값이 있는 경우의 return value */
        }
    }

    return 0;           /* 중복 PID 값이 없는 경우의 return value */
}

/*
 * create_unique_pid()
 * ------------------------------------------------------------
 * unique PID를 무작위적으로 생성
 *
 *  1. 1000~9999 범위 내에서 PID를 생성
 *  2. 중복 PID 값인지 검사
 *  3. 중복인 경우 다시 생성
 *  4. 중복이 아니면 해당 PID를 반환
 */
static int create_unique_pid(const Process processes[], int count) {
    int pid;

    do {
        pid = random_between(MIN_PID, MAX_PID);
    } while (pid_exists(processes, count, pid));

    return pid;
}

/*
 * compare_by_arrival_time()
 * ------------------------------------------------------------
 * qsort()에서 사용할 비교 함수
 *
 * processes[] 배열을 arrival_time 오름차순으로 정렬
 * 정렬 후 processes[0]은 P0, processes[1]은 P1처럼 출력
 *
 * return 값에 따라,
 *  음수: a가 b보다 앞에 와야 함
 *  0   : a와 b의 순서가 같아도 무방
 *  양수: b가 a보다 앞에 와야 함
 */
static int compare_by_arrival_time(const void *left, const void *right) {
    const Process *a = (const Process *)left;
    const Process *b = (const Process *)right;

    if (a->arrival_time != b->arrival_time) {
        return a->arrival_time - b->arrival_time;
    }

    /* arrival_time이 동일한 경우, tie breaker로서, PID 값 기준 정렬 */
    return a->pid - b->pid;
}

/*
 * create_processes()
 * ------------------------------------------------------------
 * process 7개를 random generate하고, arrival time 순으로 정렬
 *
 * workflow:
 *  1. PID 값 랜덤 생성
 *  2. arrival time 값 랜덤 생성
 *  3. priority 값 랜덤 생성
 *  4. CPU burst 횟수 랜덤 생성
 *  5. CPU burst time 값 랜덤 생성
 *  6. I/O burst time 값 랜덤 생성
 *  7. 최소 하나 이상의 프로세스가 I/O를 가지도록 검사
 *  8. arrival time 기준으로 processes[] 배열 정렬
 */
static void create_processes(Process processes[]) {
    int i, j;
    int has_io = 0;

    /* PROCESS_COUNT(7)만큼 process를 하나씩 생성 */
    for (i = 0; i < PROCESS_COUNT; i++) {
        /* 중복되지 않는 PID 값 생성 */
        processes[i].pid = create_unique_pid(processes, i);

        /* arrival time 및 priority를 정해진 범위 내에서 랜덤 생성 */
        processes[i].arrival_time = random_between(MIN_ARRIVAL_TIME, MAX_ARRIVAL_TIME);
        processes[i].priority = random_between(MIN_PRIORITY, MAX_PRIORITY);

        /* CPU burst 횟수 랜덤 생성 */
        processes[i].cpu_burst_count =
            random_between(MIN_CPU_BURST_COUNT, MAX_CPU_BURST_COUNT);

        /* I/O는 CPU burst와 CPU burst 사이에서 발생한다고 가정 */
        processes[i].io_burst_count = processes[i].cpu_burst_count - 1;

        /* 적어도 하나 I/O가 있는 process가 발생했을 경우 flag로 표시 */
        if (processes[i].io_burst_count > 0) {
            has_io = 1;
        }

        /*
         * CPU burst time 및 I/O burst time 생성
         * j번째 CPU burst time을 생성한 뒤, 그 뒤에 I/O가 있는 경우 j번째 I/O burst도 생성
         */
        for (j = 0; j < processes[i].cpu_burst_count; j++) {
            processes[i].cpu_bursts[j] = random_between(MIN_CPU_BURST_TIME, MAX_CPU_BURST_TIME);

            if (j < processes[i].io_burst_count) {
                processes[i].io_bursts[j] = random_between(MIN_IO_BURST_TIME, MAX_IO_BURST_TIME);
            }
        }
    }

    /*
     * 모든 process의 cpu_burst_count가 1로 생성되면 I/O가 하나도 없게 되므로
     * 과제 조건을 맞추기 위해 첫 번째 process에 I/O를 1개 강제로 삽입
     */
    if (!has_io) {
        processes[0].cpu_burst_count = 2;
        processes[0].io_burst_count = 1;

        processes[0].cpu_bursts[0] = random_between(MIN_CPU_BURST_TIME, MAX_CPU_BURST_TIME);
        processes[0].io_bursts[0] = random_between(MIN_IO_BURST_TIME, MAX_IO_BURST_TIME);
        processes[0].cpu_bursts[1] = random_between(MIN_CPU_BURST_TIME, MAX_CPU_BURST_TIME);
    }

    /*
     * (마무리) arrival time의 순서와 index가 일치하도록 정렬
     * 정렬 후에는 processes[0]이 P0, processes[1]이 P1 등이 된다.
     */
    qsort(processes, PROCESS_COUNT, sizeof(Process), compare_by_arrival_time);
}

/*
 * init_queue()
 * ------------------------------------------------------------
 * queue를 비어 있는 상태로 초기화
 *
 * front: 다음에 dequeue할 위치
 * rear : 마지막으로 enqueue된 위치
 * count: 현재 queue에 들어 있는 process 개수
 */
static void init_queue(Queue* queue) {
    queue->front = 0;
    queue->rear = -1;
    queue->count = 0;
}

/*
 * is_queue_empty()
 * ------------------------------------------------------------
 * queue가 비어 있는지 확인
 */
static int is_queue_empty(const Queue* queue) {
    return queue->count == 0;
}

/*
 * is_queue_full()
 * ------------------------------------------------------------
 * queue가 가득 찼는지 확인
 * process 개수가 7개이므로 queue에는 최대 7개의 index만 저장 가능
 */
static int is_queue_full(const Queue* queue) {
    return queue->count == PROCESS_COUNT;
}

/*
 * enqueue()
 * ------------------------------------------------------------
 * queue의 뒤쪽에 process index를 enqueue
 *
 * return 1: 삽입 성공
 * return 0: 삽입 실패 - queue가 full
 */
static int enqueue(Queue* queue, int process_index) {
    if (is_queue_full(queue)) {
        return 0;
    }

    queue->rear = (queue->rear + 1) % PROCESS_COUNT;
    queue->data[queue->rear] = process_index;
    queue->count++;

    return 1;
}

/*
 * dequeue()
 * ------------------------------------------------------------
 * queue의 앞쪽 process index를 dequeue
 *
 * ready queue에서는 CPU를 할당받을 process를 꺼낼 때
 * waiting queue에서는 I/O가 끝난 process를 꺼내 ready queue로 보낼 때
 *
 * return nonnegative integer: queue에서 꺼낸 process의 index
 * return -1: queue가 empty
 */
static int dequeue(Queue* queue) {
    int process_index;

    if (is_queue_empty(queue)) {
        return -1;
    }

    process_index = queue->data[queue->front];
    queue->front = (queue->front + 1) % PROCESS_COUNT;
    queue->count--;

    /* queue가 비면 front/rear를 초기 상태로 되돌림 */
    if (queue->count == 0) {
        queue->front = 0;
        queue->rear = -1;
    }

    return process_index;
}

/* remove_from_queue()
 * ------------------------------------------------------------
 * queue에서 I/O가 끝난 process를 waiting queue에서 제거할 때 사용
 * dequeue는 queue의 임의의 위치에 있는 item을 바로 제거할 수 없기 때문에 추가한 함수
 * 나머지 process들의 순서는 동일하게 유지시킨다.
 */
static int remove_from_queue(Queue* queue, int process_index) {
    Queue temp;
    int removed = 0;
    int current;

    init_queue(&temp);

    while (!is_queue_empty(queue)) {
        current = dequeue(queue);

        if (current == process_index && !removed) {
            removed = 1;
        }
        else {
            enqueue(&temp, current);
        }
    }

    *queue = temp;
    return removed;
}

/*
 * config()
 * ------------------------------------------------------------
 * scheduling 시작 전 시스템 환경 초기화
 *
 * workflow:
 *  1. ready queue 초기화
 *  2. waiting queue 초기화
 *  3. simulator의 현재 시간을 0으로 초기화
 *  4. round robin용 time quantum 값 설정
 *  5. arrival_time이 현재 시간 이하인 process를 ready queue에 삽입
 *
 * waiting queue는 CPU burst 후 I/O 발생시 사용되므로 초기에는 empty
 */
static void config(SchedulerConfig* scheduler_config, const Process processes[]) {
    init_queue(&scheduler_config->ready_queue);
    init_queue(&scheduler_config->waiting_queue);

    scheduler_config->current_time = 0;
    scheduler_config->next_arrival_index = 0;
    scheduler_config->time_quantum = DEFAULT_TIME_QUANTUM;

    /* arrival_time이 0인 process는 scheduling 시작 시점부터 바로 ready queue에 들어갈 수 있음 */
    while (scheduler_config->next_arrival_index < PROCESS_COUNT &&
        processes[scheduler_config->next_arrival_index].arrival_time <= scheduler_config->current_time) {
        enqueue(&scheduler_config->ready_queue, scheduler_config->next_arrival_index);
        scheduler_config->next_arrival_index++;
    }
}

/*
 * print_queue()
 * ------------------------------------------------------------
 * queue 내부 process index를 출력
 *
 * 출력용 '복사본'을 만들어 dequeue하므로 실제 queue 내용에는 변동 없음
 */
static void print_queue(const char* queue_name, const Queue* queue) {
    Queue temp = *queue;
    int process_index;

    printf("%s: ", queue_name);

    if (is_queue_empty(queue)) {
        printf("(empty)\n");
        return;
    }

    while (!is_queue_empty(&temp)) {
        process_index = dequeue(&temp);
        printf("P%d", process_index);

        if (!is_queue_empty(&temp)) {
            printf(" -> ");
        }
    }

    printf("\n");
}

/*
 * print_config()
 * ------------------------------------------------------------
 * Config() 수행 후의 시스템 환경 출력
 * ready queue 및 waiting queue가 제대로 초기화되었는지 확인
 */
static void print_config(const SchedulerConfig* scheduler_config) {
    printf("\n=== System Config ===\n");
    printf("Current Time       : %d\n", scheduler_config->current_time);
    printf("Time Quantum       : %d\n", scheduler_config->time_quantum);

    if (scheduler_config->next_arrival_index < PROCESS_COUNT) {
        printf("Next Arrival Index : P%d\n", scheduler_config->next_arrival_index);
    }
    else {
        printf("Next Arrival Index : none\n");
    }

    print_queue("Ready Queue        ", &scheduler_config->ready_queue);
    print_queue("Waiting Queue      ", &scheduler_config->waiting_queue);
}

/*
 * print_processes()
 * ------------------------------------------------------------
 * 생성된 process 정보를 표 형태로 출력 (중간 점검용)
 */
static void print_processes(const Process processes[]) {
    int i, j;

    printf("\n=== Randomly Created Processes ===\n");

    printf("%-6s %-6s %-8s %-8s %-7s %-6s %s\n", "Name", "PID", "Arrival", "Priority", "CPUcnt", "IOcnt", "Burst Sequence");
    printf("--------------------------------------------------------------------------------\n");

    for (i = 0; i < PROCESS_COUNT; i++) {
        printf("P%-5d %-6d %-8d %-8d %-7d %-6d ",
               i,
               processes[i].pid,
               processes[i].arrival_time,
               processes[i].priority,
               processes[i].cpu_burst_count,
               processes[i].io_burst_count);

        /* burst sequence 출력 코드 */
        for (j = 0; j < processes[i].cpu_burst_count; j++) {
            printf("CPU(%d)", processes[i].cpu_bursts[j]);

            if (j < processes[i].io_burst_count) {
                printf(" -> IO(%d) -> ", processes[i].io_bursts[j]);
            }
        }

        printf("\n");
    }
}

/* get_total_cpu_time()
 * ------------------------------------------------------------
 * evaluation에서 waiting time을 계산하기 위해 한 process의 전체 CPU burst time 합을 반환
 */
static int get_total_cpu_time(const Process* process) {
    int i;
    int total = 0;

    for (i = 0; i < process->cpu_burst_count; i++) {
        total += process->cpu_bursts[i];
    }

    return total;
}

/* get_total_io_time()
 * ------------------------------------------------------------
 * evaluation에서 waiting time을 계산하기 위해 한 process의 전체 I/O burst time 합을 반환
 */
static int get_total_io_time(const Process* process) {
    int i;
    int total = 0;

    for (i = 0; i < process->io_burst_count; i++) {
        total += process->io_bursts[i];
    }

    return total;
}

/* add_gantt_segment()
 * ------------------------------------------------------------
 * Gantt chart에 새로운 실행 구간을 추가
 * 같은 process가 바로 이어서 실행되는 경우에는 구간을 나누지 않고 기존 구간과 합침
 */
static void add_gantt_segment(GanttSegment gantt[], int* gantt_count, int process_index, int start_time, int end_time) {
    if (start_time >= end_time) {
        return;
    }

    if (*gantt_count > 0 &&
        gantt[*gantt_count - 1].process_index == process_index &&
        gantt[*gantt_count - 1].end_time == start_time) {
        gantt[*gantt_count - 1].end_time = end_time;
        return;
    }

    if (*gantt_count >= MAX_GANTT_SEGMENTS) {
        printf("Warning: Gantt Chart segment limit exceeded.\n");
        return;
    }

    gantt[*gantt_count].process_index = process_index;
    gantt[*gantt_count].start_time = start_time;
    gantt[*gantt_count].end_time = end_time;
    (*gantt_count)++;
}

/*
 * print_gantt_chart()
 * ------------------------------------------------------------
 * scheduling 결과로 만들어진 Gantt chart 출력
 */
static void print_gantt_chart(const GanttSegment gantt[], int gantt_count) {
    int i;

    printf("\nGantt Chart\n");
    printf("------------------------------------------------------------\n");

    if (gantt_count == 0) {
        printf("(empty)\n");
        return;
    }

    printf("[%d]", gantt[0].start_time);

    for (i = 0; i < gantt_count; i++) {
        if (gantt[i].process_index == IDLE_PROCESS) {
            printf("--IDLE--");
        }
        else {
            printf("--P%d--", gantt[i].process_index);
        }

        printf("[%d]", gantt[i].end_time);
    }

    printf("\n");
}

/* print_evaluation()
 * ------------------------------------------------------------
 * 각 process의 completion time, turnaround time, waiting time 출력
 *
 * turnaround time = completion time - arrival time
 * waiting time    = turnaround time - total CPU time - total I/O time
 */
static void print_evaluation(const char* algorithm_name, const Process processes[], const int completion_time[]) {
    int i;
    int turnaround_time;
    int waiting_time;
    int total_turnaround_time = 0;
    int total_waiting_time = 0;

    printf("\n=== %s Evaluation ===\n", algorithm_name);
    printf("%-6s %-8s %-11s %-12s %-12s %-8s %-8s\n",
        "Name", "Arrival", "Completion", "Turnaround", "Waiting", "CPU", "I/O");
    printf("------------------------------------------------------------------------\n");

    for (i = 0; i < PROCESS_COUNT; i++) {
        int total_cpu_time = get_total_cpu_time(&processes[i]);
        int total_io_time = get_total_io_time(&processes[i]);

        turnaround_time = completion_time[i] - processes[i].arrival_time;
        waiting_time = turnaround_time - total_cpu_time - total_io_time;

        total_turnaround_time += turnaround_time;
        total_waiting_time += waiting_time;

        printf("P%-5d %-8d %-11d %-12d %-12d %-8d %-8d\n",
            i,
            processes[i].arrival_time,
            completion_time[i],
            turnaround_time,
            waiting_time,
            total_cpu_time,
            total_io_time);
    }

    printf("\nAverage Turnaround Time: %.2f\n", (double)total_turnaround_time / PROCESS_COUNT);
    printf("Average Waiting Time   : %.2f\n", (double)total_waiting_time / PROCESS_COUNT);
}

/* find_next_event_time()
 * ------------------------------------------------------------
 * ready queue가 비어 있을 때 다음으로 발생할 event time을 찾는다.
 *
 * event 1: 새로운 process의 arrival
 * event 2: waiting queue에 있는 process의 I/O completion
 * 
 * ready queue가 비어서 CPU가 idle 상태일 때는 위 time까지 current_time 값을 이동
 * preemptive algorithm에서는 event time까지 CPU를 실행시키고 상황에 따라 재판단 수행
 */
static int find_next_event_time(const SchedulerConfig* scheduler_config, const Process processes[], const int io_completion_time[]) {
    int i;
    int next_event_time = INF;

    if (scheduler_config->next_arrival_index < PROCESS_COUNT) {
        next_event_time = processes[scheduler_config->next_arrival_index].arrival_time;
    }

    for (i = 0; i < PROCESS_COUNT; i++) {
        if (io_completion_time[i] != -1 && io_completion_time[i] < next_event_time) {
            next_event_time = io_completion_time[i];
        }
    }

    return next_event_time;
}

/* update_ready_queue_until()
 * ------------------------------------------------------------
 * target_time까지 발생한 arrival 및 I/O completion event를 ready queue에 반영
 *
 * Non-preemptive algorithm에서는 CPU burst 도중 event가 발생해도 현재 process를 중단하지 않는다.
 * 대신 CPU burst가 끝난 직후, 그동안 발생한 event들을 시간 순서대로 ready queue에 반영한다.
 *
 * Preemptive algorithm에서는 CPU를 next event time까지만 실행한 뒤 이 함수를 호출한다.
 * 따라서 event 발생 시점마다 ready queue를 갱신하고 다시 scheduling 판단을 한다. (preempt 활성화)
 */
static void update_ready_queue_until(SchedulerConfig* scheduler_config, const Process processes[], int io_completion_time[], int target_time) {
    int i;

    while (1) {
        int event_time = INF;

        if (scheduler_config->next_arrival_index < PROCESS_COUNT && processes[scheduler_config->next_arrival_index].arrival_time <= target_time) {
            event_time = processes[scheduler_config->next_arrival_index].arrival_time;
        }

        for (i = 0; i < PROCESS_COUNT; i++) {
            if (io_completion_time[i] != -1 && io_completion_time[i] <= target_time && io_completion_time[i] < event_time) {
                event_time = io_completion_time[i];
            }
        }

        if (event_time == INF) {
            break;
        }

        if (scheduler_config->current_time < event_time) {
            scheduler_config->current_time = event_time;
        }

        while (scheduler_config->next_arrival_index < PROCESS_COUNT && processes[scheduler_config->next_arrival_index].arrival_time <= event_time) {
            enqueue(&scheduler_config->ready_queue, scheduler_config->next_arrival_index);
            scheduler_config->next_arrival_index++;
        }

        for (i = 0; i < PROCESS_COUNT; i++) {
            if (io_completion_time[i] != -1 && io_completion_time[i] <= event_time) {
                remove_from_queue(&scheduler_config->waiting_queue, i);
                enqueue(&scheduler_config->ready_queue, i);
                io_completion_time[i] = -1;
            }
        }
    }

    scheduler_config->current_time = target_time;
}

/* get_lottery_ticket_count()
 * ------------------------------------------------------------
 * lottery scheduling에서 process가 받을 ticket 개수를 할당
 * priority가 높을수록 더 많은 ticket을 받아야 한다.
 * 따라서 ticket 수는 priority와 반비례 관계
 *   priority 1 -> 5 tickets
 *   priority 2 -> 4 tickets
 *   priority 3 -> 3 tickets
 *   priority 4 -> 2 tickets
 *   priority 5 -> 1 ticket
 */
static int get_lottery_ticket_count(const Process* process) {
    return MAX_PRIORITY - process->priority + 1;
}

/* select_process_from_ready()
 * ------------------------------------------------------------
 * algorithm에 따른 기준으로 ready queue에서 실행할 process를 선택
 *
 * algorithm별 선택 기준:
 *  FCFS / RR      : ready queue의 첫 process
 *  SJF 계열       : remaining CPU time이 가장 짧은 process
 *  Priority 계열  : priority 값이 가장 작은 process
 *  Lottery        : priority에 따라 ticket을 부여하고 random ticket으로 선택
 *
 * 선택된 process는 ready queue에서 제거되고, 나머지 process들의 순서는 유지된다.
 */
static int select_process_from_ready(Queue* ready_queue, const Process processes[], const int remaining_cpu_time[], int algorithm) {
    Queue temp;
    int i;
    int selected_position = -1;
    int selected_process = -1;
    int selected_value = INF;
    int original_count = ready_queue->count;

    if (is_queue_empty(ready_queue)) {
        return -1;
    }

    /*
     * FCFS와 Round Robin은 ready queue에 먼저 들어온 process를 선택
     */
    if (algorithm == ALG_FCFS || algorithm == ALG_ROUND_ROBIN) {
        return dequeue(ready_queue);
    }

    /*
     * Lottery Scheduling
     * ------------------------------------------------------------
     * ready queue 안에 있는 process들에게 ticket을 부여하고,
     * 1부터 total_tickets 사이의 ticket 번호를 random으로 뽑는다.
     *
     * ticket을 많이 가진 process일수록 선택될 확률이 높다.
     */
    if (algorithm == ALG_LOTTERY) {
        int total_tickets = 0;
        int winning_ticket;
        int ticket_sum = 0;

        /*
         * ready queue에 있는 process들의 ticket 총합 계산
         */
        for (i = 0; i < original_count; i++) {
            int queue_position = (ready_queue->front + i) % PROCESS_COUNT;
            int process_index = ready_queue->data[queue_position];

            total_tickets += get_lottery_ticket_count(&processes[process_index]);
        }

        /*
         * 1부터 total_tickets 사이에서 당첨 ticket 번호를 random으로 선택
         */
        winning_ticket = random_between(1, total_tickets);

        /*
         * ready queue 순서대로 ticket 범위를 누적하면서
         * winning_ticket을 포함하는 process를 찾는다.
         */
        for (i = 0; i < original_count; i++) {
            int queue_position = (ready_queue->front + i) % PROCESS_COUNT;
            int process_index = ready_queue->data[queue_position];

            ticket_sum += get_lottery_ticket_count(&processes[process_index]);

            if (winning_ticket <= ticket_sum) {
                selected_position = i;
                selected_process = process_index;
                break;
            }
        }
    }
    else {
        /*
         * SJF / Priority Scheduling
         * ------------------------------------------------------------
         * SJF는 remaining CPU time이 가장 짧은 process 선택
         * Priority는 priority 값이 가장 작은 process 선택
         */
        for (i = 0; i < original_count; i++) {
            int queue_position = (ready_queue->front + i) % PROCESS_COUNT;
            int process_index = ready_queue->data[queue_position];
            int value;

            if (algorithm == ALG_NON_PREEMPTIVE_SJF || algorithm == ALG_PREEMPTIVE_SJF) {
                value = remaining_cpu_time[process_index];
            }
            else {
                value = processes[process_index].priority;
            }

            if (selected_position == -1 || value < selected_value || (value == selected_value && process_index < selected_process)) {
                selected_value = value;
                selected_process = process_index;
                selected_position = i;
            }
        }
    }

    /*
     * circular queue에서는 중간 element를 바로 제거하기 어렵기 때문에,
     * 선택된 process만 제외하고 temp queue에 다시 넣는다.
     */
    init_queue(&temp);

    for (i = 0; i < original_count; i++) {
        int process_index = dequeue(ready_queue);

        if (i != selected_position) {
            enqueue(&temp, process_index);
        }
    }

    *ready_queue = temp;
    return selected_process;
}

/* get_algorithm_name()
 * ------------------------------------------------------------
 * algorithm index에 해당하는 이름 반환
 * 문장 출력과 evaluation title에 사용
 */
static const char* get_algorithm_name(int algorithm) {
    switch (algorithm) {
    case ALG_FCFS:
        return "FCFS";
    case ALG_NON_PREEMPTIVE_SJF:
        return "Non-Preemptive SJF";
    case ALG_PREEMPTIVE_SJF:
        return "Preemptive SJF";
    case ALG_NON_PREEMPTIVE_PRIORITY:
        return "Non-Preemptive Priority";
    case ALG_PREEMPTIVE_PRIORITY:
        return "Preemptive Priority";
    case ALG_ROUND_ROBIN:
        return "Round Robin";
    case ALG_LOTTERY:
        return "Lottery";
    default:
        return "Unknown";
    }
}

/* is_preemptive_algorithm()
 * ------------------------------------------------------------
 * event(arrival 또는 I/O event)가 발생했을 때 현재 실행 중인 process를 중단하고 다시 scheduling 판단을 해야 하는 algorithm인지 검사
 */
static int is_preemptive_algorithm(int algorithm) {
    return algorithm == ALG_PREEMPTIVE_SJF ||
        algorithm == ALG_PREEMPTIVE_PRIORITY;
}

/* get_run_end_time()
 * ------------------------------------------------------------
 * 선택된 process를 현재 시점에서 언제까지 실행할지 결정
 *
 * Non-preemptive algorithm:
 *   현재 CPU burst가 끝날 때까지 실행
 *
 * Preemptive SJF / Preemptive Priority:
 *   현재 CPU burst가 끝나기 전 arrival 또는 I/O completion event가 발생하면 그 event time까지만 실행하고 다시 process 선택
 *
 * Round Robin:
 *   CPU burst completion과 time quantum 중 더 빠른 시점까지만 실행
 */
static int get_run_end_time(const SchedulerConfig* scheduler_config, const Process processes[], const int io_completion_time[], const int remaining_cpu_time[], int process_index, int algorithm) {
    int cpu_finish_time = scheduler_config->current_time + remaining_cpu_time[process_index];
    int run_end_time = cpu_finish_time;

    if (algorithm == ALG_ROUND_ROBIN || algorithm == ALG_LOTTERY) {
        int quantum_end_time =
            scheduler_config->current_time + scheduler_config->time_quantum;

        if (quantum_end_time < run_end_time) {
            run_end_time = quantum_end_time;
        }
    }
    else if (is_preemptive_algorithm(algorithm)) {
        int next_event_time =
            find_next_event_time(scheduler_config, processes, io_completion_time);

        if (next_event_time < run_end_time) {
            run_end_time = next_event_time;
        }
    }

    return run_end_time;
}

/* run_scheduler()
 * ------------------------------------------------------------
 * FCFS, SJF, Priority, RR, Lottery algorithm을 실행시키는 공통 engine
 *
 * 공통 workflow:
 *  1. process의 arrival 처리
 *  2. I/O completion 처리
 *  3. ready queue / waiting queue 관리
 *  4. Gantt chart 기록
 *  5. completion time 기록
 *  6. evaluation 출력
 *
 * algorithm별 차이:
 *  1. ready queue에서 어떤 process를 선택하는가
 *  2. 선택된 process를 언제까지 실행하는가
 *  3. 실행 후 아직 CPU burst가 남아 있으면 ready queue에 다시 넣을 것인가
 */
static void run_scheduler(const Process processes[], const SchedulerConfig* base_config, int algorithm) {
    SchedulerConfig run_config = *base_config;
    GanttSegment gantt[MAX_GANTT_SEGMENTS];
    int gantt_count = 0;

    int current_cpu_burst_index[PROCESS_COUNT];
    int remaining_cpu_time[PROCESS_COUNT];
    int io_completion_time[PROCESS_COUNT];
    int completion_time[PROCESS_COUNT];
    int completed_count = 0;
    int i;

    for (i = 0; i < PROCESS_COUNT; i++) {
        current_cpu_burst_index[i] = 0;
        remaining_cpu_time[i] = processes[i].cpu_bursts[0];
        io_completion_time[i] = -1;
        completion_time[i] = -1;
    }

    printf("\n=== %s Scheduling Result ===\n", get_algorithm_name(algorithm));

    while (completed_count < PROCESS_COUNT) {
        int process_index;
        int run_start_time;
        int run_end_time;
        int executed_time;

        update_ready_queue_until(&run_config, processes, io_completion_time, run_config.current_time);

        if (is_queue_empty(&run_config.ready_queue)) {
            int next_event_time = find_next_event_time(&run_config, processes, io_completion_time);

            if (next_event_time == INF) {
                break;
            }

            add_gantt_segment(gantt, &gantt_count, IDLE_PROCESS, run_config.current_time, next_event_time);

            update_ready_queue_until(&run_config, processes, io_completion_time, next_event_time);
            continue;
        }

        process_index = select_process_from_ready(&run_config.ready_queue, processes, remaining_cpu_time, algorithm);

        if (process_index == -1) {
            break;
        }

        run_start_time = run_config.current_time;

        run_end_time = get_run_end_time(&run_config, processes, io_completion_time, remaining_cpu_time, process_index, algorithm);

        executed_time = run_end_time - run_start_time;

        if (executed_time <= 0) {
            enqueue(&run_config.ready_queue, process_index);
            update_ready_queue_until(&run_config, processes, io_completion_time, run_config.current_time);
            continue;
        }

        add_gantt_segment(gantt, &gantt_count, process_index, run_start_time, run_end_time);

        remaining_cpu_time[process_index] -= executed_time;
        run_config.current_time = run_end_time;

        update_ready_queue_until(&run_config, processes, io_completion_time, run_config.current_time);

        if (remaining_cpu_time[process_index] == 0) {
            current_cpu_burst_index[process_index]++;

            if (current_cpu_burst_index[process_index] < processes[process_index].cpu_burst_count) {
                int io_index = current_cpu_burst_index[process_index] - 1;

                remaining_cpu_time[process_index] = processes[process_index].cpu_bursts[current_cpu_burst_index[process_index]];

                io_completion_time[process_index] = run_config.current_time + processes[process_index].io_bursts[io_index];

                enqueue(&run_config.waiting_queue, process_index);
            }
            else {
                completion_time[process_index] = run_config.current_time;
                completed_count++;
            }
        }
        else {
            /*
             * CPU burst가 아직 끝나지 않았는데 실행이 멈춘 경우
             *  - Preemptive algorithm: event 발생으로 재선택 필요
             *  - RR: time quantum 만료
             *
             * 아직 실행할 CPU 시간이 남아 있는 process이므로 ready queue에 다시 삽입
             */
            enqueue(&run_config.ready_queue, process_index);
        }
    }

    print_gantt_chart(gantt, gantt_count);
    print_evaluation(get_algorithm_name(algorithm), processes, completion_time);
}

/*
 * clear_input_buffer()
 * ------------------------------------------------------------
 * 잘못된 input이 들어왔을 때 input buffer를 비운다. (예외 처리용 함수)
 */
static void clear_input_buffer(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        ;
    }
}

/*
 * print_algorithm_menu()
 * ------------------------------------------------------------
 * 실행할 scheduling algorithm을 선택할 menu 출력
 */
static void print_algorithm_menu(void) {
    printf("\n=== Scheduling Algorithm Menu ===\n");
    printf("1. FCFS\n");
    printf("2. Non-Preemptive SJF\n");
    printf("3. Preemptive SJF\n");
    printf("4. Non-Preemptive Priority\n");
    printf("5. Preemptive Priority\n");
    printf("6. Round Robin\n");
    printf("7. Lottery\n");
    printf("0. Exit\n");
}

/*
 * select_algorithm()
 * ------------------------------------------------------------
 * menu에서 사용자의 입력을 받아 scheduling algorithm 번호를 반환
 *
 * return 0: simulator 종료
 * return 1~7: 각 scheduling algorithm과 대응
 */
static int select_algorithm(void) {
    int choice;

    while (1) {
        print_algorithm_menu();
        printf("Select an algorithm: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            clear_input_buffer();
            continue;
        }

        if (choice >= 0 && choice <= 7) {
            return choice;
        }

        printf("Invalid selection. Please choose agin.\n");
    }
}

/* run_fcfs()
 * ------------------------------------------------------------
 * FCFS scheduling 실행 함수
 *
 * 실제 scheduling loop는 공통 함수 run_scheduler()에서 수행하는 것으로 끝난다.
 */
static void run_fcfs(const Process processes[], const SchedulerConfig* base_config) {
    run_scheduler(processes, base_config, ALG_FCFS);
}

/* run_non_preemptive_sjf()
 * ------------------------------------------------------------
 * Non-Preemptive SJF scheduling 실행 함수
 *
 * 실제 scheduling loop는 공통 함수 run_scheduler()에서 수행하는 것으로 끝난다.
 */
static void run_non_preemptive_sjf(const Process processes[], const SchedulerConfig* base_config) {
    run_scheduler(processes, base_config, ALG_NON_PREEMPTIVE_SJF);
}

/* run_preemptive_sjf()
 * ------------------------------------------------------------
 * preemptive SJF scheduling 실행 함수
 * 
 * 실제 scheduling loop는 공통 함수 run_scheduler()에서 수행하고,
 * 여기서는 preemptive SJF 방식으로 실행하도록 algorithm 번호만 전달한다.
 */
static void run_preemptive_sjf(const Process processes[],
    const SchedulerConfig* base_config) {
    run_scheduler(processes, base_config, ALG_PREEMPTIVE_SJF);
}

/* run_non_preemptive_priority()
 * ------------------------------------------------------------
 * non-preemptive priority scheduling 실행 함수
 * 
 * 실제 scheduling loop는 공통 함수 run_scheduler()에서 수행하고,
 * 여기서는 non-preemptive priority 방식으로 실행하도록 algorithm 번호만 전달한다.
 */
static void run_non_preemptive_priority(const Process processes[],
    const SchedulerConfig* base_config) {
    run_scheduler(processes, base_config, ALG_NON_PREEMPTIVE_PRIORITY);
}

/* run_preemptive_priority()
 * ------------------------------------------------------------
 * preemptive priority scheduling 실행 함수
 * 
 * 실제 scheduling loop는 공통 함수 run_scheduler()에서 수행하고,
 * 여기서는 preemptive priority 방식으로 실행하도록 algorithm 번호만 전달한다.
 */
static void run_preemptive_priority(const Process processes[],
    const SchedulerConfig* base_config) {
    run_scheduler(processes, base_config, ALG_PREEMPTIVE_PRIORITY);
}

/* run_round_robin()
 * ------------------------------------------------------------
 * RR scheduling 실행 함수
 * 
 * 실제 scheduling loop는 공통 함수 run_scheduler()에서 수행하고,
 * 여기서는 RR 방식으로 실행하도록 algorithm 번호만 전달한다.
 */
static void run_round_robin(const Process processes[],
    const SchedulerConfig* base_config) {
    run_scheduler(processes, base_config, ALG_ROUND_ROBIN);
}

/* run_lottery()
 * ------------------------------------------------------------
 * Lottery scheduling 실행 함수
 *
 * 실제 scheduling loop는 공통 함수 run_scheduler()에서 수행하고,
 * 여기서는 Lottery 방식으로 실행하도록 algorithm 번호만 전달한다.
 */
static void run_lottery(const Process processes[], const SchedulerConfig* base_config) {
    run_scheduler(processes, base_config, ALG_LOTTERY);
}

/*
 * schedule()
 * ------------------------------------------------------------
 * 사용자가 menu에서 입력한 번호에 맞는 scheduling 함수를 호출
 *
 * 실제 algorithm 구현은 run_fcfs(), run_round_robin() 등의 내부에 추가 예정
 */
static void schedule(int selected_algorithm, const Process processes[], const SchedulerConfig* base_config) {
    switch (selected_algorithm) {
    case 1:
        run_fcfs(processes, base_config);
        break;
    case 2:
        run_non_preemptive_sjf(processes, base_config);
        break;
    case 3:
        run_preemptive_sjf(processes, base_config);
        break;
    case 4:
        run_non_preemptive_priority(processes, base_config);
        break;
    case 5:
        run_preemptive_priority(processes, base_config);
        break;
    case 6:
        run_round_robin(processes, base_config);
        break;
    case 7:
        run_lottery(processes, base_config);
        break;
    default:
        printf("Invalid algorithm selected.\n");
        break;
    }
}

int main(void) {
    /* process 7개를 저장하는 배열의 정의 */
    Process processes[PROCESS_COUNT];

    /* config()에서 초기화할 scheduler 환경 정보 */
    SchedulerConfig scheduler_config;

    /* 사용자가 menu에서 선택한 scheduling algorithm 번호 */
    int selected_algorithm;

    /*
     * 난수 생성기의 seed
     * 현재는 결과 확인과 디버깅을 용이하게 하기 위해 고정 seed 2026으로 설정
     * 매번 다른 데이터 생성하기: srand((unsigned int)time(NULL));
     */
    srand(2026);

    create_processes(processes);

    print_processes(processes);

    config(&scheduler_config, processes);

    print_config(&scheduler_config);

    /*
     * 사용자가 종료를 선택할 때까지 algorithm 선택 menu를 반복 출력
     * 선택된 algorithm에 따라 schedule()에서 해당 run 함수로 분기
     */
    while (1) {
        selected_algorithm = select_algorithm();

        if (selected_algorithm == 0) {
            printf("Exit scheduler simulator.\n");
            break;
        }

        schedule(selected_algorithm, processes, &scheduler_config);
    }

    return 0;
}
