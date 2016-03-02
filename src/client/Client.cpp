#include "Client.h"

void TCPClientServer::wcallback(Handler* handler, msg_t type){
    if( type == EXISTS_OBJECT || type == EXISTS_CHUNKS || type == ADD_CHUNKS || 
    type == ADD_OBJECT ){
        handler->clear();
        register_event(handler, EPOLLIN, EPOLL_CTL_MOD);
    }else{
        close( handler->get_fd() );
        delete handler;
    }
}

void TCPClientServer::rcallback(Handler* handler, msg_t type){  
    char* data = handler->get_in_data();
 
    if( type == OBJECT){
        char digest[DIGEST_LENGTH];
        char exists;

        memcpy(digest, data, DIGEST_LENGTH);
        memcpy(&exists, data + DIGEST_LENGTH, sizeof(char));
        
        std::string alpha  = digest_to_string( digest) ;
        printf("ff : %s\n", digest);
        m_objects->lock();
        objects->push_back( make_pair(digest, exists) );
        m_objects->unlock();
    }else if( type == CHUNKS ){
        size_t num = 0;
        char* end = data + sizeof(size_t);
        num = strtoull(data, &end, 0);
        printf("Getting chunks!!!!!!!!!!!!!!!!!! %d_%d", num, handler->get_in_offset());
        data += sizeof(size_t);
        for(int i = 0 ; i<num; i++, data+=sizeof(char)+DIGEST_LENGTH){
            char digest[DIGEST_LENGTH];
            char exists;
            memcpy(digest, data, DIGEST_LENGTH);
            memcpy(&exists, data+DIGEST_LENGTH, 1);
        
            m_objects->lock();
            objects->push_back( make_pair(digest, exists) );
            m_objects->unlock();
        }
    }else if( type == OBJECT_ADDED ){
        char digest[DIGEST_LENGTH];
        char exists;
        
        memcpy(digest, data, DIGEST_LENGTH);
        memcpy(&exists, data+DIGEST_LENGTH, sizeof(char));
        
        m_additions->lock();
        if( additions->find( string(digest) ) != additions->end() )
            additions->erase( string(digest) );
        m_additions->unlock();
    }else if( type == CHUNKS_ADDED ){
        size_t num = 0;
        char* end = data + sizeof(size_t);
        num = strtoull(data, &end, 0);
        
        data += sizeof(size_t);
        for(int i = 0 ; i<num; i++, data+=sizeof(char)+DIGEST_LENGTH){
            char digest[DIGEST_LENGTH];
            memcpy(digest, data, DIGEST_LENGTH);
        
            m_additions->lock();
            if( additions->find( string(digest) ) != additions->end() )
                additions->erase( string(digest) );
            m_additions->unlock();
        }
    }
    
    close( handler->get_fd() );
    delete handler;
}


///Save functions
Client::Client(const char* port, NodeMap* _nodes) : nodes(_nodes){
    cf = new ChunkFactory("chunks_factory_conf.data");
    
    pipe(pfds);
    alive.store(true,std::memory_order_relaxed); 
    
    server = new TCPClientServer(port, pfds[0], &alive, &tasks, &m_tasks,
    &objects, &m_objects,&additions, &m_additions);
    
    t_server = std::thread( run_server,server );
    t_server.detach();
} 

Client::~Client(){
    delete cf;
}

void Client::clear_objects(){
    m_objects.lock();
    objects.clear();
    m_objects.unlock();
}


///agregate digest in : sizedigest1digest2....
void Client::build_digests(list<Chunk*>& chunks, char* digests){
    list<Chunk*>::iterator it = chunks.begin();
    
    sprintf(digests, "%lu", chunks.size());
    
    for(int k=sizeof(size_t); it != chunks.end() ; k += DIGEST_LENGTH, it++)
        (*it)->_digest( digests+k );
}

void Client::buid_digests_map(vector<Chunk*>& chunks, map<string, Chunk*>& map){
    vector<Chunk*>::iterator it = chunks.begin();

    for(; it != chunks.end() ; it++)
        map[ (*it)->str_digest() ] = *it;
}

