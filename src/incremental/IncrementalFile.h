#ifndef INCREMENTALFILE_CPP_INCLUDED
#define INCREMENTALFILE_CPP_INCLUDED

#include <boost/filesystem.hpp>
#include <sstream>
#include <fstream>

#include "Revision.h"
#include "Chunk.h"

using namespace std;
namespace fs = boost::filesystem;

class IncrementalFile{
	protected :
		///Static attributs
		fs::path dir;          ///Location of the incremental file dir : /d1/d2/incr_dir must exist
		uint32_t num_revisions = 0; 
		uint32_t num_chunks	= 0;  

		///Working attributs
		RevisionHandler* handler = NULL;
		
		forward_list<fs::path> fileGarbage;
		state init_state = NO_INIT;
		
		/**
		 * @brief create a new tmpfile, managed by the current instance, no delete needed
		 */
		fs::path tmpfile();
		
		/**
		 * @brief Build the n° revision, using the minimal number of chunks
		 * @param n				- n° rev 
		 */
		WorkingRev lazyLoading(int n);
		
		/**
		 * @brief Build the n° revision
		 * @param n				- n° rev 
		 */
		WorkingRev loading(int n);
		
	public :
		enum state{GLOBAL_INIT, LAZY_INIT, NO_INIT  }
		
		IncrementalFile(const char* d) : dir(d){ handler = new RevisionHandler(); }
		IncrementalFile(fs::path d ){ 
			dir = d.str().c_str();
			handler = new RevisionHandler();
		}
		
		~IncrementalFile(){
			delete handler;
			
			while( !fileGarbage.empty() )
				fs::remove( fileGarbage.pop_front() );
		}

		uint64_t getId(){ return id;}
		uint32_t getNum_revisions(){ return num_revisions; }
		uint32_t getNum_chunks(){ return num_chunks; }
		
		void setDir( fs::path d ){ dir = d; }
		void setNum_revisions( uint32_t n ){ num_revisions = n; }
		void setNum_revisions( const char* n ){ (stringstream(n)) >> num_revisions; }
		void setNum_chunks( uint32_t n ){ num_chunks = n; }
		void setNum_chunks( const char* n ){ (stringstream(n)) >> num_chunks; }
		
		vector<Chunk> getChunks(){ return chunks; }
		string getContent(){ return content; }
		Revision* getRevision(){ return rev; }

		
		
		/**
		 * @brief Extract table and hydrate revisionHandler metadata 
		 */
		void lazyInit();
		
		/** 
		 * @brief Build the whole descriptor and hydrate handler
		 */
		void init();
		 
		/**
		 * @brief Return the content of the n° rev
		 * @param n
		 */
		WorkingRev get(int n);
		
		/**
		 * @brief create new chunks, and update the last one
		 */
		void makeChunks();
		
		/**
		 * @brief 
		 */
		void save();
		
		/**
		 * @brief Add the current  data to  the tree, do not save it.
		 */
		void newRevision(WorkingRev& data);
};

#endif //INCREMENTALFILE_CPP_INCLUDED
