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
 *
 * cpu_burst_count가 2:
 *   CPU -> I/O -> CPU
 *
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
 *  0   : 순서가 같음
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

int main(void) {
    /* process 7개를 저장하는 배열의 정의 */
    Process processes[PROCESS_COUNT];

    /*
     * 난수 생성기의 seed
     * 현재는 결과 확인과 디버깅을 용이하게 하기 위해 고정 seed 2026으로 설정
     * 매번 다른 데이터 생성하기: srand((unsigned int)time(NULL));
     */
    srand(2026);

    create_processes(processes);

    print_processes(processes);

    return 0;
}
