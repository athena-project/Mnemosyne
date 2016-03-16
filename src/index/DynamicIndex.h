#ifndef MNEMOSYNE_INDEX_DYNAMICINDEX_H
#define MNEMOSYNE_INDEX_DYNAMICINDEX_H

#include <boost/filesystem.hpp>
#include <unordered_map>
#include <list>
#include <cstddef>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <inttypes.h>

#include "../utility/hash.cpp"

namespace fs = boost::filesystem;
using namespace std;
//~224Ko
#define MAX_DIGESTS 200 //>2
#define uint64_s sizeof(uint64_t)
#define d 10 //strictement supérieur à 1
#define CACHE_SIZE 100 //en block >=1

class Block{
    protected:
        static int alpha_id;
        uint64_t name = 0;
    
        uint64_t size=0; //in digests
        string location;
        char id[DIGEST_LENGTH];
        
        string path; //couteux à conserver...
        
        FILE* file = NULL;
        char* buffer = NULL;
    public:    
        Block(string _path) : name( Block::alpha_id++ ){
            path = _path;
            init();
        }
        Block(string path, char* data, uint64_t _size);
        
        void init();
        
        ~Block();
        
        void clean();
        
        bool is_full(); ///-1 très important
        
        bool is_loaded();
        
        char* get_id();
        
        Block* split();
        
        bool load();
            
        bool store();
        
        int get_pos_s(char* digest);
        int get_pos_i(char* digest, int pos_s);
        
        bool add(char* digest);
        
        bool exists(char* digest);
        
        void print(int step);
};

class LRU{
    protected:
        list< Block*> items; 
        unordered_map<Block*, decltype(items.begin())> items_map;
        
        size_t size = 0;
        size_t offset = 0; //debut du cache, permet d'ameliorer les inserts en o(1)
        size_t max_size = 0;
        
    public:
        LRU( size_t m) : max_size( m ){}
       
        ~LRU();
        void add(Block* item);
};

/**
 * dans un chaque block  : id min digest ie le premier
 * 
 */
class BNode{
    protected:
        bool leaf = true;
        string path;
        char id[DIGEST_LENGTH];//Min des ids de fils
        
       
        
        BNode* children[2*d]; //ordered by min id of each, peut être utilisé directement un tableau de BNode**
        size_t size_c = 0;
        
        Block* blocks[2*d]; // usefull for leaves only
        size_t size_b = 0;
    public:
        BNode(string _path) : path(_path){ 
            leaf = true;
        }
        
        BNode(string _path, BNode* left, BNode* right);
        
        BNode(string _path, BNode** data, uint64_t size);
        
        BNode(string _path, Block** data, uint64_t size);
        
        ~BNode();
       
        bool is_full();
        
        char* get_id();
        
        int get_child_pos_s(char* digest);
        int get_block_pos_s(char* digest);
        
        int get_child_pos_i(char* digest, int pos_s);
        int get_block_pos_i(char* digest, int pos_s);
        
        BNode* split();
        
        bool add_digest(char* digest, LRU* cache);
        
        bool exists_digest(char* digest, LRU* cache);
        
        void print(int step);
};  

class BTree{
    protected:
        LRU* cache = NULL;
        BNode* root = NULL;
        
        string path;
    public:
        BTree(){}
    
        BTree(string _path) : path( _path ){
            root = new BNode(path.c_str());
            cache = new LRU( CACHE_SIZE );
        }
        
        ~BTree();
        
        bool add_digest(char* digest);
        
        bool exists_digest(char* digest);
        
        void print();
};

#endif //MNEMOSYNE_INDEX_DYNAMICINDEX_H
