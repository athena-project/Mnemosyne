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
    public:    
        BinBlock(const char* _path) : Block(_path){
        }
        
        /**
         * Reconstruct data from file
         */
        BinBlock(const char* _path, uint64_t _name) : Block(_path, _name){
        }
        
        BinBlock(const char* path, const char* data, uint64_t _size);
        
        void set_id(const char* _id){ memcpy(id, _id, DIGEST_LENGTH); }
        
        bool load(bool degraded_mod);
        
        void merge(BinBlock* right);
};

class BinNode : public BNode{
    /**
     * For complexity, m is the number of blocks stored or of children
     * so m is in [d, 2*d] and n is the number of digest stored and n' number of blocks
     */
    public:
        BinNode(const char* _path) : BNode(_path){ 
            leaf = true;
        }
        
        BinNode(const char* _path, BinNode* left, BinNode* right) : BinNode(_path, left, right){}
        
        BinNode(const char* _path, BinNode** data, uint64_t size) : BinNode(_path, data, size){}
        
        BinNode(const char* _path, BinBlock** data, uint64_t size) : BinNode(_path, data, size){}
       
        /**
         * @param digest - id of bin
         */
        bool add_bin(BinBlock* bin, const char* digest, LRU* cache);
        
        BinBlock* get_bin(const char* digest, LRU* cache);
};  

class BinTree : public BTree{
    public:    
        BinTree(string _path, BinNode* _root=NULL){
            path =_path ;
            fs_path = fs::path( path);  
            
            if( !fs::exists(fs_path) || !fs::is_directory(fs_path))
                throw "Can not create BTree : invalid path ";
    
            root = ( _root == NULL ) ? new BinNode(path.c_str()) : _root;

            cache = new LRU( CACHE_SIZE );
        }
        
        bool add_bin( BinBlock* bin);
        
        BinBlock* get_bin(const char* digest);

};
#endif //MNEMOSYNE_INDEX_BININDEX_H
