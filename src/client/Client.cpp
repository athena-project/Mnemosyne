#include "Client.h"

void TCPClientServer::wcallback(Handler* handler, msg_t type){
	close( handler->get_fd() );
	free( handler );
}

void TCPClientServer::rcallback(Handler* handler, msg_t type){	
	if( type ==  OBJECT){
		Msg m( handler->get_in_data());
		char digest[SHA224_DIGEST_LENGTH];
		char exists;
		memcpy(digest, m.get_data(), SHA224_DIGEST_LENGTH);
		memcpy(&exists, m.get_data()+SHA224_DIGEST_LENGTH, sizeof(char));
		
		m_objects->lock();
		objects->push_back( make_pair(digest, exists) );
		m_objects->unlock();
	}else if( type == OBJECTS ){
		Msg m( handler->get_in_data());
		size_t num = 0;
		char* end = m.get_data() + sizeof(size_t);
		num = strtoull(m.get_data(), &end, 0);
		
		int i = 0;
		char* ptr = m.get_data() + sizeof(size_t);
		for(; i<num; i++, ptr+=sizeof(char)+SHA224_DIGEST_LENGTH){
			char digest[SHA224_DIGEST_LENGTH];
			char exists;
			memcpy(digest, ptr, SHA224_DIGEST_LENGTH);
			memcpy(&exists, ptr+SHA224_DIGEST_LENGTH, 1);
		
			m_objects->lock();
			objects->push_back( make_pair(digest, exists) );
			m_objects->unlock();
		}
	}
	
	struct epoll_event listen_event;
	listen_event.events = EPOLLOUT | EPOLLET;	
	listen_event.data.ptr=static_cast<void*>( handler );
	printf("Msg incomming : |");
	write (1, handler->get_in_data(), handler->get_in_offset()); //Debug

	s = epoll_ctl (efd, EPOLL_CTL_MOD, handler->get_fd(), &listen_event);
	if(s == -1){
		perror("epoll_ctl-rcallback");
		close( handler->get_fd() );
		free( handler );
		abort();
	}
	
}



int main(){}
