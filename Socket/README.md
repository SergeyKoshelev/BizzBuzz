## Main part:
Server and client programms that have some **options**:  
1) UDP/TCP switch  
2) Remote shell with command "shell"  
3) Server starts as daemon and gives its pid  
4) Broadcast with command "findall  
5) Client isolation based on subprocesses  
6) Error handling  
7) Logging in file "log.txt"  
Also add. features:  
8)Buildsystem  
9)Shared library  

**Future options:**  
1)Udp control  
2)Cryptography  
3)Log routines  
4)Signal handling  

**Libraries**: lib.h, lib.c  
**Programms**:  
	server.c & client.c  
**Shared library**:  
	TCP.c & UDP.c  
**Compile**:  
1)make  
2)./server [protocol]
3)./client [protocol] [ip]  
Protocol can be "udp" or "tcp"  
**Commands in client**:  
	1)print [str]  
	2)findall  
	3)ls  
	4)cd [new_dir]  
	5)exit	//from shell or from client//  
	6)shell  
	7)exit  

