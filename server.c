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
#include <fcntl.h>
#include <sys/stat.h>

#define BUF_SIZE 1024
#define MAX 15
#define NAME_SIZE 20

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct clients_description {
	int index;
	pthread_t thread_ID;
	int sockfd;
	int partner_ID;
	int busy;
} clients_t;

clients_t clients_struct[MAX];

void initial_sessions()
{
	int i;
	for (i = 0; i < MAX; i++){
		clients_struct[i].index = i;
		clients_struct[i].sockfd = -1;
		clients_struct[i].partner_ID = -1;
		clients_struct[i].busy = 1;
	}
}

int search_available()
{
	int i;
	for (i = 0; i < MAX; i++){
		if (clients_struct[i].sockfd == -1){
			return i;
		}
	}
	return -1;
}
/*
void send_to_all(char *buf)
{
	int i;
	for (i = 0; i < MAX; i++){
		if (clients_struct[i].sockfd != -1){
			printf("just send to__%d this string__%s\n", clients_struct[i].sockfd, buf);
			write(clients_struct[i].sockfd, buf, BUF_SIZE);
		}
	}
}
*/
int list_send(clients_t *my_client)
{
	int save_all;
	int i, len, count = 0;
	char buf_list[BUF_SIZE];
	save_all = creat("save", S_IRUSR|S_IWUSR|S_IRGRP);
	for (i = 0; i < MAX; i++){
		if (clients_struct[i].sockfd != -1 && clients_struct[i].partner_ID == -1 &&
		    my_client->index != i){
				len = sprintf(buf_list, "You can join: %d\n", i);
				write(save_all, (void *)buf_list, len);
				bzero((char *)buf_list, sizeof(buf_list[0]) * BUF_SIZE);
				count++;
		}
	}
	close(save_all);
	if (count == 0){
		send(my_client->sockfd, "There is no available connection\n", 33, 0);
		return 1;
	}
	save_all = open("save", O_RDONLY);
	len = read(save_all, (void *)buf_list, len * count);
	send(my_client->sockfd, buf_list, len, 0);
	close(save_all);
	return 0;
}

void clients_choise_handler(clients_t *me, int choise)
{
	clients_struct[choise].partner_ID = me->sockfd;
	me->partner_ID = clients_struct[choise].sockfd;
	printf("мой sockfd у моего партнера_%d sock моего партнера_%d\n\n", 
			clients_struct[choise].partner_ID, me->partner_ID);
	me->busy = 0;
	clients_struct[choise].busy = 0;
	
}
	
			
void *threads_handler(void * session_index)
{
	int recvd, choise;
	char buf[BUF_SIZE];
	char convert[2];
	int client_s;
	client_s = *((int *)session_index);
	printf("client_s %d__sockfd__%d___index is %d\n", client_s, clients_struct[client_s].sockfd,
		   clients_struct[client_s].index);
	bzero((char *)buf, sizeof(buf[0]) * BUF_SIZE);
	while(clients_struct[client_s].busy){
		recvd = recv(clients_struct[client_s].sockfd, buf, sizeof(buf), 0);
		if (recvd <= 0){
		fprintf(stderr, "choosing error!!\n");
		break;
	}
		if (recvd > 0){
			if ((strcmp(buf, "LIST")) == 0){
				pthread_mutex_lock(&mutex);
				list_send(&clients_struct[client_s]);
				pthread_mutex_unlock(&mutex);
			}
			if ((strncmp("CH ", buf, 3)) == 0){
				convert[0] = buf[3];
				convert[1] = '\0';
				choise = atoi(convert);
				pthread_mutex_lock(&mutex);
				clients_choise_handler(&clients_struct[client_s], choise);
				pthread_mutex_unlock(&mutex);
				break;
			}
			bzero((char *)buf, sizeof(buf[0]) * BUF_SIZE);
		}
	}
	sprintf(buf, "hey i am your partner: %d\n", clients_struct[client_s].sockfd);
	send(clients_struct[client_s].partner_ID, buf, 26, 0);
	while (1)
	{
		recvd = recv(clients_struct[client_s].sockfd, buf, sizeof(buf), 0);
		if (recvd <= 0){
			fprintf(stderr, "error recvd < 0!!!\n");
			break;
		}
		
		if (recvd > 0){
			printf("\n");
			printf("got it__%s__from__%d\n", buf, clients_struct[client_s].partner_ID);
			//pthread_mutex_lock(&mutex);
			send(clients_struct[client_s].partner_ID, buf, sizeof(buf), 0);
			//pthread_mutex_unlock(&mutex);
		}
		bzero((char *)buf, sizeof(buf[0]) * BUF_SIZE);
	}
		close(clients_struct[client_s].sockfd);
		clients_struct[client_s].sockfd = -1;
		clients_struct[client_s].partner_ID = -1;
		pthread_exit(NULL);		
	
}

int main(int argc, char *argv[])
{
	int sock, connection, port, aid;
	int cur_index;
	struct sockaddr_in server, client;
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
	
	initial_sessions();
	while(1){
		cur_index = search_available();
		if ((connection = accept(sock, (struct sockaddr *)&client, &len)) > 0){
			clients_struct[cur_index].sockfd = connection;
			printf("we got client: IP %s___assign to session %d__\n", inet_ntoa(client.sin_addr),
			       cur_index);
			if (pthread_create(&(clients_struct[cur_index].thread_ID), NULL, 
				&threads_handler, &(clients_struct[cur_index].index)) > 0){
				printf("pthread_create() error!\n");
				break;
			}
		} else {
			printf("accept() error!\n");
			break;
		}
	}
	remove("save");	
	close(connection);
	close(sock);
	return 0;
}
