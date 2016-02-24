#include "Metadata.h"
const char* locationMetadata(const char *name, fs::path path_dir){
	unsigned char digest_name[ SHA224_DIGEST_LENGTH ];
	char _name[ SHA224_DIGEST_LENGTH ];
	SHA224((unsigned char*)name, strlen(name), digest_name);	
	
	for(int i=0; i <SHA224_DIGEST_LENGTH; i++)
		sprintf(_name+i, "%02x", digest_name[i]);
		
	return (path_dir/fs::path(_name) ).string().c_str();
}

bool buildMetadata(const char *name, vector<Chunk>& chunks, fs::path path_dir){
	size_t s_length = Chunk::s_length();
	char buffer[ BUFFER_LEN * s_length ];
	ofstream meta_file(locationMetadata(name, path_dir), ios::binary);
	
	if( !meta_file ){
		perror("buildMeta");
		return false;
	}
	
	uint64_t offset = 0;
	for(uint64_t i=0; i< chunks.size() ; i++, offset+=s_length){
		if( offset <  BUFFER_LEN * s_length ){
			meta_file.write(buffer, offset);
			offset = 0;
		}
		
		chunks[i].serialize( buffer+offset );
	}
	
	if( offset != 0)
		meta_file.write(buffer, offset);
		
	return true;
}

bool extractChunks(const char *name, vector<Chunk>& chunks, fs::path path_dir){
	size_t s_length = Chunk::s_length();
	char buffer[ BUFFER_LEN * s_length ];
	ifstream meta_file(locationMetadata(name, path_dir), ios::binary);
	
	if( !meta_file ){
		perror("extractChunks");
		return false;
	}
	
	meta_file.read( buffer, BUFFER_LEN * s_length );
	while( meta_file.gcount() == BUFFER_LEN * s_length){
		uint64_t offset = 0;
		for(uint64_t i=0; i< BUFFER_LEN ; i++, offset+=s_length)
			chunks.push_back( Chunk(buffer + offset) );
		
	}

	size_t size = meta_file.gcount() / s_length;
	for(uint64_t i=0, offset = 0; i< size ; i++, offset+=s_length)
		chunks.push_back( Chunk(buffer + offset) );
		
	return true;
}
