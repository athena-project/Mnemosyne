#include "Chunk.h"

void Chunk::readSize(){
	uint64_t pos = descriptor.tellg();
	uint64_t s=0;
	
	descriptor.seekg(0, descriptor.end);
	s = descriptor.tellg();
	descriptor.seekg(pos, descriptor.beg);
	
	return s;
}

void Chunk::load(){
	descriptor.close();
	decompress( x_path.str().c_str() , path.str().c_str());
	descriptor.open( path.str().c_str() );
}

void Chunk::loadInto(const char* location ){
	decompress( x_path.str().c_str() , fopen( inpath, "ab"););
}


void Chunk::save(){
	descriptor.flush();
    compress( path.str().c_str() , x_path.str().c_str());
}

void Chunk::write(fstream& data, uint64_t idBeginning, uint64_t size){
	char* buffer[BUFFER_LENGTH];
	data.seekg( idBeginning, data.beg );
	

	for(uint64_t i=0; i<(size%BUFFER_LENGTH); i++){
		data.read(buffer, BUFFER_LENGTH);
		descriptor.write( buffer, BUFFER_LENGTH );
	}
	
	int64_t length = size-(size%BUFFER_LENGTH) * BUFFER_LENGTH;
	char* buffer[length];
	
	data.read(buffer, length);
	descriptor.write( buffer, length );
}

void Chunk::read(fstream& data, ChunkPiece piece){
	read( data, piece.getBeginning(), piece.getEnd() );
}

void Chunk::read(fstream& data, uint64_t idBeginning, uint64_t size){
	char* buffer[BUFFER_LENGTH];
	descriptor.seekg( idBeginning, data.beg );
	

	for(uint64_t i=0; i<(size%BUFFER_LENGTH); i++){
		descriptor.read(buffer, BUFFER_LENGTH);
		data.write( buffer, BUFFER_LENGTH );
	}
	
	int64_t length = size-(size%BUFFER_LENGTH) * BUFFER_LENGTH;
	char* buffer[length];
	
	descriptor.read(buffer, length);
	data.write( buffer, length );
}
