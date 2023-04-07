#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

#define ROWS 10
#define COLS 10
#define GENERATIONS 10

int grid[ROWS][COLS];
int next[ROWS][COLS];
int shmid;
int* shared;

void initialize() {
    int i, j;
    srand(time(NULL));
    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            grid[i][j] = rand() % 2;
        }
    }
}

void print() {
    int i, j;
    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            printf("%d ", grid[i][j]);
        }
        printf("\n");
    }
}

int countNeighbors(int row, int col) {
    int count = 0;
    int i, j;
    for (i = -1; i <= 1; i++) {
        for (j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;
            int r = row + i;
            int c = col + j;
            if (r < 0 || r >= ROWS || c < 0 || c >= COLS) continue;
            count += grid[r][c];
        }
    }
    return count;
}

void computeGrid(int row_start, int row_end) {
    int i, j;
    for (i = row_start; i <= row_end; i++) {
        for (j = 0; j < COLS; j++) {
            int neighbors = countNeighbors(i, j);
            if (grid[i][j]) {
                if (neighbors < 2 || neighbors > 3) {
                    next[i][j] = 0;
                } else {
                    next[i][j] = 1;
                }
            } else {
                if (neighbors == 3) {
                    next[i][j] = 1;
                } else {
                    next[i][j] = 0;
                }
            }
        }
    }
}

void childProcessGeneration(int pipefd[]) {
    close(pipefd[0]);
    int row_start, row_end;
    read(pipefd[1], &row_start, sizeof(row_start));
    read(pipefd[1], &row_end, sizeof(row_end));
    computeGrid(row_start, row_end);
    write(pipefd[1], shared, ROWS * COLS * sizeof(int));
    close(pipefd[1]);
    exit(0);
}

int main() {
    printf("Initial Grid\n");
    initialize();
    print();

    for(int itr=2; itr <= GENERATIONS; itr++)
    {
    	 key_t key = ftok("game_of_life", 'R');
    	 shmid = shmget(key, ROWS * COLS * sizeof(int), IPC_CREAT | 0666);
    	if (shmid < 0) {
    		perror("shmget");
       		exit(1);
    	}

    	shared = (int*) shmat(shmid, NULL, 0); 
    	if (shared == (int*) -1) {
    		perror("shmat");
   		 exit(1);
    	}

    	int i, j;
    	for (i = 0; i < ROWS; i++) {
        	for (j = 0; j < COLS; j++) {
        	    shared[i * COLS + j] = grid[i][j];
        	}
    	}

    	int num_processes = 4;
    	int pipes[num_processes][2];
    	for (i = 0; i < num_processes; i++) {
        	if (pipe(pipes[i]) == -1) {
        	    perror("pipe");
        	    exit(1);
       		 }
    	}

    	int rows_per_process = ROWS / num_processes;
   	int row_start = 0;
    	int row_end = rows_per_process - 1;
    	for (i = 0; i < num_processes; i++) {
        	pid_t pid = fork();
        	if (pid == 0) {
        	    childProcessGeneration(pipes[i]);
        	} else if (pid < 0) {
        	    perror("fork");
        	    exit(1);
        	} else {
        	    close(pipes[i][1]);
        	    write(pipes[i][0], &row_start, sizeof(row_start));
        	    write(pipes[i][0], &row_end, sizeof(row_end));
        	    row_start += rows_per_process;
        	    row_end += rows_per_process;
        	}
    	}	

    	for (i = 0; i < num_processes; i++) {
        	 int row_start = i * rows_per_process;
        	 int row_end = row_start + rows_per_process - 1;
        	 read(pipes[i][0], shared + row_start * COLS, rows_per_process * COLS * sizeof(int));
        	 close(pipes[i][0]);
    	}	

    	for (i = 0; i < ROWS; i++) {
        	for (j = 0; j < COLS; j++) {
        	    grid[i][j] = shared[i * COLS + j];
        	}
    	}	

    	shmdt(shared);
    	shmctl(shmid, IPC_RMID, NULL);

    	printf("\nGrid of Generation # %d\n", itr);   	
    	print();
    	}
    	return 0;
}
