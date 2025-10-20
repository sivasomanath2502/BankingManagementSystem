#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>

#define PORT 8080

int main()
{
	int sock = socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	inet_pton(AF_INET,"127.0.0.1",&serv_addr.sin_addr);

	connect(sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr));

	char id[32],pwd[32],role[32];
	printf("User ID:");
	scanf("%s",id);
	printf("Password:");
	scanf("%s",pwd);
	
	write(sock,id,sizeof(id));
	write(sock,pwd,sizeof(pwd));

	read(sock,role,sizeof(role));
	if(strcmp(role,"INVALID") == 0)
		printf("Login Failed\n");
	else
		printf("Login successful Role: %s\n",role);

	close(sock);
	return 0;
}