void Client::group_by_id(vector<Chunk*>& chunks, map<uint64_t, list<Chunk*> >& buffers){
    uint64_t tmp_id;
    printf("nodes_%d\n", nodes->size());
    for(int i=0; i<chunks.size(); i++){
        tmp_id = nodes->rallocate( chunks[i]->ptr_digest(), DIGEST_LENGTH )->get_id();
        
        if( buffers.find(tmp_id) == buffers.end() )
            buffers[tmp_id]=list<Chunk*>();
        
        buffers[tmp_id].push_back( chunks[i] );
    }
} 

void Client::group_by_ids(list<Chunk*>& chunks, map<uint64_t, list<Chunk*> >& buffers){
    list<Chunk*>::iterator it = chunks.begin();
    vector<Node*> _nodes;
    uint64_t tmp_id;
    
    for(; it != chunks.end() ; it++){
        _nodes = nodes->wallocate( (*it)->ptr_digest(), DIGEST_LENGTH );
        
        for( int k=0; k< _nodes.size(); k++){
            tmp_id = _nodes[k]->get_id();
            
            if( buffers.find(tmp_id) == buffers.end() )
                buffers[tmp_id]=list<Chunk*>();
            
            buffers[tmp_id].push_back( (*it) );
        }
    }
} 

///n act as a condition
bool Client::wait_objects(int n){
    struct timespec req, rem;
    uint64_t duration = 0;
    bool flag =false;
    
    req.tv_sec  = 0;
    req.tv_nsec = DELAY; //0.001 s
    
    while( !flag && duration < TIME_OUT){
        nanosleep(&req, &rem);
        duration += DELAY;
        //printf("size : %d\n", objects.size());
        m_objects.lock();
        flag = (objects.size() == n);
        m_objects.unlock();
    }
    
    return flag;
}

bool Client::wait_additions(){
    struct timespec req, rem;
    uint64_t duration = 0;
    bool flag =false;
    
    req.tv_sec  = 0;
    req.tv_nsec = DELAY; //0.001 s
    
    while( !flag && duration < TIME_OUT){
        nanosleep(&req, &rem);
        duration += DELAY;
        
        m_additions.lock();
        flag = (additions.size() == 0 );
        m_additions.unlock();
    }
    
    return flag;
}

void Client::populate_additions( list<Chunk*>& chunks ){
    list<Chunk*>::iterator it = chunks.begin();
    
    m_additions.lock();
    for(; it != chunks.end() ; it++)
        additions[ (*it)->str_digest() ] = true;
    
    m_additions.unlock();
}

void Client::populate_additions( char* digest ){
    m_additions.lock();
    additions[ digest_to_string(digest) ] = true;
    m_additions.unlock();
}

///true : file already exists, else not
bool Client::dedup_by_file(const char* location, char* file_digest){

    if( !hashfile(location, file_digest) ){
        perror("Hashing failed");
        return false;
    }
    Node* prime = nodes->rallocate( file_digest, DIGEST_LENGTH);
    send(EXISTS_OBJECT, file_digest, DIGEST_LENGTH, prime->get_host(), 
        prime->get_port());

    if( !wait_objects( 1 ) )
        return false;
    
    if( objects.front().second ){//file exists
        clear_objects();
        return true;
    }else
        clear_objects();
        
    return false;
}

