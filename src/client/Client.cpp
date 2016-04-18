#include "Client.h"

void TCPClientServer::wcallback(Handler* handler, msg_t type){
    if( type == EXISTS_OBJECT || type == EXISTS_CHUNKS || type == ADD_CHUNKS || 
    type == ADD_OBJECT ){
        handler->clear();
        register_event(handler, EPOLLIN, EPOLL_CTL_MOD); //reste actif
    }else{
        TCPServer::wcallback(handler, type);
    }
}
//update protocol : on ne doit envoyer/recevoir que les chunks à dedupliquer
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

void Client::dedup_chunks(vector<Chunk*>& former_chunks, vector<Chunk*>& chunks, 
unordered_map<string, bool>& mem_chunks, size_t begin){    
    vector<Chunk*>::iterator it = former_chunks.begin() + begin;
    for(; it<former_chunks.end() ; it++){
        if( mem_chunks.find( (*it)->str_digest() ) == mem_chunks.end()){

            mem_chunks[ (*it)->str_digest() ] = true;
            chunks.push_back(*it);
        }
    }
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

void Client::group_by_id(vector<Chunk*>& chunks, unordered_map<uint64_t, list< list<Chunk*> > >& buffers, size_t  begin){
    vector<Chunk*>::iterator it = chunks.begin()+begin;
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

///for writting
void Client::group_by_id(list<Chunk*>& chunks, unordered_map<uint64_t, list< list<Chunk*> > >& buffers){
    list<Chunk*>::iterator it = chunks.begin();
    vector<Node*> _nodes;
    uint64_t tmp_id=0;
    
    list< Chunk* >* current ;

    for(; it != chunks.end() ; it++){
        _nodes = nodes->wallocate( (*it)->ptr_digest(), DIGEST_LENGTH );
        
        for( size_t i=0; i<_nodes.size(); i++){
            tmp_id = _nodes[i]->get_id();

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

    vector<Chunk*> former_chunks;
    vector<Chunk*> chunks;
    unordered_map<string, bool> mem_chunks; //local dedup
    unordered_map<string, Chunk*> chunks_map; 
    unordered_map<uint64_t, list< list<Chunk*> > > buffers; //node_id => chunks of this node
    
    Timer t1;
    int pos =0;
    while( cf->next(location, former_chunks, 3000) ){
        dedup_chunks(former_chunks, chunks, mem_chunks, pos);
        group_by_id(chunks, buffers, pos);

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
        pos += 3000;
    }
    buid_digests_unordered_map(chunks, chunks_map);

    printf("Splitting into chunks and sending %lf\n", t1.elapsed());
    
    Timer t3;
    if( !wait_objects( chunks.size() ) ){
        for(size_t i = 0; i<former_chunks.size() ; i++)
            delete former_chunks[i];
        printf("fucked!!!\n");
        return false;
    }
    printf("Waiting chunks1 %lf\n", t3.elapsed());
        
        
    Timer t4;
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
    printf("dedup chunks %lf\n", t4.elapsed());
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
        printf("storgin and dedup chunks %lf\n", t4.elapsed());
        
        ///Send new chunk to sd
        unordered_map<uint64_t, list< list<Chunk*> > > _buffers;
        group_by_id( to_dedup, _buffers);

        for(unordered_map<uint64_t, list< list<Chunk*> > >::iterator it = _buffers.begin() ; 
        it != _buffers.end(); it++){
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
    }
    ///Send file digest
    vector<Node*> _nodes = nodes->wallocate( file_digest, DIGEST_LENGTH );
    for(int k=0 ; k< _nodes.size() ; k++)       
        send(ADD_OBJECT, file_digest, DIGEST_LENGTH, _nodes[k]->get_host(), _nodes[k]->get_port());
    populate_additions( file_digest );
    
    if( !wait_additions() ){
        for(size_t i = 0; i<former_chunks.size() ; i++)
            delete former_chunks[i];
        return false;
    }

    
    clear_objects();
    
    Timer t6;
    if( !buildMetadata(name, file_digest, former_chunks, path_dir) ){
        for(size_t i = 0; i<former_chunks.size() ; i++)
            delete former_chunks[i];
        return false;
    }
    
    int bb=0;
    for(size_t i = 0; i<former_chunks.size() ; i++){
        bb += former_chunks[i]->get_length();
    }
    for(size_t i = 0; i<former_chunks.size() ; i++)
        delete former_chunks[i];
    
    printf("metadata building %lf in %d\n", t6.elapsed(), bb);
    return true;
}

struct ChunkAsc{
    bool operator() (Chunk* lhs, Chunk* rhs) const{
        return memcmp(lhs->get_digest(), rhs->get_digest(), DIGEST_LENGTH )<0 ;
    }
};

//bool Client::bsave(const char* name, const char* location, fs::path path_dir){
    //char file_digest[DIGEST_LENGTH];
    //if( dedup_by_file(location, file_digest) )
        //return true;

    //vector<Chunk*> former_chunks;
    //vector<Chunk*> chunks;
    //unordered_map<string, bool> mem_chunks; //local dedup
    //unordered_map<string, Chunk*> chunks_map; 
    //unordered_map<uint64_t, list< list<Chunk*> > > buffers; //node_id => chunks of this node
    
    //int pos =0;
    //while( cf->next(location, former_chunks, 3000) ){
        //dedup_chunks(former_chunks, chunks, mem_chunks, pos);
    //}
    
    //std::sort( chunks.begin(), chunks.end(), ChunkAsc() ); //ameliorer dedup local à l'aide de l'ordre
    //char* bin = new char[ (1+chunks.size()) * DIGEST_LENGTH];
    //for( size_t i = 0; i<chunks.size(); i++)
        //memcpy(bin + (i +1) * DIGEST_LENGTH, chunks[i]->get_digest(), DIGEST_LENGTH);
    ////memcpy(bin_id, bin+(hunks.size()-1)*DIGEST_LENGTH, DIGEST_LENGTH);
    
    ////un seul big msg 
    ////si un seul big msg faut augmenter la taille max des buffers. => 256Mo ??
    
    ////format bin : bin_id|data
    //for(size_t i=0 ; i < min( BIN_R, chunks.size()) ; i++){
        //memcpy(bin, chunks[chunks.size()-1-i]->get_digest(), DIGEST_LENGTH);
        //Node* node = nodes->rallocate( chunks[chunks.size()-1-i]->get_digest(), DIGEST_LENGTH );
        //send(EXISTS_BIN, bin, (1+chunks.size()) * DIGEST_LENGTH, node->get_host(), node->get_port());
    //}
    
    //Timer t3;
    //if( !wait_objects( chunks.size() ) || !wait_bin() ){
        //for(size_t i = 0; i<former_chunks.size() ; i++)
            //delete former_chunks[i];
        //printf("fucked!!!\n");
        //return false;
    //}
    //printf("Waiting chunks1 %lf\n", t3.elapsed());
    ////on attent la liste de chunks à deduplique ou un msg donnant l'ensemble du bin à dédupliquer
    
    ////on store
    
    ////on envoit fichier et bin
    
//}

bool Client::load(const char* name, const char* location, fs::path path_dir){
    ofstream os( location, ios::binary);    
    if( !os ){
        perror("Client::load");
        return false;
    }
    
    vector<Chunk*> chunks;
    char file_digest[ DIGEST_LENGTH ];
    extractChunks( name, file_digest, chunks, path_dir);
    
    char* buffer = static_cast<char*>(malloc(BUFFER_MAX_SIZE));

    size_t aa=0;

    for(uint64_t i = 0 ; i<chunks.size() ; i++){
        string tmp=(path_dir/fs::path(chunks[i]->str_digest())).string();
        ifstream is(tmp.c_str(), ios::binary);

        is.read( buffer, chunks[i]->get_length() );
        os.write( buffer, chunks[i]->get_length() );
        aa += chunks[i]->get_length();
        
        is.close();
    }
    printf("size calculated %d\n", aa);
    os.close();
    free(buffer);
    
    for(size_t i=0 ; i<chunks.size() ; i++)
        delete chunks[i];
    
    char restored_digest[ DIGEST_LENGTH ];
    if( !hashfile(location, restored_digest) ){
        perror("Hashing failed");
        return false;
    }
    
    if( memcmp(file_digest, restored_digest, DIGEST_LENGTH) != 0) //file corrupted
        return false;
    
    return true;
}
