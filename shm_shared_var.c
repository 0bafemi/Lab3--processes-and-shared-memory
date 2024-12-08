#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define NUM_ITERATIONS 25

void ClientProcess(int *SharedMem);

int main() {
    int ShmID;
    int *ShmPTR;
    pid_t pid;
    int status;

    // Create shared memory for 2 integers
    ShmID = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0666);
    if (ShmID < 0) {
        perror("*** shmget error (server) ***");
        exit(1);
    }
    printf("Server has received a shared memory segment...\n");

    ShmPTR = (int *)shmat(ShmID, NULL, 0);
    if (ShmPTR == (int *)-1) {
        perror("*** shmat error (server) ***");
        exit(1);
    }
    printf("Server has attached the shared memory...\n");

    // Initialize BankAccount and Turn
    ShmPTR[0] = 0; // BankAccount
    ShmPTR[1] = 0; // Turn

    // Create the child process
    pid = fork();
    if (pid < 0) {
        perror("*** fork error (server) ***");
        exit(1);
    } else if (pid == 0) {
        ClientProcess(ShmPTR);
        exit(0);
    }

    // Parent process (Dear Old Dad)
    int i;
    for (i = 0; i < NUM_ITERATIONS; i++) {
        sleep(rand() % 6); // Sleep for 0-5 seconds
        int account = ShmPTR[0];

        // Strict alternation - wait for turn
        while (ShmPTR[1] != 0);

        if (account <= 100) {
            int depositAmount = rand() % 101; // Random deposit amount [0-100]
            if (depositAmount % 2 == 0) {
                account += depositAmount;
                printf("Dear old Dad: Deposits $%d / Balance = $%d\n", depositAmount, account);
            } else {
                printf("Dear old Dad: Doesn't have any money to give\n");
            }
        } else {
            printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", account);
        }

        ShmPTR[0] = account; // Update the BankAccount
        ShmPTR[1] = 1;       // Set Turn to child
    }

    // Wait for child process to finish
    wait(&status);
    printf("Server has detected the completion of its child...\n");

    // Cleanup
    shmdt((void *)ShmPTR);
    printf("Server has detached its shared memory...\n");
    shmctl(ShmID, IPC_RMID, NULL);
    printf("Server has removed its shared memory...\n");
    printf("Server exits...\n");
    exit(0);
}

void ClientProcess(int *SharedMem) {
    printf("   Client process started\n");

    int i;
    for (i = 0; i < NUM_ITERATIONS; i++) {
        sleep(rand() % 6); // Sleep for 0-5 seconds
        int account = SharedMem[0];
        int balanceNeeded = rand() % 51; // Random withdrawal amount [0-50]
        printf("   Poor Student needs $%d\n", balanceNeeded);

        // Strict alternation - wait for turn
        while (SharedMem[1] != 1);

        // Withdrawal process
        if (balanceNeeded <= account) {
            account -= balanceNeeded;
            printf("   Poor Student: Withdraws $%d / Balance = $%d\n", balanceNeeded, account);
        } else {
            printf("   Poor Student: Not Enough Cash ($%d)\n", account);
        }

        SharedMem[0] = account; // Update the BankAccount
        SharedMem[1] = 0;       // Set Turn to parent
    }

    printf("   Client process is about to exit\n");
}
