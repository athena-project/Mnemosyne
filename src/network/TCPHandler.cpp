#include "TCPHandler.h"

/**
 * Begin usefull things
 */
 
int make_non_blocking (int sfd){
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

int set_keepalive(int sfd){
    int optval;
    socklen_t optlen = sizeof(optval);
    
    if(getsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
        perror("getsockopt()");
        return false;
    }
    printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));

    /* Set the option active */
    optval = 1;
    optlen = sizeof(optval);
    if(setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        perror("setsockopt()");
        return false;
    }
   printf("SO_KEEPALIVE set on socket\n");

   /* Check the status again */
    if(getsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
        perror("getsockopt()");
        return false;
    }
    printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));
    return true;
}

Handler* create_handler(const char* host, int port){
    printf("host %s, port %d\n", host, port);
    
    ///Create sending socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    if (sockfd < 0){
        perror("ERROR opening socket");
        return NULL;
    }
    
    addr.sin_family = AF_INET;  
    inet_pton(AF_INET, host, &addr.sin_addr);
    addr.sin_port = htons(port);
    
    if(connect(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0){
        perror("ERROR connecting");
        return NULL;
    }
    
    if( make_non_blocking(sockfd) == -1 ){
        perror("ERROR pcallback");
        return NULL;
    }
    
    if( !set_keepalive(sockfd) ){
        return NULL;
    }
    
    return new Handler(sockfd);
}

/**
 * Begin Handler part
 */
int Handler::add_to_in(char* buf, int size){
    if( in_offset+size > in_length){
        if( 2*in_length > MAX_SIZE_IN )
            return -1;
        
        //char* tmp = new char[ 2 * in_length ];
        //printf("try_alloc %d\n", 2 * in_length);
        char* tmp = static_cast<char*>(realloc(in_data, in_length<<1 ));
        if( tmp == NULL )
            return -1;
       
        in_data = tmp;
        in_length = in_length << 1;
    }
    
    memcpy(in_data + in_offset, buf, size);
    in_offset += size; 
    return 0;
}
        
int Handler::out_write(){
    int ret = write(fd, out_data + out_offset, out_length);
    if( ret!= -1){
        clear_out();
    }
}

void Handler::clear_out(){
    delete[] out_data;
    out_data = NULL;
    out_offset = out_length = 0;
}

void Handler::clear(){
    clear_out();
    
    delete[] in_data;        
    in_data = new char[in_length];
    in_offset = 0 ;
    in_length = BUFF_SIZE ;
}

/**
 *  Begin HandlerManager
 */
HandlerManager::~HandlerManager(){
    map< pair<const char*, int>, Handler*>::iterator it = actives.begin();
    for( ; it != actives.end() ; it++)
        delete it->second;

    for( it = passives.begin() ; it != passives.end() ; it++)
        delete it->second;
}

pair<bool, Handler*> HandlerManager::add(pair<const char*, int> key, bool create){
    if( passives.find(key) != passives.end() ){
        set_active(key);
        return pair<bool, Handler*>(true, actives[key] );
    }
    
    if( actives.find(key) != actives.end() )
        return pair<bool, Handler*>(true, NULL); //we have to wait
    
    if( actives.size() >= max_number && passives.size() == 0)
        return pair<bool, Handler*>(true, NULL); //we have to wait
    
    if( actives.size() + passives.size() >= max_number)
        passives.erase( passives.begin() );
    
    Handler* handler = NULL;
    if( create ){
        handler = create_handler(key.first, key.second);
        if( handler == NULL )
            return pair<bool, Handler*>( false, NULL);
    }else
        handler = new Handler(-1);
    
    set_active(handler);
    return pair<bool, Handler*>(true, handler);
}

bool HandlerManager::exists(pair<const char*, int> key){
    return actives.find(key) != actives.end() || passives.find(key) != passives.end(); 
}

void HandlerManager::set_active(pair<const char*, int> key){
    actives[ key ] = passives[ key ];
    passives.erase( key );
}

void HandlerManager::set_active(Handler* item){
    pair<const char*, int> key(item->get_host(), item->get_port());
    set_active( key );
}

void HandlerManager::set_passive(Handler* item){
    item->clear();
    
    pair<const char*, int> key(item->get_host(), item->get_port());
    passives[ key ] = actives[ key ];
    actives.erase( key );
}

void HandlerManager::remove(Handler* item){
    pair<const char*, int> key(item->get_host(), item->get_port());
    if( actives.find( key ) != actives.end() ){
        delete actives[ key ];
        actives.erase( key );
    }else if(passives.find( key ) != passives.end()){
        delete passives[ key ];
        passives.erase( key );
    }
}
        
/**
 *  Begin TCPServer part
 */
TCPServer::TCPServer(const char* port, int _pfd, std::atomic<bool>* _alive, 
list<Task*>*_tasks, mutex* _m_tasks){
    handlerManager = new HandlerManager();
    
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

    //event.data.fd = sfd;
    event.data.ptr = static_cast<void*>( new Handler(sfd));
    event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);
    if(s == -1){
        perror("epoll_ctl-TCPServer2");
        abort();
    }

    /* Buffer where events are returned */  
    events = static_cast<struct epoll_event*>(calloc(MAXEVENTS, sizeof event));
}

