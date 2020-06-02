
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <mysql.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/time.h>

using namespace std;
int iNum;

// mysql 连接
int connectToMysql(){
	MYSQL * conn = mysql_init(NULL);
	conn = mysql_real_connect(conn, "127.0.0.1", "root", "root", "game", 0, NULL, 0);
	if (!conn){
		printf("connect mysql error \n");
		return -1;
	}
	printf("connect mysql success\n");
	int res = mysql_query(conn, "select * from account");
	if (res){
		printf("mysql_query error %s\n", mysql_error(conn));
		return -1;
	}else{
		MYSQL_RES* result = mysql_store_result(conn);
		if (result){
			for(int i = 0; i < mysql_num_rows(result); i++){  // 结果行
				MYSQL_ROW row = mysql_fetch_row(result);
				for (int j = 0; j < mysql_num_fields(result); j++){ // 结果列
					printf("%s \t", row[j]);
				}
				printf("\n");
			}
		}
		mysql_free_result(result);
	}
	mysql_close(conn);
	return 0;
}

struct clientInfo
{	
	int clientfd;
	char* sIp;
};

// 用来接受某个clinet的消息和返回给它收到了
void* recvThread(void* arg){
	printf("recvThread start \n");
	struct clientInfo * temp;
	temp = (struct clientInfo *) arg;
	
	int clientfd = temp->clientfd;
	char * sClientIp = temp->sIp;
	char recvBuff[255];
	char byeBuff[] = "bye";

	fd_set rfds;
	struct timeval tv;
	int retval, maxfd;
	while(1){
		// 把可读文件描述符集合清空
		FD_ZERO(&rfds);
		// 把标准输入的文件描述符加入到集合中
		FD_SET(0, &rfds);
		// 找出文件描述符集合中最大的文件描述符
		maxfd = 0;
		FD_SET(clientfd, &rfds);
		if (maxfd < clientfd){
			maxfd = clientfd;
		}
		// 设置超时时间
		tv.tv_sec = 5; // 倒计时
		tv.tv_usec = 0;
		// 等待聊天
		retval = select(maxfd + 1, &rfds, NULL, NULL, &tv);
		if (retval == -1){
			printf("select error \n");
			break;
		}else if (retval == 0){
			printf("服务端没有输入 客户端也没有信息 等待，，，\n");
			continue;
		}else{
			// 客户端发来消息
			if(FD_ISSET(clientfd, &rfds)){
				bzero(recvBuff, sizeof(recvBuff));
				int n = recv(clientfd, recvBuff, sizeof(recvBuff)-1, NULL);
				if (n > 0){
					printf("sClientIp = %s recv msg = %s\n", sClientIp, recvBuff);
				}
				if(strcmp(recvBuff, byeBuff) == 0){
					printf(" ip = %s disconnect\n", sClientIp);
					char bye[] = "bye";
					send(clientfd, bye, strlen(bye), NULL);
					close(clientfd);
					break;
				}
			}
			// 输入消息
			if(FD_ISSET(0, &rfds)){
				char buf[1024];
				fgets(buf, sizeof(buf), stdin);
				send(clientfd, buf, sizeof(buf), 0);
			}
		}
	}
	return 0;
}

int main()
{	
	// 连接客户端
	int iRes = connectToMysql();
	// scoket准备
	const char* ip = "127.0.0.1";
	int port = 7766;
	int sockfd = socket(PF_INET, SOCK_STREAM, 0); // 创建一个套接字
	if (sockfd < 0){
		printf("create sockfd faild\n");
		return -1;
	}
	printf("create sockfd suss\n");

	struct sockaddr_in serv_addr; // 一般是存储地址和ip 用于信息的显示及存储使用
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port); // 转换为网络字节序，即大端模式
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 将主机的无符号长整形数转换为网络字节序 INADDR_ANY就是0.0.0.0 表示不确定地址 或任意地址 
	if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		printf("bind sockfd faild\n");
		return -1;
	}
	printf("bind sockfd suss\n");
	listen(sockfd, 1024);

	// 等待客户端连接 并为每个客户端创建一个线程用来接受消息
	while(1){

		struct sockaddr_in client_addr;
		socklen_t client_addrlength = sizeof(client_addr);
		int clientfd;
		clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addrlength);
		if(clientfd < 0){
			printf("accept faild\n");
		}
		char* sClientIp = inet_ntoa(client_addr.sin_addr);
		printf("clientfd connect %s \n", sClientIp);
		// 线程函数参数结构体；
		struct clientInfo* cInfo;
		cInfo = (struct clientInfo*)malloc(sizeof(struct clientInfo));
		cInfo->clientfd = clientfd;
		cInfo->sIp = sClientIp;
		pthread_t thredId;
		// 创建一个线程用来收这个客户端消息
		if( pthread_create(&thredId, NULL, recvThread, (void*)cInfo) != 0 ){
			printf("create thread faild\n");
			exit(0);
		}
	}
	close(sockfd);
	return 0;
}




