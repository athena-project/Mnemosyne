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

//faudrat ajouter le nombre max de noeud charger en memoire un cache des noeuds

/**
 * Range [3, [
 * Should be between one undred and few thousands to fit the price ratio between
 * RAM and hard-disk
 */
#define MAX_DIGESTS 200

#define uint64_s sizeof(uint64_t)

/**
 * Number of children by node of the BTree will be between [d, 2*d]
 * Condition : d > 2, if(d=2 : we will obtained a binary tree)
 */
#define d 10 //>1

/**
 * Number of blocks that can be loaded into cache( in RAM)
 */
#define CACHE_SIZE 400 //en block >=1

class Block{
    /**
     * For complexity, n is the number of digests stored
     * be aware that n is bounded by a small constant
     */
    protected:
        static uint64_t alpha_id;
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
        
        /**
         * Reconstruct data from file
         */
        Block(const char* _path, uint64_t _name) : name( _name){
            Block::alpha_id = max( Block::alpha_id, _name+1); 
            path = _path;
            init();
            
            /// Recovery part
            load( true );
            clean();
        }
        
        Block(const char* path, const char* data, uint64_t _size);
        
        /**
         * Generate location
         */
        inline void init(){
            char _name[sizeof(uint64_t)+1];
            _name[sizeof(uint64_t)]=0;

            sprintf(_name, "%" PRIu64 "", name);     
            location = (fs::path(path) / fs::path(_name)).string();
            location = (fs::path(path) / fs::path(_name)).string();
        }
        
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
         * @return true if size == MAX_DIGEST/2
         */
        bool is_half();
        
        /**
         * Note that it is designed to be called after a deletion,
         * ie when size == 0
         * @return true if the tree must be localy rebalance
         */
        bool is_empty();
        
        /**
         * Note that it is designed to be called after a deletion,
         * ie when size == MAX_DIGEST / 2 -1
         * @return true if the tree must be localy rebalance
         */
        bool is_underloaded();
        
        /**
         * @return true if the buffer contain the data, false otherwith 
         */
        bool is_loaded();
        
        const char* get_id();
        const char* get_buffer();
        uint64_t get_size();
        
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
         * @param degraded_mod - if we are trying to restore data after a crash
         */
        bool load(bool degraded_mod=false);
        
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
        int get_pos_s(const char* digest);
        
        /**
         * Retrun the potential position of the digest in buffer for an addition
         * Complexity : 
         *      -Worst case O(log n) 
         *      -Average case O(log n)  
         * 
         * @warning the block must be loaded( ensured by caller)
         * @param pos_s - otential position of the digest in buffer for a get 
         */
        int get_pos_i(const char* digest, int pos_s);
        
        
        /**
         * Complexity : 
         *      -Worst case O(1) 
         *      -Average case O(1)
         * @warning the address return is still manager and owned by the block
         * @return the smallest digest of the block
         */
        const char* pick_smallest();
        
        /**
         * Complexity : 
         *      -Worst case O(1) 
         *      -Average case O(1)
         * @warning the address return is still manager and owned by the block
         * @return the greatest digest of the block
         */
        const char* pick_greatest();
        
        /**
         * Add digest to buffer
         * Complexity : 
         *      -Worst case O(n) 
         *      -Average case O(n/2) 
         * 
         * @warning the block must be loaded( ensured by caller)
         */
        bool add(const char* digest);
        
        /**
         * Delete digest from buffer
         * Complexity : 
         *      -Worst case O(n) 
         *      -Average case O(n/2) 
         * 
         * @warning the block must be loaded( ensured by caller)
         */
        bool remove(const char* digest);
        
        /**
         * Add digest to buffer
         * Complexity : 
         *      -Worst case O(log n) 
         *      -Average case O(log n) 
         * 
         * @warning the block must be loaded( ensured by caller)
         */
        bool exists(const char* digest);
        
        
        /**
         * Merge data in the two block in the current node
         * 
         * 
         * @warning the two blocks must be loaded( ensured by caller), this function
         * assume that this < right ( for _.id order )
         */
        void merge(Block* right);
        