//already in cache
bool TCPServer::register_event(Handler *handler, int option, int mod){
    struct epoll_event _event;
    _event.events       = option;   
    _event.data.ptr     = static_cast<void*>( handler );
    
    if(epoll_ctl(efd, mod, handler->get_fd(), &_event) == -1){
        perror("register_event");
        close( handler->get_fd() );
        delete handler;
        return false;
    }

    return true;
}

bool TCPServer::unregister_event(Handler *handler){
    if(epoll_ctl (efd, EPOLL_CTL_DEL, handler->get_fd(), NULL) == -1){
        perror("unregister_event");
        handlerManager->remove(handler);
        return false;
    }
    
    handlerManager->set_passive( handler );
    return true;
}
    
int TCPServer::create_and_bind (const char *port){
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, sfd;

    memset (&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* All interfaces */

    s = getaddrinfo(NULL, port, &hints, &result);
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

void TCPServer::pcallback(){
    Task* task;
    m_tasks->lock();
    
    for(size_t i=0; i<tasks->size(); i++){
        task = tasks->front();
        tasks->pop_front();
         task->print();
        bool exists = handlerManager->exists( pair<const char*, int>(task->host, task->port) );
        pair<bool, Handler*> data = handlerManager->add( pair<const char*, int>(task->host, task->port) );
        
        if( !data.first ){
            delete task;
            continue;
        }
        
        if( data.second == NULL ){
            tasks->push_back( task );
            continue;
        }

        (data.second)->set_out_data( task->type, task->steal_data(), task->length );
        delete task;
        
        if( exists )
            register_event( data.second, EPOLLOUT, EPOLL_CTL_MOD);
        else
            register_event( data.second, EPOLLOUT, EPOLL_CTL_ADD);
    }
   
    m_tasks->unlock();
}

void TCPServer::wcallback(Handler* handler, msg_t type){
    unregister_event( handler );
}

void TCPServer::rcallback(Handler* handler, msg_t type){    
    unregister_event( handler );
}

int TCPServer::run(){
    struct epoll_event send_event;
    send_event.data.ptr= new Handler(pfd );
    send_event.events = EPOLLIN;

    s = epoll_ctl (efd, EPOLL_CTL_ADD, pfd, &send_event);
    if(s == -1){
        perror("epoll_ctl-TCPServer3");
        abort();
    }
    
    while( alive->load(std::memory_order_relaxed) ){
        int n = epoll_wait(efd, events, MAXEVENTS, EPOLL_TIMEOUT);
        Handler* handler = NULL;
        
        for(int i = 0; i < n; i++){
            handler = static_cast<Handler*>(events[i].data.ptr);
            
            if( handler == NULL){
                perror("epoll error unknown, no handler");
            }else if( (events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) 
                ){
                fprintf (stderr, "epoll error\n");
                
                if( pfd != handler->get_fd() ){
                    unregister_event( handler );
                    handlerManager->remove( handler );
                }
            }else if(   pfd == handler->get_fd() ){             
                char buf[32];
                while( read(pfd, buf, sizeof buf) == sizeof(buf) ){}
                pcallback();
            }else if(sfd == handler->get_fd() ){//One or more connection
                while( true ){
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

                    //if(s == 0){
                        //printf("Accepted connection on descriptor %d "
                                //"(host=%s, port=%s)\n", infd, hbuf, sbuf);
                    //}

                    if(s == 0 && make_non_blocking(infd) != -1){
                        char* end = sbuf + NI_MAXSERV;
                        
                        pair<bool, Handler*> data = handlerManager->add( 
                            pair<const char*,int>(hbuf, strtol(sbuf, &end, 0)) 
                        );
                        if( data.second == NULL ){
                            perror("Can not accept connexion, serveur overloaded");
                            close(infd);
                        }
                        (data.second)->set_fd( infd );
                        register_event( (data.second), EPOLLIN | EPOLLET, EPOLL_CTL_ADD);
                    }
                }
            }else if(events[i].events & EPOLLIN ){//Data waiting to be read
                int done = 0;

                while (1){
                    ssize_t count;
                    char buf[BUFF_SIZE];
                    count = read(handler->get_fd(), buf, sizeof buf);
                    if(handler->get_in_offset() == handler->get_expected_in_length()){//All data read
                        done = 1;
                        break;
                    }else if(count == -1 || count == 0){
                        break;
                    }

                    s = handler->add_to_in(buf, count);
                    if(s == -1){
                        perror("write");
                        abort();
                    }
                }
                
                if(done){
                    rcallback( handler, handler->get_int_type() );
                }
            }else if(events[i].events & EPOLLOUT ){
                int ret;                
                ret = handler->out_write();
       
                if( (-1 == ret && EINTR == errno) || 
                ret < handler->get_out_length()){ //Sent partial data
       
                    register_event(handler,  EPOLLOUT | EPOLLET, EPOLL_CTL_MOD);
                                       
                    if(-1 != ret){//previous write : only partial data
                        handler->decr_out_length( ret );
                        handler->incr_out_offset( ret );   
                    }
                   
                }else if(-1 == ret){ //error
                    perror("write");
                    unregister_event( handler );
                }else{ //entire data was written          
                    wcallback( handler, handler->get_out_type() );
                }
            }else{
                printf("it is an other event\n");
            }
        }
        
        m_tasks->lock();
        bool flag = tasks->size() > 0;
        m_tasks->unlock();
        if( flag )
            pcallback();
    }
}
