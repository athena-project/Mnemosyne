#include "Chunk.h"

void print_sha_sum(const unsigned char* md) {
    for(int i=0; i <SHA224_DIGEST_LENGTH; i++) {
        printf("%02x",md[i]);
    }
    printf("\n");
}

		
bool ChunkFactory::shift(){
	if( not (i+WINDOW_LENGTH < buffer_size || (i=0, buffer_size=update_buffer())))
		return false;
		
	char tmp[WINDOW_LENGTH];
	memcpy(tmp, buffer+i, WINDOW_LENGTH);
	
	for(int k=0; k<WINDOW_LENGTH; (k++, i++, current++, size++))
		hf->eat( buffer[i] );	
	
	return true;
}

list<uint64> ChunkFactory::chunksIndex(){
	list<uint64> index;
	buffer_size=update_buffer();
	shift();
	
	index.push_back( 0 );
	
	///Body
	for( ; i<buffer_size || (i=0, buffer_size=update_buffer()) ; (i++, current++, size++)){
		if( (size > MIN_LENGTH) && (hf->hashvalue == 0 || size>=MAX_LENGTH) ){ /// p = 1/2^AVERAGE_LENGTH
			index.push_back( current );
			size = 0;
			if( not shift())
				break;
		}

	   ///Next step
	   hf->update( window.add(buffer[i]), buffer[i]); 
	}
	
	if( index.back() != current-1)
		index.push_back( current );
		
	return index;
}

///File to chunks
vector<Chunk> ChunkFactory::split(const char* location){
	is.open(location, ios::binary);
	if( !is )
		throw FileError;
		
	int fisd=0;
	buffer_size = i = current = size = 0;
	list<uint64> index = chunksIndex();	
	vector<Chunk> chunks(index.size()-1);///nbr de poteaux et d'intervalles
	list<uint64>::iterator it=index.begin();
	uint64 last = *(it++);

	shift();
	char* pos=buffer;
	char* data = new char[BUFFER_MAX_SIZE];
	char* data_ptr = data;

	for(int j=0; it!=index.end(); (j++, last=*(it++))){
		size = (*it)-last;
		i += size;
		if( i >= BUFFER_MAX_SIZE ){
			strcpy(data, pos);
			int length = (i - BUFFER_MAX_SIZE);
			shift();
			pos=buffer;
			
			strcpy(data + length, pos);
			data_ptr = data;
		}else 
			data_ptr = pos;
			
		chunks[j] = Chunk(size, data_ptr);
		pos += size;
	}
	
	buffer_size = i = current = size=0;
	delete[] data;
	is.close();
	return chunks;
}

void ChunkFactory::save(const char* location){
	saveIntoFile(location);
}

pair<Chunk, Chunk> min_2(vector<Chunk>& chunks){
	if( chunks.size() < 2)
		throw "chunks size error";
	
	Chunk& min1 = chunks[0];
	Chunk& min2 = chunks[1];
	
	for(vector<Chunk>::iterator it=chunks.begin(); it!=chunks.end();it++){
		if( (*it) < min1 ){
			if( min1 < min2 )
				min2 = min1;
			min1 = *it;	
		}else if( (*it) < min2 ) ///here, min1 <= *it 
			min2 = *it;	
	}
	
	return pair<Chunk, Chunk>( min1, min2 );
}

int main(){
	/////g++ -g -std=c++11 -g -o3 chunk.cpp -o chunk -lssl -lcrypto  -lboost_serialization
	ChunkFactory cf("krh.ser");
	//ChunkFactory cf;
	vector<Chunk> chunks = cf.split("zero.zero");
	cout<<"num_chunks "<< chunks.size()<<endl; 
	min_2(chunks);
}
