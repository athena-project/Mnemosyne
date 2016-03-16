#ifndef MNEMOSYNE_INDEX_DYNAMICINDEX_H
#define MNEMOSYNE_INDEX_DYNAMICINDEX_H

#include <list>
#include <cstddef>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <unordered_map>

#include <boost/filesystem.hpp>

#include "../utility/hash.cpp"

namespace fs = boost::filesystem;
using namespace std;



/**
 * Range [2, [
 * Should be between one undred and few thousands to fit the price ratio between
 * RAM and hard-disk
 */
#define MAX_DIGESTS 200

#define uint64_s sizeof(uint64_t)

/**
 * Number of children by node of the BTree will be between [d, 2*d]
 * Condition : d > 1, if(d=2 : we will obtained a binary tree)
 */
#define d 10 //strictement supérieur à 1

/**
 * Number of blocks that can be loaded into cache( in RAM)
 */
#define CACHE_SIZE 100 //en block >=1



class Block{
    /**
     * For complexity, n is the number of digests stored
     * be aware that n is bounded by a small constant
     */
    protected:
        static int alpha_id;
        uint64_t name = 0;
    
        uint64_t size=0; // number of digests stored
        string location; // file location to store data
        char id[DIGEST_LENGTH]; // max of the digest stored
        
        const char* path;
        
        FILE* file = NULL;
        char* buffer = NULL; // where digests are stored in RAM if loaded
    public:    
        Block(const char* _path) : name( Block::alpha_id++ ){
            path = _path;
            init();
        }
        
        Block(const char* path, char* data, uint64_t _size);
        
        /**
         * Generate location
         */
        inline void init();
        
        ~Block();
        
        /**
         * Clean RAM ie free buffer and close file if needed
         */
        void clean();
        
        /**
         * Note that it is designed to be called after an addition : so we use -1
         * @return true if no more digests can be added
         */
        bool is_full();
        
        /**
         * @return true if the buffer contain the data, false otherwith 
         */
        bool is_loaded();
        
        char* get_id();
        
        /**
         * Split the current data in two block, the separator is the median, 
         * be aware that the split is made in place for half of the data.
         * The left part is the current block
         * 
         * @warning the block must be loaded( ensured by caller)
         * @return the right part  
         */
        Block* split();
        
        /**
         * Load the data of the block in RAM ie in buffer
         */
        bool load();
        
        /**
         * Store the  data( in buffer) in the file( see. attribute location)
         */
        bool store();
        
        /**
         * Retrun the potential position of the digest in buffer for a get 
         * Complexity : 
         *      -Worst case O(log n) 
         *      -Average case O(log n)
         * @warning the block must be loaded( ensured by caller)
         */
        int get_pos_s(char* digest);
        
        /**
         * Retrun the potential position of the digest in buffer for an addition
         * Complexity : 
         *      -Worst case O(log n) 
         *      -Average case O(log n)  
         * 
         * @warning the block must be loaded( ensured by caller)
         * @param pos_s - otential position of the digest in buffer for a get 
         */
        int get_pos_i(char* digest, int pos_s);
        
        /**
         * Add digest to buffer
         * Complexity : 
         *      -Worst case O(n) 
         *      -Average case O(n/2) 
         * 
         * @warning the block must be loaded( ensured by caller)
         */
        bool add(char* digest);
        
        /**
         * Add digest to buffer
         * Complexity : 
         *      -Worst case O(log n) 
         *      -Average case O(log n) 
         * 
         * @warning the block must be loaded( ensured by caller)
         */
        bool exists(char* digest);
        
        void print(int step=0);
};

class LRU{
    /**
     * For complexity, n is the number of blocks stored
     */
    protected:
        list< Block*> items; 
        unordered_map<Block*, decltype(items.begin())> items_map;
        
        size_t size = 0;
        size_t max_size = 0;
        
    public:
        LRU( size_t m) : max_size( m ){}
       
        ~LRU();
        
