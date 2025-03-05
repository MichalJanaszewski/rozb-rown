#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h> 

#define MAX_CHILDREN 16
#define REPEAT 1000

int *range, *result, *vector, vector_size;
int shm_range_id, shm_result_id, shm_vector_id;
volatile sig_atomic_t data_ready = 0;

void sigusr1_handler(int signo) { }

void child_process(int id) {
    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    pause();

    clock_t start_time, end_time;
    double total_time = 0.0;

    for (int r = 0; r < REPEAT; r++) {
        int start = range[id], end = range[id + 1], sum = 0;

        start_time = clock();
        for (int i = start; i < end; i++) sum += vector[i];
        end_time = clock(); 
        
        total_time += ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    }

    double average_time = total_time / REPEAT; 

    printf("Child %d average time: %.6f seconds\n", id, average_time);

    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <vector_file> <N_processes> <vector_size>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *filename = argv[1];
    int n = atoi(argv[2]);
    if (n <= 0 || n > MAX_CHILDREN) {
        fprintf(stderr, "Invalid number of processes. Must be 1-%d.\n", MAX_CHILDREN);
        return EXIT_FAILURE;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    vector_size = atoi(argv[3]);

    shm_range_id = shmget(IPC_PRIVATE, (n + 1) * sizeof(int), IPC_CREAT | 0666);
    shm_result_id = shmget(IPC_PRIVATE, n * sizeof(int), IPC_CREAT | 0666);
    shm_vector_id = shmget(IPC_PRIVATE, vector_size * sizeof(int), IPC_CREAT | 0666);

    range = (int *)shmat(shm_range_id, NULL, 0);
    result = (int *)shmat(shm_result_id, NULL, 0);
    vector = (int *)shmat(shm_vector_id, NULL, 0);

    for (int i = 0; i < vector_size; i++) {
        fscanf(file, "%d", &vector[i]);
    }
    fclose(file);

    int chunk = vector_size / n;
    int remainder = vector_size % n;
    range[0] = 0;
    for (int i = 0; i < n; i++) {
        range[i + 1] = range[i] + chunk + (i < remainder ? 1 : 0);
    }

    pid_t pids[n];
    for (int i = 0; i < n; i++) {
        if ((pids[i] = fork()) == 0) {
            child_process(i); 
        }
    }

    sleep(1);
    for (int i = 0; i < n; i++) {
        kill(pids[i], SIGUSR1);
    }

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    int final_sum = 0;
    for (int i = 0; i < n; i++) {
        final_sum += result[i];
    }

    printf("Final sum: %d\n", final_sum);

    shmdt(range);
    shmdt(result);
    shmdt(vector);
    shmctl(shm_range_id, IPC_RMID, NULL);
    shmctl(shm_result_id, IPC_RMID, NULL);
    shmctl(shm_vector_id, IPC_RMID, NULL);

    return EXIT_SUCCESS;
}
