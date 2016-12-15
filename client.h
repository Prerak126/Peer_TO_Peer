#ifndef client_h
#define client_h

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

using namespace std;

#define SERVERPORT "8888"  // the port users will be connecting to
#define CLIENTPORT "6666"	// port on which client listens for other peers' connections
#define PEERPORT "7777"		//port which a client uses to connect with other clients' listening port

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAXTHREADS 3

#define MAXDATASIZE 100

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"


pthread_t threads[MAXTHREADS]; //thread[0] for ping, thread[1] for peer send, thread[2] for peer rcv
mutex mtx;
FILE *f;
string argument;
bool pingAlive, sendAlive, rcvAlive;	//to check running status of the 3 threads.


//for recv timeout in ACK from server
struct timeval tv;

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


void *sendPing(void *fd)
{
	int rv;
	int sockfd=(int)fd;
	// cout<<sockfd<<"ee";
	char buf[5];
	while(1)
	{
		pingAlive=true;
		if(send(sockfd, "PING", 5, 0) <0)
		{
			fprintf(stderr,"Error in sending PING to server\n");
		}
		memset(buf, '\0', 5);
		if((rv = recv(sockfd, buf, 5, 0))>0)
		{
			if(strcmp(buf, "ACK"))
				cout<<"Ping ACKed\n";	
		}
		else 
		{
			if(sendAlive || rcvAlive)
			{
				cout<<"Server Down! Application will exit after current chat is over...\n";
				close(sockfd);
				pingAlive=false;
				pthread_exit(NULL);				
			}
			else
			{
				cout<<"Server Down! Application will exit"<<endl;
				exit(1);
			}
		}
		usleep(10000000); //ping every 10 sec
	}
}

bool getOnlineClients(int sockfd)
{
	int rv;
	if(send(sockfd, "LIST", 5, 0) <0)
	{
		fprintf(stderr,"Error in sending LIST to server\n");
	}

	char buf[MAXDATASIZE];
	memset(buf, '\0', MAXDATASIZE);
	if((rv = recv(sockfd, buf, MAXDATASIZE-1, 0))>0)
	{
		cout<<"Online clients :-\n";
		cout<<string(buf);
	}
	else
	{
		cout<<"Server Down! Application will exit after current chat is over...\n";
		close(sockfd);
		return false;
	}
	return true;

}


void *chatSend(void *fd)
{
	int socket=(int)fd;
	int msgNo=1;
	while(1)
	{
		if(!rcvAlive)		//if rcv thread is dead
		{
			cout<<"Connection to peer closed."<<endl;
			close(socket);		//consider this. this should not be done.
			
			sendAlive=false;
			pthread_exit(NULL);
		}

		sendAlive=true;
		string msg;
		string buf;

		
		msgNo++;
		
		getline(cin, msg);
		
		if(msg=="/exit")	//client wishes to close connection
		{
			cout<<"Received exit msg from user. Application will exit.\n";
			send(socket, (char *)(msg.c_str()), size_t(msg.size()), 0);
			exit(1);
		}

		buf="#"+to_string(msgNo-1)+":";
		buf+=msg;

		if(send(socket, (char *)(buf.c_str()), size_t(buf.size()), 0) <0)
		{
			fprintf(stderr,"Error in sending msg to peer\n");
			close(socket);		//consider this. this should not be done.
			
			sendAlive=false;
			pthread_exit(NULL);
		}

		//writting to chat file
		mtx.lock();
		if (argument=="file")
		{
			f=fopen("chat.txt", "a");
			fprintf(f, "Prerak%s\n", (char *)(buf.c_str()));
			fclose(f);
		}
		mtx.unlock();

	}
}

void *chatRcv(void *fd)
{
	int socket=(int)fd, rv;
	while(1)
	{
		if(!sendAlive)		//if send thread is dead
		{
			cout<<"Connection to peer closed."<<endl;
			close(socket);		//consider this. this should not be done.
			
			rcvAlive=false;
			pthread_exit(NULL);
		}

		rcvAlive=true;
		char msg[MAXDATASIZE];
		string buf;
		memset(msg, '\0', MAXDATASIZE);
		if((rv=recv(socket, msg, MAXDATASIZE, 0))>0)
		{
			string sMsg= string(msg);
			 // cout<<endl<<sMsg<<endl;
			if(sMsg=="/exit") 
			{
				cout<<"Connection closed by peer. Application will exit"<<endl;
				exit(1);
			}
			if(sMsg.substr(0,3)=="ACK")		//ACK for msg received
			{
				// buf+="-------------------------------------------------\n";
				buf+="\t\t\tMessage:"+sMsg.substr(3,sMsg.size()-3)+" seen.\n";
				// buf+="-------------------------------------------------\n";
			}
			

			else
			{
				string ack="ACK";	//to extract msgNo. from message and build the ACK response
				int i=1;	//skipping the starting '#'
				while(sMsg[i]!=':')
				{
					ack+=sMsg[i];		//extracting msgNo. from the message.
					i++;
				}
				buf="\tNamita"+sMsg+"\n";

				//sending ACK for the above message.

				if(send(socket, (char *)(ack.c_str()), size_t(ack.size()), 0) <0)
				{
					fprintf(stderr,"Error in sending ACK to peer\n");
					cout<<"Connection to peer closed."<<endl;
					close(socket);		//consider this. this should not be done.
					
					rcvAlive=false;
					pthread_exit(NULL);
				}

			}
		}
		else if (rv<0)
		{
			perror("rcvsdfsdf");
			exit(1);
		}
		else //rv==0
		{
			cout<<"Connection closed by peer. Chat will exit.\n";
			exit(1);
			
		}

		//writting to chat file.
		mtx.lock();
		if (argument=="file")
		{
			f=fopen("chat.txt", "a");
			fprintf(f, "%s", (char *)(buf.c_str()));
			fclose(f);
		}
		else cout<<buf<<endl;
		mtx.unlock();
	}

}

#endif
