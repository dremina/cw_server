#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>


int main(int argc, char* argv[])
{
    int cfd, i;
    int recvd;
    struct sockaddr_in addr;
    char ch[1024];
    cfd=socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); /* Check for server on loopback */
    addr.sin_port=htons(29008);
    if(connect(cfd, (struct sockaddr *)&addr,
    sizeof(addr))<0) {
        perror("connect error");
        return -1;
    }
    while (1){
		// инфа серверу.
		i = 0;
		printf("__\n");
		do {
			scanf("%c", &ch[i]);
			i++;
		} while (ch[i - 1] != '\n');
		ch[i - 1] = '\0';
	
		if (write(cfd, ch, i) != i){
			fprintf(stderr, "writing error from client\n");
			return -1;
		}
		bzero((char *)ch, sizeof(ch[0]) * 1024);  
		recvd = recv(cfd, ch, sizeof(ch), 0); 
		if (recvd > 0){
			printf("Reply from Server: %s\n",ch);
		}
		bzero((char *)ch, sizeof(ch[0]) * 1024);  
		
	// if(write(cfd, &ch, 2)<0) perror("write");
	//	if(recv(cfd, (void *)ch, sizeof(ch), 0) < 0) perror("read");
		//printf("Reply from Server: %s\n",ch);	
		}
		close(cfd);
		return 0;
}
