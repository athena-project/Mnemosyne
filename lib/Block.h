#ifndef BLOCK_H_INCLUDED
#define BLOCK_H_INCLUDED

#include <stdint.h>
#include <mysql++/mysql++.h>
#include <vector>

#include "Manager.h"

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

                /**
                 * Order by id
                **/
                vector<Block> get( vector<uint64_t>& ids );
                Block get( uint64_t id );
        };
    }
}

#endif // BLOCK_H_INCLUDED
