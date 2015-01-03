<h1> Lan File Transfer </h1>
An application to send and receive files over LAN.

<h4> Instructions </h4>
	1) Change directory to source directory
	2) source has following files :
		node.cpp : implementing server side
		user.cpp : implementing client side
		md5.h : library for finding md5 sum
		FileMesh.cfg : configuration file for server network
	3) Compile user.cpp using
		g++ -o user user.cpp -std=c++0x
	4) Compile node.cpp using
		g++ -o node node.cpp
	5) Run node using 
		./node <node-ID>
	6) Run user using
		./user <request type s or r> <filename / filehash>

<h4> Interface </h4>

	For node.cpp

		Running ./node <node-ID> requires ID of node to be bbrought up as command line argument

	For user.cpp

		Running ./user <request type s or r> <filename / filehash> requires two command line arguments
		First is request type.
			s : to store file on server
			r : to retrieve file stored on server
		Second is the filename or filehash.
			filename : in case of store request
			filehash : in case of get request

	A variable hostIp which specifies the Ip address fo system on which host is run has to be modified once before copiling user.cpp
	Default server on which client requests get routed can be changed by modifying nodeIp, nodePort variables in user.cpp
