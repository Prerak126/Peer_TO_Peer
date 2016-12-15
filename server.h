#ifndef server_h
#define server_h

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <map>
#include <pthread.h>
#include <mutex>
#include <iostream>
#include <csignal>

using namespace std;

#define PORT "8888"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAXTHREADS 100

#define TTL 30

#define MAXDATASIZE 100

struct info
{
	int sock_fd;
	string ip;
};

std::map<string, info> clientMap;
std::map<string, int> threadMap;
pthread_t threads[MAXTHREADS];
mutex mtx;

//for recv timeout
struct timeval tv;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}


void *threadForClient(void* s)
{
	string client_ip = (char*)s;
	char buf[5];

	// string table;
	int rv=0;
	while((rv = recv(clientMap[client_ip].sock_fd, buf, 5, 0))>0)
	{
		string table;
		if(strcmp (buf, "PING")==0)
		{
			
			cout<<"PING from"<<client_ip<<endl;
			
			if(send(clientMap[client_ip].sock_fd, "ACK", 3, 0) <0)
			{
				fprintf(stderr,"Error in sending ping ACK to %s\n",clientMap[client_ip].ip.c_str());
			}
		}
		if(strcmp (buf, "LIST")==0)
		{
			mtx.lock();
			for(auto it=clientMap.begin();it!=clientMap.end();it++)
			{
				table += it->second.ip;
				table += "\n";
			}
			mtx.unlock();

			if(send(clientMap[client_ip].sock_fd, table.c_str(), MAXDATASIZE-1, 0) < 0)
			{
				fprintf(stderr,"Error in sending client table to %s\n",clientMap[client_ip].ip.c_str());
			}
		}
	}

	if(rv<=0)		//recv timedout implies client no longer alive
	{
		cout<<"Client "<<client_ip<<"unreachable. It will be disconnected!"<<endl;
		mtx.lock();
		close(clientMap[client_ip].sock_fd);
		clientMap.erase(client_ip);
		mtx.unlock();
		pthread_exit(NULL);
	}
}

#endif
