#ifndef MNEMOSYNE_NETWORK_TCPCLIENT_H
#define MNEMOSYNE_NETWORK_TCPCLIENT_H

#define MAXEVENTS 64
#define BUFF_SIZE 1024
#define MAX_SIZE_IN 65536

#define uint64_s sizeof(uint64_t)
#define int_s sizeof(int)
#define size_s sizeof(size_t)

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

#include <thread>
#include <atomic> 
#include <mutex> 
#include <limits> 
#include <list> 
#include <time.h>
#include <string>


using namespace std;

enum msg_t{
    EXISTS_OBJECT, //client->cms value digest
    EXISTS_CHUNKS, //client->cmsvalue nm_objs digst1 digsetn
    OBJECT, //cms->client value digest exists(true-false : char)
    CHUNKS, //cms->client value nm_objs digest1 exists1(true-false : char) digsetn existsn
    ADD_CHUNKS, //client->cmsvalue nm_objs digst1 digsetn
    ADD_OBJECT,
    OBJECT_ADDED,
    CHUNKS_ADDED
    };

static const size_t HEADER_LENGTH = 2 * uint64_s;

class Task{
    public:     
        msg_t type;
        uint64_t length;
        char* data = NULL; //owned until transmit to handler
        bool data_owned = true;
        
        const char* host;
        unsigned int port;
        
        Task(){}
        
        Task(msg_t _type, char* _data, uint64_t _length, const char* _host, 
        unsigned int _port) : type(_type), data(_data), length(_length), 
        host(_host), port(_port){}
            
        ~Task(){
            if( data_owned)
                free( data );
        }
        
        char* steal_data(){ data_owned = false; return data; }
};

class Handler{
    protected:
        int fd;
        char * aplha;
        char* in_data = NULL; //owned
        uint32_t in_offset = 0 ;
        uint32_t in_length = BUFF_SIZE ;
        
        msg_t out_type;
        char* out_data = NULL; //owned
        uint32_t out_offset = 0;
        uint32_t out_length = 0;
        
    public:
        Handler(int _fd) : fd(_fd){
            in_data = new char[in_length];
        } 
        
        ~Handler(){ 
            free( in_data);
            free( out_data);
        }
        
        int get_fd(){ return fd; }
        
        msg_t get_int_type(){
            char* end = in_data + 2*uint64_s;
            return static_cast<msg_t>(strtoull(in_data+uint64_s, &end, 0));
        }
        char* get_in_data(){ return in_data+HEADER_LENGTH; }
        uint32_t get_in_length(){ return in_length; }
        uint32_t get_in_offset(){ return in_offset; }
                
        //Transfert ownership;
        void set_out_data(msg_t _type, char* _data, uint64_t _length){ 
            if( out_data != NULL)
                delete[] out_data;
            
            out_type = _type;   
            out_data = _data;
            out_length = _length;
        }
        
        //Transfert ownership;
        void send(msg_t _type, char* _data, uint64_t _length){
            sprintf(_data, "%" PRIu64 "", _length+HEADER_LENGTH);
            sprintf(_data+uint64_s, "%" PRIu64 "", static_cast<uint64_t>(_type));

            set_out_data(_type, _data, _length+HEADER_LENGTH);
        }
        
        msg_t get_out_type(){ return out_type; }
        uint32_t get_out_length(){ return out_length; }
        uint64_t get_expected_in_length(){ 
            if( in_offset < size_s )
                return std::numeric_limits<uint64_t>::max();
                
            char *end=in_data + size_s;
            return strtoull(in_data, &end, 0); 
        }
        uint32_t get_out_offset(){ return out_offset; }

        void decr_out_length( uint32_t ret ){ out_length-= ret; }
        void incr_out_offset( uint32_t ret ){ out_offset+= ret; }  
        
        int add_to_in(char* buf, int size);
        
        int out_write();
        void clear();
};

class TCPServer{
    protected :
        int efd;
        int sfd;
        int s;
        
        int pfd;
        
        struct epoll_event* events = NULL;
    
        atomic<bool>* alive;
        list<Task*>* tasks;
        mutex* m_tasks;
    public :
        TCPServer(){}
    
        TCPServer(const char* port, int _pfd, std::atomic<bool>* _alive, 
        list<Task*>* _tasks, mutex* _m_tasks);
        
        ~TCPServer(){
            if( events == NULL )
                delete[] events;
            close (sfd);
            close( pfd);
        }
    
        bool register_event(Handler *handler, int option, int mod);
    
        int create_and_bind (const char *port);

        int make_non_blocking (int sfd);
        
        //called when new data to send
        void pcallback();
        
        //called when finished to read
        virtual void rcallback(Handler* handler, msg_t type);
        
        //called when finished to write
        virtual void wcallback(Handler* handler, msg_t type);
    
        int run();
};

class TCPHandler{
    protected:
        int pfds[2];
        thread t_server;
        TCPServer* server = NULL;
        atomic<bool> alive;
        
        list<Task*> tasks;
        mutex m_tasks;
        
    public:
        TCPHandler(){}
    
        TCPHandler(const char* port){
            pipe(pfds);
            alive.store(true,std::memory_order_relaxed); 
            server = new TCPServer(port, pfds[0], &alive, &tasks, &m_tasks);
            t_server = std::thread( run_server,server );
            t_server.detach();
        }
        
        ~TCPHandler(){
            close( pfds[1]);
            alive.store(false,std::memory_order_relaxed); 
            
            if( server != NULL)
                delete server;
            else
                close( pfds[0] );
        }
        
        static void run_server(TCPServer* t){ t->run(); }
        
        
        void send(msg_t _type, char* _data, uint64_t _length, const char* _host, unsigned int port){
            char* buffer = new char[_length + HEADER_LENGTH];           
            sprintf(buffer, "%" PRIu64 "", _length+HEADER_LENGTH);
            sprintf(buffer+uint64_s, "%" PRIu64 "", static_cast<uint64_t>(_type));
            memcpy(buffer+HEADER_LENGTH, _data, _length);
            
            m_tasks.lock();
            tasks.push_back( new Task(_type, buffer, _length+HEADER_LENGTH, _host, port) );
            m_tasks.unlock();
            
            write(pfds[1], "0", 1);     
        }
        

};
#endif
