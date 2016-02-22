#include "TCPHandler.h"

int Handler::add_to_in(char* buf, int size){
	if( in_offset+size > in_length){
		if( 2*in_length > MAX_SIZE_IN )
			return -1;
		
		char* tmp = new char[ 2 * in_length ];
		memcpy(tmp, in_data, in_offset);
		in_length << 1;
	}
	
		
	memcpy(in_data + in_offset, buf, size);
	in_offset += size; 
	return 0;
}
		
int Handler::out_write(){
	return write(fd, out_data + out_offset, out_length);
}


///TCPServer
TCPServer::TCPServer(char* port, int _pfd, std::atomic<bool>* _alive, 
list<Task>*_tasks, mutex* _m_tasks){
	pfd= _pfd;
	struct epoll_event event;
	alive = _alive;
	tasks = _tasks; 
	m_tasks = _m_tasks; 
	
	sfd = create_and_bind (port);
	if(sfd == -1)
		abort();

	s = make_non_blocking (sfd);
	if (s == -1)
		abort();

	s = listen (sfd, SOMAXCONN);
	if(s == -1){
		perror("listen");
		abort();
	}

	efd = epoll_create1(0);
	if(efd == -1){
		perror("epoll_create-TCPServer");
		abort();
	}

	event.data.fd = sfd;
	event.events = EPOLLIN | EPOLLET;
	s = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);
	if(s == -1){
		perror("epoll_ctl-TCPServer2");
		abort();
	}

	/* Buffer where events are returned */	
	events = static_cast<struct epoll_event*>(calloc(MAXEVENTS, sizeof event));
}
	
int TCPServer::create_and_bind (char *port){
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s, sfd;

	memset (&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
	hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
	hints.ai_flags = AI_PASSIVE;     /* All interfaces */

	s = getaddrinfo (NULL, port, &hints, &result);
	if(s != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror (s));
		return -1;
	}

	for(rp = result; rp != NULL; rp = rp->ai_next){
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
		continue;

		s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
		if (s == 0)
			break;

		close (sfd);
	}

	if (rp == NULL){
		fprintf(stderr, "Could not bind\n");
		return -1;
	}

	freeaddrinfo(result);

	return sfd;
}

int TCPServer::make_non_blocking (int sfd){
  int flags, s;

	flags = fcntl(sfd, F_GETFL, 0);
	if(flags == -1){
		perror("fcntl");
		return -1;
	}

	flags |= O_NONBLOCK;
	s = fcntl (sfd, F_SETFL, flags);
	if (s == -1){
		perror ("fcntl");
		return -1;
	}

  return 0;
}

void TCPServer::pcallback(){
	Task t;
	m_tasks->lock();
    for(int i=0; i<tasks->size(); i++){
		t = tasks->front();
		tasks->pop_front();
			
		///Create sending socket
		int sockfd = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in addr;
		if (sockfd < 0){
			perror("ERROR opening socket");
			continue;
		}
		
		addr.sin_family = AF_INET;	
		inet_pton(AF_INET, t.host, &addr.sin_addr);
		addr.sin_port = htons(t.port);
		
		if(connect(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0){
			perror("ERROR connecting");
			continue;
		}
		
		if( make_non_blocking(sockfd) == -1 ){
			perror("ERROR pcallback");
			continue;
		}
		
		Handler* n_handler = new Handler(sockfd);
		
		n_handler->set_out_data( (t.msg)->serialize() );
		n_handler->set_out_length( (t.msg)->s_length() );

		struct epoll_event out_event;
		out_event.events = EPOLLOUT;
		out_event.data.ptr=static_cast<void*>( n_handler );
		s = epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, &out_event);
				
		if(s == -1){
			perror("epoll_ctl-pcallback2");
			close( n_handler->get_fd() );
			free( n_handler );
			abort();
		}
	}
	m_tasks->unlock();
}

void TCPServer::wcallback(Handler* handler, msg_t type){
	close( handler->get_fd() );
	free( handler );
}

