#include "Chunk.h"

///Chunk
Chunk::Chunk(){}

Chunk::Chunk(int b, int l) : length(l), begin(b){}

Chunk::Chunk(int b, int l, const char* _data, bool cached) : length(l), begin(b){
    update( _data, cached);
}

Chunk::Chunk(char* _s_data){            
    char* end = _s_data + uint64_s;
    begin = strtoull(_s_data, &end, 0);
    
    end += uint64_s;
    length = strtoull(_s_data+uint64_s, &end, 0);

    memcpy(digest_c, _s_data + 2*uint64_s, DIGEST_LENGTH);
    digest_str = digest_to_string( digest_c );

}

void Chunk::update(const char* _data, bool cached){
    if( cached ){
        inner_data = new char[length];
        memcpy(inner_data, _data, length);
    }
    
    SHA224((unsigned char*)_data, length, digest);  
    
    digest_to_char(digest_c, digest);
    digest_str = digest_to_string( digest_c );

}

Chunk::~Chunk(){
    free( inner_data  );
}


uint64_t Chunk::get_length(){ return length; }
uint64_t Chunk::get_begin(){ return begin; }
char* Chunk::get_data(){ return inner_data; }

bool Chunk::operator<( const Chunk& c2) const{
    return memcmp( digest, c2.digest, SHA224_DIGEST_LENGTH) < 0; //c1 < c2
}

void Chunk::_digest(char *tmp){ 
    memcpy(tmp, digest_c, DIGEST_LENGTH);
}

char* Chunk::ptr_digest(){ return digest_c; }

string Chunk::str_digest(){
    return digest_str;
}

size_t Chunk::s_length(){ return DIGEST_LENGTH + 2 * uint64_s; }

void Chunk::serialize(char* buff){          
    sprintf(buff, "%" PRIu64 "", begin);
    sprintf(buff + uint64_s, "%" PRIu64 "", length);
    memcpy(buff+ 2*uint64_s, digest_c, DIGEST_LENGTH);
}

///ChunkFactory
ChunkFactory::ChunkFactory(){
    hf = new KarpRabinHash<uint64>(WINDOW_LENGTH,AVERAGE_LENGTH );
    buffer = new char[BUFFER_MAX_SIZE];
    window = new UltraFastWindow(WINDOW_LENGTH);
}

ChunkFactory::ChunkFactory( const char* location){
    hf = new KarpRabinHash<uint64>(WINDOW_LENGTH,AVERAGE_LENGTH );
    buffer = new char[BUFFER_MAX_SIZE];
    window = new UltraFastWindow(WINDOW_LENGTH);    
    getFromFile(location);
}
    
ChunkFactory::~ChunkFactory(){
    delete hf;
    delete window;
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

// number before stopping
bool ChunkFactory::chunksIndex(vector<Chunk*>& index, size_t number){
    ///Body
    int j=0 ;
    for(; (i<buffer_size || (i=0, buffer_size=update_buffer()) ) && j<number; (i++, current++, size++)){
        if( (size > MIN_LENGTH) && (hf->hashvalue == 0 || size>=MAX_LENGTH) ){ /// p = 1/2^AVERAGE_LENGTH
            index.push_back( new Chunk(begin, size) );
            begin = current;
            size = 0;
            j++;
            if( not shift())
                break;
        }

       ///Next step
       hf->update( window->add(buffer[i]), buffer[i]); 
    }
    
    if( begin != current-1 && j!=number && j!=0)
        index.push_back( new Chunk(begin, size) );

    return j > 0;
}

///File to chunks
bool ChunkFactory::split(const char* location, vector<Chunk*>& chunks, size_t number){
    bool flag = chunksIndex( chunks, number );
    bool cached = size_file < CACHING_THRESHOLD;
    //printf("spliting spliting spliting %d\n", chunks.size());
    for(; i_split<chunks.size(); i_split++){
        chunks[i_split]->update(ptr_src, cached);
        ptr_src += chunks[i_split]->get_length();
    }
    
    return flag;   
}

bool ChunkFactory::next(const char* location, vector<Chunk*>& chunks, size_t size){
    if( !loaded ){
        is.open(location, ios::binary);
        fd = open(location, O_RDONLY);
        
        if( !is || fd == 0)
            perror("ChunkFactory::split");
        
        size_file = size_of_file(fd);
        src = static_cast<char*>(mmap(NULL, size_file, PROT_READ, MAP_PRIVATE, fd, 0));
        ptr_src = src;

        
        
        buffer_size=update_buffer();
        shift();
        
        loaded = true;
    }
    
    if( split(location, chunks, size) )
        return true;
    else{
        is.close();
        munmap( src, size_file);
        close( fd ); 
        loaded=false;
        return false;
    }
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


///// Begin ChunkIterator
//ChunkIterator::ChunkIterator(const char* location, const char* _file) : file(_file){
    //factory = new ChunkFactory( location );
//}

//ChunkIterator::~ChunkIterator(){
    //if( factory != NULL )
        //delete factory;
//}

//void next(vector<Chunk*>&  chunks, size_t size){
    
//}
