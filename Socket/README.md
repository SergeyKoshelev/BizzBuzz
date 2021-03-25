# Sockets:

## Main part:
Server and client programms that have some **options**:  
1) UDP/TCP switch (TCP under construction)  
2) Remote shell with command "shell"  
3) Server starts as daemon and gives its pid  
4) Broadcast with command "findall  
5) Client isolation based on subprocesses  
6) Error handling  
7) Logging in file "log.txt"  

**Future options:**  
1)Udp control  
2)Buildsystem  
3)Shared library  
4)Cryptography  
5)Log routines  
6)Testing  
7)Signal handling  

**Libraries**: lib.h, TCP.h, UDP.h  
**Programms**:  
	server.c & client.c  
**Compile**:  
1)gcc server.c -o server  
2)gcc client.c -o client  
3)./server  
4)./client [ip]  
**Commands in client**:  
	1)print [str]  
	2)findall  
	3)ls  
	4)cd [new_dir]  
	5)exit	//from shell or from client//  
	6)shell  
	7)exit  

## Trash:
**UNIX**: server_UNIX.c & client_UNIX.c  
**Customiztion**: server_TCP.c & client_TCP.c  