        /**
         * Delete the associate file
         * @warning all data will be lost
         */
        void unlink();
        
        void print(int step=0);
};

class LRU{
    /**
     * For complexity, n is the number of blocks stored
     */
    protected:
        list< Block*> items; 
        unordered_map<Block*, list<Block*>::iterator> items_map;
        
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
        /**
         * Delete item from cache
         * Complexity :
         *     -Worst case O(n)
         *     -Amortized case O(1)
         * 
         * Disk access :
         *      -Worst case : 1 (store) 
         */
        void remove(Block* item);

};

class BNode{
    /**
     * For complexity, m is the number of blocks stored or of children
     * so m is in [d, 2*d] and n is the number of digest stored and n' number of blocks
     */
    protected:
        bool leaf = true;
        const char* path;
        char id[DIGEST_LENGTH]; // max of the digest stored
        
        BNode* children[2*d+1]; // ordered by their id( ASC), leaves excepted
        size_t size_c = 0;
        
        Block* blocks[2*d+1]; // ordered by their id( ASC), leaves only
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
         * Lost all its ownership over object, without destroying them
         * to used after a merge
         */
        void clear();
       
        /**
         * Note that it is designed to be called after an addition : so we use -1
         * @return true if no more children/bocks can be added
         */
        bool is_full();
        
        /**
         * @return true if size == d/2
         */
        bool is_half();
        
        /**
         * Note that it is designed to be called after a deletion, 
         * ie when size_b(or_c) == d-1
         * 
         * @return true if the tree must be localy rebalance
         */
        bool is_empty();
        
        /**
         * @return true if it's a leaf 
         */
        bool is_leaf();
        
        const char* get_id();
        size_t get_size_b();
        size_t get_size_c();
        Block** get_blocks();
        BNode** get_children();
        /**
         * Retrun the potential position of the digest in children for a get 
         * Complexity : 
         *      -Worst case O(log m) 
         *      -Average case O(log m)
         */
        int get_child_pos_s(const char* digest);
        
        /**
         * Retrun the potential position of the digest in blocks for a get 
         * Complexity : 
         *      -Worst case O(log m) 
         *      -Average case O(log m)
         */
        int get_block_pos_s(const char* digest);
        
        /**
         * Retrun the potential position of the digest in children for an addition
         * Complexity : 
         *      -Worst case O(log m) 
         *      -Average case O(log m)  
         * 
         * @param pos_s - potential position of the digest in children for a get 
         */
        int get_child_pos_i(const char* digest, int pos_s);
        
        /**
         * Retrun the potential position of the digest in blocks for an addition
         * Complexity : 
         *      -Worst case O(log m) 
         *      -Average case O(log m)  
         * 
         * @param pos_s - potential position of the digest in blocks for a get 
         */
        int get_block_pos_i(const char* digest, int pos_s);
        
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
        bool add_digest(const char* digest, LRU* cache);
        
        
        /**
         * Add block to the tree, used during the recovery phase
         * Complexity : 
         *      -Worst case O(m + MAX_DIGESTS + log n )
         *      -Average case O(m/2 + MAX_DIGESTS/2 + log n)
         * 
         * @param digest    - id of the block   
         */
        bool add_block(Block* block, const char* digest);
        
        /**
         * Delete block from the node 
         * Complexity :
         *      -Worst case O(m)
         *      -Average case O(m/2)
         * 
         * Disk access :
         *      -Worst case : 
         * 
         */
        void remove_block(int pos, LRU* cache);
        
        /**
         * Delete child from the node 
         * Complexity :
         *      -Worst case O(m)
         *      -Average case O(m/2)
         */
        void remove_child(int pos);
        
        /**
         * Complexity :
         *      -Worst case O(m)
         *      -Best case O(m)
         * @warning Block* now owned by caller
         * assume that size_b>d
         * @return the smallest block (for _.id order)
         */
        Block* lost_smallest_block();
        
