#ifndef BLOCK_H_INCLUDED
#define BLOCK_H_INCLUDED

#include <stdint.h>
#include <stdio.h> // remove file
#include <mysql++/mysql++.h>
#include <vector>

#include <boost/filesystem.hpp>

#include "Manager.h"
#include "Chunk.h"

using namespace std;

namespace Athena{
    namespace Mnemosyne{

        class Block{
            protected :
                uint64_t id;
            public :
                static const uint32_t CHUNKS = 2000; //2000 chuncks by block

                Block();
                Block( uint64_t id){ this->id = id; }

                uint64_t getId(){ return id; }
                void setId(uint64_t i){ id=i; }
        };

        class BlockManager : public Manager{
            protected :

            public :
                BlockManager();
                ~BlockManager();

                void insert( Block block);
                void insert( vector< Block > block );
                vector<Block> get( string fieldsNeeded, string where, string order, string limit );

                uint64_t count( string where="", string order="", string limit="" );

                /**
                 * Order by id
                **/
                vector<Block> get( vector<uint64_t>& ids );
                Block get( uint64_t id );
        };

        class BlockHandler{
            protected :
                vector<string> files;
            public :
                BlockHandler();
                ~BlockHandler();

                static string DIR(){ return ""; }
                static string TMP_DIR(){ return ""; }

                string getChunk( Block& block, uint64_t idChunk );

                void makeBlocks();//Build blocks
        };
    }
}

#endif // BLOCK_H_INCLUDED
