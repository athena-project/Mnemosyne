//#include <list>
#include <fstream>
#include <vector>
#include <utility>
#include <exception>
#include <string.h>
#include <stdio.h>


#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <openssl/sha.h>

#include "rabinkarphash.h"



#define MIN_LENGTH (1<<10) ///au minimun n-3<n<n+3 pour min<average<max
#define MAX_LENGTH (1<<16)
#define AVERAGE_LENGTH 13 ///in bytes
#define WINDOW_LENGTH 48 ///in bytes exp()>>exp(average_length)
#define BUFFER_MAX_SIZE (1<<24)

using namespace std;

class FileError: public exception{
  virtual const char* what() const throw(){
    return "FileError happened";
  }
} FileError;

///Utilitaires
//class FastListNode{
	//public:
		//FastListNode* next=NULL;
		//FastListNode* previous=NULL;
		//char data;
		
		//FastListNode(char c) : data(c){}
		
		//~FastListNode(){
			//if( next != NULL)
				//delete next;
		//}	
//};

class UltraFastWindow{
	protected:
		int beg; ///indice de dbut
		char last;
		char* window=NULL;
		size_t size=0;
		
	public:
		UltraFastWindow(){
			size = WINDOW_LENGTH;
			window = new char[WINDOW_LENGTH];
			last = *window;
			beg=0;
		}
		
		~UltraFastWindow(){
			delete[] window;
		}
		
		char add(char c){
			last = *(window +  beg);
			beg = (beg+1) % size;
			*(window + beg )=c;
			
			return last;
		}
		
		void set(char* data){
			memcpy(window, data, size);
			beg = 0;
			last = *window;
		}	
};

//class FastList{
	//protected:
		//FastListNode* first=NULL;
		//FastListNode* last=NULL;
	
	//public:
		//FastList(){}
		
		//~FastList(){
			//if( first != NULL )
				//delete first;
		//}
		
		//void clear(){
			//delete first;
			//first = last = NULL;
		//}
		
		//char front(){
			/////if list empty -> exception
			//return first->data;
		//}
		
		//void pop_front(){
			/////if list empty -> exception
			//char c = first->data;
			
			//FastListNode* tmp = first->next;
			//if( tmp !=NULL){
				//first->next->previous = NULL;
				//first->next = NULL;
				//delete first;
				//first = tmp;
			//}else{
				//first = last = NULL;
			//}
		//}
		
		//void push_back(char c){
			//if( last != NULL ){
				//last->next = new FastListNode(c);
				//last->next->previous = last;
				//last = last->next;
			//}else{
				//first = last = new FastListNode(c);
			//}
		//}
//};

template <typename hashvaluetype>
void saveIntoFile(KarpRabinHash<hashvaluetype>* krh, const char* file){
    std::ofstream ofile(file);
    if( !ofile )
		throw FileError;
    boost::archive::binary_oarchive ar(ofile);

    ar << (*krh);
    ofile.close();
    return;
}

template <typename hashvaluetype>
void getFromFile(KarpRabinHash<hashvaluetype>* krh, const char* file){
    std::ifstream ifile(file);
    if( !ifile )
		throw FileError;
    boost::archive::binary_iarchive ar(ifile);

    ar >> (*krh);
    ifile.close();
}

void print_sha_sum(const unsigned char* md) {
    for(int i=0; i <SHA224_DIGEST_LENGTH; i++) {
        printf("%02x",md[i]);
    }
    printf("\n");
}

///SubChunk
class Chunk{
	protected:
		unsigned char digest[SHA224_DIGEST_LENGTH];
		int length;
	
	public:
		Chunk(){}
		
		Chunk(int l, const char* data) : length(l){
			SHA224((unsigned char*)data, length, digest);	
		}
		
		bool operator<( const Chunk& c2) const{
			return memcmp( digest, c2.digest, SHA224_DIGEST_LENGTH) < 0; //c1 < c2
		}
		
		const unsigned char* get_digest(){ return digest; }
};

class ChunkFactory{
	protected:
		uint32 buffer_size = 0;
		uint32 i =0; ///position buffer

		uint64 current=0;///position dans le fichier
		uint64 size=0; ///taille du chunk courant
		char* buffer;

		//FastList window;
		UltraFastWindow window;
		ifstream is;

		KarpRabinHash<uint64>* hf = NULL;
	public:
		ChunkFactory(){
			hf = new KarpRabinHash<uint64>(WINDOW_LENGTH,AVERAGE_LENGTH );
			buffer = new char[BUFFER_MAX_SIZE];
		}
		
		ChunkFactory( const char* location){
			hf = new KarpRabinHash<uint64>(WINDOW_LENGTH,AVERAGE_LENGTH );
			buffer = new char[BUFFER_MAX_SIZE];
			getFromFile(hf, location);
		}
			
		~ChunkFactory(){
			if( hf != NULL)
				delete hf;
			if( buffer != NULL)
				delete[] buffer;
			if( is.is_open() )
				is.close();
		}
		
		uint64 update_buffer(){
			is.read(buffer, BUFFER_MAX_SIZE);
			return is.gcount();
		}
		
		bool shift(){
			if( not (i+WINDOW_LENGTH < buffer_size || (i=0, buffer_size=update_buffer())))
				return false;
				
			//window.clear();
			char tmp[WINDOW_LENGTH];
			memcpy(tmp, buffer+i, WINDOW_LENGTH);
			
			for(int k=0; k<WINDOW_LENGTH; (k++, i++, current++, size++)){
				hf->eat( buffer[i] );
				//window.push_back( buffer[i] );
			}	
			
			return true;
		}
		
		list<uint64> chunksIndex(){
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
			   //hf->update( window.front(), buffer[i]); 
			   //window.pop_front();
			   //window.push_back( buffer[i] );
			}
			
			if( index.back() != current-1)
				index.push_back( current );
			return index;
		}
		
		///File to chunks
		vector<Chunk> split(const char* location){
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
				//print_sha_sum( chunks[j].get_digest());
				pos += size;
			}
			
			buffer_size = i = current = size=0;
			//window.clear();
			delete[] data;
			is.close();
			return chunks;
		}
		
		void save(const char* location){
			saveIntoFile(hf, location);
		}
};

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