void TCPServer::rcallback(Handler* handler, msg_t type){	
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
	
int TCPServer::run(){
	struct epoll_event send_event;
	send_event.data.fd= pfd;
	send_event.events = EPOLLIN;
	s = epoll_ctl (efd, EPOLL_CTL_ADD, pfd, &send_event);
	if(s == -1){
		perror("epoll_ctl-TCPServer3");
		abort();
	}
	
	while( alive->load(std::memory_order_relaxed) ){
		int n, i;

		n = epoll_wait(efd, events, MAXEVENTS, -1);
		for(i = 0; i < n; i++){
			if( (events[i].events & EPOLLERR) ||
				(events[i].events & EPOLLHUP) 
				){
				fprintf (stderr, "epoll error\n");
				
				if( events[i].data.ptr != NULL )
					free(events[i].data.ptr);
				close (events[i].data.fd);
				
				continue;
			}else if( pfd == events[i].data.fd){
				char buf[32];
				printf("sending\n");
				while( read(pfd, buf, sizeof buf) == sizeof(buf) ){}
				pcallback();
				
			}else if(sfd == events[i].data.fd){//One or more connection
				while (1){
					struct sockaddr in_addr;
					socklen_t in_len;
					int infd;
					char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

					in_len = sizeof in_addr;
					infd = accept(sfd, &in_addr, &in_len);
					if(infd == -1){
						if( (errno == EAGAIN) ||
						  (errno == EWOULDBLOCK) ){//Connections processed
							break;
						}else{
							perror("accept");
							break;
						}
					}

					s = getnameinfo(&in_addr, in_len,
									hbuf, sizeof hbuf,
									sbuf, sizeof sbuf,
									NI_NUMERICHOST | NI_NUMERICSERV);

					if(s == 0){
						printf("Accepted connection on descriptor %d "
								"(host=%s, port=%s)\n", infd, hbuf, sbuf);
					}

					s = make_non_blocking (infd);
					if(s == -1)
						abort();

					struct epoll_event listen_event;
					listen_event.events = EPOLLIN | EPOLLET;	
					listen_event.data.ptr=static_cast<void*>( new Handler( infd ) );
					
					s = epoll_ctl (efd, EPOLL_CTL_ADD, infd, &listen_event);
					if(s == -1){
						perror("epoll_ctl-sfd");
						free(events[i].data.ptr);
						abort();
					}
				}
				continue;
			}else if(events[i].events & EPOLLIN){//Data waiting to be read
				int done = 0;
				Handler* handler = static_cast<Handler*>(events[i].data.ptr);

				while (1){
					ssize_t count;
					char buf[BUFF_SIZE];
					count = read(handler->get_fd(), buf, sizeof buf);
					if(count == -1){
						if(errno != EAGAIN){//All data read
							perror("read");
							done = 1;
						}
						break;
					}else if(count == 0){//End-Of-File, connection closed
						done = 1;
						break;
					}

					s = handler->add_to_in(buf, count);
					if(s == -1){
						perror("write");
						abort();
					}
				}

				if(done)
					rcallback( handler, Msg::get_type( handler->get_in_data() ) );
			}else if(events[i].events & EPOLLOUT){
				int ret;				
				Handler* handler = static_cast<Handler*>(events[i].data.ptr);
				printf("begin writting\n");
				ret = handler->out_write();
	   
				if( (-1 == ret && EINTR == errno) || 
					ret < handler->get_out_length()){ //Sent partial data
	   
					struct epoll_event listen_event;
					listen_event.events = EPOLLOUT | EPOLLET;	
					listen_event.data.ptr=static_cast<void*>( handler );
					
					s = epoll_ctl (efd, EPOLL_CTL_ADD, handler->get_fd(), &listen_event);
					if(s == -1){
						perror("epoll_ctl-out");
						close( handler->get_fd() );
						free( handler );
						abort();
					}
				   					   
					if(-1 != ret){//previous write : only partial data
						handler->decr_out_length( ret );
						handler->incr_out_offset( ret );   
					}
				   
				}else if(-1 == ret){ //error
					perror("write");
					close( handler->get_fd() );
					free( handler );
					abort();
				}else{ //entire data was written          
					printf("\nAdding Read Event.\n");   
					wcallback( handler, Msg::get_type( handler->get_out_data() ) );
				}
	
			}else{
				printf("it is an other event\n");
			}
		}
	}
}


//int main(){
	//TCPHandler t("1599");
	//struct timespec req, rem;
	//req.tv_sec  = 0;
	//req.tv_nsec = 11999999;
	//while(1){  
		//Msg m(I, "coucou", strlen("coucou"));
		//t.send(m, "127.0.1.1", 1597);
		//nanosleep(&req, &rem);
	//}
//}