        /**
         * Add item to cache, and loaded( if it is not )
         * Complexity :
         *     -Worst case O(n)
         *     -Amortized case O(1)
         * Disk access :
         *      -Worst case : 2 (store + load) 
         */
        void add(Block* item);
};

class BNode{
    /**
     * For complexity, m is the number of blocks stored or of children
     * so m is in [d, 2*d] and n is the number of digest stored
     */
    protected:
        bool leaf = true;
        const char* path;
        char id[DIGEST_LENGTH]; // max of the digest stored
        
        BNode* children[2*d]; // ordered by their id( ASC), leaves excepted
        size_t size_c = 0;
        
        Block* blocks[2*d]; // ordered by their id( ASC), leaves only
        size_t size_b = 0;
    public:
        BNode(const char* _path) : path(_path){ 
            leaf = true;
        }
        
        BNode(const char* _path, BNode* left, BNode* right);
        
        BNode(const char* _path, BNode** data, uint64_t size);
        
        BNode(const char* _path, Block** data, uint64_t size);
        
        ~BNode();
       
        /**
         * Note that it is designed to be called after an addition : so we use -1
         * @return true if no more children/bocks can be added
         */
        bool is_full();
        
        char* get_id();
        
        /**
         * Retrun the potential position of the digest in children for a get 
         * Complexity : 
         *      -Worst case O(log m) 
         *      -Average case O(log m)
         */
        int get_child_pos_s(char* digest);
        
        /**
         * Retrun the potential position of the digest in blocks for a get 
         * Complexity : 
         *      -Worst case O(log m) 
         *      -Average case O(log m)
         */
        int get_block_pos_s(char* digest);
        
        /**
         * Retrun the potential position of the digest in children for an addition
         * Complexity : 
         *      -Worst case O(log m) 
         *      -Average case O(log m)  
         * 
         * @param pos_s - potential position of the digest in children for a get 
         */
        int get_child_pos_i(char* digest, int pos_s);
        
        /**
         * Retrun the potential position of the digest in blocks for an addition
         * Complexity : 
         *      -Worst case O(log m) 
         *      -Average case O(log m)  
         * 
         * @param pos_s - potential position of the digest in blocks for a get 
         */
        int get_block_pos_i(char* digest, int pos_s);
        
        /**
         * Split the current children/blocks in two nodes, the separator is the 
         * median id, be aware that the split is made in place for half of the data.
         * The left part is the current block
         * 
         * Complexity : 
         *      -Worst case O(d)
         *      -Average case O(d) 
         * @return the right part  
         */
        BNode* split();
        
        /**
         * Add digest to the node
         * Complexity : 
         *      -Worst case O(m + MAX_DIGESTS + log n )
         *      -Average case O(m/2 + MAX_DIGESTS/2 + log n)
         * 
         * Disk access :
         *      -Worst case : 4 (2 store + 2 load) 
         */
        bool add_digest(char* digest, LRU* cache);
        
        /**
         * Check if exists
         * Complexity : 
         *      -Worst case O(log n)
         *      -Average case O(log n)
         * 
         * Disk access :
         *      -Worst case : 2 (store + load) 
         */
        bool exists_digest(char* digest, LRU* cache);
        
        void print(int step=0);
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
        
        /**
         * Add digest to the tree
         * Complexity : 
         *      -Worst case O(m + MAX_DIGESTS + log n )
         *      -Average case O(m/2 + MAX_DIGESTS/2 + log n)
         * 
         * Disk access :
         *      -Worst case : 4 (2 store + 2 load) 
         */
        bool add_digest(char* digest);
        
        /**
         * Check if exists
         * Complexity : 
         *      -Worst case O(log n)
         *      -Average case O(log n)
         * 
         * Disk access :
         *      -Worst case : 2 (store + load) 
         */
        bool exists_digest(char* digest);
        
        void print();
};

#endif //MNEMOSYNE_INDEX_DYNAMICINDEX_H
