#include "Client.h"

void TCPClientServer::wcallback(Handler* handler, msg_t type){
	if( type == EXISTS_OBJECT || type == EXISTS_CHUNKS || type == ADD_CHUNKS || 
	type == ADD_OBJECT ){
		handler->clear();
		register_event(handler, EPOLLIN, EPOLL_CTL_MOD);
	}else{
		close( handler->get_fd() );
		free( handler );
	}
}

void TCPClientServer::rcallback(Handler* handler, msg_t type){	
	char* data = handler->get_in_data() + Msg::HEADER_LENGTH;
	
	if( type == OBJECT){
		char digest[SHA224_DIGEST_LENGTH];
		char exists;
		
		memcpy(digest, data, SHA224_DIGEST_LENGTH);
		memcpy(&exists, data+SHA224_DIGEST_LENGTH, sizeof(char));
		
		m_objects->lock();
		objects->push_back( make_pair(digest, exists) );
		m_objects->unlock();
	}else if( type == CHUNKS ){
		size_t num = 0;
		char* end = data + sizeof(size_t);
		num = strtoull(data, &end, 0);
		
		data += sizeof(size_t);
		for(int i = 0 ; i<num; i++, data+=sizeof(char)+SHA224_DIGEST_LENGTH){
			char digest[SHA224_DIGEST_LENGTH];
			char exists;
			memcpy(digest, data, SHA224_DIGEST_LENGTH);
			memcpy(&exists, data+SHA224_DIGEST_LENGTH, 1);
		
			m_objects->lock();
			objects->push_back( make_pair(digest, exists) );
			m_objects->unlock();
		}
	}else if( type == OBJECT_ADDED ){
		char digest[SHA224_DIGEST_LENGTH];
		char exists;
		
		memcpy(digest, data, SHA224_DIGEST_LENGTH);
		memcpy(&exists, data+SHA224_DIGEST_LENGTH, sizeof(char));
		
		m_additions->lock();
		if( additions->find( string(digest) ) != additions->end() )
			additions->erase( string(digest) );
		m_additions->unlock();
	}else if( type == CHUNKS_ADDED ){
		size_t num = 0;
		char* end = data + sizeof(size_t);
		num = strtoull(data, &end, 0);
		
		data += sizeof(size_t);
		for(int i = 0 ; i<num; i++, data+=sizeof(char)+SHA224_DIGEST_LENGTH){
			char digest[SHA224_DIGEST_LENGTH];
			memcpy(digest, data, SHA224_DIGEST_LENGTH);
		
			m_additions->lock();
			if( additions->find( string(digest) ) != additions->end() )
				additions->erase( string(digest) );
			m_additions->unlock();
		}
	}
	
	close( handler->get_fd() );
	free( handler );
}


///Save functions
Client::Client(char* port, char* _nodes){
	nodes = new NodeMap(3);
	cf = new ChunkFactory("krh.ser");
	
	pipe(pfds);
	alive.store(true,std::memory_order_relaxed); 
	
	server = new TCPClientServer(port, pfds[0], &alive, &tasks, &m_tasks,
	&objects, &m_objects,&additions, &m_additions);
	
	t_server = std::thread( run_server,server );
	t_server.detach();
} 

Client::~Client(){
	delete nodes;
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
	
	for(int k=sizeof(size_t); it != chunks.end() ; k += SHA224_DIGEST_LENGTH)
		memcpy(digests+k, (*it)->_digest(), SHA224_DIGEST_LENGTH);
}

void Client::buid_digests_map(vector<Chunk>& chunks, map<string, Chunk*>& map){
	vector<Chunk>::iterator it = chunks.begin();

	for(; it != chunks.end() ; it++)
		map[ string(it->_digest()) ] = &(*it);
}

void Client::group_by_id(vector<Chunk>& chunks, map<uint64_t, list<Chunk*> >& buffers){
	uint64_t tmp_id;
	for(int i=0; i<chunks.size(); i++){
		tmp_id = nodes->rallocate( chunks[i]._digest(), SHA224_DIGEST_LENGTH )->get_id();
		
		if( buffers.find(tmp_id) == buffers.end() )
			buffers[tmp_id]=list<Chunk*>();
		
		buffers[tmp_id].push_back( &chunks[i] );
	}
} 

