#include "Client.h"

void TCPClientServer::wcallback(Handler* handler, msg_t type){
    if( type == EXISTS_OBJECT || type == EXISTS_CHUNKS || type == ADD_CHUNKS || 
    type == ADD_OBJECT ){
        handler->clear();
        register_event(handler, EPOLLIN, EPOLL_CTL_MOD);
    }else{
        TCPServer::wcallback(handler, type);
    }
}

void TCPClientServer::rcallback(Handler* handler, msg_t type){  
    char* data = handler->get_in_data();
 
    if( type == OBJECT){
        char exists = data[DIGEST_LENGTH];
        
        m_objects->lock();
        objects->push_back( make_pair(digest_to_string(data), exists) );
        m_objects->unlock();
    }else if( type == CHUNKS ){
        uint64_t num = 0;
        char* end = data + uint64_s;
        num = strtoull(data, &end, 0);

        data += uint64_s;
        for(int i = 0 ; i<num; i++, data+=sizeof(char)+DIGEST_LENGTH){
            char exists = data[DIGEST_LENGTH];

            m_objects->lock();
            objects->push_back( make_pair(digest_to_string(data), exists) );
            m_objects->unlock();
        }
    }else if( type == OBJECT_ADDED ){
        std::string digest = digest_to_string( data );

        m_additions->lock();
        if( additions->find( digest ) != additions->end() )
            additions->erase( digest );
        m_additions->unlock();
    }else if( type == CHUNKS_ADDED ){
        uint64_t num = 0;
        char* end = data + uint64_s;
        num = strtoull(data, &end, 0);

        data += uint64_s;
        for(int i = 0 ; i<num; i++, data+=DIGEST_LENGTH){
            std::string digest = digest_to_string( data );

            m_additions->lock();
            if( additions->find( digest ) != additions->end() )
                additions->erase( digest );
            m_additions->unlock();
        }
    }
    
    TCPServer::rcallback(handler, type);
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
    
    sprintf(digests, "%" PRIu64 "", chunks.size());
    
    for(int k=uint64_s; it != chunks.end() ; k += DIGEST_LENGTH, it++)
        (*it)->_digest( digests+k );
}

void Client::buid_digests_unordered_map(vector<Chunk*>& chunks, unordered_map<string, Chunk*>& unordered_map){
    vector<Chunk*>::iterator it = chunks.begin();

    for(; it != chunks.end() ; it++)
        unordered_map[ (*it)->str_digest() ] = *it;
}

void Client::group_by_id(vector<Chunk*>& chunks, unordered_map<uint64_t, list< list<Chunk*> > >& buffers){
    vector<Chunk*>::iterator it = chunks.begin();
    uint64_t tmp_id;
    
    list< Chunk* >* current ;

    for(; it != chunks.end() ; it++){
        tmp_id = nodes->rallocate( (*it)->ptr_digest(), DIGEST_LENGTH )->get_id();
        
        if( buffers.find(tmp_id) == buffers.end() ){
            buffers[tmp_id]=list< list<Chunk*> >( );
            buffers[tmp_id].push_front( list<Chunk*>() );
        }

        current = &buffers[tmp_id].back();
        if( current->size() >= BUNDLE_MAX_SIZE ){
            buffers[tmp_id].push_back( list<Chunk*>() );
            current = &buffers[tmp_id].back();
        }
        
        current->push_back( (*it) );

    }
} 

void Client::group_by_id(list<Chunk*>& chunks, unordered_map<uint64_t, list< list<Chunk*> > >& buffers){
    list<Chunk*>::iterator it = chunks.begin();
    uint64_t tmp_id;
    
    list< Chunk* >* current ;

    for(; it != chunks.end() ; it++){
        tmp_id = nodes->rallocate( (*it)->ptr_digest(), DIGEST_LENGTH )->get_id();
        
        if( buffers.find(tmp_id) == buffers.end() ){
            buffers[tmp_id]=list< list<Chunk*> >( );
            buffers[tmp_id].push_front( list<Chunk*>() );
        }

        current = &buffers[tmp_id].back();
        if( current->size() >= BUNDLE_MAX_SIZE ){
            buffers[tmp_id].push_back( list<Chunk*>() );
            current = &buffers[tmp_id].back();
        }
        
        current->push_back( (*it) );

    }
} 

