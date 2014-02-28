// Header and Include Files
/********************************************************************************/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <iostream>
#include <cstdlib>
#include <string.h>
#include "md5.h"
#include <stdio.h>
#include "fcntl.h"
#include <unistd.h>
using namespace std;
/********************************************************************************/

// Macros
#define BACKLOG 5
#define LENGTH 512

// Main Function
/********************************************************************************/
int main(int argc, char** argv){

	/* 
		Input Validation
		Request type (s or r) and File Description (filename or filehash) to be given as first and second command line arguments
	*/
	if(argc!=3) {
		cout<<"Wrong Use : Usage: "<<argv[0]<<" s/r filename/filehash"<<endl;
		return 0;
	}

	// Parsing user input
	char type = argv[1][0];
	if (type!='r' && type!='s') { // check if request type is invalid
		cout<<"Incorrect Input"<<endl;
		return -1;
	} 

	MD5 md5; // Holds md5 sum of requested or sent file

	// Ip Port Configuration
	string hostIp = "10.13.50.17"; // Give system's IP
	string nodeIp = "10.13.50.175"; // Advertised Public IP of server network (IP of default server on which first request is sent) 
	string hostPort, nodePort = "8010"; // Ports to be used for server and host

	/*	
		UDP connection Setup
		udpControl : Socket to send UDP request to server
		servaddr : holds server address info (Port and IP)
		cliaddr : holds host address info (Port and IP)
		rqstMsg : buffer to store UDP request message	
	*/
	int udpControl;
	struct sockaddr_in servaddr, cliaddr;
	string rqstMsg = "";
	udpControl = socket(AF_INET,SOCK_DGRAM,0); // specify domain and type of socket (Internet with UDP connection) 
	if(udpControl==-1){ // check if UDP socket couldn't be created
		cout<<"Error Occured...\n";
		return 0;
	}
	bzero(&servaddr, sizeof(servaddr)); // set all bytes of server address to zero
	servaddr.sin_family = AF_INET; // use internet family addressing
	inet_aton(nodeIp.c_str(), &(servaddr.sin_addr)); // give IP to servaddr
	servaddr.sin_port = htons(atoi(nodePort.c_str())); // give port to servaddr

	/*	
		TCP connection Setup
		tcpSock : Socket to set up TCP connection with server
		tcpSockSend : Socket to be used for file transaction over TCP (returned by server)				
		addr_local : holds address info of host (Port and IP)
		addr_remote : holds address info of remote server (Port and IP) 
	*/
	int tcpSock, tcpSockSend;
	struct sockaddr_in addr_local,addr_remote;
	tcpSock = socket(AF_INET, SOCK_STREAM, 0); // specify domain and type of socket (Internet with TCP connection) 
	cout<<"Waiting for Server...."<<endl;
	if(tcpSock==-1){ // check if TCP socket couldn't be created
		cout<<"Error Occured...tcp\n";
		return 0;
	}

	socklen_t sin_size = sizeof(struct sockaddr_in);
	bzero(&addr_local, sizeof(addr_local)); // set all bytes of addr_local to zero
	addr_local.sin_family = AF_INET; // using internet family addressing
	inet_aton(hostIp.c_str(), &(addr_local.sin_addr)); // giving IP to socket
	addr_local.sin_port =htons(0); // asking for random free port
	bind(tcpSock, (struct sockaddr*)&addr_local, sizeof(struct sockaddr)); // binding Ip, port to socket
	getsockname(tcpSock,(struct sockaddr *)&addr_local,&sin_size); // get info of created TCP socket
	hostPort = to_string(ntohs(addr_local.sin_port)); // get allotted port

	char* filename = argv[2]; // get filename / filehash from user input

	// Building UDP connection request message
	// Format : s/r + <md5 sum> + <host ip:port> 
	if(strcmp(argv[1],"s")==0){
		char* filehash = md5.digestFile(filename);
		filename = filehash;
	}
	rqstMsg += argv[1][0];
	rqstMsg += filename;
	rqstMsg += (hostIp + ":" + hostPort);
	int requestLength = rqstMsg.length() + 1;
	rqstMsg = (char)requestLength + rqstMsg;
	if (strlen(filename)!=32) { // check if filehas provided or generated is of valid length (32 bytes)
		cout<<"Incorrect filehash provided"<<endl;
		return -1;
	}

	/*
		Requesting to server
		servaddr : holds server address info
		rqstMsg : request message sent
		0 : flags (set to zero)
	*/
	if(sendto(udpControl, rqstMsg.c_str(),rqstMsg.length(), 0, (struct sockaddr *)&servaddr,sizeof(servaddr))==rqstMsg.length()){
		
		// Wait for server's response
		listen(tcpSock,BACKLOG); // listening on tcpSock for response
		// BACKLOG is the number of maximum connections allowed on incoming queue
		// Accepting response from remote server specified by addr_remote (Port and IP)
		tcpSockSend = accept(tcpSock, (struct sockaddr *)&addr_remote, &sin_size); // TCP socket returned by server

		char* fs_name = argv[2]; // get filename / filehash from user input 

		// In case of failed transaction TCP socket is closed using close(tcpSock)
		// Processing store request
		if (type == 's') {
			char sdbuf[LENGTH]; // Send buffer
		    FILE *fs = fopen(fs_name, "rb"); // opening file to be sent
		    if(fs == NULL) { // check if file not found on host
		        cout<<fs_name<<" not found on host"<<endl;
		        close(tcpSock);
				return -1;
		    }
		    cout<<"Sending file "<<fs_name<<" to server"<<endl;

		    bzero(sdbuf, LENGTH); // set all bytes of sending message buffer to zero
		    int fs_block_sz; // holds number of bytes read from file
		    while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs))>0) {
		    	int s = send(tcpSockSend, sdbuf, fs_block_sz, 0); // sending data to server
		        if(s < 0) { // check if sending to server failed
		            cout<<"Failed to send file "<<fs_name<<endl;
		        	close(tcpSock);
		            return -1;
		        }
		        bzero(sdbuf, LENGTH); // set all bytes of sending message buffer to zero
		    }
		    close(tcpSock); // close TCP socket after successfull transaction
			cout<<"File sending successfull. To retrieve use filehash : "<<filename<<endl;
		}
		// Processing get request
	    else {
	    	char revbuf[LENGTH]; // Receive buffer
	    	char fr_name[100]; // Name of file to be used for saving after receiving it from server
	    	cout<<"Enter name for received file : ";
	    	cin>>fr_name;
	    	FILE *fr = fopen(fr_name, "wb"); // opening new file to write data received
			if(fr == NULL) { // check if file couldn't be opened
				cout<<"Error writing file"<<endl;
		        close(tcpSock);
				return -1;
			}
			else {
				bzero(revbuf, LENGTH); // set all bytes of sending message buffer to zero
				int fr_block_sz = 0;
				bool received = false; // To tell user about successfull or failed transaction
			    while((fr_block_sz = recv(tcpSockSend, revbuf, LENGTH, 0)) > 0) { // receiving data from server
					int write_sz = fwrite(revbuf, sizeof(char), fr_block_sz, fr); // writing received data to file
			        if(write_sz < fr_block_sz){ // check if writing to file failed
			            cout<<"File write failed."<<endl;
		        		close(tcpSock);
			            return -1;
			        }
					bzero(revbuf, LENGTH); // set all bytes of sending message buffer to zero
					received = true;
				}
				if(fr_block_sz < 0) {
					cout<<"Error in file receiving"<<endl;
		        	close(tcpSock);
					return -1;
				}
				if(received)
			    	printf("Ok received from server!\n");
			    else
			    	cout<<"Error in receiving file"<<endl;
			    fclose(fr); // 
			}
		    close(tcpSock); // close TCP socket after successfull transaction
	    }
	}
	return 0;
}
/********************************************************************************/
