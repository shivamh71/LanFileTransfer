// Header and Include files 
/********************************************************************************/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <iostream>
#include <cstdlib>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
using namespace std;
/********************************************************************************/

// Macros
#define LENGTH 512

// Finding modulo of hexadecimal md5 sum
/********************************************************************************/
int modulo(char* hex, int total){
	int remainder = 0;
	for(int i=0;i<32;i++){
		if(hex[i]<=57)
			remainder = (16*remainder + (hex[i]-48))%total;
		else
			remainder = (16*remainder + (hex[i]-87))%total;
	}
	return remainder;
}
/********************************************************************************/

// Main Function
/********************************************************************************/
int main(int argc, char** argv){

	/* 
		Input validation
		ID of server to be put online is given as command line input
	*/
	if(argc!=2){
		cout<<"Wrong Use : Usage: "<<argv[0]<<" node_number"<<endl;
		return -1;
	}

	vector<vector<string> > nodeConfig; // Holds network configuration info

	// Reading configuration file
	FILE* fileMesh = fopen("FileMesh.cfg","r"); // File pointer to read network config file
	while (!feof(fileMesh)) {
		char temp[30], folder[100];
		char port[6], ip[16];
		fscanf(fileMesh,"%s %s",temp,folder); 
		/*
			Each line of config file has a server IP, port and corresponding folder info
			Parsing config file to extract all server info
		*/
		int j;
		for (int i=0;i<30;i++) {
			if (temp[i]==':') {
				ip[i] = 0;
				j = i;
				break;
			}
			else {
				ip[i] = temp[i];
			}
		}
		for (int i=j+1;i<30;i++) {
			if ((temp[i]==0)) {
				port[i - j - 1] = 0;
				break;
			}
			else {
				port[i-j-1] = temp[i];
			}
		}
		// Entire network configuration is stored in nodeConfig
		vector<string> tempConfig;
		tempConfig.push_back(ip);
		tempConfig.push_back(port);
		tempConfig.push_back(folder);
		nodeConfig.push_back(tempConfig);
	}
	fclose(fileMesh);
	/*
		i : Node ID
		nodeCOnfig[i][0] : IP of server i
		nodeCOnfig[i][0] : Port at which server i operates
		nodeCOnfig[i][0] : Directory info of server i
	*/

	/*	
		UDP connection Setup
		udpControl : Socket to recieve UDP request from clients
		node number = id of server contacted
		total_node = total number of nodes
		servaddr : holds server address info (Port and IP)
		cliaddr : holds connecting address info (Port and IP)
		rqstMsg : buffer to store UDP request message	
	*/
	int udpControl, node_number, total_node;
	node_number = atoi(argv[1]);
	total_node = nodeConfig.size();
	struct sockaddr_in servaddr, cliaddr;
	char rqstMsg[1000];

	// Bringing up the UDP socket to get client requests
	udpControl = socket(AF_INET,SOCK_DGRAM,0); // specify domain and type of socket (Internet with UDP connection)
	if(udpControl==-1){ // check if socket is not created
		cout<<"Error Occured... Node cannot be established\n";
		return -1;
	}
	bzero(&servaddr, sizeof(servaddr)); // setting all bytes of servaddr to zero
	servaddr.sin_family = AF_INET; // using internet family addressing
	inet_aton(nodeConfig[node_number][0].c_str(), &(servaddr.sin_addr)); // giving IP to servaddr
	servaddr.sin_port =htons(atoi(nodeConfig[node_number][1].c_str())); // giving Port to servaddr
	if(bind(udpControl, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1) { // binding Ip, Port to socket
		cout<<"Error Occured... Node cannot be established\n";
		return -1;
	}
	cout<<nodeConfig[node_number][0]<<":"<<atoi(nodeConfig[node_number][1].c_str())<<" node online"<<endl;

	// Waiting for client
	while(1){
		
		/*
			Listening to client
			cliaddr : holds client address info
			n : number of bytes received
			rqstMsg : buffer to keep received message
			1000 : maximum length of buffer
			0 : flags (set to zero)
			len : length of address stored in cliaddr
		*/
		socklen_t len = 0;
		int n = recvfrom(udpControl, rqstMsg, 1000, 0, (struct sockaddr *) &cliaddr, &len); // 
		rqstMsg[n] = 0;
		
		// Parsing the request message received from client
		int length = rqstMsg[0]; // holds total length of request message
		if (length!=n) { // check length of received message matches total length field
			cout<<"Wrong UDP request"<<endl;
			continue;
		}
		char type = rqstMsg[1]; // store or get file request
		char filehash[33];
		strncpy(filehash, &rqstMsg[2], 32); // md5 sum of file questioned
		filehash[32] = 0;

		if (type!='s' && type!='r') continue; // Input Validation
		int mod = modulo(filehash, total_node); // Finding correct server node ID using md5 hash

		// Check if server ID matches filehash
		if(node_number==mod){
			cout<<"Correct Server Node Reached"<<endl;
			/*
				Parsing client request message
				ip : IP of client
				port : Port info of client
			*/
			char ip[16];
			int j;
			for(int i=0;i<16;i++){
				if(rqstMsg[i+34]==':'){
					ip[i] = 0;
					j = i+34;
					break;
				}
				ip[i] = rqstMsg[i+34];
			}
			char port[6];
			for(int i=0;i<6;i++){
				if(i+j+1 == n){
					port[i]=0;
					break;
				}
				port[i] = rqstMsg[i+j+1];
			}
			cout<<"Client Details "<<ip<<":"<<port<<endl;

			/*	
				TCP connection Setup
				tcpSock : Socket to set up TCP connection with client				
				remote_addr : holds address info of remote client (Port and IP)
				revbuf : buffer to store received message
				sendbuf : buffer to store message to be sent	
			*/
			int tcpSock;
			char revbuf[LENGTH], sdbuf[LENGTH];
			struct sockaddr_in remote_addr;
			tcpSock = socket(AF_INET, SOCK_STREAM, 0); // specify domain and type of socket (Internet with TCP connection)
			if(tcpSock==-1){ // check if socket is not created 
				cout<<"Error Occured...TCP Socket not working\n";
				continue;
			}
			bzero(&remote_addr, sizeof(remote_addr)); // setting all bytes of remote_addr to zero
			remote_addr.sin_family = AF_INET; // using internet family addressing
			inet_aton(ip, &(remote_addr.sin_addr)); // giving IP to remote_addr
			remote_addr.sin_port =htons(atoi(port)); // giving Port to remote_addr

			/* 
				Connecting with client
				tcpSock : socket used to connect with client (remote host)
			*/
			int check = connect(tcpSock, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));
			if(check==-1){ // check if TCP connection with client couldn't be established
				cout<<"Problem connecting with client"<<endl;
				continue;
			}

			string fr_name = nodeConfig[node_number][2] + filehash; // Filename to be used in case of store/get request

			/* 
				Processing store file request
				In case of failed file transaction TCP socket is closed using : close(tcpSock)
			*/
			if (type == 's') {
				// Opening new file on server to store data received from client
				FILE *fr = fopen(fr_name.c_str(), "wb");
				if(fr == NULL) { // check if file was correctly opened
					cout<<"Error writing file"<<endl;
					close(tcpSock);
					continue;
				}
				else {
					bzero(revbuf, LENGTH); // setting all bytes of received message buffer to zero
					int fr_block_sz = 0; // holds length of received message from client
				    while((fr_block_sz = recv(tcpSock, revbuf, LENGTH, 0)) > 0) { // receiving data from client
						int write_sz = fwrite(revbuf, sizeof(char), fr_block_sz, fr);
				        if(write_sz < fr_block_sz){ // check if file writing failed
				            cout<<"File write failed."<<endl;
				            break;
				        }
						bzero(revbuf, LENGTH); // set all bytes of received message buffer to zero
					}
					if(fr_block_sz < 0) { // check if there was error is receiving data from client
						cout<<"Error in file receiving"<<endl;
						continue;
					}
				    printf("Ok received from client!\n");
				    fclose(fr);
				}
				close(tcpSock); // closing TCP connection after successfull transaction
			}
			// Processing GET file request
			else {
				FILE *fs = fopen(fr_name.c_str(), "rb"); // opening file requested by server
			    if(fs == NULL) { // check if file is not found on server
			        cout<<fr_name<<" not found on server"<<endl;
			        close(tcpSock);
					continue;
			    }
			    // set all bytes of sending message buffer to zero
			    bzero(sdbuf, LENGTH); 
			    int fs_block_sz; 
			    while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs))>0) { // read data from file
			        if(send(tcpSock, sdbuf, fs_block_sz, 0) < 0) { // sending data to client and also checking if sending failed
			            cout<<"Failed to send file "<<fr_name<<endl;
			            break;
			        }
			        bzero(sdbuf, LENGTH); // set all bytes of sending message buffer to zero
			    }
				close(tcpSock); // close TCP socket after successfull transaction
			}	
		}
		// In case server ID doesn't match with md5 hash forward client request to appropriate server
		else {
			cout<<"Incorrect server node Reached"<<endl;
			// Forwarding client request
			bzero(&cliaddr, sizeof(cliaddr)); // set all bytes of cliaddr to zero
			cliaddr.sin_family = AF_INET; // use internet family addressing
			inet_aton(nodeConfig[mod][0].c_str(), &(cliaddr.sin_addr)); // gining IP to cliaddr
			cliaddr.sin_port =htons(atoi(nodeConfig[mod][1].c_str())); // giving Port to cliaddr
			cout<<"Forwarding client request to "<<nodeConfig[mod][0]<<":"<<nodeConfig[mod][1]<<endl;
			/*
				Forwarding client request
				udpControl : UDP socket used to forward request
				rqstMsg : request message received from client (message to be forwarded)
				cliaddr : address of correct server node
			*/
			if(sendto(udpControl,rqstMsg,strlen(rqstMsg),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr))!=strlen(rqstMsg)) {
				cout<<"Error forwarding the request"<<endl; // check if request forwarding failed
				continue;
			}
		}
	}
	return 0;
}
/********************************************************************************/