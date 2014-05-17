#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED

#include <list>
#include <vector>
#include <stdint.h>
#include <fstream>

#include "Manager.h"
#include "Block.h"

using namespace std;

namespace Athena{
    namespace Mnemosyne{

        class Chunk{
            protected :
                uint64_t id;
                uint64_t block_id;
                uint32_t size;
            public :
                static const uint32_t CHUNK_SIZE_MAX = 64*1024; //octect

                Chunk();
                Chunk( uint64_t id, uint64_t block_id);
                Chunk( uint64_t id, uint64_t block_id, uint32_t size);

                uint64_t getId(){ return id; }
                uint64_t getBlock_id(){ return block_id; }
                uint32_t getSize(){ return size; }

                void setId( uint64_t i){ id=i; }
                void setBlock_id( uint64_t i){ block_id=i; }
                void setSize( uint64_t i){ size=i; }
        };

        class ChunkManager : public Manager{
            public :
                ChunkManager();
                ~ChunkManager();

                void insert( Chunk chunk);
                void insert( vector< Chunk > chunks );
                vector<Chunk> get( string fieldsNeeded, string where, string order, string limit);

                /**
                 * Order by id
                **/
                vector<Chunk> get(vector<uint64_t>& ids);
                Chunk get(uint64_t id);

        };

        class ChunkHandler {
            protected :
                list< string > files; ///All the tmpFiles which was created by this instance
            public :
                ChunkHandler();
                ~ChunkHandler();

                string getFile( Chunk& chunk );
        };
    }
}

#endif // CHUNK_H_INCLUDED
