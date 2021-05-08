## Main part:
Server and client programms that have some **options**:  
1) UDP/TCP switch  
2) Remote shell with command "shell"  
3) Server starts as daemon and gives its pid  
4) Broadcast with command "findall"  
5) Client isolation based on subprocesses  
6) Error handling  
7) Logging in file "server.log" via log routines  
8) Buildsystem: "make"/"make clean"  
9) Shared Library/Functions via structure  
10) Simple cryptography with xor (^)  

**Probable future options:**  
1)Udp control  
3)Signal handling  

**Libraries**: lib.h, lib.c, log.h, log.c  
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
	5)shell (with all its commands)  
	6)quit  

