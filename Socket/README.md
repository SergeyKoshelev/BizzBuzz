Sockets:

UNIX:  
library: my_server.h  
Programms: server_UNIX.c & client_UNIX.c  

TCP:  
library: my_server.h  
Programms: server_TCP.c & client_TCP.c  


UDP:  
Using UDP protocol, broadcast for command "findall", output in client, log of commands in server  
library: UDP.h  
Programms: server_UDP.c & client_UDP.c  
Compile:  
1)gcc server_UDP.c -o server  
2)gcc client_UDP.c -o client  
3)./server  
4)./client [ip]  
Commands in client for server:  
1)print [str]  
2)findall  
3)ls  
4)cd [new_dir]  
5)exit  
