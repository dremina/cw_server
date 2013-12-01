#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define HEADER_SIZE sizeof(struct msg)

enum msg_type {info, chat, game, choice, bye};

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

void GetName (char name[]) {
	//bc_box(10, 5, 40, 2);
	//mt_gotoXY(23, 5);
	printf("Enter your name:\n");
//	mt_gotoXY(12, 6);
	scanf ("%s", name);
	
}

int main (int argc, char *argv[]) {

	int len, 
		sockfd, 
		cn;
	char *ip = "127.0.0.1";
	struct msg *recvd = NULL;
	struct sockaddr_in server;
	char name[20];
	char number_game[2];

 /*	if (argc != 2){
    	printf("Error. There are not arguments \n"); 
        exit(1);
    }*/
   // port = atoi(argv[1]);

//	signal(SIGALRM, alarm_handler);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) { 
		perror("socket"); 
		exit(-1); 
	}

	bzero(&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(7777);
	inet_aton(ip, &(server.sin_addr));

	cn = connect(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
	if (cn < 0) {
		perror("connect");
		close(sockfd);
		exit(-1);
	}
	printf("Connect completed.\n");

	//mt_clrscr();
	//GetName(name);
	printf("Enter your name:\n");
	scanf ("%s", name);
	len = strlen(name);
	Send_Msg(name, len, info, sockfd);
	//mt_clrscr();

	while(1){
		recvd = Recv(sockfd);
		if (recvd > 0) {
			if (recvd->type == choice){
				printf("%s\n", recvd->buffer);
				printf("____\n");
				scanf("%s", number_game);
				len = strlen(number_game);
				Send_Msg(number_game, len, choice, sockfd);
			}	
			break;
		}			
	}
	sleep(20);
	close(sockfd);
	return 0;
}
