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
#define HEADER_SIZE sizeof(struct msg)

enum msg_type {info, chat, game, choice, bye};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct clients_description {
	int index;
	pthread_t thread_ID;
	int sockfd;
	int partner_ID;
	char name[NAME_SIZE];
} clients_t;

clients_t clients_struct[MAX];

struct msg {
	int type;
	int size;
	char buffer[0];
};

struct msg *Recv(int sockfd)
{
	struct msg *tmp;
	tmp = malloc(HEADER_SIZE);
	read(sockfd, tmp, HEADER_SIZE);
	tmp = realloc(tmp, HEADER_SIZE + tmp->size);
	read(sockfd, tmp->buffer, tmp->size);
	return tmp;
}

void Send_Msg(char *buf, int len, int type, int sockfd)
{
	struct msg *tmp;
	tmp = malloc(sizeof(struct msg) + len);
	tmp->size = len;
	tmp->type = type;
	strcpy(tmp->buffer, buf);
	write(sockfd, tmp, sizeof(struct msg) + len);
}

void initial_sessions()
{
	int i;
	for (i = 0; i < MAX; i++){
		clients_struct[i].index = i;
		clients_struct[i].sockfd = -1;
		clients_struct[i].partner_ID = -1;
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

int list_send(clients_t *my_client)
{
	int save_all;
	int i, len, count = 0;
	char buf_list[BUF_SIZE];
	char err[] = "There is no available connection\n";
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
		Send_Msg(err, 33, 0, my_client->sockfd);
		return 1;
	}
	save_all = open("save", O_RDONLY);
	len = read(save_all, (void *)buf_list, len * count);
	Send_Msg(buf_list, len, choice, my_client->sockfd);
	close(save_all);
	return 0;
}

void clients_choice_handler(clients_t *me, int cho)
{
	clients_struct[cho].partner_ID = me->sockfd;
	me->partner_ID = clients_struct[cho].sockfd;
}
				
void *threads_handler(void * session_index)
{
	struct msg *recvd = NULL;
	char convert[2];
	int cho;
	int client_s;
	client_s = *((int *)session_index);
	printf("client_s %d__sockfd__%d___index is %d\n", client_s, clients_struct[client_s].sockfd,
		   clients_struct[client_s].index);
	while(1){
		recvd = Recv(clients_struct[client_s].sockfd);
		if (recvd == 0){
		fprintf(stderr, "choosing error!!\n");
		break;
		}
		if (recvd > 0){
			if (recvd->type == info){
				strcpy(clients_struct[client_s].name, recvd->buffer);
				pthread_mutex_lock(&mutex);
				list_send(&clients_struct[client_s]);
				pthread_mutex_unlock(&mutex);
			}
			if (recvd->type == choice){
				convert[0] = recvd->buffer[0];
				convert[1] = '\0';
				cho = atoi(convert);
				pthread_mutex_lock(&mutex);
				clients_choice_handler(&clients_struct[client_s], cho);
				pthread_mutex_unlock(&mutex);
			}
			printf("___%d\n", clients_struct[client_s].partner_ID);
			if (recvd->type == bye){
				break;
			}
		}
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
