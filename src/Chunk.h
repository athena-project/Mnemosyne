#ifndef MNEMOSYNE_CHUNK_H
#define MNEMOSYNE_CHUNK_H

#include <fstream>
#include <vector>
#include <utility>
#include <exception>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <openssl/sha.h>

#include "hash/rabinkarphash.h"
#include "utility/fifo.cpp"
#include "utility/filesystem.cpp"
#include "utility/hash.cpp"

#define MIN_LENGTH (1<<10) ///au minimun n-3<n<n+3 pour min<average<max
#define MAX_LENGTH (1<<16)
#define AVERAGE_LENGTH 13 ///in bytes
#define WINDOW_LENGTH 48 ///in bytes exp()>>exp(average_length)
#define BUFFER_MAX_SIZE (1<<24)

///taille max d'un fichier completement conserver en mémoire, 1Mo
#define CACHING_THRESHOLD (1<<24)

#define uint64_s sizeof(uint64_t)
using namespace std;

///si le fichier n'est pas trop gros on en stocke dans les chunks, 
///sinon on en stocke qu'une partie..
class Chunk{
    protected:
        uint64_t begin; 
        uint64_t length;
        
        unsigned char digest[SHA224_DIGEST_LENGTH];
        char digest_c[DIGEST_LENGTH];       
        std::string digest_str;
        
        char* inner_data = NULL; //used if file_size<CACHING_THRESHOLD
        
    public:
        Chunk();
        Chunk(int b, int l);
        Chunk(int b, int l, const char* _data, bool cached=true);
        Chunk(char* _s_data);
            
        ~Chunk();
        
        void update(const char* _data, bool cached=true);
        
        uint64_t get_length();
        uint64_t get_begin();
        char* get_data();
        
        bool operator<( const Chunk& c2) const;
        
        unsigned char* get_digest();
        
        void _digest(char* tmp);
        
        string str_digest();
        char* ptr_digest();
        
        static size_t s_length();
        
        void serialize(char* buff);
};

class ChunkFactory{ //one chunk factory for one file
    protected:
        uint32 buffer_size = 0;
        uint32 i =0; ///position buffer

        uint64 begin=0; /// pos of the begnning of the current chunk
        uint64 current=0;///position dans le fichier
        uint64 size=0; ///taille du chunk courant
        char* buffer = NULL;

        //FastList window;
        UltraFastWindow *window = NULL;
        
        ifstream is;
        int fd = 0;
        char* src;
        char* ptr_src;
        uint64_t size_file;
        size_t i_split = 0;
        
        bool loaded = false; /// if file opened

        KarpRabinHash<uint64>* hf = NULL;
    public:
        ChunkFactory();
        
        ChunkFactory( const char* location);
            
        ~ChunkFactory();
    
        void saveIntoFile(const char* file);

        void getFromFile(const char* file);
            
        uint64 update_buffer();
        
        bool shift();
        
        bool chunksIndex(vector<Chunk*>& index, size_t number);
        
        bool split(const char* location, vector<Chunk*>& chunks, size_t number);
        
        bool next(const char* location, vector<Chunk*>& chunks, size_t size);
        
        void save(const char* location);
};

class ChunkIterator{
    protected:
        ChunkFactory* factory = NULL;
        const char* file =""; // file to dedup
    public:
        ChunkIterator(const char* location, const char* file);
        ~ChunkIterator();
        
        void next(vector<Chunk*>&  chunks, size_t size);
};
#endif
