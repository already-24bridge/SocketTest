#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/time.h>
#include <list>
#include <map>

using namespace std;

int sockfd;
socklen_t len;
struct sockaddr_in serv_addr;
std::list<int> li; // list 用来存放套接字 这样可以连接多个client
std::list<int> rli; // 移除连接的client
std::map<int, string> tNameMap; // [clientfd] = name

void* getConn(void* arg){
	while(1){
		int conn = accept(sockfd, (struct sockaddr*)&serv_addr, &len);
		li.push_back(conn);
		// printf("conn %d\n", conn);
		char sName[255];
		int n = recv(conn, sName, sizeof(sName)-1, 0);
		string sTemp = sName;
		sTemp.erase(sTemp.end() -1); // 删除最后一个换行符
		tNameMap[conn] = sTemp;
		printf("conn ect %d name = %s\n",conn, sTemp.c_str());
	}
}

void* recvData(void* arg){
	struct timeval tv;
	tv.tv_sec = 15;
	tv.tv_usec = 0;
	while(1){
		std::list<int>::iterator it;
		for (it=li.begin(); it!=li.end(); ++it){
			fd_set rfds;
			FD_ZERO(&rfds);
			int maxfd = 0;
			int retval = 0;
			FD_SET(*it, &rfds);
			if (maxfd < *it){
				maxfd = *it;
			}
			retval = select(maxfd + 1, &rfds, NULL, NULL, &tv);
			if(retval == -1){
				printf("select errror\n" );
			}else if(retval == 0) {
				// printf("no message waiting\n");
			}else {
				char buff[1024];
				bzero(buff, sizeof(buff)); // memset(buff, 0, sizeof(buff));
				int len = recv(*it, buff, sizeof(buff)-1, 0);
				char byeBuff[] = "bye";
				if (len > 0){
					printf("recv from[%s] mes = %s \n", tNameMap[*it].c_str(), buff );
					if (strncmp(buff, byeBuff, 3) == 0 ){ // 这里不使用 strcmp是因为buff的长度是1024 最后是'bye\0' 跟发送来的bye比较值不等 
						printf("recv %d req close\n", *it);
						rli.push_back(*it);
					}
				}
			}
		}
		for (it = rli.begin(); it != rli.end(); ++it){
			char buff[] = "bye";
			printf("send %d bye\n", *it );
			send(*it, buff, sizeof(buff), 0);
			li.remove(*it);
			tNameMap.erase(*it);
			close(*it);
		}
		rli.clear();
		sleep(1);
	}
}

void* sendMess(void* arg){
	while(1){
		char buf[1024];
		fgets(buf, sizeof(buf), stdin);
		std::list<int>::iterator it;
		for (it=li.begin(); it!=li.end(); ++it){
			send(*it, buf, sizeof(buf), 0);
			printf("send %d msg:%s\n", *it, buf);
		}
	}
}

int main()
{	
	// scoket准备
	const char* ip = "127.0.0.1";
	int port = 7766;
	sockfd = socket(PF_INET, SOCK_STREAM, 0); // 创建一个套接字
	if (sockfd < 0){
		printf("create sockfd faild\n");
		return -1;
	}
	printf("create sockfd suss\n");

	memset(&serv_addr, 0, sizeof(serv_addr)); // 一般是存储地址和ip 用于信息的显示及存储使用
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port); // 转换为网络字节序，即大端模式
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 将主机的无符号长整形数转换为网络字节序 INADDR_ANY就是0.0.0.0 表示不确定地址 或任意地址 
	if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		printf("bind sockfd faild\n");
		return -1;
	}
	printf("bind sockfd suss\n");
	listen(sockfd, 1024);
	len = sizeof(serv_addr);

	pthread_t connThreadId;
	if( pthread_create(&connThreadId, NULL, getConn, NULL) != 0 ){
			printf("create connect thread faild\n");
			exit(0);
	}
	pthread_detach(connThreadId); // 分离线程 将此线程同当前进程分离，使其成为一个独立线程。
	
	pthread_t sendThreadId;
	if( pthread_create(&sendThreadId, NULL, sendMess, NULL) != 0 ){
			printf("create send thread faild\n");
			exit(0);
	}
	pthread_detach(sendThreadId);

	pthread_t recvThreadId;
	if( pthread_create(&recvThreadId, NULL, recvData, NULL) != 0 ){
			printf("create recv thread faild\n");
			exit(0);
	}
	pthread_detach(recvThreadId);
	while(1){

	}
	close(sockfd);
	return 0;
}
