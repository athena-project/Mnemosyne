#include "Chunk.h"

///Chunk
Chunk::Chunk(){}

Chunk::Chunk(int b, int l, const char* _data, bool cached) : length(l), begin(b){
	if( cached ){
		data = new char[length];
		memcpy(data, _data, length);
	}
	
	SHA224((unsigned char*)_data, length, digest);	
}

Chunk::Chunk(char* _s_data){			
	char* end = _s_data + uint64_s;
	begin = strtoull(_s_data, &end, 0);
	
	end += uint64_s;
	length = strtoull(_s_data+uint64_s, &end, 0);

	memcpy(digest, _s_data + 2*uint64_s, SHA224_DIGEST_LENGTH);
}

Chunk::~Chunk(){
	if( data != NULL)
		delete[] data;
}


uint64_t Chunk::get_length(){ return length; }
uint64_t Chunk::get_begin(){ return begin; }
char* Chunk::get_data(){ return data; }

bool Chunk::operator<( const Chunk& c2) const{
	return memcmp( digest, c2.digest, SHA224_DIGEST_LENGTH) < 0; //c1 < c2
}

unsigned char* Chunk::get_digest(){ return digest; }

char* Chunk::_digest(){ 
	char buffer[SHA224_DIGEST_LENGTH];
	for(int i=0; i <SHA224_DIGEST_LENGTH; i++){
		sprintf(buffer+i, "%02x", digest[i]);
	}
}

const char* Chunk::c_digest(){ return _digest(); }

size_t Chunk::s_length(){ return SHA224_DIGEST_LENGTH + 2 * uint64_s; }

void Chunk::serialize(char* buff){			
	sprintf(buff, "%" PRIu64 "", begin);
	sprintf(buff + uint64_s, "%" PRIu64 "", length);
	for(int i=0; i <SHA224_DIGEST_LENGTH; i++)
		sprintf(buff + 2*uint64_s+i, "%02x", digest[i]);
}

///ChunkFactory
ChunkFactory::ChunkFactory(){
	hf = new KarpRabinHash<uint64>(WINDOW_LENGTH,AVERAGE_LENGTH );
	buffer = new char[BUFFER_MAX_SIZE];
	window = UltraFastWindow(WINDOW_LENGTH);
}

ChunkFactory::ChunkFactory( const char* location){
	hf = new KarpRabinHash<uint64>(WINDOW_LENGTH,AVERAGE_LENGTH );
	buffer = new char[BUFFER_MAX_SIZE];
	window = UltraFastWindow(WINDOW_LENGTH);	
	getFromFile(location);
}
	
ChunkFactory::~ChunkFactory(){
	if( hf != NULL)
		delete hf;
	if( buffer != NULL)
		delete[] buffer;
	if( is.is_open() )
		is.close();
}	

void ChunkFactory::saveIntoFile(const char* file){
	std::ofstream ofile(file);
	if( !ofile )
		perror("ChunkFactory::saveIntoFile");
	boost::archive::binary_oarchive ar(ofile);

	ar << (*hf);
	ofile.close();
	return;
}

void ChunkFactory::getFromFile(const char* file){
	std::ifstream ifile(file);
	if( !ifile )
		perror("ChunkFactory::getFromFile");
	boost::archive::binary_iarchive ar(ifile);

	ar >> (*hf);
	ifile.close();
}
	
uint64 ChunkFactory::update_buffer(){
	is.read(buffer, BUFFER_MAX_SIZE);
	return is.gcount();
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
		perror("ChunkFactory::split");
		
	bool cached = size_of_file( is ) < CACHING_THRESHOLD;
		
	int fisd=0;
	buffer_size = i = current = size = 0;
	list<uint64> index = chunksIndex();	
	vector<Chunk> chunks(index.size()-1);///nbr de poteaux et d'intervalles
	list<uint64>::iterator it=index.begin();
	uint64_t last = *(it++);

	shift();
	char* pos=buffer;
	char* data = new char[BUFFER_MAX_SIZE];
	char* data_ptr = data;
	uint64_t beg=0;

	for(uint64_t j=0; it!=index.end(); (j++, last=*(it++))){
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
			
		chunks[j] = Chunk(beg, size, data_ptr, cached);
		pos += size;
		beg += size;
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

//int main(){
	///////g++ -g -std=c++11 -g -o3 chunk.cpp -o chunk -lssl -lcrypto  -lboost_serialization
	//ChunkFactory cf("krh.ser");
	////ChunkFactory cf;
	//vector<Chunk> chunks = cf.split("zero.zero");
	//cout<<"num_chunks "<< chunks.size()<<endl; 
	//min_2(chunks);
//}