///n act as a condition
bool Client::wait_objects(int n){
    struct timespec req, rem;
    uint64_t duration = 0;

    m_objects.lock();
    uint64_t last_n = objects.size();
    m_objects.unlock();
    
    req.tv_sec  = 0;
    req.tv_nsec = DELAY;
    
    while( last_n != n && duration < TIME_OUT){
        nanosleep(&req, &rem);
        duration += DELAY;

        m_objects.lock();
        
        duration += (last_n  == objects.size()) ? DELAY : 0;
        last_n = objects.size();
       
        m_objects.unlock();        
    }
    
    return last_n == n;
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
    unordered_map<string, Chunk*> chunks_map; 
    unordered_map<uint64_t, list< list<Chunk*> > > buffers; //node_id => chunks of this node

    cf->split(location, chunks);
    buid_digests_unordered_map( chunks, chunks_map);

    ///Send requests
    group_by_id( chunks, buffers);

    for(unordered_map<uint64_t, list< list<Chunk*> > >::iterator it = buffers.begin() ; 
    it != buffers.end(); it++){
        for(list< list<Chunk*> >::iterator it_bundle = (it->second).begin() ; it_bundle != (it->second).end(); it_bundle++){ 
            size_t buffer_len = it_bundle->size() * DIGEST_LENGTH + uint64_s;
            char buffer[buffer_len];

            build_digests( *it_bundle, buffer);

            Node* node = nodes->get_node( it->first );
            send(EXISTS_CHUNKS, buffer, buffer_len, node->get_host(), node->get_port());
        }
    }
    buffers.clear();
    
    if( !wait_objects( chunks.size() ) ){
        for(size_t i = 0; i<chunks.size() ; i++)
            delete chunks[i];

        return false;
    }

    ///Select chunks to dedup
    list<Chunk*> to_dedup;
    std::string tmp_digest;
    char tmp_exists;
    
    m_objects.lock();
    for(int i=0, num = objects.size() ; i<num ; i++){
        tmp_digest = objects.front().first;
        tmp_exists = objects.front().second;
        
        if( !tmp_exists && chunks_map.find(tmp_digest) != chunks_map.end()){
            to_dedup.push_back( chunks_map[ tmp_digest ] );
        }
        objects.pop_front();
    }
    m_objects.unlock();

    ///Store chunks
    if( to_dedup.size() > 0){               
        if( to_dedup.front()->get_data() != NULL ){ ///Chunks' cache enable 
            for(list<Chunk*>::iterator it = to_dedup.begin() ; //maybe we can compress chunk ?
            it != to_dedup.end(); it++){    
                string tmp=(path_dir/fs::path((*it)->str_digest())).string();
                ofstream c_file(tmp.c_str(), ios::binary);
                c_file.write( (*it)->get_data(), (*it)->get_length() );
            }
        }
        else{   ///Chunks' cache not enable : file too big
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

        ///Send new chunk to sd
        unordered_map<uint64_t, list< list<Chunk*> > > _buffers;
        group_by_id( to_dedup, _buffers);
        
        for(unordered_map<uint64_t, list< list<Chunk*> > >::iterator it = buffers.begin() ; 
        it != buffers.end(); it++){
            for(list< list<Chunk*> >::iterator it_bundle = (it->second).begin() ; it_bundle != (it->second).end(); it_bundle++){ 
                size_t buffer_len = it_bundle->size() * DIGEST_LENGTH + uint64_s;
                char buffer[ buffer_len ];
                
                build_digests( *it_bundle, buffer);
                populate_additions( *it_bundle );
              
                Node* node = nodes->get_node( it->first );
                send(ADD_CHUNKS, buffer, buffer_len, node->get_host(), node->get_port());
            }
        }
        _buffers.clear();
        
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
        
    if( !buildMetadata(name, file_digest, chunks, path_dir) ){
        for(size_t i = 0; i<chunks.size() ; i++)
            delete chunks[i];
        return false;
    }

    return true;
}

bool Client::load(const char* name, const char* location, fs::path path_dir){
    vector<Chunk*> chunks;
    char file_digest[ DIGEST_LENGTH ];
    extractChunks( name, file_digest, chunks, path_dir);
    ofstream os( location, ios::binary);    
    
    if( !os ){
        perror("Client::load");
        return false;
    }
    
    char* buffer = static_cast<char*>(malloc(BUFFER_MAX_SIZE)); //because new failed

    for(uint64_t i = 0 ; i<chunks.size() ; i++){
        string tmp=(path_dir/fs::path(chunks[i]->str_digest())).string();
        ifstream is(tmp.c_str(), ios::binary);
        
        is.read( buffer, chunks[i]->get_length() );
        os.write( buffer, chunks[i]->get_length() );
        
        is.close();
    }
    os.close();
    free(buffer);
    
    char restored_digest[ DIGEST_LENGTH ];
    if( !hashfile(location, restored_digest) ){
        perror("Hashing failed");
        return false;
    }
    
    if( memcmp(file_digest, restored_digest, DIGEST_LENGTH) != 0) //file corrupted
        return false;
    
    return true;
}
