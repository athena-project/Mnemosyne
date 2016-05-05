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

///we  only received chunks to store
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
        m_objects->lock();
        for(int i = 0 ; i<num; i++, data+=sizeof(char)+DIGEST_LENGTH){
            char exists = data[DIGEST_LENGTH];
            objects->push_back( make_pair(digest_to_string(data), exists) );
        }
        m_objects->unlock();
    }else if( type == BIN ){
        uint64_t num = 0;
        char* digest = data;
        char* end = data + uint64_s + DIGEST_LENGTH;
        num = strtoull(data + DIGEST_LENGTH, &end, 0);

        data += uint64_s + DIGEST_LENGTH;
        
        m_bins->lock();
        list<string>* tmp = &((*bins)[digest_to_string(digest)]);
        (*tmp) = list<string>();
        for(int i = 0 ; i<num; i++, data+=DIGEST_LENGTH)
            tmp->push_back( digest_to_string(data) );
        m_bins->unlock();

    }else if( type == WHOLE_BIN){
        m_bins->lock();

        list<string>* tmp = &((*bins)[digest_to_string(data)]);
        (*tmp) = list<string>();
        tmp->push_back( "" );
        
        m_bins->unlock();
    }else if( type == OBJECT_ADDED ){
        std::string digest = digest_to_string( data );

        m_additions->lock();
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
            additions->erase( digest );
            m_additions->unlock();
        }
    }else if( type == BIN_ADDED ){
        std::string digest = digest_to_string( data );

        m_additions->lock();
        additions->erase( digest );
        m_additions->unlock();
    }
    
    TCPServer::rcallback(handler, type);
}

