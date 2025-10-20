#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>

int main()
{
	int fd = open("data/accounts.dat",O_RDWR | O_CREAT,06444);
	if(fd < 0)
	{
		perror("open");
		return 1;
	}

	char record[] = "1001,5000.00\n";
	write(fd,record,strlen(record));

	lseek(fd,0,SEEK_SET);
	char buf[64];
	int n = read(fd,buf,sizeof(buf)-1);
	buf[n] = '\0';

	printf("File Contents:\n%s\n",buf);
	close(fd);
	return 0;
}
