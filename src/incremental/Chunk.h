#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED


#include <list>
#include <vector>
#include <fstream>

#include "Manager.h"
#include "Xz.h"

using namespace std;

class ChunkPiece{ ///Data [beginning,end[
	protected:
		uint64_t id = 0;
		uint32_t beginning = 0;
		uint32_t end =0;
	public:
		ChunkPiece( uint64_t i, uint32_t b, uint32_t e) : id(i), beginning(b), end(e){}
		
		uint32_t getBeginning(){ return beginning; }
		uint32_t getEnd(){ return end; }
		
		void setEnd(uint32_t e){ end = e; }
};
class Chunk : public Xz{
	protected :
		uint64_t id = 0;
		uint32_t size =0;
		
		///Working attributs
		fs::path path = "";
		fs::path x_path = "";
		fstream descriptor;
	
	public :
		static const uint32_t CHUNK_SIZE_MAX = 64 * 1024; //Bytes
		static const uint64_t BUFFER_LENGTH = 64 * 1024;


		Chunk(){} 
		Chunk( uint64_t i, uint32_t s) : id(i), size(s){}
		Chunk( uint64_t i, uint32_t s, fs::path dir) : id(i), size(s){ setPath(dir); }
		Chunk(ChunkPiece& p, fs::path dir) : id(p.id){ setPath(dir); }
		
		~Chunk(){
			if( path != "" )
				descriptor.close();
			fs::remove( path );
		}
		
		uint64_t getId(){ return id; }
		uint32_t getSize(){ return size; }
		
		fstream getDescriptor(){ return descriptor; }
		
		void setId( uint64_t i){ id=i; }
		void setSize( uint64_t s){ size=s; }
		
		/**
		 * @brief read the size of path
		 */
		uint64_t readSize();
		
		/**
		 * @param dir           - /dir1/dir2/parent_dir
		 */
		 
		 void setPath( fs::path dir ){
			ostringstream stime;
			stime<<time(NULL);
			path = dir + fs::path("/") + fs.path( id.str() ) + fs::path( stime.str() );
			x_path = dir + fs::path("/") + fs.path( id.str() ) + fs.path( ".xz" );
			if( fs:exists(path) )
				return setPath( dir );
			
			fstream tmpFs( path.str().c_str() ); ///Write the file
			
			if(!tmpFs)
				return setPath( dir );
				
			tmpFs.close();
		 }
		
		/**
		 * @brief xz to tmp file
		 */
		void load();
		void loadInto(const char* location );
		
		
		/**
		 * @brief create the file(compressed)
		 */
		void save();
		
		/**
		 * @brief Write the data into path
		 * @param stream        - data
		 * @param idBeginning   - location in stream
		 * @param size          - size of the data
		 */
		void write(fstream& data, uint64_t idBeginning, uint64_t size); 
		
		
		/**
		 * @brief write the chunk content from idBeginning to idBeginning+size at the end of a stream
		 * @param stream        - output stream
		 * @param idBeginning   - location in chunk content
		 * @param size          - size of the content needed
		 */
		void read(fstream& data, uint64_t idBeginning, uint64_t size);
		void read(fstream& data, ChunkPiece piece);
};

#endif // CHUNK_H_INCLUDED
