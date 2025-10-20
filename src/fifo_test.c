#include<stdio.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>

int main()
{
	mkfifo("data/bankfifo",0666);
	if(fork() == 0){
		int fd = open("data/bankfifo",O_WRONLY);
		write(fd,"Deposit 500\n",12);
		close(fd);
	}
	else{
		char buf[32];
		int fd = open("data/bankfifo",O_RDONLY);
		read(fd,buf,sizeof(buf));
		printf("Received: %s\n",buf);
		close(fd);
	}
	unlink("data/bankinfo");
	return 0;
}


