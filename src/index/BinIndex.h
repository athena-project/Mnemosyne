#ifndef MNEMOSYNE_INDEX_BININDEX_H
#define MNEMOSYNE_INDEX_BININDEX_H

#include <list>
#include <cstddef>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <unordered_map>
#include <math.h>

#include <boost/filesystem.hpp>

#include "../utility/hash.cpp"
#include "../utility/filesystem.cpp"
#include "../Chunk.h"
#include "DynamicIndex.h"

/**
 * Bin transfert protocol :
 * id_bin|nbr_chunks|chunks
 */

class BinBlock : public Block{
    protected:
    public:    
        BinBlock(const char* _path) : Block(_path){
        }
        
        /**
         * Reconstruct data from file
         */
        BinBlock(const char* _path, uint64_t _name) : Block(_path, _name){
        }
        
        BinBlock(const char* path, const char* data, uint64_t _size);
        
        bool load(bool degraded_mod);
        
        void merge(BinBlock* right);
};

class BinNode : public BNode{
    /**
     * For complexity, m is the number of blocks stored or of children
     * so m is in [d, 2*d] and n is the number of digest stored and n' number of blocks
     */
    protected:
        bool leaf = true;
        
        const char* path;
        char id[DIGEST_LENGTH]; // max of the digest stored
        
        BinNode* children[2*d+1]; // ordered by their id( ASC), leaves excepted
        size_t size_c = 0;
        
        BinBlock* blocks[2*d+1]; // ordered by their id( ASC), leaves only
        size_t size_b = 0;

    public:
        BinNode(const char* _path) : BNode(_path){ 
            leaf = true;
        }
        
        BinNode(const char* _path, BNode* left, BNode* right) : BinNode(_path, left, right){}
        
        BinNode(const char* _path, BNode** data, uint64_t size) : BinNode(_path, data, size){}
        
        BinNode(const char* _path, BinBlock** data, uint64_t size) : BinNode(_path, data, size){}
       
        /**
         * @param digest - id of bin
         */
        bool add_bin(BinBlock* bin, const char* digest, LRU* cache);
        
        BinBlock* get_bin(const char* digest, LRU* cache);
};  

class BinTree : public BTree{
    public:    
        BinTree(string _path, BinNode* _root=NULL) : BTree( _path, _root){}
        
        bool add_bin( BinBlock* bin);
        
        BinBlock* get_bin(const char* digest);

};
#endif //MNEMOSYNE_INDEX_BININDEX_H
