#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<fcntl.h>

#define PORT 8080

void *handle_client(void *arg);
int validate_user(const char *id,const char *pwd,char *role);

int main()
{
	int server_fd,new_socket;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

	server_fd = socket(AF_INET,SOCK_STREAM,0);
	setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	bind(server_fd,(struct sockaddr *)&address,sizeof(address));
	listen(server_fd,5);

	printf("Server started on port %d\n",PORT);

	while(1){
		new_socket = accept(server_fd,(struct sockaddr *)&address,(socklen_t *)&addrlen);
		if(new_socket<0)
		{
			perror("accept");
			continue;
		}
		printf("Client Connected: %s:%d\n",inet_ntoa(address.sin_addr),ntohs(address.sin_port));

		pthread_t tid;
		int *client_sock = malloc(sizeof(int));
		*client_sock = new_socket;
		pthread_create(&tid,NULL,handle_client,client_sock);
		pthread_detach(tid);
	}
	close(server_fd);
	return 0;
}
void *handle_client(void *arg){
	int sock = *(int *)arg;
	free(arg);
	char buffer[256],role[32];
	char id[32],pwd[32];

	read(sock,id,sizeof(id));
	read(sock,pwd,sizeof(role));

	if(validate_user(id,pwd,role))
		write(sock,role,sizeof(role));
	else
		write(sock,"INVALID",8);

	close(sock);
	return NULL;
}

int validate_user(const char *id,const char *pwd,char *role)
{
	int fd = open("data/users.dat",O_RDONLY);
	if(fd<0)
		return 0;
	char line[128],file_id[32],file_pwd[32],file_role[32];
	FILE *fp = fdopen(fd,"r");
	while(fgets(line,sizeof(line),fp)){
		sscanf(line,"%[^:]:%[^:]:%s",file_id,file_pwd,file_role);
		if(!strcmp(id,file_id) && !strcmp(pwd,file_pwd)){
			strcpy(role,file_role);
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