bool Client::save(const char* name, const char* location, fs::path path_dir){
    char file_digest[DIGEST_LENGTH];
    if( dedup_by_file(location, file_digest) )
        return true;

    vector<Chunk*> chunks;
    map<string, Chunk*> chunks_map; 
    map<uint64_t, list<Chunk*> > buffers; //node_id => chunks of this node

    cf->split(location, chunks);
    buid_digests_map( chunks, chunks_map);

    ///Send requests
    group_by_id( chunks, buffers);
    
    for(map<uint64_t, list<Chunk*> >::iterator it = buffers.begin() ; 
    it != buffers.end(); it++){
        size_t buffer_len = (it->second).size() * DIGEST_LENGTH + sizeof(size_t);
        char buffer[buffer_len];
        
        build_digests( it->second, buffer);
        
        Node* node = nodes->get_node( it->first );
        send(EXISTS_CHUNKS, buffer, buffer_len, node->get_host(), node->get_port());
    }
    
    printf("Before waiting!!!!!!!! %d\n", chunks.size());
    if( !wait_objects( chunks.size() ) ){
        for(size_t i = 0; i<chunks.size() ; i++)
            delete chunks[i];
        return false;
    }
    printf("End of waiting\n");
    ///Select chunks to dedup
    list<Chunk*> to_dedup;
    char* tmp_digest;
    char tmp_exists;
    
    m_objects.lock();
    for(int i=0; i<objects.size(); i++){
        printf("%d\n", i);
        tmp_digest = objects.front().first;
        tmp_exists = objects.front().second;
        if( !tmp_exists && chunks_map.find(tmp_digest) != chunks_map.end()){
            to_dedup.push_back( chunks_map[ tmp_digest ] );
        }
        objects.pop_front();
    }
    printf("end_of_save\n");
    m_objects.unlock();
    return false;
    ///Store chunks
    if( to_dedup.size() > 0){               
        if( to_dedup.front()->get_data() != NULL ){
            for(list<Chunk*>::iterator it = to_dedup.begin() ; 
            it != to_dedup.end(); it++){    
                string tmp=(path_dir/fs::path((*it)->str_digest())).string();
                ofstream c_file(tmp.c_str(), ios::binary);
                c_file.write( (*it)->get_data(), (*it)->get_length() );
            }
        }
        else{   
            char *src;
            int fd = open(location, O_RDONLY);
            uint64_t size_file = size_of_file(fd);
            src = static_cast<char*>( 
                mmap(NULL, size_file, PROT_READ, MAP_PRIVATE, fd, 0));
            
            
            for(list<Chunk*>::iterator it = to_dedup.begin() ; 
            it != to_dedup.end(); it++){
                string tmp=(path_dir/fs::path((*it)->str_digest())).string();
                ofstream c_file(tmp.c_str(), ios::binary);
                
                c_file.write( src+(*it)->get_begin(), (*it)->get_length() );
                c_file.close();
            }
            
            munmap( src, size_file);
            close( fd );    
        }
            return false;

        ///Send new chunk to sd
        map<uint64_t, list<Chunk*> > _buffers;
        group_by_ids( to_dedup, _buffers);
        
        for(map<uint64_t, list<Chunk*> >::iterator it = buffers.begin() ; 
        it != buffers.end(); it++){
            size_t buffer_len = (it->second).size() * DIGEST_LENGTH + sizeof(size_t);
            char buffer[ buffer_len ];
            
            build_digests( it->second, buffer);
            populate_additions( it->second );
            
            Node* node = nodes->get_node( it->first );
            send(ADD_CHUNKS, buffer, buffer_len, node->get_host(), node->get_port());
        }
        
        ///Send file digest
        vector<Node*> _nodes = nodes->wallocate( file_digest, DIGEST_LENGTH );
        for(int k=0 ; k< _nodes.size() ; k++)       
            send(ADD_OBJECT, file_digest, DIGEST_LENGTH, _nodes[k]->get_host(), _nodes[k]->get_port());
        populate_additions( file_digest );
        
        if( !wait_additions() ){
            for(size_t i = 0; i<chunks.size() ; i++)
                delete chunks[i];
            return false;
        }
    }
    clear_objects();
    
    
    
    if( !buildMetadata(name, chunks, path_dir) ){
        for(size_t i = 0; i<chunks.size() ; i++)
            delete chunks[i];
        return false;
    }
    return true;
}

bool Client::load(const char* name, const char* location, fs::path path_dir){
    vector<Chunk*> chunks;
    extractChunks( name, chunks, path_dir);
    ofstream os( location, ios::binary);    
    
    if( !os ){
        perror("Client::load");
        return false;
    }
    
    char buffer[BUFFER_MAX_SIZE];
    uint64_t b_length = 0;
    
    for(uint64_t i = 0 ; i<chunks.size() ; i++){
        string tmp=(path_dir/fs::path(chunks[i]->str_digest())).string();
        ifstream is(tmp.c_str(), ios::binary);
        
        is.read( buffer, chunks[i]->get_length() );
        os.write( buffer, chunks[i]->get_length() );
        
        is.close();
    }
    os.close();
    
    return true;
}
