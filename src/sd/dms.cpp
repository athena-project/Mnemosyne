#include "dms.h"


void TCPMapServer::wcallback(Handler* handler, msg_t type){
	close( handler->get_fd() );
	free( handler );
}

void TCPMapServer::rcallback(Handler* handler, msg_t type){	
	char* data = handler->get_in_data() + Msg::HEADER_LENGTH;
	
	if( type == EXISTS_OBJECT){
		char digest[SHA224_DIGEST_LENGTH+1];
		digest[SHA224_DIGEST_LENGTH] = 0;
		char exists;
		
		memcpy(digest, data, SHA224_DIGEST_LENGTH);
		m_objects->lock();
		exists = ( objects->find( string(digest) ) != objects->end() );
		m_objects->unlock();
		
		handler->clear();
		
		Msg m(OBJECT, SHA224_DIGEST_LENGTH + 1);
		memcpy(m.get_data(), digest, SHA224_DIGEST_LENGTH);
		m.get_data()[ SHA224_DIGEST_LENGTH ] = exists;

		
		handler->set_out_data( m.serialize() );
		handler->set_out_length( m.s_length() );
		register_event(handler, EPOLLOUT, EPOLL_CTL_MOD);
	}else if( type == EXISTS_CHUNKS ){
		char* end = data + sizeof(uint64_t);
		uint64_t num = strtoull(data, &end, 0);
		
		Msg m(CHUNKS, SHA224_DIGEST_LENGTH * num + num);
		char digest[SHA224_DIGEST_LENGTH+1];
		digest[SHA224_DIGEST_LENGTH] = 0;
		char exists;
		
		data += sizeof(uint64_t);
		for(int i = 0 ; i<num; i++, data+=SHA224_DIGEST_LENGTH){
			memcpy(digest, data, SHA224_DIGEST_LENGTH);
			
			m_objects->lock();
			exists = ( objects->find( string(digest) ) != objects->end() );
			m_objects->unlock();
			
			memcpy(m.get_data() + i * (SHA224_DIGEST_LENGTH + 1), digest, 
				SHA224_DIGEST_LENGTH);
			m.get_data()[ i * (SHA224_DIGEST_LENGTH + 1) + SHA224_DIGEST_LENGTH] = exists;
		}
		
		handler->set_out_data( m.serialize() );
		handler->set_out_length( m.s_length() );
		register_event(handler, EPOLLOUT, EPOLL_CTL_MOD);
	}else if( type == ADD_OBJECT ){
		char digest[SHA224_DIGEST_LENGTH];
		
		memcpy(digest, data, SHA224_DIGEST_LENGTH);
		m_objects->lock();
		(*objects)[string(digest)]=true;
		m_objects->unlock();
		
		handler->clear();
		Msg m(OBJECT_ADDED, digest, SHA224_DIGEST_LENGTH);
		
		handler->set_out_data( m.serialize() );
		handler->set_out_length( m.s_length() );
		register_event(handler, EPOLLOUT, EPOLL_CTL_MOD);
	}
	else if( type == ADD_CHUNKS ){
		char* end = data + sizeof(uint64_t);
		uint64_t num = strtoull(data, &end, 0);
		
		Msg m(CHUNKS_ADDED, SHA224_DIGEST_LENGTH * num + num);
		char digest[SHA224_DIGEST_LENGTH];
		
		data += sizeof(uint64_t);
		for(int i = 0 ; i<num; i++, data+=SHA224_DIGEST_LENGTH){
			memcpy(digest, data, SHA224_DIGEST_LENGTH);
			
			m_objects->lock();
			(*objects)[ string(digest) ] = true,
			m_objects->unlock();
			
			memcpy(m.get_data() + i * SHA224_DIGEST_LENGTH, digest, 
				SHA224_DIGEST_LENGTH);
		}
		
		handler->set_out_data( m.serialize() );
		handler->set_out_length( m.s_length() );
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
