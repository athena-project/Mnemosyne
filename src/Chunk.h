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

using namespace std;

class FileError: public exception{
  virtual const char* what() const throw(){
    return "FileError happened";
  }
} FileError;


///si le fichier n'est pas trop gros on en stocke dans les chunks, 
///sinon on en stocke qu'une partie..
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
			window = UltraFastWindow(WINDOW_LENGTH);
		}
		
		ChunkFactory( const char* location){
			hf = new KarpRabinHash<uint64>(WINDOW_LENGTH,AVERAGE_LENGTH );
			buffer = new char[BUFFER_MAX_SIZE];
			window = UltraFastWindow(WINDOW_LENGTH);	
			getFromFile(location);
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
		
		bool shift();
		
		list<uint64> chunksIndex();
		
		vector<Chunk> split(const char* location);
		
		void save(const char* location);
};
#endif
