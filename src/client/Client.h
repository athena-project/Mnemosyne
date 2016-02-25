#ifndef MNEMOSYNE_CLIENT_H
#define MNEMOSYNE_CLIENT_H

#include <openssl/sha.h>

#include "../network/TCPHandler.h"
#include "../hrw.cpp"
#include "../Chunk.h"
#include "../utility/hash.cpp"

#include <map>
#include <string>
#include <fstream>
#include <list>

#include <sys/mman.h>
#include "../Metadata.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
using namespace std;

#define FILE_BUFFER 65536
//nano seconde, 0.001s
#define DELAY 1000000 
//nano seconde, 10s
#define TIME_OUT 10000000000


class TCPClientServer : public TCPServer{
	protected:
		list< pair<char*, char> > *objects;
		mutex *m_objects;
		
		map<string,bool> *additions;
		mutex *m_additions;
	
	public: 
		TCPClientServer(char* port, int _pfd, std::atomic<bool>* _alive, 
		list<Task>* _tasks, mutex* _m_tasks, 
		list< pair<char*, char> >* _objects, mutex* _m_objects,
		map<string, bool>* _additions, mutex* _m_additions){
			TCPServer(port, _pfd, _alive, _tasks, _m_tasks);
			objects = _objects;
			m_objects = _m_objects;
			additions = _additions;
			m_additions = _m_additions;
		}
		
		//called when finished to read
		void rcallback(Handler* handler, msg_t type);
		
		//called when finished to write
		void wcallback(Handler* handler, msg_t type);
};

class Client : public TCPHandler{ 
	protected:
		NodeMap* nodes = NULL; //3-replication
		ChunkFactory* cf = NULL;
			
		list< pair<char*, char> > objects; //sert pour savoir s'il existe ou non
		mutex m_objects;	
				
		map<string,bool> additions;
		mutex m_additions;
			
	public:
		Client(char* port, char* _nodes);
		~Client();
		
		void clear_objects();
		
		///agregate digest in : sizedigest1digest2....
		void build_digests(list<Chunk*>& chunks, char* digests);
		
		void buid_digests_map(vector<Chunk>& chunks, map<string, Chunk*>& map);
		void group_by_id(vector<Chunk>& chunks, map<uint64_t, list<Chunk*> >& buffers);
		void group_by_ids(list<Chunk*>& chunks, map<uint64_t, list<Chunk*> >& buffers);
		///n act as a condition
		bool wait_objects(int n);
		bool wait_additions();
		
		void populate_additions( list<Chunk*>& chunks );
		void populate_additions( char* digest );
		
		///true : file already exists, else not
		bool dedup_by_file(const char* location, char* file_digest);
		bool save(const char* name, const char* location, fs::path path_dir);
		
		
		///to load file
		
		bool load(const char* name, const char* location, fs::path path_dir);
};
#endif
