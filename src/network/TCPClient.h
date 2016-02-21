#ifndef MNEMOSYNE_NETWORK_TCPCLIENT_H
#define MNEMOSYNE_NETWORK_TCPCLIENT_H


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
#include <inttypes.h>
#include <arpa/inet.h>

using namespace std;

enum msg_t{I};

class Msg{
	protected:
		msg_t type;
		char* data = NULL;
		size_t length = 0;
	public:
		Msg(msg_t t, char* d) : type(t), data(d){}
		
		Msg(char* s_data){
			char* end = s_data + sizeof(uint64_t);
			length = strtoull(s_data, &end, 0);
			
			end += sizeof(uint64_t);
			type = static_cast<msg_t>(strtoull(s_data+sizeof(uint64_t), &end, 0));
			
			data = new char[length];
			memcpy(data, s_data + 2*sizeof(uint64_t), length);
		}
		
		~Msg(){ 
			if( data != NULL)
				delete[] data; 
		}
		
		size_t s_length(){ return length + 2 * sizeof(uint64_t); }
		
		static msg_t get_type(char* data){
			char* end = data + 2*sizeof(uint64_t);
			return static_cast<msg_t>(strtoull(data+sizeof(uint64_t), &end, 0));
		}
		
		char* serialize(){
			char* buff = new char[ length + sizeof(uint64_t) * 2 ];

			sprintf(buff, "%" PRIu64 "", length);
			sprintf(buff+sizeof(uint64_t), "%" PRIu64 "", static_cast<uint64_t>(type));
			memcpy( buff+sizeof(uint64_t) * 2, data, length );
			
			return buff;
		}
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
		
		char* get_out_data(){ return in_data; }
		void set_out_data(char* d){ in_data = d; }
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
		
		int pfd;
		
		struct epoll_event* events = NULL;
	
	public :
		TCPServer(){}
	
		TCPServer(char* port, int _pfd);
		
		~TCPServer(){
			if( events == NULL )
				delete[] events;
				
			free (events);
			close (sfd);
			close( pfd);
		}
	
		int create_and_bind (char *port);

		int make_socket_non_blocking (int sfd);
		
		//called when new data to send
		void pcallback(Handler& handler, msg_t type);
		
		//called when finished to read
		void rcallback(Handler* handler, msg_t type);
		
		//called when finished to write
		void wcallback(Handler* handler, msg_t type);
	
		int run();
		
		void operator()(){
			run();
		}
};

class TCPHandler{
	protected:
		int pfds[2];
		TCPServer client;
		
	public:
		TCPHandler(char* port){
			pipe(pfds);
			client = TCPServer(port, pfds[0]);
		}
		
		~TCPHandler(){
			close( pfds[1]);
		}
		
		void send(Msg& msg, int port, struct in_addr _host){
			char* host = inet_ntoa( _host );
			char buff[ msg.s_length() + 2*sizeof(int) + strlen(host) ];
			
			sprintf(buff, "%d", port);
			sprintf(buff+sizeof(int), "%lu",  strlen(host)); //permet de passer facilement de l'ipv6 à ipv4
			memcpy(buff+sizeof(int)+sizeof(size_t), host, strlen(host) );
			memcpy(buff+sizeof(int)+sizeof(size_t) + strlen(host), msg.serialize(),  msg.s_length() );
			
			write(pfds[1], buff, msg.s_length() + sizeof(int) + sizeof(size_t) +strlen(host));
		}
};
#endif
