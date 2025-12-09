#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_DAD_SLEEP 5
#define MAX_STUDENT_SLEEP 5
#define MAX_MOM_SLEEP 10

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Usage: %s <numParents:1|2> <numStudents>\n", argv[0]);
        exit(1);
    }

    int numParents = atoi(argv[1]);   // 1 = Dad only, 2 = Dad + Mom
    int numStudents = atoi(argv[2]);

    int ShmID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (ShmID < 0) {
        perror("shmget error");
        exit(1);
    }

    int *BankAccount = (int *) shmat(ShmID, NULL, 0);
    if (BankAccount == (int *) -1) {
        perror("shmat error");
        exit(1);
    }

    *BankAccount = 0;

    sem_t *mutex = sem_open("bank_sem", O_CREAT, 0644, 1);
    if (mutex == SEM_FAILED) {
        perror("semaphore initialization");
        exit(1);
    }

    srand(time(NULL));

    pid_t pid;

    if (numParents == 2) {

        pid = fork();
        if (pid == 0) {
            // MOM PROCESS
            srand(time(NULL) ^ (getpid() << 16));

            while (1) {
                sleep(rand() % (MAX_MOM_SLEEP + 1));

                printf("Lovable Mom: Attempting to Check Balance\n");

                sem_wait(mutex);

                int localBalance = *BankAccount;

                if (localBalance <= 100) {

                    int amount = rand() % 125; // 0-124

                    localBalance += amount;

                    printf("Lovable Mom: Deposits $%d / Balance = $%d\n",
                           amount, localBalance);

                    *BankAccount = localBalance;
                }

                sem_post(mutex);
            }
            exit(0);
        }
    }

    pid = fork();
    if (pid == 0) {
        // DAD PROCESS
        srand(time(NULL) ^ (getpid() << 16));

        while (1) {
            sleep(rand() % (MAX_DAD_SLEEP + 1));

            printf("Dear Old Dad: Attempting to Check Balance\n");

            sem_wait(mutex);

            int localBalance = *BankAccount;
            int r = rand();

            if (r % 2 == 0) {
                if (localBalance < 100) {

                    int amount = rand() % 101; // 0–100

                    if (amount % 2 == 0) {
                        localBalance += amount;
                        printf("Dear old Dad: Deposits $%d / Balance = $%d\n",
                               amount, localBalance);
                        *BankAccount = localBalance;
                    } else {
                        printf("Dear old Dad: Doesn't have any money to give\n");
                    }

                } else {
                    printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n",
                           localBalance);
                }
            } else {
                printf("Dear Old Dad: Last Checking Balance = $%d\n",
                       localBalance);
            }

            sem_post(mutex);
        }
        exit(0);
    }

    for (int s = 0; s < numStudents; s++) {

        pid = fork();
        if (pid == 0) {

            int id = s + 1; // student numbering 1..N
            srand(time(NULL) ^ (getpid() << 16));

            while (1) {
                sleep(rand() % (MAX_STUDENT_SLEEP + 1));

                printf("Poor Student(%d): Attempting to Check Balance\n", id);

                sem_wait(mutex);

                int localBalance = *BankAccount;
                int r = rand();

                if (r % 2 == 0) {

                    int need = rand() % 51;

                    printf("Poor Student(%d) needs $%d\n", id, need);

                    if (need <= localBalance) {
                        localBalance -= need;
                        printf("Poor Student(%d): Withdraws $%d / Balance = $%d\n",
                               id, need, localBalance);
                    } else {
                        printf("Poor Student(%d): Not Enough Cash ($%d)\n",
                               id, localBalance);
                    }

                    *BankAccount = localBalance;

                } else {
                    printf("Poor Student(%d): Last Checking Balance = $%d\n",
                           id, localBalance);
                }

                sem_post(mutex);
            }
            exit(0);
        }
    }

    // Parent waits forever
    while (1)
        pause();

    return 0;
}
