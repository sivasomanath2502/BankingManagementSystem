#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<unistd.h>

union semun{
	int val;
};

int main()
{
	key_t key = ftok("data",75);
	int shmid = shmget(key,sizeof(int),0666 | IPC_CREAT);
	int semid = semget(key,1,0666 | IPC_CREAT);
	union semun u;
	u.val = 1;
	semctl(semid,0,SETVAL,u);

	int *balance = (int *)shmat(shmid,NULL,0);
	*balance = 1000;

	struct sembuf P = {0,-1,0};
	struct sembuf V = {0,-1,0};

	for(int i=0;i<5;i++){
		semop(semid,&P,1);
		*balance += 100;
		printf("Deposit done, Balance=%d\n",*balance);
		semop(semid,&V,1);
		sleep(1);
	}

	shmdt(balance);
	shmctl(shmid,IPC_RMID,NULL);
	semctl(semid,0,IPC_RMID);
	return 0;
}
