#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#define BUF_SIZE 1024
#define MAX 15
#define NAME_SIZE 20

pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct clients_description {
	int index;
	pthread_t thread_ID;
	int sockfd;
	char name[NAME_SIZE + 1];
} clients_t;

clients_t clients_struct[MAX];

void initial_sessions()
{
	int i;
	for (i = 0; i < MAX; i++){
		clients_struct[i].index = i;
		clients_struct[i].sockfd = -1;
	}
}

int search_availiable()
{
	int i;
	for (i = 0; i < MAX; i++){
		if (clients_struct[i].sockfd == -1){
			return i;
		}
	}
	return -1;
}

void send_to_all(clients_t *sender, char *buf)
{
	int i;
	for (i = 0; i < MAX; i++){
		if (clients_struct[i].sockfd != -1){
			printf("send:%d\n", sender->index);
			write(clients_struct[i].sockfd, buf, BUF_SIZE);
		}
	}
}
			
void *threads_handler(void * session_index)
{
	int recvd;
	char buf[BUF_SIZE];
	//int isconnected = 1;
	int client_s;
	client_s = *((int *)session_index);
	printf("session %d__fd__%d\n", client_s, clients_struct[client_s].sockfd);
	
	bzero((char *)buf, sizeof(buf[0]) * BUF_SIZE);
	while(1){
		recvd = recv(clients_struct[client_s].sockfd, buf, sizeof(buf), 0);
		/*if (!recvd){
			continue;
			//fprintf(stderr, "error recvd!!\n");
			//isconnected = 0;
			//close(clients_struct[client_s].sockfd);
			//break;
		//}*/
		if (recvd > 0){
			printf("got it__%s\n", buf);
			pthread_mutex_lock(&send_mutex);
			send_to_all(&clients_struct[client_s], buf);
			pthread_mutex_unlock(&send_mutex);
		}
		bzero((char *)buf, sizeof(buf[0]) * BUF_SIZE);
	}
		close(clients_struct[client_s].sockfd);
		pthread_exit(NULL);		
}

int main(int argc, char *argv[])
{
	int sock, connection, port, aid;
	int cur_index;
	struct sockaddr_in server, client;
	//pthread_t threads;
	socklen_t len = sizeof(struct sockaddr_in);
	if (argc != 3){
		printf("мало аргументов командной строки!\n"); //первый аргумент порт, второй max_client
		exit(1);
	}
	port = atoi(argv[1]); // преобразуем в 4 байтовое целое (int)
//	MAX_CLIENT = atoi(argv[2]);
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (socket < 0){
		printf("socket() fail\n");
		exit(1);
	}
	
	bzero((char *)&server, len);
	server.sin_family = AF_INET; // domain для удаленной оконечной точки
	server.sin_addr.s_addr = htonl(INADDR_ANY); //может принимать соединение с любого сетевого интерфейса.
	server.sin_port = htons(port); // сетевой порядок байтов. - htons, устанавливаем порт
	
	aid	= bind(sock, (struct sockaddr *)&server, len); //связываем инф. о оконечной локальной точке
	if (aid == -1){
		printf("bind() fail\n");
		exit(1);
	}
	
	aid = listen(sock, MAX);
	if (aid == -1){
		printf("listen () fail\n");
		exit(1);
	} 
	/*connection = accept(sock, (struct sockaddr *)&client, &len);
	printf("we got client: IP %s\n", inet_ntoa(client.sin_addr));
	if (connection < 0){
		printf("accept() fail\n");
		exit(1);
	}	
	read(connection, &ch, sizeof(ch)); 
	printf("client sent me  %s\n", ch);
	write(connection, &buf, 4);*/
	initial_sessions();
	while(1){
		cur_index = search_availiable();
		if ((connection = accept(sock, (struct sockaddr *)&client, &len)) > 0){
			clients_struct[cur_index].sockfd = connection;
			printf("we got client: IP %s___assign to session %d__\n", inet_ntoa(client.sin_addr),
			       cur_index);
			if (pthread_create(&(clients_struct[cur_index].thread_ID), NULL, 
				&threads_handler, &(clients_struct[cur_index].index)) > 0){
				printf("pthread_create() error!\n");
				exit(1);
			}
		} else {
			printf("accept() error!\n");
			exit(1);
		}
		sleep(3);
	}
	close(connection);
	close(sock);
	return 0;
}