///Save functions
Client::Client(const char* port, NodeMap* _nodes) : nodes(_nodes){
    cf = new ChunkFactory("chunks_factory_conf.data");
    
    pipe(pfds);
    alive.store(true,std::memory_order_relaxed); 
    
    server = new TCPClientServer(port, pfds[0], &alive, &tasks, &m_tasks,
    &objects, &m_objects, &bins, &m_bins, &additions, &m_additions);
    
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

bool Client::wait_bins(list<string>& w_bins){
    struct timespec req, rem;
    uint64_t duration = 0;
    bool flag =false;
    
    req.tv_sec  = 0;
    req.tv_nsec = DELAY; //0.001 s
    
    while( !flag && duration < TIME_OUT){
        nanosleep(&req, &rem);
        duration += DELAY;

        m_bins.lock();
        list<string>::iterator it = w_bins.begin();
        while(it!=w_bins.end()){
            if( bins.find(*it) != bins.end() )
                w_bins.erase( it++ );
            else
                ++it;
        }
        flag = w_bins.empty();
        m_bins.unlock();
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

void Client::build_to_dedup(list<Chunk*>& to_dedup, unordered_map<string, Chunk*>& chunks_map){
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
}

void Client::build_to_dedup_b(list<Chunk*>& to_dedup, vector<Chunk*>& chunks){//o(nlog(n))
    //we will do kind of intersectiob
    unordered_map<string,bool> m_chunks;  //to dedup eventually
    unordered_map<string,bool> tmp_chunks;  //to dedup eventually
    
    m_bins.lock();

    for( map<string, list<string> >::iterator it = bins.begin(); it!= bins.end(); it++)
        if( it->second.front() == "")
            bins.erase( it );
            
    if( bins.size() == 0){ //case where all bins ""
        for( size_t i=0; i<chunks.size() ; ++i)
            to_dedup.push_back( chunks[i] );
        return;
    }
    
    for( list<string>::iterator it2 = (bins.begin()->second).begin() ; it2 != (bins.begin()->second).end() ; it2++)
        m_chunks[ *it2 ] = true;
        
    for( map<string, list<string> >::iterator it = bins.begin()++; it!= bins.end(); it++){
        tmp_chunks.clear();
        
        for( list<string>::iterator it2 = it->second.begin() ; it2 != it->second.end() ; it2++)
            tmp_chunks[ *it2 ] = true;
    
        for( unordered_map<string,bool>::iterator it2 = m_chunks.begin() ; it2 != m_chunks.end() ; it2++){
            if( tmp_chunks.find( it2->first ) == tmp_chunks.end() )
                m_chunks.erase( it2);
        }
    }
    m_bins.unlock();

    //m_chunks//to les ids des chunks a conserver
    for( size_t i=0; i<chunks.size() ; ++i)
        if( m_chunks.find( chunks[i]->str_digest() ) != m_chunks.end() )
            to_dedup.push_back( chunks[i] );
    return;
}

bool Client::store_chunks(list<Chunk*>& to_dedup, const char* location, fs::path path_dir){
    printf("trying to store chunks %zu\n", to_dedup.size()); 
    if( to_dedup.size() == 0)
        return false;
        
    if( to_dedup.front()->get_data() != NULL ){ ///Chunks' cache enable 
        printf("hello storing chunks\n");   
        for(list<Chunk*>::iterator it = to_dedup.begin() ; //maybe we can compress chunk ?
        it != to_dedup.end(); it++){    
            string tmp=(path_dir/fs::path((*it)->str_digest())).string();
            ofstream c_file(tmp.c_str(), ios::binary);
            c_file.write( (*it)->get_data(), (*it)->get_length() );
        }
    }else{   ///Chunks' cache not enable : file too big
                printf("hello storing chunks 55\n");   

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
    return true;
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
    build_to_dedup(to_dedup, chunks_map);

    printf("dedup chunks %lf\n", t4.elapsed());
    ///Store chunks
    if( !store_chunks(to_dedup, location, path_dir)){
        for(size_t i = 0; i<former_chunks.size() ; i++)
            delete former_chunks[i];
        return false;
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
        return memcmp(lhs->ptr_digest(), rhs->ptr_digest(), DIGEST_LENGTH )<0 ;
    }
};

bool Client::bsave(const char* name, const char* location, fs::path path_dir){
    char file_digest[DIGEST_LENGTH];
    if( dedup_by_file(location, file_digest) )
        return true;

    vector<Chunk*> former_chunks;
    vector<Chunk*> chunks;
    unordered_map<string, bool> mem_chunks; //local dedup
    unordered_map<string, Chunk*> chunks_map; 
    unordered_map<uint64_t, list< list<Chunk*> > > buffers; //node_id => chunks of this node
    
    int pos =0;
    while( cf->next(location, former_chunks, 3000) ){
        dedup_chunks(former_chunks, chunks, mem_chunks, pos);
    }
    
    std::sort( chunks.begin(), chunks.end(), ChunkAsc() ); //ameliorer dedup local à l'aide de l'ordre
    char* bin = new char[ (1+chunks.size()) * DIGEST_LENGTH + sizeof(uint64_t)];
    sprintf(bin+sizeof(uint64_t), "%" PRIu64 "", chunks.size());

    for( size_t i = 0; i<chunks.size(); i++)
        memcpy(bin + sizeof(uint64_t) + (i +1) * DIGEST_LENGTH, chunks[i]->ptr_digest(), DIGEST_LENGTH);
    
    //si un seul big msg faut augmenter la taille max des buffers. => 256Mo ??    
    list<string> waiting_bins;
    for(size_t i=0 ; i < min( (size_t)BIN_R, chunks.size()) ; i++){
        printf("sending bins\n");
        memcpy(bin, chunks[chunks.size()-1-i]->ptr_digest(), DIGEST_LENGTH);//id of current bin
        Node* node = nodes->rallocate( bin, DIGEST_LENGTH );
        send(EXISTS_BIN, bin, (1+chunks.size()) * DIGEST_LENGTH, node->get_host(), node->get_port());
        waiting_bins.push_back( digest_to_string(bin));
        printf("bins send to %s %d\n", chunks[chunks.size()-1-i]->ptr_digest() ,node->get_port());
    }
    
    Timer t3;
    if( !wait_bins( waiting_bins ) ){
        for(size_t i = 0; i<former_chunks.size() ; i++)
            delete former_chunks[i];
        printf("fucked!!!\n");
        return false;
    }
    printf("Waiting chunks1 %lf\n", t3.elapsed());
    
    //on store
    Timer t4;
    list<Chunk*> to_dedup;
    build_to_dedup_b(to_dedup, chunks);
    printf("dedup chunks %lf\n", t4.elapsed());
    
    if( !store_chunks(to_dedup, location, path_dir) ){
        for(size_t i = 0; i<former_chunks.size() ; i++)
            delete former_chunks[i];
        return false;
    }
    
    for(size_t i=0 ; i < min( (size_t)BIN_W, chunks.size()) ; i++){
        memcpy(bin, chunks[chunks.size()-1-i]->ptr_digest(), DIGEST_LENGTH); //id of current bin
        vector<Node*> _nodes = nodes->wallocate( bin, DIGEST_LENGTH );
        for(size_t j=0 ; j < _nodes.size() ; j++)
            send(ADD_BIN, bin, (1+chunks.size()) * DIGEST_LENGTH, _nodes[j]->get_host(), _nodes[j]->get_port());
        populate_additions( bin );
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
    printf("size calculated %zu\n", aa);
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