        /**
         * Complexity :
         *      -Worst case O(1)
         *      -Best case O(1)
         * @warning Block* now owned by caller
         * assume that size_b>d
         * @return the greatest block (for _.id order)
         */
        Block* lost_greatest_block();
        
        /**
         * Complexity :
         *      -Worst case O(m)
         *      -Best case O(m)
         * @warning BNode* now owned by caller
         * assume that size_c>d
         * @return the smallest child (for _.id order)
         */
        BNode* lost_smallest_child();
        
        /**
         * Complexity :
         *      -Worst case O(1)
         *      -Best case O(1)
         * @warning BNode* now owned by caller
         * assume that size_c>d
         * @return the greatest child (for _.id order)
         */
        BNode* lost_greatest_child();
        
        /**
         * Become owner of the block arg, and this block is the greatest of those
         * owned by the current node
         * Complexity :
         *      -Worst case O(1)
         *      -Best case O(1)
         * 
         * @warning assume that size_b<2d-1
         */
        void win_block(Block* right);
        
        /**
         * Become owner of the node arg, and this node is the greatest of those
         * owned by the current node
         * Complexity :
         *      -Worst case O(1)
         *      -Best case O(1)
         * 
         * @warning assume that size_c<2d-1
         */
        void win_child(BNode* right);
        
        /**
         * Try to rebalance all the children
         * Complexity :
         *      -Worst case O(n' log(n'))
         *      -Best case O(m)
         */
        void rebalance();
        
        /**
         * Try to rebalance the child node current_c at pos_c
         * Complexity :
         *      -Worst case O(n'/m log(n'/m))
         *      -Best case O(1)
         * 
         */
        void rebalance(BNode* current_c, size_t pos_c);
        
        /**
         * Delete digest from the node 
         * Complexity :
         *      -Worst case O()
         *      -Average case O()
         * 
         * Disk access :
         *      -Worst case : 
         * 
         */
        bool remove_digest(const char* digest, LRU* cache);
         
        /**
         * Check if exists
         * Complexity : 
         *      -Worst case O(log n)
         *      -Average case O(log n)
         * 
         * Disk access :
         *      -Worst case : 2 (store + load) 
         */
        bool exists_digest(const char* digest, LRU* cache);
        
        /**
         * Merge data in the two node in the current node
         * 
         * @warning this function assume that this < right ( for _.id order ), 
         * moreover the two nodes must be siblings
         */
        void merge(BNode* right);
        
        /**
         * Partition the node in two sets
         * @param ratio - proportion of children/blocks to move in the new node
         * 
         * Complexity : 
         *      -Worst case O(n' log n')
         * 
         * @warning return object owned by caller
         */
        BNode* split_in_2(float ratio = 0.5);
        
        void print(int step=0);
};  

class BTree{
    protected:
        LRU* cache = NULL;
        BNode* root = NULL;
        
        string path;
        fs::path fs_path;
    public:
        BTree(){}
    
        BTree(string _path, BNode* _root=NULL) : path( _path ){
            fs_path = fs::path( path);  
            
            if( !fs::exists(fs_path) || !fs::is_directory(fs_path))
                throw "Can not create BTree : invalid path ";
                
            root = ( _root == NULL ) ? new BNode(path.c_str()) : _root;
            
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
        bool add_digest(const char* digest);
        
        /**
         * Check if exists
         * Complexity : 
         *      -Worst case O(log n)
         *      -Average case O(log n)
         * 
         * Disk access :
         *      -Worst case : 2 (store + load) 
         */
        bool exists_digest(const char* digest);
        
        
        bool remove_digest(const char* digest);
        
        
        /**
         * Partition the tree in two substrees
         * @param ratio - proportion of children/blocks to move in the new tree
         * 
         * Complexity : 
         *      -Worst case O(n' log n')
         * 
         * @warning BTree* returned now owned by caller
         */
        BTree* split_in_2(float ratio);
        
        
        /**
         * Try recovering from data store on disk
         */
        bool recover();
        
        void print();
};

#endif //MNEMOSYNE_INDEX_DYNAMICINDEX_H
