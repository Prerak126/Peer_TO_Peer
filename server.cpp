#include "server.h"

int main(int argc, char const *argv[])
{
	signal(SIGPIPE, SIG_IGN);

	int sockfd, new_fd, yes=1, rv, rc, threadCnt=0;  
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;

	char s[INET6_ADDRSTRLEN];
	string client_ip;

	//for recv timeout
	tv.tv_sec = TTL;  /* 30 Secs Timeout */
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		//for recv timeout
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval)))
		{
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}


	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1)
	{
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		client_ip = s;

		if(threadCnt<MAXTHREADS)
		{
			threadMap[client_ip]=threadCnt++;
			if (send(new_fd, "CONN", 5, 0) == -1)	//send CONN msg to new client
				perror("send");
		}
		else 
		{
			fprintf(stderr,"Server ran out of threads\n");
			if (send(new_fd, "DISC", 5, 0) == -1)		//send DISC msg to new client
				perror("send");
			continue;
		}
		

		mtx.lock();
				
		clientMap[client_ip].ip = client_ip;
		clientMap[client_ip].sock_fd = new_fd;

		mtx.unlock();
		
		if((rc = pthread_create(&threads[threadMap[client_ip]], NULL , threadForClient, (void*)s))!=0)
		{
			fprintf(stderr,"Error:unable to create thread, %d\n",rc);
			close(clientMap[client_ip].sock_fd);
			clientMap.erase(client_ip);
		}

	}


	return 0;
}
