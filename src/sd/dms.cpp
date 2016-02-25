#include "dms.h"


void TCPMapServer::wcallback(Handler* handler, msg_t type){
	close( handler->get_fd() );
	free( handler );
}

void TCPMapServer::rcallback(Handler* handler, msg_t type){	
	char* data = handler->get_in_data();
	
	if( type == EXISTS_OBJECT){
		char *buff = new char[SHA224_DIGEST_LENGTH + 1 + HEADER_LENGTH];
		char* buffer = buff + HEADER_LENGTH;
		memcpy(buffer, data, SHA224_DIGEST_LENGTH);
		
		m_objects->lock();
		buffer[SHA224_DIGEST_LENGTH] = ( objects->find( digest_to_string(data) ) != objects->end() );
		m_objects->unlock();
		
		handler->clear();
		
		handler->send(OBJECT, buff, SHA224_DIGEST_LENGTH+1); //transfert ownership
		register_event(handler, EPOLLOUT, EPOLL_CTL_MOD);
	}else if( type == EXISTS_CHUNKS ){
		char* end = data + sizeof(uint64_t);
		uint64_t num = strtoull(data, &end, 0);
		
		char *buf = new char[ SHA224_DIGEST_LENGTH * num + num + HEADER_LENGTH];
		char *buffer = buf + HEADER_LENGTH;
		
		uint64_t pos = 0; 
		data += sizeof(uint64_t);
		
		m_chunks->lock();
		for(int i = 0 ; i<num; i++, data+=SHA224_DIGEST_LENGTH, 
		pos += SHA224_DIGEST_LENGTH + 1){
			memcpy(buffer + pos, data, SHA224_DIGEST_LENGTH);
						
			buffer[pos + SHA224_DIGEST_LENGTH] = ( 
				chunks->find( digest_to_string(data) ) != objects->end() );
		}
		m_chunks->unlock();
		
		handler->send(CHUNKS, buf, SHA224_DIGEST_LENGTH * num + num);//transfert ownership
		register_event(handler, EPOLLOUT, EPOLL_CTL_MOD);
	}else if( type == ADD_OBJECT ){
		char *buff = new char[SHA224_DIGEST_LENGTH + HEADER_LENGTH];
		char *buffer = buff + HEADER_LENGTH;
		memcpy(buffer, data, SHA224_DIGEST_LENGTH);
		
		m_objects->lock();
		(*objects)[digest_to_string(buffer)]=true;
		m_objects->unlock();
		
		handler->clear();
		
		handler->send(OBJECT_ADDED, buffer, SHA224_DIGEST_LENGTH); //transfert ownership
		register_event(handler, EPOLLOUT, EPOLL_CTL_MOD);
	}
	else if( type == ADD_CHUNKS ){
		char* end = data + sizeof(uint64_t);
		uint64_t num = strtoull(data, &end, 0);
		
		char *buff = new char[ SHA224_DIGEST_LENGTH * num + HEADER_LENGTH];
		memcpy(buff + HEADER_LENGTH, handler->get_in_data(), SHA224_DIGEST_LENGTH * num);

		data += sizeof(uint64_t);
		m_chunks->lock();
		for(int i = 0 ; i<num; i++, data+=SHA224_DIGEST_LENGTH)					
			(*chunks)[ digest_to_string(data) ] = true;
		m_chunks->unlock();
		
		handler->send(CHUNKS_ADDED, buff, SHA224_DIGEST_LENGTH * num);//transfert ownership
		register_event(handler, EPOLLOUT, EPOLL_CTL_MOD);
	}else{
		close( handler->get_fd() );
		free( handler );
	}
}

MapServer::MapServer(char* port, char* _nodes){
	nodes = new NodeMap(3);

	pipe(pfds);
	alive.store(true,std::memory_order_relaxed); 
	
	server = new TCPMapServer(port, pfds[0], &alive, &tasks, &m_tasks,
	&objects, &m_objects, &chunks, &m_chunks);
	
	t_server = std::thread( run_server,server );
	t_server.detach();
} 

MapServer::~MapServer(){
	delete nodes;
}
