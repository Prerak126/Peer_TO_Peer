###To run server###

g++ -std=c++11 server.cpp server.h -lpthread -o server

./server

###To run client### 

g++ -std=c++11 -fpermissive -w client.cpp client.h -lpthread -o client

./client 155.246.163.104 stdin

or ./client 155.246.163.104 file

__NOTE:__ the argument to ./client is the IP address of the server.
_________________________
__IMPORTANT NOTE:__ 

Different clients __CANNOT__ be on the same machine. They MUST be on different machines with different IPs for the chat application to function. However, the server, and one of the clients can be on the same machine, in which case that client will be run by command ./client 155.246.163.104 file or ./client 155.246.163.104 stdin
__________________________

if stdin argument is used, chat will show on terminal although formatting might be bad
if file argument is used, the chat is saved in chat.txt. Messages from peer are not shown on the terminal

__________________________

If you are using the file mode for clients, then use the following tail command in a new terminal which shows the contents of the file in incremental fashion, as and when it is updated.

tail -f chat.txt


___________________________


###SAMPLE OUTPUT IN CHAT###


	Prerak#1:Hie
	
				Message:1 seen.
	
		Namita#1:Hie

		Namita#2:Whats up

	Prerak#2:Doing well
	
				Message:2 seen.
____________________________

Prerak means the client (1st PEER) itself which has built the server and Namita means the Peer with whom Prerak is chatting. #1 denotes the message no. (to verify seen status of that message). While chatting sending a `/exit` message closes the connection and terminates the application.

In file mode, you will only see the messages you enter and __not__ the received ones (on terminal). However, messages are always received and sent synchronously. They are not displayed on terminal, rather dumped into file so that the order of message and acknowledgment (Message: seen thing) delivery can been shown to be correct. On terminal, if user was entering some message, while a reply was being received, then the user's message will flow above in the terminal, leading to confusion and give a "not so comforting" experience. 


###FOR TERMINATING EITHER SERVER OR CLIENT PREMATURELY###
Use CTRL + C, not CTRL + Z, as the latter could leave ports still bound, impeding re-execution of server or client.