void Client::group_by_ids(list<Chunk*>& chunks, map<uint64_t, list<Chunk*> >& buffers){
	list<Chunk*>::iterator it = chunks.begin();
	vector<Node*> _nodes;
	uint64_t tmp_id;
	
	for(; it != chunks.end() ; it++){
		_nodes = nodes->wallocate( (*it)->_digest(), SHA224_DIGEST_LENGTH );
		
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
		additions[ string((*it)->c_digest()) ] = true;
	
	m_additions.unlock();
}
void Client::populate_additions( char* digest ){
	char buffer[SHA224_DIGEST_LENGTH + 1];
	buffer[SHA224_DIGEST_LENGTH] = 0;
	
	for(int i=0; i <SHA224_DIGEST_LENGTH; i++)
		sprintf(buffer+i, "%02x", digest[i]);
	
	
	m_additions.lock();
	additions[ string(buffer) ] = true;
	m_additions.unlock();
}

///true : file already exists, else not
bool Client::dedup_by_file(const char* location, char* file_digest){
	if( !hashfile(location, file_digest) ){
		perror("Hashing failed");
		return false;
	}
	Node* prime;
	
	Msg m(EXISTS_OBJECT, file_digest, SHA224_DIGEST_LENGTH);
	prime = nodes->rallocate( file_digest, SHA224_DIGEST_LENGTH);
	send(m, prime->get_host(), prime->get_port());
	
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
	char file_digest[SHA224_DIGEST_LENGTH];
	if( dedup_by_file(location, file_digest) )
		return true;
	
	vector<Chunk> chunks;
	map<string, Chunk*> chunks_map; 
	map<uint64_t, list<Chunk*> > buffers; //node_id => chunks of this node

	
	chunks = cf->split(location);
	buid_digests_map( chunks, chunks_map);
		
	///Send requests
	group_by_id( chunks, buffers);
	
	for(map<uint64_t, list<Chunk*> >::iterator it = buffers.begin() ; 
	it != buffers.end(); it++){
		size_t buffer_len = (it->second).size() * SHA224_DIGEST_LENGTH + sizeof(size_t);
		Msg* m = new Msg(EXISTS_CHUNKS, buffer_len); //handle by send
		
		build_digests( it->second, m->get_data());
		
		Node* node = nodes->get_node( it->first );
		send(m, node->get_host(), node->get_port());
	}
	
	if( !wait_objects( chunks.size() ) )
		return false;
	
	///Select chunks to dedup
	list<Chunk*> to_dedup;
	char* tmp_digest;
	char tmp_exists;
	
	m_objects.lock();
	for(int i=0; i<objects.size(); i++){
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
		if( to_dedup.front()->get_data() != NULL ){
			for(list<Chunk*>::iterator it = to_dedup.begin() ; 
			it != to_dedup.end(); it++){	
				string tmp=(path_dir/fs::path((*it)->c_digest())).string();
				ofstream c_file(tmp.c_str(), ios::binary);
				c_file<< (*it)->get_data();
			}
		}
		else{	
			char *src;
			int fd = open(location, O_RDONLY);
			uint64_t size_file = lseek(fd, 0, SEEK_END);
			src = static_cast<char*>( 
				mmap(NULL, size_file, PROT_READ, MAP_PRIVATE, fd, 0));
			
			
			for(list<Chunk*>::iterator it = to_dedup.begin() ; 
			it != to_dedup.end(); it++){
				string tmp=(path_dir/fs::path((*it)->c_digest())).string();
				ofstream c_file(tmp.c_str(), ios::binary);
				
				c_file.write( src+(*it)->get_begin(), (*it)->get_length() );
				c_file.close();
			}
			
			munmap( src, size_file);
			close( fd );	
		}
		
		///Send new chunk to sd
		map<uint64_t, list<Chunk*> > _buffers;
		group_by_ids( to_dedup, _buffers);
		
		for(map<uint64_t, list<Chunk*> >::iterator it = buffers.begin() ; 
		it != buffers.end(); it++){
			size_t buffer_len = (it->second).size() * SHA224_DIGEST_LENGTH + sizeof(size_t);
			Msg* m = new Msg(ADD_CHUNKS, buffer_len); //handle by send
			
			build_digests( it->second, m->get_data());
			
			Node* node = nodes->get_node( it->first );
			send(m, node->get_host(), node->get_port());
		}
		
		///Send file digest
		vector<Node*> _nodes = nodes->wallocate( file_digest, SHA224_DIGEST_LENGTH );
		for(int k=0 ; k< _nodes.size() ; k++){
			Msg* m = new Msg(ADD_OBJECT, SHA224_DIGEST_LENGTH); //handle by send
			for(int i=0; i <SHA224_DIGEST_LENGTH; i++)
				sprintf(m->get_data()+i, "%02x", file_digest[i]);
		
			send(m, _nodes[k]->get_host(), _nodes[k]->get_port());
		}
		
		
		//
		// I	l faut vérifier que tous les serveurs distants aient répondu
		//
	}
	clear_objects();
	
	
	
	if( !buildMetadata(name, chunks, path_dir) )
		return false;
	
	return true;
}

bool Client::load(const char* name, const char* location, fs::path path_dir){
	vector<Chunk> chunks;
	extractChunks( name, chunks, path_dir);
	ofstream os( location, ios::binary);	
	
	if( !os ){
		perror("Client::load");
		return false;
	}
	
	char buffer[BUFFER_MAX_SIZE];
	uint64_t b_length = 0;
	
	for(uint64_t i = 0 ; i<chunks.size() ; i++){
		string tmp=(path_dir/fs::path(chunks[i].c_digest())).string();
		ifstream is(tmp.c_str(), ios::binary);
		
		is.read( buffer, chunks[i].get_length() );
		os.write( buffer, chunks[i].get_length() );
		
		is.close();
	}
	os.close();
	
	return true;
}
