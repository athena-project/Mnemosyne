#ifndef MNEMOSYNE_CHUNK_H
#define MNEMOSYNE_CHUNK_H

#include <fstream>
#include <vector>
#include <utility>
#include <exception>
#include <string.h>
#include <stdio.h>


#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <openssl/sha.h>

#include "hash/rabinkarphash.h"
#include "utility.cpp"

#define MIN_LENGTH (1<<10) ///au minimun n-3<n<n+3 pour min<average<max
#define MAX_LENGTH (1<<16)
#define AVERAGE_LENGTH 13 ///in bytes
#define WINDOW_LENGTH 48 ///in bytes exp()>>exp(average_length)
#define BUFFER_MAX_SIZE (1<<24)

///taille max d'un fichier completement conserver en mémoire, 1Mo
#define CACHING_THRESHOLD 1048576

#define uint64_s sizeof(uint64_t)
using namespace std;

class FileError: public exception{
  virtual const char* what() const throw(){
    return "FileError happened";
  }
} FileError;


uint64_t size_of_file(istream& is){
	uint64_t last = is.tellg();
	uint64_t size = 0;
	
	is.seekg (0, is.end);
	size = is.tellg();
	is.seekg(last);
	
	return size;
}

///si le fichier n'est pas trop gros on en stocke dans les chunks, 
///sinon on en stocke qu'une partie..
class Chunk{
	protected:
		uint64_t begin;	
		uint64_t length;
		
		unsigned char digest[SHA224_DIGEST_LENGTH];
		char* data = NULL; //used if file_size<CACHING_THRESHOLD
	public:
		Chunk(){}
		
		Chunk(int b, int l, const char* _data, bool cached=true) : length(l), begin(b){
			if( cached ){
				data = new char[length];
				memcpy(data, _data, length);
			}
			
			SHA224((unsigned char*)_data, length, digest);	
		}
		
		Chunk(char* _s_data){			
			char* end = _s_data + uint64_s;
			begin = strtoull(_s_data, &end, 0);
			
			end += uint64_s;
			length = strtoull(_s_data+uint64_s, &end, 0);

			memcpy(digest, _s_data + 2*uint64_s, SHA224_DIGEST_LENGTH);
		}
			
		~Chunk(){
			if( data != NULL)
				delete[] data;
		}
		
		uint64_t get_length(){ return length; }
		uint64_t get_begin(){ return begin; }
		char* get_data(){ return data; }
		
		bool operator<( const Chunk& c2) const{
			return memcmp( digest, c2.digest, SHA224_DIGEST_LENGTH) < 0; //c1 < c2
		}
		
		unsigned char* get_digest(){ return digest; }
		
		char* _digest(){ 
			char buffer[SHA224_DIGEST_LENGTH];
			for(int i=0; i <SHA224_DIGEST_LENGTH; i++) {
				sprintf(buffer+i, "%02x", digest[i]);
			}
		}
		
		const char* c_digest(){ return _digest(); }
		
		static size_t s_length(){ return SHA224_DIGEST_LENGTH + 2 * uint64_s; }
		
		void serialize(char* buff){			
			sprintf(buff, "%" PRIu64 "", begin);
			sprintf(buff + uint64_s, "%" PRIu64 "", length);
			for(int i=0; i <SHA224_DIGEST_LENGTH; i++)
				sprintf(buff + 2*uint64_s+i, "%02x", digest[i]);
		}
};

class ChunkFactory{
	protected:
		uint32 buffer_size = 0;
		uint32 i =0; ///position buffer

		uint64 current=0;///position dans le fichier
		uint64 size=0; ///taille du chunk courant
		char* buffer = NULL;

		//FastList window;
		UltraFastWindow window;
		ifstream is;

		KarpRabinHash<uint64>* hf = NULL;
	public:
		ChunkFactory(){
			hf = new KarpRabinHash<uint64>(WINDOW_LENGTH,AVERAGE_LENGTH );
			buffer = new char[BUFFER_MAX_SIZE];
			window = UltraFastWindow(WINDOW_LENGTH);
		}
		
		ChunkFactory( const char* location){
			hf = new KarpRabinHash<uint64>(WINDOW_LENGTH,AVERAGE_LENGTH );
			buffer = new char[BUFFER_MAX_SIZE];
			window = UltraFastWindow(WINDOW_LENGTH);	
			getFromFile(location);
		}
			
		~ChunkFactory(){
			if( hf != NULL)
				delete hf;
			if( buffer != NULL)
				delete[] buffer;
			if( is.is_open() )
				is.close();
		}	
	
		void saveIntoFile(const char* file){
			std::ofstream ofile(file);
			if( !ofile )
				throw FileError;
			boost::archive::binary_oarchive ar(ofile);

			ar << (*hf);
			ofile.close();
			return;
		}

		void getFromFile(const char* file){
			std::ifstream ifile(file);
			if( !ifile )
				throw FileError;
			boost::archive::binary_iarchive ar(ifile);

			ar >> (*hf);
			ifile.close();
		}
			
		uint64 update_buffer(){
			is.read(buffer, BUFFER_MAX_SIZE);
			return is.gcount();
		}
		
		bool shift();
		
		list<uint64> chunksIndex();
		
		vector<Chunk> split(const char* location);
		
		void save(const char* location);
};
#endif
