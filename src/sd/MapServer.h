#ifndef MNEMOSYNE_DMS_DMS_H
#define MNEMOSYNE_DMS_DMS_H
//dedup unordered_map server

#include <unordered_map>
#include <string>
#include <mutex>
#include <openssl/sha.h>

#include "../network/TCPHandler.h"
#include "../utility/hash.cpp"
#include "../hrw.cpp"
#include "../index/BinIndex.h"

using namespace std;


class TCPMapServer : public TCPServer{
    protected:
        //unordered_map<string, bool>* chunks;
        BTree* chunks;
        mutex* m_chunks;
        
        BinTree* bins;
        mutex* m_bins;
        
        unordered_map<string, bool>* objects;
        mutex* m_objects;
    
    public: 
        TCPMapServer(const char* port, int _pfd, std::atomic<bool>* _alive, 
        list<Task*>* _tasks, mutex* _m_tasks, 
        unordered_map<string, bool>* _objects, mutex* _m_objects,
        BTree* _chunks, mutex* _m_chunks, BinTree* _bins, mutex* _m_bins) : TCPServer(port, _pfd, _alive, _tasks, _m_tasks){
            objects = _objects;
            m_objects = _m_objects;
            chunks = _chunks;
            m_chunks = _m_chunks;
            bins = _bins;
            m_bins = _m_bins;
        }
        
        //called when finished to read
        void rcallback(Handler* handler, msg_t type);
        
        //called when finished to write
        void wcallback(Handler* handler, msg_t type);
};

class MapServer : TCPHandler{
    protected:
        NodeMap* nodes = NULL; //3-replication

        BTree* chunks;
        mutex m_chunks;
        
        BinTree* bins;
        mutex m_bins;
        
        unordered_map<string, bool> objects;
        mutex m_objects;
    
    public: 
        MapServer(const char* port, NodeMap* _nodes, const char* path);
        ~MapServer();
        
        void run();
};

#endif //MNEMOSYNE_DMS_DMS_H
