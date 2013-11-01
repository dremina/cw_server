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


int main(int argc, char *argv[])
{
	int sock, connection, port, aid, MAX_CLIENT;
	struct sockaddr_in server, client;
	socklen_t len = sizeof(struct sockaddr_in);
	char ch[10];
	char buf[30] = "oook";
	if (argc != 3){
		printf("мало аргументов командной строки!\n"); //первый аргумент порт, второй max_client
		exit(1);
	}
	port = atoi(argv[1]); // преобразуем в 4 байтовое целое (int)
	MAX_CLIENT = atoi(argv[2]);
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (socket < 0){
		printf("socket() fail\n");
		exit(1);
	}
	
	bzero((char *)&server, len);
	server.sin_family = AF_INET; // domain для удаленной оконченной точки
	server.sin_addr.s_addr = htonl(INADDR_ANY); //может принимать соединение с любого сетевого интерфейса.
	server.sin_port = htons(port); // сетевой порядок байтов. - htons, устанавливаем порт
	
	aid	= bind(sock, (struct sockaddr *)&server, len); //связываем инф. о конечной локальной точке
	if (aid == -1){
		printf("bind() fail\n");
		exit(1);
	}
	
	aid = listen(sock, MAX_CLIENT);
	if (aid == -1){
		printf("listen () fail\n");
		exit(1);
	} 
	connection = accept(sock, (struct sockaddr *)&client, &len);
	printf("client %s\n", inet_ntoa(client.sin_addr));
	if (connection < 0){
		printf("accept() fail\n");
		exit(1);
	}	
	read(connection, &ch, sizeof(ch));
	printf("client sent me  %s\n", ch);
	write(connection, &buf, 4);
	/*while(1){
		connection = accept(socket, (struct sockaddr *)&client, &len);
		if (connection > 0){
			printf("client %s\n", inet_ntoa(client.sin_addr));
		} else {
			perror("accept");
			exit(1);
		}	
	} */
	close(connection);
	close(sock);
	return 0;
}
	
