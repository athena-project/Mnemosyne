#ifndef MNEMOSYNE_CLIENT_H
#define MNEMOSYNE_CLIENT_H

#include <openssl/sha.h>

#include "../network/TCPHandler.h"
#include "../hrw.cpp"
#include "../Chunk.h"
#include "../utility/hash.cpp"

#include <unordered_map>
#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <algorithm>

#include <sys/mman.h>
#include "../index/BinIndex.h"
#include "../Metadata.h"
#include "../utility/bench.cpp"


#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
using namespace std;

#define BUNDLE_MAX_SIZE 1000
#define FILE_BUFFER 65536
//nano seconde, 0.001s
#define DELAY 1000000 
//nano seconde, 10s
#define TIME_OUT 10000000000
#define BUFFER_MAX_SIZE (1<<24)


#define BIN_W 2 //see extreme binning
#define BIN_R 2

class TCPClientServer : public TCPServer{
    protected:
        list< pair<string, char> > *objects;
        mutex *m_objects;
        
        unordered_map<string,bool> *additions;
        mutex *m_additions;
    
    public: 
        TCPClientServer(const char* port, int _pfd, std::atomic<bool>* _alive, 
        list<Task*>* _tasks, mutex* _m_tasks, 
        list< pair<string, char> >* _objects, mutex* _m_objects,
        unordered_map<string, bool>* _additions, mutex* _m_additions) : TCPServer(port, _pfd, _alive, _tasks, _m_tasks){
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
            
        list< pair<string, char> > objects; //sert pour savoir s'il existe ou non
        mutex m_objects;    
                
        unordered_map<string,bool> additions;
        mutex m_additions;
            
    public:
        Client(const char* port, NodeMap* _nodes);
        ~Client();
        
        void clear_objects();
        
        /**
         * Local dedup du ficier
         */
        void dedup_chunks(vector<Chunk*>& chunks, vector<Chunk*>& local_chunks, 
        unordered_map<string, bool>& mem_chunks, size_t begin);
        
        ///agregate digest in : sizedigest1digest2....
        void build_digests(list<Chunk*>& chunks, char* digests);
        
        void buid_digests_unordered_map(vector<Chunk*>& chunks, unordered_map<string, Chunk*>& unordered_map);
        void group_by_id(vector<Chunk*>& chunks, unordered_map<uint64_t, list< list<Chunk*> > >& buffers, size_t begin);
        void group_by_id(list<Chunk*>& chunks, unordered_map<uint64_t, list< list<Chunk*> > >& buffers);
        ///n act as a condition
        bool wait_objects(int n);
        bool wait_additions();
        
        void populate_additions( list<Chunk*>& chunks );
        void populate_additions( char* digest );
        
        ///true : file already exists, else not
        bool dedup_by_file(const char* location, char* file_digest);
        
        bool save(const char* name, const char* location, fs::path path_dir);
        
        ///use bin 
        bool bsave(const char* name, const char* location, fs::path path_dir);
        
        bool save_bench(const char* name, const char* location, fs::path path_dir){
            Timer t;
            bool flag = save(name, location, path_dir);
            double delay = t.elapsed();
            printf("\nSave operation : \n \
                       time : %lf\n  \
                       size : %" PRIu64 "\n", delay, size_of_file(location));
            
            return flag;
        }
        
        
        ///to load file
        
        bool load(const char* name, const char* location, fs::path path_dir);
        bool load_bench(const char* name, const char* location, fs::path path_dir){
            Timer t;
            bool flag = load(name, location, path_dir);
            double delay = t.elapsed();
            printf("\nSave operation : \n \
                       time : %lf\n  \
                       size : %" PRIu64 "\n", delay, size_of_file(location));
            return flag;
        }
};
#endif
