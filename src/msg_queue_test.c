#include<stdio.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<string.h>

struct msgbuf{
	long mtype;
	char mtext[100];
};

int main()
{
	key_t key = ftok("data",65);
	int msgid = msgget(key,0666 | IPC_CREAT);

	struct msgbuf msg;
	msg.mtype = 1;
	strcpy(msg.mtext,"Loan Application: CustID=1001 Amount=50000");
	msgsnd(msgid,&msg,sizeof(msg.mtext),0);
	printf("Sent: %s\n",msg.mtext);

	msgrcv(msgid,&msg,sizeof(msg.mtext),1,0);
	printf("Received: %s\n",msg.mtext);

	msgctl(msgid,IPC_RMID,NULL);
	return 0;
}
