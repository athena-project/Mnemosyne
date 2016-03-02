#include "MapServer.h"


void TCPMapServer::wcallback(Handler* handler, msg_t type){
    close( handler->get_fd() );
    delete handler;
}

void TCPMapServer::rcallback(Handler* handler, msg_t type){     
    char* data = handler->get_in_data();
    
    
    printf("\n\n");
    for(int i=0; i<72; i++)
        printf("%c", data[i]);
    printf("\n");
    if( type == EXISTS_OBJECT){
        char *buff = new char[DIGEST_LENGTH + 1 + HEADER_LENGTH];
        char* buffer = buff + HEADER_LENGTH;
        memcpy(buffer, data, DIGEST_LENGTH);
        
        m_objects->lock();
        std::string stre = digest_to_string(data);
        buffer[DIGEST_LENGTH] = ( objects->find( digest_to_string(data) ) != objects->end() );
        m_objects->unlock();
        
        handler->clear();
        
        handler->send(OBJECT, buff, DIGEST_LENGTH+1); //transfert ownership
        register_event(handler, EPOLLOUT, EPOLL_CTL_MOD);
    }else if( type == EXISTS_CHUNKS ){
        char* end = data + sizeof(uint64_t);
        uint64_t num = strtoull(data, &end, 0);
        
        char *buf = new char[ DIGEST_LENGTH * num + num + HEADER_LENGTH + uint64_s ];
        char *buffer = buf + HEADER_LENGTH;
        sprintf(buffer, "%" PRIu64 "", num);
        buffer+=uint64_s;
        
        uint64_t pos = 0; 
        data += sizeof(uint64_t);
        
        m_chunks->lock();
        for(int i = 0 ; i<num; i++, data+=DIGEST_LENGTH, 
        pos += DIGEST_LENGTH + 1){
            memcpy(buffer + pos, data, DIGEST_LENGTH);
                        
            buffer[pos + DIGEST_LENGTH] = ( 
                chunks->find( digest_to_string(data) ) != objects->end() );
        }
        m_chunks->unlock();
        printf("SEND CHUNKS!!!!!!!! %d_%d\n", num, DIGEST_LENGTH * num + num + HEADER_LENGTH + uint64_s);
        handler->send(CHUNKS, buf, DIGEST_LENGTH * num + num + HEADER_LENGTH);//transfert ownership
        register_event(handler, EPOLLOUT, EPOLL_CTL_MOD);
    }else if( type == ADD_OBJECT ){
        char *buff = new char[DIGEST_LENGTH + HEADER_LENGTH];
        char *buffer = buff + HEADER_LENGTH;
        memcpy(buffer, data, DIGEST_LENGTH);
        
        m_objects->lock();
        (*objects)[digest_to_string(buffer)]=true;
        m_objects->unlock();
        
        handler->clear();
        
        handler->send(OBJECT_ADDED, buffer, DIGEST_LENGTH + HEADER_LENGTH); //transfert ownership
        register_event(handler, EPOLLOUT, EPOLL_CTL_MOD);
    }
    else if( type == ADD_CHUNKS ){
        char* end = data + sizeof(uint64_t);
        uint64_t num = strtoull(data, &end, 0);
        
        char *buff = new char[ DIGEST_LENGTH * num + HEADER_LENGTH + uint64_s];
        memcpy(buff + HEADER_LENGTH, handler->get_in_data(), DIGEST_LENGTH * num + uint64_s);

        data += sizeof(uint64_t);
        m_chunks->lock();
        for(int i = 0 ; i<num; i++, data+=DIGEST_LENGTH)                    
            (*chunks)[ digest_to_string(data) ] = true;
        m_chunks->unlock();
        
        handler->send(CHUNKS_ADDED, buff, DIGEST_LENGTH * num + HEADER_LENGTH + uint64_s);//transfert ownership
        register_event(handler, EPOLLOUT, EPOLL_CTL_MOD);
    }else{
        close( handler->get_fd() );
        delete handler;
    }
}

MapServer::MapServer(const char* port, NodeMap* _nodes) : nodes(_nodes){

    pipe(pfds);
    alive.store(true,std::memory_order_relaxed); 
    
    server = new TCPMapServer(port, pfds[0], &alive, &tasks, &m_tasks,
    &objects, &m_objects, &chunks, &m_chunks);
    
    t_server = std::thread( run_server,server );
    t_server.detach();
} 

MapServer::~MapServer(){printf("sdufg\n");}


void MapServer::run(){
    while( true ){
        sleep( 5 );
    }
}
