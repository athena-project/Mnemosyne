#ifndef MNEMOSYNE_DMS_DMS_H
#define MNEMOSYNE_DMS_DMS_H
//dedup map server

#include <map>
#include <string>
#include <mutex>
#include <openssl/sha.h>

#include "../network/TCPHandler.h"
#include "../utility/hash.cpp"
#include "../hrw.cpp"

using namespace std;


class TCPMapServer : public TCPServer{
	protected:
		map<string, bool>* chunks;
		mutex* m_chunks;
		map<string, bool>* objects;
		mutex* m_objects;
	
	public: 
		TCPMapServer(const char* port, int _pfd, std::atomic<bool>* _alive, 
		list<Task>* _tasks, mutex* _m_tasks, 
		map<string, bool>* _objects, mutex* _m_objects,
		map<string, bool>* _chunks, mutex* _m_chunks) : TCPServer(port, _pfd, _alive, _tasks, _m_tasks){
			objects = _objects;
			m_objects = _m_objects;
			chunks = _chunks;
			m_chunks = _m_chunks;
		}
		
		//called when finished to read
		void rcallback(Handler* handler, msg_t type);
		
		//called when finished to write
		void wcallback(Handler* handler, msg_t type);
};

class MapServer : TCPHandler{
	protected:
		NodeMap* nodes = NULL; //3-replication

		map<string, bool> chunks;
		mutex m_chunks;
		map<string, bool> objects;
		mutex m_objects;
	
	public: 
		MapServer(const char* port, NodeMap* _nodes);
		~MapServer();
		
		void run();
};

#endif //MNEMOSYNE_DMS_DMS_H
