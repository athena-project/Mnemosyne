#ifndef MNEMOSYNE_INDEX_BININDEX_H
#define MNEMOSYNE_INDEX_BININDEX_H

#include <list>
#include <cstddef>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <unordered_map>

#include <boost/filesystem.hpp>

#include "../utility/hash.cpp"
#include "DynamicIndex.h"

#define RATIO_INSERTION 0.05 //5% pour du tri par insertion plus, on fait du tri nlogn

class BinBlock : public Block{
    /**
     * For complexity, n is the number of digests stored
     * be aware that n is not bounded (probabilistic bounded only)
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
        BinBlock(const char* _path) : name( BinBlock::alpha_id++ ){
            path = _path;
            init();
        }
        
        /**
         * Reconstruct data from file
         */
        BinBlock(const char* _path, uint64_t _name) : name( _name){
            BinBlock::alpha_id = max( Block::alpha_id, _name+1); 
            path = _path;
            init();
            
            /// Recovery part
            load( true );
            clean();
        }
        
        BinBlock(const char* path, const char* data, uint64_t _size);
        
        /**
         * Add k digests to  current bin
         * @param digests - digests to add
         * @param lenght - number of digests to add 
         */
        bool add(const char** digests, size_t length){

};


#endif //MNEMOSYNE_INDEX_BININDEX_H
