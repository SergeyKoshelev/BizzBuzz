# Sockets:

## UNIX:  
**Library**: my_server.h  
**Programms**: server_UNIX.c & client_UNIX.c  

## TCP:  
**Library**: my_server.h  
**Programms**: server_TCP.c & client_TCP.c  


## UDP:  
Using UDP protocol, broadcast for command "findall", output in client, log of commands in server, shell  
**Library**: UDP.h  
**Programms**: 
	server_UDP.c & client_UDP.c  
**Compile**:
		gcc server_UDP.c -o server
		gcc client_UDP.c -o client
		./server
		./client [ip]
**Commands in client**:  
	1)print [str]  
	2)findall  
	3)ls  
	4)cd [new_dir]  
	5)exit	//from shell or from client//  
	6)shell  
	7)exit  
