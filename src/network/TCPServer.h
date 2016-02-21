#ifndef MNEMOSYNE_NETWORK_TCPSERVER_H
#define MNEMOSYNE_NETWORK_TCPSERVER_H

//https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/
#define MAXEVENTS 64

#define BUFF_SIZE 2048
#define MAX_SIZE_IN 65536

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>

using namespace std;

enum msg_t{I};

class Msg{
	protected:
		msg_t type;
		char* data;
	public:
		Msg(msg_t t) : type(t){}
};

class Handler{
	protected:
		int	fd;
		
		char* in_data = NULL;
		uint32_t in_offset = 0 ;
		uint32_t in_length = 1024 ;
		
		char* out_data = NULL;
		uint32_t out_offset = 0;
		uint32_t out_length = 0;
		
	public:
		Handler(int _fd) : fd(_fd){
			in_data = new char[in_length];
		} 
		
		~Handler(){ 
			if( in_data != NULL)
				delete[] in_data; 
		}
		
		int get_fd(){ return fd; }
		char* get_in_data(){ return in_data; }
		uint32_t get_in_length(){ return in_length; }
		uint32_t get_in_offset(){ return in_offset; }
		
		uint32_t get_out_length(){ return out_length; }
		uint32_t get_out_offset(){ return out_offset; }

		void decr_out_length( uint32_t ret ){ out_length-= ret; }
		void incr_out_offset( uint32_t ret ){ out_offset+= ret; }  
		
		int add_to_in(char* buf, int size);
		
		int out_write();
};



class TCPServer{
	protected :
		int efd;
		int sfd;
		int s;
		
		struct epoll_event* events = NULL;
	
	public :
		TCPServer(char* port);
		
		~TCPServer(){
			if( events == NULL )
				delete[] events;
		}
	
		int create_and_bind (char *port);

		int make_socket_non_blocking (int sfd);
		
		void callback(Handler* handler);
	
		int run();
};
#endif
