#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/shm.h>
#include <fcntl.h>
using namespace std;
#define BUFFER_SIZE 1024

int main()
{	
	fd_set rfds;
    struct timeval tv;
    int retval, maxfd;

	const char* ip = "127.0.0.1";
	int port = 7766;
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	int sock_cli = sockfd;
	if (sockfd < 0){
		printf("crate sockfd failed\n");
		return -1;
	}
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port); // 服务端口号
	serv_addr.sin_addr.s_addr = inet_addr(ip); // 服务ip inet_addr用于IPv4的ip转换 即十进制转二进制
	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		printf("connect failed\n");
		return -1;
	}
	printf("connect success \n");
	printf("please send your name\n");
	char sName[255];
	fgets(sName, sizeof(sName), stdin);
	send(sockfd, sName, sizeof(sName), 0);
	while(1){

		/*把可读文件描述符的集合清空*/
        FD_ZERO(&rfds);
        /*把标准输入的文件描述符加入到集合中*/
        FD_SET(0, &rfds);
        maxfd = 0;
        /*把当前连接的文件描述符加入到集合中*/
        FD_SET(sock_cli, &rfds);
        /*找出文件描述符集合中最大的文件描述符*/   
        if(maxfd < sock_cli)
            maxfd = sock_cli;
        /*设置超时时间*/
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        /*等待聊天*/
        retval = select(maxfd+1, &rfds, NULL, NULL, &tv);
        if(retval == -1)
        {
            printf("select出错，客户端程序退出\n");
            break;
        }
        else if(retval == 0)
        {
            // printf("客户端没有任何输入信息，并且服务器也没有信息到来，waiting...\n");
            continue;
        }
        else
        {
            /*服务器发来了消息*/
            if(FD_ISSET(sock_cli,&rfds))
            {
                char recvbuf[BUFFER_SIZE];
                int len;
                len = recv(sock_cli, recvbuf, sizeof(recvbuf)-1,0);
                if (len > 0){
	                printf("recv from server %s", recvbuf);
	                if (strncmp(recvbuf, "bye",3) ==0 ){
	                	break;
	                }
                	memset(recvbuf, 0, sizeof(recvbuf));
                }
            }
            /*用户输入信息了,开始处理信息并发送*/
            if(FD_ISSET(0, &rfds))
            {
                char sendbuf[BUFFER_SIZE];
                fgets(sendbuf, sizeof(sendbuf), stdin);
                send(sock_cli, sendbuf, strlen(sendbuf),0); //发送
                memset(sendbuf, 0, sizeof(sendbuf));
            }
        }
	}
	printf("end connect\n");
	close(sockfd);
	return 0;
}
